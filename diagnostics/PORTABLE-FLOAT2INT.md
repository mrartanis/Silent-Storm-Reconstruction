# Float2Int: перенос округления для игрового ядра

`Misc/Tools.h::Float2Int` используется в сетке ИИ, высотах, вокселях и
разрушаемых зданиях. Исторический Win32/x86 использует `FISTP`, а Windows
x64 — `CVTSS2SI`: обе инструкции округляют по текущему режиму FPU и
возвращают `INT32_MIN` при NaN или результате вне диапазона int32.
Прежняя ветка для всех не-x86 платформ безусловно вызывала SSE intrinsic,
поэтому не могла компилироваться для ARM64.

`Misc/PortableFloat2Int.h` реализует тот же целочисленный результат через
`std::nearbyint` и явную проверку диапазона. Windows x86/x64 продолжают
использовать свои прежние инструкции; на ARM64 игровой вызов из `Tools.h`
теперь попадает в переносимую ветку. Изменения округления на Windows в
этом коммите нет. Плавающие флаги исключений не сравнивались: контракт
касается возвращаемого int32 и соблюдения активного режима округления.

`PortableFloat2IntTests` проверяет четыре режима (`nearest-even`, к нулю,
вверх, вниз), положительные и отрицательные половины, границы int32,
переполнение, NaN и бесконечность. На x86-64 тест сравнивает каждый
результат непосредственно с `_mm_cvtss_si32`; на ARM64 сверяет
таблицу округления и граничные значения. Результаты: Windows x64 CTest
27/27; Linux x86-64 и ARM64/QEMU по 16/16 с ASan/UBSan
(`ASAN_OPTIONS=detect_leaks=0`). На Linux-хосте исходник теста и
заголовок находились в `/tmp/s2-matrix-links.zCkO0O/src`, сборки —
`build-x64` и `build-arm64` в том же каталоге. Повторить:

```sh
cmake --build /tmp/s2-matrix-links.zCkO0O/build-x64 --target PortableFloat2IntTests -j 16
ASAN_OPTIONS=detect_leaks=0 ctest --test-dir /tmp/s2-matrix-links.zCkO0O/build-x64 --output-on-failure
cmake --build /tmp/s2-matrix-links.zCkO0O/build-arm64 --target PortableFloat2IntTests -j 16
ASAN_OPTIONS=detect_leaks=0 ctest --test-dir /tmp/s2-matrix-links.zCkO0O/build-arm64 --output-on-failure
```

Ограничение: это перенос одного арифметического пути, а не сборка всей
игры на Linux/ARM64 и не прямой паритет всей симуляции со Steam. Для
последнего нужны одинаковое игровое состояние и сценарий на оригинальном
Steam EXE и новом порте. Чистый Windows-архив и игровой smoke указать
после их завершения.
