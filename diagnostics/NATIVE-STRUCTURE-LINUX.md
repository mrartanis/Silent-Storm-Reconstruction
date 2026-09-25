# Native object-graph serializer on Linux

Stage 2 now builds the game's original `FileIO/BasicChunk1.cpp` and
`FileIO/Cruncher.cpp` on Linux x86-64 and ARM64/QEMU, linked to its actual
`Streams.cpp` and `Misc/Basic2.cpp` object ownership code. The same sources
remain in Windows `FileIO`. This includes the `CStructureSaver` object
registry, tagged chunks, 32-bit wire IDs, and the release LZ/bitstream
compressor. `Script/` now links to this serializer on Linux; its temporary
throwing save stub was removed.

`StructureWireWrite`/`StructureWireRead` create two owned objects and an
alias, write the graph, then restore values `42`, `77`, and identity. Their
80-byte output is byte-identical on Windows x64, Linux x86-64, and
ARM64/QEMU (SHA-256
`9f58b2abbaca33460edaea81268c3b616f57a49a34e10895a1662942487e1691`).
The Linux and ARM64 readers also accepted the Windows-generated file.
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

Full CTest: Windows x64 85/85, Linux x86-64 55/55, ARM64/QEMU 55/55.
The Windows `Game.exe` also rebuilt. Reproduce targeted tests with:

```sh
cmake --build BUILD_DIR --target StructureWireProbe NativeLuaRuntimeTests NativeCruncherTests -j 16
ASAN_OPTIONS=detect_leaks=0 ctest --test-dir BUILD_DIR -R '^(StructureWire|NativeLuaRuntimeTests|NativeCruncherTests)' --output-on-failure
ASAN_OPTIONS=detect_leaks=0 BUILD_DIR/NativeCruncherTests /path/to/Fonts.res
```

This is not complete Linux game save/load. The Linux target does not yet
compile the game's `Misc/Geom.h` types, so geometry field codecs in
`GeometryWire.h` are temporarily excluded there; Windows keeps them. Most
`Main/` object classes and Lua game bindings still have no Linux target, and
no Linux mission or game loop is claimed. The archive and a full Windows
mission load remain separate regression gates.
