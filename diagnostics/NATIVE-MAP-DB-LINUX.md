# Original map-template database regression (stage 2)

`NativeMapDatabaseTests` loads an original `game.db` through the game's
`NDatabase::Serialize` and typed `DBFormat` classes. It then walks **all**
`CTemplVariant` records used by `MapBuild.cpp`, checking their `CTemplate`
back-references and hashing the game-facing template dimensions, borders,
cut-floor limits, weather, script ID and sorted IDs of placed rectangles,
final elements, units, explosions, terrain spots and waypoints. This is a
semantic digest: it does not hash pointers, container allocation order or
32/64-bit object layouts. A null template/placed-object link or broken
template-to-variant membership fails the test.

The baseline `G:/SS/lab/baseline/game.db` gives:

```
templates=2666 variants=4726 populated=4150 rectangles=19684 zero_size=3 digest=B485B25F228A7D08
```

Windows x86 (supplemental restored-source comparator), Windows x64,
Linux GCC x64 with ASan/UBSan, Linux GCC ARM64 under QEMU, and Linux Clang
x64 all produced this exact line on 2026-09-25. Three zero-size templates
are present in the original database, so the test counts rather than rejects
them. This test checks typed *data and relationships*; it does not yet call
`BuildMap`, resolve a complete mission's terrain/route network, or prove
Linux gameplay. Those remain stage-2 work.

Build and run on Windows with the game's CMake target:

```
cmake --build <windows-build> --target NativeMapDatabaseTests --config RelWithDebInfo --parallel 16
ctest --test-dir <windows-build> -C RelWithDebInfo -R '^NativeMapDatabaseTests$' --output-on-failure
```

The Windows CTest is registered when `S2_GAME_DIR/game.db` exists. On Linux,
configure the portable target with `-DS2_GAME_DB_PATH=<original-game.db>`:

```
cmake --build <linux-build> --target NativeMapDatabaseTests -j 12
ctest --test-dir <linux-build> -R '^NativeMapDatabaseTests$' --output-on-failure
```

For cross-ARM64 execution on an x64 Linux host, invoke the target with
`qemu-aarch64-static -L /usr/aarch64-linux-gnu` and the same database path;
use `ASAN_OPTIONS=detect_leaks=0` if the ARM target is sanitizer-instrumented.

On 2026-09-27 the test gained a scenario-root graph check. All five
`GlobalMaps` select three scenarios; all 53 `ScenarioZones` belong to those
scenarios. Their nonzero `TemplateID1..3` fields name 52 distinct root
templates/variants. Following every variant's nested rectangle templates
gives 526 templates and 985 variants, with no missing root or child link;
the sorted-variant FNV digest is `C568A7F7EB837585`. The check runs on
Windows x64, GCC Linux x86-64 and ARM64/QEMU, and Clang Linux x86-64 with
the same counts and digest. Use `NativeMapDatabaseTests <game.db> --roots`
to print root and closure variant IDs. In particular, variants 3829 and
4526 are direct authored roots; 218, 810 and 2223 are outside this
ScenarioZone graph and may belong to other game modes. The closure is an
upper bound because it includes all variants regardless of runtime flags,
and it does not prove that every listed variant is selected in play or that
all game modes originate from `ScenarioZones`.
The expanded Windows x64 suite passed 146/146 and GCC Linux x86-64 passed
121/121 under ASan/UBSan/LSan. On ARM64/QEMU, both new graph and
`NativeWorldMission3829` cases passed under ASan/UBSan with leak detection
off; the unchanged full 120-case suite had passed immediately before this
test-only expansion. The Clang x86-64 graph and mission cases passed under
ASan/UBSan/LSan as focused checks.

`NativeScenarioRootMaps` is an extended CTest regression for the entire
authored scenario-root set. The portable CMake driver
`diagnostics/RunScenarioRootMaps.cmake` obtains the 52 IDs from
`NativeMapDatabaseTests <game.db> --roots`, then launches a fresh
`NativeMissionMapProbe` process for each root. Every process invokes the
original `BuildMap` with seed 123, parses all selected mission scripts with
the original Lua parser, and must report `built=1`. The probe also freezes
the game's auxiliary clock-seeded RNGs only in this paired diagnostic mode.
The driver hashes each
map's geometry/object summary, route summary, and behavior/script digest
into one platform-comparable SHA-256 fingerprint. A fresh process avoids
carrying map-builder state from one mission into the next. This tests map
construction and script syntax, not `CWorld` post-init, Lua effects, player
actions, or Steam behavioral parity. It does not claim that all 985 variants
in the transitive graph are selected in a campaign.

Run it after building both probe targets, with the original `game.db` and
`.res` paths configured as for the other mission probes:

```
ctest --test-dir <windows-build> -C RelWithDebInfo -R '^NativeScenarioRootMaps$' --output-on-failure
ASAN_OPTIONS=detect_leaks=1 UBSAN_OPTIONS=print_stacktrace=1:halt_on_error=1 \
  ctest --test-dir <linux-x64-build> -R '^NativeScenarioRootMaps$' --output-on-failure
ASAN_OPTIONS=detect_leaks=0 UBSAN_OPTIONS=print_stacktrace=1:halt_on_error=1 \
  ctest --test-dir <linux-arm64-build> -R '^NativeScenarioRootMaps$' --output-on-failure
```

The first Windows x64 and GCC Linux x86-64 full runs passed 52/52 roots,
but comparing fingerprints exposed a real Linux nondeterminism in map 3830.
Item-level tracing found three wall/solid objects with corrupted coordinates.
`CMapBuilder::AddBuildingObjects` constructed `ptShift` without initializing
it, then added it to every fragment position even when no rotation-specific
shift applied. The corrected neutral shift is `(0,0,0)`; it is used for wall
fragments and unrotated solids. The game path is changed by this fix, not
just the test. Five repeated Linux x64 map-3830 runs had varied before the
fix; three repeated post-fix runs gave the same geometry digest
`2DD3DDC11A704B89`, matching Windows x64 and ARM64/QEMU under ASan/UBSan.
Map 3840 also matches on all three architectures. The previous fixed
Windows map-4526 checksum was updated from the former uninitialized-stack
result to `A65A2070B18C1067`.

The next full comparison isolated a second, probe-only mismatch in root
5716. Six group-route points have `bExists=0`; their position fields are
undefined until resolved. The probe now hashes their IDs, existence flags,
commands and defined values, not the uninitialized positions. Root 5716
then gave the same geometry and behavior digests on Windows x64, Linux x64
and ARM64/QEMU.

With both corrections (before the later item-coordinate normalization),
Windows x64 passed 148/148 CTests, including all
52 roots in 61.26 seconds. GCC Linux x86-64 passed the extended 52-root
test under ASan/UBSan/LSan in 494.30 seconds. Both produced the same
aggregate digest, now asserted by the test:
`2c281f9de3688e97011ccd069f4eeb2c1f74c7f04bb52df4de955d35286c2629`.
On ARM64/QEMU the focused map-3830 regression and the updated map-4526
regression passed under ASan/UBSan; maps 3840 and 5716 produced the same
geometry, route and behavior digests in direct probes. The ordinary ARM64
suite passed 122/122 under ASan/UBSan with `-LE extended` (leak detection
off under QEMU). The entire 52-root ARM64/QEMU extended test is not claimed
complete.

On 2026-09-29 the final ARM64/QEMU 52-root sweep was rerun after the
cross-architecture comparison exposed item-only floating-point noise in
roots 4517, 4518, 5003 and 5429. The probe now canonicalizes item tuples
before hashing (stable parent/floor/flags, x/y at centimetre precision and
z at map-unit precision); this preserves gameplay-relevant placement while
excluding sub-unit floor FPU noise and insertion order. The x64 baseline
was regenerated as
`c79a9c158d3f64204b1529ae576f4b74e59388dd7f9531ad725f2994dcf3894d`, and
ARM64/QEMU then completed all 52/52 roots under ASan/UBSan (LSan disabled)
with that exact digest.

The Linux Clang x86-64 `NativeMissionMapProbe` was rebuilt from the same
source revision and the complete `NativeScenarioRootMaps` test was rerun;
all 52/52 roots passed in 89.80 seconds with the same
`c79a9c158d3f64204b1529ae576f4b74e59388dd7f9531ad725f2994dcf3894d`
aggregate digest. An earlier differing result came from the stale probe
binary, not from an architecture mismatch.
