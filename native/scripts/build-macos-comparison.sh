#!/usr/bin/env bash
# Native Apple Silicon debug build with active assertions, tests and app packaging.
set -euo pipefail
[[ "$(uname -s)" == Darwin && "$(uname -m)" == arm64 ]] || {
  echo 'Run this script in a native Terminal on an Apple Silicon Mac (not Rosetta).' >&2; exit 1;
}
xcode-select -p >/dev/null 2>&1 || {
  echo 'Install Apple command line tools with xcode-select --install, then retry.' >&2; exit 1;
}
if ! command -v brew >/dev/null && [[ -x /opt/homebrew/bin/brew ]]; then
  eval "$(/opt/homebrew/bin/brew shellenv)"
fi
command -v brew >/dev/null || {
  echo 'Install Homebrew from https://brew.sh, then retry.' >&2; exit 1;
}
[[ "$(brew --prefix)" == /opt/homebrew ]] || {
  echo 'Use the native Apple Silicon Homebrew installation in /opt/homebrew.' >&2; exit 1;
}
brew install qt cmake ninja nasm autoconf automake libtool pkg-config openssl@3
NATIVE_DIR="$(cd "$(dirname "$0")/.." && pwd)"
BUILD_DIR="$NATIVE_DIR/build-comparison-arm64"
mkdir -p "$BUILD_DIR"
# Keep the complete output for troubleshooting a local build.
exec > >(tee "$BUILD_DIR/build.log") 2>&1
cmake -S "$NATIVE_DIR" -B "$BUILD_DIR" -G Ninja \
  -DCMAKE_BUILD_TYPE=Debug -DBUILD_TESTING=ON \
  -DCMAKE_OSX_ARCHITECTURES=arm64 \
  -DCMAKE_PREFIX_PATH="$(brew --prefix qt)" \
  -DOPENSSL_ROOT_DIR="$(brew --prefix openssl@3)"
cmake --build "$BUILD_DIR" --parallel 3
ctest --test-dir "$BUILD_DIR" --output-on-failure
# Use a new staging directory on each run to avoid merging old app bundles.
PACKAGE_DIR="$(mktemp -d "$BUILD_DIR/package.XXXXXX")"
APP_PATH="$PACKAGE_DIR/FreeSCP.app"
cp -R "$BUILD_DIR/ui-qt/winscp-qt.app" "$APP_PATH"
"$(brew --prefix qt)/bin/macdeployqt" "$APP_PATH" -no-strip
codesign --force --deep --sign - "$APP_PATH"
codesign --verify --deep --strict "$APP_PATH"
lipo -verify_arch arm64 "$APP_PATH/Contents/MacOS/winscp-qt"
ditto -c -k --keepParent "$APP_PATH" "$PACKAGE_DIR/FreeSCP-macOS-arm64.zip"
printf '\nApplication: %s\nZIP: %s\nLog: %s\n' "$APP_PATH" "$PACKAGE_DIR/FreeSCP-macOS-arm64.zip" "$BUILD_DIR/build.log"
echo 'Development build with ad-hoc signing; not notarized by Apple.'
open -R "$APP_PATH"
