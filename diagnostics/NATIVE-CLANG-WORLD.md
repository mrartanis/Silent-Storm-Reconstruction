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
The same helper now guards all four `nK` deductions in `RPGGame.cpp` and
`RPGBullet.cpp`. `NativeAttackRulesTests` covers normal, reversed and
sentinel-size intervals without licensed resources.

Direct static comparison with the unchanged Steam `Game.exe` (SHA-256
`4F417593A9F73E2BFDE12D83CDBD694AE47EECB92812140D67B8B343E4A95705`)
found the `GetAPASubstraction` body at RVA `0x28F5A0`. The same opcode shape
is named by the RussianGold private PDB at RVA `0x289530`: it computes
`(max(exit, 0) - max(enter, 0)) * density`, including a negative result for
a reversed interval. The port therefore retains that float rule exactly;
only the four integer-energy deductions guard an out-of-range conversion.
This is direct evidence for the formula, not a live Steam grenade-damage
comparison. The safety result for an out-of-range cost is a port policy,
not a measured Steam behavior for that exceptional interval.

The RussianGold `Game.exe` SHA-256 is
`2F8AD658D1D8B33CDA06270E1BCFC05962B9008C1DE3D09BA26D3AA7C52D329A`.
To repeat the static comparison, query its matching private PDB with
`dbh -s:<RussianGold directory> <RussianGold Game.exe> "x *GetAPASubstraction*"`
(the reported symbol address uses a synthetic image base `0x01000000`).
Then disassemble Gold at image address `0x689530` and the Steam EXE at
`0x68F5A0` with a PE-aware `objdump -d --start-address=... --stop-address=...`.
Both bodies call the same clamp routine twice, subtract entry from exit,
and multiply by material density. The Steam EXE was copied to a scratch
directory for this read-only analysis; its installed files were not edited.

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
After retaining the Steam float formula, Windows x64 passed 141/141 again,
GCC Linux x86-64 passed 118/118, and the Clang headless-world plus attack
subset passed 15/15 under ASan/UBSan with LeakSanitizer. ARM64's repeat
matrix passed 118/118 under ASan/UBSan with leak detection disabled for
QEMU; it took about 400 seconds. This repeat covers the Steam-formula
correction, not just the earlier overflow guard.

The clean native-media Windows x64 archive from the corrected source commit
`4b2c61f` is `G:\SS\lab\builds\stage2-steam-apa-20260927-01`. Its
`Game.exe` SHA-256 is
`62563E865FB53C77950C619D2CB3B2D7153E18736A05891C90756BE8162E228C`;
`fmod.dll` is absent. This newer archive was not separately launched.

A clean native-media Windows x64 archive from commit `298dc40` is at
`G:\SS\lab\builds\stage2-clang-penetration-20260927-01`. Its `Game.exe`
SHA-256 is
`B68D56FB836CA27EBE634D55BF863C55F43366E4D4FB3FDEDD0A195E625789AE`;
`fmod.dll` is absent. A separate live graphical smoke test of this archive
was not performed.

An isolated smoke of that archive was subsequently run in
`G:\SS\lab\runs\stage2-clang-penetration-smoke-20260927-01` with linked
resources and `-SkipIntro`. The `Game.exe` process opened a responsive
`Silent Storm` window, and its debugger log reached
`NATIVE-MUSIC playing: Res\Music\Mainmenu.wav`; no crash dump was created.
The test process was then deliberately stopped. Both screen-copy and
computer-use captures showed the covering Codex window, not the DirectX
contents, so this is **not** visual confirmation of the menu or a game-play
test. The linked baseline resources matched their manifest hashes after the
run (2451/2451 files); the isolated user-data directory remained inside
the LabRun.
