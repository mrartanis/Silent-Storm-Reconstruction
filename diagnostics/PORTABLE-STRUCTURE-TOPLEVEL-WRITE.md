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

The clean native save/reload and original Steam read are recorded below once
verified. Other nested fields are audited separately; this change does not
declare the whole object graph portable.
