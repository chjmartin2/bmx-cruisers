# All five courses

Version 1.1 includes all five course sequences recovered from BMX Racers. **Play Game** starts at Course One with one life: a crash ends the run. Only completing all five courses in order on an unchanged pace qualifies for the **High Score Board**. **Practice Courses** allows any selected course and unlimited retries, without high-score qualification. The fifth course has a final completion message.

Continuing between courses accumulates the score in the HUD. Direct course selection begins a new score. Best scores are retained per course and pace, with separate overall records for continued rides. Existing Course One records migrate from the 1.0 save format.

| Course | Original screen sequence offset | Rows to native finish | Native scrolling delay at start |
|---|---:|---:|---:|
| 1 | 0 | 210 | 64 |
| 2 | 8 | 250 | 56 |
| 3 | 18 | 375 | 48 |
| 4 | 33 | 500 | 40 |
| 5 | 53 | 625 | 32 |

The extraction walks the original row loader at $C412 and the finish routine at $5500. Each later snapshot retains the post-finish map, loader pointer, course counter and movement settings. The six score digits are cleared for independent course tests; the presentation carries the preceding score during continuous play. Joystick bounds remain original. Modern contacts follow the displayed rider and visible scenery; -OriginalCollision retains reference pixel-mask checks.

Each course has its own terrain mask, source RAM/I/O, smoothed road/water/bank meshes, meadow heights, scenery and finish panels. The established rider customization, camera controls, pace settings, environmental effects and guitar arrangement apply to all five. Original PAL timing includes the native increase in scrolling speed between courses, in addition to the chosen pace multiplier.

## Original objects restored in 1.3

Version 1.3 restores the swimming-pool and car-park graphics that the 1.2 release incorrectly reduced to generic park surfaces. The retained 1.2 trailer predates these corrections; the 1.3 screenshots show the restored sites.

| Course | Swimming pools | Parking areas | Parked cars | Oil slicks |
|---|---:|---:|---:|---:|
| 1 | 0 | 0 | 0 | 0 |
| 2 | 1 | 0 | 0 | 0 |
| 3 | 1 | 2 | 4 | 8 |
| 4 | 0 | 2 | 4 | 8 |
| 5 | 1 | 3 | 6 | 12 |

Objects are recognized by original character codes, colors and their map context. Blue car bodies must not become water, and red car glass must not become bridge rails. The original RAM, movement, sequence and timing bytes remain unchanged. Pools retain their rectangular tiled surrounds and step notch, with blue water, stainless ladders and metal perimeter fencing. Car parks have paved surfaces, bay lines, colored 3D cars and visible dark oil patches. Natural ponds retain their smooth banks and vegetation.

Car contacts use their displayed rectangular body footprints; oil patches use inset visible boundaries. Every cosmetic ride retains the common rider footprint. `tools/verify-original-sites.ps1` checks contacts, clear passes, pool coping and existing bridge rails in the packaged game on all five courses; `tests/test_course_features.py` checks every original occurrence and the shared-glyph regressions.

`tools/export_all_courses.py` regenerates the four later courses from the original data. `tools/test-core.cmd` includes independent completion/retry checks for all five. `tools/verify-all-courses.ps1` runs the packaged game through all five courses and the actual level transitions. Add `-UHD` for 4K rendering.

Developer replay inputs use -1/0/1 for left/neutral/right, 2/3 for down/up, and 4/5/6/7 for left-down/right-down/left-up/right-up. These are ordinary joystick inputs; replay verification never suppresses collision.
