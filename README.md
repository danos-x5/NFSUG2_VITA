<a id="readme-top"></a>

<div align="center">

<p align="center">
  <br />
  A fan project that enables you to play your own PC copy of Need For Speed Underground 2 (Version 1.2) on your PS VITA. This is not the game files for Need For Speed Underground 2. You must provide your own legally obtained copy of the game. This project provides a compatibility layer along with performance optimisations to allow the PC version of the game to run on a PS VITA system. This project does not condone piracy.
  <br />
  <br />
  This is not a reverse engineer of the game. All aspects of the game are functionally identical.The game's own code is translated to C++ ahead of time from your copy of `SPEED2.EXE`, and a compatibility layer gives it the Windows and Direct3D 9 services it expects. 
  <br />
  <br />
  As mentioned, code was only modified to be more performant or for compatibility due to the PS VITA system having major hardware differences to a Windows computer. These changes have been proven not to change the functionality of the game in any way. Nonetheless, it is very possible that these changes have brought about unexpected bugs/crashes that do not exist in the PC version, and this is something that will constantly be addressed with updates.
  <br />
  <br />
  <a href="#install">Install</a>
  &nbsp;·&nbsp;
  <a href="#controls">Controls</a>
  &nbsp;·&nbsp;
  <a href="#settings">Settings</a>
  &nbsp;·&nbsp;
  <a href="../../issues">Report Bug</a>
</p>

> **Playtest.** The game is playable in all aspects. The entire career mode is working, and the framerate is good in most scenes.
> Expect rough edges; [Known issues](#known-issues) lists the ones we know.

## Requirements

- Need for Speed Underground 2 for PC, **version 1.2**, installed (your own copy). The whole game folder is needed,
  about 1.7 GB, or about 2 GB with the converted movies.
- `SPEED2.EXE` should be the original 1.2 executable, with SHA-256
  `f9dd86c054878ce6276beb07c1fd61874f7a1e4bf1f241b084c65b73e24168a7`. See [Mods](#mods) for anything else.
- Only the **US release** of version 1.2 has been tested. The EU release will be tested soon.
- PS Vita with HENkaku / Ensō and [VitaShell](https://github.com/TheOfficialFloW/VitaShell)
- About 2.5 GB free on the memory card
- [CapUnlocker](https://github.com/GrapheneCt/CapUnlocker) is recommended: it lets the port use the Vita's fourth
  CPU core.
- [PSVshell](https://github.com/Electry/PSVshell) is optional but recommended in this playtest state: overclocking
  the CPU to 500 MHz with it increases performance.

The port sets the Vita's CPU and GPU clocks itself, so the game runs without an overclocking plugin.

## Install

1. Install `NFSU2VITA.vpk` with VitaShell.
2. Convert the movies in your game folder on the PC first (see [Movies](#movies)). Without this step the movies
   will be very slow.
3. On your PC, open the game folder (the one with `SPEED2.EXE`) and copy **everything inside it**, every file and
   folder including `MOVIES`, to `ux0:data/NeedForSpeedUnderground2/` (VitaShell's FTP or USB mode works).
   `SPEED2.EXE` must end up at `ux0:data/NeedForSpeedUnderground2/SPEED2.EXE`, not in a subfolder.
4. Start the game from the home screen.

If the game closes right away, `ux0:data/NeedForSpeedUnderground2/NeedForSpeedUnderground2.log` says what is missing.

Your career is saved in `ux0:data/NeedForSpeedUnderground2/save/`. Copy that folder to back it up.

### PC save files

Save files from the PC game work. On the PC each profile is a folder named after the profile, in
`%LocalAppData%\NFS Underground 2\`. Copy that folder, with the file inside it, to

```
ux0:data/NeedForSpeedUnderground2/save/NFS Underground 2/<profile name>/
```

so a profile called `Player` ends up as `ux0:data/NeedForSpeedUnderground2/save/NFS Underground 2/Player/Player`. Then load it
from the game's profile screen. The button settings saved in a PC profile come with it; change them in the game's
Controls screen if they do not suit the Vita.

### Movies

The game's movies are the `.vp6` files in its `MOVIES` folder, and the port needs them there. On their own they
are decoded in software, and the picture will be very slow on the Vita while the sound plays normally. With an
H.264 copy of each movie beside the original, the Vita's own video decoder draws the picture and the movies play
smoothly. To make the copies, with [ffmpeg](https://ffmpeg.org) installed, run this once on the PC:

```sh
python3 tools/vita/movies_convert.py "<game folder>/MOVIES"
```

It writes one `.mp4` next to each `.vp6` (29 files, about 290 MB) and changes nothing else. Then copy the game
folder to the Vita as described above, so that `MOVIES` holds both. If you already copied the game, copy the
`MOVIES` folder again.

The `.mp4` files hold the picture only. The game still plays the sound from the `.vp6` files, so both are needed,
and an `.mp4` opened on a PC is silent.

## Controls

The Vita appears to the game as a PlayStation 2 controller. The racing buttons can be changed in the game's own
Controls screen (Cross is shown as Button 3) and are saved with your profile.

### Menus

| Button | Action |
|--------|--------|
| D-pad, left stick | Move |
| Cross | Select |
| Circle | Back |
| Start | Start, skip movie |

### Driving

| Button | Action |
|--------|--------|
| R | Accelerate |
| L | Brake / reverse |
| Left stick | Steer |
| Cross | Handbrake |
| Circle | Nitrous |
| Triangle / Square | Shift up / down |
| Rear touch, upper corners | Shift |
| Rear touch, lower corners | Look back / change camera |
| Select | Reset car to the road |
| Start | Pause |

### Free roam

| Button | Action |
|--------|--------|
| D-pad up | World map |
| D-pad right | Enter a race or shop |
| D-pad down | Career statistics |
| D-pad left | Read SMS |

Text fields, such as a profile name, open the Vita keyboard. To quit, use the PS button.

## Settings

With nothing changed the game runs locked to 30 frames a second, in widescreen, with the touch screen on as a replacement for a mouse.

The first time the game starts it writes a settings file, `ux0:data/NeedForSpeedUnderground2/NeedForSpeedUnderground2.cfg`.
Every setting in it is listed at its default behind a `#`. To change one, remove the `#` at the start of its line,
edit the value and start the game again, for example:

```
frame_rate=0
```

Delete the file to get a fresh copy on the next start.

| Setting | Values | What it does |
|---------|--------|--------------|
| `frame_rate` | `30` (default), `0` | Locked to 30 frames a second for steady pacing, or unlocked |
| `widescreen` | `on` (default), `off` | Full 16:9 picture with the HUD at the screen edges, or the original 4:3 one with black bars |
| `gfx_preset` | `vita` (default), `max` | Graphics the port is tuned for, or everything on (much slower) |
| `pad_layout` | `vita` (default), `custom`, `game`, `classic` | Button layout: the port's default, a fixed layout that ignores the game's Controls screen, the game's own defaults, or a layout with the camera and look back on the face buttons |
| `rear_touch` | `on` (default), `off` | Rear touch pad for shifting and the camera |
| `front_touch` | `on` (default), `off` | Touching the screen works as a mouse click in the menus; no cursor is drawn |
| `world_lod_pixels` | `0` (default, off), a width in pixels | Scenery narrower than this on screen uses its simpler model. `128` gives more speed; higher values show more pop-in |
| `world_min_pixels` | `0` (default, off) to `64` | Scenery smaller than this on screen is not drawn. More speed, more pop-in |
| `boot_race` | `off` (default), `on` | Skip the menus and start a test race |
| `log_info` | `off` (default), `on` | Write a detailed log to `NeedForSpeedUnderground2.log`, useful when reporting a problem. Errors are logged either way |

The graphics settings can be changed in the game's own Display menu, and they are kept between sessions. The
resolution stays at 960x544. Above the Medium detail setting the Vita will definitely struggle with performance.

## Mods

All mods are untested at this stage. The only kinds that could work are PC mods that change `SPEED2.EXE` or the
game's asset files. They may cause crashes or major performance problems.

The port checks `SPEED2.EXE` when it starts. A modified executable of the original size starts with a warning in
`NeedForSpeedUnderground2.log`; one of another size does not start.

## Known issues / caveats

- The game aims for 30 fps and drops below it in busy moments: race starts, heavy smoke and dense traffic. Graphics
  settings above Medium make this much worse.
- At lower settings the framerate is very stable and can even hit 60fps during gameplay if the framerate is set to unlocked mode.
- On-screen button prompts show PC key names, such as "Enter", not Vita buttons.
- `gfx_preset=max` is well below 30 fps.
- No online or LAN play.
- Sound bug where sometimes the music continues playing after entering the pause menu

## Build

You need your installed copy of the game, [Python 3](https://www.python.org/downloads/),
[CMake](https://cmake.org) 3.16 or newer, [Ninja](https://ninja-build.org), [VitaSDK](https://vitasdk.org) with
`VITASDK` set, and [LLVM](https://llvm.org) (clang and lld) with `NFSU_LLVM` set to its root.

On Windows, one script does everything:

```powershell
tools/ug2-build.ps1 -Game "<folder with SPEED2.EXE>"   # -> build-Release-clang/NFSU2VITA.vpk
```

By hand, from the repository root:

```sh
# 0. Where VitaSDK and LLVM are installed (your paths may differ)
export VITASDK=/usr/local/vitasdk
export NFSU_LLVM=/usr/local/opt/llvm

# 1. Recompiled game code from your game folder (about 5 minutes)
python3 tools/recompiler/generate.py --game ug2 --binaries "<game folder>" --out src/ug2/generated

# 2. Effect tables from the same executable
python3 -I tools/fx/fxtable.py --exe "<game folder>/SPEED2.EXE" --out include/winapi/d3d9_fxtables.h

# 3. The VPK
cmake -S . -B build-vita -G Ninja -DCMAKE_BUILD_TYPE=Release -DNFSU_COMPILER=clang -DNFSU_GAME_OPT=-O2
ninja -C build-vita                                    # -> build-vita/NFSU2VITA.vpk
```

- The generator is made for the 1.2 executable named under [Requirements](#requirements). With any other it prints a
  warning and asks before continuing (`--accept-sha256 <hash>` skips the question); see [Mods](#mods).
- The clang build uses the profile in `pgo/` by default; `pgo/README.md` explains how it was recorded.
- Shaders ship prebuilt in `shaders/gxp/`.
- `-DNFSU_COMPILER=gcc` builds with VitaSDK's own compiler instead; it is slower on the console.

| Path | Contents |
|------|----------|
| `include/` | virtual x86 CPU, runtime, Win32 and platform headers |
| `src/lib/` | runtime: guest memory, threads, Win32, C runtime, Direct3D 9 and D3DX |
| `src/platform/` | Vita backend: GXM renderer, audio, input, sync, logging |
| `src/ug2/` | entry point, native replacements of game functions |
| `shaders/` | Cg shaders and compiled binaries |
| `tools/` | recompiler and build scripts |

## Credits

- [UG1VITA](https://github.com/NDRWhun/UG1VITA): the Need for Speed Underground port this project grew out of
- [GrapheneCt](https://github.com/GrapheneCt): CapUnlocker
- [Electry](https://github.com/Electry): PSVshell
- [ThirteenAG](https://github.com/ThirteenAG): the WidescreenFixesPack, read as a guide to what a wider picture
  needs fixed; none of its code is used

## License

MIT for the port's own code ([LICENSE](LICENSE)); third-party parts: [THIRD_PARTY.md](THIRD_PARTY.md).

Unofficial, non-commercial fan port, not affiliated with or endorsed by Electronic Arts. *Need for Speed* is a
trademark of Electronic Arts. You must own the game.

<!-- BADGES -->

[platform-shield]: https://img.shields.io/badge/PS%20Vita-native-2f7bff?style=for-the-badge&labelColor=1a1f3d&logo=playstation&logoColor=white
[platform-url]: https://vitasdk.org
[resolution-shield]: https://img.shields.io/badge/960x544-native-b44dff?style=for-the-badge&labelColor=1a1f3d
[resolution-url]: #settings
[renderer-shield]: https://img.shields.io/badge/renderer-sceGxm-ff3b5c?style=for-the-badge&labelColor=1a1f3d
[renderer-url]: #build
[license-shield]: https://img.shields.io/badge/license-MIT-ff3b5c?style=for-the-badge&labelColor=1a1f3d
[license-url]: LICENSE
