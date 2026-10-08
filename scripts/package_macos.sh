#!/bin/bash
# package_macos.sh — universal build, sign, package, notarize and staple the Vibetron VT-369 installer.
# Produces dist/Vibetron-VT-369-<version>.pkg installing the AU and VST3 into /Library/Audio/Plug-Ins
# and the PACE-signed AAX into /Library/Application Support/Avid/Audio/Plug-Ins.
#
# AAX signing needs PACE_ACCOUNT (iLok account ID) and PACE_WCGUID (wrap config GUID from PACE Central),
# from the environment or scripts/pace.env (untracked). Store the account password in wraptool's
# keychain once with: wraptool sync --account <id> --password <password>
#
# Usage: ./scripts/package_macos.sh [--publish]
#   --publish  also tag v<version> and publish a GitHub release with the .pkg attached
#              (requires a clean, pushed working tree; bump the version in CMakeLists.txt first)

set -euo pipefail
cd "$(dirname "$0")/.."

TEAM_ID="H674XZA5GP"
APP_SIGN_ID="Developer ID Application: Bryan DiMaio ($TEAM_ID)"
PKG_SIGN_ID="Developer ID Installer: Bryan DiMaio ($TEAM_ID)"
NOTARY_PROFILE="vibetronics"
WRAPTOOL="/Applications/PACEAntiPiracy/Eden/Fusion/Current/bin/wraptool"
[[ -f scripts/pace.env ]] && source scripts/pace.env

PRODUCT="Vibetron VT-369"
PKG_ID_BASE="com.vibetron.vt369"
VERSION=$(sed -nE 's/^project\(Vibetron VERSION ([0-9.]+)\).*/\1/p' CMakeLists.txt)
BUILD_DIR="build-release"
ARTEFACTS="$BUILD_DIR/Vibetron_artefacts/Release"
WORK="$BUILD_DIR/pkg-work"
DIST="dist"
PKG_OUT="$DIST/Vibetron-VT-369-$VERSION.pkg"
TAG="v$VERSION"

PUBLISH=0
for arg in "$@"; do
    case "$arg" in
        --publish) PUBLISH=1 ;;
        *) echo "Unknown option: $arg"; exit 2 ;;
    esac
done

if [[ "$PUBLISH" == 1 ]]; then
    # A release must correspond to a commit that exists on GitHub.
    git diff --quiet && git diff --cached --quiet || { echo "Commit your changes before publishing."; exit 1; }
    git fetch -q origin
    [[ "$(git rev-parse HEAD)" == "$(git rev-parse @{u})" ]] || { echo "Push main before publishing."; exit 1; }
    if git rev-parse -q --verify "refs/tags/$TAG" > /dev/null && [[ "$(git rev-list -n1 "$TAG")" != "$(git rev-parse HEAD)" ]]; then
        echo "Tag $TAG already points at another commit. Bump the version in CMakeLists.txt."; exit 1
    fi
fi

[[ -x "$WRAPTOOL" ]] || { echo "wraptool not found at $WRAPTOOL"; exit 1; }
[[ -n "${PACE_ACCOUNT:-}" && -n "${PACE_WCGUID:-}" ]] || { echo "Set PACE_ACCOUNT and PACE_WCGUID (environment or scripts/pace.env)."; exit 1; }

echo "=== 1. Universal release build (arm64 + x86_64) ==="
cmake -B "$BUILD_DIR" -G Xcode \
    -DCMAKE_OSX_ARCHITECTURES="arm64;x86_64" \
    -DVIBETRON_INSTALL_AAX=OFF > /dev/null
cmake --build "$BUILD_DIR" --config Release --target Vibetron_AU Vibetron_VST3 Vibetron_AAX -- -quiet

AU="$ARTEFACTS/AU/$PRODUCT.component"
VST3="$ARTEFACTS/VST3/$PRODUCT.vst3"
AAX="$ARTEFACTS/AAX/$PRODUCT.aaxplugin"
for b in "$AU" "$VST3" "$AAX"; do
    [[ -d "$b" ]] || { echo "Missing build output: $b"; exit 1; }
    lipo -info "$b/Contents/MacOS/$PRODUCT"
done

echo "=== 2. Stage and sign inside-out (hardened runtime + timestamp) ==="
rm -rf "$WORK" && mkdir -p "$WORK/au-root" "$WORK/vst3-root" "$WORK/aax-root"
ditto "$AU" "$WORK/au-root/$PRODUCT.component"
ditto "$VST3" "$WORK/vst3-root/$PRODUCT.vst3"
for b in "$WORK/au-root/$PRODUCT.component" "$WORK/vst3-root/$PRODUCT.vst3"; do
    find "$b" \( -name "*.dylib" -o -name "*.so" \) -print0 | while IFS= read -r -d '' f; do
        codesign --force --sign "$APP_SIGN_ID" --options runtime --timestamp "$f"
    done
    codesign --force --sign "$APP_SIGN_ID" --options runtime --timestamp "$b/Contents/MacOS/$PRODUCT"
    codesign --force --sign "$APP_SIGN_ID" --options runtime --timestamp "$b"
    codesign --verify --deep --strict --verbose=2 "$b"
done

# AAX: wraptool applies the PACE signature and the Developer ID signature together.
# Don't codesign the bundle afterwards; that would invalidate the PACE signature.
ditto "$AAX" "$WORK/aax-root/$PRODUCT.aaxplugin"
"$WRAPTOOL" sign --verbose --account "$PACE_ACCOUNT" --wcguid "$PACE_WCGUID" --signid "$APP_SIGN_ID" \
    --allowsigningservice --dsigharden --dsig1-compat off \
    --in "$WORK/aax-root/$PRODUCT.aaxplugin" --out "$WORK/aax-root/$PRODUCT.aaxplugin"
"$WRAPTOOL" verify --in "$WORK/aax-root/$PRODUCT.aaxplugin"
codesign --verify --deep --strict --verbose=2 "$WORK/aax-root/$PRODUCT.aaxplugin"

echo "=== 3. Component packages (non-relocatable) ==="
make_component_pkg () {   # root, install location, identifier, output
    local root="$1" location="$2" ident="$3" out="$4"
    local plist="$WORK/$(basename "$out" .pkg).plist"
    pkgbuild --analyze --root "$root" "$plist" > /dev/null
    # Never let Installer "update" a same-ID bundle found elsewhere on disk (e.g. a dev build).
    /usr/libexec/PlistBuddy -c "Delete :0:BundleIsRelocatable" "$plist" 2>/dev/null || true
    /usr/libexec/PlistBuddy -c "Add :0:BundleIsRelocatable bool false" "$plist"
    pkgbuild --root "$root" --component-plist "$plist" --install-location "$location" \
             --identifier "$ident" --version "$VERSION" "$out"
}
make_component_pkg "$WORK/au-root" "/Library/Audio/Plug-Ins/Components" "$PKG_ID_BASE.au" "$WORK/au.pkg"
make_component_pkg "$WORK/vst3-root" "/Library/Audio/Plug-Ins/VST3" "$PKG_ID_BASE.vst3" "$WORK/vst3.pkg"
make_component_pkg "$WORK/aax-root" "/Library/Application Support/Avid/Audio/Plug-Ins" "$PKG_ID_BASE.aax" "$WORK/aax.pkg"

echo "=== 4. Product installer (choices: AU, VST3, AAX) ==="
cat > "$WORK/distribution.xml" <<EOF
<?xml version="1.0" encoding="utf-8"?>
<installer-gui-script minSpecVersion="2">
    <title>$PRODUCT</title>
    <organization>$PKG_ID_BASE</organization>
    <domains enable_localSystem="true"/>
    <options customize="allow" require-scripts="false" hostArchitectures="arm64,x86_64"/>
    <os-version min="11.0"/>
    <choices-outline>
        <line choice="au"/>
        <line choice="vst3"/>
        <line choice="aax"/>
    </choices-outline>
    <choice id="au" title="Audio Unit (AU)" description="For Logic Pro, GarageBand, REAPER and other AU hosts.">
        <pkg-ref id="$PKG_ID_BASE.au"/>
    </choice>
    <choice id="vst3" title="VST3" description="For REAPER, Ableton Live, Cubase, Studio One and other VST3 hosts.">
        <pkg-ref id="$PKG_ID_BASE.vst3"/>
    </choice>
    <choice id="aax" title="AAX" description="For Pro Tools.">
        <pkg-ref id="$PKG_ID_BASE.aax"/>
    </choice>
    <pkg-ref id="$PKG_ID_BASE.au" version="$VERSION">au.pkg</pkg-ref>
    <pkg-ref id="$PKG_ID_BASE.vst3" version="$VERSION">vst3.pkg</pkg-ref>
    <pkg-ref id="$PKG_ID_BASE.aax" version="$VERSION">aax.pkg</pkg-ref>
</installer-gui-script>
EOF
mkdir -p "$DIST"
rm -f "$PKG_OUT"
productbuild --distribution "$WORK/distribution.xml" --package-path "$WORK" \
             --sign "$PKG_SIGN_ID" --timestamp "$PKG_OUT"
pkgutil --check-signature "$PKG_OUT" | head -4

echo "=== 5. Notarize ==="
RESULT="$WORK/notary_result.json"
xcrun notarytool submit "$PKG_OUT" --keychain-profile "$NOTARY_PROFILE" --wait \
      --output-format json | tee "$RESULT"
STATUS=$(python3 -c "import json,sys; print(json.load(open(sys.argv[1]))['status'])" "$RESULT")
if [[ "$STATUS" != "Accepted" ]]; then
    echo "Notarization failed: $STATUS — fetching log"
    SUB_ID=$(python3 -c "import json,sys; print(json.load(open(sys.argv[1]))['id'])" "$RESULT")
    xcrun notarytool log "$SUB_ID" --keychain-profile "$NOTARY_PROFILE"
    exit 1
fi

echo "=== 6. Staple and validate ==="
xcrun stapler staple "$PKG_OUT"
xcrun stapler validate "$PKG_OUT"
spctl --assess --type install --verbose "$PKG_OUT"

if [[ "$PUBLISH" == 1 ]]; then
    echo "=== 7. Publish GitHub release $TAG ==="
    if gh release view "$TAG" > /dev/null 2>&1; then
        gh release upload "$TAG" "$PKG_OUT" --clobber
    else
        git rev-parse -q --verify "refs/tags/$TAG" > /dev/null || git tag -a "$TAG" -m "$PRODUCT $VERSION"
        git push -q origin "$TAG"
        gh release create "$TAG" "$PKG_OUT" --verify-tag --title "$PRODUCT $VERSION" --notes "$PRODUCT $VERSION installers.

- macOS (.pkg, notarized): AU, VST3 and AAX (Pro Tools), universal (Apple Silicon + Intel), macOS 11 or later. Choose formats via Customize.
- Windows (.exe, 64-bit): VST3 and Standalone. Built and attached by GitHub Actions a few minutes after publishing."
    fi
    gh release view "$TAG" --json url --jq .url
fi

echo "Done: $PKG_OUT"
