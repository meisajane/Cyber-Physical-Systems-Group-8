# SANDPRINT



<img width="3188" height="2391" alt="IMG_9302" src="https://github.com/user-attachments/assets/de278af9-057a-44c7-8a2b-ae63183694d2" />



Group 8, Cyber Physical Systems Submission

Qiaoyun Fang, Zhuxuan Ji, Meisa Jane Siridumrongphun

# Overview

Project Sandprint explores a social-tactile interaction with technology with a drawing experienced shaped by the human touch. Inspired by lovers' names written in sand, prehistoric hand stencils and the transience of a hand shake.

The sculpure measures 400mm x 400mm x 183mm and is constructed simply with frosted clear Perspex to draw all attention to the pattern drawn on the black sand. The project mechanically implements a CoreXY plotter for accurate drawings using coordinates and is simple to build.

# Bill of Materials

Everything needed to build and run Project Sandprint: a CoreXY magnet plotter based on [ZenXY v3 by V1 Engineering](https://docs.v1e.com/zenxy/) (CC BY-SA 4.0), adapted to the course kit's 28BYJ-48 motors and an ESP32, with six pressure sensors as input.

Quantities are what the finished machine uses. Spares are listed only where we kept them on purpose.

**Source key**

| Tag | Meaning |
|---|---|
| Kit | Course electronics kit (CPS build resources) |
| ACE | Collected from the ACE workshop |
| Bought | Purchased for this project |
| Printed | 3D-printed by us (files in `1_official_print/` and `2_custom_print/`) |

## Contents

1. [Summary](#1-summary)
2. [Electronics](#2-electronics)
3. [Input sensing](#3-input-sensing)
4. [Motion system](#4-motion-system)
5. [3D-printed parts](#5-3d-printed-parts)
6. [Fasteners](#6-fasteners)
7. [Drawing surface](#7-drawing-surface)
8. [Consumables](#8-consumables)
9. [Tools](#9-tools)
10. [Power budget](#10-power-budget)
11. [Pin map](#11-pin-map)
12. [Design decisions](#12-design-decisions)
13. [Sources and licences](#13-sources-and-licences)

## 1. Summary

| Subsystem | Main parts | Count |
|---|---|---|
| Control | ESP32 DevKit (ESP32-WROOM-32D) | 1 |
| Actuation | 28BYJ-48 stepper + ULN2003 driver board | 2 sets |
| Sensing | FSR pressure sensors, optical endstops | 6 + 2 |
| Frame | 2020 V-slot extrusion | 3 lengths |
| Motion | Mini V wheels, GT2 idlers, GT2 belt | 10, 8, 1 belt |
| Printed | Official ZenXY parts + custom adapters | 16 part types |
| Power | 5 V 2 A power bank | 1 |

## 2. Electronics

| Component | Qty | Source | Role | Why this part |
|---|---|---|---|---|
| ESP32 DevKit V1 (ESP32-WROOM-32D) | 1 | Kit | Runs the single Arduino sketch: serial commands, CoreXY maths, stepping, homing, sensor sampling | Enough GPIO for 8 motor pins, 2 endstops and 6 analogue inputs on one board, so the whole system needs only one sketch |
| 28BYJ-48 stepper motor, 5 V | 2 | Kit | Motor A and motor B of the CoreXY | Runs directly from the 5 V power bank; 4076 half-steps per turn gives 0.008 mm per step through a 16T pulley |
| ULN2003 driver board | 2 | Kit | Switches about 100 mA per coil; the ESP32 pins can only supply a few mA | Matches the 28BYJ-48 and its 5-pin plug |
| Optical endstop (slot type, 1 m lead) | 2 | Kit | X and Y homing at the front-left corner | Contact-free, so the belt-piece X trigger can pass through without wear |
| 5 V 2 A power bank | 1 | Kit | Powers both ULN2003 boards | Required by the brief |
| USB breakout to pin header | 1 | ACE | Takes 5 V and GND from the power bank to the breadboard rail | Lets the power bank feed the motors without cutting a cable |
| Breadboard, full size | 1 | ACE | Power rails and sensor dividers | No soldering, quick to rewire |
| Jumper wires, male-male and female-female | about 30 | Kit | ESP32 to drivers, endstops and sensors | |
| USB data cable | 1 | Have | Laptop to ESP32 for upload and serial | Must carry data, not only charge |

## 3. Input sensing

| Component | Qty | Source | Role | Why this part |
|---|---|---|---|---|
| Force-sensitive resistor (FSR) | 6 | Kit | Three per person; the press is the interaction | Resistance falls as force rises, read as a voltage by the ESP32 ADC |
| 10 kΩ resistor | 6 | Kit | Pull-down in each FSR voltage divider | Gives a useful range for finger pressure on the 0 to 3.3 V ADC |
| BOOT push button | 1 | On the ESP32 board | Start button pressed by a third person | Already on the board (GPIO 0), no extra wiring |

Each FSR is wired as a divider: 3V3 to FSR, FSR to the GPIO, GPIO through 10 kΩ to GND. Unpressed reads near 0; a firm press reads near 4095.

## 4. Motion system

| Component | Qty | Source | Notes |
|---|---|---|---|
| 2020 V-slot extrusion, Y rails | 2 × 313 mm | ACE, cut | ZenXY formula: table + 43 mm. Both must be the same length to keep the gantry square |
| 2020 V-slot extrusion, X beam | 1 × 285 mm | ACE, cut | ZenXY formula: table + 15 mm |
| Mini V wheel with 625 bearing (Befenybay, Ø15.3 mm) | 10 | Bought | 3 per Y block, 4 on the Core |
| GT2 idler, 16 teeth, 3 mm bore | 8 | Bought | Toothed on all positions; the smooth-idler positions run on the belt's back face |
| GT2 belt, 6 mm, open | about 2.7 m | Have | One continuous belt with two half twists at the rear crossing, plus a 40 mm offcut as the X trigger |
| GT2 16T pulley | 2 | Printed | See section 5 |

Drawing area after assembly: 233 × 236 mm, stored on the ESP32 with the `L 233 236` command.

## 5. 3D-printed parts

All parts printed in PLA, 3 walls, 20% infill, no supports unless noted.

### Official ZenXY v3 parts (`1_official_print/`)

| File | Qty | Function |
|---|---|---|
| `core.stl` | 1 | Magnet carriage; clamps both belt ends and the fixed belt section |
| `cornermin.stl`, `cornermax.stl` | 1 each | Rear corners; each holds two idlers |
| `motormin.stl`, `motormax.stl` | 1 each | Front corners; motor mount, idlers, endstops on MotorMin |
| `y_block_min.stl`, `y_block_max.stl` | 1 each | Carry the X beam along the Y rails |
| `y-trigger.stl` | 1 | Blocks the Y endstop at the front |
| `silent-belt.stl` | 1 | Separates the two belt runs where they cross (optional) |

### Custom parts (`2_custom_print/`)

| File | Qty | Why we designed it |
|---|---|---|
| `V_wheel_plate_3wheel_Yblock_x2.stl` | 2 | Replaces the V1 50 mm wheel plate, which could not be shipped in time; counterbored for M5 heads, hex pockets for nuts |
| `V_wheel_plate_4wheel_Core_x1.stl` | 1 | Same interface for the Core, with four wheels for stiffness |
| `Wheel_spacer_5x8x7.4_x10.stl` | 10 | Lifts each wheel so its V groove meets the rail edge; 8 mm OD touches only the bearing inner race |
| `Adapter_leg_28BYJ_left.stl`, `_right.stl` | 1 each | Mounts the 28BYJ-48 under the NEMA 17 motor mounts and sets the frame height (139 mm) |
| `Rear_leg_left.stl`, `Rear_leg_right.stl` | 1 each | Rear support legs, same height |
| `GT2_16T_long_hub_pulley_x2.stl` | 2 | 16T pulley with a long hub and M3 set screw for the 28BYJ-48 Ø5 flatted shaft |
| `Silent_belt_wall_spacer.stl` | 1 | Spaces the belt silencer off the enclosure wall |

## 6. Fasteners

| Part | Used | Source | Where |
|---|---|---|---|
| M5 × 25 pan or button head | 17 | Bought | Wheel axles 10, wheel plates to blocks 6, magnet height screw 1 |
| M5 nut | 16 | Bought | Wheel axles 10, plate hex pockets 6 |
| M5 × 10 | 4 | Bought | Y block tabs into T-nuts to lock the X beam |
| M5 hammer T-nut | 4 | Have | 2 per X beam end |
| M5 flat washer, 1 mm steel | as needed | ACE | Wheel spacer shims |
| M3 × 20 | 32 | Have | Idlers, motor mounts, endstops |
| M3 × 6 pan head | 4 | Have | 28BYJ-48 ears into the adapter legs |
| M3 nut | 16 | ACE | Behind idlers and endstops |
| Cable ties, small | 2 | Have | Belt fold lock and belt tensioner |

## 7. Drawing surface

| Component | Qty | Source | Notes |
|---|---|---|---|
| Acrylic base plate | 1 | [confirm size and thickness] | Sand sits on top; the magnet runs underneath |
| Neodymium magnet Ø12 × 6 mm | 2, stacked | ACE | Sits in the Core pocket; the M5 screw underneath sets the gap to the plate |
| Steel ball Ø10 to 13 mm | 1 (+1 spare) | ACE | Larger balls follow better through coarse sand |
| Fine sand or baking soda | about 500 g | Bought | 2 to 3 mm layer; finer grain gives crisper lines |
| Enclosure box, 400 × 400 mm inner | 1 | [confirm] | Covers the mechanism; carries no load |

## 8. Consumables

| Item | Use |
|---|---|
| VHB double-sided tape, 1 mm | Rear legs to corner blocks, legs to the enclosure floor |
| Medium-strength threadlocker (or nail varnish) | Wheel axle nuts after adjustment |
| Masking tape and marker | Labelling left and right parts |
| Heat-shrink or electrical tape | Wire joints |

## 9. Tools

Calipers, 2.5 mm and 4 mm hex keys, PH1 and PH2 screwdrivers, file, Ø3 and Ø5.2 drill bits (to clear printed holes by hand), flush cutters, steel rule, saw for the extrusions.

## 10. Power budget

Estimates from component ratings, not measured.

| Load | Current at 5 V | Notes |
|---|---|---|
| 28BYJ-48 × 2, half-step | up to 400 mA | About 100 mA per energised coil, at most 2 coils per motor at once |
| ESP32, Wi-Fi off | about 80 mA | From USB, not the power bank |
| Optical endstops × 2 | about 40 mA | From 3V3 |
| FSR dividers × 6 | under 2 mA | 3.3 V across 10 kΩ at full press |
| ULN2003 indicator LEDs | about 10 mA | |
| **Total** | **about 0.5 A peak** | Well inside the 2 A power bank |

Coils switch off after 300 ms idle, so the idle draw is close to zero. Some power banks turn off at very low current; press the power bank button to wake it before a demo.

## 11. Pin map

| ESP32 pin | Connects to |
|---|---|
| RX2 (GPIO 16), TX2 (17), D18, D19 | ULN2003 A, IN1 to IN4 |
| D21, D22, D23, D25 | ULN2003 B, IN1 to IN4 |
| D32 | X endstop signal |
| D33 | Y endstop signal |
| D13, D34, D14 | Person A FSRs |
| D27, D26, D35 | Person B FSRs |
| BOOT (GPIO 0) | Start button, on board |
| 3V3 | Endstop VCC, FSR high side |
| GND | Common ground with the power bank |

ULN2003 `+` and `−` connect to the power bank 5 V and GND. GPIO 12 is left unused because it is a boot strapping pin.

## 12. Design decisions

- **28BYJ-48 instead of NEMA 17.** NEMA 17 needs 12 to 24 V, which the brief does not allow. The 28BYJ-48 runs from 5 V; the cost is low torque, handled by capping each motor at 400 half-steps per second.
- **Custom wheel plates and spacers.** The V1 wheel plates could not arrive in time. Our plate keeps the same 2 × M5 @ 20 mm interface, so the official blocks need no changes.
- **Toothed idlers everywhere.** One part type to buy; on the smooth-back positions they behave like smooth idlers.
- **Optical endstops.** No contact wear, and the X trigger is a belt offcut that passes through the slot.
- **FSRs with 10 kΩ dividers.** The simplest analogue input in the kit; one resistor value for all six keeps wiring and documentation consistent, and Wekinator absorbs the sensor-to-sensor variation through training.

## 13. Sources and licences

- ZenXY v3, V1 Engineering, CC BY-SA 4.0: https://docs.v1e.com/zenxy/
- AccelStepper library, Mike McCauley
- oscP5 library, Andreas Schlegel
- Wekinator, Rebecca Fiebrink: http://www.wekinator.org/

## Software Design

Software workflow overview

```
ESP32 sketch            Processing sketch                Wekinator
(zenxy_esp32.ino)       (zenxy_draw.pde)
6 FSRs ── PRESS a b ──► forward [a, b] ── OSC 6448 ──► k-NN classifier
                                                        │
motors ◄── M x y ◄──── pattern 1 to 5 ◄── OSC 12000 ────┘
```

### `zenxy_esp32.ino`

The code in this file was written mostly by Anthropic's Claude (Claude Opus 5.5 and Claude Fable 5.1). The pressure-recording routine is adapted from code written by Qiaoyun Fang.

- **Serial command protocol:** Reads one text command per line at 115200 baud (`M x y`, `H`, `F`, `P`, `?`, `S`) and replies `ok` when each one finishes, so Processing never overfills the serial buffer.
- **CoreXY kinematics:** Converts each target point in millimetres into absolute step counts for the two motors (A = (x + y) × 127.4, B = (x − y) × 127.4). Absolute targets stop rounding errors from building up over thousands of segments.
- **Straight-line motion:** Uses AccelStepper's MultiStepper so both motors start and finish each segment together. Each motor is capped at 400 half-steps per second because a 45-degree move loads a single 28BYJ-48 and makes it skip at higher rates.
- **Homing:** Seeks the Y endstop first, then the X endstop (the X trigger only reaches its sensor when Y is at its minimum), then backs off slowly along X and then Y to avoid a full-speed reverse start.
- **Drawing area:** The size is stored in the ESP32's flash with `L 233 236`, so the same sketch works after the frame is resized.
- **Pressure recording:** On the BOOT button (or `P`), samples all six FSRs at 10 Hz for 5 s, takes each sensor's median to reject spikes from shaking hands, averages each person's three medians and sends `PRESS a b`. The button is ignored while the machine is moving.
- **Power saving:** Switches both motors' coils off after 300 ms without motion so the motors do not overheat the printed mounts.

### `zenxy_draw.pde`

The code in this file was written mostly by Anthropic's Claude (Claude Opus 5.5 and Claude Fable 5.1).

- **Start-up:** Opens the ESP32's serial port, homes the table, reads the stored drawing size and sets the drawing speed to 2.5 mm/s.
- **OSC bridge:** Forwards each `PRESS a b` from the ESP32 to Wekinator (port 6448, `/wek/inputs`) and listens for the class it returns (port 12000, `/wek/outputs`).
- **Pattern generation:** Turns the class into one of five parametric curves, centred and scaled to the drawing area. The average pressure of the two people sets each pattern's detail (number of turns, petals or rings).
- **Path planning:** Splits each curve into segments of at most 1.5 mm and sends them one at a time, waiting for `ok`. Travels to each pattern's start along X first, then Y, because a diagonal travel move loads a single motor.
- **Training mode:** Press `t` to record Wekinator examples without the machine drawing.
- **Visualisation:** Shows the planned path in grey, the drawn part in purple, the latest A and B readings and the last Wekinator class.

### Instructions for how to run (macOS)

1. Upload `zenxy_esp32.ino` to the ESP32 (board: ESP32 Dev Module). Close the Arduino Serial Monitor afterwards.
2. Wake the 5 V power bank that powers the motors.
3. Open the Wekinator project and press **Run**.
4. Run `zenxy_draw.pde` in Processing (needs the oscP5 library) and wait for `state: idle`.
5. Two people press their sensors; a third person presses the ESP32's BOOT button. The machine draws the matching pattern.

## Machine Learning

Wekinator dashboard

The system uses 2 input features: the average pressure of person A and of person B.

### Wekinator Settings

| Setting | Value |
|---|---|
| Inputs | 2, OSC port 6448, `/wek/inputs` |
| Outputs | 1 classifier, 5 classes, OSC port 12000, `/wek/outputs` |
| Model | k-Nearest Neighbour [confirm k] |
| Training examples | 40 (8 per class) |

### Feature Design and Preprocessing

Each person presses three FSRs for 5 s. The ESP32 takes the median of each sensor's 50 samples, which removes spikes from hand tremor, then averages the three medians into one value per person (raw ADC, 0 to 4095). Two values describe both how hard the pair pressed and how balanced they were, which is what separates the five classes. All six sensors use the same 10 kΩ divider; sensor-to-sensor variation is absorbed by training on the real hardware.

### Gesture and Model Behaviour

The classes describe the relationship between two people rather than one person's pose. Two of them (4 and 5) depend on who pressed harder, so a single combined average could not separate them. The class boundaries are diagonal in the A–B plane, which is why a learned classifier fits better than fixed thresholds.

| Class | How the two people press | Pattern | Detail set by average pressure |
|---|---|---|---|
| 1 | Both light | Spiral | 6 to 14 turns |
| 2 | Both medium | Rose | 3 to 7 petals |
| 3 | Both firm | Rings | 4 to 10 rings |
| 4 | A firm, B light | Lissajous | Frequency ratio 3:2 to 5:4 |
| 5 | A light, B firm | Spirograph | Inner gear size |

Pattern images are in `patterns/`.

### Evaluation

[Fill in: 3 or 4 people who did not record training data each try every class twice. Record intended class vs Wekinator class in a 5 × 5 table and report accuracy. Repeat with a decision tree to compare models.]

### Improvements with more resources

- Record training data from many more pairs so the model learns a wider range of hand strengths.
- Add a per-person calibration press (maximum force) to normalise strong and weak users.
- Add time-based features, such as the correlation between the two pressure curves, to measure how synchronised the pair is, not only how hard they press.

**Limitations:** With only two averaged inputs, presses with the same averages but different timing are indistinguishable, and "medium" sits close to both "light" and "firm", so class 2 is the most likely to be confused.

## Libraries and tools

- AccelStepper and MultiStepper, Mike McCauley
- Preferences (ESP32 Arduino core), Espressif
- oscP5 and netP5, Andreas Schlegel
- Processing Serial library
- Wekinator, Rebecca Fiebrink
