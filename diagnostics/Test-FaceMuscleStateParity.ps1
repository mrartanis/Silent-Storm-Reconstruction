param(
    [Parameter(Mandatory)][string]$HeadFile,
    [Parameter(Mandatory)][string]$SequenceFile,
    [Parameter(Mandatory)][string]$TreeFile,
    [Parameter(Mandatory)][string]$GameRoot,
    [Parameter(Mandatory)][string]$X86Probe,
    [Parameter(Mandatory)][string]$X64Probe,
    [Parameter(Mandatory)][int]$Time,
    [double]$Tolerance = 0.00001,
    [string]$OutputDirectory
)
$ErrorActionPreference = 'Stop'
if ($Time -lt 0) { throw 'Time must be nonnegative' }
$head = (Resolve-Path -LiteralPath $HeadFile).Path
$sequence = (Resolve-Path -LiteralPath $SequenceFile).Path
$tree = (Resolve-Path -LiteralPath $TreeFile).Path
$game = (Resolve-Path -LiteralPath $GameRoot).Path
$x86 = (Resolve-Path -LiteralPath $X86Probe).Path
$x64 = (Resolve-Path -LiteralPath $X64Probe).Path
if (!$OutputDirectory) { $OutputDirectory = Join-Path (Split-Path -Parent $head) 'muscle-state-comparison' }
$output = [IO.Path]::GetFullPath($OutputDirectory)
New-Item -ItemType Directory -Path $output -Force | Out-Null
$prior = @{
    Path = $env:PATH
    Times = $env:S2_FACE_SAMPLE_TIMES
    SelectedTime = $env:S2_FACE_STATE_SNAPSHOT_TIME
    MusclePrefix = $env:S2_FACE_MUSCLE_SNAPSHOT_PREFIX
    NativePrefix = $env:S2_FACE_NATIVE_MUSCLE_SNAPSHOT_PREFIX
    ForcedName = $env:S2_FACE_FORCE_MACRO_NAME
    ForcedValue = $env:S2_FACE_FORCE_MACRO_VALUE
}
try {
    $env:PATH = "$game;$($prior.Path)"
    $env:S2_FACE_SAMPLE_TIMES = [string]$Time
    $env:S2_FACE_STATE_SNAPSHOT_TIME = [string]$Time
    $env:S2_FACE_FORCE_MACRO_NAME = $null
    $env:S2_FACE_FORCE_MACRO_VALUE = $null
    foreach ($run in 'first','repeat') {
        $env:S2_FACE_MUSCLE_SNAPSHOT_PREFIX = Join-Path $output "$run-muscles"
        & $x86 $head (Join-Path $output "$run-face.csv") $sequence $tree
        if ($LASTEXITCODE -ne 0) { throw "x86 muscle-state probe failed: $run" }
    }
    $env:S2_FACE_NATIVE_MUSCLE_SNAPSHOT_PREFIX = Join-Path $output 'native-muscles'
    & $x64 $head (Join-Path $output 'native-face.csv') $sequence $tree
    if ($LASTEXITCODE -ne 0) { throw 'x64 muscle-state probe failed' }
} finally {
    $env:PATH = $prior.Path
    $env:S2_FACE_SAMPLE_TIMES = $prior.Times
    $env:S2_FACE_STATE_SNAPSHOT_TIME = $prior.SelectedTime
    $env:S2_FACE_MUSCLE_SNAPSHOT_PREFIX = $prior.MusclePrefix
    $env:S2_FACE_NATIVE_MUSCLE_SNAPSHOT_PREFIX = $prior.NativePrefix
    $env:S2_FACE_FORCE_MACRO_NAME = $prior.ForcedName
    $env:S2_FACE_FORCE_MACRO_VALUE = $prior.ForcedValue
}
$first = [IO.File]::ReadAllBytes((Join-Path $output "first-muscles-sequence-$Time-post-physics.bin"))
$repeat = [IO.File]::ReadAllBytes((Join-Path $output "repeat-muscles-sequence-$Time-post-physics.bin"))
$count = [BitConverter]::ToInt32($first, 0)
if ($count -le 0 -or $first.Length -ne $repeat.Length -or $first.Length -ne 4 + 256 * $count) {
    throw 'Invalid x86 muscle-state snapshot'
}
for ($i = 0; $i -lt $count; ++$i) {
    foreach ($offset in 100,104,108,112,116,120,124,128) {
        $at = 4 + 256 * $i + $offset
        for ($byte = 0; $byte -lt 4; ++$byte) {
            if ($first[$at + $byte] -ne $repeat[$at + $byte]) {
                throw "x86 semantic muscle state is nondeterministic: muscle[$i] offset $offset"
            }
        }
    }
}
$native = @(Import-Csv -LiteralPath (Join-Path $output "native-muscles-sequence-$Time-post-physics.csv"))
if ($native.Count -ne $count) { throw 'Different native muscle count' }
$culture = [Globalization.CultureInfo]::InvariantCulture
$maximum = 0.0
$active = 0
for ($i = 0; $i -lt $count; ++$i) {
    if ([int]$native[$i].muscle -ne $i) { throw "Native muscle row order differs at $i" }
    $actual = [BitConverter]::ToSingle($first, 4 + 256 * $i + 124)
    $candidate = [double]::Parse($native[$i].amplitude, $culture)
    $delta = [Math]::Abs([double]$actual - $candidate)
    $maximum = [Math]::Max($maximum, $delta)
    if ($delta -gt $Tolerance) {
        throw "Muscle-state mismatch at muscle[$i]: x86=$actual x64=$candidate delta=$delta"
    }
    if ([Math]::Abs($actual) -gt $Tolerance) { ++$active }
}
if (!$active) { throw 'Selected frame has no active head muscle; choose another time or sequence' }
Write-Output "FACE MUSCLE STATE PARITY PASS: time=$Time, muscles=$count, active=$active, max delta=$maximum; animated vertex parity not implied."
