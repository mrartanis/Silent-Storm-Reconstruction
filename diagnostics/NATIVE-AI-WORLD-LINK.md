# Linux AI/world link boundary (stage 2)

The full-library diagnostic links the real `NativeAILogicTests` object
against every current Linux game archive with `--gc-sections`. It is a
dependency probe, not an executable mission or a substitute for gameplay
tests.

The original `aiMap.cpp` owns `NAI::shareAIModel` on Windows. A separate
Linux definition in `aiObjectLoader.cpp` was needed while only the
geometry loader linked. Once the Linux world pulled both translation
units, the linker reported a multiple definition (including ASan's ODR
metadata). `aiMap.cpp` now declares the Linux instance instead of
defining it; `aiObjectLoader.cpp` is its sole Linux owner. Windows keeps
the original definition next to the other map shares, preserving the
`CBasicShareBase` registration order used by `SerializeShared`.

After that correction the probe has no duplicate cache definition.
Its remaining unresolved symbols are the two casts for
`NGScene::CNonePart` and the object-base cast for
`NGScene::CLightGroup`. The archive map shows the real ownership:
`wDecal.cpp` pulls `GDecalTarget.cpp`, whose saved `parts` vector owns
`CNonePart` pointers; `wUnitServer.cpp` pulls `wDebris.cpp`, whose
`CDFrozenItem::Visit` reaches `CLightGroup` through `GView.h`.
`CNonePart` derives from `IPart` in the renderer combiner, while
`CLightGroup` retains a concrete `CGScene`. Neither is only a missing
registration macro. Do not manufacture cast symbols or dummy scene
objects to make this diagnostic green. The portable game-world boundary
still needs a genuine scene/render separation before a headless Linux
mission can be claimed.

A direct Linux syntax check of `GSceneInternal.h` succeeds, but compiling the
real `GCombiner.cpp` stops at `GfxBuffers.h` requiring `D3D9.h`. `IPart`'s
constructor and destructor call `CPerMaterialCombiner` methods; moving only
its class registration would not produce a working data type. This confirms
that the next step is a genuine CPU scene/combiner boundary, not an include
case fix or linker shim. The diagnostic used the unmodified source after the
probe; no experimental scene edit was retained.

Reproduce the probe on Linux from the configured build directory:

```sh
g++ -fsanitize=address,undefined -Wl,--gc-sections \
  CMakeFiles/NativeAILogicTests.dir/diagnostics/NativeAILogicTests.cpp.o \
  -o /tmp/s2-ai-link-probe -Wl,--start-group lib*.a -Wl,--end-group
```

The expected result for this stage is a failed link with only those
three distinct scene symbols, and no `shareAIModel` multiple definition.
After this change `Game.exe` built on Windows x64 and full CTest passed
123/123. Linux GCC x86-64 and ARM64/QEMU built all targets and passed
98/98 each under ASan/UBSan. These regressions include the original
mission-map builder but still do not execute a Linux AI/world turn.

After the CPU combiner extraction (`GCombinerCore.cpp`) and real
`CNonePart` registration (`GScenePartCore.cpp`), the same full-archive link
resolves both `CNonePart` casts without a shim. Its only remaining distinct
unresolved symbol is `CastToObjectBaseImpl<NGScene::CLightGroup>` from
`CDFrozenItem::Visit`. `CLightGroup` still owns a concrete `CGScene` and
calls `FreeLightGroup` on destruction; it must be separated without losing
that ownership rule. The earlier three-symbol output above is historical.

The next probe exposed an incomplete-type problem rather than a missing
implementation: `wDebris.cpp` included `GView.h`, which only forward-declared
`CLightGroup`. Its real definition now lives in `GSceneInternal.h`, and
`wDebris.cpp` includes that header. The registration remains in
`GSceneInternal.cpp`; no dummy cast or replacement scene was introduced.
`s2_game_world_entities` needs the existing Linux `main_case_include`
directory because `GScene.h` includes `DG.h` while the source file is
`DG.H`. A fresh-build probe can force the formerly unresolved caller:

```sh
g++ -fsanitize=address,undefined -Wl,--gc-sections \
  -Wl,-u,_ZN6NWorld12CDFrozenItem5VisitEPNS_14IRenderVisitorE \
  CMakeFiles/NativeScenePartCoreTests.dir/diagnostics/NativeScenePartCoreTests.cpp.o \
  -o /tmp/s2-scene-link-probe-x64 -Wl,--start-group lib*.a -Wl,--end-group
ASAN_OPTIONS=detect_leaks=0 /tmp/s2-scene-link-probe-x64
```

The x86-64 probe succeeds and prints the scene-part wire checksum. An
ARM64 link with `aarch64-linux-gnu-g++` and the same forced symbol also
succeeds; its QEMU/ASan probe passes. This is a linked-library boundary, not yet a
Linux mission executable or a complete renderer-free gameplay loop.

`NativeAILogicTests` is now an ordinary Linux CMake/CTest target on x86-64
and ARM64. It links the original `CAILogic` lifecycle against the portable
static-library graph and forces `CDFrozenItem::Visit(IRenderVisitor*)` into
that link, so the former `CLightGroup` break cannot silently disappear
through linker garbage collection. Run it with `ctest --test-dir <build>
-R '^NativeAILogicTests$' --output-on-failure` (and
`ASAN_OPTIONS=detect_leaks=0` under QEMU). The test covers pause, resume
and finish, not AI decisions, routing, Lua-driven mission state or a Linux
turn loop.
