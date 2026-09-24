# Этап 2: ARM64-проверка переносимых модулей

2026-09-24, исходный коммит `c4c80be`. На разрешённом Linux-хосте
`artanis.c.ibgene.org` (Ubuntu 22.04, x86-64) установлены
`g++-aarch64-linux-gnu`, `qemu-user-static`, `cmake` и `ninja-build` из
репозиториев Ubuntu. `git archive HEAD` распакован в изолированный
`/tmp/s2-arm64.8vX0vt/src`; лицензионные `game.db` и `Fonts.res` передавались
только на время проверок и затем удалены с хоста. Локальные оригиналы в lab
не изменялись.

На Windows источник готовился командой
`git archive --format=tar --output=G:/SS/lab/evidence/stage2-arm64-source-c4c80be.tar HEAD`
и передавался `scp` в `source.tar` указанного временного каталога. На Linux
выполнены `mkdir src` и `tar -xf source.tar -C src`. Исходные игровые данные
передавались отдельно через `scp` в этот же каталог, сверялись `sha256sum`,
а после проб удалены только две точные временные копии. Не добавлять их в Git.

Проверяемая сборка — настоящие AArch64 ELF-файлы, выполняемые через
`qemu-aarch64-static` с кросс-системными библиотеками Ubuntu. Команды:

```sh
cmake -S /tmp/s2-arm64.8vX0vt/src -B /tmp/s2-arm64.8vX0vt/build -G Ninja \
  -DCMAKE_SYSTEM_NAME=Linux -DCMAKE_SYSTEM_PROCESSOR=aarch64 \
  -DCMAKE_C_COMPILER=aarch64-linux-gnu-gcc \
  -DCMAKE_CXX_COMPILER=aarch64-linux-gnu-g++ \
  '-DCMAKE_CROSSCOMPILING_EMULATOR=/usr/bin/qemu-aarch64-static;-L;/usr/aarch64-linux-gnu' \
  -DCMAKE_BUILD_TYPE=RelWithDebInfo
cmake --build /tmp/s2-arm64.8vX0vt/build -j 6
ctest --test-dir /tmp/s2-arm64.8vX0vt/build --output-on-failure
```

Все 7 переносимых тестов прошли: пути, `.res`-индекс, чанки структуры,
игровые FaceGen/голова/кривые/последовательности. Та же матрица собрана с
`-DCMAKE_CXX_FLAGS=-fsanitize=address,undefined` и
`-DCMAKE_EXE_LINKER_FLAGS=-fsanitize=address,undefined`; с
`ASAN_OPTIONS=detect_leaks=0` все 7 тестов прошли под ASan/UBSan. LeakSanitizer
при QEMU user-mode завершался ошибкой инфраструктуры, поэтому проверка утечек
здесь **не** заявляется.

Оригинальная `game.db` с SHA-256
`314D7AA9E6339F0E6826BF2A0C71FDAD992AEC1D6074A2ED763A6F6FA62D62DF`
декодирована ARM64-пробником как 155 таблиц, 1 отношение, 2 822 643 целых,
1 624 746 float, 487 562 UTF-16 строк и 1 link; shape-errors 0. Общий хеш
`801dc8c67f9069c3`, хеш отношений `35228b258fdbcf32`. Вывод `--all`
содержит 157 строк и побуквенно совпал с Windows x64 — все 155 табличных
значений/хешей. Декодер `game.db` отдельно прошёл под ARM64 ASan/UBSan.

`PortableStructureProbe game.db --objects --nested --shape`: 10 строк ARM64
совпали с Windows x64; найдены 155 записей и 155 тел объектов. Для
`Fonts.res` ARM64/Windows совпали список из 18 записей и чтение/хеш каждого
из 18 ресурсов; четыре выбранных ресурса отдельно прочитаны под ARM64
ASan/UBSan. Пример запуска пробника:

```sh
qemu-aarch64-static -L /usr/aarch64-linux-gnu \
  /tmp/s2-arm64.8vX0vt/build/PortableGameDatabaseProbe data/game.db --all
ASAN_OPTIONS=detect_leaks=0 qemu-aarch64-static -L /usr/aarch64-linux-gnu \
  /tmp/s2-arm64.8vX0vt/build-asan/PortableGameDatabaseProbe data/game.db
```

Это подтверждает переносимость перечисленных независимых читателей и
алгоритмов на ARM64. Это **не** сборка всей игры для Linux/ARM64, не проверка
реального ARM64-железа или GPU, не прямой паритет игровых правил с x86-эталоном
и не проверка утечек. Эти условия DoD этапа 2 остаются открытыми.
