# Portable Effects.res payloads

The game's `CParticlesLoader` now decodes the on-disk `Effects.res` payload
explicitly rather than overlaying packed track descriptors and key arrays on
host objects. The resource retains its four-byte little-endian payload length,
12-byte header, 34-byte particle descriptors, and five tracks with 32-bit
payload-relative offsets. Decoded runtime keys are owned by `CParticlesInfo`
and naturally aligned. The packed `NGScene::TKey` ABI remains for animated-light
save records; it is no longer used as the runtime effect-key container.

The untouched Steam `Effects.res` has SHA-256
`a78b8f0ce7d38e1612c3c22ccc7d885551cd01beab7d6d5c5968354553d2b81c`.
`PortableEffectCorpusProbe` decoded its entire index and produced the same
semantic aggregate on Windows x86/x64, Linux x86-64, and ARM64/QEMU:

```
effects=280 particles=94612 keys=1917803 fnv64=89e2a0af09d801b5
```

The Linux copy's SHA-256 matches the Steam file. Synthetic valid and malformed
records are covered by `PortableEffectDataTests`. Windows x64 CTest passed
79/79; Linux x86-64 and ARM64/QEMU passed 46/46 each under ASan/UBSan (leak
detection disabled under QEMU). The Windows x86 diagnostic test and corpus
probe also passed. These checks verify the effect data format and parser; they
do not establish visual parity of particles or a playable Linux/ARM64 game.

Clean Windows x64 archive `stage2-effect-data-wire-20260925-01` loaded
`LIGHT_KEY_NEW` to the game loop (`LOAD-DESERIALIZE-COMPLETE`,
`LOAD-SLOT-DONE`) and exited on the harness `quit` command without a crash
dump. The first attempt at this smoke used an empty copied save directory and
failed in `CICLoad`; the complete `game.sav` and `restart.sav` files were then
copied and the run repeated successfully. This checks the integrated Windows
load path, not visual parity of particle effects. SDL3 and bgfx are not
integrated into the game here; those are stages 3 and 4 of the renovation plan.
