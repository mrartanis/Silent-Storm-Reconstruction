# Portable top-level structure writer

`CStructureSaver::Finish` previously wrote the top-level chunk-length word,
format version, and object-table entries by copying host integers/bools into
the save stream. It now uses explicit little-endian encoders from
`PortableStructureChunks` for the 1/4-byte length, 4-byte version, and
9-byte object record (type ID, 32-bit wire ID, validity byte). The reader
decodes the version explicitly as well. The disk format is unchanged.

`PortableStructureChunkTests` checks short/extended length boundaries,
known bytes, invalid lengths, and an exact 9-byte object record. Windows x64
Game compiled and CTest passed 80/80. Linux x86-64 and ARM64/QEMU built the
portable modules and passed 47/47 each under ASan/UBSan (`detect_leaks=0`);
the Windows x86 diagnostic test also passed. These are format checks, not a
claim that the full game builds on Linux/ARM64.

Clean Windows x64 archive `stage2-structure-topwrite-20260925-01` loaded the
prior `SAVE_HEADER_NEW` mission, wrote `TOPWRITE_NEW`, reloaded it to
`LOAD-SLOT-DONE`, and exited without a dump. The isolated original Steam EXE
(SHA-256 `4f417593a9f73e2bfde12d83cdbd694ae47eecb92812140d67b8b343e4a95705`)
showed `TOPWRITE_NEW` first in the load menu and opened it to the mission
screen with the character, motorcycles, and building. Evidence:
`D:\SS-lab\steam-ui-oracle-01\screen-topwrite-new.png`. Source and copied
`game.sav` retained SHA-256
`1385447ae22f6da374f44453bbb93034d99e2e7476e5faa9034d026b4ee3e16a`;
`Test-SteamUnchanged.ps1` passed 2,698 installed files.

Other nested fields are audited separately; this specific save compatibility
does not declare the whole object graph portable or prove gameplay parity.
