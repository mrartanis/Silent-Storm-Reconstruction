# Game lock tokens on portable core (stage 2)

`CLockObject` is not an editor-only type. `CLockable` occurs in the
game's inventory-item, stationary-weapon and unit-server state, and the
lock token has original save type ID `0x23065400`. Windows already
compiled `Main/Locks.cpp`; Linux did not, so its static save-class
registration was absent. The original source now compiles in the Linux
portable target, and the headless world probe links it with whole-archive
semantics so registration is retained even when a particular scenario
does not create a live lock.

`NativeLockTokenTests` checks registration, first acquisition, re-acquisition
by the same owner, denial of another owner, then serializes and reads the
real `CLockable` token/owner object graph with `CStructureSaver`. After
loading, ownership and exclusion still hold. The identical 70-byte wire
record and FNV digest `5A6C2F212363E2E7` were obtained on diagnostic
Windows x86, target Windows x64, Linux GCC/Clang x86-64 and ARM64/QEMU.
GCC x86-64 ran under ASan/UBSan/LSan; ARM64/QEMU under ASan/UBSan.
The full Windows x64 CTest passed 168/168; ordinary Linux GCC x86-64
CTest passed 137/137 under sanitizers.

Build `NativeLockTokenTests` and run its registered CTest case; it does
not require original resource files. This tests the lock-token contract
and save registration, not every inventory or cannon command that uses
the lock or a live lock inside a mission. Windows x86 is diagnostic only.
