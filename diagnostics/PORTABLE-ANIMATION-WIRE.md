# Игровые ключи анимации и позы костей на диске

Коммит `a19a875` заменил raw-копирование структур
`NAnimation::SBoneAnimKey`, `SRootAnimKey`, `SAddBoneAnimKey`,
`SMSRAnimKey` и `SBonePose` в `CStructureSaver` на явные little-endian
поля: `float32` компонентов и, где нужно, знаковый `int32` родительской
кости. Переносятся типы игрового загрузчика анимаций и скелета, а не
весь инструментарий редактора. `SMSRAnimKey` входит в формат
`CFileAnimation::Recalc`, но именно в проверенной миссии его чтение
отдельно не наблюдалось.

`PortableAnimationWireTests` проверяет байты, round-trip и неверную длину
записи без зависимости от архитектуры. `NativeAnimationWireTests`
проверяет все пять игровых структур и соответствие прежним дисковым
байтам на текущем Windows ABI. После переноса Windows x64 CTest прошёл
68/68, целевые x86-тесты — 2/2; Linux x86-64 и ARM64/QEMU под ASan/UBSan
прошли по 40/40. Это проверка формата и сборки тестовых целей, не сборка
всей игры под Linux или ARM64.

Чистые архивы одного коммита: x64 с FFmpeg/miniaudio
`D:\SS-lab\builds\stage2-animation-wire-20260925-01` и диагностический
x86 `D:\SS-lab\builds\stage2-animation-wire-x86-20260925-01`.
В `D:\SS-lab\runs\stage2-animation-clean-01` x64 загрузил прежний
`MAP_STOREY_NEW`, сохранил `ANIMATION_NEW`, повторно его загрузил до
`LOAD-SLOT-DONE` и вышел через harness без crash dump. В этом прогоне
`_wireaudit.log` в проходе загрузки сообщил 14 оставшихся raw-путей
(в основном декали, свет, камера); последние пустые блоки журнала
относятся к другим проходам сериализатора и не отменяют эти находки.
Из-за ограниченного сценария и того, что аудит не покрывает ручной
`AddRawData(void*)`, число 14 не является полным списком сырых записей игры.
Новый `game.sav` (SHA-256
`55ac2168d975b36cdfd95376143db9acef36a09b66571aa770ef81056e38c195`)
прочитала и чистая восстановленная x86-сборка в
`D:\SS-lab\runs\stage2-animation-x86-01` до `LOAD-SLOT-DONE`, без дампа.

Более сильная проверка — неизменённый оригинальный Steam EXE в
изолированной копии: он показал `ANIMATION_NEW` в меню и открыл слот до
игрового экрана с персонажем, мотоциклами, зданием, 55 AP и 75/75 HP.
После штатного выхода хеш копии сейва совпал с источником, установленная
Steam-игра прошла контроль неизменности 2698/2698 файлов. Подробности
оригинального EXE и метода — в `STEAM-SAVE-ORACLE.md`. Это подтверждает
чтение конкретного сейва, но не паритет всех анимаций, поз или кадров.

Повторение: запустить `ctest --test-dir <build> --output-on-failure` на
соответствующей платформе. Для игрового цикла создать LabRun из указанного
x64-архива через `New-LabRun.ps1 -SkipIntro -LinkResources`, скопировать
слот в `<run>/user-data/save/default`, записать его имя в
`<run>/game/_loadslot.txt`, запустить `Start-LabRun.ps1 -GameArguments
'-windowed -800 -harness -loadslot'`. После `LOAD-SLOT-DONE` послать
`save ANIMATION_NEW`, затем `load ANIMATION_NEW` и `quit` через
`<run>/game/_harness_cmd.txt`; проверить `_saveload.log`, `_wireaudit.log`,
отсутствие дампа и хеш `game.sav`. Для Steam использовать только
изолированную копию и обычное меню загрузки, без подмены EXE/DLL.
