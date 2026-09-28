# Effective AI collision geometry resources (stage 2)

## Geometry used to construct AI hulls

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

## Precomputed open/closed door collision

`CAIMap::AddFlippingHull` obtains `AIBSPTrees` by the door ID and selects
an open or closed precalculated collision set for the current destruction
stage. `NativeAIBSPResourceTests` enumerates the effective package and
loose IDs, strictly deserializes original tags 3 and 4, calls the original
`CLoadTwoBSPTrees` loader, and hashes every stage's sorted piece IDs,
grid cells, collision bounds and extents. This is an original game path,
not a request to port an editor facility.

All 193 package IDs are overridden by 193 shipping loose files. Expected
output:

```
records=193 loose_files=193 overrides=193 loose_only=0 open_stages=965 closed_stages=965 pieces=1006 cells=265169 digest=4D794836A2553C87
```

Windows x86 diagnostic, Windows x64, Linux GCC/Clang x86-64 and
ARM64/QEMU produced the same values. Linux x86-64 used ASan/UBSan/LSan;
ARM64/QEMU used ASan/UBSan with LSan disabled. The original
`AIBSPTrees.res` SHA-256 is
`5570A6E383879E3739735D4CA69F84293EED1FE0D1AE4FBF9B41D025C877DD56`.
Run the registered `NativeAIBSPResourceTests` CTest or directly execute
`NativeAIBSPResourceTests <original-res-directory>`.

This verifies loaded precomputed data, not door collision after an actual
opening/destruction sequence in a mission. That dynamic path still needs
its own state-transition regression.
With this second resource test registered, the non-`extended` suite passed
158/158 on Windows x64 Release and 131/131 on Linux GCC x86-64 under
ASan/UBSan/LSan.

## Inverse bind poses for animated AI hulls

`CAIMap::AddAnimatedHull` and `AddFlippingHull` attach `CFileAIBind` to
the skinning path by AI-geometry ID. The original `CFileAIBind` reads
tag 4 of `AIBinds.res` into inverse bind-pose matrices. The
`NativeAIBindResourceTests` corpus test strictly reads all indexed IDs,
calls that original loader, and hashes every float component in semantic
row/column order rather than hashing the host `SHMatrix` memory layout.

The original package contains 211 IDs and 1577 matrices. There is no
loose `aibinds` directory in the current shipping resource set. Expected
output:

```
records=211 matrices=1577 digest=5E3D53E8692EC077
```

Windows x86 diagnostic, Windows x64, Linux GCC/Clang x86-64 and
ARM64/QEMU produced the same values. Linux x86-64 used ASan/UBSan/LSan;
ARM64/QEMU used ASan/UBSan with LSan disabled. The original
`AIBinds.res` SHA-256 is
`AEBD0611D3290EAB61AB63BB42DE1D57373D20588D3AC01F4AADE8133E33DE79`.
Run the registered `NativeAIBindResourceTests` CTest or execute
`NativeAIBindResourceTests <original-res-directory>` directly.

This proves matrix decoding and original-loader construction, not the
animated collision pose over time or every `CBind::Recalc` branch.
With this third resource test registered, the non-`extended` suite passed
159/159 on Windows x64 Release and 132/132 on Linux GCC x86-64 under
ASan/UBSan/LSan.

The same original `CBind` path is used by rendered models through
`GView.cpp`: `shareBinds` loads inverse poses by geometry ID from
`Binds.res`, while AI hulls use `AIBinds.res`. The resource test now also
strictly reads every `Binds.res` ID, constructs the original `CFileBind`
loader, compares every loaded matrix field bit-for-bit with the direct
serializer read, and hashes the matrices in semantic field order. There
is no shipping loose `binds` directory. Expected combined output:

```
ai_records=211 ai_matrices=1577 ai_digest=5E3D53E8692EC077 model_records=396 model_matrices=7188 model_digest=49A7E7CBF80CB004
```

Windows x86 diagnostic, Windows x64, Linux GCC/Clang x86-64, and ARM64/QEMU
agreed exactly. Linux x86-64 ran under ASan/UBSan/LSan; ARM64/QEMU ran
under ASan/UBSan with leak checking disabled. This is corpus decoding of
model inverse poses, not proof that each ID is used by a shipped model,
nor a tested model/skeleton association or rendered animation. The CPU
bind-pose regression below currently uses `AIBinds`, not `Binds`.

## CPU bind-pose recomputation

`NativeAIBindPoseTests` runs the original `CBind::Recalc` with real
`AIBinds` inverse matrices and real `Skeletons` bone hierarchies. It
selects deterministic count-matched pairs: non-scaled bind/skeleton
2/2 with 42 bones and scaled bind/skeleton 236/50 with five bones.
These two AI pairs are selected by bone count for a CPU regression; the
test does not assert that the game associates those records in a model.

For each pair the test forms a pose from the original skeleton defaults,
then advances DG frames, changes root/child positions and changes root
scale alone. It requires parent/global matrices to update after position
changes; scale-only changes must affect the `bScale` branch and leave the
static-skeleton-scale branch unchanged. All output matrix components are
hashed in semantic order. Expected result:

```
non_scaled_bind=2 skeleton=2 bones=42 scaled_bind=236 skeleton=50 bones=5 ai_digest=AD6A5D46B06FF2AB
```

Windows x86 diagnostic, Windows x64, Linux GCC/Clang x86-64 and
ARM64/QEMU produced this exact value; Linux x86-64 used
ASan/UBSan/LSan, ARM64/QEMU used ASan/UBSan with LSan disabled. Run
`NativeAIBindPoseTests <original-res-directory> <original-game.db>` or
its CTest target.
This exercises core CPU binding across three frames, not a rendered
animation or the downstream collision response in a mission.
After registration, the non-`extended` suites passed 160/160 on Windows
x64 Release and 133/133 on Linux GCC x86-64 under ASan/UBSan/LSan.

The follow-up also loads the original `game.db` and iterates its registered
`Models` records. It creates each model through the game's
`GetModelVariant`, then intersects the selected geometry and skeleton IDs
with `Binds.res` and `Skeletons.res`. There are 110 candidate model
records with both resources and matching bone counts. The first by model
record ID is model 1248, geometry/bind 1486, skeleton 142, nine bones.
This is the same association used by `CGameView::CreateSkin`; it is not
an accidental match between independent package indexes. The test runs
the original `CBind::Recalc` over three DG frames for this pair as well.
Expected additional output:

```
model_pairs=110 model=1248 bind=1486 skeleton=142 bones=9 model_digest=EDD325C53B7749A8
```

Windows x86 diagnostic, Windows x64, Linux GCC/Clang x86-64 and
ARM64/QEMU matched exactly; the Linux runs used ASan/UBSan and x86-64
also LSan. The pose changes remain synthetic, not an original animation
clip. This still does not exercise GPU skinning or a model inside a live
mission; those claims require a separate test.
After the database-backed association was added, the non-`extended`
CTest suites passed 160/160 on Windows x64 Release and 133/133 on
Linux GCC x86-64 under ASan/UBSan/LSan.
