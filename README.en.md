# Silent Storm Reconstruction

*[Русский](README.md) | English*

Reconstruction and modernization of the **Silent Storm v1.2** engine, based on
[Nival's source code](https://github.com/nival/Silent-Storm), v1.1 PDB files
(RussianPatch1) and analysis of the v1.2 release. The original Silent Storm
source code belongs to Nival. This project aims to preserve the original game's
behavior while bringing its engine to modern 64-bit systems.

## What has been implemented

Status as of **October 23, 2026**:

- **64-bit engine and game.** A complete x64 game build, with fixes for pointer
  size assumptions, legacy inline assembly and 32-bit data structures.
  Portable resource, save and network state formats; original game data loads
  without advance conversion.
- **Replacements for closed libraries.** The game-used functionality of FMOD,
  Bink and LifeStudio/GDP has been reimplemented with open code: FFmpeg/miniaudio
  handle sound and music, FFmpeg decodes original Bink movies, and our native
  LifeStudio implementation handles heads and animation. The modern game build
  requires none of the original libraries' DLLs or SDKs.
- **Native facial animation and FaceGen.** Original heads, muscles, bones,
  macro trees and sequences; expressions, speech, blinking, blending and face
  generation. Geometry and animation are compared against the original x86
  library, including a corpus of **6,780 sequences** for a reference head.
  This is a verified corpus, not a guarantee of identical rendered pixels
  for every possible FaceGen combination.
- **Cross-platform game.** SDL3 handles windows, keyboard, mouse, text input,
  clipboard and system events. The complete game builds for Windows x64,
  Linux x64 and Linux ARM64; the portable core includes Lua, AI, combat,
  resources, campaign logic and saves.
- **bgfx renderer.** The direct D3D9 renderer and DirectInput have been replaced
  by bgfx and SDL3. Windows uses bgfx's Direct3D 11 backend and Linux uses Vulkan;
  the game engine no longer depends on the legacy DirectX runtime.
- **Modern display and interface.** Resizable windows, borderless fullscreen,
  widescreen resolutions and Hor+ projection, adaptive panels and UI scaling,
  FreeType/Liberation vector fonts and aspect-preserving movies. Fixes cover
  inventory, character screens, loading artwork and widescreen hero selection.
  Native system cursors are also available.
- **Graphics and performance.** FXAA on the 3D scene without blurring the UI,
  anisotropic filtering, full texture quality during camera movement/zoom,
  improved directional shadows with filtered edges. Fixes address lighting
  flicker, changing shadows, object highlights, startup rendering without focus
  and hangs when restoring the window. Optimizations cover geometry
  transformation/uploads, GPU buffers, draw batching and framebuffer reuse;
  frame profiling tools are included.
- **HD textures.** **2,837 accepted textures**, enlarged 4× on each axis:
  world/environment, clothing, heads/hair, equipment, weapons, interface and
  effects. Available artwork in the world, heads/hair, clothing and main-menu
  categories is complete; technical masks retain their originals. HD is enabled
  by default and can be toggled without restarting. Load order is mods → HD →
  original. The pack is built from PNGs in Git, with no image generation during
  builds or gameplay. Coverage of all game resources is still incomplete.
- **Two-player tactical multiplayer.** Direct TCP/IPv4 connections, encounter
  map selection, separate preparation bases, squads of 1–6 soldiers,
  levels 1–20 and supply levels 1–14. Character selection, equipment, perks
  and readiness carry into combat. Movement/AP, shooting, inventory, healing,
  mines, grenades, destruction, interrupts and match results are synchronized.
  Quick matches without preparation and Windows ↔ Linux play are supported.
  World replication and network command handling have been optimized.
- **Original gameplay fixes.** Mission entry and random encounters, clues and
  notifications, dialogue, aiming, grenades, Panzerkleins, scripted transitions,
  state loading and resource database mods. Profile names, save-slot names
  and save paths support Unicode.
- **Development tools.** CMake replaces the old Visual Studio solution;
  automated core/renderer/network checks, x86/x64 comparisons, isolated game
  runs, FPS measurements and Windows packaging with game data, libraries,
  fonts and the HD pack.

## Supported platforms

| Platform | Status and verification limits |
| --- | --- |
| **Windows x64** | Main game build; MSVC, SDL3, bgfx/D3D11. Graphics, normal input, audio/video, saves and networking are exercised. |
| **Linux x64** | Complete native game; Clang, SDL3/X11, bgfx/Vulkan. Gameplay and network runs use Xvfb/lavapipe, including Windows ↔ Linux. Physical GPUs, audio devices and HiDPI have not been separately verified. |
| **Linux ARM64** | Complete AArch64 build; game, graphics and core/network checks have run through QEMU and ARM64 lavapipe. Physical ARM64 hardware and a graphical network client on that hardware remain unverified. |
| **Windows x86** | Retained as a diagnostic reference for comparisons with the original 32-bit DLLs, rather than the main distribution target. |
| **macOS** | Game support is not claimed yet: a complete build and run on a real Mac remain unverified. |

A full campaign playthrough and complete behavioral/visual parity with the
Steam release have not yet been established. The reports below describe tested
scenarios and remaining differences.

## Build

Engine releases are separate from original game data and the HD pack. The
**Engine release** workflow builds Windows x64 and Linux x64 for `v*` tags and
publishes a GitHub Release after successful checks. See
[RELEASING.md](packaging/RELEASING.md) for tagging and checks without publishing.

### Dependencies

- **CMake 3.21+**, **Python 3.11+**, **Pillow**. NumPy is only needed to
  calibrate new HD textures; Pillow is sufficient to build the existing pack.
- **SDL3 3.4.16**, development package for the target architecture.
- **bgfx.cmake v1.161.9510-579**, with the pinned bgfx/bimg/bx trees.
  Versions, hashes and directory layout: [SDL3 + bgfx](probes/sdl3-bgfx/README.md).
  Dependencies live outside this repository; CMake does not download bgfx.
- **FFmpeg 8.1**, a shared development package with headers and libraries for
  `avformat`, `avcodec`, `avutil`, `swscale` and `swresample`;
  **miniaudio 0.11.25**, a directory containing `miniaudio.h`.
- **FreeType 2.14.3** is downloaded by CMake with SHA-256 verification.
  Set `S2_FREETYPE_SOURCE_DIR` to use a local source tree. Liberation Fonts 2.1.5
  are bundled in `assets/fonts` and copied into the build.
- **Silent Storm v1.2 game data** for gameplay and resource tests. Use a separate
  complete copy of the installation: `game.db`, `start.cfg`, `tree.mma`, `cfg`,
  `res` and `scripts`. Point `S2_GAME_DIR` at that copy. The versioned
  `network-version.txt` is copied beside the engine and installed with it;
  players do not generate it from their installed game data.

HD is built by default. The generated pack alone occupies about **9.2 GB**;
allow additional space for the PNG-containing clone, dependencies and build.
`-DS2_BUILD_HD_TEXTURES=OFF` skips pack generation, but does not delete an
existing pack or change the in-game HD setting.

### Windows x64

Requires **Visual Studio 2022 or newer**, the **Desktop development with C++**
workload, **MSVC v143** and **Windows SDK 10**. Builds have also been checked
with VS 2026. Run this PowerShell example from the repository root, replacing
the paths with your own. Here `bgfx.cmake` is a complete tree containing
the `bgfx`, `bimg` and `bx` subdirectories.

```powershell
python -m venv build/hd-python
$hdPython = (Resolve-Path build/hd-python/Scripts/python.exe).Path
& $hdPython -m pip install Pillow
$deps = 'C:/s2-deps'

cmake -S . -B build -A x64 `
  "-DSDL3_DIR=$deps/SDL3-3.4.16/cmake" `
  "-DS2_BGFX_CMAKE_ROOT=$deps/bgfx.cmake" `
  "-DS2_BGFX_DIR=$deps/bgfx.cmake/bgfx" `
  "-DS2_BIMG_DIR=$deps/bgfx.cmake/bimg" `
  "-DS2_BX_DIR=$deps/bgfx.cmake/bx" `
  "-DS2_FFMPEG_ROOT=$deps/ffmpeg-8.1" `
  "-DS2_MINIAUDIO_INCLUDE_DIR=$deps/miniaudio-0.11.25" `
  "-DPython3_EXECUTABLE=$hdPython" `
  -DS2_GAME_DIR=C:/Games/SilentStorm-test `
  -DS2_BUILD_MEDIA_PROBE=ON -DS2_BUILD_MINIAUDIO_MUSIC_PROBE=ON `
  -DS2_ENABLE_NATIVE_MUSIC=ON -DS2_ENABLE_NATIVE_SFX=ON `
  -DS2_ENABLE_NATIVE_VIDEO=ON -DS2_BUILD_HD_TEXTURES=ON
cmake --build build --config Release --parallel
```

Outputs: `build/Release/Game.exe`, DLLs, `fonts`, `res-hd` and
`network-version.txt`. All native media flags in this example are needed for
sound, music and movies: the base x64 configuration uses media stubs without
them. The modern build's `binkw32.dll` is our compatibility library compiled
for x64, despite its historical filename.

For debugging, build `RelWithDebInfo` and open `build/A5.sln`.
`build.bat` and `build-debug.bat` wrap Release and RelWithDebInfo builds:
after full configuration they reuse cached dependency paths/options, and
the `SDL3_DIR` environment variable must be set. That variable alone is not
enough for a first build: the bgfx and media settings above are also required.

Additional tools include `DataImport`, `PkgBuilder`, `FontGen`, `TexConv` and
`TexMipStrip`. The legacy `ShaderCompiler` tool optionally requires DirectX SDK
June 2010 and is skipped without it. The game renderer uses bgfx's `shaderc`
and does not require that SDK. `MapEdit`, `Scintilla`, `OpenDynamix` and
`LSConverter` remain in the source tree but are excluded from the build.

### Linux x64

The verified configuration uses **Clang 18**, Ninja, SDL3 with X11/XInput2,
Vulkan, FFmpeg, miniaudio, development packages for **libgsf-1, libpng, zlib,
X11/OpenGL**, and `pkg-config`. XInput2 is required for relative mouse input.
The paths below refer to dependencies prepared in advance.

```sh
python3 -m venv build/hd-python
build/hd-python/bin/python -m pip install Pillow
deps=/opt/s2-deps

cmake -S . -B build-linux -G Ninja \
  -DCMAKE_BUILD_TYPE=RelWithDebInfo \
  -DCMAKE_C_COMPILER=clang-18 -DCMAKE_CXX_COMPILER=clang++-18 \
  -DS2_BUILD_GAME=ON \
  -DSDL3_DIR="$deps/sdl3/lib/cmake/SDL3" \
  -DS2_BGFX_CMAKE_ROOT="$deps/bgfx.cmake" \
  -DS2_BGFX_DIR="$deps/bgfx.cmake/bgfx" \
  -DS2_BIMG_DIR="$deps/bgfx.cmake/bimg" \
  -DS2_BX_DIR="$deps/bgfx.cmake/bx" \
  -DS2_BUILD_MEDIA_PROBE=ON -DS2_BUILD_MINIAUDIO_MUSIC_PROBE=ON \
  -DS2_FFMPEG_ROOT="$deps/ffmpeg-8.1" \
  -DS2_MINIAUDIO_INCLUDE_DIR="$deps/miniaudio-0.11.25" \
  -DPython3_EXECUTABLE="$PWD/build/hd-python/bin/python" \
  -DS2_GAME_DIR=/opt/games/SilentStorm-test \
  -DBGFX_WITH_WAYLAND=OFF -DS2_BUILD_HD_TEXTURES=ON
cmake --build build-linux --parallel
```

Outputs: `build-linux/Game`, `fonts`, `res-hd` and `network-version.txt`.
The Linux game target enables native media; separate Windows
`S2_ENABLE_NATIVE_*` flags are unnecessary. Without `S2_BUILD_GAME=ON`, CMake
builds portable libraries and checks, rather than the complete game.
The loader must be able to find SDL3/FFmpeg shared libraries, for example
through `LD_LIBRARY_PATH`.

### Linux ARM64

Cross-compilation uses
[LinuxArm64Clang.cmake](cmake/Toolchains/LinuxArm64Clang.cmake). You need Clang 18,
an ARM64 GCC toolchain and a populated ARM64 sysroot containing all target
dependencies, including SDL3, FFmpeg, libgsf, libpng and zlib. Add these options
to the Linux configuration above, removing the explicit
`CMAKE_C_COMPILER`/`CMAKE_CXX_COMPILER` options:

```text
-DCMAKE_TOOLCHAIN_FILE=cmake/Toolchains/LinuxArm64Clang.cmake
-DS2_ARM64_SYSROOT=/opt/s2-arm64/sysroot
-DS2_ARM64_GCC_TOOLCHAIN=/usr
-DS2_BGFX_SHADERC=/opt/s2-host/bin/shaderc
```

Use a separate `build-arm64` directory, ARM64 `SDL3_DIR`/`S2_FFMPEG_ROOT`
paths and the sysroot's `PKG_CONFIG_SYSROOT_DIR`/`PKG_CONFIG_LIBDIR`.
`shaderc` and Python run on the build host. Tests through QEMU require
`CMAKE_CROSSCOMPILING_EMULATOR` with `qemu-aarch64-static -L <sysroot>`.
For a complete environment example, see
[Build-Stage6-Linux.sh](diagnostics/Build-Stage6-Linux.sh); this laboratory
script requires prepared dependency directories. Verification and limits:
[BGFX-LINUX.md](diagnostics/BGFX-LINUX.md),
[MODERN-DISPLAY.md](diagnostics/MODERN-DISPLAY.md).

## Running and settings

Copy the executable, **all libraries from your build**, `fonts`, the complete
`res-hd` directory and `network-version.txt` into a separate copy of the game
data. Launch from that copy: the working directory must contain the original
game data. On Linux, the corresponding shared libraries must also be available.

Graphics settings expose resolution, window mode, anisotropy, FXAA, VSync,
FPS and HD. **Display and interface** opens UI scaling and vector font options.
Main user configuration values: `gfx_resolution=WxH`, `gfx_fullscreen=0/1`,
`ui_scale=0/75/100/125/150/200` and `ui_vector_fonts=0/1`. Display changes have
confirmation and automatic rollback. Set `S2_USER_DATA_DIR` to select a separate
profile/save/settings directory, including when running two game instances.

For multiplayer, open **Network game**: the host creates a match and the second
player enters its IPv4 address and port (default TCP **7780**; `127.0.0.1` for
local play). Both sides need compatible builds from identical sources, matching
game data and active mods; HD does not affect compatibility. The host's TCP
port must be reachable by the client. Internet matchmaking, automatic resource
transfer, reconnection, network saves, anti-cheat and a dedicated AI opponent
mode are not implemented yet. See [NETWORK_MODE.md](diagnostics/NETWORK_MODE.md).

## Checks and packaging

After building all targets:

```powershell
ctest --test-dir build -C Release --output-on-failure
& $hdPython assets/terrain-hd/validate_pack.py --pack build/Release/res-hd
```

On Linux, use `ctest --test-dir build-linux --output-on-failure`.
Resource tests are registered when `S2_GAME_DIR` contains the required data;
graphics checks require a display and a working graphics backend. Extended
scenarios can take a long time. Historical results and known limits are
recorded in the reports below.

To create a standalone Windows x64 archive from a completed build:

```powershell
python diagnostics/Package-Game.py --game-data C:/Games/SilentStorm-test --build build/Release --crt-runtime "C:/Path/To/Microsoft.VC143.CRT" --output SilentStorm-Windows-x64.7z
```

Requires the x64 Visual C++ Runtime from Visual Studio and 7-Zip for `.7z`;
use `.zip` for ZIP output. The packager includes game data, DLLs, fonts,
HD, `Play.cmd`, instructions and a checksum, and verifies the pack and archive.
HD is included by default; explicitly disable it with `--no-hd-textures`.
The `.7z` format uses LZMA2 with a 512 MiB dictionary, requiring that much
memory for extraction. An unpacked copy remains alongside the archive.

## Detailed reports

- [Facial animation and FaceGen comparisons with the original](diagnostics/FACE-PARITY.md).
- [SDL3/bgfx on Windows](diagnostics/BGFX-WINDOWS.md),
  [Linux x64 and ARM64](diagnostics/BGFX-LINUX.md).
- [Display, interface, fonts and graphics options](diagnostics/MODERN-DISPLAY.md).
- [Renderer performance](diagnostics/FRAME-PROFILING.md),
  [lighting stability](diagnostics/LIGHTING-STABILITY-20261003.md),
  [startup and window restoration fixes](diagnostics/LAST-THREE-BUGS-2026-10-06.md).
- [HD pack contents, building and image provenance](HD-TEXTURES.md),
  [machine-readable coverage](assets/terrain-hd/expanded/coverage.json).
- [Multiplayer and its checks](diagnostics/NETWORK_MODE.md).
- [x86 baseline](STAGE0-BASELINE.md), [stabilization log](STABILIZATION.md),
  [known interface differences](diagnostics/MANUAL-BUGS-2026-09-23.md).

Most detailed reports are currently in Russian.
