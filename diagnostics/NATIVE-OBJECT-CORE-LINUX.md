# Native game object ownership on Linux

Stage 2 now compiles the **original** `Misc/Basic2.cpp` and `Misc/EventsBase.cpp`
as `s2_game_objects` on Linux x86-64 and ARM64. These are the engine's actual
`CObjectBase` / `CPtr` / `CObj` / `CMObj` ownership and typed event-dispatch
mechanisms, not separate lookalike data decoders. Windows continues to build
the same sources inside `Misc`. GCC template lookup fixes (`this->` and
`typename`) are shared by both platforms. The Windows-only `IsBadReadPtr`
save-teardown guard is not pretended to exist on Linux; the Linux path
performs normal ownership only.

`NativeObjectCoreTests` checks two-owner release, observer invalidation,
eventual deletion, interface-to-object cross-cast, typed event delivery and
unregistration, and context reset. It uses
the real object code on Windows x64, Linux x86-64 and ARM64/QEMU. On Linux,
both builds use ASan/UBSan; run with:

```sh
cmake --build BUILD_DIR --target NativeObjectCoreTests -j 8
ASAN_OPTIONS=detect_leaks=0 ctest --test-dir BUILD_DIR -R '^NativeObjectCoreTests$' --output-on-failure
```

This is a prerequisite, **not completion** of stage 2. Linux still does not
build or run the gameplay loop, Lua interpreter, AI, combat or native game
database object instantiation. In particular, save-load teardown's dangling
reference workaround remains Windows-only; the root ownership issue must be
fixed or covered independently before Linux game save/load can be claimed.
