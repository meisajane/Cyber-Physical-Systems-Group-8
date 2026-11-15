# Custom 3D-printed parts

Designed by us to adapt ZenXY v3 (V1 Engineering, CC BY-SA 4.0) to the 28BYJ-48 motors and the parts we could source in time. The official ZenXY parts (core, corners, motor mounts, Y blocks, Y trigger, belt silencer) are downloaded from https://docs.v1e.com/zenxy/ and are not included here.

Print settings: PLA, 0.2 mm layers, 3 walls, 20% infill, no supports. Print the pulleys at 0.12 mm layers so the GT2 teeth come out sharp.

| File | Qty to print | Purpose |
|---|---|---|
| `V_wheel_plate_3wheel_Yblock_x2.stl` | 2 | Wheel plate for each Y block; replaces the V1 50 mm plate. Counterbores for M5 heads, hex pockets for M5 nuts |
| `V_wheel_plate_4wheel_Core_x1.stl` | 1 | Wheel plate for the Core, four wheels for stiffness; same 2 × M5 @ 20 mm interface |
| `Wheel_spacer_5x8x7.4_x10.stl` | 10 | One per wheel. Lifts the V groove to the rail edge; 8 mm OD touches only the bearing inner race |
| `Adapter_leg_28BYJ_left.stl` | 1 | Mounts a 28BYJ-48 under MotorMin and sets the frame height (139 mm) |
| `Adapter_leg_28BYJ_right.stl` | 1 | Same for MotorMax |
| `Rear_leg_left.stl` | 1 | Rear support leg, 139 mm frame height; bonded to CornerMin with VHB |
| `Rear_leg_right.stl` | 1 | Same for CornerMax |
| `GT2_16T_long_hub_pulley_x2.stl` | 2 | 16-tooth GT2 pulley with a long hub and an M3 set-screw hole for the 28BYJ-48's Ø5 flatted shaft |
| `Silent_belt_wall_spacer.stl` | 1 | Spaces the belt silencer off the enclosure wall (optional) |

The `_x2`, `_x10` suffix is the number of copies to print.
