# Widescreen

The game is laid out for 640x480. The Vita screen is 960x544 (aspect 1.765, a little narrower than 16:9).

## Reference and licence

The list of problems below was made by reading ThirteenAG's NFSU2 FusionFix (WidescreenFixesPack, GPLv3) as a map of
facts: which function, address or constant matters and what goes wrong at a wider aspect. None of its code, tables
(HudLayout.dat) or tuned numbers is used here. Every value in the port comes from our own math on the screen size or
from our own observations (logs, screenshots). Addresses are SPEED2.EXE 1.2 (SHA-256 f9dd86c0...68a7).

Math used below: W x H = 960 x 544, k = (H / W) / 0.75 = 0.7556 (the part of the screen width the 4:3 picture
spans), the visible 2D canvas is 640 / k = 847.06 units wide, and its x range is -103.53 to 743.53.

## How the port does it

One setting, `widescreen` (default on). Off keeps the 640x480 game, pillarboxed in 4:3. `ui_log=on` logs the HUD and
menu draws (package, object, top-level group, canvas box, place, screen pixels) when a package's draws change; with
`widescreen=off` every draw is logged with place `4:3` and its pillarboxed screen pixels.

- Back buffer: the racing resolution native (0x5BF610) gives 960x544.
- 2D canvas: the x scale and offset operands of the 2D polygon (0x5C5020) and line quad (0x50DCD0) code read two slots
  (0x7E7FE0, 0x7E7FE4, ug2.py PATCHES) that widescreen.cpp writes. The canvas keeps the 4:3 scale, centred:
  clip x = x * k / 320 - k. FusionFix widens the canvas and moves its centre constants (item 3) instead; the centred
  mapping gives the same picture for centre-relative objects, keeps absolute-canvas ones (mirror, dyno graph) centred
  without their own patches, and lets `widescreen=off` switch at run time.
- HUD: during a HUD package's flush (0x537480: `HUD_*`, `Chyron_IG*`, `2PHud*`) each render object (0x51A5D0) is drawn
  with the mapping of its place: left (canvas x 0 at the screen's left edge), right, centre, or full (a full-width
  image is stretched). The place of a top-level group comes from its canvas extent in the previous flush (centre x
  below 213.3: left, above 426.7: right); `Chyron_IG` goes left as a whole.
- Menus: centred; objects wholly outside canvas x 0..640 (kept off screen by the 4:3 game) stay off screen.
- 3D: the projection native (0x5C7EC0) writes the factors two patched operands read: for views 1, 2, 4 and 5 the
  x scale is multiplied by k and the vertical half angle divided by k (the 4:3 vertical angle, wider horizontal one).

## Checklist

Status: done, n/a (not needed on the Vita or by this design), not done. "(Vita)" marks rows with adjustments
specific to the Vita or to 960x544 that come from our own earlier work, not from FusionFix.

| # | Item | Address / function | Problem at a wider aspect | Status | Verified |
|---|---|---|---|---|---|
| 1 | Back buffer size (Vita) | racing resolution 0x5BF610 (g_RacingResolution 0x870D1C) | Only 4:3 modes: the picture is stretched or pillarboxed | done: 960x544, kept at 544 high (not letterboxed to 16:9) | host log `back buffer 960x544`; `widescreen=off` gives 640x480 |
| 2 | 2D canvas mapping (Vita) | x scale [0x79AC10] (1/320) in the 2D polygon 0x5C5020 and line quads 0x50DCD0; x offset [0x784250] | The 640x480 FE/HUD space is stretched across the screen | done: centred, clip x = x * k / 320 - k; menu objects wholly outside x 0..640 stay off screen | host ui_log: full-screen menu images at px 117..843 |
| 3 | FE screen centre | immediates 320.0 at 0x51B3CB, 0x5368C8, 0x536A99, 0x536CC9, 0x537011, 0x48B640, 0x50B4F5 | Objects are placed around x 320, not the centre of a wider canvas | n/a: the centred mapping (row 2) keeps x 320 at the screen centre | by design |
| 4 | Screen-ratio to canvas x | sub_4EBEC0 (mouse cursor, from sub_4EC5A0): [0x797D58] 640.0, [0x797D50] 320.0 | A position given as a part of the screen width lands in the 4:3 area only | done for the front touch (`front_touch=on`): the touch is mapped through the inverse of the centred canvas mapping (row 2) before `GetCursorPos`; the cursor sprite itself is not drawn (docs/pad-binding.md) | host boot_steps touches: screen px 173, 479, 785 give canvas x 49, 319, 589 (left arrow, centre icon, right arrow of the main menu) |
| 5 | HUD element anchoring (Vita) | element x,y in sub_51B0B0 (0x51B190), minimap blips sub_51D620 (0x51D655), sub_4C66F0 (0x4C6753), stop / wrong-way sign sub_505150 (0x50516B) | HUD elements meant for the screen edges stay inside the centred 4:3 area | done at draw time (0x537480, 0x51A5D0): top-level groups by extent; Chyron_IG and MINIMAP_* textures go left | host ui_log URL race: tachometer and boxes right (px 735..1034), minimap and icons left (px 19..182), EA Trax banner left as one piece (px -22..323) |
| 6 | HUD alpha clear (nitrous gauge, minimap) (Vita) | sub_5C7140 quad, canvas corners 640.0 at 0x5C726A, 0x5C727A, drawn through 0x5C5020; called from 0x5CBABF (also fades 0x5C7A7B, 0x5C7A99, flash 0x5CB760) | Destination alpha is not cleared at the sides: the N2O arc and minimap blend with the scene's alpha | done: x corners -320 and 960; and the layer now draws this world_min alpha-only pass at all (commit 8d0d391), so no minimap workaround | host dump: quad clip x -1.511..1.511 at 960x544 |
| 7 | Cutscene letterbox bars | sub_5CBDC0 (called from 0x5CC1F6), quads x 0..640 | The race intro bars do not reach the screen sides | done: x corners -320 and 960 | patch applied; not seen (the boot race skips the intro) |
| 8 | Cutscene pillarbox bars | sub_5CBDC0 (added bars) | Only above 16:9 | n/a: 960x544 is below 16:9 | |
| 9 | Rear-view mirror | sub_5CBF40, quad x 200..440 through 0x5C5020 | Off centre once the canvas origin moves to the screen edge | n/a: the centred mapping keeps it centred | by design; not seen |
| 10 | Dyno result graph | offset {79, 73} at [0x7FA3C0] | Off centre once the canvas origin moves to the screen edge | n/a: the centred mapping keeps it centred (the exe's value is left as is) | by design; not seen |
| 11 | Controls-screen picture | fimul [0x870980] (screen width) at 0x5C315C | The picture is stretched with the screen width | done: reads the 4:3 width (725) from a slot | patch applied; not seen |
| 12 | Movies | movie quad in sub_536850 (0x536A34) | Full-screen movies must keep 4:3 and need black bars | done by row 2: movie objects stay 4:3, the present clears the bars | host ui_log: LS_EAlogo, THX, PSA movies at px 117..843 |
| 13 | FE re-layout on a size change | MakeLoadedPackagesDirty (call at 0x51244F) | Packages keep a layout made for the old size | n/a: the size is fixed when the device is made, before any FE draw | |
| 14 | 3D projection (hor+) | sub_5C7EC0: aspect = target h / w at 0x5C7F56, [0x784250] at 0x5C7FEA, [0x784260] at 0x5C7FFC | The horizontal angle stays as at 4:3 and the vertical one shrinks (vert-) | done: views 1, 2, 4, 5: x scale * k, vertical half angle / k | host log: view 1 (960x544) x 0.7556, vertical 0.6618; reflection and 4:3 views unchanged |
| 15 | Rain droplets | sub_5C7AB0 (0x5C7CE5 and +30) | Droplets are stretched sideways | not done: the port does not draw rain yet | |
| 16 | HUD aspect constraint | setting, applies to row 5 | Choice of how far the HUD moves out | n/a: one fixed screen | |

Vita rows and their sources (our earlier work, commit fdc19a0 and the older widescreen worktree): 1 (stay at 544, k =
min(1, (h/w)/0.75)), 2 (menus centred and off-screen objects hidden, after the parked menu layout pass), 5 (the EA
Trax pieces placed apart; minimap icons from this work's ui_log), 6 (the 640x480 alpha-clear quad and the N2O arc
reading the scene's alpha; the minimap mask analysis).

## Not widescreen (excluded)

From the same source, left out on purpose: free look / 360 camera, NOS trail length, frame-rate and tick fixes, spring
camera, particle time step, audio time at high frame rates, sticky steering, allocator and archive locks, subtitles,
no disc, skip intro and splash/profile notices, input and controller bindings, raw mouse and cursor confinement, the
driving FOV multiplier option, MSAA edge clearing, SMAA and console gamma, texture packs, static text, update notice,
registry and save folders, language, resolution list and options menu, windowed modes and the window procedure, and
the crash fix for a movie object drawn without a movie playing.
