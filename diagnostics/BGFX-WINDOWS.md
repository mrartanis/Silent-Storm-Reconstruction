# Игра на bgfx в Windows

Первый шаг реализации пункта 4: Windows x64 Game.exe переведён на bgfx.
Linux отложен по решению пользователя. Общий REFACTOR.md не изменён.
Рабочая интеграция и визуальный паритет со Steam рассматриваются отдельно.

## Что реализовано

- Основная игра использует `GfxBgfx.cpp` и `BgfxBackend.cpp` вместо `Gfx.cpp`.
  Старые материалы, проходы, CPU-анимация и интерфейс сохранены. Ресурсы GPU,
  шейдеры, состояния, render targets, чтение кадра и представление окна
  выполняются через bgfx. Windows backend bgfx — Direct3D 11.
- Game.exe больше не линкуется с d3d9 или dxguid. D3D9-заголовки пока нужны
  для исторических токенов шейдеров, описаний вершин и состояний; COM-устройств
  D3D9 в новом backend нет. Удаление этих обозначений относится к дальнейшему
  отделению переносимого графического интерфейса.
- Фактический байткод игровых шейдеров переводится в HLSL при сборке.
  `shaderc` компилирует 470 вариантов, включая cube samplers и представление
  кадра. Комментарии старых шейдеров не используются как источник программы.
  Неподдержанные инструкции останавливают сборку. Game.exe получает готовые
  бинарники; D3DX-компилятор во время работы не нужен.
- Исторические упакованные вершины преобразуются в float-атрибуты перед
  отправкой. Сохранены значения SHORT2 и порядок компонент D3DCOLOR.
  Исправлена разница центров пикселей между старым и новым rasterizer.
- Проходы выполняются последовательно; каждый framebuffer удерживает свои
  поверхности до конца кадра. Сохранены stencil read/write masks, alpha test,
  смешивание, depth test, cubemaps, гамма и изменение размера окна.
- `Build-Lab.ps1` принимает закреплённые исходники bgfx, bimg и bx,
  сохраняет их пути и выбранный рендерер в метаданных сборки.

Положение центров пикселей описано в [документации Microsoft](https://learn.microsoft.com/en-us/windows/win32/direct3d10/d3d10-graphics-programming-guide-resources-coordinates).
Привязка sampler в PS 1.4 следует [описанию texld](https://learn.microsoft.com/en-us/windows/win32/direct3dhlsl/texld---ps-1-4).

## Сборка и тесты

MSVC v143, Windows SDK 10, x64 RelWithDebInfo. SDL3 3.4.16, bgfx.cmake
v1.161.9510-579. Закреплённые коммиты:

- bgfx: `81d81fba72c42d348c589514c774bbfe01e110fa`;
- bimg: `87aaad3ac882e741889fdd4263224e5d12c26f99`;
- bx: `25315498841259323e18f549d1ad9d9aba6632cc`.

Настоящие музыка, SFX и видео используют нативные замены этапа 2.
Зависимости и лицензионные данные остаются вне Git-репозитория.
Для чистого архива из корня рабочего репозитория:

```powershell
.\diagnostics\Build-Lab.ps1 -NativeMedia `
  -BuildDirectory G:\SS\lab\build-x64-stage4 `
  -BuildId stage4-bgfx-windows-20261001-01 `
  -FFmpegRoot G:\SS\lab\tools\ffmpeg-n8.1-lgpl-shared\ffmpeg-n8.1-latest-win64-lgpl-shared-8.1 `
  -MiniaudioIncludeDir G:\SS\lab\tools\miniaudio-0.11.25
```

После сборки всех targets:

```powershell
$ctest = 'G:\SS\lab\tools\VS2022\Common7\IDE\CommonExtensions\Microsoft\CMake\CMake\bin\ctest.exe'
& $ctest --test-dir G:\SS\lab\build-x64-stage4 -C RelWithDebInfo `
  --output-on-failure -E 'NativeScenarioRoot|NativeMapDatabaseRoots' -j 8
```

Итоговый прогон 2026-10-01: **173/173**, 11.61 с. Исключены только прежние
расширенные scenario/map roots; это не проверка всего набора миссий.
Анимированные источники света теперь включены: 21 ресурс, 1026 ключей
положения, 138 цвета и 429 радиуса, digest `3835B5072F5C5D8C`.

`BgfxRendererTests` работает на настоящем GPU NVIDIA `10de:2482`, с
Direct3D 11 через bgfx. Он читает результат GPU и проверяет depth test,
порядок проходов, render targets, загрузку текстур, независимые stencil bits
0x80/0x40, alpha blending/test, cube sampling, точное соответствие texels
пикселям при linear filtering, resize и shutdown. `BgfxRuntimeImportTests`
проверяет обычные и отложенные PE-импорты Game.exe: D3D9/D3DX9 отсутствуют.

## Живые визуальные проверки

Доказательства остаются в `G:\SS\lab`, вне репозитория:

- `runs\stage4-bgfx-dev-20261001-02\evidence\menu.png`: 3D-меню,
  персонаж, огонь, текст и кнопки.
- `runs\stage4-bgfx-dev-20261001-03\evidence`: учебная миссия, модальные
  подсказки, ландшафт, растительность, персонаж, тень и портрет;
  `blast-0.png` показывает взрыв и разрушение здания;
  `face-editor.png` показывает реальный FaceGen UI и модель персонажа.
  Есть кадры после сохранения/загрузки и изменения размера окна.
- `runs\stage4-d3d9-reference-20261001-01\evidence\focused-scene.png`:
  прежняя сборка этапа 3 загрузила тот же слот `bgfx_visual`.
- `runs\stage4-steam-reference-20261001-01\evidence\mission.png`:
  оригинальный Steam EXE загрузил тот же слот. SHA-256 EXE:
  `4F417593A9F73E2BFDE12D83CDBD694AE47EECB92812140D67B8B343E4A95705`.

Сверены SHA-256 всех 2451 файлов `baseline\res` и `game.db`:
2452 совпадения, изменений нет. Конкретные прогоны не подтверждают
исчерпывающий визуальный паритет всех материалов и миссий.

## Известное расхождение освещения

Пользователь отметил 2026-10-01: здания, башни и заборы выглядят
неосвещёнными относительно Steam; внутри зданий темно. Исправление
отложено по его просьбе. Причина не установлена; это открытая визуальная
задача, а не подтверждённый паритет освещения. При последующей проверке
нужны одинаковые сохранение, камера и время суток и отдельная сверка
static lightmaps, динамического света, нормалей и material passes.
