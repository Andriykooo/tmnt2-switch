Thread title:
[Release] TMNT 2: Battle Nexus native Switch port (v0.1.0 beta)

Forum: Nintendo Switch → Homebrew Development and Emulators → Releases

---

Hi everyone!

This is a native Nintendo Switch port of **Teenage Mutant Ninja Turtles 2: Battle Nexus** (PC, 2004). It is not emulation.

**How it works**
[xntrz](https://github.com/xntrz/tmnt2) has been reimplementing the PC game for years and publishes a WebAssembly build of it. I recompiled that build to C with wasm2c and wrote a Switch runtime around it. The runtime replaces the browser parts with native ones:
- WebGL → OpenGL ES 3 (Mesa)
- WebAudio/OpenAL → openal-soft
- HTML5 Gamepad → Joy-Con
- IndexedDB → saves on the SD card

**You need your own copy of the PC game.** No game data is distributed.

[SCREENSHOT 1: title screen]
[SCREENSHOT 2: gameplay]
[SCREENSHOT 3: pause menu / character select]

**Download and source:** https://github.com/Andriykooo/tmnt2-switch/releases

**Install**
1. Copy the `switch/` folder from the release/repo to the root of your SD card.
2. Put `TMNT.DAT`, `TMNTE.DAT`, `WaveData.bin`, `SeData.bin` and `TrSeData.bin` from your PC copy into `sdmc:/switch/tmnt2/`.
3. Launch via title override (hold R on a game) so the port gets full RAM.

**Controls**
- A: attack A
- X: attack B
- Y: dash
- L: shuriken
- B: jump
- R / ZR: guard
- R + X: grab, then X to throw
- ZL: switch turtle
- +: pause
- + and − together: quit

**What works**
- Full game, menus and saving
- Sound effects

**Known issues**
- No voices or music if your copy lacks `voice*.bin` / `strbgm.vas`.
- Movies are placeholders.
- This is a beta.

Please attach `sdmc:/switch/tmnt2/log.txt` to bug reports.

**Credits**
- xntrz for the reimplementation
- devkitPro / libnx / switch-mesa / openal-soft / WABT
