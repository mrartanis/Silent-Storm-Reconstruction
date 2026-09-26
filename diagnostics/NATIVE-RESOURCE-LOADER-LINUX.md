# Original resource-package and waypoint loading (stage 2)

The game's `FilesPackage.cpp` now builds on Linux for **read-only** `.res`
loading. The Windows-only directory rescan and package creation functions
are editor tools and are excluded from this Linux target. Package indexing
still uses the existing portable release/Steam decoder, but bytes are read
through the original `CPackageStream` rather than a replacement game API.
`NativeResourcePackageTests` compares every original package stream with
the portable decoder: `Waypoints.res` contains 2,364 entries and 109,472
payload bytes, exactly equal on Windows x86/x64 and Linux GCC x64/ARM64 and
Clang x64.

The original `GResource.cpp` now also builds on Linux. Its game-used opener
keeps the release order: a loose file in the latest resource directory wins,
then the latest `.res` package containing the ID, then the base package.
Linux normalizes backslashes and resolves each path component against its
actual case; ambiguous case-insensitive matches are rejected. It uses
recursive mutexes, a condition variable, and a joined worker thread in
place of Win32 synchronization primitives. `CFileRequest` readiness and
the in-flight flag are atomic; the queue marks a request in-flight before
removing it, so `CloseAllResources` cannot pass a just-dequeued read.
`NativeResourceOpenerTests` covers base package ID 8 (28 bytes), a later
loose override (14 bytes), and the same 14 bytes via asynchronous request.
All platforms produced:

```
package_id=8 package_bytes=28 loose_bytes=14 async_bytes=14
```

`NativeWaypointResourceTests` then exercises the **original**
`CWaypointLoader` and game serializer on `Waypoints.res`. The baseline has
2,362 successfully decoded waypoints with 717 commands and semantic digest
`718C2878C2808774` on Windows x86/x64 and Linux GCC x64/ARM64/Clang x64.
IDs 8 and 9 throw during strict `CWaypoint` deserialization on *all* these
builds; the test explicitly expects those two failures. The legacy loader
leaves a non-null partially constructed object after catching the exception,
so simply checking `GetValue()` would give false confidence. This is a
recorded original-data/loader limitation, not evidence that every package
entry represents a valid waypoint.

`NativeAIRouteResourceTests` exercises the two other original
`aiWaypoint.cpp` loaders that the game uses for AI route data:
`CUnitAIInfoLoader` over every `Units.res` entry and
`CUnitGroupAIInfoLoader` over every `Groups.res` entry. It first performs
strict `CUnitAIInfo` deserialization, then compares every route's waypoint
vector with the normal loader result. The Steam-matching baseline contains
169 unit and 42 group records; all 211 decode, with 211 routes and 561
waypoint-name references. The semantic digest is `F76E40DB1564FEE0` on
Windows x86/x64 and Linux GCC x64/ARM64. These route integers are IDs of
`NDb::CWaypointName` in `game.db`, **not** resource IDs in `Waypoints.res`;
this is how the original `BuildMap` resolves them. Three references do not
resolve in the baseline database: unit source 330/name 46, unit source
1680/name 66, and group source 43/name 441. `BuildMap` skips unresolved
names. Both `.res` inputs and `game.db` match the installed Steam
files by SHA-256, so the test records the source-data behavior and does not
invent replacements.

For this AI-route packet, Windows x86 passed the targeted oracle test,
Windows x64 passed a full build and CTest 105/105, and Linux GCC x64 and
ARM64/QEMU with ASan/UBSan passed 79/79 each. These are typed data-loader
checks, not live path-following or enemy-decision tests.
No new game archive was made for this packet: it adds a diagnostic target
over already linked game loaders and does not change `Game.exe` source.

Configure the portable build with
`-DS2_RESOURCE_PACKAGE_PATH=<original-Waypoints.res>`, then run:

```
cmake --build <linux-build> --target NativeResourcePackageTests NativeResourceOpenerTests NativeWaypointResourceTests NativeAIRouteResourceTests -j 12
ctest --test-dir <linux-build> -R '^Native(ResourcePackage|ResourceOpener|WaypointResource|AIRouteResource)Tests$' --output-on-failure
```

On Windows, build the same four targets with `--config RelWithDebInfo` and
run CTest with `-C RelWithDebInfo`; the AI-route test is registered when
`game.db`, `Units.res`, and `Groups.res` are present together (the Linux
configuration uses `S2_RESOURCE_PACKAGE_PATH` to find their `res` directory).
The Linux x64 and ARM64 GCC builds
use ASan/UBSan (ARM64 under QEMU with `ASAN_OPTIONS=detect_leaks=0`). Full
regressions after the resource-loader port passed Windows x64 97/97 and
Linux GCC x64/ARM64 plus Clang x64 71/71 each.

Clean native-media Windows x64 archive
`G:\SS\lab\builds\stage2-resource-loader-20260925-01` was built from
`4d59560`. Fresh linked-resource run
`G:\SS\lab\runs\stage2-resource-loader-clean-01` loaded `DB_OLD` to
`LOAD-SLOT-DONE`, accepted `quit`, and exited without a crash dump or
`fmod.dll`. Archive/run `Game.exe` SHA-256 matched:
`50045E9F1EFD5B9F0087A8FED7CE5DD2AF543AC248ACCB18B05C652CBC200AB2`.
This validates the Windows game after the common resource-header and queue
changes; the Linux opener/waypoint evidence is from the separately linked
native tests above.

This proves package/loose/async resource access and waypoint deserialization,
not full map assembly. `BuildMap` still needs its building, terrain, unit,
script and path-network dependencies linked and executed against a mission.
