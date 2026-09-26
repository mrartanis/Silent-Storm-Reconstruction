# Linux save-slot file layer

`Main/iSaveManagerLinux.cpp` ports the original `CSaveManager` profile/slot
operations to the Linux user-data root without moving the resource cwd. The
active profile still comes from `NGlobal::game_profile`; names use the existing
`S2U8:` config encoding and UTF-8 filesystem names. Slots remain flat file
directories under `<user-data>/save/<profile>/<slot>/`. The original file-stream
layer reads and writes the explicit little-endian save header and its 320×200
thumbnail.

`NativeSaveManagerTests` exercises a Unicode profile and slot, header + trailing
payload, save/load, thumbnail, enumeration, deletion, rejected traversal, and
symlink refusal. Test files live only under `S2_USER_DATA_DIR` in the CMake
build tree. The test also verifies that cwd and its resource-side `save/` are
untouched. Run on Linux x64 and ARM64 with:

```
cmake --build <build-dir> --target NativeSaveManagerTests -j 16
ASAN_OPTIONS=detect_leaks=0 ctest --test-dir <build-dir> -R NativeSaveManagerTests --output-on-failure
```

This is the slot *file layer*, not a bootable Linux game or a test of whole
mission-state serialization. The localized reserved-name checks
(`IsValidCustomName`) and alternating quick-save slot selection
(`GetQuickSaveSlot`) still depend on UI database strings and are not linked in
the Linux core target. Linux's slot time uses the host locale and preserves the
retail sort-key arithmetic. Steam cross-save compatibility is not a product
requirement; this test only proves the port's own file operations.
