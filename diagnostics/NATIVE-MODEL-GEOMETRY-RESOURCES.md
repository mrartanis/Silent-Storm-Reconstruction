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
record by lower-16-bit ID. They are not classified as editor-only or
unneeded without tracing their callers. Conversely, a database geometry
record is not proof that its model appears in normal play. This audit
decodes the model resource format but does not construct
`CObjectInfo`, run `ConvertVertices`/`ConvertWeights`, skin on GPU, or
display the mesh in a mission. Those remain separate integration gates.

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
