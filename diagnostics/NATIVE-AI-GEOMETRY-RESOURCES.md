# Effective AI collision geometry resources (stage 2)

`NativeAIGeometryResourceTests` selects `CAIGeometry` IDs from the original
`game.db`, intersects them with the effective `AIGeometries` resource set,
and reads every selected ID through the game's `CResourceOpener` and
`CLoadGeometryInfo` loader. This is an actual map-collision input path
(`CAIMap::AddHull`, `GetGeometry`, and `GetSpheres`), not a renderer-only or
editor resource. Strict serialization reads the original top-level fields
and stored pieces; unlike `CLoadGeometryInfo::Recalc`, the test reports
exceptions rather than swallowing them. The semantic digest includes all
points, triangles, weights, spheres, junctions, collision-grid cells and
bounds, in sorted piece-ID order; it does not hash host C++ object layouts.

The current shipping package has 2043 IDs. The game's AI-geometry database
table lists 1982 of them. All 1974 loose `res/aigeometries` files override
those package IDs; the other eight database-listed IDs come from the package.
The 61 package-only IDs absent from that table are not forced through this
corpus test. A package-only mirror therefore tests a different
effective input set, even if its package index parses successfully.

Expected output for the original supplied data:

```
available=2043 database=1982 geometries=1982 loose_files=1974 overrides=1974 loose_only=0 points=124110 triangles=206210 spheres=4853 pieces=3972 precalc=14932 grid_cells=4224664 digest=E3F1B8241671D9EC
```

Windows x64, Linux GCC x86-64, Linux Clang x86-64 and ARM64/QEMU produced
these exact semantic values. A separately built Windows x86 diagnostic
produced the same digest; it is an additional data oracle, not a product
build. The Linux x86-64 runs used ASan/UBSan/LSan and the complete resource
mirror; ARM64/QEMU used ASan/UBSan with LSan disabled. The
original `AIGeometries.res` SHA-256 is
`A027A1923C728404D6F746F3E05831FDB6DA727D1DD7DD95CB632435A11238EB`.
Licensed data and test outputs remain outside Git.

Configure Windows with `S2_GAME_DIR` and Linux with `S2_GAME_DB_PATH` plus
`S2_RESOURCE_PACKAGE_PATH`, then run:

```
ctest --test-dir <windows-build> -C RelWithDebInfo -R '^NativeAIGeometryResourceTests$' --output-on-failure
ASAN_OPTIONS=detect_leaks=1 UBSAN_OPTIONS=halt_on_error=1 ctest --test-dir <linux-build> -R '^NativeAIGeometryResourceTests$' --output-on-failure
```

This covers deserialization and original geometry-loader construction for
all database-listed IDs. It does not establish that every listed ID is
reached in normal play, nor prove every collision query, dynamic damage
state, visual geometry or Steam-equivalent AI decision. Those require
separate mission and later behavior checks.

After this test was added, the non-`extended` CTest suite passed 157/157
on Windows x64 Release and 130/130 on Linux GCC x86-64 under
ASan/UBSan/LSan. The earlier 52-root strict world audit is a separate,
long-running gate; this resource test does not replace it.
