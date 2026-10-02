# Silent Storm

*Русский | [English](README.en.md)*

Исходники Silent Storm принадлежат Nival, оригинальный репозиторий: 
https://github.com/nival/Silent-Storm

В этом репозитории ведется доработка исходников с конечной целью -
получение behaviour-equivalent версии Silent Storm v1.2, путём анализа
.pdb файлов версии v1.1 (RussianPatch1) и декомпиляции v1.2.

Текущий статус на 2026-10-02: игра использует SDL3 и bgfx на Windows x64,
Linux x64 и Linux ARM64. Реализован этап 6: изменяемое окно, безрамочный режим,
современные разрешения, равномерный масштаб и адаптивный интерфейс, Hor+ камера,
ролики без растяжения и поставляемые векторные шрифты FreeType/Liberation.
Linux проверяется на Vulkan/lavapipe; ARM64 — через QEMU. Физические Linux GPU,
Linux HiDPI, macOS и полный паритет кампании со Steam остаются непроверенными.
Матрица разрешений, обычный ввод, результаты тестов, сборки и ограничения — в
[отчёте этапа 6](diagnostics/MODERN-DISPLAY.md).
Дополнительно исправлены широкие панели, раскладка инвентаря/персонажа, экран
загрузки и вход в миссии/случайные столкновения на карте кампании. Отложенные
отличия оружейника и визуальные отличия индикаторов навыков отмечены в
[журнале известных ошибок](diagnostics/MANUAL-BUGS-2026-09-23.md).

Контрольная база этапа 0 для выбранных Windows/x86-сценариев зафиксирована в
[STAGE0-BASELINE.md](STAGE0-BASELINE.md); подробный журнал —
[STABILIZATION.md](STABILIZATION.md). Это не означает прохождение всей кампании
или готовность кроссплатформенного порта.

<div align="center">
  <table>
    <tr>
      <td colspan="3" align="center">
        <img width="300" alt="Screenshot 2026-07-17 143626" src="https://github.com/user-attachments/assets/a7fa15d4-6be7-491d-abf9-882c63bd00cd" />
      </td>
    </tr>
    <tr>
      <td colspan="3" align="center">
        <img width="300" alt="Screenshot 2026-07-18 143925" src="https://github.com/user-attachments/assets/b7021f4c-839f-48cc-a508-4541a851f195" />
        <img width="300" alt="Screenshot 2026-07-18 144009" src="https://github.com/user-attachments/assets/731740e4-74d0-41db-940a-5c687bb2fbd5" />
      </td>
    </tr>
  </table>
</div>

---

## Сборка

### Требования
- **Windows** с **Visual Studio 2022** (нужен компонент "Разработка классических
  приложений на C++", MSVC v143, Windows SDK 10) или новее. Проверено на VS 2026.
- **CMake 3.21+**
- Целевые игровые сборки — **Windows x64, Linux x64, Linux ARM64**. Linux требует
  Clang 18; ARM64 — соответствующий sysroot и host shaderc. Команды приведены в
  [отчёте этапа 6](diagnostics/MODERN-DISPLAY.md).
- **SDL3 3.4.16**: пакет разработки с `cmake/SDL3Config.cmake` и `SDL3.dll`;
  Linux/X11 требует XInput2 для относительной мыши.
- **FreeType 2.14.3** загружается CMake с проверкой SHA-256 либо задаётся через
  `S2_FREETYPE_SOURCE_DIR`. Liberation Fonts 2.1.5 поставляются в `assets/fonts`.
- *Необязательно:* **DirectX SDK June 2010** (https://www.microsoft.com/en-us/download/details.aspx?id=6812) - нужен
  только для сборки инструмента `ShaderCompiler`; если его нет, инструмент
  пропускается автоматически.

### Сборка
Запустите **`build.bat`** или выполните в терминале из папки репозитория:

Для `build.bat` и `build-debug.bat` предварительно задайте переменную окружения
`SDL3_DIR`, указывающую на каталог `cmake` пакета SDL3 3.4.16.

```
cmake -S . -B build -A x64 -DSDL3_DIR=G:/SS/lab/tools/sdl3-3.4.16/SDL3-3.4.16/cmake
cmake --build build --config Release
```

Результаты появятся в **`build\Release\`** - `Game.exe` и инструменты (`DataImport`,
`PkgBuilder`, `FontGen`, `TexConv`, `TexMipStrip`, `ShaderCompiler`).

Для отладки запустите **`build-debug.bat`** (собирает конфигурацию `RelWithDebInfo`),
откройте **`build\A5.sln`** в Visual Studio и запустите отладку проекта `Game`.
CMake попытается подсунуть вашу папку с игрой в рабочий каталог отладчика, если
у него это не получится, то нужно указать её вручную. Правой кнопкой по проекту
`Game` - Свойства - Отладчик - Рабочий каталог. Пример 
`E:/SteamLibrary/steamapps/common/Silent Storm`

### Запуск игры
Создайте отдельную копию игровых данных и положите туда `Game.exe`, `SDL3.dll`
и остальные DLL своей сборки. Оригинальную установку сохраняйте неизменной.
Рабочий каталог должен содержать `game.db`, `cfg` и полный `res`.
Для лаборатории используйте `diagnostics/New-LabRun.ps1` и `Start-LabRun.ps1`.

Игра использует SDL3 и bgfx. Скопируйте каталог `fonts` рядом с бинарником,
чтобы шрифты работали без системной установки. Параметры `gfx_resolution=WxH`,
`gfx_fullscreen=0/1`, `ui_scale=0/75/100/125/150/200` и `ui_vector_fonts=0/1`
сохраняются в пользовательском конфиге. Режим и масштаб доступны в графических
настройках через **Display and interface**. Подробности и проверенные команды:
[MODERN-DISPLAY.md](diagnostics/MODERN-DISPLAY.md).

### Примечания
- импортируемые проприетарные библиотеки fmod / Bink / LifeStudio **генерируются во время сборки** из
  зафиксированных таблиц экспорта `.def` в каталоге `third_party/` - оригинальные SDK не требуются.
- `MapEdit`, `Scintilla`, `OpenDynamix` и `LSConverter` оставлены в репозитории, но **не
  собираются** (по тем или иным причинам, с MapEdit там вообще всё сложно); их списки 
  исходников сохранены в `sources.cmake`.
