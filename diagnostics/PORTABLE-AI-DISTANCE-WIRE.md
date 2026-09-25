# Этап 2: сохранённые дистанции между зонами ИИ

`NAI::CColouredWaysCalcer` сохраняет карту `distances`, значения которой
имеют тип `NAI::SDistanceInfo` (`Main/aiColourer.h`, класс `0x72452120`,
тег 6 карты и тег 2 значения). В миссионном raw-аудите тип встречался
28 раз за загрузку. Это игровое состояние поиска пути, а не редакторская
структура; указателей внутри значения нет.

Исторический формат значения — 32 байта: `distance` (uint16),
`parent` (зона 8 байт), `isProceeded`/`isInList` (два байта),
`prev` и `next` (по 8 байт). Зона состоит из `wColor` (uint16) и
`nLayer` (int32). Между полями есть байты выравнивания. Кодек
`FileIO/PortableAIDistanceWire.h` их игнорирует при чтении и обнуляет
при записи; ненулевое значение флага декодируется как `true`, обратно
записывается 0/1. Игровая специализация в `Main/aiColourer.h`
восстанавливает именно поля. Тесты проверяют точные байты,
смещения MSVC x86/x64, отрицательные слои, ненулевые padding-байты,
неканонический флаг и неверную длину.

На 2026-09-25: Windows x64 CTest 60/60, целевые Windows x86-тесты
2/2, Linux x86-64 и ARM64/QEMU под ASan/UBSan по 36/36. Чистый x64
архив `D:\SS-lab\builds\stage2-ai-distance-wire-20260925-01` из
коммита `e355fe7` загрузил старый слот `stational weapons`, записал
`AI_DISTANCE_NEW` и повторно открыл его до `LOAD-SLOT-DONE`.
`SDistanceInfo` исчез из raw-аудита, дампа нет. Чистая восстановленная
x86-сборка из архива
`D:\SS-lab\builds\stage2-ai-distance-wire-x86-20260925-01` открыла
тот же новый слот до `LOAD-SLOT-DONE` без дампа. Каталоги запусков:
`D:\SS-lab\runs\stage2-ai-distance-clean-01` и
`D:\SS-lab\runs\stage2-ai-distance-x86-clean-01`. Контрольные SHA-256
всех 23 `.res` остались неизменными.

Основные команды регрессии из корня реконструкции:

```powershell
& 'G:\SS\lab\tools\VS2022\Common7\IDE\CommonExtensions\Microsoft\CMake\CMake\bin\ctest.exe' --test-dir 'D:\SS-lab\build-x64-stage2' -C RelWithDebInfo --output-on-failure -j 16
& 'G:\SS\lab\tools\VS2022\Common7\IDE\CommonExtensions\Microsoft\CMake\CMake\bin\ctest.exe' --test-dir 'D:\SS-lab\build-x86-stage2' -C RelWithDebInfo -R '^(PortableAIDistanceWireTests|NativeAIDistanceWireTests)$' --output-on-failure
ssh artanis.c.ibgene.org 'ASAN_OPTIONS=detect_leaks=0 ctest --test-dir /tmp/s2-matrix-links.zCkO0O/build-x64 --output-on-failure -j 16 && ASAN_OPTIONS=detect_leaks=0 ctest --test-dir /tmp/s2-matrix-links.zCkO0O/build-arm64 --output-on-failure -j 4'
```

Дополнительная прямая проверка: неизменённый Steam EXE открыл тот же
`AI_DISTANCE_NEW` до видимого игрового экрана с 55 AP и 75/75 здоровья.
Процедура, хеши и снимки — в [STEAM-SAVE-ORACLE.md](STEAM-SAVE-ORACLE.md).
Это сильнее чтения восстановленной x86-сборкой, но всё ещё не доказывает
совпадение выбранных маршрутов или динамического поведения ИИ. Steam
остаётся поведенческим эталоном; x86-реконструкция — диагностикой.
