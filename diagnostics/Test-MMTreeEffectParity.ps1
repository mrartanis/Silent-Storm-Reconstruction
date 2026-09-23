param(
    [Parameter(Mandatory)][string]$HeadFile,
    [Parameter(Mandatory)][string]$SequenceFile,
    [Parameter(Mandatory)][string]$TreeFile,
    [Parameter(Mandatory)][string]$GameRoot,
    [Parameter(Mandatory)][string]$X86Probe,
    [Parameter(Mandatory)][string]$NativeCurveEvaluate,
    [Parameter(Mandatory)][string]$MacroName,
    [Parameter(Mandatory)][int]$MacroRecordOffset,
    [Parameter(Mandatory)][int]$EffectRecordOffset,
    [Parameter(Mandatory)][int]$MuscleIndex,
    [float]$Expression = 0.5,
    [double]$Tolerance = 0.00001,
    [string]$OutputDirectory
)
$ErrorActionPreference = 'Stop'
$head = (Resolve-Path -LiteralPath $HeadFile).Path
$sequence = (Resolve-Path -LiteralPath $SequenceFile).Path
$tree = (Resolve-Path -LiteralPath $TreeFile).Path
$game = (Resolve-Path -LiteralPath $GameRoot).Path
$x86 = (Resolve-Path -LiteralPath $X86Probe).Path
$native = (Resolve-Path -LiteralPath $NativeCurveEvaluate).Path
if ($MuscleIndex -lt 0 -or $MacroRecordOffset -lt 0 -or $EffectRecordOffset -lt 0) {
    throw 'Offsets and muscle index must be nonnegative'
}
if (!$OutputDirectory) { $OutputDirectory = Join-Path (Split-Path -Parent $head) 'effect-comparison' }
$output = [IO.Path]::GetFullPath($OutputDirectory)
New-Item -ItemType Directory -Path $output -Force | Out-Null
$culture = [Globalization.CultureInfo]::InvariantCulture
$expressionText = $Expression.ToString('G9', $culture)
$prior = @{
    Path = $env:PATH
    Times = $env:S2_FACE_SAMPLE_TIMES
    SelectedTime = $env:S2_FACE_STATE_SNAPSHOT_TIME
    MusclePrefix = $env:S2_FACE_MUSCLE_SNAPSHOT_PREFIX
    ForcedName = $env:S2_FACE_FORCE_MACRO_NAME
    ForcedValue = $env:S2_FACE_FORCE_MACRO_VALUE
}
try {
    $env:PATH = "$game;$($prior.Path)"
    $env:S2_FACE_SAMPLE_TIMES = '0'
    $env:S2_FACE_STATE_SNAPSHOT_TIME = '0'
    $env:S2_FACE_FORCE_MACRO_NAME = $MacroName
    $env:S2_FACE_FORCE_MACRO_VALUE = $expressionText
    foreach ($run in 'first','repeat') {
        $prefix = Join-Path $output "$run-muscles"
        $env:S2_FACE_MUSCLE_SNAPSHOT_PREFIX = $prefix
        & $x86 $head (Join-Path $output "$run-face.csv") $sequence $tree
        if ($LASTEXITCODE -ne 0) { throw "x86 isolated effect probe failed: $run" }
    }
} finally {
    $env:PATH = $prior.Path
    $env:S2_FACE_SAMPLE_TIMES = $prior.Times
    $env:S2_FACE_STATE_SNAPSHOT_TIME = $prior.SelectedTime
    $env:S2_FACE_MUSCLE_SNAPSHOT_PREFIX = $prior.MusclePrefix
    $env:S2_FACE_FORCE_MACRO_NAME = $prior.ForcedName
    $env:S2_FACE_FORCE_MACRO_VALUE = $prior.ForcedValue
}
$first = Join-Path $output 'first-muscles-sequence-0-post-physics.bin'
$repeat = Join-Path $output 'repeat-muscles-sequence-0-post-physics.bin'
$pre = Join-Path $output 'first-muscles-sequence-0-pre.bin'
$before = [IO.File]::ReadAllBytes($pre)
$after = [IO.File]::ReadAllBytes($first)
$afterRepeat = [IO.File]::ReadAllBytes($repeat)
$count = [BitConverter]::ToInt32($after, 0)
if ($count -le $MuscleIndex -or $count -le 0 -or
    $before.Length -ne $after.Length -or $after.Length -ne $afterRepeat.Length -or
    $after.Length -ne 4 + 256 * $count) {
    throw 'Invalid x86 muscle-state snapshot'
}
for ($i = 0; $i -lt $count; ++$i) {
    foreach ($offset in 100,104,108,112,116,120,124,128) {
        $at = 4 + 256 * $i + $offset
        for ($byte = 0; $byte -lt 4; ++$byte) {
            if ($after[$at + $byte] -ne $afterRepeat[$at + $byte]) {
                throw "x86 semantic muscle state is nondeterministic: muscle[$i] offset $offset"
            }
        }
    }
}
$changed = @(for ($i = 0; $i -lt $count; ++$i) {
    $start = 4 + 256 * $i
    for ($j = 0; $j -lt 256; ++$j) {
        if ($before[$start + $j] -ne $after[$start + $j]) { $i; break }
    }
})
if ($changed.Count -ne 1 -or $changed[0] -ne $MuscleIndex) {
    throw "Isolated macro changed unexpected muscles: $($changed -join ',')"
}
$oracle = [BitConverter]::ToSingle($after, 4 + 256 * $MuscleIndex + 124)
$macroLine = @(& $native $tree $MacroRecordOffset $expressionText)
if ($LASTEXITCODE -ne 0 -or $macroLine.Count -ne 1 -or
    $macroLine[0] -notmatch 'value=([^,]+),minimum=') {
    throw 'Native macro-curve evaluation failed'
}
$macroValue = [float]::Parse($Matches[1], $culture)
# A direct AddMacroMuscle call enters the definition's children with the
# original expression. The definition record's own curve is used only when
# that record is traversed as an operation from a parent macro.
$effectLine = @(& $native $tree $EffectRecordOffset $expressionText)
if ($LASTEXITCODE -ne 0 -or $effectLine.Count -ne 1 -or
    $effectLine[0] -notmatch 'value=([^,]+),minimum=') {
    throw 'Native effect-curve evaluation failed'
}
$effectValue = [float]::Parse($Matches[1], $culture)
$delta = [Math]::Abs([double]$oracle - [double]$effectValue)
if ($delta -gt $Tolerance) {
    throw "MMLF effect differs: x86=$oracle native=$effectValue delta=$delta"
}
Write-Output "MMTREE EFFECT PARITY PASS: $MacroName muscle[$MuscleIndex] x86=$oracle native=$effectValue delta=$delta; stored macro curve=$macroValue is not applied on direct call."
