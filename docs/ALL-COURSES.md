# All five courses

The development build includes all five course sequences recovered from BMX Racers. Select **Choose Course** from the main menu to practice any course. After a finish, **Continue to Course N** opens the next course and starts the ride; **Ride Again** retries the current course. The fifth course has a final completion message.

Continuing between courses accumulates the score in the HUD. Direct course selection begins a new score. Best scores are retained per course and pace, with separate overall records for continued rides. Existing Course One records migrate from the 1.0 save format.

| Course | Original screen sequence offset | Rows to native finish | Native scrolling delay at start |
|---|---:|---:|---:|
| 1 | 0 | 210 | 64 |
| 2 | 8 | 250 | 56 |
| 3 | 18 | 375 | 48 |
| 4 | 33 | 500 | 40 |
| 5 | 53 | 625 | 32 |

The extraction walks the original row loader at $C412 and the finish routine at $5500. Each later snapshot retains the post-finish map, loader pointer, course counter and movement settings. The six score digits are cleared for independent course tests; the presentation carries the preceding score during continuous play. Original collision masks and joystick bounds remain authoritative.

Each course has its own terrain mask, source RAM/I/O, smoothed road/water/bank meshes, meadow heights, scenery and finish panels. The established rider customization, camera controls, pace settings, environmental effects and guitar arrangement apply to all five. Original PAL timing includes the native increase in scrolling speed between courses, in addition to the chosen pace multiplier.

`tools/export_all_courses.py` regenerates the four later courses from the original data. `tools/test-core.cmd` includes independent completion/retry checks for all five. `tools/verify-all-courses.ps1` runs the packaged game through all five courses and the actual level transitions. Add `-UHD` for 4K rendering.

Developer replay inputs use -1/0/1 for left/neutral/right, 2/3 for down/up, and 4/5/6/7 for left-down/right-down/left-up/right-up. These are ordinary joystick inputs; replay verification never suppresses collision.
