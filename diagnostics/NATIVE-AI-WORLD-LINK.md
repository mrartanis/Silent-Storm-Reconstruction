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
