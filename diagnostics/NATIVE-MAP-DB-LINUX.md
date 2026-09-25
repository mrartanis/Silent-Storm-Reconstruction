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
