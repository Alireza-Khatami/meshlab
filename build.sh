#!/usr/bin/env bash
set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
BUILD_DIR="$SCRIPT_DIR/build"

VCPKG_ROOT="C:/Users/alirz/Projects/vcpkg"
VCPKG_TOOLCHAIN="$VCPKG_ROOT/scripts/buildsystems/vcpkg.cmake"
QT5_DIR="$VCPKG_ROOT/vcpkg_installed/x64-windows/share/cmake/Qt5"

echo "==> Initialising submodules (vcglib)..."
git -C "$SCRIPT_DIR" submodule update --init --recursive

echo "==> Configuring (CMake)..."
cmake \
  -B "$BUILD_DIR" \
  -G "Visual Studio 17 2022" \
  -A x64 \
  -DCMAKE_TOOLCHAIN_FILE="$VCPKG_TOOLCHAIN" \
  -DQt5_DIR="$QT5_DIR" \
  "$SCRIPT_DIR"

echo "==> Building (Debug)..."
cmake --build "$BUILD_DIR" --config Debug --parallel \
  -- /p:WarningLevel=0 /verbosity:minimal /nologo

echo ""
echo "Build finished. Binaries are in: $BUILD_DIR/src/distrib/"



# Build succeeded. Here's a summary of what was produced:

# meshlab.exe — main application at build\src\distrib\Debug\meshlab.exe
# UseCPUOpenGL.exe — software renderer helper
# All core DLLs — meshlab-common.dll, meshlab-common-gui.dll, plus external libs (GLEW, lib3ds, lib3mf, muparser, E57, xerces)
# .pdb debug symbols alongside every binary for full debugger support
# Plugins built under build\src\distrib\plugins\Debug\


#The Qt5 DLLs need to be deployed alongside meshlab.exe. The easiest fix is to run windeployqt from the vcpkg Qt installation:

echo "==> Deploying Qt5 runtime DLLs (windeployqt)..."
WINDEPLOYQT="$VCPKG_ROOT/vcpkg_installed/x64-windows/tools/qt5/debug/bin/windeployqt.exe"
MESHLAB_EXE="$BUILD_DIR/src/distrib/Debug/meshlab.exe"

"$WINDEPLOYQT" --debug "$MESHLAB_EXE"

echo ""
echo "Qt5 deployment complete. meshlab.exe is ready to run."

echo "==> Copying missing runtime DLLs (zlib, OpenSSL, Qt platform plugin)..."
VCPKG_DEBUG_BIN="$VCPKG_ROOT/vcpkg_installed/x64-windows/debug/bin"
VCPKG_RELEASE_BIN="$VCPKG_ROOT/vcpkg_installed/x64-windows/bin"
VCPKG_DEBUG_PLUGINS="$VCPKG_ROOT/vcpkg_installed/x64-windows/debug/plugins"
DISTRIB_DEBUG="$BUILD_DIR/src/distrib/Debug"

cp "$VCPKG_DEBUG_BIN/libssl-3-x64.dll"    "$DISTRIB_DEBUG/"
cp "$VCPKG_DEBUG_BIN/libcrypto-3-x64.dll" "$DISTRIB_DEBUG/"
cp "$VCPKG_RELEASE_BIN/zlib1.dll"         "$DISTRIB_DEBUG/"

mkdir -p "$DISTRIB_DEBUG/platforms"
cp "$VCPKG_DEBUG_PLUGINS/platforms/qwindowsd.dll" "$DISTRIB_DEBUG/platforms/"

echo "==> Adding vcpkg bin dirs to PATH..."
export PATH="$VCPKG_DEBUG_BIN:$VCPKG_RELEASE_BIN:$PATH"

echo "==> Copying plugins to distrib/Debug/plugins/ (next to meshlab.exe)..."
mkdir -p "$DISTRIB_DEBUG/plugins"
cp "$BUILD_DIR/src/distrib/plugins/Debug/"*.dll "$DISTRIB_DEBUG/plugins/"


# Copied all 75 plugin DLLs to the correct location distrib/Debug/plugins/ immediately so you can use MeshLab right now
# Added the copy step to build.sh so it happens automatically on every future build
# Try opening an .obj or .off file now — it should work. If MeshLab still doesn't show them in the open dialog, go to Edit → Preferences and check the plugin path is pointing to build\src\distrib\Debug\plugins.


echo ""
echo "All done. Run: $DISTRIB_DEBUG/meshlab.exe"
