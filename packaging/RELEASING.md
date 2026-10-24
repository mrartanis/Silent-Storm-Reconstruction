# Engine releases

Push a tag beginning with `v` to `mrartanis/Silent-Storm-Reconstruction`.
The `Engine release` workflow builds and tests Windows x64 and Linux x64,
then publishes both archives and `SHA256SUMS` as a GitHub Release.
Tags with a hyphen, such as `v0.1.0-rc1`, produce prereleases.
Use a new tag for each release; published assets are never overwritten.

```powershell
git tag -a v0.1.0 -m 'Engine v0.1.0'
git push origin v0.1.0
```

Create the tag on a commit containing `.github/workflows/engine-release.yml`.
GitHub Actions must have permission to create releases (`contents: write` is
requested only by the publication job). No personal token or Steam account is
needed. A failed build/test prevents publication. Failed uploads leave a draft
that the same workflow can recover on rerun; an existing published release is
left unchanged.

For a check without publishing, select **Actions → Engine release → Run
workflow** on the branch to test. Its archives remain downloadable as workflow
artifacts for seven days. A manual run never publishes a release, even on a tag.
The manual `platform` choice can limit a check to Windows or Linux; tag pushes
always build and require both platforms.

The workflow uses a sparse checkout that excludes `assets/terrain-hd` and
explicitly disables HD building. SDL3, bgfx/bimg/bx, miniaudio and FFmpeg inputs
are pinned in `.github/scripts/prepare-deps.py`; FreeType is pinned in CMake.
FFmpeg is built from the same LGPL source on both platforms without external
codec autodetection. Dependency caches are separate per runner image and keyed
by the download/build recipes. Engine archives use an explicit file allowlist.

No Steam data, historical DLLs, saves, laboratory files or HD textures are
included. `Package-Game.py` remains the separate local full-game packaging tool.
HD is built and uploaded independently. The end-user guide is
[INSTALLATION_RU.md](../INSTALLATION_RU.md); each engine archive also includes it,
a minimal README and network-version.txt. Players do not generate this file.

## Network compatibility

`network-version.txt` in the repository is the source of truth. Its three lines
are the format marker (`S2-NETWORK-1`), engine compatibility version (currently
`1`) and data-family version (`silent-storm-1.2`). CMake copies this exact file
into both platform builds and embeds the compatibility version in the engine.
Neither Git commits, compiler/OS differences nor a player's installed files
change the version automatically. Ordinary compatible changes keep it intact.

Increment the second line when changing wire formats, replicated state or other
network behavior that cannot interoperate with the previous version. Change the
third line when deliberately changing the compatible gameplay-data family.
Mod authors must maintain their own compatible manifests; installed data is no
longer hashed automatically. Keep the file beside game.db when installing.

The Linux archive bundles SDL3/FFmpeg; system libraries and Vulkan drivers are
provided by the operating system. It targets the Ubuntu 24.04/glibc 2.39 baseline.
CI graphics checks use software Vulkan; they do not establish hardware GPU or
audio-device compatibility.

## Separate HD release

Use a tag beginning with `hd-`, for example `hd-v0.1`; it does not match the
engine workflow's `v*` trigger. Build the native runtime pack locally from the
approved PNGs, with Python 3.11+ and Pillow. Use a new staging directory outside
the checkout. From the checkout, for example:

```powershell
python assets/terrain-hd/build_pack.py --output ../hd-release-stage/res-hd
python assets/terrain-hd/validate_pack.py --pack ../hd-release-stage/res-hd --no-update-queue
Copy-Item INSTALLATION_RU.md ../hd-release-stage/INSTALLATION_RU.md
Copy-Item LICENSE.md ../hd-release-stage/HD-LICENSE.md
Set-Location ../hd-release-stage
```

`--no-update-queue` keeps release verification from rewriting authoring metadata.
Ship only the native `Textures*.res`, `manifest.json`, instructions and notices.
Do not ship the build-state cache, PNG source tree, original game data or saves.

For v0.1, five configurations were compared on the same 220,204,118-byte sample.
ZIP used 90,874,926 bytes; Delta:4 + LZMA2 used 58,864,122 bytes, about 35% less.
Plain LZMA2 and a four-byte literal-position model produced larger results.
The complete release uses a 1 GiB dictionary, solid mode, 273 fast bytes, BT4
and 1,024 match-finder cycles. Two compression threads retain one continuous
LZMA2 stream; increasing the thread count can split it into independent chunks
and change the compression ratio. On the 32 GiB build machine this setting
uses roughly 11 GiB for compression. Extraction needs roughly 1 GiB for the
dictionary. Compression is substantially slower than ZIP.

With the 7-Zip executable on PATH, run from the staging directory:

```powershell
7z a -t7z -mx=9 -m0=Delta:4 -m1=LZMA2:d=1024m:fb=273:mf=bt4:mc=1024 -ms=on -mmt=2 -mtm=off -v1900m silent-storm-hd-vNEXT.7z 'res-hd/*.res' res-hd/manifest.json INSTALLATION_RU.md HD-LICENSE.md
7z t silent-storm-hd-vNEXT.7z.001
7z x silent-storm-hd-vNEXT.7z.001 -overify
```

Use a fresh archive name. The release's volumes are at most 1,900 MiB each,
below GitHub's 2 GiB per-asset limit. Users download all volumes and extract
only `.7z.001`. These are parts of one archive, not independent archives.
Compare every extracted `.res` SHA-256 with the native manifest before upload:

```powershell
$hdManifest = Get-Content res-hd/manifest.json -Raw | ConvertFrom-Json
foreach ($hdArchive in $hdManifest.archives) {
  $hdHash = (Get-FileHash (Join-Path verify/res-hd $hdArchive.file) -Algorithm SHA256).Hash
  if ($hdHash -ne $hdArchive.sha256) { throw "HD payload differs: $($hdArchive.file)" }
}
```

Record the source commit, manifest checksum, compression options, volume names,
sizes and SHA-256 in a release index, with `SHA256SUMS` for downloaded assets.
Use the v0.1 release's `hd-v0.1-index.json` as an example. Create an HD release
as a draft, upload and verify every asset's remote size and SHA-256, then publish
with `--latest=false` so the engine remains the main latest release. Explicitly
pass `--repo mrartanis/Silent-Storm-Reconstruction` to release commands.

Attach the current `INSTALLATION_RU.md` to both releases. If packaging changes
after an engine tag, keep the tag and its binaries intact, explain the change
in release notes and point to the attached updated guide. The HD archive should
also include that updated guide.
