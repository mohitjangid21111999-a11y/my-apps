#!/usr/bin/env bash
set -euo pipefail
# This cloud machine's prepared dependencies live outside the checkout.
cd "$(dirname "$0")/.."
export CPATH=/workspace/sysroot/usr/include:/workspace/sysroot/usr/include/freetype2:/workspace/sysroot/usr/include/x86_64-linux-gnu
export LIBRARY_PATH=/workspace/sysroot/usr/lib/x86_64-linux-gnu
export PKG_CONFIG_PATH=/workspace/sysroot/usr/lib/x86_64-linux-gnu/pkgconfig
export PKG_CONFIG_SYSROOT_DIR=/workspace/sysroot
cm=/workspace/build-tools/cmake/data/bin/cmake
"$cm" -S . -B build -DFETCHCONTENT_SOURCE_DIR_JUCE=/workspace/JUCE -DCMAKE_BUILD_TYPE=Release -DCMAKE_PREFIX_PATH=/workspace/sysroot/usr -DMOVOX_VALIDATE_HOST=ON
"$cm" --build build -j 3
/workspace/build-tools/cmake/data/bin/ctest --test-dir build --output-on-failure
