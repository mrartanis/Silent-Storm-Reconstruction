# Building-piece CPU clipping (stage 2)

`GBuilding.cpp` asks `CWallObjectInfoClipper` and
`CSolidObjectInfoClipper` for wall and solid fragments. Both original
classes live in `Main/GObjectInfo.cpp`, which is now compiled into the
portable Linux scene-data target as well as the Windows game. The plane
hash no longer reads a `float` through an incompatible `int*`; its bits
are copied with `std::memcpy`.

`NativeBuildingClipperTests` derives candidate construction geometry IDs
from the original `game.db` `ConstructionParts` table and reads part
geometry from the original `Geometries.res`. It starts the game's lazy
resource-loading thread, then calls the original DG-backed solid and wall
clippers for resource 345, part 0. One wall request has no clip; another
uses the game's encoded clip flags and produces a different mesh. The
resulting position triangles are canonicalized by sorted vertices,
quantized at 1/4096 and hashed, so vertex enumeration order does not
affect the cross-architecture comparison.

Expected output:

```
resource_id=345 part_id=0 solid_triangles=26 wall_triangles=10 clipped_triangles=8 digest=99B3282EA22ED8EA
```

Build the `NativeBuildingClipperTests` target and run its registered CTest
case with the original `game.db` and `res/Geometries.res` configured.
The executable also accepts `<game.db> <res-directory>` directly.
Diagnostic Windows x86, target Windows x64, Linux GCC/Clang x86-64 and
ARM64/QEMU matched. GCC x86-64 was checked with ASan/UBSan/LSan;
ARM64/QEMU with ASan/UBSan. Windows x86 is a diagnostic reference, not a
supported release target. The full Windows x64 CTest passed 167/167;
ordinary Linux GCC x86-64 CTest passed 136/136 with sanitizers.

This is a real CPU clipping and lazy-loading check, not merely a parser
test. It exercises one original construction part and one clipping
combination. It does not establish coverage of all 28,188 parts, all
clip-mask combinations, GPU geometry, or rendering in a mission.
