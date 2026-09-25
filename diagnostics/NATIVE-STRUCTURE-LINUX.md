# Native object-graph serializer on Linux

Stage 2 now builds the game's original `FileIO/BasicChunk1.cpp` and
`FileIO/Cruncher.cpp` on Linux x86-64 and ARM64/QEMU, linked to its actual
`Streams.cpp` and `Misc/Basic2.cpp` object ownership code. The same sources
remain in Windows `FileIO`. This includes the `CStructureSaver` object
registry, tagged chunks, 32-bit wire IDs, and the release LZ/bitstream
compressor. `Script/` now links to this serializer on Linux; its temporary
throwing save stub was removed.

`StructureWireWrite`/`StructureWireRead` create two owned objects and an
alias, write the graph, then restore values `42`, `77`, and identity. The
initial 80-byte object-only output was byte-identical on Windows x64, Linux
x86-64, and ARM64/QEMU (SHA-256
`9f58b2abbaca33460edaea81268c3b616f57a49a34e10895a1662942487e1691`).
The Linux and ARM64 readers also accepted the Windows-generated file.

Geometry codecs are now included in the actual Linux `CStructureSaver`, not
just stand-alone wire helpers. The graph probe additionally stores `CVec3`,
`CQuat`, `SPlane`, and `SHMatrix` fields in each object and verifies their
restored values. The extended files from Windows x64, Linux x86-64, and ARM64/QEMU are
byte-identical (SHA-256
`ec8c86ed563e644be55b1336534fa3f3a7487ef59e33ff27d7d79e54ca170358`),
and all three readers accepted cross-architecture files, including Windows
to Linux/ARM64 and ARM64 to Windows.
`NativeLuaRuntimeTests` now saves a `Script` instance containing global
`wrapper_result=39`, restores it, executes another chunk, and reaches `42`.
`NativeCruncherTests` checks compressed and stored round-trips for a
deterministic block and the original 185,840-byte `Fonts.res` on all three
architectures. Linux tests run under ASan/UBSan (under QEMU with
`ASAN_OPTIONS=detect_leaks=0`).

ASan exposed overlapping `memcpy` in the serializer's in-place nested-chunk
writer. It now uses `memmove`, and internal source offsets are saved before
`CMemoryStream` may reallocate. `CMemoryStream::SetSize` also grows the
allocation before forming the new end pointer. GCC-specific template and
class-registration macro fixes are shared with Windows.

Full CTest with the geometry changes: Windows x64 85/85, Linux x86-64 GCC
55/55, Linux ARM64 GCC/QEMU 55/55 (`ASAN_OPTIONS=detect_leaks=0`, serial
QEMU tests), and Linux x86-64 Clang 14 release 55/55. The Clang probe has
the same byte-identical geometry/object-graph output as the other three.
Clang 14 needed explicit paths to this host's GCC 11 C++ headers and library;
its default detection selected incomplete GCC 12. A separate Clang 14
ASan/UBSan build compiled, but test execution showed intermittent sanitizer
segfaults across unrelated tests, so it is not counted as passing; GCC's
ASan/UBSan runs remain the memory/UB gate.
The Windows `Game.exe` also rebuilt. Reproduce targeted tests with:

```sh
cmake --build BUILD_DIR --target StructureWireProbe NativeLuaRuntimeTests NativeCruncherTests -j 16
ASAN_OPTIONS=detect_leaks=0 ctest --test-dir BUILD_DIR -R '^(StructureWire|NativeLuaRuntimeTests|NativeCruncherTests)' --output-on-failure
ASAN_OPTIONS=detect_leaks=0 BUILD_DIR/NativeCruncherTests /path/to/Fonts.res
```

On this Linux host, Clang 14 requires its installed GCC 11 standard-library
paths (the autodetected GCC 12 installation is incomplete):

```sh
export CPLUS_INCLUDE_PATH=/usr/include/c++/11:/usr/include/x86_64-linux-gnu/c++/11
export LIBRARY_PATH=/usr/lib/gcc/x86_64-linux-gnu/11
cmake -S SOURCE_DIR -B CLANG_BUILD_DIR -G Ninja -DCMAKE_BUILD_TYPE=RelWithDebInfo -DCMAKE_C_COMPILER=clang -DCMAKE_CXX_COMPILER=clang++ -DS2_SCRIPT_CORPUS_DIR=LUA_SCRIPTS_DIR
cmake --build CLANG_BUILD_DIR -j 12
ctest --test-dir CLANG_BUILD_DIR --output-on-failure -j 12
```

On this Windows host, the Visual Studio developer environment plus
`MSBuild.exe ... /p:UseEnv=true` avoids the generated SDK include path's empty
segments; the full Release build and CTest were run in
`D:\SS-lab\build-x64-structure-01` because G: was full.

This is not complete Linux game save/load. The Linux target now compiles the
game's `Misc/Geom.h` types and geometry field codecs. The three legacy
anonymous aggregates containing constructed vectors use plain numeric/vector
members on non-Windows compilers; their wire layout and values are tested.
Most `Main/` object classes and Lua game bindings still have no Linux target, and
no Linux mission or game loop is claimed. The archive and a full Windows
mission load were checked separately:

Clean native-media x64 archive
`D:\SS-lab\builds\stage2-native-structure-20260925-01` came from commit
`1465ccc`. Isolated LabRun `D:\SS-lab\runs\stage2-native-structure-01`
loaded `TOPWRITE_NEW` to `LOAD-SLOT-DONE`, exited normally, and produced no
crash dump. Its copied `game.sav` retained SHA-256
`1385447ae22f6da374f44453bbb93034d99e2e7476e5faa9034d026b4ee3e16a`.
This checks the Windows game after shared serializer fixes; it does not
claim a Linux mission load.
