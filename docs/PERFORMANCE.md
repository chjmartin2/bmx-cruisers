# Version 1.3 performance notes — 2026-11-02

Version 1.3 reduces rendering and course-loading work in every graphics preset. It defaults to DirectX 11 / Shader Model 5 and does not require Nvidia-specific features, ray tracing, Lumen or virtual shadow maps. A modern four-core CPU and 8 GB RAM are provisional targets; minimum hardware requirements have not been independently established on a range of PCs.

## Graphics presets

Extract the whole Windows ZIP and run BMXCruiser.exe. In a development checkout, **Play BMX Cruisers.cmd** opens the latest packaged build. **Picture & Sound > Graphics** offers:

| Preset | Scene rendering | Presentation |
|---|---|---|
| Performance, default | Up to 900p, 60-fps limit | Temporal reconstruction, full-resolution interface, reduced decoration, no dynamic shadows |
| Balanced | Up to 1080p, 60-fps limit | More decoration and conventional shadows |
| High | 1080p or optional 4K, 60-fps limit | More foliage, trees, effects and shadow detail |

The coarse 720p/FXAA presentation in the earlier local preview is replaced by a 900p scene, temporal reconstruction, sharper texture sampling and fuller low-cost bushes. Performance still deliberately reduces decoration and shadows. Balanced is the next choice when picture quality matters more than the smallest rendering budget. The interface stays at the window's full resolution in every preset, including at 4K. Performance and Balanced do not supersample smaller windows.

Changing Graphics reloads the selected course from the start-screen settings page. No graphics setting changes original timing, steering, score, the rider footprint or obstacle positions. Scores, appearance and camera preferences are saved separately.

## Measured comparison

Same collision-enabled Course One replay, Normal pace, 1920×1080 window, uncapped for measurement. Samples cover elapsed capture seconds 3–21, excluding most startup and the finish transition. The earlier local build uses its 1080p scene. The new presets use their scene budgets above. Desktop background work can affect these figures.

Host: Intel Core i9-14900KS, Nvidia RTX 3090, Windows. These measurements show improvement on this host; they do not predict a particular integrated GPU's frame rate.

| Metric | Earlier local build | Performance | Balanced | High, 1080p |
|---|---:|---:|---:|---:|
| Median frame time | 13.77 ms | 2.70 ms | 2.77 ms | 5.49 ms |
| 95th percentile frame time | 15.47 ms | 5.28 ms | 4.43 ms | 7.02 ms |
| Median game-thread work | 1.50 ms | 1.14 ms | 1.04 ms | 1.34 ms |
| Median GPU time | 13.37 ms | 2.68 ms | 2.76 ms | 5.45 ms |
| Graphics memory reported by Unreal | 2,620 MB | 569 MB | 530 MB | 769 MB |

High's median frame cost is about 2.5 times lower than the earlier build; Performance's is about 5.1 times lower. Performance uses roughly 78% less graphics memory in this capture. Texture residency and transient allocations mean memory use need not increase strictly with the preset. Normal play remains capped at 60 fps to avoid spending the savings on unnecessary frames.

With Unreal limited to four logical CPU cores and process affinity restricted to four hardware threads, the complete Course One replay held approximately 60 fps with sound enabled. Peak application working memory was 825 MB. A Course Five parking-area scene also held approximately 60 fps and peaked at 986 MB. These checks still used the RTX 3090; limiting CPU cores does not emulate an older CPU or integrated GPU.

The earlier local DirectX 11 preview also started and rendered through Microsoft Basic Render Driver with the Nvidia device unselected. That was a software-rendering compatibility check, not a performance qualification. A working hardware graphics driver is recommended. No integrated GPU was available on this host for a direct frame-rate test.

## Changes shared by every preset

- Large mesh JSON files are authoring inputs only. The packaged runtime uses compressed, validated binary mesh caches and small metadata files, with normals prepared offline. Vertex positions retain source double precision.
- Terrain, trails and water render in spatial chunks, allowing off-screen portions to be culled. Water contact triangles are unchanged.
- DirectX 11 lighting avoids Lumen, virtual shadows, distance fields, ray tracing and per-frame sky capture. This includes a lighting-quality tradeoff across all presets.
- Lighter presets omit the duplicate scanned shrub layer and reduce tiny prop geometry, tree crowns, litter, localized sound streams and fireworks particles. Large vivid finish bursts remain.
- Decorative wildlife, ripples and dust use a lower update rate in the lighter presets. Human animation, controls, camera motion and the recovered PAL simulation remain independent of that rate.
- Scene rendering is independent of desktop/window resolution. A sharp 4K HUD does not require rendering the whole park at 4K.

## Validation

- Native timing, scrolling, completion and retry checks passed for all five original courses.
- Display/DPI/layout tests passed. The packaged settings screen, 4K modes and Xbox pause/menu controls were checked.
- Every cached water triangle retains exactly the original coordinates and winding. All five sampled collision maps match the prior scenery-restoration build byte for byte: 12,265,500 positions checked.
- All five packaged pool, car, oil, coping, swept-contact and bridge-rail checks passed.
- The collision-enabled Course One replay completed at Normal pace in all three presets. A real bridge-rail strike retains the visible crash, input lock and result rules.
- The ZIP excludes saves, logs, symbols, authoring mesh JSON and editable engine-template character sources. Required notices and the Microsoft runtime installer are included.

Local captures and reports in `release-work/` remain outside Git. `tools/summarize_performance.py` summarizes Unreal CSV captures. A comparable capture uses the packaged game with `-RenderOffscreen -ProfileCourse -csvCaptureFrames=0 -csvCompression=0 -ReplayFile="<absolute replay path>" -Difficulty=2 -GraphicsPreset=0 -ResX=1920 -ResY=1080 -AutoQuit=31 -ExecCmds="t.MaxFPS 0,r.VSync 0"`. Keep the shipped cap enabled during ordinary play.
