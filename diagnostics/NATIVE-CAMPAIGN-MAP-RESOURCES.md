# Campaign-map resource data on 64-bit hosts

`NativeCampaignMapResourceTests` exercises the original game-used
`CChapterInfoLoader` and `CGlobalInfoLoader` against the shipped
`Chapters.res` and `Globals.res`. It first performs strict typed decoding
where the package entry is a matching record, then hashes every typed field
returned by the game's loader: map/scenario IDs, deploy/image positions,
sectors, point lists, sector types and IDs. The hash includes exact float bits
and string bytes. This is a data-loading test, not a rendered-map or campaign
behavior comparison with Steam.

The original `Chapters.res` has 27 entries. Twenty decode strictly as
`CChapterInfo`; seven large entries (IDs 1, 2, 4, 6, 7, 8, 9) have a
different field shape and are covered as raw package bytes, not labelled
as images or claimed to be typed chapter descriptions. `Globals.res` has
four entries; three decode strictly. Entry 2 has a six-byte field at tag 5
where the reconstructed `CVec2` path expects eight bytes. The game loader
catches this exception and returns an already-partially-read object
(`nMapID=523`, two sectors). The regression pins the exact mismatch and
loaded values rather than accepting an arbitrary exception.

Reachability narrows the product gate. The normal side menu hardcodes Axis
and Allies (side IDs 1 and 2); the shipped `game.db` maps them to global maps
3 and 4. Those two `Globals.res` entries decode strictly, and their sectors
reference 14 unique chapter IDs (`12`, `16`–`28`). Every one of those 14
`Chapters.res` descriptions decodes strictly. The database also contains
global-map rows 1, 5 and 6 and chapter-map rows outside this reachable set;
`Globals.res` ID 2 is not a `GlobalMaps` row, while rows 5 and 6 lack entries
in this resource package. They may matter to debug commands, custom content
or other paths, but this evidence does not establish them as normal-menu
gameplay. Do not infer full custom-campaign support from this test. A future
report involving one of these IDs needs an address-specific investigation.

On the baseline resources the result is: 20 typed chapters, 14 normal-menu
reachable chapters, 7 other large entries, 176 chapter sectors, 4 globals
(one partial and not referenced by `GlobalMaps`), 19 global sectors,
digest `4833295F6BB75174`. The same result passed Windows x64, Linux GCC
x86-64 with ASan/UBSan/LSan, Linux GCC ARM64/QEMU with ASan/UBSan, and Linux
Clang x86-64 release. The native loader source (`ChapterInfo.cpp` and
`GlobalInfo.cpp`) now also compiles on Linux; no editor-only loader is needed.
After linking the common serializer change, the ordinary Windows x64 suite
passed 153/153 and the Linux GCC x86-64 suite passed 124/124.

Reproduce with a configured build whose resource path points into the
original `res` directory:

```text
ctest --test-dir <windows-build> -C RelWithDebInfo -R ^NativeCampaignMapResourceTests$ --output-on-failure
ASAN_OPTIONS=detect_leaks=1 ctest --test-dir <linux-x64-build> -R ^NativeCampaignMapResourceTests$ --output-on-failure
ASAN_OPTIONS=detect_leaks=0 ctest --test-dir <linux-arm64-build> -R ^NativeCampaignMapResourceTests$ --output-on-failure
<windows-build>/RelWithDebInfo/NativeMapDatabaseTests.exe <game.db> --campaign-ids
```

For direct ARM64 execution, use the configured CTest emulator or
`qemu-aarch64-static -L /usr/aarch64-linux-gnu` with the two `.res` paths.
`S2_RESOURCE_PACKAGE_PATH` determines the resource directory in the portable
CMake build. `NativeMapDatabaseTests` also checks the side 1/2 to global-map
3/4 relationship without the optional print flag. The resources themselves
stay outside version control.
