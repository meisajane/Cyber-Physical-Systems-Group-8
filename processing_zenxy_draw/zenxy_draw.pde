/*
  ZenXY pattern player
  - On start: homes the table (H), reads the drawing size (?), then waits.
  - Press 1 to 5 to draw a pattern. Press s to stop.
  - Patterns live here on the laptop as formulas. The ESP32 only receives "M x y" lines and
    replies "ok" after each one (handshake), so it never stores a pattern.
  - Sensor loop: a third person presses the button on the table (or p here). The ESP32 records
    both people for 5 s and sends "PRESS a b". This sketch forwards [a, b] to Wekinator
    (OSC port 6448, /wek/inputs). Wekinator's classifier answers with a class 1 to 5
    (OSC port 12000, /wek/outputs), and that class picks the pattern.
  Needs the oscP5 library: Sketch > Import Library > Manage Libraries > oscP5.
  Close the Arduino Serial Monitor before running: only one program can open the port.
*/
import processing.serial.*;
import oscP5.*;
import netP5.*;

OscP5 osc;
NetAddress wekinator;
float pressA = -1, pressB = -1;      // last averages from the ESP32, raw ADC 0 to 4095
int lastClass = 0;                   // last class from Wekinator
boolean training = false;            // t toggles: forward presses to Wekinator but never draw

Serial port;
ArrayList<PVector> pts = new ArrayList<PVector>();
int idx = 0;
String state = "connecting";
int openedAt;
float X_MAX = 233, Y_MAX = 236;      // replaced by the size the ESP32 reports
int current = 0;                     // pattern being drawn, 1 to 5
float param = 0.5;                   // 0 to 1, later set by the bonding score
final String[] NAMES = {"", "Spiral", "Rose", "Rings", "Lissajous", "Spirograph"};
final float STEP_MM = 1.5;           // max segment length sent to the ESP32
final float MARGIN = 10;             // keep this far from the edges, in mm
float lastX = 0, lastY = 0;          // last position sent to the ESP32
float speed = 2.5;                   // drawing speed in mm/s, sent as "F" after homing; [ and ] change it

void setup() {
  size(620, 660);
  String name = null;
  for (String s : Serial.list()) { println(s); if (s.contains("cu.usbserial")) name = s; }   // macOS: use cu.*, not tty.*
  if (name == null) { println("No usbserial port found"); exit(); return; }
  port = new Serial(this, name, 115200);
  port.bufferUntil('\n');
  openedAt = millis();
  osc = new OscP5(this, 12000);                    // listen for Wekinator outputs
  wekinator = new NetAddress("127.0.0.1", 6448);   // send Wekinator inputs
}

// ---------- the five patterns, all centred and scaled to the table ----------
// Each returns a point in mm relative to the centre. t runs 0..1 along the pattern; R is the usable radius.
PVector patternPoint(int p, float t, float R) {
  float a, r;
  switch (p) {
  case 1:  // Archimedean spiral, 6 to 14 turns
    float turns = 6 + 8 * param;
    a = t * turns * TWO_PI; r = R * t;
    return new PVector(r * cos(a), r * sin(a));
  case 2:  // Rose curve r = cos(k a), k = 3 to 7 petals
    int k = 3 + round(4 * param);
    a = t * TWO_PI * (k % 2 == 1 ? 1 : 2);
    r = R * cos(k * a);
    return new PVector(r * cos(a), r * sin(a));
  case 3:  // Concentric rings joined by short radial steps, 4 to 10 rings
    int n = 4 + round(6 * param);
    float u = t * n; int ring = min(n - 1, floor(u)); float f = u - ring;
    r = R * (ring + 1) / n; a = f * TWO_PI;
    return new PVector(r * cos(a), r * sin(a));
  case 4:  // Lissajous figure, frequency ratio 3:2 to 5:4
    int fx = 3 + round(2 * param), fy = fx - 1;
    a = t * TWO_PI;
    return new PVector(R * sin(fx * a + HALF_PI), R * sin(fy * a));
  case 5:  // Spirograph (hypotrochoid)
    float Rb = 1, rb = 0.25 + 0.2 * param, d = 0.6;
    int loops = round(1 / gcd(Rb, rb));
    a = t * TWO_PI * loops;
    float x = (Rb - rb) * cos(a) + d * cos((Rb - rb) / rb * a);
    float y = (Rb - rb) * sin(a) - d * sin((Rb - rb) / rb * a);
    float s = R / (Rb - rb + d);
    return new PVector(x * s, y * s);
  }
  return new PVector(0, 0);
}

float gcd(float a, float b) {      // for the spirograph loop count, on 0.05 steps
  int A = round(a * 20), B = round(b * 20);
  while (B != 0) { int t = A % B; A = B; B = t; }
  return A / 20.0;
}

void buildPattern(int p) {
  pts.clear();
  float cx = X_MAX / 2, cy = Y_MAX / 2, R = min(X_MAX, Y_MAX) / 2 - MARGIN;
  int N = 4000;
  PVector last = null;
  for (int i = 0; i <= N; i++) {
    PVector q = patternPoint(p, i / (float) N, R);
    q.add(cx, cy);
    if (last == null || PVector.dist(q, last) >= STEP_MM || i == N) { pts.add(q); last = q; }
  }
  println(NAMES[p] + ": " + pts.size() + " points, about " + nf(pathLength() / speed / 60, 0, 1) + " min at " + speed + " mm/s");
}

float pathLength() {
  float L = 0; for (int i = 1; i < pts.size(); i++) L += PVector.dist(pts.get(i), pts.get(i - 1)); return L;
}

void startPattern(int p, float prm) {
  if (!state.equals("idle") && !state.equals("done") && !state.equals("stopped")) return;
  current = p; param = constrain(prm, 0, 1);
  buildPattern(p);
  // Travel to the start along X first, then Y: an X or Y move shares the load between both
  // motors, while a 45-degree travel move puts it all on one motor and makes it skip.
  if (pts.size() > 0) pts.add(0, new PVector(pts.get(0).x, lastY));
  idx = 0; state = "drawing"; sendNext();
}

// ---------- screen ----------
final float S = 2.4;
float sx(float x) { return 30 + x * S; }
float sy(float y) { return 610 - y * S; }

void draw() {
  background(245);
  if (state.equals("connecting") && millis() - openedAt > 4000) { send("H"); state = "homing"; }
  noFill(); stroke(200); rect(sx(0), sy(Y_MAX), X_MAX * S, Y_MAX * S);
  stroke(190); beginShape(); for (PVector p : pts) vertex(sx(p.x), sy(p.y)); endShape();
  stroke(83, 74, 183); strokeWeight(2);
  beginShape(); for (int i = 0; i < idx && i < pts.size(); i++) vertex(sx(pts.get(i).x), sy(pts.get(i).y)); endShape();
  strokeWeight(1); fill(30); noStroke();
  text("state: " + state + (current > 0 ? "   pattern " + current + " " + NAMES[current] + "   " + idx + " / " + pts.size() : ""), 30, 22);
  text("keys: 1 Spiral  2 Rose  3 Rings  4 Lissajous  5 Spirograph   p record   t train mode   s stop   [ ] speed " + nf(speed, 0, 1) + " mm/s", 30, 40);
  if (training) { fill(186, 117, 23); text("TRAINING MODE: presses go to Wekinator, the machine will not draw. Press t to leave.", 30, 76); fill(30); }
  String ready = state.equals("idle") || state.equals("done") || state.equals("stopped") ? "Ready: press the button" : "";
  text("A " + (pressA < 0 ? "-" : nf(pressA, 0, 0)) + "   B " + (pressB < 0 ? "-" : nf(pressB, 0, 0)) + "   Wekinator class " + (lastClass == 0 ? "-" : str(lastClass)) + "   " + ready, 30, 58);
}

void keyPressed() {
  if (key >= '1' && key <= '5') startPattern(key - '0', random(1));
  if (key == 's') { send("S"); state = "stopped"; }
  if (key == 'p' && isIdle()) { send("P"); state = "recording"; }
  if (key == 't') { training = !training; println(training ? "TRAINING MODE: presses go to Wekinator, nothing draws" : "RUN MODE"); }
  if ((key == '[' || key == ']') && !state.equals("drawing")) {
    speed = constrain(speed + (key == ']' ? 0.5 : -0.5), 1, 6);
    state = "speed"; send(String.format(java.util.Locale.US, "F %.1f", speed));
  }
}

// ---------- serial ----------
void serialEvent(Serial p) {
  String line = p.readStringUntil('\n');
  if (line == null) return;
  line = trim(line);
  println("<< " + line);
  if (line.startsWith("ZenXY ready") && state.equals("connecting")) { send("H"); state = "homing"; return; }
  if (line.startsWith("error")) { state = line; return; }
  if (line.equals("RECORDING")) { if (isIdle()) state = "recording"; return; }
  if (line.startsWith("PRESS")) {
    String[] t = splitTokens(line);
    if (t.length >= 3) {
      pressA = float(t[1]); pressB = float(t[2]);
      OscMessage m = new OscMessage("/wek/inputs");
      m.add(pressA); m.add(pressB);
      osc.send(m, wekinator);
      println("-> Wekinator " + pressA + " " + pressB);
    }
    if (state.equals("recording")) state = training ? "idle" : "waiting for Wekinator";
    return;
  }
  if (line.startsWith("pos") && state.equals("query")) {
    String[] t = splitTokens(line);
    for (int i = 0; i < t.length - 2; i++) if (t[i].equals("size")) { X_MAX = float(t[i + 1]); Y_MAX = float(t[i + 2]); }
    state = "speed"; send(String.format(java.util.Locale.US, "F %.1f", speed)); return;
  }
  if (line.equals("ok")) {
    if (state.equals("homing")) { state = "query"; send("?"); return; }
    if (state.equals("speed")) { state = "idle"; return; }
    if (state.equals("drawing")) sendNext();
  }
}

void sendNext() {
  if (idx >= pts.size()) { state = "done"; return; }
  PVector q = pts.get(idx++);
  lastX = q.x; lastY = q.y;
  send(String.format(java.util.Locale.US, "M %.2f %.2f", q.x, q.y));
}

void send(String c) { port.write(c + "\n"); println(">> " + c); }

boolean isIdle() { return state.equals("idle") || state.equals("done") || state.equals("stopped"); }

// Wekinator classifier output: one value, the class 1 to 5.
// Only act on it right after a recording, so Wekinator messages during training never start a drawing.
void oscEvent(OscMessage m) {
  if (!m.checkAddrPattern("/wek/outputs")) return;
  int c = round(m.get(0).floatValue());
  if (c < 1 || c > 5) return;
  lastClass = c;
  println("<- Wekinator class " + c);
  if (state.equals("waiting for Wekinator") && !training) {
    state = "idle";
    startPattern(c, constrain((pressA + pressB) / 2 / 4095.0, 0, 1));   // overall force sets the pattern detail
  }
}
