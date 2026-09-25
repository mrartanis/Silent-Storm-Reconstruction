# Original map builder Linux boundary (stage 2)

`s2_game_map_build` compiles the original `Main/MapBuild.cpp` on Linux GCC
x64/ARM64 and Clang x64. The changes are case-correct paths/includes,
standard C++ dependent-type qualifiers, explicit `int` enum forward
declarations, and a named temporary random generator where MSVC accepted
the address of an rvalue. Windows x64 and supplemental x86 still build the
same game translation unit. The three shared waypoint/unit loader objects
now live with their types in `aiWaypoint.cpp`, allowing the independent
`ConvertFlags` section to link without pulling unfinished resource loading.

`NativeMapFlagsTests` links the original `MapBuild.cpp`, loads an original
`game.db` through `NDatabase::Serialize`, and calls the actual
`ConvertFlags` used by `wMain.cpp` for mission parameters. With explicit
`Day`, `Night`, and an unknown parameter, the baseline result is:

```
attributes=9 day=6 night=5 flags=5,6
```

The exact result was observed on Windows x86/x64, Linux GCC x64 with
ASan/UBSan, Linux GCC ARM64 with ASan/UBSan under QEMU, and Linux Clang x64.
Full CTest suites: Windows x64 94/94 and each Linux configuration 68/68.

This is a *linked game-map entry point*, not a complete mission load.
`BuildMap` itself still depends on `aiWaypoint.cpp`'s resource loaders and
`GResource.cpp`; those use Windows-specific synchronization/file APIs and
backslash resource paths. They are not replaced by stubs in this test.
Consequently Linux mission geometry, terrain assembly, AI routes, and
scripted placement are not yet verified. The next integration boundary is
the real resource/package opener, followed by `BuildMap` on a game-used
variant and comparison with the Windows/x86 output.

Reproduction:

```
cmake --build <linux-build> --target NativeMapFlagsTests -j 12
ctest --test-dir <linux-build> -R '^NativeMapFlagsTests$' --output-on-failure
cmake --build <windows-build> --target NativeMapFlagsTests --config RelWithDebInfo --parallel 16
ctest --test-dir <windows-build> -C RelWithDebInfo -R '^NativeMapFlagsTests$' --output-on-failure
```

Linux CTest needs `-DS2_GAME_DB_PATH=<original-game.db>` at configure time;
Windows CTest uses `S2_GAME_DIR/game.db` when available. A cross-ARM64
sanitizer run may be invoked with `ASAN_OPTIONS=detect_leaks=0
qemu-aarch64-static -L /usr/aarch64-linux-gnu <arm64-test> <game.db>`.
