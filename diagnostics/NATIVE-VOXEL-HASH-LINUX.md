# Blast-object key hash width (stage 2)

The game-used `NAI::SVoxelObjectHash` in `aiVoxelRender.h` indexed
`CExplVoxelRenderer::CObjectsHash` by casting an object pointer through
`int`. On x64 this discarded the upper 32 address bits. The map holds
objects hit by the voxel explosion wave, and `wExplTracker.cpp` later
iterates it while processing blast results. Equality still compared the
full pointers, so the truncation caused collisions rather than making
different objects equal, but it is a real pointer-width dependency in
the gameplay path.

`HashVoxelObjectKey` now returns `size_t` and uses the full
`uintptr_t` address plus the 32-bit user ID. On x86 its bit formula
matches the former low-word XOR. `PortableVoxelObjectHashTests` checks
that contract and, on 64-bit hosts, checks that equal low halves with
different high address bits produce different hashes. The actual
`SVoxelObjectHash` delegates to this helper. No on-disk fields or key
equality were changed.
The focused test also ran on the auxiliary restored Windows x86 build:
both x86 and x64 returned low-word hash `7EDCBA88` for the fixed key.
The original Steam x86 executable was not instrumented for this hash,
so this is source-formula parity, not a direct Steam runtime trace.

Because changing a hash can change unordered-map iteration order,
this patch alone does not establish parity of damage ordering or chain
reactions with the Steam x86 game. The current focused test is about
pointer-width correctness; a live explosion/damage comparison is still
required by stage 2. No claim about a Linux mission follows from this.

Reproduce the focused test after configuring the repository:

```
ctest --test-dir G:\SS\lab\build-x64-stage2 -C RelWithDebInfo -R ^PortableVoxelObjectHashTests$ -V
ASAN_OPTIONS=detect_leaks=0 ctest --test-dir build-x64 -R ^PortableVoxelObjectHashTests$ -V
ASAN_OPTIONS=detect_leaks=0 ctest --test-dir build-arm64 -R ^PortableVoxelObjectHashTests$ -V
```

Full regression matrix on 2026-09-26: Windows x64 120/120;
Linux x86-64 and ARM64/QEMU 94/94 each under ASan/UBSan.

Clean native-media Windows x64 archive from source commit `d9599bd`:
`G:\SS\lab\builds\stage2-voxel-object-hash-20260926-01`.
`Game.exe` SHA-256 is
`A7D957E162700D1CDFB2EA8571AD0E55122AAE08331CE23EA544C461B573D8EA`.
The archive has no `fmod.dll` file, and the executable has no
`fmod.dll` or `FSOUND_` imports. No live-game explosion smoke is
claimed from this archive.
