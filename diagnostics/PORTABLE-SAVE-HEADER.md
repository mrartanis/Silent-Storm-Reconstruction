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

Clean Windows x64 archive `stage2-save-header-wire-20260925-01` loaded the old
`LIGHT_KEY_NEW` slot to the game loop, saved `SAVE_HEADER_NEW` (5,601,310-byte
`game.sav`), decoded its new header (`magic=828ca022`, `mods=0`, 64,000 pixels),
loaded that slot again (`LOAD-DESERIALIZE-COMPLETE`, `LOAD-SLOT-DONE`), and
exited through the harness without a crash dump. The new screenshot pixel
hash was `865ae361a188f494`. Commands were delivered to the isolated LabRun's
`game/_harness_cmd.txt` one at a time. Write a temporary file and rename it
atomically to `_harness_cmd.txt`: direct creation can race the polling harness
and produce an empty, silently consumed command.

These checks establish the header byte contract, architecture-independent
decoded values, and one native Windows load/save path. They do not prove full
save compatibility with the original Steam EXE or gameplay parity; direct
Steam loading of this new slot remains to be checked.
