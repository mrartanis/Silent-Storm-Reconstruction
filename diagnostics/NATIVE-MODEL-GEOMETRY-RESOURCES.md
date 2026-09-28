# Game-model geometry resource audit (stage 2)

`CGameView::AddModelPart` and `AddSkinModelPart` request `Geometries` by
`SPartKey(GeometryID, material-part)`. The lower 16 bits of the package
ID carry the database geometry record and the upper bits carry the part.
`CObjectInfoLoader::RecalcValue` reads fields 1, 2, 3, and 5: loaded
vertices, polygon indices, polygon ends, and skin weights. These are
game-used data fields, not editor-only capabilities.

`NativeModelGeometryResourceTests` loads the original `game.db` and
`Geometries.res`, intersects package entries with the `CGeometry` table,
then reads all four fields through the game's `CResourceOpener` and
`CStructureSaver`. It uses the same portable 60-byte vertex and 12-byte
weight codecs as `CObjectInfoLoader`; the digest covers every decoded
field in semantic order, not the C++ object layout. It also checks that
polygon vertex indices and weight vertex references stay within each
record's vertex array. The shipping package has no loose `geometries`
override directory.

Expected output:

```
records=6824 skipped=966 vertices=980946 indices=1994631 polygons=620259 weights=361772 invalid_indices=0 invalid_weights=0 digest=C7F41DAD5636E94A
```

The package contains 7790 entries; 966 do not map to a `CGeometry`
record by lower-16-bit ID. The separate building-piece loader was traced:
it also builds its key from a `CGeometry` DB pointer, and its test confirms
all 814 construction geometry IDs have DB rows. Thus those 966 entries
cannot be selected by either identified base-game geometry loader; their
provenance is unknown, not asserted editor-only. Conversely, a database geometry
record is not proof that its model appears in normal play. The original
corpus audit decodes all DB-linked records; the subsequent CPU assembly
check below constructs `CObjectInfo` for three examples. Neither test
skins on GPU or displays a mesh in a mission.

The original `Geometries.res` SHA-256 is
`138FB3465590A90E11EA556AA71DE88CFEDBD1B68E1840F0E40F7C02163EFE16`.
Run `NativeModelGeometryResourceTests <original-game.db>
<original-res-directory>` or its registered CTest case. The Windows
x86 build is only a diagnostic reference; the target products remain
Windows x64, Linux x86-64, and ARM64.

Windows x86 diagnostic, Windows x64, Linux GCC/Clang x86-64 and
ARM64/QEMU produced the exact counts and digest above. Linux x86-64
ran under ASan/UBSan/LSan; ARM64/QEMU ran under ASan/UBSan with leak
checking disabled. The non-`extended` suites passed 161/161 on Windows
x64 Release and 134/134 on Linux GCC x86-64 after registration.

## CPU object assembly

The next integration packet moves the vertex packing used by
`CObjectInfoLoader::RecalcValue` into `GObjectInfoLoadCore.h`. The Windows
game loader calls the shared `AssignLoadedObjectInfo` routine; the Linux
headless test calls that exact routine after reading the original resource
fields. It executes the original `CObjectInfo::Assign` and
`CTriVertexCacheOptimizer`, not a replacement mesh builder. These original
CPU classes now compile into `s2_game_scene_data` on Linux. In headless mode
the cache optimizer uses the renderer's pre-device default cache size 10;
Windows uses `NGfx::nVCacheSize` (also 10 before device inspection).

The resource test assembles package ID 1 (static geometry without weights)
and the DB-linked model geometry IDs 1486 and 3087 (both weighted). Its
second semantic digest covers resulting triangle/vertex/position/weight
counts, position floats, position references, packed basis and UV values,
sorted weights and bone IDs, and optimized polygon topology. Expected
output appended to the existing corpus line:

```
assembled=3 weighted=2 object_digest=C421973B117E0717
```

The hash matched diagnostic Windows x86, target Windows x64, Linux GCC/Clang
x86-64 and ARM64/QEMU. GCC x86-64 ran under ASan/UBSan/LSan; ARM64/QEMU
ran under ASan/UBSan without LSan. The ordinary Linux GCC suite passed
134/134, and the complete Windows x64 suite passed 165/165 after all
targets were built. This checks three real records, not every
runtime-reachable model, lazy-request scheduling, GPU skinning, clipping
of building pieces, or display in a mission. The 966 extra package IDs
remain undecoded, with no identified base-game loader path.
