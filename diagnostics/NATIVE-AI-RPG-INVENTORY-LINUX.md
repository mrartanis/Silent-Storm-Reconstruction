# Specialized AI actions and RPG inventory/vision on Linux (stage 2)

The original game source units `aiChoosePlace.cpp`, `aiSnipeAction.cpp`,
`aiLootAction.cpp`, and `aiHeavyGunAction.cpp` now compile in the Linux
`s2_game_ai_actions` library. The source units `RPGItemMap.cpp`,
`RPGMedals.cpp`, `RPGInventory.cpp`, and `RPGVision.cpp` compile in
`s2_game_rpg_inventory_vision`. These are game-used paths: combat logic
constructs the specialized AI actions, RPG units create inventories,
and mission games create a vision tracker. Store uses `CItemsMap`.
The portability edits replace implicit Windows PCH declarations and
case-insensitive include names; no game rules were intentionally changed.

`NativeRPGItemMapTests` executes the original store placement-grid
construction, resize, and clear paths under ASan/UBSan on Linux x86-64
and ARM64/QEMU. It does not exercise item placement or the game's
inventory UI. The complete Windows x64 game builds and its CTest matrix
passes 115/115. Linux x86-64 and ARM64/QEMU pass 89/89 each.

The real `NativeAILogicTests` diagnostic link against all available
Linux game archives still fails: 118 distinct unresolved symbols,
down from 154 before this group. Remaining dependencies are chiefly
world objects, execution/attack helpers, Lua/scenario hooks, animation,
and timers. The Linux game is not executable yet; no symbol stubs or
linker bypass were used.

Behavioral parity remains open. In particular, comments in the
reconstructed `aiSnipeAction.cpp` describe a deferred begin-snipe
decision, and `aiLootAction.cpp` documents omitted retail reachability
and batching behavior. Compilation and a grid test do not resolve these
known differences; compare game decisions with the Steam reference
before claiming AI parity. This batch has no in-game smoke because the
remote Windows D3D session cannot create a device even for an older
known-good archive.
