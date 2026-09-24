# SDL3 + bgfx: малая проба этапа 1

Отдельный CMake-проект создаёт окно SDL3 и очищает его через bgfx. В режиме
`--headless` создаётся скрытое окно (например, с `SDL_VIDEODRIVER=dummy`) и
используется bgfx Noop. Это проверяет стык библиотек и платформенный handle,
но не рендерер игры и не GPU на headless-машине.

Зависимости закреплены на SDL3 `release-3.4.16`, `bgfx.cmake`
`v1.161.9510-579` (`de08a6080b39994ab8a9eddb82e79e18bc3df7bd`) и его
submodule-коммитах:

- bgfx `81d81fba72c42d348c589514c774bbfe01e110fa`;
- bimg `87aaad3ac882e741889fdd4263224e5d12c26f99`;
- bx `25315498841259323e18f549d1ad9d9aba6632cc`.

Зависимости загружаются вне игрового Git-репозитория. CMake принимает полный
checkout `bgfx.cmake` через `S2_BGFX_CMAKE_ROOT`; если submodule-деревья
расположены отдельно, задаются `S2_BGFX_DIR`, `S2_BIMG_DIR`, `S2_BX_DIR`.
SDL3 ищется стандартным `find_package(SDL3 CONFIG REQUIRED)`; путь к пакету
можно передать через `SDL3_DIR` или `CMAKE_PREFIX_PATH`.
Для проверенных локальных архивов SHA-256:
`SDL3-devel-3.4.16-VC.zip` —
`1A784CB2A5C64D56FE7A62090FE9D242D9865F235E4EA9678F1A6BA4E693E7DE`,
`bgfx` на указанном коммите —
`F9894506A03B4587D210936D4773DCD553760FF432587E63BC7B598EFF54E6A6`.

Пример Windows (MSVC, пакет SDL3-devel-3.4.16-VC.zip):

```powershell
cmake -S probes/sdl3-bgfx -B build-sdl3-bgfx -G 'Visual Studio 17 2022' -A x64 `
  -DSDL3_DIR=<SDL3-devel>/cmake -DS2_BGFX_CMAKE_ROOT=<bgfx.cmake> `
  -DS2_BGFX_DIR=<bgfx> -DS2_BIMG_DIR=<bimg> -DS2_BX_DIR=<bx>
cmake --build build-sdl3-bgfx --config RelWithDebInfo --target S2SDL3BgfxProbe
# Добавить каталог SDL3.dll в PATH и запустить:
build-sdl3-bgfx/RelWithDebInfo/S2SDL3BgfxProbe.exe
```

Пример Linux без дисплея (SDL3 предварительно собран с dummy video):

```sh
cmake -S probes/sdl3-bgfx -B build-sdl3-bgfx -DCMAKE_BUILD_TYPE=Release \
  -DCMAKE_PREFIX_PATH=<SDL3-install> -DS2_BGFX_CMAKE_ROOT=<bgfx.cmake> \
  -DS2_BGFX_DIR=<bgfx> -DS2_BIMG_DIR=<bimg> -DS2_BX_DIR=<bx> \
  -DBGFX_WITH_WAYLAND=OFF
cmake --build build-sdl3-bgfx --target S2SDL3BgfxProbe --parallel
SDL_VIDEODRIVER=dummy LD_LIBRARY_PATH=<SDL3-install>/lib \
  build-sdl3-bgfx/S2SDL3BgfxProbe --headless
```

На Linux `bgfx.cmake` даже для Noop требует X11/OpenGL development packages.
На Ubuntu 22.04 предоставленного сервера понадобились `libgl-dev` и
`libglx-dev`. Для видимого кадра также поставлены `libxext-dev` и `xvfb`,
SDL3 пересобрана с X11. Проба запускается так:

```sh
xvfb-run -a -s '-screen 0 1024x768x24' env \
  VK_ICD_FILENAMES=/usr/share/vulkan/icd.d/lvp_icd.x86_64.json \
  SDL_VIDEODRIVER=x11 LD_LIBRARY_PATH=<SDL3-X11-install>/lib \
  build-sdl3-bgfx/S2SDL3BgfxProbe
```

`vulkaninfo --summary` с тем же ICD подтверждает `llvmpipe`. На macOS
команды конфигурации аналогичны Linux, но проверка на настоящем Mac ещё не
выполнена.

Проверено 2026-09-24: Windows/x64 — `SDL driver=windows`, bgfx Direct3D 11,
1800 кадров и видимая заливка окна; Linux x86-64 — `SDL driver=dummy`, bgfx
Noop, 3 кадра, и отдельно X11 + Vulkan/llvmpipe под Xvfb, 600 кадров с
захватом заливки. Снимки находятся вне Git в `G:\SS\lab\evidence`:
`sdl3-bgfx-win64.png` и `sdl3-bgfx-linux.png`.
