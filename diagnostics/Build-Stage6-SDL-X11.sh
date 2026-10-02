#!/usr/bin/env bash
# Native x64 SDL with XInput2: relative mouse input must not depend on root size.
set -eu
base=${1:?stage-6 directory}
source=${2:?SDL 3.4.16 source directory}
sdk=${3:-}
extra=()
if [ -n "$sdk" ]; then
  extra=( -DCMAKE_C_FLAGS="-I$sdk/usr/include"
    -DCMAKE_INCLUDE_PATH="$sdk/usr/include"
    -DCMAKE_LIBRARY_PATH="$sdk/usr/lib/x86_64-linux-gnu" )
fi
mkdir -p "$base/evidence"
cmake -S "$source" -B "$base/sdl-x64-build" -G Ninja \
  -DCMAKE_BUILD_TYPE=RelWithDebInfo -DCMAKE_INSTALL_PREFIX="$base/sdl-x64" \
  -DSDL_X11=ON -DSDL_X11_XINPUT=ON -DSDL_WAYLAND=OFF -DSDL_X11_XTEST=OFF \
  -DSDL_TESTS=OFF -DSDL_EXAMPLES=OFF -DSDL_SHARED=ON -DSDL_STATIC=OFF \
  "${extra[@]}" > "$base/evidence/config-sdl-xinput.log" 2>&1
cmake --build "$base/sdl-x64-build" -j16 > "$base/evidence/build-sdl-xinput.log" 2>&1
cmake --install "$base/sdl-x64-build" > "$base/evidence/install-sdl-xinput.log" 2>&1
