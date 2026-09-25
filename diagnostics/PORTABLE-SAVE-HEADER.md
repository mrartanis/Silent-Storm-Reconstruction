# Portable save header

The retail save header is 256,008 bytes: little-endian 32-bit magic, a
little-endian 32-bit active-mod count, and a 320 × 200 screenshot in BGRA byte
order. `CICLoad`, `CICSave`, and `CSaveManager::GetSlotScreenShot` now use
`FileIO/PortableSaveHeader.h` rather than reading/writing a host
`SSaveFileHeader` image. Negative mod counts and truncated headers are rejected.
The following mod-directory strings and compressed object graph retain their
existing format and are outside this specific change.

`PortableSaveHeaderTests` checks exact bytes, all pixels, round-trip,
truncation, null input, and invalid mod counts. The probe read the existing
`LIGHT_KEY_NEW/game.sav` header with the same result on Windows x86/x64,
Linux x86-64, and ARM64/QEMU:

```
magic=828ca022 mods=0 pixels=64000 fnv64=285f443760998b61
```

The Linux copy was transferred from the same lab save. Windows x64 CTest
passed 80/80; Linux x86-64 and ARM64/QEMU under ASan/UBSan passed 47/47 each
(leak detection disabled under QEMU). The Windows x86 test and probe passed.
Run the probe as `PortableSaveHeaderProbe <path-to-game.sav>`; it reads only
the fixed header and does not inspect mod strings or the remaining save.

These checks establish the header byte contract and architecture-independent
decoded values, not complete save compatibility or gameplay parity. Native
clean-archive load/save and Steam-EXE results are recorded separately once
verified.
