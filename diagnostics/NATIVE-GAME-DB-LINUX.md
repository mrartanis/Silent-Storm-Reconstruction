# Original game.db runtime on Linux

Stage 2 now builds and **executes** the game's original `ADOImport/BasicDB.cpp`
and all 18 game-used `DBFormat/Data*.cpp` translation units on Linux x86-64
and ARM64. This is the release-v1 columnar `game.db` path: the original
`CDBRecord::Import()` methods materialize typed records, then original
`NDb::BuildMapLinks` creates game links. SQL/ADO import and refresh remain
Windows/editor-only. This test does not load a mission or imply that the whole
game loop has been ported to Linux.

The test input is `G:\SS\lab\baseline\game.db`, SHA-256
`314D7AA9E6339F0E6826BF2A0C71FDAD992AEC1D6074A2ED763A6F6FA62D62DF`.
`NativeGameDatabaseLoadTests` independently decodes that file with
`PortableGameDatabase`, then compares the source row count against the
original runtime record count for **all 155 tables**. It also compares the
IEEE-754 `SpecFactor` bytes for every material record against its source
column: 3,498 checks. GCC x86-64 ASan/UBSan, GCC ARM64 ASan/UBSan under QEMU,
and Clang x86-64 all load the file and pass. Complete suites: Windows x64
89/89; Linux GCC x86-64, GCC ARM64, and Clang x86-64 each 60/60. The ARM64 `CMAKE_CROSSCOMPILING_EMULATOR`
is `qemu-aarch64-static -L /usr/aarch64-linux-gnu`; set
`ASAN_OPTIONS=detect_leaks=0` because LeakSanitizer cannot run under QEMU's
ptrace. The original database load logs 69 grouped unresolved-reference
diagnostics. That remains a separate data/behavior comparison with Steam;
record-count parity does not establish that every reference is valid.

On the authorized Linux host, after copying this source tree and the licensed
`game.db` to `/tmp/s2-matrix-links.zCkO0O/`, run:

```sh
cmake -S /tmp/s2-matrix-links.zCkO0O/src -B /tmp/s2-matrix-links.zCkO0O/build-x64 \
  -DCMAKE_BUILD_TYPE=Release -DS2_GAME_DB_PATH=/tmp/s2-matrix-links.zCkO0O/game.db
cmake --build /tmp/s2-matrix-links.zCkO0O/build-x64 -j 16
ASAN_OPTIONS=detect_leaks=0 ctest --test-dir /tmp/s2-matrix-links.zCkO0O/build-x64 --output-on-failure -j 8

cmake -S /tmp/s2-matrix-links.zCkO0O/src -B /tmp/s2-matrix-links.zCkO0O/build-arm64 \
  -DS2_GAME_DB_PATH=/tmp/s2-matrix-links.zCkO0O/game.db
cmake --build /tmp/s2-matrix-links.zCkO0O/build-arm64 -j 16
ASAN_OPTIONS=detect_leaks=0 ctest --test-dir /tmp/s2-matrix-links.zCkO0O/build-arm64 --output-on-failure -j 8
```

Clang 14 on this host needs the installed GCC 11 standard-library include
and linker paths:

```sh
export CPLUS_INCLUDE_PATH=/usr/include/c++/11:/usr/include/x86_64-linux-gnu/c++/11:/usr/include/c++/11/backward
export LIBRARY_PATH=/usr/lib/gcc/x86_64-linux-gnu/11
cmake -S /tmp/s2-matrix-links.zCkO0O/src -B /tmp/s2-matrix-links.zCkO0O/build-clang-release \
  -G Ninja -DCMAKE_CXX_COMPILER=/usr/bin/clang++ -DCMAKE_BUILD_TYPE=Release \
  -DS2_GAME_DB_PATH=/tmp/s2-matrix-links.zCkO0O/game.db
cmake --build /tmp/s2-matrix-links.zCkO0O/build-clang-release -j 16
ctest --test-dir /tmp/s2-matrix-links.zCkO0O/build-clang-release --output-on-failure -j 8
```

The original Windows x64 `ADOImport`, `DBFormat`, `Main`, and `Game` targets
still build; all 89 Windows tests pass. On this lab's copied Visual Studio,
the full MSBuild command must inherit the SDK environment:

```cmd
call G:\SS\lab\tools\VS2022\VC\Auxiliary\Build\vcvars64.bat
G:\SS\lab\tools\VS2022\Common7\IDE\CommonExtensions\Microsoft\CMake\CMake\bin\cmake.exe --build G:\SS\lab\build-x64-stage2 --config Release --parallel 16 -- /p:UseEnv=true
```

No Linux `TerrainInfo` executable test or mission/gameplay claim follows from
this database test. Text conversion in original `Misc/StrProc.cpp` uses UTF-8
by default on Linux and supports explicit `SetCodePage(1251)` without OS
conversion modules; `NativeStrProcTests` checks both encodings on x86-64 and
ARM64. Other code pages fall back to `iconv`, whose modules may not exist in
the ARM64 test sysroot; code-page call sites in the remaining game runtime
still need auditing.

Clean Windows x64 native-media archive
`G:\SS\lab\builds\stage2-native-game-db-20260925-01` was produced from
source commit `3e044c2`. A new linked-resource LabRun
`G:\SS\lab\runs\stage2-native-game-db-clean-01` contains no `fmod.dll`,
loaded the existing `DB_OLD` save to `LOAD-SLOT-DONE`, accepted `quit`, exited
normally, and produced no crash dump. This checks that the shared source edits
did not break Windows mission loading; it is not Linux gameplay evidence.
