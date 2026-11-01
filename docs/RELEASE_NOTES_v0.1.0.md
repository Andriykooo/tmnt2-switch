# TMNT 2: Battle Nexus for Switch v0.1.0 (beta)

This is the first public build of a native Nintendo Switch port of *Teenage Mutant Ninja Turtles 2: Battle Nexus* (PC, 2004).

The port runs the [xntrz/tmnt2](https://github.com/xntrz/tmnt2) reimplementation natively. Its web build is recompiled with wasm2c, and a custom runtime supplies GLES3 graphics, OpenAL sound, Joy-Con input and SD-card saves. It is not an emulator.

**Game data is not included.** You need your own copy of the PC game.

## Install
1. Download `tmnt2.nro` from this release, or copy the `switch/` folder from the repository.
2. Copy it to `sdmc:/switch/tmnt2/`, together with the rest of the `switch/tmnt2/` folder (fonts, touch data, movie placeholders).
3. Add these files from your PC copy of the game: `TMNT.DAT`, `TMNTE.DAT`, `WaveData.bin`, `SeData.bin`, `TrSeData.bin`.
4. Start the Homebrew Menu through title override (hold **R** while launching any game) so the port gets full RAM.

## Controls
| Button | Action |
|---|---|
| A | Attack A / OK |
| X | Attack B |
| Y | Dash |
| L | Shuriken |
| B | Jump / back |
| R or ZR | Guard |
| R + X | Grab, then X to throw |
| ZL | Switch turtle |
| + | Pause |
| L3 / R3 | Gauge / camera |
| + and − together | Quit |

## Status
- ✅ Story mode, menus, saving (to `sdmc:/switch/tmnt2/ifs/`), sound effects
- ⚠️ Voices and music need `voice0-3.bin` and `strbgm.vas`. Some PC copies don't have them.
- ⚠️ Cutscene movies are placeholders. Movie playback isn't supported yet.
- ⚠️ This is a beta, tested on one console. Send bug reports with `sdmc:/switch/tmnt2/log.txt` attached.

## Credits
- [xntrz](https://github.com/xntrz/tmnt2) for the reimplementation this port is built on
- devkitPro, libnx, switch-mesa, openal-soft and WABT (wasm2c)
