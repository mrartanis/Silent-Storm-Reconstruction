#!/usr/bin/env bash
set -euo pipefail
deps=$1
platform=$2
jobs=${S2_BUILD_JOBS:-4}
if [[ "$platform" == windows ]]; then
  deps=$(cygpath -u "$deps")
  # MSYS also has a link program. FFmpeg must find the MSVC linker first.
  export PATH="$(cygpath -u "$VCToolsInstallDir")/bin/Hostx64/x64:$PATH"
  toolchain=(--toolchain=msvc --arch=x86_64 --target-os=win32)
else
  toolchain=()
  cmake -S "$deps/SDL3-3.4.16" -B "$deps/sdl-build" -G Ninja \
    -DCMAKE_BUILD_TYPE=Release -DCMAKE_INSTALL_PREFIX="$deps/install" \
    -DSDL_SHARED=ON -DSDL_STATIC=OFF -DSDL_TESTS=OFF -DSDL_X11=ON -DSDL_X11_XINPUT=ON -DSDL_WAYLAND=OFF
  cmake --build "$deps/sdl-build" --parallel "$jobs"
  cmake --install "$deps/sdl-build"
fi
mkdir -p "$deps/ffmpeg-build"
cd "$deps/ffmpeg-build"
# No GPL/nonfree components or automatically detected third-party codecs.
# Native Bink, PCM and Vorbis decoders remain enabled.
../ffmpeg-8.1/configure --prefix="$deps/install" "${toolchain[@]}" \
  --disable-autodetect --enable-shared --disable-static --disable-programs \
  --disable-doc --disable-network --disable-encoders --disable-muxers \
  --disable-avdevice --disable-avfilter --disable-x86asm
make -j"$jobs"
make install
if [[ "$platform" == windows ]]; then
  # FFmpeg's MSVC install puts import libraries beside the DLLs in bin.
  # Expose the conventional development layout expected by MediaProbe.cmake.
  mkdir -p "$deps/install/lib"
  for component in avformat avcodec avutil swscale swresample; do
    cp "$deps/install/bin/$component.lib" "$deps/install/lib/"
  done
fi
# Keep the build configuration alongside the exact source shipped in releases.
cp ffbuild/config.log "$deps/ffmpeg-config.log"
