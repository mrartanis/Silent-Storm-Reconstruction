# Game-used mod manager: portable filesystem path

`CModManager` is called by the custom-game menu and by save loading when a
save header names active mod directories. It is therefore a game path, not an
editor-only helper. The Linux build now compiles the original manager and
keeps its activation order: validate every requested description before
teardown; clear holders, resources and database; reload `game.db`; register
base resources; attempt each mod database and register its resource directory
even if that database is absent; then increment the database version and
publish the active mod list. The Windows implementation retains its original
Win32 file-enumeration and diagnostic behavior.

On Linux, directory enumeration uses `std::filesystem`; description and
mod-database paths go through the existing case-insensitive, backslash-aware
game path resolver. Description text remains a byte string as on Windows.
The original database and resource loaders remain in use; this does not add
an editor/ADO import path or change the save-file format.
The original save header stores mod directory names as narrow byte strings.
The baseline and this fixture use ASCII names; non-ASCII Windows code-page
mod names have not been mapped to a Linux filesystem encoding and are not
claimed portable by this test.

`NativeModManagerTests` creates its own scratch directory inside the build,
copies the original `game.db`, and uses a description-only mod. It verifies
discovery, mixed-case selection, successful base reload, active-mod/version
state, rejection of an unavailable mod *before* clearing the live database,
and reactivation with no mods. Separate loose resource markers at the same
`Globals/3` ID prove that an active mod overrides base resources, that a
failed preflight leaves the override active, and that deactivation restores
the base resource. The test deliberately covers the original behavior where
a mod without `game.db` is still activated as a resource directory. It does
**not** prove merging an authored mod `game.db`, UI interaction, or
round-tripping a save that names a mod. Those remain separate tests before
claiming full mod support on Linux.

The focused test passed Windows x64, Linux GCC x86-64 with ASan/UBSan/LSan,
Linux GCC ARM64/QEMU with ASan/UBSan, and Linux Clang x86-64 release. Run:

```text
ctest --test-dir <windows-build> -C RelWithDebInfo -R ^NativeModManagerTests$ --output-on-failure
ASAN_OPTIONS=detect_leaks=1 ctest --test-dir <linux-x64-build> -R ^NativeModManagerTests$ --output-on-failure
ASAN_OPTIONS=detect_leaks=0 ctest --test-dir <linux-arm64-build> -R ^NativeModManagerTests$ --output-on-failure
```

`S2_GAME_DB_PATH` must point to the original `game.db`; the test never edits
that source file. It removes only its uniquely named successful fixture under
the configured build scratch directory and retains a failed fixture for
debugging.
