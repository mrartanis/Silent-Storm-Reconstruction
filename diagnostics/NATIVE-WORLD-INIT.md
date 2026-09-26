# Stage 2: original world and Lua initialization

`NativeWorldInitProbe` reads the original `game.db`, registers the original
resource directory, checks the four `AutoLoadScripts` records and their loose
files, constructs the real `NWorld::CWorld(CGlobalGame*)`, executes its script
threads, and checks globals from `Constants.l`, `TriggersManager.l` and
`Common.l`. It runs on Windows x64, Linux x86-64 and Linux ARM64/QEMU. Linux
uses ASan/UBSan with `ASAN_OPTIONS=detect_leaks=0` under QEMU.

The database stores paths such as `scripts\Hint.l`. On non-Windows hosts,
`CScript::RunScriptFile` now uses the existing
`S2FileIO::ResolveGameResourcePath` component resolver: separators are
normalized, exact case is preferred, and ambiguous case-insensitive matches
are rejected. The generic file stream and user saves retain their original
read/write behavior. The world probe checks each database-named script path
through that resolver. The Linux CTest stages the four original Lua files under
`<build>/world-probe-root/Scripts`; Windows uses the configured game directory.

Run the targeted checks:

```sh
cmake --build <linux-build> --target NativeStreamsTests NativeWorldInitProbe -j 16
ASAN_OPTIONS=detect_leaks=0 ctest --test-dir <linux-build> \
  -R '^(NativeStreamsTests|NativeWorldInitProbe)$' --output-on-failure
```

```powershell
cmake --build <windows-build> --config RelWithDebInfo --target NativeStreamsTests NativeWorldInitProbe -j 16
ctest --test-dir <windows-build> -C RelWithDebInfo -R '^(NativeStreamsTests|NativeWorldInitProbe)$' --output-on-failure
```

This proves world construction and startup Lua execution on game data, not
loading a mission, advancing a world segment, AI decisions, rendering or
audio. The test requires the original `game.db`, `Waypoints.res`, and the
four autoload `.l` files. A missing data configuration omits the data test;
the probe executable is still built.

The subsequent `--mission` mode uses the same executable to run actual
`CreateRandom` and `RunPostInit`; its distinct tests, results and remaining
limits are documented in `NATIVE-WORLD-MISSION-LINUX.md`.

On 2026-09-26 the full CTest matrix after this change passed 127/127 on
Windows x64 and 104/104 on both Linux x86-64 and ARM64/QEMU. The Linux
targets were built with GCC and ASan/UBSan. These counts include the new
world probe but do not replace a live mission or Steam gameplay comparison.
