# Lua runtime state records on disk

The mission save audit after `RAY_BOUNDS_NEW` still reported native writes
of `TMinfo` (15 tag-method indices, 60 bytes) and `CallInfo` (five integer
fields, 20 bytes). Commit `64c7787` gives both records an explicit
little-endian signed-32-bit field codec. It preserves their existing Windows
wire bytes and does not change Lua execution rules or claim to port all of
Lua to ARM64.

`PortableLuaStateWireTests` checks exact byte order, round trips and bad
lengths/pointers; `NativeLuaStateWireTests` checks both structures against
the old Windows memory layout. Windows x64 built `Game.exe` and passed
CTest 72/72; targeted Windows x86 tests passed 2/2. Linux x86-64 and
ARM64/QEMU each passed 42/42 portable tests under ASan/UBSan. The Linux
results are headless core checks, not playable game builds.

The clean native-media x64 archive is
`D:\SS-lab\builds\stage2-lua-state-wire-20260925-01`. Its isolated run
`D:\SS-lab\runs\stage2-lua-state-clean-01` loaded `RAY_BOUNDS_NEW`, saved
`LUA_STATE_NEW`, loaded that new slot to `LOAD-SLOT-DONE`, and exited without
a crash dump. SHA-256 of `LUA_STATE_NEW/game.sav` is
`d3cbf6b96c3f9730173c76d69b6eef18ba339b800bb3da84a05bdee16d4d53e1`.
The full `_wireaudit.log` has eight remaining raw paths in this mission
instead of ten; neither `TMinfo` nor `CallInfo` remains. These eight paths
are rendering/light records, but one mission and the raw-codec audit do not
cover every game serialization path.

The reconstructed x86 archive in
`D:\SS-lab\runs\stage2-lua-state-x86-01` loaded the unchanged new slot to
`LOAD-SLOT-DONE` and exited. This checks backward-format readability, not
behavioral parity with the shipped game. The original Steam executable's
main menu was opened from an isolated copy, but this run did not provide a
reliable observation of the new slot loading; the direct Steam gate for
`LUA_STATE_NEW` remains open.

Reproduce with CMake/CTest on each architecture. On Windows create a clean
archive using `diagnostics/Build-Lab.ps1 -Architecture x64 -NativeMedia`,
then a `New-LabRun.ps1 -SkipIntro -LinkResources` run. Copy an earlier slot
under `<run>/user-data/save/default`, put its name in
`<run>/game/_loadslot.txt`, and use `Start-LabRun.ps1` with
`-windowed -800 -harness -loadslot`. After the first `LOAD-SLOT-DONE`, send
`save LUA_STATE_NEW`, `load LUA_STATE_NEW`, then `quit` via
`<run>/game/_harness_cmd.txt`, one command at a time. Inspect the complete
`_wireaudit.log`, `_saveload.log`, save hash, and crash-dump directory.
