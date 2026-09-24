# Этап 2: переносимое чтение игровых таблиц `game.db`

`FileIO/PortableGameDatabase.{h,cpp}` читает release-v1 `game.db` без Windows SDK,
ADO и `CStructureSaver`: верхние чанки, таблицу 155 объектов, карту ID таблиц,
колонночные массивы целых, IEEE-754 float и UTF-16LE строк, имена колонок,
описатели колонок и отношения. Это только используемый игрой путь
`CDBTableDataStorage`; редакторский импорт ADO не переносится. Живые игровые
объекты/ссылки и `BuildMapLinks` пока по-прежнему создаёт Windows-загрузчик.

Исходная база `G:\SS\lab\baseline\game.db` (SHA-256
`314D7AA9E6339F0E6826BF2A0C71FDAD992AEC1D6074A2ED763A6F6FA62D62DF`)
дала одинаковый результат на Windows x64 и Linux x86-64 (GCC 11.4,
`-std=c++17 -O1 -g -Wall -Wextra -Werror -fsanitize=address,undefined
-fno-omit-frame-pointer`):

```text
tables 155 relations 1 int-cells 2822643 float-cells 1624746 string-cells 487562 relation-links 1 shape-errors 0 hash 801dc8c67f9069c3
```

Переносимый пробник сравнен с журналом чистого Windows/x64 запуска
`G:\SS\lab\runs\stage2-field-bounds-clean-x64-01`: все 155 ID таблиц и
четвёрки `(rows, int-fields, float-fields, string-fields)` совпали без
исключений. Общий хеш выше считается из декодированных значений и имён;
равенство между ОС проверяет переносимость самого читателя. Отключаемая
диагностика `S2_DB_PARITY=1` выводит `DB-PARITY` хеш каждой таблицы
непосредственно из игрового `CDBTableDataStorage`. Чистый архив
`stage2-game-db-20260924-01` (коммит `24eea6a`, нативное медиа без FMOD)
запущен как `stage2-game-db-parity-x64-01`: все 155 хешей таблиц совпали
с переносимым пробником, различий 0. `DB_OLD` загрузился до
`LOAD-SLOT-DONE`, `quit` завершил игру/отладчик, дампа нет.

Отношения имеют отдельный хеш `35228b258fdbcf32`: Windows и Linux
переносимые пробники дали одинаковый результат под ASan/UBSan. Сверка
отношений с игровым загрузчиком добавлена в `S2_DB_PARITY=1` и требует
нового чистого запуска после этой правки.

Повторение локально:

```powershell
& 'G:\SS\lab\build-x64\RelWithDebInfo\PortableGameDatabaseProbe.exe' 'G:\SS\lab\baseline\game.db' --all
```

Границы: проверена одна выбранная оригинальная база v1, не все региональные
издания и повреждённые файлы; переносимый декодер ещё не подключён вместо
игрового `NDatabase::Serialize`. ARM64 и macOS не проверены. Репозиторий не
содержит лицензированные `game.db` и сохранения.
