# Этап 2: игровая таблица стоимости клетки ИИ

`NAI::CSquareMapCosts` (`Main/aiWaveSearch.h`) хранит двумерный массив
`SMoveInfo<CTPoint<unsigned char>, WORD>`: родительская клетка (x, y)
и накопленная стоимость пути. Миссионный raw-аудит обнаружил этот
точный 4-байтный тип 12 раз при чтении массива тега 3 объекта
`0x70142160`. Это игровая память поиска пути, а не редакторская функция.

`FileIO/PortableSquareMoveWire.h` задаёт явный формат из двух byte
координат и little-endian uint16 стоимости. Специализация игрового
`StructureFieldCodec` читает и пишет поля по отдельности, не копируя
host-структуру. Длина осталась 4 байта. Нативный тест Windows x64/x86
сверяет 64 комбинации с точной прежней раскладкой MSVC; переносимый тест
проверяет граничные значения, ожидаемые байты, round-trip и отказы при
неверной длине/нулевых указателях.

На 2026-09-25: Windows x64 CTest 64/64, целевые Windows x86-тесты 2/2,
Linux x86-64 и ARM64/QEMU по 38/38 с ASan/UBSan. Чистый x64-архив
`D:\SS-lab\builds\stage2-square-move-wire-20260925-01` из `c14ff24`
с нативным медиа без `fmod.dll` в запуске
`D:\SS-lab\runs\stage2-square-move-clean-01` загрузил прежний
`BUILDING_PART_NEW`, записал `SQUARE_MOVE_NEW` и прочитал его до
`LOAD-SLOT-DONE`. Игра закрылась без дампа, точный `SMoveInfo` исчез
из raw-журнала. SHA-256 `game.sav`:
`3f32f98a039a922fb2e6fb8d92ed1882b8da94435277afa31aa253d57857f779`.
Чистый восстановленный x86-архив
`D:\SS-lab\builds\stage2-square-move-wire-x86-20260925-01`
прочитал новый слот в `D:\SS-lab\runs\stage2-square-move-x86-read-01`
до `LOAD-SLOT-DONE`, затем вышел без дампа.

Оригинальный Steam EXE с SHA-256 из [STEAM-SAVE-ORACLE.md](STEAM-SAVE-ORACLE.md)
показал `SQUARE_MOVE_NEW` первым в меню загрузки и открыл его до
игрового экрана: персонаж у мотоциклов и здания, 55 AP и 75/75
здоровья. После штатного выхода хеш копии сейва совпал с источником,
2698 файлов установленной Steam-игры и 23 исходных `.res` остались
неизменными. Это прямая совместимость конкретного сейва с основным
эталоном, но не доказательство совпадения маршрутов и решений ИИ.

Команды регрессии из корня репозитория:

```powershell
& 'G:\SS\lab\tools\VS2022\Common7\IDE\CommonExtensions\Microsoft\CMake\CMake\bin\ctest.exe' --test-dir D:\SS-lab\build-x64-stage2 -C RelWithDebInfo --output-on-failure -j 16
& 'G:\SS\lab\tools\VS2022\Common7\IDE\CommonExtensions\Microsoft\CMake\CMake\bin\ctest.exe' --test-dir D:\SS-lab\build-x86-stage2 -C RelWithDebInfo -R SquareMove --output-on-failure
ssh artanis.c.ibgene.org 'ASAN_OPTIONS=detect_leaks=0 ctest --test-dir /tmp/s2-matrix-links.zCkO0O/build-x64 --output-on-failure -j 16 && ASAN_OPTIONS=detect_leaks=0 ctest --test-dir /tmp/s2-matrix-links.zCkO0O/build-arm64 --output-on-failure -j 4'
```
