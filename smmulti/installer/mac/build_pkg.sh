#!/bin/bash
# Uso (desde la raíz del proyecto, después de compilar):  bash installer/mac/build_pkg.sh
set -euo pipefail
VERSION=1.0.0
ART=build/SoundMotelMultidrive_artefacts/Release
ROOT=build/pkgroot

rm -rf "$ROOT" dist
mkdir -p "$ROOT/Library/Audio/Plug-Ins/VST3" "$ROOT/Library/Audio/Plug-Ins/Components" dist

cp -R "$ART/VST3/Sound Motel Multidrive.vst3"    "$ROOT/Library/Audio/Plug-Ins/VST3/"
cp -R "$ART/AU/Sound Motel Multidrive.component" "$ROOT/Library/Audio/Plug-Ins/Components/"

codesign --force --deep -s - "$ROOT/Library/Audio/Plug-Ins/VST3/Sound Motel Multidrive.vst3"
codesign --force --deep -s - "$ROOT/Library/Audio/Plug-Ins/Components/Sound Motel Multidrive.component"

pkgbuild --root "$ROOT" \
         --identifier com.thesoundmotel.multidrive.pkg \
         --version "$VERSION" \
         --install-location / \
         "dist/SoundMotelMultidrive-$VERSION-macOS.pkg"
echo "Listo: dist/SoundMotelMultidrive-$VERSION-macOS.pkg"
