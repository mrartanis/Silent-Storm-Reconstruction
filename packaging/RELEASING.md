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

The workflow uses a sparse checkout that excludes `assets/terrain-hd` and
explicitly disables HD building. SDL3, bgfx/bimg/bx, miniaudio and FFmpeg inputs
are pinned in `.github/scripts/prepare-deps.py`; FreeType is pinned in CMake.
FFmpeg is built from the same LGPL source on both platforms without external
codec autodetection. Dependency caches are separate per runner image and keyed
by the download/build recipes. Engine archives use an explicit file allowlist.

No Steam data, historical DLLs, saves, laboratory files or HD textures are
included. `Package-Game.py` remains the separate local full-game packaging tool.
HD is built and uploaded independently; the detailed end-user installation guide
can be maintained separately. Each engine archive already includes a minimal
README and the versioned network-version.txt. Players do not generate this file.

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
