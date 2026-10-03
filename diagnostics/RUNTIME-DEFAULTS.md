# Default renderer behavior without environment overrides

The normal renderer already enables the production paths when the corresponding
diagnostic environment variables are absent. No launcher flags are required.

| Variable | Behavior without the variable |
| --- | --- |
| `S2_GEOMETRY_TRANSIENT` | Persistent GPU geometry buffers |
| `S2_GEOMETRY_NO_BATCH` | Compatible draw batching enabled |
| `S2_GEOMETRY_SLOW_INDEX_SCAN` | Fast typed index bounds scan |
| `S2_GEOMETRY_FULL_REBUILD` | Partial geometry updates enabled |
| `S2_GEOMETRY_INDEX_KB` | 64 KiB CPU index ring |
| `S2_GEOMETRY_INDEX_NO_APPEND` | Append index snapshots into larger GPU banks |
| `S2_GEOMETRY_INDEX_REWIND_ON_THRASH` | Preserve the index ring during same-frame cache aging |
| `S2_PERF_TRANSIENT_MB` | 64 MiB vertex / 16 MiB index transient budget |
| `S2_LIGHTING_MOVING_SKY_SAMPLES` | Stable sky samples during camera movement and at rest |
| `S2_PERF_PART_TYPES` | Per-part diagnostic logging disabled |
| `S2_TEXTURE_DIAGNOSTICS` | Texture diagnostic accounting disabled |

The latest preview launcher previously cleared these variables; it did not
enable alternative defaults. Those clearing lines have been removed. The larger
transient budget is only an optional stress-test override: production geometry
uses persistent buffers. The tested small CPU ring and GPU append behavior
remain the default. `IndexRingRuntimeTests` now exercises the default ring size
without setting `S2_GEOMETRY_INDEX_KB` in CTest.

`S2_USER_DATA_DIR` remains in the local launcher to select the user's existing
save/config directory. It is a location override, not a rendering switch.
The native cursor and full texture quality are selected by the existing
`ui_hwcursor` / `gfx_texture_streaming` config variables, whose registered
defaults are 1 / 0. No graphics setting defaults were changed in this audit.

Diagnostic overrides remain available for explicit A/B comparisons. Some legacy
switches test presence, so setting them to `0` still activates them; leave them
unset for normal play. The launch command needs only the user-data location and
the usual window/config arguments.
