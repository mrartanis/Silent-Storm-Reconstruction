# Portable address seed for game-generated heads (stage 2)

`NRPG::CUnit::SetHead` seeds a newly generated `CHeadInfo` from the unit's
address. The former implementation folded the upper address word only under
`_M_X64`: Windows x64 did so, but Linux x86-64 and ARM64 silently used only
the lower 32 bits. `Main/HeadSeed.h` now folds a widened 64-bit value on every
host. A 32-bit address keeps the original low-word seed; on a 64-bit host the
seed is `low32 ^ high32`, matching the existing Windows x64 calculation.

`NativeHeadSeedTests` checks both cases with synthetic addresses. It is
registered in the Windows and portable CMake builds and requires no licensed
game resources. The check is deliberately about the game's seed arithmetic,
not exact generated-face equality across processes: address-space layout can
still make the seed differ between runs and operating systems. It also does
not test the separate editor-only FaceGen API or GPU rendering.

From an existing configured Windows build in a VS x64 developer shell:

```powershell
cmake --build G:\SS\lab\build-x64-stage2 --config RelWithDebInfo --target Game NativeHeadSeedTests --parallel 12
ctest --test-dir G:\SS\lab\build-x64-stage2 -C RelWithDebInfo -R '^NativeHeadSeedTests$' --output-on-failure
```

```sh
cmake --build build-x64 --parallel 16
ctest --test-dir build-x64 -R '^NativeHeadSeedTests$' --output-on-failure
cmake --build build-arm64 --parallel 16
ASAN_OPTIONS=detect_leaks=0 UBSAN_OPTIONS=print_stacktrace=1:halt_on_error=1 \
  ctest --test-dir build-arm64 \
  -R '^NativeHeadSeedTests$' --output-on-failure
```

These Linux commands run from the parent of the already configured build
directories; the ARM64 test needs QEMU user-mode execution configured in
CTest. `detect_leaks=0` is only for the ARM64/QEMU combination, not native
x86-64 sanitizer runs.

Validation on 2026-09-27: Windows x64 built `Game.exe` and passed 141/141
CTest tests; Linux x86-64 passed 118/118 under ASan/UBSan with LeakSanitizer;
Linux ARM64 under QEMU passed 118/118 under ASan/UBSan with leak detection
disabled only for QEMU. After changing the test to use explicit failures
instead of release-disabled `assert`, `NativeHeadSeedTests` was rebuilt and
rerun successfully on all three targets. No visual face or live-game parity
is claimed by this arithmetic test.
