# Original FileIO streams on Linux and ARM64

Stage 2 now builds the game's original `FileIO/Streams.cpp` as
`s2_game_streams` on Linux x86-64 and ARM64, while Windows continues to
build it inside `FileIO`. This is the memory, buffered-file, and bit-stream
layer used by the game's resource and save serializers. GCC-incompatible
in-class template specializations were replaced with equivalent overloads.
The wide-path file opener converts to UTF-8 explicitly on Linux; the Windows
path remains `_wfopen`. `CMemoryStream` growth now saves offsets before
deleting the old allocation.

`NativeStreamsTests` exercises short/long serialized strings, memory growth,
buffered file reads across the 1,024-byte window, and a Cyrillic filename.
It can also read an original file through `CFileStream` and compare all bytes
against independent `std::ifstream` input. The lab used the unmodified
`Fonts.res` (185,840 bytes, SHA-256
`a66f1fe5fd37b05c3436a2630c09d3ba598c928b88bc72e832d8fd984ee494ab`):
Windows x64, Linux x86-64, and ARM64/QEMU all printed FNV-1a 64-bit
`7e3701ab724a0a0f`.

Reproduce the data check after building `NativeStreamsTests`:

```powershell
& 'D:\SS-lab\build-x64-lua-01\RelWithDebInfo\NativeStreamsTests.exe' 'G:\SS\lab\baseline\res\Fonts.res'
```

```sh
ASAN_OPTIONS=detect_leaks=0 BUILD_X64/NativeStreamsTests /path/to/Fonts.res
ASAN_OPTIONS=detect_leaks=0 /usr/bin/qemu-aarch64-static -L /usr/aarch64-linux-gnu BUILD_ARM64/NativeStreamsTests /path/to/Fonts.res
```

The Linux builds used `-fsanitize=address,undefined`. Full CTest results:
Windows x64 84/84 (plus manual four-script Lua autoload), Linux x86-64
52/52, ARM64/QEMU 52/52. This stream-only milestone did not yet port
`CStructureSaver`; the subsequent serializer work is in
`NATIVE-STRUCTURE-LINUX.md`. Package object instantiation and the gameplay
loop are still absent on Linux.
Large-file and arbitrary-seek behavior outside the exercised original data
remains to be audited before declaring the entire FileIO layer portable.

A clean Windows x64 native-media archive
`D:\SS-lab\builds\stage2-native-streams-20260925-01` was built from commit
`266c60e`. Isolated LabRun `D:\SS-lab\runs\stage2-native-streams-01`
loaded mission slot `TOPWRITE_NEW` to `LOAD-SLOT-DONE` and exited with no
crash dump. The copied `game.sav` retained SHA-256
`1385447ae22f6da374f44453bbb93034d99e2e7476e5faa9034d026b4ee3e16a`.
This is a Windows game regression check, not Linux gameplay.
