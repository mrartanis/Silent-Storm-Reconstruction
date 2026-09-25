# Этап 2: точки AI-геометрии

`NAI::SJunction` встречается в игре как массив 16-байтных raw-записей:
три координаты `CVec3`, байт `bGround` и три байта выравнивания. Данные
используются `CGeometryInfo` для геометрии объектов, а не только редактором.
`FileIO/PortableJunction.h` теперь читает три little-endian float и флаг,
игнорирует старые байты padding и записывает их нулями. Специализация
`StructureFieldCodec<SJunction>` подключена в `Main/aiObject.h`.

`PortableJunctionTests` фиксирует байты, знаковый ноль и канонизацию
ненулевого флага/padding. `NativeJunctionTests` проверяет размер,
смещения и совпадение значимых байтов с объектом MSVC. На 2026-09-25
Windows x64 Game собран, CTest 37/37; два новых теста прошли и на
диагностическом Windows x86. Linux x86-64 и ARM64/QEMU прошли по 21/21
с ASan/UBSan (`detect_leaks=0` на QEMU). Это контракт данных, а не
проверка поведения AI-геометрии против Steam.
