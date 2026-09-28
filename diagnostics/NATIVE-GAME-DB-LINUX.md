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

On 2026-09-27 `NativeGameDatabaseLoadTests` was strengthened to compare the
ID of every decoded source row against the original typed runtime table, not
only table sizes. The original `game.db` passed for all 239,310 records in
all 155 tables on Windows x64, GCC Linux x86-64 and ARM64/QEMU, and Clang
Linux x86-64. The full Windows suite passed 144/144 and GCC Linux x86-64
passed 120/120 under ASan/UBSan/LSan. This catches lost or
mis-keyed records even when table counts match. The 69 groups of unresolved
references remain: this test does not establish that their field-to-target
table mapping matches the Steam executable or that every missing reference
is harmless to gameplay.

The original database's UTF-16 strings are not all ASCII. An optional
`NativeGameDatabaseLoadTests <game.db> --nonascii` audit reports counts and
first code points without printing licensed text. It found Cyrillic in two
`Scripts.CodeText` rows and non-ASCII characters in two `UIControls.IDText`
rows. Previously the columnar `std::string` import truncated each wide
character to one byte. The game-used narrow-string importer now explicitly
encodes Windows-1251, matching the Windows host's ACP=1251 for this Russian
Steam data while leaving the process-wide conversion setting unchanged.
`NativeStrProcTests` checks explicit CP1251 (including U+2018 → 0x91) even
when the default setting is UTF-8. The database load test additionally checks
that original UI control 2656 imports that CP1251 byte in its `IDText`.
The 113-script corpus still parses;
its byte digest changed from `B5163E4E76664106` (truncated characters) to
`C2462A66D562BAF6` on both Windows and Linux x86-64. This is a tested
encoding choice for this original database, not a claim about every regional
edition. After the change, Windows x64 built `Game.exe` and passed 145/145
CTest; GCC Linux x86-64 passed 120/120 with ASan/UBSan/LSan; the focused
Clang script/world checks also passed. GCC Linux ARM64/QEMU passed 120/120
under ASan/UBSan with leak detection disabled; the full run took about
854 seconds. The UI-byte assertion was added after this full run began;
the updated ARM64 diagnostic passed separately in 101 seconds.

A clean native-media Windows x64 archive from source commit `ee3ebe5` is at
`G:\SS\lab\builds\stage2-db-cp1251-20260927-02`. Its `Game.exe` SHA-256 is
`D5DDB0D590E96503EFCDEE030A1A8D9D0C4869BD842F096666D0022ECFFB7116`;
`fmod.dll` is absent. The build used `Build-Lab.ps1 -NativeMedia -BuildJobs 16`
with the configured FFmpeg/miniaudio roots and VS `vcvars64.bat` environment;
run the script with PowerShell 7 (`pwsh`) in that environment so the final
`Get-FileHash` step is available. The first attempt using Windows PowerShell
left an incomplete `...-01` directory and is not an archive gate.
`New-LabRun.ps1 -SkipIntro -LinkResources` staged the `...-02` archive as
`G:\SS\lab\runs\stage2-db-cp1251-smoke-20260927-01`. The live process
opened a responsive `Silent Storm` window, logged `DB-STORAGE: loaded 155
columnar tables via portable v1` and `NATIVE-MUSIC playing:
Res\Music\Mainmenu.wav`, and created no crash dump. `CloseMainWindow` then
closed both Game and debugger. The screen was not visually inspected and no
mission was played in this smoke run.

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

Stage-2 follow-up: the original game-used `Textures.AverageColor`,
`AmbientLights` color fields, and `UIControls.Type` were imported by passing
`DWORD*` or enum pointers as `int*`. The importer writes synchronously, so
these paths now read into a real `int` and explicitly convert to the stored
type. This removes a strict-aliasing assumption without changing the signed
32-bit database word, the color-channel order, or the UI enum values.
`NativeGameDatabaseLoadTests` compares the loaded values against the source
columns for all 5,801 texture colors, 1,917 UI types, and 1,056 ambient-light
color values (including the ambient-subtracted direct-light value). The
checks passed on Windows x64, Linux GCC x86-64 with ASan/UBSan/LSan, Linux
GCC ARM64/QEMU with ASan/UBSan, and Linux Clang release. These are typed
import checks for the shipped `game.db`, not visual-lighting or UI-rendering
parity tests.
After relinking the headless world probe against the changed database
library, mission root 5247 also completed scripted start, save/load, hero
turn, end-turn transfer, and a second save/load on Linux GCC x86-64 and
ARM64/QEMU. This checks one mission continuation with the updated importer;
it is not a Steam behavioral comparison or a Linux graphical-game launch.

Clean Windows x64 native-media archive
`G:\SS\lab\builds\stage2-native-game-db-20260925-01` was produced from
source commit `3e044c2`. A new linked-resource LabRun
`G:\SS\lab\runs\stage2-native-game-db-clean-01` contains no `fmod.dll`,
loaded the existing `DB_OLD` save to `LOAD-SLOT-DONE`, accepted `quit`, exited
normally, and produced no crash dump. This checks that the shared source edits
did not break Windows mission loading; it is not Linux gameplay evidence.
