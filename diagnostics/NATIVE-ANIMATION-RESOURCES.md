# Effective animation resources on 64-bit targets

`NativeAnimationResourceTests` reads every ID in the original
`Skeletons.res`, `Locators.res`, and `Animations.res` indexes, plus numeric
IDs present only as loose files. Each entry is deserialized strictly with
the game's `CStructureSaver` fields and through the corresponding original
`CFileSkeleton`, `CFileLocators`, or `CFileAnimation` lazy loader. The test
hashes named bones, parent indices, transform float bits, animation headers,
and every key, not the host C++ structure bytes.

The shipping `res/animations` directory has 38 loose files: 36 override
package IDs and two supply IDs absent from `Animations.res` (2628 and 2629). The game opens
loose files before package entries. A Linux scratch directory containing only
the 23 `.res` files decoded all 2,855 package animations successfully but
produced a different digest from Windows starting at animation ID 960.
The ID 960 package and loose payloads are both valid; this was an input-set
difference, not a compiler or codec defect. The resource corpus test over
`.res` packages alone is therefore a package-container test, not proof of
the effective game resource set.

With the original loose directories present, the test requires:

```
skeletons=139 bones=2080 locators=152 locator_bones=152 animations=2857 keys=6734199 digest=351FF3B89A911CE8
```

Windows x64 Release, Linux GCC x86-64 under ASan/UBSan/LSan, Linux Clang
x86-64 under ASan/UBSan/LSan, and ARM64/QEMU under ASan/UBSan (LSan off)
all produced those exact values. A separately built Windows x86 diagnostic
from the restored source produced the same values as an additional 32-bit
data oracle; x86 is not a target product build. ARM64/QEMU took about 122 seconds.
The three `.res` inputs had identical SHA-256 on Windows and Linux:
`Animations.res` `DBBB4DFEB9F6FB1D2367D0977AC863FB4D9AE0E4061D5F82404DFBDD5D5BEE92`,
`Skeletons.res` `9553E7F80AF45444E49CBA3CF5DBC007CB4603DA8406252D80F1484BBCBCBD35`,
and `Locators.res` `E131BC6845DAEB6EFE707C4BFCF4337DD92715A259870A8D48D2D7CD3D6F84F2`.
The
loose-resource ZIP transferred to the Linux lab had matching SHA-256
`5F99975E2A3DC59C38FD685C43A17EA78617E2720B243F682D1745D0BDF1BE26`.
Licensed resources and the ZIP remain outside Git.

Run `NativeAnimationResourceTests <original-res-directory>` or the
registered CTest target after configuring `S2_GAME_DIR` on Windows or
`S2_RESOURCE_PACKAGE_PATH` on Linux:

```
ctest --test-dir <windows-build> -C Release -R '^NativeAnimationResourceTests$' --output-on-failure
ASAN_OPTIONS=detect_leaks=1 UBSAN_OPTIONS=halt_on_error=1 ctest --test-dir <linux-build> -R '^NativeAnimationResourceTests$' --output-on-failure
```

On ARM64/QEMU, set `ASAN_OPTIONS=detect_leaks=0`. The test fails if
loose-only animations or any effective animation fields are missing. This
covers data loading and portable interpretation, not visual animation
playback, every game resource class, or dynamic parity with Steam.
