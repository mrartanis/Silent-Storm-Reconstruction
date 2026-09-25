# Этап 2: ключи игровых ресурсов

Чистый миссионный raw-аудит обнаружил `NGScene::STextureKey` (2 392
записи за загрузку и повторную загрузку) и `NGScene::SPartKey`
(1 400 записей). Первый хранит ID текстуры и флаги обёртки/
прозрачности, второй — ID ресурса и номер части модели. Оба типа
используются игровыми загрузчиками, не только редактором.

`FileIO/PortableResourceKeyWire.h` фиксирует общий восьмибайтный
little-endian формат пары signed int32. Специализации в
`Main/GTexture.h` и `Main/GResource.h` читают и пишут поля отдельно.
`PortableResourceKeyWireTests` проверяет точные байты, отрицательные
значения и границы int32; расширенный `NativeZoneWireTests` сравнивает
формат с раскладкой MSVC x86/x64.

На 2026-09-25 Windows x64 CTest 49/49, Linux x86-64 и ARM64/QEMU
под ASan/UBSan по 29/29, целевые диагностические x86-тесты 2/2.
Чистый x64-архив `stage2-resource-key-wire-20260925-01` загрузил
старый миссионный слот `stational weapons`, записал `RESOURCE_KEY_NEW`
и открыл его повторно (`LOAD-SLOT-DONE` оба раза). Оба точных типа
исчезли из `_wireaudit.log`, дампа нет. Чистый архив восстановленной
x86-сборки `stage2-resource-key-wire-x86-20260925-01` прочитал новый
слот до `LOAD-SLOT-DONE` без дампа. Артефакты запусков:
`C:\SS-lab\runs\stage2-resource-key-wire-clean-01` и
`C:\SS-lab\runs\stage2-resource-key-wire-x86-clean-01`.
Совпадение формата с восстановленной x86-сборкой не заменяет сверку
видимого результата со Steam.
