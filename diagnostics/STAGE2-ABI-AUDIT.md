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
| `CShortPtrAllocator::pointer` | `Main/Cache.h` uses `DWORD` for an index, not a host pointer. All actual instantiations found are in `GfxBuffers`, `GLightmap` and `GTransparent`. | Render cache only; no reached world serialization to audit under 3c. |
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
work outside this headless-world gate. The final source-list finding is
recorded in 3b below.

## 3b: pointers, x86 scalar assembly and alignment (closed for the current source boundary)

The game-used `SVoxelObjectHash` previously narrowed an object pointer
through `int`. It now hashes the full `uintptr_t` and has a high-half
collision regression (`NATIVE-VOXEL-HASH-LINUX.md`). `Main/RPGUnit.cpp`
folds a full-width address explicitly into a 32-bit head seed, with
`NativeHeadSeedTests`; that fold is a game rule, not an accidental cast.
`Script/lsaver.cpp` keeps function pointers in `uintptr_t` maps while the
file contains registered string IDs, not addresses. `FileIO/BasicChunk1.cpp`
uses `uintptr_t` only to detect whether a write source aliases its own
buffer; it retains the size/bounds check before forming an offset.
The raw `(int)pMaterial.GetPtr()` in `GSceneInternal.h` is in a commented
class. `iMission.cpp`'s `CSound* → CObjectBase*` cast is in the Windows
client command layer, not in the portable world; its inheritance
assumption belongs to a later client audit.

The first typed-dereference scan found `Script/lstate.cpp`'s `*(int*)ud`:
its only caller passes `&stacksize` from `lua_open`, an aligned local
`int`. Byte-buffer casts in `PortableStructureChunks`,
`PortableGameDatabase`, and `Misc/StrProc` use `char`/`uint8_t` views,
not misaligned multi-byte loads. Raw float/int aliases in `GMatShare`,
`MemObject`, `GParticleInfo`, `GTransparent` and D3D buffer code belong
to deferred scene/renderer files per `STAGE2-SOURCE-BOUNDARY.md`; they
are **not** declared safe, merely outside this headless-core gate.

The assembly-backed `Misc/Tools.h` scalar `Min<float>`/`Max<float>`
proved a live portability difference. Diagnostic Windows x86 selects
the *first* input's bits for equal operands (`-0/+0` included), while
the old x64 fallback of `Max<float>` selected the second; x86 x87
`fcomp` also selects the second operand on an unordered comparison,
while the old `Min<float>` fallback selected the first. The C++ branch
now reproduces the x86 selection. `PortableFloat2IntTests` pins five
ordered input pairs, including both zero orders, both NaN positions and
ordinary `2/1`, by exact IEEE-754 bits. All five pass on diagnostic
Windows x86, Windows x64, Linux GCC x86-64 (ASan/UBSan/LSan) and
ARM64/QEMU (ASan/UBSan, LSan off). Its pre-existing conversion tests
also cover current rounding modes and edge/NaN behavior; no naked x86
assembly is required on x64/ARM64. The remaining `select_*` assembly
helpers and `MemSetDWord` have no game caller found by source search;
MMX-bound helpers are called by renderer/particle files only.

The final source-list pass found no other unguarded x86 assembly in the
portable game targets: the remaining hits are commented helpers,
`2DSceneSW`, `SWTexture`, `GfxBuffers`, `GCombiner`, `GSceneParticles`,
and `Bound` render/particle paths. `GetCPUID`, `Sign` and `Float2Int`
are architecture-guarded. The `luaMakeCallParamsVector` format `f`
branch has a pre-existing invalid `va_arg(..., float)` (varargs promote
to `double`), but every game call found uses only `i`, `p`, `s` or an
empty format; it is recorded as latent, not claimed as a reached game
failure. A future game use of `f` must repair and test that branch.

This is a scoped source/call-edge audit and address test, not the final
ABI matrix. Reproduce the new scalar check by building and running
`PortableFloat2IntTests` on each target; its `float_minmax` lines show
both input and result words. Next is item 3c (wire/layout), followed by
3d (paths/encoding).

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
live release path. The subsequent 3b pass is recorded above. A claim of
item-3 completion needs 3c/3d and the final matrix, not this first pass.

## 3c: serialized structures and direct file streams (in progress)

The primary game input and save route is `CStructureSaver`. Its typed
`StructureFieldCodec` paths encode little-endian scalars and the reached
game structs explicitly; the fallback raw scalar/array paths report
`RAW-TYPE` only when reached under `g_bWireAudit`. On the complete resource
mirror, Linux Clang x86-64 under ASan/UBSan/LSan completed strict party
save/load/continued-tick audits for all 52 active scenario roots with zero
raw types and no unexplained wire findings (`RAW-TYPE-AUDIT.md`). This is
strong coverage of scenario initialization, not an exhaustive enumeration
of every dynamic save state. Selected older GUI saves also reached zero
raw types. No source caller of `CStructureSaver::AddRawData` exists in the
current game tree; this does not prove every direct stream is typed.

Direct-stream inventory, classified by actual call edge:

| Entry point | Game reachability and present contract | 3c action |
|---|---|---|
| `FilesPackage::Open` | Game resource loader. It uses `PortablePackageIndex` to parse the 8-byte signature/offset and index words explicitly, then opens the resolved package for resource payloads. All 23 shipped package families have read coverage in `STAGE2-DATA-ROOTS.md`. | Retain package-index regression; do not infer endian safety from the legacy `SFileInfo` host struct. |
| `FilesPackage::Update` / `RescanDir` | Windows-only package writer called by tooling, not the game resource read path. | Outside the game-only stage-2 gate. |
| `CStructureSaver` | `game.db`, resource object streams and save state. | Inspect the remaining non-codec/raw and special-object edges statically, and pair any reached gap with a byte-level test. |
| `CDataStream::operator<<` / `>>` and direct `Read` / `Write` | Generic host-memory operations exist, but call sites must be classified; template definition alone is not proof of a game wire field. `FilesPackage`'s 4-byte integer reads and `Cruncher`'s 1/4-byte operations are the relevant lower-level format edges. | Audit each reached direct-format caller's width/endian contract. |
| `bmpfile.cpp`, `iMain.cpp`, `GTexture.cpp`, `SWTexture.cpp` | Bitmap screenshot output or client-side image/texture loading. | Defer visual formats to the graphics/client stage. |
| `LSHead.cpp` animator-stream `Write` | Writes an in-memory animation stream, not a new host-struct file header. | Covered by the typed animation resource path; recheck if a direct disk edge appears. |

The first direct-stream fix removes native-`int` byte copies from the
`CDataStream` short/long string-length prefix and both
`CNetCompressor` 4-byte uncompressed-size prefixes. They now decode and
encode explicit little-endian words; the compressed reader rejects a
length above `INT_MAX` before allocating output. `NativeStreamsTests`
pins the exact one-byte and four-byte string prefixes for 5- and
300-byte strings; `NativeCruncherTests` pins the exact size prefix for
both packed and stored streams and rejects `0xffffffff`. Diagnostic
Windows x86 and target Windows x64 pass both tests; Linux GCC x86-64
passes with ASan/UBSan/LSan and ARM64/QEMU with ASan/UBSan (LSan off).
The full Windows x64 `Game.exe` also builds. These are byte-contract
checks, not a claim that all direct-file paths or game states are covered.

The bounded next pass is: (1) complete the direct-stream caller inventory;
(2) verify original package/database/save field widths and endian handling
against fixed byte vectors on Windows x64, Linux x86-64 and ARM64; (3)
record any uncovered dynamic save-state classes explicitly rather than
calling the 52-root audit exhaustive. Cross-loading x64 saves in the
Steam client is not a product gate.
