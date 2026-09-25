# Полный индексный корпус Steam `.res` (этап 2)

Эталон данных — неизменённая установка Steam
`G:\SteamLibrary\steamapps\common\Silent Storm\res`, а не собранный из
восстановленных исходников x86-архив. `PortablePackageCorpusProbe`
открывает каждый `.res` через игровой `PortablePackageIndex`, затем
читает **каждый индексный ID** через `Read` и считает FNV-1a-64 над
little-endian ID, длиной и байтами ресурса. Файлы перечисляются в
лексическом порядке; строка на архив содержит число записей, сумму
длин и хеш. Хеш — быстрый регрессионный отпечаток, не криптографическое
доказательство тождественности файлов. Отдельно следует сверять SHA-256
переданных исходных архивов.

Windows x64 на установленном Steam-комплекте прочитал все 23 архива:
57 880 ресурсов, 2 200 910 768 байт индексных данных. Общий размер
самих архивов больше за счёт индекса и неиндексированных областей.
В частности, `Textures.res` дал 9 880 ресурсов и FNV-1a-64
`1461b9f29bb55491`, `Sounds.res` — 8 466 ресурсов и
`499edd1450e40d26`. Повторить:

Контрольный вывод (Windows x64, Linux x86-64, ARM64/QEMU):

```text
AIBSPTrees.res entries=193 bytes=1185905 fnv64=2f94d4c486e4c3c8
AIBinds.res entries=211 bytes=106203 fnv64=c1fedd47cd24dcf0
AIGeometries.res entries=2043 bytes=22862531 fnv64=5005175ee45692d9
Animations.res entries=2855 bytes=142643593 fnv64=9475afbc8d056094
Binds.res entries=396 bytes=469923 fnv64=ac089c6c6ae1f8f4
Buildings.res entries=6331 bytes=68144194 fnv64=3eaab8a2dcbd983a
Chapters.res entries=27 bytes=16595573 fnv64=2598c2c80c543aa1
Effects.res entries=280 bytes=19833464 fnv64=d46f62585ac27b37
Fonts.res entries=18 bytes=185237 fnv64=67921489702c3a8b
Geometries.res entries=7790 bytes=167728286 fnv64=92061a2384f872fd
Globals.res entries=4 bytes=1568 fnv64=375d1bf95533f41c
Groups.res entries=42 bytes=1542 fnv64=41fa55cde68c07ef
Heads.res entries=134 bytes=5631272 fnv64=37da53bfa2e142c4
LRTextures.res entries=5912 bytes=4839540 fnv64=e7e79662d1e21121
Lights.res entries=21 bytes=20370 fnv64=caf1b199afc0dddd
Locators.res entries=152 bytes=10641 fnv64=ce91320e4c6e12ca
Sequences.res entries=6782 bytes=14375539 fnv64=9015141da0dc13e2
Skeletons.res entries=139 bytes=133412 fnv64=2df1334fd4cc7937
Sounds.res entries=8466 bytes=412767690 fnv64=499edd1450e40d26
Terrain.res entries=3671 bytes=21214581 fnv64=5ad765318c0c6dd3
Textures.res entries=9880 bytes=1302045372 fnv64=1461b9f29bb55491
Units.res entries=169 bytes=4860 fnv64=fb1e9097aaa878cf
Waypoints.res entries=2364 bytes=109472 fnv64=83051633dbdabcd3
total archives=23 entries=57880 bytes=2200910768
```

Повторить сборку и Windows-чтение:

```powershell
$env:UCRTContentRoot='C:\Program Files (x86)\Windows Kits\10\'
$cmake='G:\SS\lab\tools\VS2022\Common7\IDE\CommonExtensions\Microsoft\CMake\CMake\bin\cmake.exe'
& $cmake --build 'G:\SS\lab\build-x64' --config RelWithDebInfo --target PortablePackageCorpusProbe --parallel 16
& 'G:\SS\lab\build-x64\RelWithDebInfo\PortablePackageCorpusProbe.exe' 'G:\SteamLibrary\steamapps\common\Silent Storm\res'
```

На `artanis.c.ibgene.org` только эти 23 файла были скопированы в
`/tmp/s2-steam-res.rjIGfU/res`: число архивов и сумма размеров совпали,
23/23 SHA-256 равны файлам установленной Steam-версии. Linux x86-64 и
ARM64/QEMU под ASan/UBSan (`ASAN_OPTIONS=detect_leaks=0`) прочли весь
корпус с кодом возврата 0. Все 24 строки (23 архива и итог) совпали с
Windows x64 дословно после нормализации перевода строк. Повторение до
удаления временной копии:

```sh
ASAN_OPTIONS=detect_leaks=0 /tmp/s2-matrix-links.zCkO0O/build-x64/PortablePackageCorpusProbe /tmp/s2-steam-res.rjIGfU/res
ASAN_OPTIONS=detect_leaks=0 qemu-aarch64-static -L /usr/aarch64-linux-gnu \
  /tmp/s2-matrix-links.zCkO0O/build-arm64/PortablePackageCorpusProbe /tmp/s2-steam-res.rjIGfU/res
```

Копировать следует только 23 файла `*.res`, не служебные распакованные
деревья из `res`. Временная Linux-копия после проверки удалена из
`/tmp/s2-steam-res.rjIGfU`; оригинальная установка Steam не менялась.
Для повторения заново создать временную директорию и передать туда
исходные `.res` из Steam, затем проверить все SHA-256.

Граница проверки: охвачены все индексные записи 23 архивов базовой игры.
Это не проверяет loose-файлы, моды, декодирование изображений/звука,
подгрузку каждого ресурса реальной игровой сессией и не является
сборкой всей игры на Linux/ARM64.

Из коммита `1d196b7` собран чистый x64-архив с native media
`C:\SS-lab\builds\stage2-steam-res-corpus-20260925-01`.
`C:\SS-lab\runs\stage2-steam-res-corpus-clean-01` загрузил старый
`DB_OLD` до `LOAD-SLOT-DONE` и завершился по `quit` без crash dump.
Этот игровой smoke подтверждает чистую Windows-интеграцию текущей
ревизии, но не заменяет полный проход пробника и не утверждает,
что игровая сессия запросила все 57 880 ресурсов.
