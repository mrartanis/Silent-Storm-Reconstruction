# Original head resources in the stage-2 portable data path

`NativeHeadResourceTests` reads the shipped `Heads.res` through the
engine's `CResourceOpener` and `CStructureSaver`, using the same tags and
field types as `CHeadMeshLoader::Recalc`: animator streams, segment
vertex counts, copied vertices, UVs, 16-bit indices, and triangles.
It then decodes every LifeStudio animator stream with the existing
portable `NativeLifeStudio::DecodeHeadVertices` implementation. The
original package contains 134 head records and 136 animator streams;
all decode. They contain 56,462 vertices and 4,825 muscles. A digest
over resource IDs, topology, UVs, vertex positions, stored muscle
weights, names, and bones is `8A6A6F3A612C18D0` on Windows x64,
Linux x86-64, and ARM64/QEMU. Derived falloff weights are checked
finite/in-range and hashed to 0.001 precision: native ARM64 floating
point differs by a few ULPs from x64, so they are not claimed to be
bit-identical.
The test asserts these values, not merely successful file opening.
The audit also found that the compact saved-head stream omits the
implicit vertex's `type` and `attribute`; the decoder previously left
them uninitialized. They now default to zero, with a focused
`NativeHeadDataTests` assertion.

The test uses the real engine resource/structure loading path and
original data. It does not construct `CHeadInfo`, initialize LifeStudio
animators, transform a head, render a face, or exercise a live game
mission. In particular, the four remaining diagnostic Linux link
symbols (`CNonePart` twice, `CLightGroup`, and `CHeadInfo` construction)
are not resolved by this resource test. The renderer-owned scene-part
classes and head cache must retain their actual behavior when brought
into the portable kernel; a registration-only substitute would not do.

Reproduce on Windows with the configured `G:\SS\lab\build-x64-stage2`:

```
cmake --build G:\SS\lab\build-x64-stage2 --config RelWithDebInfo --target NativeHeadResourceTests --parallel 16
ctest --test-dir G:\SS\lab\build-x64-stage2 -C RelWithDebInfo -R ^NativeHeadResourceTests$ -V
```

On Linux, configure with
`-DS2_HEAD_RESOURCE_PATH=/path/to/original/Heads.res`, build the
`NativeHeadResourceTests` target, then run
`ASAN_OPTIONS=detect_leaks=0 ctest --test-dir build-x64 -R ^NativeHeadResourceTests$ -V`.
The ARM64 configuration uses the same resource and test via QEMU.

Full regression matrix on 2026-09-26: Windows x64 119/119;
Linux x86-64 and ARM64/QEMU 93/93 each under ASan/UBSan.
