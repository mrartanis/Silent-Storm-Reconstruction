# Этап 2: маски частей сцены в сохранении

`NGScene::CPartFlags` — восемь 32-битных блоков маски частей объекта
(`Main/GRenderCore.h`). Игра сохраняет два экземпляра в `CCombinedPart`:
`lastNewFlags` и `ignoredParts` (`Main/GSceneInternal.h`, теги 4 и 6).
В исходном миссионном raw-аудите каждый тег встречался 125 раз за загрузку.
Это игровой путь загрузки сцены, не редакторский API.

`FileIO/PortablePartFlagsWire.h` фиксирует 32-байтный little-endian формат
восьми signed int32 без зависимости от раскладки класса в памяти.
Специализация `StructureFieldCodec<NGScene::CPartFlags>` читает и пишет
блоки через `GetBlock`/`SetBlock`. Тесты проверяют точные байты,
отрицательные маски, границы int32, неверную длину и байтовое совпадение
с нативной раскладкой MSVC x86/x64.

На 2026-09-25: Windows x64 CTest 58/58; целевые Windows x86-тесты 2/2;
Linux x86-64 и ARM64/QEMU под ASan/UBSan по 35/35. Чистый x64-архив
`D:\SS-lab\builds\stage2-part-flags-wire-20260925-01` из коммита
`2f0abfe` загрузил старый слот `stational weapons`, записал
`PART_FLAGS_NEW` и открыл его повторно до `LOAD-SLOT-DONE`. Оба
`CPartFlags` исчезли из raw-журнала, дампа нет. Чистый архив
восстановленной x86-сборки
`D:\SS-lab\builds\stage2-part-flags-wire-x86-20260925-01` прочитал
этот же новый слот до `LOAD-SLOT-DONE` без дампа. Каталоги запусков:
`D:\SS-lab\runs\stage2-part-flags-clean-01` и
`D:\SS-lab\runs\stage2-part-flags-x86-clean-01`. Все 23 исходных
`.res` сохранили контрольные SHA-256; записи сейвов изолированы в
`user-data` соответствующих запусков.

Команды регрессии из корня реконструкции:

```powershell
& 'G:\SS\lab\tools\VS2022\Common7\IDE\CommonExtensions\Microsoft\CMake\CMake\bin\ctest.exe' --test-dir 'D:\SS-lab\build-x64-stage2' -C RelWithDebInfo --output-on-failure -j 16
& 'G:\SS\lab\tools\VS2022\Common7\IDE\CommonExtensions\Microsoft\CMake\CMake\bin\ctest.exe' --test-dir 'D:\SS-lab\build-x86-stage2' -C RelWithDebInfo -R '^(PortablePartFlagsWireTests|NativePartFlagsWireTests)$' --output-on-failure
ssh artanis.c.ibgene.org 'ASAN_OPTIONS=detect_leaks=0 ctest --test-dir /tmp/s2-matrix-links.zCkO0O/build-x64 --output-on-failure -j 16 && ASAN_OPTIONS=detect_leaks=0 ctest --test-dir /tmp/s2-matrix-links.zCkO0O/build-arm64 --output-on-failure -j 4'
```

Проверка доказывает перенос формата и совместимость сейва с текущими
«восстановленными» x86-сорцами. Она не доказывает, что маски порождают
те же видимые части и тени, что оригинальная Steam-игра: Steam остаётся
поведенческим и визуальным эталоном, а x86-реконструкция — диагностикой.
