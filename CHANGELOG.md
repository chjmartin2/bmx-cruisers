# Release history

## 1.3.0 — 2026-11-02

- Added Performance (default), Balanced and High graphics presets, independent of gameplay pace and interface resolution. Performance uses up to a 900p scene, fewer decorative details, a smaller streaming pool, no costly dynamic shadows, and a 60-fps cap.
- Replaced the development preview's coarse 720p/FXAA presentation with 900p temporal reconstruction, sharper texture sampling and fuller low-cost bushes. The interface stays at the window's full resolution. Balanced uses up to 1080p; High retains optional 4K.
- Switched the default renderer to DirectX 11 / Shader Model 5. Lumen, ray tracing, virtual shadows and real-time sky capture are no longer prerequisites.
- Replaced large runtime mesh JSON parsing and normal generation with compact validated binary mesh caches. Spatial draw chunks let off-screen terrain stop rendering without changing shorelines, course geometry or water contacts.
- Removed the duplicate scanned shrub layer from the lighter presets. Added inexpensive opaque crowns so every bush remains visible and readable. Reduced tiny prop geometry, background trees, litter, localized audio streams and decorative update rates.
- Reduced the Performance fireworks particle budget while retaining their large, vivid bursts. Rider motion, controls, collision and original PAL simulation remain independent of decorative animation rates.

- Restored all swimming pools in Courses 2, 3 and 5 with straight tiled surrounds, the original step notch, ladders, blue water and metal fencing.
- Restored all parking sections in Courses 3, 4 and 5, including their asphalt, bay markings, red/blue parked cars and oil slicks at the original positions.
- Corrected shared character-code interpretation: blue cars no longer create false water surfaces, and red car graphics no longer create false bridge rails. Cars and oil have visible, consistent modern contact boundaries.
- Kept original course sequences, movement, timing and the common footprint for every ride style.

## 1.2.0 — 2026-11-01

- Restored collision on bridge guardrails and posts, using their displayed thickness and the same rider footprint as other hazards.
- Added a visible crash sequence: impact flash, a short fall and dust burst, or water spray and expanding splash rings. The scene remains visible for 1.8 seconds before result controls appear; the race and score stop immediately.
- Rebuilt all five shorelines with continuous contour smoothing, a clipped meadow mesh and a sloping grass-to-wet-soil bank. Reeds and stones follow the curved shoreline; grass tufts stay clear of the wet edge. Water contacts use the displayed contour.
- Replaced pastel firework blobs with large colored starbursts, white-hot heads, glowing trails, launch streaks and flickering embers.
- Made the complete top status bar solid dark gray above the red line.
- Added Xbox Menu/Start pause and resume, View/Back return to the main menu, and control hints that follow keyboard/controller use. Returning to the main menu ends a one-life run.

## 1.1.0 — 2026-11-01

- Added the four remaining original courses, using the original loader/finish transitions and per-course native scroll timing.
- Added keyboard-accessible course selection, next-course continuation, current-course retry and a final Course Five celebration.
- Separated Play Game from Practice Courses. Play Game has one life and starts at Course One; only five uninterrupted, ordered course finishes qualify for the new per-pace high-score board. Practice allows unrestricted course selection and retries without entering that board. Old practice/retry records cannot qualify retroactively.
- Continuing carries the HUD score forward; direct selection starts a new score. Best scores are stored per course and pace, with overall records and migration of existing saves.
- Extended terrain, shoreline interpolation, fences, grass, wildlife, water effects and terrain-following finish panels to every course.
- Added original-data export tools and collision-enabled completion/transition verification for all five courses.
- Added explicit left analog-stick axes alongside D-pad and keyboard riding.
- Replaced the old sprite collision mask during normal play with swept contacts against the displayed rider and visible scenery. Smooth water boundaries and inset shrub contacts make near-misses more consistent. All cosmetic rides share one footprint; original masks remain available for reference verification.
- Restored missing white boulders, including the misleading dirt patch near the start of Course Three.
- Made the finish celebration deliberately oversized, with 1,440 chunky firework pieces.
- Brightened the park for a sunny day, with higher sunlight and more fill in shadows.
- Removed per-frame rebuilding of hidden fireworks and reused ripple drawing data. Added shader pipeline caching and a short Get Ready interval before the race clock starts.

Normal preserves the original per-course PAL movement and timing; modern visible contacts intentionally replace pixel-mask edge behavior. Selected-course retries are unlimited in Practice. The Windows build remains experimental; animation, materials and hardware coverage have the same beta limitations.

## 1.0.0-beta.1 — 2026-10-31

First public Windows beta of BMX Cruisers. Course One from the C64 BMX Racers is presented in an Unreal Engine 5 park, with the recovered game instructions and collision masks driving the ride.

- Five course paces; Normal uses the original PAL instruction clock. Left/right steering now stays at Normal's real-time rate across every pace.
- Centered, elevated camera with adjustable tilt and distance. Up/down moves the rider within the view.
- Terrain-following fences and finish checkerboards, rippling water, uneven grass, moving foliage and decorative wildlife.
- Boy/girl riders, seven skin tones, casual/extreme clothing, eight helmet and vehicle colors, and six cosmetic ride styles.
- Keyboard-accessible menus, picture/sound settings, credits, saved preferences, per-pace high scores and finish fireworks.
- Electric-guitar arrangement of the recovered melody, percussion and synthesized environmental effects.
- 1080p and 4K scene detail, with a fitted window or single-monitor fullscreen and consistent HUD sizing.

### Checked for this beta

Portable gameplay, steering, motion and display tests pass. Packaged Normal, Cruise and Expert replays complete with collisions enabled and score 210. Normal finish and Expert 4K screenshots confirm both checkerboards sit above the terrain. Appearance checks cover six styles and seven skin tones on both avatars. The release trailer is captured from the executable.

### Known limits

Only Course One is playable. No iOS, macOS, multiplayer or mobile touch build is included. System requirements are a target, not a tested range of hardware. Curved visual shorelines can differ slightly from the original pixel collision boundary. Rider animation and some materials remain beta quality; close garage angles can stress the virtual-shadow budget. The adapter is purpose-built for this game, not a complete cycle-exact C64 emulator.
