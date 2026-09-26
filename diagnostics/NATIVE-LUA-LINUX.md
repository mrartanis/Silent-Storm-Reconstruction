# Original Lua runtime on Linux and ARM64

Stage 2 now builds the game's modified Lua 4 parser, VM, thread scheduler,
function registry, and `Script` wrapper from the **original `Script/` sources**
into `s2_game_lua` on Linux x86-64 and ARM64. This is runtime execution, not
merely decoding a saved Lua record. The native Windows `Script` target still
uses the same source files. `Instruction` and `lint32` are fixed 32-bit words
on all hosts; `IntPoint` hashes full-width pointers rather than truncating
them to Windows `unsigned long`. `NativeLuaWordTests` obtains identical
little-endian instruction hash `292565a5d957c367` on Windows x64, Linux
x86-64, and ARM64/QEMU.

`NativeLuaRuntimeTests` executes a Lua chunk through the game's thread
scheduler (result `96`), then executes another through the `Script` wrapper
(result `39`). With `--autoload SCRIPT_DIR`, it queues the four original
startup scripts in game order, runs 25 scheduler frames with the standard
`Sleep` and `StartThread` functions, and checks selected game constants,
trigger state, and four `out` calls. Each script also passed the parser-only
probe. The licensed files stay in the lab, not the repository:

| Original script | Bytes | SHA-256 |
| --- | ---: | --- |
| `Constants.l` | 1,493 | `9d1cf1a9a11aafc49d0a2b5078e7828dedf0f6c84ba5776af4763d81700c9a80` |
| `TriggersManager.l` | 1,108 | `1c8f517ff2205fd74c076bc87a2ecf3f8238a0bab4092629ff20d59d9bd5ec30` |
| `Common.l` | 15,196 | `2786c9a37bbf6d14c285dcd2f3e7b0c33ad8291c5c75fb2949804f3099655727` |
| `Hint.l` | 101 | `3c8608e8c2b3b91724662862cc08794adda78c1af514c7ac77507b9333d65568` |

To include this resource-dependent regression in CTest, configure with
`-DS2_SCRIPT_CORPUS_DIR=<directory containing these four .l files>` and run
`ctest --test-dir BUILD_DIR -R '^NativeLua' --output-on-failure`. Linux builds
use `-fsanitize=address,undefined`; under ARM64/QEMU set
`ASAN_OPTIONS=detect_leaks=0` because LeakSanitizer cannot operate under
QEMU's ptrace-based execution. The runtime test now round-trips a live
`Script` through the original Linux `CStructureSaver`: global `39` is
restored, then another Lua chunk advances it to `42`. This replaced the
earlier throwing persistence placeholder; see `NATIVE-STRUCTURE-LINUX.md`.

Verified 2026-09-25: Windows x64 CTest 84/84, Linux x86-64 and ARM64/QEMU
51/51 each, including the autoload corpus. These checks do **not** close
stage 2: most game-specific Lua bindings in `Main/` and the world/AI/combat
loop still have no Linux target. The earlier Linux persistence stub has since
been replaced by the real serializer, but only a bounded Lua-state round-trip
has been verified. No Linux `Game.exe` or playable mission is claimed.

A clean Windows x64 native-media archive
`D:\SS-lab\builds\stage2-lua-runtime-20260925-01` was built from commit
`c8f2abf`. Its isolated LabRun `D:\SS-lab\runs\stage2-lua-runtime-01`
loaded the mission slot `TOPWRITE_NEW` to `LOAD-SLOT-DONE`, then exited with
no crash dump. The copied `game.sav` kept SHA-256
`1385447ae22f6da374f44453bbb93034d99e2e7476e5faa9034d026b4ee3e16a`.
This verifies a Windows game regression boundary, not Linux gameplay.

## Authored `game.db` script corpus and a game-used binding (2026-09-26)

`NativeMissionScriptCorpusTests` loads the original `game.db` through the
typed database, iterates every `NDb::CScript` record in stable ID order, and
parses every nonempty source with the game's modified Lua VM. The baseline
contains 113 records, one intentionally empty, and 355,575 bytes of nonempty
source. The ordered ID/source digest is `B5163E4E76664106` on Windows x64
and Linux GCC x64/ARM64; the test asserts all four values. This is authored game
data, not editor-only input. `NativeMissionMapProbe` independently confirms
that `BuildMap` selects and parses the 1,016-byte and 4,266-byte scripts in
variants 810 and 4526. `--print-scripts` on that probe displays the selected
source locally for binding triage; the licensed text is not checked in.

Variant 4526 calls `random(29)` to select an enemy patrol branch. The
original `NScript::luaRandom` implementation now has its own compilation
unit, shared by Windows `Main` and Linux `s2_game_lua`. A deterministic
runtime regression registers that actual binding, calls its one-argument,
two-argument, and invalid-argument forms, and checks the game generator's
results. Seed 2026 gives patrol `10` and selected value `7` on Windows x64,
Linux GCC x64, and ARM64/QEMU. This is a real game-used Lua binding, but it
does not claim that `GetUnit`, route actions, cameras, dialogs, or the live
mission world are portable yet.

After this packet, the full Windows x64 build and CTest passed 104/104;
Linux GCC x64 and ARM64/QEMU with ASan/UBSan passed 78/78 each. The updated `Game.exe` runtime
smoke remains open: the current D3D desktop cannot create a device even for
the older known-good archive, so build success is not treated as playability.

A clean native-media Windows x64 archive
`G:\SS\lab\builds\stage2-script-corpus-20260926-01` was produced from
source commit `7d6a9d25456e47766d00be91eb8bd50d9942d9e6` with 16 build
jobs. `Game.exe` SHA-256 is
`DB66DD392B8F795E59FF955FB7E7DF0F5EAEC52C4B804B56724A9ED8A1B93BD9`.
The archive contains five FFmpeg runtime DLLs and no FMOD DLL; `Game.exe`
has no `fmod.dll` or `FSOUND_` import. Game runtime smoke remains unverified
for the D3D reason above.
