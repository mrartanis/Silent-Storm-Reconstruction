# Campaign-map resource data on 64-bit hosts

`NativeCampaignMapResourceTests` exercises the original game-used
`CChapterInfoLoader` and `CGlobalInfoLoader` against the shipped
`Chapters.res` and `Globals.res`. It first performs strict typed decoding
where the package entry is a matching record, then hashes every typed field
returned by the game's loader: map/scenario IDs, deploy/image positions,
sectors, point lists, sector types and IDs. The hash includes exact float bits
and string bytes. This is a data-loading test, not a rendered-map or campaign
behavior comparison with Steam.

The original `Chapters.res` has 27 entries. Twenty are `CChapterInfo` records
and decode strictly; seven large entries (IDs 1, 2, 4, 6, 7, 8, 9) are not
that type and cannot be passed to `CChapterInfo::operator&` (their scalar
tags have different sizes). The package-level test separately verifies the
bytes of all 27. The four `Globals.res` entries are loaded by
`CGlobalInfoLoader`; three decode strictly. Entry 2 has a six-byte field at
tag 5 where the reconstructed `CVec2` path expects eight bytes. The original
game loader catches this exception and returns an already-partially-read
object (`nMapID=523`, two sectors). The regression pins this exact mismatch
and the loaded values instead of accepting any exception or presenting the
entry as fully decoded. Its full semantics remain a targeted investigation
if that specific campaign map misbehaves; this test alone does not prove it.

On the baseline resources the result is: 20 typed chapters, 7 non-chapter
entries, 176 chapter sectors, 4 globals (one partial), 19 global sectors,
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
```

For direct ARM64 execution, use the configured CTest emulator or
`qemu-aarch64-static -L /usr/aarch64-linux-gnu` with the two `.res` paths.
`S2_RESOURCE_PACKAGE_PATH` determines the resource directory in the portable
CMake build. The resources themselves stay outside version control.
