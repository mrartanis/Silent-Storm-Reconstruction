# Этап 2: переносимый заголовок чанков `game.db`

Игровой `Game/Main.cpp` читает оригинальный `game.db` через
`NDatabase::Serialize` и `CStructureSaver`. Формат верхнего уровня —
однобайтовый tag и кодированная длина: один байт для короткого чанка или
четыре little-endian байта для длинного; длина — значение без младшего
флага, сдвинутое вправо на один бит. `ReadShortChunkSave` теперь вызывает
`S2FileIO::DecodeStructureLength` из отдельного модуля без Windows SDK.
Проверка выхода за файл происходит до выделения буфера. Формат объекта
внутри чанка этим изменением не заменён.

Оригинальный `G:\SS\lab\baseline\game.db`: SHA-256
`314D7AA9E6339F0E6826BF2A0C71FDAD992AEC1D6074A2ED763A6F6FA62D62DF`,
36 314 320 байт. На Windows x86, Windows x64 и Linux x86-64 одна и та же
копия дала четыре одинаковые строки `id offset length FNV-1a-64`:

```text
4 2 4 ad2aca7747985764
1 11 1916 0a013bb2c3adbb6d
0 1932 1395 770e938debb5e196
2 3332 36310988 3b4894954230d73f
```

Linux-сборка GCC 11.4 использовала `-std=c++17 -O1 -g -Wall -Wextra
-Werror -fsanitize=address,undefined -fno-omit-frame-pointer`;
`PortableStructureChunkTests` и `PortableStructureProbe` завершились без
сообщений санитайзеров. На Windows x86/x64 пересобраны `Game` и probe,
по 8/8 CTest проходят. Изолированный Linux-каталог с копией `game.db`
после проверки удалён.

Следующий слой — верхняя объектная таблица (чанк 0). Каждая запись занимает
ровно 9 байт: little-endian `typeId`, little-endian `wireId`, однобайтовый
флаг действительности. `CStructureSaver::Start` использует общий переносимый
декодер `DecodeStructureObjectTable` вместо чтения трёх полей в host-типы.
Опция `PortableStructureProbe game.db --objects` на той же базе дала
`objects 155 valid 155 unique-ids 155` на Windows x86/x64 и Linux x86-64;
Linux GCC 11.4 под ASan/UBSan не сообщил ошибок. Объектная фабрика и
десериализация полей объектов остаются в Windows-игре, поэтому это ещё не
Linux-загрузчик таблиц.

Вложенные поля объектов теперь читаются через `DecodeStructureChunkAt`:
проверяются границы тега, однобайтовой/четырёхбайтовой длины и полезной
нагрузки до обращения к памяти. Этот же декодер используется в
`CStructureSaver::ReadShortChunk`, но создание объектов и привязка полей
ещё зависят от Windows-реализации. Опция
`PortableStructureProbe game.db --objects --nested` полностью прошла
36 310 988 байт чанка 2 как 155 вложенных чанков на Windows x86/x64 и
Linux x86-64; Linux GCC 11.4 с ASan/UBSan ошибок не сообщил.

Коммит `4b762dc` собран из чистого дерева в архивы
`stage2-nested-20260924-01` (x64, нативное медиа без FMOD) и
`stage2-nested-x86-20260924-01` (x86). В запусках
`stage2-nested-clean-x64-01` и `stage2-nested-clean-x86-01` команда
`load DB_OLD` дала `LOAD-DESERIALIZE-COMPLETE (1 interfaces)` и
`LOAD-SLOT-DONE`, затем `quit` завершил обе игры и оба отладчика;
`crash.dmp` отсутствует. Проверен один старый сейв, не вся кампания.

Широкие строки структуры имеют дисковое представление UTF-16LE, не массив
host-`wchar_t`. `DataChunkString(std::wstring&)` теперь использует
`DecodeStructureUtf16`/`EncodeStructureUtf16`: Windows сохраняет 16-битные
code units, Linux с 32-битным `wchar_t` собирает корректные surrogate pairs
в Unicode scalar values и возвращает их в исходные UTF-16LE байты при записи.
Непарные суррогаты сохраняются для совместимости со старыми данными;
нечётная длина отклоняется. Контрактные тесты проверяют кириллицу,
дополнительную плоскость, непарный суррогат, встроенный NUL и некорректную
длину на Windows x86/x64 и Linux x86-64 под ASan/UBSan. Полный паритет
игровых строк и локализаций этим ещё не доказан.

Коммит `33776b6` собран в чистые архивы `stage2-utf16-20260924-01`
(x64, нативное медиа без FMOD) и `stage2-utf16-x86-20260924-01`
(исторический x86-оракул). В `stage2-utf16-clean-x64-01` старый `DB_OLD`
дал `LOAD-DESERIALIZE-COMPLETE (1 interfaces)` и `LOAD-SLOT-DONE`; после
`quit` игра и отладчик завершились, дампа нет. x86-архив прошёл 8/8 CTest,
игра в отдельной лабораторной копии только запущена и штатно закрыта:
после решения пользователя отказаться от 32-битной целевой сборки её
save/load здесь повторно не проверялся.

Скалярные и raw-поля больше не читаются через безусловный `memcpy` запрошенного
host-размера: `CopyStructureField` проверяет равенство размера дискового поля
и назначения до копирования. `CStructureSaver::DataChunk`/`RawData` вызывают
этот переносимый помощник и сообщают ошибку при несовпадении, вместо чтения
соседнего чанка. Контрактные тесты проверяют точный, укороченный и удлинённый
буфер, null-указатель и нулевую длину; Windows x64 8/8 CTest и Linux x86-64
GCC 11.4 с ASan/UBSan прошли. Паритет всех скалярных значений базы ещё не
доказан: текущий путь всё ещё копирует little-endian байты в host-поля.

Коммит `eee6271` архивирован как `stage2-field-bounds-20260924-01`
(чистая Windows x64 с нативным медиа без FMOD). В лабораторном запуске
`stage2-field-bounds-clean-x64-01` игра загрузила тот же `DB_OLD`:
`LOAD-DESERIALIZE-COMPLETE (1 interfaces)`, `LOAD-SLOT-DONE`, затем `quit`.
Процессы игры и отладчика завершились; `crash.dmp` отсутствует. На этом
сохранении и оригинальном `game.db` строгая проверка размеров не обнаружила
несовпадений, но другие сохранения пока не проверены.

Коммит `dbedba8` архивирован как `stage2-db-objects-20260924-01` (x64,
нативное медиа без FMOD) и `stage2-db-objects-x86-20260924-01` (x86).
Чистые запуски `stage2-db-objects-clean-x64-01` и
`stage2-db-objects-clean-x86-01` загрузили тот же `DB_OLD`:
`LOAD-DESERIALIZE-COMPLETE (1 interfaces)` и `LOAD-SLOT-DONE` в каждом
`_saveload.log`. После `quit` обе игры и оба отладчика завершились,
`crash.dmp` нет. Это проверяет фактическое применение нового декодера в
игре на одном сохранении, но не весь набор таблиц и сценариев.

Коммит `101edc8` собран в чистые архивы
`G:\SS\lab\builds\stage2-db-chunks-20260924-01` (x64) и
`G:\SS\lab\builds\stage2-db-chunks-x86-20260924-01` (x86). Из каждого
создан отдельный запуск с отключённой стартовой заставкой:
`G:\SS\lab\runs\stage2-db-chunks-clean-x64-01` и
`G:\SS\lab\runs\stage2-db-chunks-clean-x86-01`. Оба загрузили одинаковую
копию старого `game.sav`/`restart.sav` из пользовательского сохранения
`user-quicksave-20260922-211453/Быстрая запись (2)` через `load DB_OLD`.
В обоих `_saveload.log` есть `LOAD-DESERIALIZE-COMPLETE (1 interfaces)` и
`LOAD-SLOT-DONE`; команда `facegenstatus` отработала, затем `quit`
завершил процессы игры и отладчика. Дампов падения не создано. Это smoke-тест
загрузки одного сохранения, а не полный регрессионный сценарий игры.

Повторение на данном стенде:

```powershell
& 'G:\SS\lab\build-x86\RelWithDebInfo\PortableStructureProbe.exe' 'G:\SS\lab\baseline\game.db'
& 'G:\SS\lab\build-x64\RelWithDebInfo\PortableStructureProbe.exe' 'G:\SS\lab\baseline\game.db'
```

На Linux, после копирования в отдельный каталог исходников
`FileIO/PortableStructureChunks.{h,cpp}`,
`diagnostics/{PortableStructureProbe,PortableStructureChunkTests}.cpp`
и исходного `data/game.db`:

```sh
g++ -std=c++17 -O1 -g -Wall -Wextra -Werror \
  -fsanitize=address,undefined -fno-omit-frame-pointer \
  FileIO/PortableStructureChunks.cpp diagnostics/PortableStructureProbe.cpp \
  -o PortableStructureProbe
sha256sum data/game.db
./PortableStructureProbe data/game.db
./PortableStructureProbe data/game.db --objects
./PortableStructureProbe data/game.db --objects --nested
```

Границы: это верхний слой чтения `game.db`, не перенос таблиц и ссылок
базы на Linux. Вложенная структура, объектная фабрика, `wchar_t`,
сжатие исторических сохранений, файлы пользователя и ARM64 ещё требуют
отдельной работы и проверок. Совпадение четырёх чанков не доказывает
паритет всех игровых записей.

Обновление 2026-09-24: `CStructureSaver::CallObjectSerialize` теперь
декодирует и кодирует арифметические и enum-поля явно как little-endian,
включая нормализацию `bool`; нечисловые raw-поля остаются непрозрачными.
Контрактный тест проверяет uint32, отрицательный int32, enum, float,
нестандартный ненулевой bool и ошибки размера. Windows x64 — 10/10 CTest,
ARM64/QEMU — целевой тест обычный и под ASan/UBSan. Коммит `dbc74b4`
собран в чистый архив `stage2-scalar-le-20260924-01` с 12 заданиями.
Запуск `stage2-scalar-le-clean-01` загрузил старый `DB_OLD`, записал
`S2_SCALAR` и вновь загрузил его до `LOAD-SLOT-DONE`; игра и отладчик
завершились без `crash.dmp`. Это один сценарий, не перенос всей игры
на Linux и не полный паритет всех сохранений.

Следующий пакет: числовые/enum `DoDataVector` и `Do2DArrayData` больше не
считываются цельным host-буфером; каждый элемент проходит тот же
little-endian кодек. Размеры массива проверяются до умножения и копирования;
нечисловые POD-блоки пока остаются raw. Контрактный тест покрывает массивы
signed int32 и bool, несовпадающую длину и null. На Windows x64 прошли
сборка `Game.exe` и 10/10 CTest, на Linux x86-64 — ASan/UBSan, на ARM64/QEMU —
обычный прогон и ASan/UBSan (без LeakSanitizer). Коммит `fc3306f` собран в
чистый x64-архив `stage2-scalar-arrays-20260924-01`; запуск
`stage2-scalar-arrays-clean-01` загрузил старый `DB_OLD`, записал
`S2_ARRAYS`, повторно загрузил его до `LOAD-SLOT-DONE` и завершился без
`crash.dmp`. Контрактные тесты сверяют точные little-endian байты, но широкое
сопоставление игровых значений с x86-эталоном остаётся открытым.

Следующий игровой raw-тип — `NGame::SActionInfo`: `CMission` сохраняет его
вектор (tag 16), `CStateAttack` — одиночное поле (tag 3). Формат по-прежнему
20 байт с историческими смещениями AP, флагов и результата команды, но поля
кодируются явно; байты выравнивания 1..3 при записи равны нулю, при чтении
не используются. Тип подключён через `StructureFieldCodec`, так что размер
дисковой записи больше не выводится из `sizeof(SActionInfo)`; прочие raw-типы
пока сохраняют прежний путь. Контрактный тест с ненулевым старым padding и
нестандартным `bool` прошёл на Windows x64 (10/10 CTest), Linux x86-64 и
ARM64/QEMU, последние две архитектуры — также под ASan/UBSan без LSan.
Коммит `077b5c2` собран в чистый x64-архив
`stage2-action-info-wire-20260924-01` без FMOD. Запуск
`stage2-action-info-wire-clean-01` загрузил старый `DB_OLD`, записал
`S2_ACTION` и повторно загрузил его до `LOAD-SLOT-DONE` без дампа.
Новый `game.sav` затем скопирован без изменения (SHA-256
`22E5A550CDB32B04B3B9C13EE89A2CD94754F36416187CC88B37AD76037E03CC`)
в запуск `stage2-action-info-wire-x86-oracle-01` из более раннего чистого
x86-архива `stage2-db-chunks-x86-20260924-01`: загрузка дошла до
`LOAD-SLOT-DONE`, оба процесса штатно завершились, дампа нет. Это
совместимость одного сейва с прежним x86-загрузчиком, не полная сверка
состояния всех игровых правил с оригинальным релизом.

Оставшиеся игровые raw-типы требуют отдельного разбора; их нельзя
автоматически переключать на поэлементные чанки, поскольку формат сохраняет
цельный blob. Ниже документируются перенесённые игровые случаи; полный аудит
всех мест `CallObjectSerialize` ещё не закончен.

Проверка взрывов (2026-09-24, в работе): два игровых raw-типа из этого списка
получили явные полевые кодеки без изменения формы blob: `SExplVoxel` — 2 байта
little-endian, `SExplVoxelCoords` — три 16-битные координаты, всего 6 байт.
Контрактный тест проверяет точные байты на Windows x64 (11/11 CTest),
Linux x86-64 и ARM64/QEMU, включая ASan/UBSan на Linux.

Для воспроизводимого вызова штатного игрового пути в `-harness` добавлены
`unitpos` (координаты юнитов), `grenades` (ID многоволновых гранат),
`explode <id> <x> <y> <z>` и `explstatus` (очередь/трекеры), а также
`explodesave <slot> <count> <id> <x> <y> <z>`: от 1 до 8 взрывов в одном
кадре, сохранение при первом ненулевом активном фронте, тайм-аут 600 кадров.
Для последней команды допускается только новый ASCII-слот. Координаты —
мировые, взрыв ограничен safe-зоной карты. Нужен загруженный уровень.
Лабораторный запуск `stage2-voxel-harness-dev-03` загрузил старый `DB_OLD`,
выполнил `explode 23 18 26 9` (пять волн), затем записал и повторно загрузил
`S2_VOXEL` до `LOAD-SLOT-DONE`, без дампа. После завершения взрыва
`explstatus` показал пустые очередь и список трекеров. Это dev-запуск с заменённым EXE поверх прежнего архива, а
не чистый архив текущего коммита.

Тест также обнаружил несовместимость старого сейва: до ввода поля мира tag 50
восстановленный мир получал null-планировщик. Эффект и звук взрыва
воспроизводились, но расчёт повреждений не ставился в очередь. Теперь первый
`CWorld::Segment` создаёт планировщик, если его нет; `explstatus` после
загрузки `DB_OLD` подтвердил начальное состояние `state=0, pending=0`, а
`explode` отработал с созданным планировщиком.

Следующий dev-запуск `stage2-voxel-wavefront-dev-01` выполнил
`explodesave WAVE_04 4 23 18 26 9`: лог подтвердил очередь из четырёх
взрывов, а перед сохранением — `wavefronts=1 trackers=4 frames=23`.
Файл `WAVE_04/game.sav` (4 658 315 байт) создан после этого сигнала;
загрузка дошла до `LOAD-SLOT-DONE`, затем `explstatus` показал завершение
волн (`state=0`, нулевые очередь и трекеры), дампа нет. Таким образом,
проверен один save/load активного фронта на Windows x64, но не точное
сопоставление повреждений с x86-эталоном и не множество разных карт/взрывов.
Эта первая проверка также выполнена заменённым dev-EXE поверх предыдущего архива.

Коммит `b30fe5d` собран в чистый x64-архив
`stage2-voxel-wavefront-20260924-01` с нативным звуком/видео без FMOD
и 16 заданиями. Из него создан `stage2-voxel-wavefront-clean-01`:
загрузка `DB_OLD`, `explodesave WAVE_CLEAN 4 23 18 26 9`, в логе
`pending=1..4`, затем `wavefronts=1 trackers=4 frames=29`; файл
`WAVE_CLEAN/game.sav` (4 740 257 байт) записан. Повторная загрузка
достигла `LOAD-SLOT-DONE`; позднее `explstatus` показал
`state=0 pending=0 trackers=0 wavefronts=0`, дампа нет. Это доказывает
save/load и последующее завершение одного активного взрыва из чистого
Windows x64-архива, но не эквивалентность повреждений x86-оригиналу и не
работу полной Linux/ARM64-игры.

Камера (2026-09-24, в работе): `ICamera::SCameraPos` сериализуется как восемь
32-битных float (32 байта: rod, pitch, yaw, roll, FOV, anchor XYZ), а
`ICamera::SCameraLimits` — как четыре float, однобайтовый `bMovie`, три
исторических байта выравнивания и ещё девять float (56 байт). Эти поля
теперь проходят явный little-endian `StructureFieldCodec`, сохраняя прежнюю
форму raw-чанка. При записи выравнивание обнуляется, при чтении игнорируется;
нестандартный ненулевой байт флага нормализуется в `true`. Используемые игрой
поля встречаются в объекте камеры, состояниях миссии и исполнителе движения
камеры; редакторские дополнительные функции сюда не включены. Контрактные
тесты `PortableCameraWireTests` и `NativeCameraWireTests` прошли на Windows
x64 (13/13 CTest), переносимый тест — на Linux x86-64 и ARM64/QEMU под
ASan/UBSan без LeakSanitizer. Коммит `8013377` собран в чистый x64-архив
`stage2-camera-wire-20260924-01` с 16 заданиями и нативным медиа без FMOD.
Запуск `stage2-camera-wire-clean-01` загрузил старый `DB_OLD`, записал
`S2_CAMERA/game.sav` (4 002 857 байт), повторно загрузил его до
`LOAD-SLOT-DONE` и штатно завершился без `crash.dmp`. Это один игровой
save/load-маршрут, а не сверка всех значений камеры: сравнение существенных
полей с x86-оригиналом остаётся открытым. Полная игра на Linux/ARM64 этим
не доказана.

Тот же `S2_CAMERA/game.sav` без изменения (SHA-256
`2A581BD5DDC501337D4AFEAEB2A65D086A11DE16CF0D927529F1CD323ED37FE3`)
загружен в отдельном запуске `stage2-camera-wire-x86-oracle-01` из ранее
собранного чистого x86-архива `stage2-db-chunks-x86-20260924-01` (коммит
`101edc8`): `LOAD-SLOT-DONE`, штатный выход обоих процессов, дампа нет.
Это подтверждает, что прежний 32-битный загрузчик принимает получившийся
сейв; поведенческий/численный паритет камеры с историческим релизом не
следует из одного успешного чтения.

Геометрия декалей (2026-09-24, в работе): `NGScene::SVertex` в игровом
`CObjectInfo::SData::verts` имеет 32-байтную запись: `pos` (три float),
`normal/texU/texV` (три 4-байтных packed-вектора) и `tex` (два float).
Packed-векторы кодируются по компонентам `z,y,x,w`, а не через host-`DWORD`.
Парный массив `SVertexWeight` имеет 20-байтные элементы (четыре float и
четыре byte-индекса). Оба остаются исходными raw-векторами по форме чанка,
но их элементы теперь проходят явный полевой кодек. Контрактный тест
`PortableVertexWireTests` проверил точные байты и неверные длины на Linux
x86-64 и ARM64/QEMU под ASan/UBSan без LeakSanitizer. На Windows x64 собраны
`Game.exe`, переносимый и нативный тест, прошли 15/15 CTest. Коммит
`b820a31` собран в чистый x64-архив `stage2-vertex-wire-20260924-01`
с 16 заданиями и нативным медиа без FMOD.

Запуск `stage2-vertex-wire-clean-01` загрузил старый `DB_OLD`, записал
контрольный `VERTEX_BASE/game.sav` (4 002 796 байт), затем выполнил
`explodesave VERTEX_BLAST 4 23 18 26 9`. Лог подтвердил четыре ожидающих
взрыва и перед сохранением `wavefronts=1 trackers=4 frames=48`.
`VERTEX_BLAST/game.sav` (4 733 182 байта) повторно загружен до
`LOAD-SLOT-DONE` без дампа. Размер сам по себе не доказывает наличие декали.
Для проверки именно этого пути `ProbeSavedDecalVertices` прочитал таблицу
объектов сейва и вложенные raw-векторы `CPerPolyDecalGeometry`: в
`VERTEX_BASE` — `decal_objects=0 vertices=0 weights=0`, в `VERTEX_BLAST` —
`decal_objects=1 vertices=5 weights=5`, с длинами 5×32 и 5×20 байт.
Воспроизведение пробника из корня реконструкции:

```powershell
$env:UCRTContentRoot='C:\Program Files (x86)\Windows Kits\10\'
$cmake='G:\SS\lab\tools\VS2022\Common7\IDE\CommonExtensions\Microsoft\CMake\CMake\bin\cmake.exe'
& $cmake --build G:\SS\lab\build-x64 --config RelWithDebInfo --target ProbeSavedDecalVertices --parallel 16
& G:\SS\lab\build-x64\RelWithDebInfo\ProbeSavedDecalVertices.exe G:\SS\lab\runs\stage2-vertex-wire-clean-01\user-data\save\default\VERTEX_BLAST\game.sav
```

Тот же `VERTEX_BLAST/game.sav` (SHA-256
`F87BBE0D3E7777473C0E58BF7953DFAB66E98EEF2A77DE121738831BE59B043F`)
без изменения скопирован в `stage2-vertex-wire-x86-oracle-01` из чистого
x86-архива `stage2-db-chunks-x86-20260924-01`: загрузка достигла
`LOAD-SLOT-DONE`, штатный выход, дампа нет. Это совместимость формата и
один маршрут ненулевой геометрии, не доказательство численного совпадения
декали и повреждений с историческим x86-релизом.

Базовые игровые геометрические поля (2026-09-24, в работе): `CVec2`, `CVec3`,
`CVec4` и `SHMatrix` теперь имеют явные полевые кодеки для исходных raw-чанков
8, 12, 16 и 64 байта соответственно. Кодеки записывают и читают float32 в
little-endian порядке без доступа к соседним полям host-структуры. Эти типы
используются в состоянии миссии, мира, частиц, декалей и света; перенос
заметно шире одного сценария. `PortableGeometryWireTests` проверил точные
байты, включая знак отрицательного нуля, и неверные размеры на Linux
x86-64 и ARM64/QEMU под ASan/UBSan без LeakSanitizer. Windows x64 сборка
игры и нативный тест прошли (17/17 CTest). Коммит `8bcec59` собран в чистый
x64-архив `stage2-geometry-wire-20260924-01` с 16 заданиями и нативным
медиа без FMOD. Запуск `stage2-geometry-wire-clean-01` загрузил старый
`DB_OLD`, записал `S2_GEOMETRY/game.sav` (4 004 325 байт), повторно загрузил
его до `LOAD-SLOT-DONE` и штатно завершился без дампа. Тот же файл без
изменения (SHA-256
`73CAA8831E212DE922360638C2BC49284F0E5C682747655A394A04334A43A265`)
передан запуску `stage2-geometry-wire-x86-oracle-01` из чистого старого
x86-архива `stage2-db-chunks-x86-20260924-01`: `LOAD-SLOT-DONE`, штатный
выход, дампа нет. Это доказывает совместимость одного save/load-маршрута,
но не численный паритет всех значений или игровых правил с историческим
x86-релизом.
`CQuat` пока остаётся raw: компоненты private в старом не-UTF-8 `Geom.h`,
и чтение их по смещению памяти здесь сознательно не используется.
