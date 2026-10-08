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

Builds universal (arm64 + x86_64) AU, VST3 and AAX, signs them, builds a signed `.pkg`, then notarizes and staples it. The installer offers each format as a choice: AU and VST3 go to `/Library/Audio/Plug-Ins`, AAX to `/Library/Application Support/Avid/Audio/Plug-Ins`. Output: `dist/Vibetron-VT-369-<version>.pkg`.

Signing requirements:

- Developer ID Application and Installer certificates, and the `vibetronics` notarytool keychain profile.
- For AAX: the PACE Eden tools (`wraptool`), an iLok holding your PACE signing certificate (run Synchronize in iLok License Manager if `wraptool` can't find it), and `PACE_ACCOUNT` / `PACE_WCGUID` set in the environment or in an untracked `scripts/pace.env`. `wraptool` applies both the PACE and Developer ID signatures, so the AAX loads in retail Pro Tools. See the header of `scripts/package_macos.sh`.

To publish, bump the version in `CMakeLists.txt` (`project(Vibetron VERSION ...)`), commit and push, then run:

```bash
./scripts/package_macos.sh --publish
```

This also tags `v<version>` and creates a GitHub release with the notarized `.pkg` attached. It refuses to run from uncommitted or unpushed changes.

The plug-in's ABOUT panel checks `api.github.com/repos/squeaksquad/Vibetron/releases/latest` (once per host session, and on demand) and shows a red dot when that release's tag is newer than the running build; DOWNLOAD UPDATE opens the release's `.pkg`. This only works while the repo is public.

Pushing the `v<version>` tag also triggers the **Windows installer** workflow (`.github/workflows/windows.yml`), which builds the VST3 and Standalone with MSVC, packages them with Inno Setup (`scripts/windows/installer.iss`) and attaches `Vibetron-VT-369-<version>-Windows.exe` to the same release. It can also be run by hand from the Actions tab; the installer is then kept as a workflow artifact. The Windows build is unsigned, so SmartScreen warns on first run. The workflow pins JUCE to a commit; update `JUCE_REF` when you update your local JUCE checkout.

## Layout

- `Source/` — processor, limiter, meters, knob, calibration panel, textures
- `scripts/package_macos.sh` — release pipeline
- `mockups/` — the original HTML design directions

## License

Vibetron is free software, licensed under the [GNU Affero General Public License v3.0](LICENSE). You may use, modify and redistribute it, including commercially, provided any version you distribute is released under the same license with its complete source. It is built on [JUCE](https://juce.com), used here under JUCE's AGPLv3 option.
