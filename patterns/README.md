# Patterns

Rendered from the same formulas as `zenxy_draw.pde`, with the detail parameter at its middle value (0.5). On the machine, the two people's average pressure moves this parameter between 0 and 1, so each drawing varies slightly.

`light/` black lines on white (README). `dark/` white lines on a transparent background (dark slides). Each pattern is provided as PNG and SVG. `patterns_overview_light.png` and `patterns_overview_dark.png` show all five in a row.

| Class | How the two people press | Pattern | File | Path length | Drawing time at 2.5 mm/s |
|---|---|---|---|---|---|
| 1 | Both light | Spiral | `pattern_1_spiral` | 3.4 m | about 22 min |
| 2 | Both medium | Rose | `pattern_2_rose` | 2.2 m | about 15 min |
| 3 | Both firm | Rings | `pattern_3_rings` | 2.8 m | about 18 min |
| 4 | A firm, B light | Lissajous | `pattern_4_lissajous` | 2.3 m | about 15 min |
| 5 | A light, B firm | Spirograph | `pattern_5_spirograph` | 12.9 m | about 86 min |

Times are minimums: diagonal segments run slower because of the per-motor speed cap.
