# BMX Cruisers — 1.0 beta / Course One

A native Windows Unreal Engine 5.8.4 game that presents the recovered C64 Course One logic in a 3D park. The title is BMX Cruisers; the original BMX Racers is acknowledged in Credits.

[Download the Windows beta](https://github.com/chjmartin2/bmx-cruisers/releases/tag/v1.0.0-beta.1) · [Project website](https://retrocomputerist.com/projects/bmx-cruisers/) · [Release notes](CHANGELOG.md)

## Play

For the release ZIP, extract everything and run **BMXCruiser.exe**. See [installation and beta limitations](docs/RELEASE-1.0-BETA.md). In a development checkout, open **Play BMX Cruisers.cmd**, or run `Build/Windows/BMXCruiser.exe`. Keep the whole Windows build folder together. The compatibility launcher **Play BMX Cruiser.cmd** forwards to the new name.

The game supports a fitted desktop window and single-monitor borderless fullscreen. F11 toggles between them. Picture detail (1080p or 4K) changes the scene render target without enlarging the desktop window or spanning multiple displays. A shared logical HUD layout preserves text and mouse alignment at different DPI settings and aspect ratios.

## Controls

| Action | Keyboard |
|---|---|
| Camera settings / live preview | C, or Camera View in the pause/settings menus |
| Camera tilt | Page Up / Page Down |
| Camera closer / farther | + / - |
| Restore default camera | Home |
| Menu selection | Up / Down / Tab |
| Adjust highlighted option | Left / Right |
| Confirm / start / retry | Enter / Space |
| Return / pause | Escape |
| Ride | Arrows / WASD |
| Pause | P |
| Restart ride | R |
| Music / effects | M / V |
| 1080p / 4K detail | 1 / 2 |
| Fullscreen | F11 |
| Screenshot | F9 |
| Garage orbit | Hold Q / E |
| Garage zoom | + / - |

Mouse actions and keyboard actions use the same menu routing. The D-pad and controller A/B also support menu navigation. Interactive rides pause when the game loses focus.

## Art and customization

The garage uses angular numbered racing plates, yellow/red highlights, a wire-grid backdrop, condensed italic typography, separate picture/sound settings and a lit rider preview. The main menu, instructions, pause/results and Credits use the same visual language.

Both riders support casual/extreme outfits, eight helmet/vehicle colors and seven independently saved skin tones. The six styles are cruiser, BMX, scooter, inline skates, quad skates and wheelchair. These choices affect rendering only. Speed, collision, score and course flow never read appearance settings.

The fitted head has a continuous tapered neck into the shirt collar. Full Extreme uses a full-face helmet with ear coverage, cheek padding, a molded chin guard, intake grille, separate visor, vents and retention hardware. Helmet color remains selectable.

The inline and quad skates now use authored material-separated boots, cuffs, liners, laces/stitches, alloy frames or trucks, polyurethane wheels, bearings/axles and toe stops. The scooter uses a shaped 53-cm deck, grip surface, neck/headset, fork, double clamp, grips, 110-mm wheels and rear brake. Helmets and protective guards are authored meshes as well. The skate assemblies follow the posed feet.

The skinned bodies are refined MIT Rocketbox derivatives (29,760 male / 34,928 female triangles). The detailed head is a 61,978-triangle neutral MetaHuman template bake with actual eye geometry, generated UV-aligned male/female skin atlases, pore normals and independent skin-tone materials. It is not a complete facial-animation MetaHuman rig. The packaged game uses standard static meshes and materials without creator services or a facial-animation plugin dependency.

The trail combines rotated/macro-blended scan layers with unique per-course cracks, repairs, drifting wear lines and dusty verges. These visual overlays preserve the original trail and obstacle footprints. The park also includes smooth visual shorelines, expanding ripples, wave normals, reeds/lily pads/driftwood/stones, birds and ducks, wind-driven foliage, gentle meadow relief, natural boundary markers, cloud shade, atmospheric depth and grit detail. Finish fireworks provide the celebration.

The recovered melody has a 48-kHz stereo electric-guitar arrangement alternating raspy and heavily overdriven phrases with bass and percussion. Authored rolling, nature, localized waterside, start/menu, crash/splash, guitar-finish and firework sounds supplement it. These are authored synthesis/foley-style effects rather than SID playback or field recordings. Music tempo is independent of ride pace.

## Original gameplay and timing

The source disk remains unchanged. SHA-256:
`9177346ca1e26f095f8f77ecd94d1bdbdf1496a5461a7d1f261fbe99f17c817e`.

The original screen sequence is **3, 4, 3, 5, 4, 6, 5, 7**. The source row loader determines the stream, including duplicated transition rows. Course completion fires at score/scroll 210. The exported view includes 24 initial rows and 209 inserted rows before the finish trigger.

The portable adapter executes recovered 6502 instructions and retains original character/sprite collision masks. It implements the subset of VIC/CIA/Kernal services used by this game; it is not a complete cycle-exact C64 emulator. No Commodore system ROMs are included in the shipped game data. Full behavioral equivalence beyond the tested comparisons remains unproven.

Normal runs the original PAL instruction simulation at 1.0x. Cruise, Relaxed, Fast and Expert run it at 65%, 80%, 120% and 140%. Scores are stored separately for each pace. Changing pace during a paused ride applies on restart. Real elapsed time drives the simulation, so slow render frames do not silently slow the race.

Left/right steering keeps Normal's real-time movement rate at every pace. Slower modes give more time to avoid obstacles without sluggish steering; faster modes bring obstacles sooner without also accelerating steering. Normal's original movement, timing and collision behavior remain unchanged. Other paces compensate rider-only lateral steps inside the simulation and sweep added steps through the original collision masks. Up/down movement retains the original per-pace behavior. The finish checkerboard conforms to the meadow triangles with two centimetres of clearance; grass tufts are excluded around its edges.

A fresh VICE 3.10 measurement recorded 30 PAL rows: mean 0.10112843285 seconds per eight-pixel row. The native core and the older independent 20-row VICE fixture differ by roughly one percent. See `reference/extracted/timing-review.json`, `timing-fresh.json` and its monitor transcript. Changing the emulator's video-standard resource reset the machine; that attempted NTSC run is invalid and was not used for calibration.

The camera remains centered on the course during play at an elevated 40-degree angle; the rider moves freely within its wider frame. Continuous scroll reconstruction and rider animation smooth the source game's discrete row movement. Presentation can lead the discrete source position by part of one tile; original collision remains authoritative.

## Build and asset authoring

Use Unreal Engine 5.8 plus Microsoft C++ Build Tools 2022 and a Windows SDK. Run:

`tools/build.ps1 -EnginePath 'C:/Program Files/Epic Games/UE_5.8' -Package`

The repository includes the prepared project assets except editable Epic template derivatives. On a fresh checkout, the build restores the head from your installed UE 5.8 MetaHumanCharacter/MetaHuman plugins; these plugins must be installed. Raw downloaded authoring sources are excluded. A complete art regeneration with -PrepareAssets also requires the source files recorded in the asset manifests. The asset preparation commandlet imports scanned scenery, authored equipment and refined riders, constructs material graphs and saves the blank Course One level that the runtime dresses. The packaged output is `Build/Windows/BMXCruiser.exe`. The project opens through `BMXCruiser.uproject`.

- `tools/author_equipment.py`: regenerate the OBJ/MTL equipment sources and inventory.
- `tools/refine_riders.py`: create refined FBX derivatives without altering the original avatar files.
- `tools/prepare_hero_head.py`: editor-only bake/import using the locally installed MetaHuman resources. Run with the MetaHumanCharacter and MetaHuman plugins enabled for that commandlet. Skin source PNGs/prompts are in `assets/source/hero`.
- `tools/smooth_course.py`: regenerate smooth visual contours and the meadow mesh using local NumPy, Shapely and mapbox-earcut.
- `tools/render_music.py`: regenerate the recovered-note guitar arrangement and sound effects.

The smooth road's area differs from the source raster by about two percent. The original raster collision and game instructions remain unchanged, so narrow visual/collision differences can occur at curved edges.

## Verification

`tools/test-core.cmd` checks the portable core: neutral collision at score 53, steering observations, render-rate-independent input, the original finish routine, and a complete collision-enabled replay at all five paces. It also checks continuous scroll presentation at 30/60/120 Hz. `tools/test-display.cmd` checks single-monitor bounds, DPI scaling, safe HUD bounds and click mapping.

`tools/verify-living-park.ps1 -Name workshop-final` runs the actual packaged game: all six appearance invariants, all seven skin tones on both avatars, and complete Normal/Expert replays including native 4K. Appearance rebuilds must preserve every byte of simulation RAM, its cycle count and run state.

Useful development flags:

- `-VerifyCosmetics -AutoQuit=16 -RenderOffscreen`: six equipment captures and invariants.
- `-VerifySkinTones -AutoQuit=25 -RenderOffscreen`: fourteen skin/gender captures and invariants.
- `-ReplayFile="absolute/path/to/course1-completion-replay.json" -AutoQuit=28`: collision-enabled route.
- `-MenuPage=0..5 -CaptureMenu`: startup, instructions, main, garage, Credits, settings.
- `-Style=0..5 -Gender=0..1 -SkinTone=0..6 -Outfit=0..1`: preview overrides.

Actual runtime screenshots and validation reports are saved under `preview`. Earlier AI concept previews are art references, not executable screenshots. Mobile/iOS builds are not implemented.

Asset provenance and licenses: [assets/CREDITS.md](assets/CREDITS.md). Additional art-pass details: [docs/ART-PASS-04.md](docs/ART-PASS-04.md).

### Course-following camera

The riding camera follows the course scroll at a fixed lateral center. Up/Down moves the rider through the frame as on the C64; Left/Right also remains visible within the trail width. The default elevated 40-degree view uses a 52-degree field of view, 23 metres behind the scroll origin and 24.5 metres above it. Players can set tilt from 30 to 75 degrees and distance from 90% to 140%, in five-unit steps. C opens a live course preview and pauses an active ride; the same controls are in Picture & Sound and the pause menu. Page Up/Page Down, +/-, and Home also work during a ride. Tilt and distance are saved independently of character customization. Garage orbit and zoom are independent.
