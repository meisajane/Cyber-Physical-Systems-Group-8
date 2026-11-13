/*
  ZenXY sand table controller
  Board : ESP32 DevKit V1 (ESP32-WROOM-32D), Arduino IDE board "ESP32 Dev Module"
  Motors: 2x 28BYJ-48 (5 V, unipolar) through 2x ULN2003, CoreXY kinematics
  Input : one line per command over USB serial, 115200 baud, from Processing
  Sensor: 6 FSRs (3 per person) and a start button pressed by a third person

  Pressure recording (median per sensor, 50 samples over 5 s) is based on code by
  [teammate name], merged into this single sketch.
  Library: AccelStepper by Mike McCauley (Arduino Library Manager)
  Mechanism: ZenXY v3 by V1 Engineering (CC BY-SA 4.0), adapted for 28BYJ-48

  Serial commands (each ends with a newline):
    M x y    move in a straight line to (x, y) in mm, replies "ok" when it arrives
    F v      set drawing speed in mm/s (default 5)
    H        home: Y to its endstop, then X to its endstop, replies "ok"
    Z        set the current position as (0, 0) without homing
    ?        report position, speed and endstop states
    R a n    raw test: move motor a (A or B) by n steps, replies "ok"
    S        stop and switch both motors off
    J dx dy  jog by (dx, dy) mm, ignoring the saved size (for calibration), replies "ok"
    L        save the current position as the far corner (X_MAX, Y_MAX) in flash, replies "ok"
    L x y    save (x, y) in mm as the far corner directly, e.g. L 233 236
    P        record the two people for 5 s (same as the button), replies "PRESS a b"
             a, b = mean of each person's three sensor medians, raw ADC 0 to 4095
             Calibrate once after the frame changes: H, then J until the Core is ~2 mm
             from the far corner, then L.
*/

#include <AccelStepper.h>
#include <MultiStepper.h>
#include <Preferences.h>

// ---------- pins (ESP32 GPIO numbers, printed as Dxx on the board) ----------
// AccelStepper HALF4WIRE wants the 28BYJ coils in the order IN1, IN3, IN2, IN4.
const int A_IN1 = 16, A_IN2 = 17, A_IN3 = 18, A_IN4 = 19;   // ULN2003 board A
const int B_IN1 = 21, B_IN2 = 22, B_IN3 = 23, B_IN4 = 25;   // ULN2003 board B
const int ENDSTOP_X = 32;                                   // optical endstop signal
const int ENDSTOP_Y = 33;
const int ENDSTOP_TRIGGERED = HIGH;   // most optical endstops output HIGH when the slot is blocked

// Pressure sensors: FSR from 3V3 to the pin, 10 kOhm from the pin to GND.
// GPIO 12 is avoided: it is a boot strapping pin and a pressed FSR at reset stops the ESP32 booting.
// ADC2 pins (13, 14, 26, 27) read normally because this sketch never turns Wi-Fi on.
const int FSR_A[3] = {13, 34, 14};    // person A
const int FSR_B[3] = {27, 26, 35};    // person B
const int BUTTON = 0;                 // the board's BOOT button (GPIO 0 to GND when pressed). Do not hold it while the board resets.
const int SAMPLES = 50;               // 50 samples x 100 ms = 5 s
const unsigned long BUTTON_IDLE_MS = 1500;   // ignore the button until the machine has been still this long

// ---------- machine constants ----------
// 28BYJ-48: 64 half-steps per rotor turn x 63.684 gearbox = 4076 half-steps per output turn.
// GT2 16-tooth pulley moves the belt 16 x 2 mm = 32 mm per output turn.
const float STEPS_PER_MM = 4076.0 / 32.0;     // = 127.4
float X_MAX = 233.0, Y_MAX = 236.0;           // drawing area in mm, loaded from flash after calibration
const float JOG_LIMIT = 600.0;                // never jog beyond this, whatever the frame
const float HOME_SPEED = 4.0;                 // mm/s while seeking an endstop
const float BACKOFF_MM = 3.0;
const unsigned long IDLE_OFF_MS = 300;        // coils off after this long without motion
const float MAX_SPS = 400.0;                  // per-motor half-step rate cap; one motor alone (45-degree moves) skipped at 450 on 10 Nov 2026

// Pin order reversed (IN4, IN2, IN3, IN1) so that +A moves the Core to +X+Y and +B to +X-Y,
// as measured on the real machine on 9 Nov 2026.
AccelStepper motorA(AccelStepper::HALF4WIRE, A_IN4, A_IN2, A_IN3, A_IN1);
AccelStepper motorB(AccelStepper::HALF4WIRE, B_IN4, B_IN2, B_IN3, B_IN1);
MultiStepper both;
Preferences prefs;

float curX = 0, curY = 0;        // current position in mm
float speedMM = 5.0;             // drawing speed in mm/s
unsigned long lastMove = 0;
bool coilsOn = false;
String line;

// ---------- CoreXY: belt A = X + Y, belt B = X - Y ----------
long toStepsA(float x, float y) { return lround((x + y) * STEPS_PER_MM); }
long toStepsB(float x, float y) { return lround((x - y) * STEPS_PER_MM); }

void coils(bool on) {
  if (on && !coilsOn) { motorA.enableOutputs(); motorB.enableOutputs(); }
  if (!on && coilsOn) { motorA.disableOutputs(); motorB.disableOutputs(); }
  coilsOn = on;
}

bool hit(int pin) { return digitalRead(pin) == ENDSTOP_TRIGGERED; }

// Straight line in XY: both motors start and finish together.
void moveRaw(float x, float y);
void moveTo(float x, float y) {
  moveRaw(constrain(x, 0, X_MAX), constrain(y, 0, Y_MAX));
}

void moveRaw(float x, float y) {
  float dist = sqrt(sq(x - curX) + sq(y - curY));
  if (dist < 0.01) return;
  long target[2] = { toStepsA(x, y), toStepsB(x, y) };
  long dA = abs(target[0] - motorA.currentPosition());
  long dB = abs(target[1] - motorB.currentPosition());
  float seconds = dist / speedMM;
  // A 45-degree line puts all the work on one motor (sqrt(2) x faster than an X move), so slow
  // the whole line down until neither motor exceeds MAX_SPS.
  seconds = max(seconds, max(dA, dB) / MAX_SPS);
  // MultiStepper limits the faster motor to its max speed; give each motor the speed it needs.
  motorA.setMaxSpeed(max(1.0f, dA / seconds));
  motorB.setMaxSpeed(max(1.0f, dB / seconds));
  coils(true);
  both.moveTo(target);
  both.runSpeedToPosition();     // blocks until both arrive
  curX = x; curY = y;
  lastMove = millis();
}

// Move one Cartesian axis toward its endstop at constant speed until it triggers.
// X-: A-, B-.  Y-: A-, B+.
bool seek(int pin, int sa, int sb, float maxTravel) {
  float sps = HOME_SPEED * STEPS_PER_MM;
  motorA.setMaxSpeed(sps); motorB.setMaxSpeed(sps);
  motorA.setSpeed(sa * sps); motorB.setSpeed(sb * sps);
  coils(true);
  long limit = lround(maxTravel * STEPS_PER_MM);
  long startA = motorA.currentPosition();
  while (!hit(pin)) {
    motorA.runSpeed(); motorB.runSpeed();
    if (abs(motorA.currentPosition() - startA) > limit) return false;
    yield();
  }
  return true;
}

// ZenXY homes Y first, then X (V1 Engineering docs): the X trigger is a short belt
// piece in the Core that only reaches the X endstop when Y is at its minimum.
void home() {
  // Seek distance is X_MAX/Y_MAX + 20 (the saved size is never larger than the real travel), so a
  // missed trigger rams the wall for a few seconds, not for JOG_LIMIT.
  bool okY = seek(ENDSTOP_Y, -1, +1, Y_MAX + 20);
  if (!okY) { Serial.println("error: Y endstop not found"); coils(false); return; }
  bool okX = seek(ENDSTOP_X, -1, -1, X_MAX + 20);   // pure X move, Y stays at 0
  if (!okX) { Serial.println("error: X endstop not found"); coils(false); return; }
  motorA.setCurrentPosition(0); motorB.setCurrentPosition(0); curX = 0; curY = 0;
  // Back off slowly: the Core may be pressed against the corner and the motors were just
  // running the other way, so a full-speed reverse start makes the 28BYJ-48 skip.
  delay(200);
  float saved = speedMM; speedMM = 1.5;
  moveTo(BACKOFF_MM, 0);
  moveTo(BACKOFF_MM, BACKOFF_MM);
  speedMM = saved;
  motorA.setCurrentPosition(0); motorB.setCurrentPosition(0); curX = 0; curY = 0;
  Serial.println("ok");
}

// Record both people for 5 s, take each sensor's median (robust to spikes from a shaking hand),
// average each person's three medians and report "PRESS a b".
int medianOf(int *v) {
  int t[SAMPLES];
  for (int i = 0; i < SAMPLES; i++) t[i] = v[i];
  for (int a = 0; a < SAMPLES - 1; a++)
    for (int b = a + 1; b < SAMPLES; b++)
      if (t[a] > t[b]) { int x = t[a]; t[a] = t[b]; t[b] = x; }
  return (t[SAMPLES / 2 - 1] + t[SAMPLES / 2]) / 2;
}

void recordPress() {
  static int data[6][SAMPLES];
  Serial.println("RECORDING");
  for (int i = 0; i < SAMPLES; i++) {
    for (int j = 0; j < 3; j++) { data[j][i] = analogRead(FSR_A[j]); data[j + 3][i] = analogRead(FSR_B[j]); }
    delay(100);
  }
  float a = 0, b = 0;
  for (int j = 0; j < 3; j++) { a += medianOf(data[j]); b += medianOf(data[j + 3]); }
  a /= 3; b /= 3;
  Serial.printf("PRESS %.0f %.0f\n", a, b);
}

void report() {
  Serial.printf("pos %.2f %.2f size %.1f %.1f speed %.1f endX %s endY %s coils %s\n",
                curX, curY, X_MAX, Y_MAX, speedMM,
                hit(ENDSTOP_X) ? "HIT" : "open", hit(ENDSTOP_Y) ? "HIT" : "open",
                coilsOn ? "on" : "off");
}

void handle(String cmd) {
  cmd.trim();
  if (cmd.length() == 0) return;
  char c = toupper(cmd[0]);
  String rest = cmd.substring(1); rest.trim();
  if (c == 'M') {
    int sp = rest.indexOf(' ');
    if (sp < 0) { Serial.println("error: use M x y"); return; }
    moveTo(rest.substring(0, sp).toFloat(), rest.substring(sp + 1).toFloat());
    Serial.println("ok");
  } else if (c == 'F') {
    float v = rest.toFloat();
    if (v > 0.5 && v < 20) { speedMM = v; Serial.println("ok"); } else Serial.println("error: speed 0.5 to 20 mm/s");
  } else if (c == 'H') {
    home();
  } else if (c == 'Z') {
    motorA.setCurrentPosition(0); motorB.setCurrentPosition(0); curX = 0; curY = 0; Serial.println("ok");
  } else if (c == '?') {
    report();
  } else if (c == 'R') {
    char m = toupper(rest[0]);
    long n = rest.substring(1).toInt();
    AccelStepper &s = (m == 'B') ? motorB : motorA;
    s.setMaxSpeed(600); s.setAcceleration(400);
    coils(true);
    s.move(n); s.runToPosition();
    lastMove = millis();
    Serial.println("ok");
  } else if (c == 'J') {
    int sp = rest.indexOf(' ');
    if (sp < 0) { Serial.println("error: use J dx dy"); return; }
    float nx = constrain(curX + rest.substring(0, sp).toFloat(), 0, JOG_LIMIT);
    float ny = constrain(curY + rest.substring(sp + 1).toFloat(), 0, JOG_LIMIT);
    moveRaw(nx, ny);
    Serial.printf("ok %.2f %.2f\n", curX, curY);
  } else if (c == 'L') {
    int sp = rest.indexOf(' ');
    if (sp > 0) {                      // L x y : set the size directly
      float nx = rest.substring(0, sp).toFloat(), ny = rest.substring(sp + 1).toFloat();
      if (nx < 50 || ny < 50 || nx > JOG_LIMIT || ny > JOG_LIMIT) { Serial.println("error: use L x y in mm"); return; }
      X_MAX = nx; Y_MAX = ny;
    } else {                           // L alone : current position is the far corner
      if (curX < 50 || curY < 50) { Serial.println("error: home and jog to the far corner first"); return; }
      X_MAX = curX; Y_MAX = curY;
    }
    prefs.putFloat("xmax", X_MAX); prefs.putFloat("ymax", Y_MAX);
    Serial.printf("ok size %.1f %.1f saved\n", X_MAX, Y_MAX);
  } else if (c == 'P') {
    recordPress();
  } else if (c == 'S') {
    coils(false); Serial.println("ok");
  } else {
    Serial.println("error: unknown command");
  }
}

void setup() {
  Serial.begin(115200);
  prefs.begin("zenxy", false);
  X_MAX = prefs.getFloat("xmax", X_MAX);
  Y_MAX = prefs.getFloat("ymax", Y_MAX);
  pinMode(ENDSTOP_X, INPUT_PULLUP);
  pinMode(ENDSTOP_Y, INPUT_PULLUP);
  pinMode(BUTTON, INPUT_PULLUP);
  analogReadResolution(12);
  motorA.setMaxSpeed(800); motorB.setMaxSpeed(800);
  both.addStepper(motorA);
  both.addStepper(motorB);
  coilsOn = true; coils(false);
  delay(300);
  Serial.println("ZenXY ready. Send ? for status.");
}

void loop() {
  while (Serial.available()) {
    char ch = Serial.read();
    if (ch == '\n' || ch == '\r') { handle(line); line = ""; }
    else line += ch;
  }
  if (coilsOn && millis() - lastMove > IDLE_OFF_MS) coils(false);
  // The button only counts while the machine is still, so a press during a drawing does nothing.
  if (digitalRead(BUTTON) == LOW && millis() - lastMove > BUTTON_IDLE_MS) {
    delay(30);
    if (digitalRead(BUTTON) == LOW) {
      recordPress();
      while (digitalRead(BUTTON) == LOW) delay(10);
      delay(50);
    }
  }
}
