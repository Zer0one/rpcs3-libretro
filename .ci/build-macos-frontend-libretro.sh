#!/bin/sh
set -eu

# Local core build against the exact MoltenVK already shipped by the frontend.
: "${RPCS3_RETROARCH_APP:?Set RPCS3_RETROARCH_APP to the RetroArch application bundle}"
RPCS3_SOURCE=$(CDPATH= cd -- "$(dirname -- "$0")/.." && pwd)
RPCS3_BUILD_DIR=${RPCS3_BUILD_DIR:-"$RPCS3_SOURCE/build-macos-arm64"}
RPCS3_LLVM_PREFIX=${RPCS3_LLVM_PREFIX:-$(brew --prefix llvm@22)}
RPCS3_MVK="$RPCS3_RETROARCH_APP/Contents/Frameworks/MoltenVK.framework/Versions/A/MoltenVK"
RPCS3_TARGET=${RPCS3_MACOS_DEPLOYMENT_TARGET:-$(sw_vers -productVersion | cut -d. -f1)}
test -f "$RPCS3_MVK"

cmake -S "$RPCS3_SOURCE" -B "$RPCS3_BUILD_DIR" -G Ninja \
    -DCMAKE_BUILD_TYPE=Release \
    -DCMAKE_C_COMPILER="$RPCS3_LLVM_PREFIX/bin/clang" \
    -DCMAKE_CXX_COMPILER="$RPCS3_LLVM_PREFIX/bin/clang++" \
    -DCMAKE_OSX_ARCHITECTURES=arm64 \
    -DCMAKE_OSX_DEPLOYMENT_TARGET="$RPCS3_TARGET" \
    -DCMAKE_OSX_SYSROOT="$(xcrun --sdk macosx --show-sdk-path)" \
    -DCMAKE_PREFIX_PATH="$(brew --prefix)" \
    -DWITH_LLVM=ON -DBUILD_LLVM=OFF -DSTATIC_LINK_LLVM=ON \
    -DLLVM_DIR="$RPCS3_LLVM_PREFIX/lib/cmake/llvm" \
    -DUSE_NATIVE_INSTRUCTIONS=OFF -DUSE_PRECOMPILED_HEADERS=OFF \
    -DUSE_SYSTEM_FFMPEG=OFF -DUSE_SYSTEM_CURL=OFF \
    -DCURL_BROTLI=OFF -DCURL_ZSTD=OFF -DUSE_NGHTTP2=OFF \
    -DUSE_SDL=OFF -DUSE_FAUDIO=OFF -DUSE_DISCORD_RPC=OFF -DUSE_LTO=OFF \
    -DUSE_SYSTEM_MVK=ON -DVulkan_INCLUDE_DIR="$(brew --prefix)/include" \
    -DVulkan_LIBRARY="$RPCS3_MVK"
cmake --build "$RPCS3_BUILD_DIR" --target rpcs3_libretro -j "${RPCS3_BUILD_JOBS:-4}"

RPCS3_DIST="$RPCS3_BUILD_DIR/dist"
mkdir -p "$RPCS3_DIST"
RPCS3_CORE="$RPCS3_DIST/rpcs3_libretro.dylib"
cp "$RPCS3_BUILD_DIR/rpcs3/libretro/rpcs3_libretro.dylib" "$RPCS3_CORE"
cp "$RPCS3_SOURCE/rpcs3/libretro/rpcs3_libretro.info" "$RPCS3_DIST/"
# LLVM's optional compression library travels with the core. MoltenVK does not:
# its @rpath/MoltenVK install name resolves to RetroArch's existing framework.
RPCS3_ZSTD=$(otool -L "$RPCS3_CORE" | awk '/libzstd/ { print $1; exit }')
if [ -n "$RPCS3_ZSTD" ]; then
    cp "$RPCS3_ZSTD" "$RPCS3_DIST/libzstd_macos_arm64.dylib"
    chmod u+w "$RPCS3_DIST/libzstd_macos_arm64.dylib"
    install_name_tool -id @loader_path/libzstd_macos_arm64.dylib "$RPCS3_DIST/libzstd_macos_arm64.dylib"
    install_name_tool -change "$RPCS3_ZSTD" @loader_path/libzstd_macos_arm64.dylib "$RPCS3_CORE"
    codesign -f -s - "$RPCS3_DIST/libzstd_macos_arm64.dylib"
fi
codesign -f -s - "$RPCS3_CORE"
otool -L "$RPCS3_CORE"
if otool -L "$RPCS3_CORE" | grep -Eq '/opt/homebrew|/usr/local'; then
    echo "Core still depends on a package-manager library" >&2
    exit 1
fi
codesign --verify "$RPCS3_CORE"
