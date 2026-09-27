# Linux Clang headless world gate (stage 2)

Clang 14 on `artanis.c.ibgene.org` was used as an additional compiler check
for the game-used headless `CWorld` path. This is not a Linux graphical
`Game.exe`, a macOS check, or a substitute for the GCC x86-64/ARM64 matrix.

The host has GCC 11's complete C++ headers and runtime, but its default
Clang search selects an incomplete GCC 12 installation. Use the explicit
GCC 11 paths shown below. The Clang ASan binary also intermittently crashed
before `main` when built as PIE (5/20 minimal test starts). `-fno-pie` and
`-no-pie` made that startup test pass 20/20; this only controls the diagnostic
executable, not the game's release build.

```sh
CC=clang CXX=clang++ cmake -S src -B build-clang-x64 -G Ninja \
  -DCMAKE_BUILD_TYPE=Release \
  -DCMAKE_CXX_FLAGS='-nostdinc++ -isystem /usr/include/c++/11 -isystem /usr/include/x86_64-linux-gnu/c++/11 -isystem /usr/include/c++/11/backward -L/usr/lib/gcc/x86_64-linux-gnu/11 -fsanitize=address,undefined -fno-pie' \
  -DCMAKE_EXE_LINKER_FLAGS='-fsanitize=address,undefined -no-pie' \
  -DS2_GAME_DB_PATH=/path/to/game.db \
  -DS2_RESOURCE_PACKAGE_PATH=/path/to/Waypoints.res
cmake --build build-clang-x64 --target NativeWorldInitProbe NativeAttackRulesTests --parallel 16
ASAN_OPTIONS=detect_leaks=1 UBSAN_OPTIONS=print_stacktrace=1:halt_on_error=1 \
  ctest --test-dir build-clang-x64 -R '^(NativeWorld|NativeAttackRulesTests)' \
  --output-on-failure -j 4
```

The compile exposed enum forward declarations without a fixed underlying
type, despite game-used definitions with `: int`. The declarations now match
the definitions; the serialized values and ABI stay `int`. It also exposed
the database registration macro calling `CObjectBase*` constructors through
an incompatible `CDBRecord* (*)()` function pointer. Captureless adapters
now return `CDBRecord*` with a valid `static_cast`, leaving the created
record class and table ID unchanged.

Clang UBSan then found an out-of-range float-to-int conversion in projectile
penetration. Explosion fragments can meet a material interval ending at the
ray's `1e10` sentinel; with density 1000, the APA cost is about `1e13`, far
outside `int`. `ApplyAPASubstraction` stops such a projectile before conversion
and preserves the previous arithmetic for finite costs below kinetic energy.
It also treats a reversed interval as zero material traversal. The same
helper now guards all four `nK` deductions in `RPGGame.cpp` and
`RPGBullet.cpp`. `NativeAttackRulesTests` covers normal, reversed and
sentinel-size intervals without licensed resources.

The separate whole-build `NativeHeightNetworkTests` link still fails under
Clang because its diagnostic target does not link the full DB/world graph.
The `NativeWorldInitProbe` target and its CTest scenarios are the claimed
Clang gate; no claim is made that every portable diagnostic target links
under Clang yet. Nor do these headless checks prove Steam-equivalent visual
effects or complete dynamic battle parity.

Validation on 2026-09-27: Windows x64 built `Game.exe` and passed 141/141
CTest tests. GCC Linux x86-64 passed 118/118 under ASan/UBSan with
LeakSanitizer. Clang Linux x86-64 passed all 14 `NativeWorld*` tests under
ASan/UBSan with LeakSanitizer in its non-PIE configuration; the focused
`NativeAttackRulesTests` also passed there. GCC Linux ARM64/QEMU passed
118/118 under ASan/UBSan with leak detection disabled for QEMU; the full
ARM64 run took about 393 seconds. These checks do not assert identical
fragment scatter or damage against the Steam executable.
