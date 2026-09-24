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
```

Границы: это верхний слой чтения `game.db`, не перенос таблиц и ссылок
базы на Linux. Вложенная структура, объектная фабрика, `wchar_t`,
сжатие исторических сохранений, файлы пользователя и ARM64 ещё требуют
отдельной работы и проверок. Совпадение четырёх чанков не доказывает
паритет всех игровых записей.
