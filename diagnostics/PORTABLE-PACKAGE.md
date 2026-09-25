# Этап 2: первый пакет переноса загрузки данных

Это проверка одного используемого игрой пути, а не завершение этапа 2.
`Main/GResource.cpp` открывает оригинальные `res/*.res` через
`FileIO/FilesPackage.cpp`. Путь чтения `CFilesPackage::Open` и
`CCachedFilesPackage::Open` теперь получает индекс из
`FileIO/PortablePackageIndex.cpp`; игровой `CPackageStream` затем читает
ресурсы по найденным смещениям. Путь создания/обновления архивов оставлен
историческим и сюда не входит.

Проверка источника данных 2026-09-25: все 23 `res/*.res` в лабораторном
`baseline` побайтно совпадают по SHA-256 с установленным Steam-набором;
`game.db` тоже совпадает (SHA-256
`314d7aa9e6339f0e6826bf2a0c71fdad992aec1d6074a2ed763a6f6fa62d62df`).
Таким образом приведённые ниже проверки исходных ресурсов относятся именно
к байтам Steam-версии, хотя старый x86-пробник — восстановленный, а не
оригинальный Steam EXE. Это не доказывает совпадение всех правил чтения
ресурсов с бинарным Steam-загрузчиком.

Путь открытия игрового архива теперь переносит Windows-разделители и
написание регистра на хост с чувствительной к регистру ФС:
`PortablePackageIndex` разрешает компоненты `./res/<name>.res` по точному
имени либо однозначному ASCII-сравнению без регистра, сохраняя разрешённый
путь для повторного чтения. При двух совпадениях с разным регистром запрос
отклоняется. Оба игровых варианта `CFilesPackage` и `CCachedFilesPackage`
используют этот путь до открытия потока; обновление/создание архивов для
редактора не менялось. Синтетический тест проверяет обратные слэши,
несовпадение регистра, повторное чтение и неоднозначность. Оригинальный
Steam `Fonts.res` (SHA-256
`a66f1fe5fd37b05c3436a2630c09d3ba598c928b88bc72e832d8fd984ee494ab`)
был прочитан на Linux x86-64 и ARM64/QEMU по заведомо неверному регистру
всех компонентов пути: 18 записей; ID 1, 6 и 33 дали те же длины и
FNV-1a-64, что Windows x64. Linux-проверки проходили под ASan/UBSan
(`detect_leaks=0`); временная копия архива на Linux-хосте удалена.

Повторение проверки исходного комплекта без копирования или изменения Steam:

```powershell
$steam='G:\SteamLibrary\steamapps\common\Silent Storm'
$baseline='G:\SS\lab\baseline'
if((Get-FileHash "$steam\game.db").Hash -ne (Get-FileHash "$baseline\game.db").Hash){throw 'game.db differs'}
$expected=@{}
Get-ChildItem "$steam\res" -File -Filter '*.res' | Get-FileHash | ForEach-Object {
  $expected[[IO.Path]::GetFileName($_.Path)]=$_.Hash
}
$actual=@(Get-ChildItem "$baseline\res" -File -Filter '*.res' | Get-FileHash)
if($expected.Count -ne 23 -or $actual.Count -ne 23 -or
   @($actual | Where-Object { $expected[[IO.Path]::GetFileName($_.Path)] -ne $_.Hash }).Count){throw 'res differs'}
```

Когда том с лабораторией заполнен, `Build-Lab.ps1 -ArchiveRoot` и
`New-LabRun.ps1 -ArchiveRoot/-RunRoot` позволяют хранить **новые** архивы
и изолированные запуски на другом томе, оставляя исходные Steam-данные,
toolchain и каталог сборки на прежнем месте. Эти параметры не удаляют и
не перемещают существующие артефакты. Для этого прогона использован
`C:\SS-lab\builds` и `C:\SS-lab\runs`; исходный `G:\SS\lab\baseline`
сохранился без изменений.
Неполный архив первого прогона, остановленного нехваткой места на G: при
копировании `Game.pdb`, перенесён без удаления свидетельств в
`C:\SS-lab\failed-builds\stage2-resource-case-20260925-01`.
Повторная чистая сборка из коммита `0784278` завершилась в
`C:\SS-lab\builds\stage2-resource-case-20260925-02` (16 заданий,
нативные звук/видео). Изолированный запуск
`C:\SS-lab\runs\stage2-resource-case-clean-01` загрузил старый
`DB_OLD` до `LOAD-SLOT-DONE` и завершился по `quit` без crash dump.
Windows x64 CTest — 26/26, Linux x86-64 и ARM64/QEMU под ASan/UBSan —
по 15/15. Это проверка игрового пути чтения на Windows и переносимого
разрешения имени на Linux/ARM64, не сборка всей игры для Linux/ARM64.

## Дисковый контракт

Заголовок: little-endian `uint32` сигнатура (`0x95938921` либо
`0x96948A22`) и `uint32` смещение индекса. Индекс сериализован
`CStructureSaver`: внешний chunk 1, внутри map chunk 1, затем пары key
(tag 1, 4 байта) и value (tag 2, два `uint32`: offset, length).
Переносимый модуль читает числа побайтно, проверяет границы и дубликаты,
строит обычный контейнер в памяти. Он не десериализует указатели или узлы
`unordered_map` с диска. Ограничение формата: архив не более 4 ГиБ.
Поля ресурсов внутри архива (текстуры, звук, модели и т. д.) ещё не
перенесены этим изменением.

## Эталон и проверка

Файл `G:\SS\lab\baseline\res\Fonts.res`: 185840 байт, SHA-256
`A66F1FE5FD37B05C3436A2630C09D3BA598C928B88BC72E832D8FD984EE494AB`.
В нём 18 записей. До подключения переносимого модуля собран и сохранён
старый x86 `PackageOracleProbe.exe`: SHA-256
`5EFA2786D3928E1717D0C21F0F7F80C853AAD40C3B3322E22EFBB9FDD8B3C467`;
копия лежит в `G:\SS\lab\evidence\package-oracle-preportable-x86.exe`.
Он читает ресурсы через исторический `OpenFilesPackage`/`CPackageStream`.

На Windows/x64 `PortablePackageProbe` и подключённый к `FileIO`
`PackageOracleProbe` дали те же пары длина/FNV-1a-64, что старый x86
загрузчик, для всех 18 ID: 1, 6, 11, 12, 14, 16, 17, 19, 20, 21,
22, 23, 25, 26, 27, 30, 31, 33. Новый x86-загрузчик также дал 18/18
совпадений со старым x86. Вся коллекция из 23 оригинальных `.res`
открылась переносимым разборщиком на Windows/x64 (проверка индекса,
но не каждого вложенного ресурса).
Дополнительно для каждого из 23 архивов выбраны первый, средний и
последний ID в отсортированном индексе: 69/69 ресурсов совпали по длине
и FNV-1a-64 между прежним x86-загрузчиком, переносимым x64-разборщиком
и новым игровым x64-путём `FileIO`. Это выборка, а не побайтная проверка
всех записей всех архивов.
Повторение выборки: `diagnostics/Test-PackageParity.ps1 -GameRoot
G:\SS\lab\baseline` (пути к трём пробам можно переопределить параметрами).

На предоставленном Linux-хосте `artanis.c.ibgene.org` (x86-64,
GCC 11.4) тот же оригинальный `Fonts.res` с той же SHA-256 прочитан
`PortablePackageProbe`, собранным с `-std=c++17 -Wall -Wextra -Werror
-fsanitize=address,undefined -fno-omit-frame-pointer`; все 18 пар
длина/FNV-1a-64 совпали с Windows и старым x86-загрузчиком. ASan/UBSan
ошибок не сообщили. Это Linux-проверка модуля, не Linux-сборка всей игры.
После сверки временный каталог с копией лицензионного `Fonts.res` на
Linux-хосте удалён; оригинал остался только в локальной лаборатории.

Обе Windows-сборки `Game` (`RelWithDebInfo`, x86 и x64) прошли. Добавлен
`PortablePackageIndexTests`: синтетический архив проверяет чтение,
отклонение неверной сигнатуры, выхода записи за границу и дубликата ID.
Он прошёл на Windows x86/x64 и Linux x86-64 под ASan/UBSan; вместе с ним
на каждой архитектуре Windows проходят пять CTest. Для дымовой проверки
новый инкрементально собранный x64 `Game.exe` (SHA-256
`BCF4CBC1C19E4B594B7308E4243B8590A619DEEAA42F23D75940330E6F787B1D`)
был подставлен в отдельный лабораторный запуск
`G:\SS\lab\runs\stage2-package-smoke-20260924-01`, созданный из
`native-sfx-20260924-01`. С `-windowed -800 -harness` процесс дошёл до
отвечающего окна `Silent Storm` и был штатно закрыт. Это именно smoke
запуска с текущим изменённым EXE, не чистая архивная сборка и не
прохождение миссии.

После коммита `bdd2a2b` скрипт `Build-Lab.ps1 -Architecture x64
-NativeMedia` создал чистый архив
`G:\SS\lab\builds\stage2-package-20260924-01` (`Game.exe` SHA-256
`591E5CC9B9FE877245F062B4BF3BA30DD6F39B1642B29B3A9BD851FF597E26ED`).
В нём нет `fmod.dll`, а `Game.exe` не импортирует FMOD. Из архива создан
`G:\SS\lab\runs\stage2-package-clean-smoke-01` с `-SkipIntro`;
`Start-LabRun.ps1` под CDB дошёл до отвечающего окна `Silent Storm`,
консоль записала `Executing .\cfg\lab-no-intro.cfg`, процесс штатно
закрылся, дампа нет. Проверка миссии и ручное прослушивание эффектов и
реплик остаются отдельными открытыми пунктами.

Повторение на данном стенде в PowerShell:

```powershell
$env:UCRTContentRoot='C:\Program Files (x86)\Windows Kits\10\'
$cmake='G:\SS\lab\tools\VS2022\Common7\IDE\CommonExtensions\Microsoft\CMake\CMake\bin\cmake.exe'
& $cmake --build G:\SS\lab\build-x64 --config RelWithDebInfo --target Game PortablePackageProbe PackageOracleProbe
$ids=1,6,11,12,14,16,17,19,20,21,22,23,25,26,27,30,31,33
$res='G:\SS\lab\baseline\res\Fonts.res'
& G:\SS\lab\evidence\package-oracle-preportable-x86.exe $res @ids
& G:\SS\lab\build-x64\RelWithDebInfo\PortablePackageProbe.exe $res @ids
& G:\SS\lab\build-x64\RelWithDebInfo\PackageOracleProbe.exe $res @ids
```

Вывод `PortablePackageProbe` содержит дополнительную первую строку с
размером архива, смещением индекса и количеством записей; при сравнении
ресурсов её следует пропустить.

Повторение Linux-проверки из временного каталога, куда скопированы ровно
`FileIO/PortablePackageIndex.{h,cpp}`,
`diagnostics/PortablePackageProbe.cpp` и оригинальный `data/Fonts.res`:

```sh
g++ -std=c++17 -O1 -g -Wall -Wextra -Werror \
  -fsanitize=address,undefined -fno-omit-frame-pointer \
  FileIO/PortablePackageIndex.cpp diagnostics/PortablePackageProbe.cpp \
  -o PortablePackageProbe
sha256sum data/Fonts.res
./PortablePackageProbe data/Fonts.res 1 6 11 12 14 16 17 19 20 21 22 23 25 26 27 30 31 33
```

Открытые границы: ARM64, macOS, правила регистра путей/кодировки, разделение
ресурсов и сохранений, регрессии игровых правил, другие загрузчики и
долгий игровой прогон. Пройденный тест одного `.res` не доказывает их.
