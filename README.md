# Silent Storm

*Русский | [English](README.en.md)*

Исходники Silent Storm принадлежат Nival, оригинальный репозиторий: 
https://github.com/nival/Silent-Storm

В этом репозитории ведется доработка исходников с конечной целью -
получение behaviour-equivalent версии Silent Storm v1.2, путём анализа
.pdb файлов версии v1.1 (RussianPatch1) и декомпиляции v1.2.

Текущий статус на 2026-09-30: Windows x64 запускает игру с SDL3-окном,
клавиатурой и мышью, сохраняя D3D9-изображение. Согласованный Windows-объём
этапа 3 проверен: 170/170 тестов, меню, консоль и учебная миссия.
Linux-запуск игры и совместные Windows/Linux-проверки выполняются на этапе 4
с bgfx. Вся кампания, полный паритет со Steam и macOS пока не проверены.
Команды и ограничения — в
[отчёте этапа 3](diagnostics/SDL3-PLATFORM-WINDOWS.md).

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
- Целевая игровая сборка — **Windows x64**.
- **SDL3 3.4.16**: пакет разработки с `cmake/SDL3Config.cmake` и `SDL3.dll`.
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

Окно, клавиатура и мышь основной игры переведены на SDL3. Этап 3 проверяется
на Windows с существующим D3D9-изображением; запуск и тесты Linux вместе с
переносом рендерера отнесены к этапу 4. Результаты и команды:
[SDL3-PLATFORM-WINDOWS.md](diagnostics/SDL3-PLATFORM-WINDOWS.md).

### Примечания
- импортируемые проприетарные библиотеки fmod / Bink / LifeStudio **генерируются во время сборки** из
  зафиксированных таблиц экспорта `.def` в каталоге `third_party/` - оригинальные SDK не требуются.
- `MapEdit`, `Scintilla`, `OpenDynamix` и `LSConverter` оставлены в репозитории, но **не
  собираются** (по тем или иным причинам, с MapEdit там вообще всё сложно); их списки 
  исходников сохранены в `sources.cmake`.
