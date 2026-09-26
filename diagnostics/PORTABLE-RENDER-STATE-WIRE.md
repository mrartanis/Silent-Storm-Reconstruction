# Миссионные записи света, тумана и декалей

После `SELECTION_INFO_NEW` полный аудит миссионного сейва показывал семь
raw-путей для шести игровых типов: `SDecalMappingInfo` встречался в двух
местах, затем `SDynamicAmbientInfo`, `SFogParams`,
`SGlobalIlluminationInfo::SDirectional`, `SDirectionalDepthInfo` и
`SLightStateCalcSeed`. Коммит `dbf055e` заменил host-копирование этих
записей явными little-endian `float32` и `int32` полями. Для
`SDirectional` байт флага записывается как 0/1, три резервных байта —
нули; старые ненулевые значения флага и произвольный padding читаются.
Семантика освещения и рендеринга не менялась. Старый не-UTF-8 заголовок
`GLightmapCalc.h` сохранён без перекодирования: специализации помещены
в отдельный `GLightmapStateWire.h` и подключены в двух местах использования.

`PortableRenderStateWireTests` проверяет 8-, 16- и 24-компонентные
float-записи, точные байты, длины и запись с флагом. В
`NativeRenderStateWireTests` все шесть кодеков сверяются с Windows ABI,
а резервные байты флага — отдельно. Windows x64 собрал `Game.exe` и
прошёл CTest 76/76; целевой Windows x86 — 2/2. Linux x86-64 и
ARM64/QEMU под ASan/UBSan прошли по 44/44 переносимых теста. На Linux
это тесты headless-ядра, а не сборки игры с графикой.

Дополнение этапа 2 (2026-09-26): `SDynamicAmbientInfo::Clear()` теперь
обнуляет и шесть `SPad::f`. Раньше цветовые векторы становились нулевыми,
но соседние float-поля оставались неинициализированными; все 96 байт
попадают в кодек и могут попасть в сохранение световой группы. Проверка
заполняет объект ненулевыми байтами, вызывает `Clear()` и требует нули
во всех 96 байтах кодека. `NativeRenderStateWireTests` добавлен в Linux
CTest; для компиляции заголовка GCC графический `EFace` явно объявлен
как `int`, без изменения значений или Windows ABI. Это исправление данных
освещения, не перенос рендерера и не проверка визуального паритета.
После изменения `Game.exe` собрался на Windows x64, полная CTest-матрица
прошла 122/122; Linux GCC x86-64 и ARM64/QEMU под ASan/UBSan прошли
по 97/97, включая новый нативный тест кодека.

Чистый x64-архив `D:\SS-lab\builds\stage2-render-state-wire-20260925-01`
в `D:\SS-lab\runs\stage2-render-state-clean-01` загрузил
`SELECTION_INFO_NEW`, записал `RENDER_STATE_NEW`, повторно открыл его
до `LOAD-SLOT-DONE` и завершился без дампа. SHA-256 нового `game.sav`:
`12f4b95809169c8d45825b28be0ae06970b77fb611941d1319ef0e624521d371`.
Полный `_wireaudit.log` показывает `RAW-TYPE-AUDIT-DONE (0 distinct
types/paths)` после загрузки миссии. Это ноль только в данной миссии и
в области охвата raw-аудита `CStructureSaver`; другие карты и ручные
`AddRawData(void*)` этим не проверены. Восстановленная диагностическая
x86-сборка в `D:\SS-lab\runs\stage2-render-state-x86-01` прочла
неизменённый слот до `LOAD-SLOT-DONE` и вышла без дампа.

Прямой оракул: оригинальный Steam EXE (SHA-256
`4f417593a9f73e2bfde12d83cdbd694ae47eecb92812140d67b8b343e4a95705`)
в изолированной копии показал `RENDER_STATE_NEW` первым в меню и
открыл до игрового экрана с персонажем, мотоциклами, зданием, 55 AP
и 75/75 здоровья. После штатного выхода хеш копии сейва совпал с
источником, `Test-SteamUnchanged.ps1` подтвердил 2698 неизменённых
файлов установленной игры. Это совместимость данного составного сейва
со Steam, а не динамический или пиксельный паритет освещения, тумана
и декалей.

Повторение: CMake/CTest на каждой архитектуре; для Windows —
`Build-Lab.ps1 -Architecture x64 -NativeMedia`, затем
`New-LabRun.ps1 -SkipIntro -LinkResources`, копия старого слота в
`<run>/user-data/save/default`, его имя в `<run>/game/_loadslot.txt`,
`Start-LabRun.ps1 -GameArguments '-windowed -800 -harness -loadslot'`.
После загрузки через `<run>/game/_harness_cmd.txt` по очереди отправить
`save RENDER_STATE_NEW`, `load RENDER_STATE_NEW`, `quit`; проверить
полный `_wireaudit.log`, `_saveload.log`, SHA-256 и отсутствие дампа.
Для Steam использовать только изолированную копию с проверенным EXE,
обычное меню загрузки и контроль хеша после чтения.
