# CPU scene/combiner boundary (stage 2)

The original `IPart`, `CPerMaterialCombiner`, and `CAutomaticCombiner`
methods have moved unchanged in behavior from D3D-dependent
`GCombiner.cpp` to `GCombinerCore.cpp`. The Linux port replaces only the
Win32 `Sleep(0)` yield in `RefreshObjectInfo` with `std::this_thread::yield`.
Windows still compiles the same CPU code in `Main`; Linux compiles it into
`s2_game_combiner_core`. Their original class IDs remain `0x02741133` and
`0x01091206`.

The real `CNonePart` class registration now lives in `GScenePartCore.cpp`
on both hosts, with class ID `0x02662160`. No cast function or scene object
was invented. `NativeCombinerCoreTests` exercises adding, sorting, marking,
removing and round-tripping the two combiner classes; the serialized graph
is 144 bytes with FNV `66A3784C237E2756` on Windows x64, Linux x64 and
ARM64. `NativeScenePartCoreTests` round-trips a default `CNonePart`: 90 bytes,
FNV `BE951C13732CB2A0` on the same three targets. Both Linux tests run
under ASan/UBSan.

The full Linux AI/world archive link has now only one distinct unresolved
scene symbol: `CLightGroup`'s object-base cast. That type retains a
concrete `CGScene` and frees its group on destruction. The extraction above
is genuine CPU state and serialization, but it is not a headless mission,
rendering parity, or a claim that every scene type is portable.
