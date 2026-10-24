Silent Storm Reconstruction - engine only

This archive contains the x64 engine, runtime libraries and fonts.
Original Silent Storm v1.2 game data and the optional HD pack are separate.
Detailed Russian installation guide: INSTALLATION_RU.md.

Use a separate copy of your game installation. Place this archive's contents
alongside game.db, start.cfg, tree.mma and the complete cfg, res and scripts
directories. Keep all loose files in res, not just the .res archives.
Use the data for Silent Storm, not the separate Sentinels campaign.
Start Play.cmd (Windows) or ./Play.sh (Linux). Saves and settings use the
engine's normal user-data directory. The original Steam installation can stay
untouched. Install an optional HD pack as res-hd alongside Game/Game.exe.

For multiplayer, keep the included network-version.txt alongside game.db.
No local generation or Python installation is needed. The file declares the
maintained network compatibility version and game-data family. Compatible
engine releases share the same version; incompatible changes require an
explicit version update by maintainers. HD does not change this version.

Linux baseline: Ubuntu 24.04 x86_64 or compatible (glibc 2.39+).
SDL3 and FFmpeg libraries are included in lib. System dependencies include
libgsf-1-114, libpng16-16t64, zlib1g, libx11-6, libxi6 and libgl1, plus a working
Vulkan driver/loader (libvulkan1). Hardware GPU/audio testing is separate from CI.
Windows baseline: Windows 10/11 x64; MSVC runtime DLLs are included.

Source and releases: https://github.com/mrartanis/Silent-Storm-Reconstruction
Engine licensing: LICENSE.md. Component notices: licenses/ and fonts/.
Portions of this software are copyright (c) The FreeType Project
(www.freetype.org). All rights reserved. See fonts/FreeType-FTL.txt.
FFmpeg is built as shared LGPL libraries without GPL/nonfree/autodetected
external components. Exact source, build configuration and recipe are included
under dependencies/. SDL3/bgfx/bimg/bx/miniaudio source references are recorded
in dependencies/sources.json. Microsoft runtime terms:
https://visualstudio.microsoft.com/license-terms/vs2022-cruntime/
