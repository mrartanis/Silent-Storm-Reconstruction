# Terrain data porting boundary

Stage 2 now compiles the game's original `Main/TerrainInfo.cpp` on Linux
x86-64 and ARM64, using its real `STerrainInfo`, `CTerrainInfoHolder`,
`CDBPtr` fields, and save-registration declarations. The module is built as
`s2_game_terrain_info` after `DG` and `CStructureSaver`. It now links into
`NativeTerrainInfoTests` on Linux; this is not a complete mission terrain loader.

Getting the actual source through GCC/Clang required portable include paths
through `TerrainInfo.h`, `DataFormat.h`, the included DBFormat headers, and
`ADOImport/BasicDB.h`; a dependent iterator type in `PushItem`; forward
declaration of `CDBTable<T>`/`NDatabase::GetTable<T>`; dependent-base member
qualification in `CDBPtr<T>`; a fixed-underlying-type forward enum for
`EHitLocation`; and a distinct Linux declaration for the game generator
otherwise named `random` (a POSIX libc name). `DataConst.h` was converted
from CP1251 to UTF-8 before its include paths changed. None of these changes
alters the terrain data algorithms or wire tags intentionally.

Linking a Linux test that constructs `CTerrainInfoHolder` needed
`NDatabase::bIsDatabaseLoading`,
`NDatabase::GetTable`, `CDBTableBase::GetDBRecord`, the record registry, and
the vtables/RTTI for `NDb::CMaterial` and `NDb::CRPGArmor`. The game's
original `ADOImport/BasicDB.cpp` now compiles on Linux as
`s2_game_database_runtime`: the game.db columnar path remains available,
while the SQL/ADO source importer and refresh path are Windows-only. The
original DBFormat records and `NDb::BuildMapLinks` now link into
`NativeGameDatabaseLoadTests`: all 155 tables from the baseline game.db
materialize on Linux x64/ARM64 (see `NATIVE-GAME-DB-LINUX.md`). That resolves
the database prerequisite. `NativeTerrainInfoTests` now executes the original
`CTerrainInfoHolder` and its DG invalidation on Linux. A second CTest invocation
loads the real baseline `game.db`, puts real `CMaterial`/`CRPGArmor` records
into a terrain spot and step-sound map, and checks the holder's deep copy and
record identities. Both variants also round-trip `STerrainInfo` through the
game's `CStructureSaver`; the database variant checks that material and armor
IDs resolve back to the same typed records. This is a synthetic terrain
round-trip, not deserialization of a mission terrain. It does not exercise
the full `CHeightLayers`/`aiGrid` path.

On the authorized Linux host, the compile boundary is reproducible with:

```sh
cmake -S /tmp/s2-matrix-links.zCkO0O/src -B /tmp/s2-matrix-links.zCkO0O/build-x64 -DS2_GAME_DB_PATH=/tmp/s2-matrix-links.zCkO0O/game.db
cmake --build /tmp/s2-matrix-links.zCkO0O/build-x64 -j 16
ASAN_OPTIONS=detect_leaks=0 ctest --test-dir /tmp/s2-matrix-links.zCkO0O/build-x64 --output-on-failure -j 8
cmake -S /tmp/s2-matrix-links.zCkO0O/src -B /tmp/s2-matrix-links.zCkO0O/build-arm64 -DS2_GAME_DB_PATH=/tmp/s2-matrix-links.zCkO0O/game.db
cmake --build /tmp/s2-matrix-links.zCkO0O/build-arm64 -j 16
ASAN_OPTIONS=detect_leaks=0 ctest --test-dir /tmp/s2-matrix-links.zCkO0O/build-arm64 --output-on-failure -j 8
export CPLUS_INCLUDE_PATH=/usr/include/c++/11:/usr/include/x86_64-linux-gnu/c++/11:/usr/include/c++/11/backward
export LIBRARY_PATH=/usr/lib/gcc/x86_64-linux-gnu/11
cmake -S /tmp/s2-matrix-links.zCkO0O/src -B /tmp/s2-matrix-links.zCkO0O/build-clang-release -DS2_GAME_DB_PATH=/tmp/s2-matrix-links.zCkO0O/game.db
cmake --build /tmp/s2-matrix-links.zCkO0O/build-clang-release -j 16
ctest --test-dir /tmp/s2-matrix-links.zCkO0O/build-clang-release --output-on-failure -j 8
```

`NativeTerrainInfoTests` also remains linked against the full Windows
`Main`/DBFormat. It checks a deep-copied 18x10 height map and
per-region DG invalidation of geometry, texture, and grass; the output is
`terrain=196 geometry=2 texture=2 grass=2 untouched=1`. Windows x64
`NativeTerrainInfoTests` and a manual invocation with the baseline `game.db`
pass. Linux GCC x86-64 sanitizer and Clang 14 x86-64 release suites each
pass 62/62. GCC ARM64 also passes 62/62 under ASan/UBSan with the
serialization assertion included. The Windows x64 suite passes 89/89.
The full `CHeightLayers`/`aiGrid` path is still downstream.
The recovered x86 build printed the same region result. This small region
test has not been compared with runtime values from the original Steam
executable; the restored x86 build is only a supplemental implementation
check.

The copied Visual Studio requires `/p:UseEnv=true` with `vcvars64.bat` to
inherit the SDK UCRT include path. With that setting, the full Windows x64
game rebuild and 89/89 CTest passed. After building `NativeTerrainInfoTests`,
the typed-record round-trip can also be checked directly with:

```powershell
& 'G:\SS\lab\build-x64-stage2\Release\NativeTerrainInfoTests.exe' 'G:\SS\lab\baseline\game.db'
```

Clean native-media Windows x64 archive
`G:\SS\lab\builds\stage2-terrain-data-compile-20260925-01` came from
source commit `bf019b3`. A fresh linked-resource LabRun
`G:\SS\lab\runs\stage2-terrain-data-compile-clean-01` loaded the existing
`DB_OLD` mission slot to `LOAD-SLOT-DONE`, accepted `quit`, exited normally,
and produced no crash dump. This is a Windows regression after shared DB
header edits; it is not evidence of Linux mission data loading.
