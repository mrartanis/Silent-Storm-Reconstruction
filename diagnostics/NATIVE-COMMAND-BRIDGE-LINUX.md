# World-to-unit command bridge on Linux (stage 2)

The game's `CCmdSetCommand::IsSkippable` has moved unchanged from
`Main/wInterface.cpp` to `Main/wCommandBridge.cpp`, which is compiled by
both the Windows `Main` target and Linux `s2_game_command_bridge`. The
original `Main/wUnitCommands.cpp` also compiles on Linux as
`s2_game_unit_commands`, including its command registrations and lock
methods; this library is not yet a linked, running unit-command world.

`NativeCommandBridgeTests` constructs real `CCmdSetCommand` wrappers around
the game's `CCmdContinue` (skippable) and `CCmdStartCombat` (not skippable),
then calls the actual bridge method. It passes on the restored Windows x86
oracle, Windows x64, Linux GCC x86-64, and Linux GCC ARM64/QEMU with
ASan/UBSan. It tests command classification, not dispatch to a live unit.
The Linux test links the bridge, but does not exercise the three lock-method
bodies or the static save/load registrations from `wUnitCommands.cpp`.

The full Windows x64 build/CTest passed 109/109; Linux GCC x86-64 and
ARM64/QEMU built all targets and passed 81/81 each under ASan/UBSan with
`ASAN_OPTIONS=detect_leaks=0`. Reproduce with `cmake --build <build>
--target NativeCommandBridgeTests -j 16` and `ctest --test-dir <build> -R
'^NativeCommandBridgeTests$' --output-on-failure` on Linux; on Windows add
`--config RelWithDebInfo` to the build and `-C RelWithDebInfo` to CTest.

The command bridge removes one linker dependency of `s2_game_ai_logic`.
Linux still lacks the linked `CUnitServer` and RPG world needed for
`CAILogic::GetCommand` and live route execution. No test double or
unresolved-symbol exception is counted as completing that gate.

A clean native-media Windows x64 archive
`G:\SS\lab\builds\stage2-command-bridge-20260926-01` was built with 16
jobs from source commit `f85441fabb09745de28ce0f779d5d96e9571db73`.
`Game.exe` SHA-256 is
`7C0CE667D88638A4A950A23D713E71825A15F9964703977AD1FB12EE3730050E`.
The archive has no FMOD DLL, and `Game.exe` has no `fmod.dll` or
`FSOUND_` import. In-game smoke remains unverified because the current
remote D3D session cannot create a device even for an older known-good
archive.
