# Visible-contact completion routes

These five joystick recordings exercise the modern visible rider/obstacle
contacts. They are separate from the original pixel-mask fixtures under
reference/extracted. Rendered replay verification keeps collision enabled.

Run `tools/verify-all-courses.ps1` against a packaged game to play all five in
order, verify every finish and transition, and check one-life qualification at
the final total of 1960. Add `-BuildPath` for another package folder. Add `-UHD`
for 4K. The native reference paths remain available with
`-OriginalCollision -ReplayDirectory reference/extracted`.

To regenerate a modern recording, first run the packaged course with
`-Course=N -BakeContacts -ContactFolder="absolute/folder" -NullRHI -AutoQuit=1`.
This exports a 5-cm offline sampling of the same floating-point visible contact
queries used by the game; the game does not load this verification grid.

Then run `tools/find-course-replay.cmd 2 output.json N 49262 180 0 1 profile.bin`.
The search uses recovered movement instructions, collision clearance and eight
CPU threads. Up/down movement is permitted in the tighter later-course sections.
It independently replays each result before writing it. Always validate the
recording in the rendered game afterward: visual smoothing is frame dependent.
