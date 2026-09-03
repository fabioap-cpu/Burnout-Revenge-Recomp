# Burnout Revenge Recompiled

An experimental static recompilation of the Xbox 360 version of Burnout
Revenge using [ReXGlue](https://github.com/rexglue/rexglue-sdk).

This repository contains project source, recompilation metadata, build
configuration, and a vendored copy of the shared ReXGlue SDK (under
`rexglue-sdk/`) with the fixes and diagnostics developed while bringing this
game up. It does **not** contain game data, Xbox system files, saves, or
compiled executables. You must provide files from your own copy of the game.

## Project status

This is an experimental source release, actively in progress. On the tested
Windows AMD64 system, the current tree can:

- boot into the game and reach the title screen, save/load, and main menu;
- select a rank and a car with full 3D rendering (reflections included);
- load into a race and drive with a complete, live HUD (speed, timer, money,
  rating, boost meter);
- complete a full play session (multiple races, menu navigation, pausing)
  **with no crashes observed**.

This meets and goes past the project's ALPHA gate and is very close to BETA
(a full crash-free session has been observed, but is not yet independently
reproduced/confirmed a second time).

### Known issues

- **Frontend/menu rendering during pause and some screens.** On the title
  screen, the pause menu, and the "Advancing your Revenge Rank" loading
  screen, the 3D world is not rendered behind the 2D overlay (it should still
  be visible, frozen, per a reference Xenia Canary capture of the same game
  state). This was originally suspected to be a color/channel-swap bug; that
  hypothesis was ruled out with direct evidence (the scanout gamma ramp,
  render target formats, vertex fetch byte-swapping, and blend constants all
  match a byte-for-byte comparison against current Xenia Canary). The real
  cause is a guest-code control-flow divergence that skips issuing the 3D
  world's draw calls in this build specifically; it has not yet been
  root-caused. See `rexglue-sdk`'s diagnostics section below for the
  reusable tooling built to investigate this class of bug.
- Frame-pacing on high-refresh-rate displays previously over-reported and
  over-presented (e.g. ~240 real `Present()` calls/sec on a 180 Hz monitor
  instead of ~180) due to a guest-triggered low-latency present path not
  synchronizing with the real-vblank-driven UI paint path. **Fixed and
  verified** in the vendored SDK (see `rexglue-sdk/src/ui/presenter.cpp`,
  `WaitForUITickFromUIThread`).
- FPS drops in some map areas have been observed but not yet profiled to a
  specific cause.

See [docs/PROJECT_STATE.md](docs/PROJECT_STATE.md) for the full, dated log of
findings, fixes, and open questions.

## Requirements

- Windows AMD64
- Git, CMake 3.25 or newer, Ninja, and Clang
- a legally obtained, extracted Burnout Revenge (Xbox 360) game tree,
  including `BurnoutRevenge_default.xex`

## Build

From the repository root:

```powershell
cmake --preset win-amd64-release -DREXSDK_DIR="$PWD\rexglue-sdk" `
  -DCMAKE_C_COMPILER="C:/Program Files/LLVM/bin/clang.exe" `
  -DCMAKE_CXX_COMPILER="C:/Program Files/LLVM/bin/clang++.exe" `
  -DCMAKE_C_FLAGS="-march=x86-64-v2" -DCMAKE_CXX_FLAGS="-march=x86-64-v2"
rexglue-sdk\out\win-amd64\bin\rexglue.exe codegen burnoutrevenge_manifest.toml
cmake --build --preset win-amd64-release --parallel
```

The build produces `burnoutrevenge.exe`, which loads its configured game
directory automatically (edit `game_data_root` in `burnoutrevenge.toml`, or
override it in `src/burnoutrevenge_app.h`'s `OnConfigurePaths`) - no
`--game_data_root` command-line argument is required at runtime.

## Diagnostics

The vendored SDK includes cvar-gated GPU diagnostics (off by default, so they
don't spam production logs) built while investigating the issues above.
Enable them in `burnoutrevenge.toml` or via the CLI to reuse the same
investigation approach on any game built on this SDK:

| cvar | Logs |
|---|---|
| `diag_present_rate` | Real `Present()` calls/sec, split by trigger source - catches double-presentation/frame-pacing bugs. |
| `diag_draw_rate` | PM4 draw packets processed/sec - catches runaway resubmission loops. |
| `diag_gamma_ramp` | Scanout gamma ramp values as the guest writes them (256-entry table and PWL paths). |
| `diag_rt_format` | Active color render target format, logged on change. |
| `diag_blend_constant` | Constant blend color (fixed-function and ROV paths), logged on change. |

## Development notes

- [Project state and session log](docs/PROJECT_STATE.md)
- [Xbox 360/ReXGlue recompilation methodology](docs/METHODOLOGY_XBOX360.md)
- [Pattern-matching rule](docs/MATCHING_XBOX360.md)
- [Hardware/ABI references](docs/XBOX360_HARDWARE_REFERENCES.md)
- [Phase plan](docs/BURNOUT_REVENGE_PLAN.md)

## Repository policy

Do not commit extracted game files, generated recompiled source
(`generated/`), user data, saves, diagnostic captures, or compiled binaries
(`out/`). Inspect every staged change before publishing it.

This project is not affiliated with or endorsed by Microsoft, Xbox, Electronic
Arts, Criterion Games, or the Burnout franchise.
