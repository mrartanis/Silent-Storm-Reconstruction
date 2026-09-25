# Этап 2: идентификатор игровой части здания

`NBuilding::SPart` (`Main/BuildingPart.h`) используется как ключ
`CBuildingGrid::updatedParts` при обновлении разрушаемого здания. В
миссионном raw-аудите его 4-байтное значение встречалось 85 раз при
чтении тега 4. Это игровой путь, а не функция редактора.

Прежняя запись зависела от раскладки MSVC-битовых полей: signed floor
(6 бит), x (13 бит), y (13 бит) в одном `DWORD`. Теперь `SPart` хранит
одно явное `uint32_t`-слово; `Misc/PortableBuildingPart.h` задаёт маски,
сдвиги, знаковое восстановление и little-endian чтение/запись. Дисковая
длина осталась 4 байта; хеширование и сравнение ключей используют то же
слово. `Main/MakeBuilding.h` получает этаж через `GetFloor()`.

`NativeBuildingPartTests` на Windows x64/x86 сравнивает все 150
комбинаций граничных floor/x/y со старой MSVC-битовой структурой;
`PortableBuildingPartTests` проверяет точный байтовый образ, граничные
значения, round-trip и отказ на неверной длине/нулевом указателе.
Windows x64 CTest: 62/62; целевые x86-тесты: 2/2. На Linux x86-64
и ARM64/QEMU CTest: по 37/37 с `-fsanitize=address,undefined`.

Чистый x64-архив из `2675ed9`:
`D:\SS-lab\builds\stage2-building-part-wire-20260925-01` (нативное
медиа без `fmod.dll`). Запуск
`D:\SS-lab\runs\stage2-building-part-clean-01` открыл прежний
`AI_DISTANCE_NEW`, записал `BUILDING_PART_NEW`, повторно открыл новый
слот до `LOAD-SLOT-DONE` и штатно завершился без дампа. SHA-256
`game.sav`: `ae22c1f7f3a0dab289c24c3fee031519b1e97bad44e9513582ce4ba30b112979`.
Точный `NBuilding::SPart` исчез из `_wireaudit.log`.
Чистая диагностическая восстановленная x86-сборка из
`D:\SS-lab\builds\stage2-building-part-wire-x86-20260925-01`
прочитала тот же новый слот до `LOAD-SLOT-DONE` в
`D:\SS-lab\runs\stage2-building-part-x86-read-01`, затем вышла без дампа.
Все 23 исходных `.res` сохранили контрольные SHA-256.

Основные команды из корня репозитория:

```powershell
& 'G:\SS\lab\tools\VS2022\Common7\IDE\CommonExtensions\Microsoft\CMake\CMake\bin\ctest.exe' --test-dir D:\SS-lab\build-x64-stage2 -C RelWithDebInfo --output-on-failure -j 16
& 'G:\SS\lab\tools\VS2022\Common7\IDE\CommonExtensions\Microsoft\CMake\CMake\bin\ctest.exe' --test-dir D:\SS-lab\build-x86-stage2 -C RelWithDebInfo -R BuildingPart --output-on-failure
ssh artanis.c.ibgene.org 'ASAN_OPTIONS=detect_leaks=0 ctest --test-dir /tmp/s2-matrix-links.zCkO0O/build-x64 --output-on-failure -j 16 && ASAN_OPTIONS=detect_leaks=0 ctest --test-dir /tmp/s2-matrix-links.zCkO0O/build-arm64 --output-on-failure -j 4'
```

Восстановленная x86-сборка служит диагностике, а не поведенческим
эталоном: им остаётся оригинальный Steam EXE. Дополнительно неизменённый
Steam EXE в изолированной копии открыл `BUILDING_PART_NEW` через меню до
игрового экрана: 55 AP, 75/75 здоровья, персонаж у двух мотоциклов и
здания. SHA-256 слота до и после этой пробы совпал. Установленная игра
прошла контроль неизменности 2698 файлов. Подробности — в
[STEAM-SAVE-ORACLE.md](STEAM-SAVE-ORACLE.md). Этот перенос всё ещё не
доказывает полного визуального паритета разрушений или поведения ИИ.
