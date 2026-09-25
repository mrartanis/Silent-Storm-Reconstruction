# Этап 2: соответствие этажей игрового здания

`SMapBuilding::SStorey` (`Main/MapBuildingInfo.h`) задаёт соответствие
относительного этажа здания глобальному. В игровом `CBuilding` оно
используется при обновлении здания; при загрузке миссионного сейва
raw-аудит обнаруживал восьмибайтные записи этого типа в массиве
тега 2 объекта `0x02741136`. Это не редакторский-only путь.

Формат состоит из двух signed int32 (`nFloor`, `nRealFloor`) в
little-endian порядке. `FileIO/PortableMapStoreyWire.h` и специализация
`StructureFieldCodec` читают/пишут поля отдельно от host-layout;
дисковая длина осталась 8 байт. Переносимый тест проверяет точные
байты, отрицательные этажи, границы int32, round-trip и ошибки длины.
Нативный тест Windows x64/x86 сверяет 49 сочетаний с прежним
MSVC-образом структуры.

На 2026-09-25 Windows x64 CTest 66/66, целевые x86-тесты 2/2;
Linux x86-64 и ARM64/QEMU — по 39/39 под ASan/UBSan. Чистый архив
`D:\SS-lab\builds\stage2-map-storey-wire-20260925-01` из `be27c57`
(нативное медиа без `fmod.dll`) в запуске
`D:\SS-lab\runs\stage2-map-storey-clean-01` загрузил прежний
`SQUARE_MOVE_NEW`, записал `MAP_STOREY_NEW`, повторно открыл его до
`LOAD-SLOT-DONE` и завершился без дампа. `SMapBuilding::SStorey`
исчез из raw-журнала. SHA-256 `game.sav`:
`60ffe959c95e248b8e8dc6c868a642b2fb5b99e9d521909a0fd0ef5da62b342b`.
Чистый восстановленный x86-архив
`D:\SS-lab\builds\stage2-map-storey-wire-x86-20260925-01`
загрузил новый слот до `LOAD-SLOT-DONE` в
`D:\SS-lab\runs\stage2-map-storey-x86-read-01` и вышел без дампа.

Оригинальный Steam EXE в изолированной копии показал новый слот первым
в меню и открыл его до игрового экрана (персонаж, два мотоцикла,
здание, 55 AP, 75/75 здоровья). После штатного выхода хеш копии
сейва совпал с источником, 2698 установленных Steam-файлов и
23 исходных `.res` сохранили контрольные SHA-256. Это прямая
проверка чтения конкретного сейва основным эталоном, но не доказательство
поведения всех отрицательных этажей или полной визуальной точности.
См. [STEAM-SAVE-ORACLE.md](STEAM-SAVE-ORACLE.md).

Команды регрессии из корня репозитория:

```powershell
& 'G:\SS\lab\tools\VS2022\Common7\IDE\CommonExtensions\Microsoft\CMake\CMake\bin\ctest.exe' --test-dir D:\SS-lab\build-x64-stage2 -C RelWithDebInfo --output-on-failure -j 16
& 'G:\SS\lab\tools\VS2022\Common7\IDE\CommonExtensions\Microsoft\CMake\CMake\bin\ctest.exe' --test-dir D:\SS-lab\build-x86-stage2 -C RelWithDebInfo -R MapStorey --output-on-failure
ssh artanis.c.ibgene.org 'ASAN_OPTIONS=detect_leaks=0 ctest --test-dir /tmp/s2-matrix-links.zCkO0O/build-x64 --output-on-failure -j 16 && ASAN_OPTIONS=detect_leaks=0 ctest --test-dir /tmp/s2-matrix-links.zCkO0O/build-arm64 --output-on-failure -j 4'
```
