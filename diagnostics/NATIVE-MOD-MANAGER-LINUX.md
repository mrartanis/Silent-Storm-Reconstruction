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
copies the original `game.db`, and uses a mod with both a description and a
complete release-format `game.db`. The mod database is a private copy of the
original with one `GlobalMaps` `StartZoneID` changed to another existing zone;
the test validates the patched database with the portable decoder before
activation. It verifies
discovery, mixed-case selection, successful base reload, active-mod/version
state, rejection of an unavailable mod *before* clearing the live database,
and reactivation with no mods. It checks that the changed zone is visible
while the mod is active and that the original zone returns after deactivation.
Separate loose resource markers at the same
`Globals/3` ID prove that an active mod overrides base resources, that a
failed preflight leaves the override active, and that deactivation restores
the base resource. The fixture also writes the active-mod count using the
portable save-header encoder and the directory name using `CFileStream`'s
game string format, reads both back, and reactivates the mod from that name.
The loader retains the original behavior where a mod
without `game.db` is still activated as a resource directory, but that branch
is no longer covered by this fixture. This test proves a full mod database
replaces one game-used value; it does **not** prove partial-table mod database
merging or UI interaction. The native fixture does not deserialize a game
world, so it is not a Linux/ARM64 full-save test.

The focused test, including the database overlay and save-header/name
round-trip, passed Windows x64, Linux GCC x86-64 with ASan/UBSan/LSan,
Linux GCC ARM64/QEMU with ASan/UBSan (556.25 seconds under QEMU), and
Linux Clang x86-64 release. Run:

```text
ctest --test-dir <windows-build> -C Release -R ^NativeModManagerTests$ --output-on-failure
ASAN_OPTIONS=detect_leaks=1 ctest --test-dir <linux-x64-build> -R ^NativeModManagerTests$ --output-on-failure
ASAN_OPTIONS=detect_leaks=0 ctest --test-dir <linux-arm64-build> -R ^NativeModManagerTests$ --output-on-failure
```

`S2_GAME_DB_PATH` must point to the original `game.db`; the test never edits
that source file. It removes only its uniquely named successful fixture under
the configured build scratch directory and retains a failed fixture for
debugging.

A separate real-game Windows x64 check used the clean native-media archive
`stage2-mod-manager-20260928-01` (Game.exe SHA-256
`C018D1B21DCC5B5C97071CCA4B91D2A09CE35E2766700967C95F8A4127385450`)
in isolated LabRun `G:\SS\lab\runs\stage2-modded-save-live-20260928-01`.
`New-ModdedSaveFixture.ps1` copied the unmodified `GEOM_NEW/game.sav` payload
into new slot `MOD_FIX`, changed its zero mod count to one, and inserted the
`TestMod` directory string before the serialized world. The original slot
SHA-256 was `C0C0D0EEDEEB970F214C49C54FC2F1D27C6D11DE5EE5207F27635A55ED9DDB7A`;
the derived `MOD_FIX/game.sav` SHA-256 is
`5D1221670A0872CB5295252F93EE21E76EAABFCACF3A35636E6515FB60A3B80A`.
The LabRun contained its own `TestMod/description.txt` and full copied
`TestMod/game.db`; the baseline and source save were not changed.

The harness loaded `MOD_FIX` to `LOAD-DESERIALIZE-COMPLETE (1 interfaces)` and
`LOAD-SLOT-DONE`. It then saved `MOD_ROUND`: its `game.sav` is 4,111,232 bytes,
has header mod count one and the expected `TestMod` string at offset 256,008,
with SHA-256 `5345CCF243BB81E43024663B8938D8140450031601C4833178790720D2F36BD4`.
Loading `MOD_ROUND` again reached both completion markers. Harness `quit`
closed the game without `crash.dmp`. This is a real Windows save/load
round-trip with a named mod, not a UI mod-selector test, Linux game run, or
proof that a mod authored independently of the base DB works.

To reproduce the live check, create a fresh `New-LabRun.ps1 -SkipIntro
-LinkResources` run from the archive above. In that run's `game` directory,
create `TestMod/description.txt` and copy its own `game.db` to
`TestMod/game.db`. Use `New-ModdedSaveFixture.ps1 -SourceSave
<GEOM_NEW/game.sav> -TargetSave <run/user-data/save/default/MOD_FIX/game.sav>
-ModDirectory TestMod`; the script refuses an existing target. Launch with
`Start-LabRun.ps1`, then send `load MOD_FIX`, `save MOD_ROUND`,
`load MOD_ROUND`, and `quit` one at a time via atomic rename to the run's
`game/_harness_cmd.txt`. Check `_saveload.log`, the saved header bytes, and
`evidence/crash.dmp`. These operations must stay inside an isolated LabRun;
do not modify the source slot or the installed game.
