# Этап 2: базовая игровая геометрия на диске

Raw-аудит загрузки оригинальных ресурсов и миссионного сейва обнаружил
`SBound`, `STriangle`, `SMassSphere` и `SSphere` в игровом пути физики,
коллизии и AI-геометрии. `FileIO/PortableGameGeometry.h` фиксирует их
старые форматы: `STriangle` — три 16-битных индекса; `SSphere` — четыре
float; `SMassSphere` — пять float; `SBound` — семь float. Все числа читаются
и пишутся как little endian, без копирования целого host-объекта.
Специализации `StructureFieldCodec` включены в `FileIO/GeometryWire.h`,
который уже подключён к игровому сериализатору `BasicChunk1.h`.

Расширенные `PortableGeometryWireTests` проверяют конкретные байты,
смещения полей, знаковый ноль и неверную длину на всех архитектурах.
`NativeGeometryWireTests` сравнивают запись с памятью MSVC-объектов и
проверяют размер/смещения на Windows x64 и диагностическом x86.
На 2026-09-25 Game собран на Windows x64, CTest 37/37; Linux x86-64
и ARM64/QEMU прошли по 21/21 с ASan/UBSan (`detect_leaks=0` на QEMU).
Это перенос используемых игрой дисковых данных этапа 2, не перенос
графического рендера этапов 3–4 и не доказательство паритета Steam-кадров.

Чистый x64-архив `stage2-game-geometry-20260925-01` из коммита `f882bd6`
в LabRun `stage2-game-geometry-clean-01` загрузил старый
`stational weapons`, записал `GAME_GEOMETRY_NEW` и повторно загрузил
его до `LOAD-SLOT-DONE`; после `quit` дампа нет. Raw-аудит того же
слота больше не содержит точных типов `SBound`, `STriangle`,
`SMassSphere`, `SSphere`. Строка `SBoundCalcer` в журнале относится
к другому raw-типу и не считается перенесённой.

Чистый диагностический x86-архив
`stage2-game-geometry-x86-diagnostic-20260925-01` из того же коммита
прочитал `GAME_GEOMETRY_NEW` до `LOAD-SLOT-DONE` в
`stage2-game-geometry-x86-read-01` и завершился без дампа. Это
проверка совместимости текущих реконструированных сборок; Steam
остаётся эталоном поведения, а не эта x86-сборка.
