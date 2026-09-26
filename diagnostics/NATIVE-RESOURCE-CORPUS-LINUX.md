# Original resource-package corpus (stage 2)

`NativeResourcePackageTests --corpus <res-dir>` enumerates the 23 shipped
`.res` files by fixed name. For each resource ID in sorted order, it reads
the payload both through the game's original `OpenFilesPackage` /
`CPackageStream` path and through `PortablePackageIndex`, then requires
byte-for-byte equality. The test hashes package names, IDs, lengths, and
payload bytes with FNV-1a and checks the result against the original
Windows baseline: 57,880 entries, 2,200,910,768 payload bytes, digest
`B90BEA736E6BF987`. The 64-bit byte counter deliberately crosses 2 GiB.

The corpus includes AI, geometry, animation, building, effect, head,
sequence, sound, terrain, and texture packages. This establishes
cross-platform container/index loading of the shipped data, not semantic
decoding of every record type, game execution, or a claim that every
package is accessed in every mission. Existing focused tests cover
game-used typed records and selected runtime paths separately.

Windows: configure with `S2_GAME_DIR` pointing to the original game
directory, then run `ctest --test-dir <build> -C RelWithDebInfo -R
'^NativeResourceCorpusTests$' --output-on-failure`. Linux: configure
with `S2_RESOURCE_PACKAGE_PATH` pointing at a `.res` in the same
directory as all 23 packages, then run `ctest --test-dir <build> -R
'^NativeResourceCorpusTests$' --output-on-failure`. Linux GCC builds
use ASan/UBSan; ARM64 executes under the configured QEMU emulator with
`ASAN_OPTIONS=detect_leaks=0`.

Verified 2026-09-26: Windows x64 built `Game.exe` and passed 123/123 CTest;
Linux GCC x86-64 and ARM64/QEMU passed 98/98 each under ASan/UBSan.
All three native corpus runs printed the same counts and digest above.
The original package bytes were kept in the lab and Linux test directory,
not committed to source control.
