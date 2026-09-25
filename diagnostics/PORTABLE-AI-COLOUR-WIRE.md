# Этап 2: связи цветовой сети маршрутов ИИ

Контрольный raw-аудит миссионного сейва обнаружил 1 492 записи
`NAI::SNeighbour` при чтении `CMapColourer` — это соседи узлов
цветовой сети маршрутов, используемой игрой. В том же модуле остаётся
`NAI::SLocalColorInfo` (средние координаты цвета). Это игровые данные
поиска пути, а не функции редактора.

`FileIO/PortableAIColourWire.h` фиксирует два little-endian формата:
`SNeighbour` — 8 байт (`uint16 wNodeNumber`, `uint16 wDistance`,
`int32 nLayer`); `SLocalColorInfo` — 4 байта (два `uint16`).
Специализации в `Main/aiColourer.h` читают и пишут поля отдельно.
`PortableAIColourWireTests` проверяет точные байты, отрицательный этаж,
длины и round-trip. Расширенный `NativeZoneWireTests` подтверждает
размеры/смещения и байтовое совпадение с MSVC x86/x64.

На 2026-09-25 Windows x64 CTest 47/47, Linux x86-64 и ARM64/QEMU
под ASan/UBSan по 27/27, целевые диагностические x86-тесты 2/2.
Чистый x64-архив `stage2-ai-colour-wire-20260925-01` загрузил старый
миссионный слот `stational weapons`, записал `AI_COLOUR_NEW` и повторно
загрузил его (`LOAD-SLOT-DONE` оба раза). Оба точных типа исчезли из
`_wireaudit.log`, дампа нет. Отдельный чистый архив восстановленной
x86-сборки `stage2-ai-colour-wire-x86-20260925-01` загрузил этот новый
слот до `LOAD-SLOT-DONE` без дампа. Артефакты запусков:
`C:\SS-lab\runs\stage2-ai-colour-wire-clean-01` и
`C:\SS-lab\runs\stage2-ai-colour-wire-x86-clean-01`.
Совместимость формата не доказывает паритет выбора маршрута с
оригинальной Steam-версией; Steam остаётся эталоном поведения,
восстановленная x86-сборка — диагностическим сравнением.
