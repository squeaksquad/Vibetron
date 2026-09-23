# Vibetron VT-369

A stereo audio plug-in (AU / VST3 / AAX) with a "Midnight Glass" hi-fi UI: backlit VU meters, a three-position CHARACTER selector (Silk, Velvet, Obsidian), OPERATE, and an OUTPUT TAME true-peak limiter (-0.1 dBTP).

Built with JUCE 8 and CMake.

## Requirements

- macOS 11+, Xcode, CMake 3.22+
- JUCE 8 at `~/Dev/JUCE` (override with `-DJUCE_DIR=...`)
- Optional: Avid AAX SDK at `~/Development/AAX/aax-sdk-2-9-0` (override with `-DAAX_SDK_DIR=...`). AAX is skipped if it isn't found.

## Development build

```bash
cmake -B build -G Xcode
cmake --build build --config Release
```

AU and VST3 are copied to `~/Library/Audio/Plug-Ins` after each build. The unsigned AAX is staged in `build/aax-staging`; to install it for Pro Tools Developer, configure with `-DVIBETRON_INSTALL_AAX=ON`.

## Release installer

```bash
./scripts/package_macos.sh
```

Builds universal (arm64 + x86_64) AU and VST3, signs them with Developer ID, builds a signed `.pkg` that installs to `/Library/Audio/Plug-Ins`, then notarizes and staples it. Output: `dist/Vibetron-VT-369-<version>.pkg`. Requires the `vibetron` notarytool keychain profile.

Bump the version in `CMakeLists.txt` (`project(Vibetron VERSION ...)`) before each release.

AAX release builds need PACE signing (wraptool) before Apple signing; this isn't wired up yet.

## Layout

- `Source/` — processor, limiter, meters, knob, calibration panel, textures
- `scripts/package_macos.sh` — release pipeline
- `mockups/` — the original HTML design directions
