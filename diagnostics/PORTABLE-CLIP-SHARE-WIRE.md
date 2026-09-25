# Этап 2: ключ кеша строительных клипперов

`NGScene::SClipShare` используется игрой при построении геометрии стен и
сплошных частей зданий (`Main/GBuilding.cpp`, `Main/GObjectInfo.cpp`). Это не
редакторский путь: в миссионном raw-аудите старой сборки тип встречался в
классах `0x00571120` (48 записей за загрузку), `0x02741131` (82) и
`0x01321151` (130), суммарно 260 за одну загрузку. Поэтому оставлять его
неявным сырым блоком означало бы зависеть от раскладки структуры MSVC.

Формат занимает 20 байт little-endian: `src.nID` и `src.nPart` — два
signed int32, `dwParts` — uint32, `nSubBlockID` и `nClip` — signed int32.
`FileIO/PortableClipShareWire.h` читает и пишет поля отдельно; игровая
специализация находится в `Main/PortableClipShareCodecs.h`. Она включена
через `Main/PortableMeshCodecs.h`, который уже используется обеими игровыми
единицами трансляции. Старый однобайтовый `GObjectInfo.h` не перекодирован.
Проверяются размер и смещения полей на MSVC x86/x64, точные байты,
отрицательные значения, границы int32 и отказ при неверной длине.

На 2026-09-25: Windows x64 CTest 56/56; Windows x86 — два целевых теста;
Linux x86-64 и ARM64/QEMU под ASan/UBSan — по 34/34. Чистый x64-архив
`D:\SS-lab\builds\stage2-clip-share-wire-20260925-01` (коммит `44e7e4c`)
загрузил старый миссионный слот `stational weapons`, записал
`CLIP_SHARE_NEW` и открыл новый слот до `LOAD-SLOT-DONE`. В raw-журнале
этого запуска больше нет `SClipShare`, дампа нет. Чистый архив
восстановленной x86-сборки
`D:\SS-lab\builds\stage2-clip-share-wire-x86-20260925-01` открыл тот же
новый слот до `LOAD-SLOT-DONE` без дампа. Каталоги запусков —
`D:\SS-lab\runs\stage2-clip-share-clean-01` и
`D:\SS-lab\runs\stage2-clip-share-x86-clean-01`. Все 23 исходных `.res`
после проверки сохранили контрольные SHA-256. Ресурсы подключались
через junction к baseline; сохранения писались в отдельные `user-data`
каталогов запусков.

Повторение основных проверок из корня реконструкции:

```powershell
& 'G:\SS\lab\tools\VS2022\Common7\IDE\CommonExtensions\Microsoft\CMake\CMake\bin\ctest.exe' --test-dir 'D:\SS-lab\build-x64-stage2' -C RelWithDebInfo --output-on-failure -j 16
& 'G:\SS\lab\tools\VS2022\Common7\IDE\CommonExtensions\Microsoft\CMake\CMake\bin\ctest.exe' --test-dir 'D:\SS-lab\build-x86-stage2' -C RelWithDebInfo -R '^(PortableClipShareWireTests|NativeClipShareWireTests)$' --output-on-failure
ssh artanis.c.ibgene.org 'ASAN_OPTIONS=detect_leaks=0 ctest --test-dir /tmp/s2-matrix-links.zCkO0O/build-x64 --output-on-failure -j 16 && ASAN_OPTIONS=detect_leaks=0 ctest --test-dir /tmp/s2-matrix-links.zCkO0O/build-arm64 --output-on-failure -j 4'
```

Это доказывает формат и совместимость с текущими восстановленными x86
сорцами, но не паритет отображения зданий или поведения игры с оригинальным
Steam-релизом. Steam остаётся эталоном игровых решений и изображения;
восстановленная x86-сборка — лишь диагностическая реализация.
