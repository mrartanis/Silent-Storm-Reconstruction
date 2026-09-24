# Этап 2: игровой генератор SRand

`SRand::Get` используется игровыми правилами. Старое выражение умножало
знаковый `int` при обновлении seed и при масштабировании результата;
переполнение такого `int` не определено C++, хотя MSVC x86 выполнял
32-битное циклическое умножение. `Misc/PortableRand.h` фиксирует именно
два 32-битных циклических произведения и целочисленное деление с усечением
к нулю. `Misc/RandomGen.cpp` теперь вызывает этот переносимый шаг. Формат
`SRandomSeed` не менялся (один 32-битный `nSeed`).

`NativeRandParityTests` сравнивает переносимый шаг с прежним выражением,
скомпилированным MSVC как отдельный тестовый оракул, а не как игровой код:
семь начальных seed × семь значений `nMax` × 1000 шагов = 49 000
сравнений seed и результата. В том числе проверен случай переполнения
второго произведения (`seed=7645`, `nMax=100000` → `-54768`). Windows
x86/x64 выдали один хеш `21476277567e2a84`, оба теста завершились с кодом
0. `PortableRandTests` фиксирует первые десять значений seed=0 и тот же
случай переполнения; он прошёл на Windows x64, Linux x86-64 и ARM64/QEMU,
последние две архитектуры — с ASan/UBSan (`detect_leaks=0`). Windows x64
игра собрана, полный CTest: 23/23.

Воспроизведение в этом стенде:

```powershell
$env:UCRTContentRoot='C:\Program Files (x86)\Windows Kits\10\'
$cmake='G:\SS\lab\tools\VS2022\Common7\IDE\CommonExtensions\Microsoft\CMake\CMake\bin\cmake.exe'
& $cmake --build G:\SS\lab\build-x64 --config RelWithDebInfo --target Game PortableRandTests NativeRandParityTests --parallel 16
& G:\SS\lab\build-x64\RelWithDebInfo\NativeRandParityTests.exe
& $cmake --build G:\SS\lab\build-x86 --config RelWithDebInfo --target NativeRandParityTests --parallel 16
& G:\SS\lab\build-x86\RelWithDebInfo\NativeRandParityTests.exe
```

Это битовый паритет конкретного восстановленного MSVC-выражения, а не
полный паритет всех игровых случайных решений с розничным x86-релизом.
`CRandomGenerator` (ISAAC) и зависящая от системных файлов/часов обычная
инициализация seed здесь не переносились и остаются отдельной работой.

Чистый архив `stage2-rand-20260924-01` из коммита `24790dd` собран с
нативным звуком/видео. Запуск `stage2-rand-clean-01` загрузил старый
`DB_OLD`, принял harness-команду `rng 123456`, затем `turnsave RAND_TURN`;
новый слот появился и повторно загрузился до `LOAD-SLOT-DONE`. После
`quit` процесс завершился без дампа. Это проверяет интеграцию команды,
передачи хода и save/load на одном сценарии; сам по себе прогон не
сверяет поведение ИИ с оригинальным релизом.
