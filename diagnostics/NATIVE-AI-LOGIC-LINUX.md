# Command-driven AI logic base on Linux (stage 2)

`Main/aiLogic.cpp`, the Windows game's actual `CAILogic` implementation,
now compiles into `s2_game_ai_logic` with GCC on Linux x86-64 and ARM64.
The source, not a stand-in, retains the live-unit keepalive, command-queue
gate, anti-cycling threshold, and save tags. Its existing Windows build
uses the same translation unit.

The portability changes are limited to the include path/case, explicit
`int`-backed enum declarations that MSVC previously accepted implicitly,
standard C++ dependent-type syntax in the included AI headers, and use of
the game's Linux ISAAC generator (`s2_game_random`) in `CheckCycling`.
They do not change the Windows random source or serialized layout. The
new `NativeAILogicTests` constructs a real detached `CAILogic` on Windows
x64 and checks pause/resume, finish, and end-of-turn transitions.

This is a compile boundary, **not a running Linux AI world**. Linking the
detached logic test on Linux still requires the original `CUnitServer`,
`CCmdSetCommand`, RPG mission cast, and related world-command methods.
These remain outside the current Linux link graph; no replacement methods
or unresolved-symbol linker exceptions were added. In particular, the
Windows lifecycle test does not prove route movement or enemy decisions.
After this packet, the full Windows x64 build and CTest passed 108/108;
Linux GCC x86-64 and ARM64/QEMU built all targets and passed 80/80 tests
each under ASan/UBSan (`ASAN_OPTIONS=detect_leaks=0`). The Linux suites
do not execute `CAILogic`; that target is compile-checked on both hosts.

Reproduce with `cmake --build <linux-build> --target s2_game_ai_logic -j 16`
on each Linux architecture, or `cmake --build <windows-build> --config
RelWithDebInfo --target NativeAILogicTests --parallel 16` followed by
`ctest --test-dir <windows-build> -C RelWithDebInfo -R '^NativeAILogicTests$'
--output-on-failure`. The data/route and Lua tests live in
`NATIVE-MISSION-BUILD-LINUX.md` and `NATIVE-LUA-LINUX.md`; they do not
substitute for the world-command link and live mission execution.

A clean native-media Windows x64 archive
`G:\SS\lab\builds\stage2-ai-logic-20260926-01` was produced with 16 jobs
from source commit `967e43e881290a87797e58c2d99f182e16b6bf50`.
`Game.exe` SHA-256 is
`0ED8E683C513FCCA7D9CEF42254B0F4E68C61AD21E72058DC0BAA49CB26251C0`.
There is no FMOD DLL in the archive and no `fmod.dll` or `FSOUND_` import
in `Game.exe`. An in-game smoke was not repeated: the current remote D3D
session cannot create a device even for an older known-good archive.
