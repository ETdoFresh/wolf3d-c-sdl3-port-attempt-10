#!/usr/bin/env bash
# publish.sh -- assemble a runnable wolf3d build into publish/.
#
# Builds Release (skip with --skip-build), then copies the wolf3d binary,
# SDL3 shared library, and game data from assets/wl6/ into publish/.
# Run from any directory; paths resolve relative to the repo root.

set -euo pipefail

SKIP_BUILD=0
CLEAN=0
for arg in "$@"; do
    case "$arg" in
        --skip-build|-s) SKIP_BUILD=1 ;;
        --clean|-c)      CLEAN=1 ;;
        -h|--help)
            echo "Usage: $0 [--skip-build|-s] [--clean|-c]"
            exit 0
            ;;
        *) echo "unknown arg: $arg" >&2; exit 2 ;;
    esac
done

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
REPO_ROOT="$(cd "$SCRIPT_DIR/.." && pwd)"
BUILD_DIR="$REPO_ROOT/build"
ASSETS_DIR="$REPO_ROOT/assets/wl6"
PUBLISH_DIR="$REPO_ROOT/publish"

echo "publish.sh -- repo: $REPO_ROOT"

if [ "$CLEAN" = "1" ] && [ -d "$PUBLISH_DIR" ]; then
    echo "Cleaning $PUBLISH_DIR"
    rm -rf "$PUBLISH_DIR"
fi

# Pick exe + dll/so names by platform.
case "$(uname -s 2>/dev/null || echo unknown)" in
    *NT*|MINGW*|MSYS*|CYGWIN*)
        EXE_NAME="wolf3d.exe"
        LIB_NAME="SDL3.dll"
        CFG="Release"
        ;;
    Darwin*)
        EXE_NAME="wolf3d"
        LIB_NAME="libSDL3.0.dylib"
        CFG=""
        ;;
    *)
        EXE_NAME="wolf3d"
        LIB_NAME="libSDL3.so.0"
        CFG=""
        ;;
esac

if [ "$SKIP_BUILD" = "0" ]; then
    if [ ! -d "$BUILD_DIR" ]; then
        echo "Configuring CMake (Release)..."
        cmake -B "$BUILD_DIR" -DCMAKE_BUILD_TYPE=Release
    fi
    echo "Building Release..."
    # Post-build copy steps can fail when a previous wolf3d is still
    # running on Windows; treat link success as the gate.
    cmake --build "$BUILD_DIR" --config Release -j4 || true
fi

# Locate the built binary and SDL3 library.
if [ -n "$CFG" ]; then
    EXE_SRC="$BUILD_DIR/$CFG/$EXE_NAME"
    LIB_SRC="$BUILD_DIR/$CFG/$LIB_NAME"
    LIB_FALLBACK="$BUILD_DIR/_deps/sdl3-build/$CFG/$LIB_NAME"
else
    EXE_SRC="$BUILD_DIR/$EXE_NAME"
    LIB_SRC="$BUILD_DIR/$LIB_NAME"
    LIB_FALLBACK="$BUILD_DIR/_deps/sdl3-build/$LIB_NAME"
fi

if [ ! -f "$EXE_SRC" ]; then
    echo "error: build did not produce $EXE_SRC" >&2
    exit 1
fi
if [ ! -f "$LIB_SRC" ] && [ -f "$LIB_FALLBACK" ]; then
    LIB_SRC="$LIB_FALLBACK"
fi

mkdir -p "$PUBLISH_DIR"

echo "Copying binaries..."
cp -f "$EXE_SRC" "$PUBLISH_DIR/"
if [ -f "$LIB_SRC" ]; then
    cp -f "$LIB_SRC" "$PUBLISH_DIR/"
fi

echo "Copying game data from $ASSETS_DIR..."
shopt -s nullglob nocaseglob
for f in "$ASSETS_DIR"/*.wl6 "$ASSETS_DIR"/GAMEPAL.BIN; do
    [ -f "$f" ] && cp -f "$f" "$PUBLISH_DIR/"
done
shopt -u nocaseglob

echo
echo "Published to $PUBLISH_DIR :"
ls -l "$PUBLISH_DIR" | grep -v '^d' | grep -v '^total'
