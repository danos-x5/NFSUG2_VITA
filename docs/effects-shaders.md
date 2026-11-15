# Effects shaders (road reflection, headlights, crowd, world bones)

Status: merged, regenerated, host-checked, Vita build compiles with 12 programs PENDING. The on-device compile and device check are NOT done: the Vita (192.168.1.237) did not answer on ports 1337 and 1338.

## What was applied

`experiments/parked/ug2-effects.patch` (23 files). 21 files applied cleanly with `git apply`. `src/platform/config.cpp` and `tools/recompiler/games/ug2.py` were merged by hand:
- config.cpp: `particle_step`, `env_faces` added to the `gfx_preset` line of `Known[]`.
- ug2.py: `0x492e10` and `0x48b1e0` added after `0x5bf610` in NATIVE_OVERRIDES; the widescreen and audio entries of other work are untouched.

Contents: FOG variants of worldreflect, worldheadlight, worldheadlightreflect; new worldbone_f.cg and a depth output in worldbone_v.cg; pass mappings in d3dx9.cpp with fallback chains; weak shader symbols in gxm_render.cpp so PENDING programs fall back; the particle buffer fix (DISCARD Lock on a busy buffer copies only the locked bytes); `src/ug2/views.cpp` with `env_faces` (1..6) and `particle_step=frame|game`.

## Regeneration

`/opt/homebrew/bin/python3.12 tools/recompiler/generate.py --game ug2 --binaries ~/Downloads/Need_for_Speed_Underground_2_Win_Installed --out src/ug2/generated` took 174 s. Host build OK.

## Host checks (Street X, virtual_clock, boot_opponents=3)

- vita preset: exit 0, "race finished 2603 frames" (same as before the merge), no "not linked". Draws per 300-frame window before/after: 339/488, 1013/994, 876/813, 618/600, 570/582, 903/962, 640/644, 800/742 (loading window differs, the rest within about 10 %).
- `gfx_preset=max`: exit 0, race finished 2603 frames, no "not linked". About 1030-1334 draws. effect_stats shows IDI_WORLDHEADLIGHT_FX about 160/frame, IDI_WORLDREFLECT_FX 0.4-5, IDI_WORLDHEADLIGHTREFLECT_FX 8-11, IDI_HORIZON_FOG_FX 1, WORLDNODEPTH 120-217.
- ug2uitest passes, ug2filetest passes (needs a scratch folder argument).

## Compile (Vita) results

Not run. All 12 are still PENDING in cmake/shaders.cmake, so the fallback chain is used: reflect, headlight and headlight-reflect passes fall back to the already compiled reflect/headlight programs without fog, the world-bone pass to the existing world program, and HORIZON_FOG to the fixed-function pass.

| program | source | defines | result |
|---|---|---|---|
| worldbone_v, worldbone_f, worldbone_f_atest | worldbone_v.cg, worldbone_f.cg | ATEST=1 | pending |
| worldreflect_fog_v/f/f_atest | worldreflect_v.cg, _f.cg | FOG=1 | pending |
| worldheadlight_fog_v/f/f_atest | worldheadlight_v.cg, _f.cg | FOG=1 | pending |
| worldheadlightreflect_fog_v/f/f_atest | worldheadlightreflect_v.cg, _f.cg | FOG=1 | pending |

To finish: `tools/shadercomp/vita_compile.py --out <dir>` (Vita awake, libshacccg installed), copy the .gxp files to shaders/gxp/, run `cmake -DGXP=<dir> -P cmake/shader_sources.cmake` for sources.txt, delete the PENDING flags, rebuild build-vita.

## Settings that show the effects

`gfx_preset=max`, or `gfx_set=g_RoadReflectionEnable:2,g_CarHeadlightEnable:1,g_HorizonFogEnable:1,g_ParticleSystemEnable:1,g_CarEnvironmentMapEnable:3` (the value form of gfx_set as in registry.cpp). `env_faces=1..6` limits cube faces per frame, `particle_step=frame` (default) or `game`.

Note (2026-11-13): the 12 programs above were compiled in commit 41954f2 and are no longer PENDING.

## Screen effects (StretchRect)

Calls seen on the host (URL race, 960x544 back buffer), one per frame each:

| Setting | Copies | Passes after the copy |
|---|---|---|
| Motion blur | back buffer -> 960x544 target, filter NONE | FILTER p3 twice (to targets), FILTER p0 onto the back buffer |
| Bleach bypass (+ tint) | back buffer -> 960x544 target, filter NONE | SCREENFILTER p0, p1, p2 |
| Light glow + overbright + streaks | back buffer -> 480x272 target, filter NONE | OVERBRIGHT, about 13 FILTER passes, reverse-subtract and BLENDFACTOR blends |
| Road reflection 3 | none | FILTER p3 twice on the reflection target |

Motion blur strength (0x5CC460): t = (speed - 25 m/s) / 100 m/s clamped to 0..1, plus 1.1 times the nitrous blend [0x870900], at most 0.65; nothing below t = 1/255. The two FILTER p3 passes average 4 taps spaced t * 0.0075 (and 4 times that) of the screen along the projected direction of travel (a zoom blur, zero at the centre), and the last pass blends the result with BLENDFACTOR = t. At 80 mph without nitrous t is about 0.1 (10% of a blur up to about 11 pixels at the corners), so the effect is faint; with nitrous it reaches 65%.

The game's caps check (0x5BE200) turns motion blur, streaks and overbright off without D3DPBLENDCAPS_BLENDFACTOR, and any effect whose technique has no programs (ValidateTechnique).

- `Device::StretchRect` (d3d9_device.cpp) adds a scene whose target is the destination (no depth) and one draw: `ff_screen_v` + `ff_tex_f`, clamp, linear for D3DTEXF_LINEAR, else point. Depth and cube sources, and back buffer to back buffer, are not copied (logged once per kind).
- A back buffer source is `render::backBufferTexture()`. `FrameRenderer::drawFrame` points it at the display buffer (ABGR, linear) or the offscreen back buffer; `planFrame` no longer carries a back buffer clear past a scene that reads it.
- `shaders/screen_f.cg` ports the game's pixel shaders: EFFECT 1 overbright (ps_1_1), 2-4 screen filter p0-p2 (ps_1_4), 5 rain drop (ps_2_0); each with _atest and the three blend factor variants. 25 binaries from one Vita compile (libshacccg SDK 3.0.0); the older binaries were kept.
- `debug_hold_frame=N` (with `virtual_clock=on`) stops the game thread at race frame N and writes the shown frame to `ux0:data/NeedForSpeedUnderground2/hold.bmp`.

Toggling the six effect settings on and off every 300 frames in a host race (null device checks) gave no errors.
