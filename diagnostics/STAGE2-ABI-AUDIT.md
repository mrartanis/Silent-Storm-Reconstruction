# Stage-2 ABI and file-contract audit (item 3, open)

Scope is the game-used CPU/source boundary in `STAGE2-SOURCE-BOUNDARY.md`
and the shipped-resource graph in `STAGE2-DATA-ROOTS.md`. This is a
category-by-category audit, not a claim that a text search or green CTest
alone establishes ABI portability. Visual output, renderer internals,
editor-only tools and arbitrary mods are outside this stage.

## 3a: game words and host `long` (first pass, 2026-09-28)

| Source/contract | Current evidence | Decision / next check |
|---|---|---|
| `STime` and `DWORD` | `Main/A5Time.h` aliases `STime` to `DWORD`. Windows SDK defines a 32-bit `DWORD`; Linux `Misc/Tools.h` explicitly defines `DWORD` as `std::uint32_t`. `NativeUnitDeathTimeWireTests` checks the actual member and 18-byte tag-35 wire vector against x86, Windows x64, Linux x86-64 and ARM64. | Width and that selected wire field are covered; not a blanket test of every timestamp arithmetic expression. |
| `CUnitArea::places` | `Main/aiActionPlaceSource.h` uses a 32-bit key; `NativeUnitAreaWireTests` checks a 26-byte vector on the same four architectures. See `NATIVE-GAME-WORD-WIRE.md`. | Previously found LP64 format mismatch is repaired. |
| `CShortPtrAllocator::pointer` | `Main/Cache.h` uses `DWORD` for an index, not a host pointer. | Its width follows the explicit `DWORD` contract; review allocator serialization under item 3c. |
| `wCheckTooMuchCorpses.cpp` | Local `unsigned long` copies of `GetDeathTime()` are 64-bit on LP64, but no caller of `CheckTooMuchCorpses` exists in `Main/`; the source itself labels the failsafe unwired. | Not a live stage-2 world path. Reopen if it is connected later. |
| `Script/lmem.cpp` | `unsigned long` memory-debug counters/header appear inside `_DEBUG`; normal game builds use the other branch. | Not a release-v1 game wire field. Debug configuration remains a separate diagnostic concern. |
| `Script/lundump.cpp` | `(long)` is used only to validate binary Lua chunk headers, which also encode host `sizeof(size_t)`. Base-game script roots are source text in `game.db` or four loose `.l` files, not binary Lua chunks. | Do not use binary-chunk portability as evidence for the original source-script path; assess only if the base loader can reach such a chunk. |
| `Script/lobject.h::ObjectHash` | The live numeric-key hash had a real LP64 difference: the original `Number → signed long → int` returned `80000000` on Windows for both `-2147483649` and `4294967295`, but `7FFFFFFF`/`FFFFFFFF` on Linux. `NumberWord` now explicitly truncates toward zero within signed 32-bit range and returns the MSVC integer-indefinite word `80000000` outside it. The local hash word is explicitly `std::uint32_t`; this is hash compression, not pointer identity storage. | **Repaired and address-tested.** See the focused Lua test and x86 oracle below. |
| `Main/scriptUI.cpp`, `Main/iCommonUI.cpp`, `Main/GParticles.*` | Additional `unsigned long` locals occur in UI or graphical particle paths. | Client/rendering-stage review; not evidence of a live headless-world ABI defect. Preserve existing code. |

Pointer-width scan in the same first pass found `FileIO/BasicChunk1.cpp`
using `std::uintptr_t` for buffer-address arithmetic, `Script/lsaver.cpp`
using `uintptr_t` for in-process function lookup and serializing registered
function IDs instead of addresses, and `Main/RPGUnit.cpp` folding the full
`uintptr_t` head seed through its explicit 32-bit helper. A raw
`(int)pMaterial.GetPtr()` in `Main/GSceneInternal.h` is inside a commented
hash class, not an active cast. This is a candidate classification, not
the completed pointer/structure/file audit of 3b–3d.

The visible x86 assembly in `Misc/Tools.h` and `Main/Bound.h` is guarded
by `_M_IX86` and has C++ alternatives for x64/ARM64. Assembly in
`2DSceneSW`, `GCombiner`, `GfxBuffers` and `SWTexture` is renderer/client
work outside this headless-world gate. The final source-list audit still
has to ensure no other reachable unguarded assembly remains.

### Reproduce this bounded source pass

From the reconstruction repository root:

```text
rg -n '\b(unsigned )?long\b|\bDWORD\b|\bSTime\b' Main FileIO Script Misc -g '*.cpp' -g '*.h'
rg -n 'uintptr_t|intptr_t|\(int\).*GetPtr|\b(__asm|_asm)\b' Main FileIO Script Misc -g '*.cpp' -g '*.h'
rg -n 'CheckTooMuchCorpses\(' Main -g '*.cpp' -g '*.h'
```

`NativeMissionScriptCorpusTests` checks the parsed constant pool of all
113 original `game.db` scripts: 18 numeric constants and zero integral
constants outside signed 32-bit range. The four loose startup scripts
have no 10+-digit decimal literal. This is a pinned property of the
base data, not a whole-program range proof: short literals use
`OP_PUSHINT`, scripts can compute values, and user/mod scripts are outside
this gate.

`NativeLuaRuntimeTests` exercises keys `2147483647`, `2147483648`,
`-2147483648`, `-2147483649` and `4294967295`. The five hash words are
`7FFFFFFF,80000000,80000000,80000000,80000000` on diagnostic Windows
x86, target Windows x64, Linux GCC x86-64 and ARM64/QEMU. On Windows,
the test separately executes the original volatile `Number → long`
expression and requires each result to match the explicit implementation.
Four additional fractional boundary/ordinary numbers check truncation
toward zero against that same x86/x64 expression.
It also creates a real Lua table, reads all five keys through a weighted
signature `1514131211`, saves the `Script` through `CStructureSaver`,
reloads it and recomputes the same signature. The serialized length is
20,365 bytes on all four architectures. Linux x86-64 passed under
ASan/UBSan/LSan and ARM64/QEMU under ASan/UBSan with LSan disabled.
The new check is an ABI regression, not a claim that any shipped script
actually uses those large keys.

Reproduce the focused test after building `NativeLuaRuntimeTests` from
this source on each target:

```text
<Windows-x64-or-diagnostic-x86-build>/RelWithDebInfo/NativeLuaRuntimeTests.exe
ASAN_OPTIONS=detect_leaks=1 UBSAN_OPTIONS=halt_on_error=1 <Linux-GCC-x64-build>/NativeLuaRuntimeTests
ASAN_OPTIONS=detect_leaks=0 UBSAN_OPTIONS=halt_on_error=1 qemu-aarch64-static -L /usr/aarch64-linux-gnu <Linux-GCC-arm64-build>/NativeLuaRuntimeTests
```

The Windows x86 diagnostic was a fresh `-A Win32` CMake build under
`G:\SS\lab\build-x86-lua-hash`; it is not a resumed x86 product build.

The FNV digest of the entire saved `Script` state differs across machines
and even between repeated Windows x64 process launches, despite identical
length and successful semantic round-trip. Thus this test must **not**
advertise deterministic whole-file bytes or cross-save compatibility;
the latter is not a product gate. The difference is not attributed to
one field without a byte-level investigation.

The live host-`long` numeric hash candidate of 3a is resolved; the other
listed uses have explicit fixed-width types or are outside this stage's
live release path. Next bounded action is item 3b (pointer/asm/alignment),
3c (wire/layout), and 3d (path/case/encoding) in that order. A claim of
item-3 completion needs the entire category report plus the final matrix,
not this first pass.
