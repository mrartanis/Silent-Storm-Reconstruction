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
