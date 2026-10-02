#!/usr/bin/env bash
# Reuse the verified stage-4 dependency installations, never their source/build directories.
set -eu
base=${1:?stage-6 work directory}
previous=${2:?verified stage-4 dependency directory}
vendor=${3:?SDL3/bgfx/FFmpeg vendor directory}
arch=${4:?x64 or arm64}
mkdir -p "$base/evidence"
common=( -DS2_BUILD_GAME=ON -DCMAKE_BUILD_TYPE=RelWithDebInfo
  -DS2_BGFX_CMAKE_ROOT="$vendor/bgfx.cmake-probe"
  -DS2_BGFX_DIR="$vendor/bgfx-81d81fba72c42d348c589514c774bbfe01e110fa"
  -DS2_BIMG_DIR="$vendor/bimg-87aaad3ac882e741889fdd4263224e5d12c26f99"
  -DS2_BX_DIR="$vendor/bx-25315498841259323e18f549d1ad9d9aba6632cc"
  -DS2_BUILD_MEDIA_PROBE=ON -DS2_BUILD_MINIAUDIO_MUSIC_PROBE=ON
  -DS2_MINIAUDIO_INCLUDE_DIR="$vendor"
  -DS2_GAME_DB_PATH=/tmp/s2-matrix-links.zCkO0O/game.db
  -DS2_RESOURCE_PACKAGE_PATH=/tmp/s2-matrix-links.zCkO0O/Waypoints.res
  -DS2_GAME_DIR="$previous/runs/x64-vulkan-final-02/game"
  -DS2_SCRIPT_CORPUS_DIR=/tmp/s2-matrix-links.zCkO0O/lua-scripts )
if [ -d "$base/freetype-src" ]; then
  common+=( -DS2_FREETYPE_SOURCE_DIR="$base/freetype-src" )
fi
if [ "$arch" = x64 ]; then
  sdl="$vendor/sdl3-x11-install"
  [ ! -d "$base/sdl-x64" ] || sdl="$base/sdl-x64"
  extra=( -DCMAKE_C_COMPILER=clang-18 -DCMAKE_CXX_COMPILER=clang++-18
    -DSDL3_DIR="$sdl/lib/cmake/SDL3"
    -DS2_FFMPEG_ROOT="$vendor/ffmpeg-8.1-install" )
elif [ "$arch" = arm64 ]; then
  arm="$previous/arm64"
  export PKG_CONFIG_SYSROOT_DIR="$arm/sysroot"
  export PKG_CONFIG_LIBDIR="$arm/sysroot/usr/lib/aarch64-linux-gnu/pkgconfig:$arm/sysroot/usr/share/pkgconfig"
  extra=( -DCMAKE_TOOLCHAIN_FILE="$base/src/cmake/Toolchains/LinuxArm64Clang.cmake"
    -DS2_ARM64_SYSROOT="$arm/sysroot" -DS2_ARM64_GCC_TOOLCHAIN=/usr
    -DCMAKE_CROSSCOMPILING_EMULATOR="qemu-aarch64-static;-L;$arm/sysroot;-E;LD_LIBRARY_PATH=/opt/s2/sdl/lib:/opt/s2/ffmpeg/lib:/usr/lib/aarch64-linux-gnu"
    -DSDL3_DIR="$arm/sysroot/opt/s2/sdl/lib/cmake/SDL3"
    -DS2_FFMPEG_ROOT="$arm/sysroot/opt/s2/ffmpeg"
    -DS2_BGFX_SHADERC="$previous/build-x64-clang18/bgfx/cmake/bgfx/shaderc" )
else exit 2; fi
cmake -S "$base/src" -B "$base/build-$arch" -G Ninja "${common[@]}" "${extra[@]}" > "$base/evidence/config-$arch.log" 2>&1
cmake --build "$base/build-$arch" -j16 > "$base/evidence/build-$arch.log" 2>&1
