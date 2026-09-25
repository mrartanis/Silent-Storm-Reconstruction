# Terrain data porting boundary

Stage 2 now compiles the game's original `Main/TerrainInfo.cpp` on Linux
x86-64 and ARM64, using its real `STerrainInfo`, `CTerrainInfoHolder`,
`CDBPtr` fields, and save-registration declarations. The module is built as
`s2_game_terrain_info` after `DG` and `CStructureSaver`; it is not yet linked
into a Linux executable or claimed as a working Linux terrain loader.

Getting the actual source through GCC/Clang required portable include paths
through `TerrainInfo.h`, `DataFormat.h`, the included DBFormat headers, and
`ADOImport/BasicDB.h`; a dependent iterator type in `PushItem`; forward
declaration of `CDBTable<T>`/`NDatabase::GetTable<T>`; dependent-base member
qualification in `CDBPtr<T>`; a fixed-underlying-type forward enum for
`EHitLocation`; and a distinct Linux declaration for the game generator
otherwise named `random` (a POSIX libc name). `DataConst.h` was converted
from CP1251 to UTF-8 before its include paths changed. None of these changes
alters the terrain data algorithms or wire tags intentionally.

The next boundary is concrete: linking a Linux test that constructs
`CTerrainInfoHolder` needs `NDatabase::bIsDatabaseLoading`,
`NDatabase::GetTable`, `CDBTableBase::GetDBRecord`, the record registry, and
the vtables/RTTI for `NDb::CMaterial` and `NDb::CRPGArmor`. The game's
original `ADOImport/BasicDB.cpp` now compiles on Linux as
`s2_game_database_runtime`: the game.db columnar path remains available,
while the SQL/ADO source importer and refresh path are Windows-only. The
original DBFormat records and `NDb::BuildMapLinks` now link into
`NativeGameDatabaseLoadTests`: all 155 tables from the baseline game.db
materialize on Linux x64/ARM64 (see `NATIVE-GAME-DB-LINUX.md`). That resolves
the database prerequisite but does not yet link or execute
`CTerrainInfoHolder` on Linux. No Linux terrain runtime-test result is
claimed yet.

On the authorized Linux host, the compile boundary is reproducible with:

```sh
cmake --build /tmp/s2-matrix-links.zCkO0O/build-x64 --target s2_game_terrain_info -j 16
cmake --build /tmp/s2-matrix-links.zCkO0O/build-arm64 --target s2_game_terrain_info -j 16
cmake --build /tmp/s2-matrix-links.zCkO0O/build-x64 --target s2_game_database_runtime -j 16
cmake --build /tmp/s2-matrix-links.zCkO0O/build-arm64 --target s2_game_database_runtime -j 16
export CPLUS_INCLUDE_PATH=/usr/include/c++/11:/usr/include/x86_64-linux-gnu/c++/11:/usr/include/c++/11/backward
export LIBRARY_PATH=/usr/lib/gcc/x86_64-linux-gnu/11
cmake --build /tmp/s2-matrix-links.zCkO0O/build-clang-release --target s2_game_terrain_info -j 16
cmake --build /tmp/s2-matrix-links.zCkO0O/build-clang-release --target s2_game_database_runtime -j 16
```

`NativeTerrainInfoTests` is currently Windows-only, linked against the full
Windows `Main`/DBFormat. It checks a deep-copied 18x10 height map and
per-region DG invalidation of geometry, texture, and grass; the output is
`terrain=196 geometry=2 texture=2 grass=2 untouched=1`. Windows x64 full
CTest passed 89/89. After the game.db runtime addition, Linux GCC x86-64 and
ARM64 sanitizer suites and Clang 14 x86-64 release each pass 60/60. These
Linux suites include compilation of `s2_game_terrain_info` and execution of
the typed database loader, but no Linux terrain execution test. The full
`CHeightLayers`/`aiGrid` path is still downstream.
The recovered x86 build printed the same region result. This small region
test has not been compared with runtime values from the original Steam
executable; the restored x86 build is only a supplemental implementation
check.

The copied Visual Studio requires `/p:UseEnv=true` with `vcvars64.bat` to
inherit the SDK UCRT include path. With that setting, the full Windows x64
game rebuild and 89/89 CTest passed.

Clean native-media Windows x64 archive
`G:\SS\lab\builds\stage2-terrain-data-compile-20260925-01` came from
source commit `bf019b3`. A fresh linked-resource LabRun
`G:\SS\lab\runs\stage2-terrain-data-compile-clean-01` loaded the existing
`DB_OLD` mission slot to `LOAD-SLOT-DONE`, accepted `quit`, exited normally,
and produced no crash dump. This is a Windows regression after shared DB
header edits; it is not evidence of Linux mission data loading.
