# BMX Cruisers 0.4 art pass

The accepted gameplay remains untouched. This pass replaces the generic interface with an angled BMX garage presentation and replaces the placeholder inline skates, quad skates and scooter with authored, material-separated meshes.

- Inline skate: fitted molded shell, padded cuff/liner, buckle, laced tongue, machined frames, four 80-mm wheels, webbed hubs and axle hardware.
- Quad skate: stitched high-top boot, laces/eyelets, twin trucks, axle hangers, four side-by-side polyurethane wheels and an angled toe stop.
- Scooter: 53-cm beveled deck, grip surface, neck/headtube, double clamp, shaped fork, 110-mm wheels, axle bolts, grips and sprung rear brake.
- Rider: refined skinned meshes (29,760 / 34,928 triangles), sportswear, skin-only tone masks and subsurface skin shading, finger articulation and skating/kicking poses. Seven saved tones apply independently of avatar and clothing.
- Trail: rotated/macro-blended material layers plus unique per-course repairs, fissures, dusty shoulders and drifting worn riding lines. Every overlay retains the original trail/obstacle footprint.
- Interface: angled numbered plates, bright yellow selection, red offset accents, wire grid, condensed italic font, separate settings page, front-facing rider preview and Q/E orbit. Existing keyboard operation and fullscreen scaling remain.

The bodies remain refined Microsoft Rocketbox derivatives; the current head is a separately baked, high-detail MetaHuman template with authored skin textures. It is not a complete animated MetaHuman character. The authored props have materially better construction than the earlier basic-shape assemblies; real-time screenshots document their actual appearance.

The fitted V3 head adds 61,978 triangles, actual eye geometry, generated UV-aligned albedo, pore normals and independent skin-tone materials. The donor chest is clipped at an interpolated neck boundary, and its trapezius flare is tapered into an anatomical neck seam. Surface normals follow that deformation. A tapered neck follows the posed torso and overlaps both that boundary and the shirt collar. The original head's hidden material explicitly supports skeletal meshes, preventing packaged builds from substituting visible default geometry. The packaged game uses standard static mesh/material assets rather than a creator-service or facial-animation dependency.

The V4 extreme helmet is an authored 42,552-triangle full-face mesh with ear coverage, recessed impact liner, cheek pads, a broad molded chin bar, mouth grille, a curved two-sided visor, edge trim, retention webbing/buckle, vents, mounting bolts and contrasting details. Its tintable composite-paint material is separate from metallic bicycle paint. Helmet fit scales with the selected rider. Both outfits use the same fitted head and neck.

To regenerate the equipment, run `tools/author_equipment.py`. After building the editor target, run `tools/prepare_helmet_fit.py` through UnrealEditor-Cmd with MetaHumanCharacter and MetaHuman enabled for the donor-head bake, then package normally. This targeted import preserves the existing face textures. `tools/verify-rider-fit.ps1` captures front, side and rear views at 4K and rejects missing-material usage warnings.

Validation (2026-10-31): Windows package succeeded; six ride-style and fourteen skin/character invariants passed. Normal 1080p and Expert 4K replays both completed at score 210 and original source time 21.225524 seconds. Four 4K fit captures passed and were inspected. User preferences were restored after the automated replays. Logs are `reference/extracted/helmet-fit-verification.log` and `rider-fit-captures-final.log`.

A 4K garage run exposed a D3D12 shader fault in mesh-distance-field shadow culling. The project now uses virtual shadow maps without that additional shadow pass; mesh distance fields remain available for Lumen. The complete recheck passed without another device fault. A nonfatal virtual-shadow-map marking-budget warning remains at the close rear garage angle; it is separate from the corrected head/helmet fit.
