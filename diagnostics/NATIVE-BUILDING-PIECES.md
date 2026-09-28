# Building-piece geometry resources (stage 2)

`GBuilding.cpp` requests `CObjectInfoPiecesLoader` for wall and solid
fragments through `SPartKey(GeometryID, material-part)`. Its resource is
`Geometries.res` field 4, a map of part IDs to `SFacesVector` vertex and
polygon arrays. This is a game render-scene path, not an editor-only
capability. `ReadObjectInfoPieces` is shared by the Windows lazy loader and
the cross-platform diagnostic; it invokes the game's `CStructureSaver`
on the original field.

`NativeBuildingPieceResourceTests` reads the release-v1 `game.db`
`ConstructionParts` table (ID 47), collects its `FirstGeometryID` and
`SecondGeometryID` values, and intersects them with the complete shipped
`Geometries.res` index. This is an intentional *superset* of parts a
particular campaign might construct: not every construction variant is
necessarily selected in ordinary play. The test reads field 4 for every
matching package ID through `CResourceOpener`, hashes decoded fields in
stable part-ID order, and validates every polygon vertex reference and
boundary. It does not merely hash raw package bytes.

Expected output:

```
construction_rows=885 geometry_ids=814 entries=2928 nonempty=2926 parts=28188 vertices=853549 indices=1049304 polygons=294617 digest=288D6D988119C2E5
empty_ids=629,856
unlinked_package_entries=966
```

The test now also checks the source-side DB join: every one of the 814
construction geometry IDs is a row in the shipped `Geometries` DB table
(ID 10). The 966 package entries whose low 16-bit geometry ID has no
such row cannot be selected by `CBuilding::Build`: its wall/solid key is
formed from the resolved `pGeometry` DB pointer. This is a reachability
claim for this game path, **not** a claim that the extra package bytes are
editor-only or decoded. `CGameView`'s model path likewise takes a
`CGeometry` DB pointer; its separate typed audit is in
`NATIVE-MODEL-GEOMETRY-RESOURCES.md`.

The original semantic digest matched diagnostic Windows x86, target
Windows x64, Linux GCC/Clang x86-64, and ARM64/QEMU. The added DB-row
and 966-unlinked assertions passed Windows x64, Linux GCC x86-64 under
ASan/UBSan, and ARM64/QEMU under ASan/UBSan (LSan off); this new
assertion has not been repeated with x86 or Clang. The two empty entries are
preserved as data, not assumed to be errors or editor-only records.
The complete Windows x64 suite passed 166/166 tests; the ordinary Linux
GCC x86-64 suite passed 135/135 tests after this registration.

Build `NativeBuildingPieceResourceTests` and run it with
`<original-game.db> <original-res-directory>`, or run the registered
CTest case in a build configured with those original resources. The
test targets Windows x64, Linux x86-64, and ARM64; x86 is a diagnostic
reference only.

This verifies the typed data consumed by the loader, not the asynchronous
lazy-request path, `CWallObjectInfoClipper`/`CSolidObjectInfoClipper`, GPU
geometry, or rendering in a mission. Those remain separate integration
checks. The 966 extra package entries remain of unknown provenance but
have no identified base-game path through these two geometry loaders.
