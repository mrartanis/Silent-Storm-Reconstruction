param(
    [Parameter(Mandatory)][string]$HeadFile,
    [Parameter(Mandatory)][string]$SequenceFile,
    [Parameter(Mandatory)][string]$TreeFile,
    [Parameter(Mandatory)][string]$GameRoot,
    [Parameter(Mandatory)][string]$X86Probe,
    [Parameter(Mandatory)][string]$X64Probe,
    [Parameter(Mandatory)][string]$NativeHeadDecode,
    [Parameter(Mandatory)][string]$NativeMacroEvaluate,
    [Parameter(Mandatory)][string]$MacroName,
    [float]$Expression = 0.5,
    [double]$Tolerance = 0.00001,
    [double]$AmplitudeDeadband = 0.0001,
    [switch]$RequireVertexParity,
    [string]$OutputDirectory
)
$ErrorActionPreference = 'Stop'
$head = (Resolve-Path -LiteralPath $HeadFile).Path
$sequence = (Resolve-Path -LiteralPath $SequenceFile).Path
$tree = (Resolve-Path -LiteralPath $TreeFile).Path
$game = (Resolve-Path -LiteralPath $GameRoot).Path
$x86 = (Resolve-Path -LiteralPath $X86Probe).Path
$x64 = (Resolve-Path -LiteralPath $X64Probe).Path
$headDecoder = (Resolve-Path -LiteralPath $NativeHeadDecode).Path
$macroEvaluator = (Resolve-Path -LiteralPath $NativeMacroEvaluate).Path
if (!$OutputDirectory) { $OutputDirectory = Join-Path (Split-Path -Parent $head) 'macro-effects-comparison' }
$output = [IO.Path]::GetFullPath($OutputDirectory)
New-Item -ItemType Directory -Path $output -Force | Out-Null
$culture = [Globalization.CultureInfo]::InvariantCulture
$expressionText = $Expression.ToString('G9', $culture)
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
    $env:S2_FACE_SAMPLE_TIMES = '0'
    $env:S2_FACE_STATE_SNAPSHOT_TIME = '0'
    $env:S2_FACE_FORCE_MACRO_NAME = $MacroName
    $env:S2_FACE_FORCE_MACRO_VALUE = $expressionText
    foreach ($run in 'first','repeat') {
        $env:S2_FACE_MUSCLE_SNAPSHOT_PREFIX = Join-Path $output "$run-muscles"
        & $x86 $head (Join-Path $output "$run-face.csv") $sequence $tree
        if ($LASTEXITCODE -ne 0) { throw "x86 macro effect probe failed: $run" }
    }
    $env:S2_FACE_NATIVE_MUSCLE_SNAPSHOT_PREFIX = Join-Path $output 'native-muscles'
    & $x64 $head (Join-Path $output 'native-face.csv') $sequence $tree
    if ($LASTEXITCODE -ne 0) { throw 'x64 native macro effect probe failed' }
} finally {
    $env:PATH = $prior.Path
    $env:S2_FACE_SAMPLE_TIMES = $prior.Times
    $env:S2_FACE_STATE_SNAPSHOT_TIME = $prior.SelectedTime
    $env:S2_FACE_MUSCLE_SNAPSHOT_PREFIX = $prior.MusclePrefix
    $env:S2_FACE_NATIVE_MUSCLE_SNAPSHOT_PREFIX = $prior.NativePrefix
    $env:S2_FACE_FORCE_MACRO_NAME = $prior.ForcedName
    $env:S2_FACE_FORCE_MACRO_VALUE = $prior.ForcedValue
}
$first = [IO.File]::ReadAllBytes((Join-Path $output 'first-muscles-sequence-0-post-physics.bin'))
$repeat = [IO.File]::ReadAllBytes((Join-Path $output 'repeat-muscles-sequence-0-post-physics.bin'))
$count = [BitConverter]::ToInt32($first, 0)
if ($count -le 0 -or $first.Length -ne $repeat.Length -or $first.Length -ne 4 + $count * 256) {
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
$names = @(& $headDecoder $head --muscles | ConvertFrom-Csv)
if ($LASTEXITCODE -ne 0 -or $names.Count -ne $count) {
    throw 'Native head muscle-name decode failed'
}
$byName = [Collections.Generic.Dictionary[string,int]]::new([StringComparer]::Ordinal)
foreach ($row in $names) {
    if (!$byName.TryAdd($row.name, [int]$row.index)) {
        throw "Duplicate head muscle name: $($row.name)"
    }
}
$effects = @(& $macroEvaluator $tree $MacroName $expressionText | ConvertFrom-Csv)
if ($LASTEXITCODE -ne 0) { throw 'Native macro-effect evaluation failed' }
$signedSquares = [double[]]::new($count)
$absoluteSums = [double[]]::new($count)
$expected = [double[]]::new($count)
$matchedEffects = 0
foreach ($effect in $effects) {
    if ([int]$effect.kind -ne 3) { continue }
    $index = 0
    if (!$byName.TryGetValue($effect.target, [ref]$index)) { continue }
    $value = [double]::Parse($effect.value, $culture)
    $signedSquares[$index] += [Math]::Sign($value) * $value * $value
    $absoluteSums[$index] += [Math]::Abs($value)
    ++$matchedEffects
}
if (!$matchedEffects) { throw 'Macro produced no effects targeting this head' }
$nativeState = @(Import-Csv -LiteralPath (Join-Path $output 'native-muscles-sequence-0-post-physics.csv'))
if ($nativeState.Count -ne $count) { throw 'Invalid x64 muscle-state snapshot' }
$maximum = 0.0
$maximumNative = 0.0
$affected = 0
for ($i = 0; $i -lt $count; ++$i) {
    $actual = [BitConverter]::ToSingle($first, 4 + 256 * $i + 124)
    if ($absoluteSums[$i] -ge $AmplitudeDeadband) {
        $expected[$i] = $signedSquares[$i] / $absoluteSums[$i]
    }
    $delta = [Math]::Abs([double]$actual - $expected[$i])
    $maximum = [Math]::Max($maximum, $delta)
    if ($delta -gt $Tolerance) {
        throw "Macro effect mismatch: $MacroName muscle[$i] $($names[$i].name) x86=$actual native=$($expected[$i]) delta=$delta"
    }
    if ([int]$nativeState[$i].muscle -ne $i) { throw "x64 muscle row order differs at $i" }
    $nativeAmplitude = [double]::Parse($nativeState[$i].amplitude, $culture)
    $nativeDelta = [Math]::Abs([double]$actual - $nativeAmplitude)
    $maximumNative = [Math]::Max($maximumNative, $nativeDelta)
    if ($nativeDelta -gt $Tolerance) {
        throw "x64 animator muscle mismatch: $MacroName muscle[$i] x86=$actual x64=$nativeAmplitude delta=$nativeDelta"
    }
    if ([Math]::Abs($actual) -gt $Tolerance) { ++$affected }
}
$x86Vertices = @(Import-Csv -LiteralPath (Join-Path $output 'first-face.csv'))
$x64Vertices = @(Import-Csv -LiteralPath (Join-Path $output 'native-face.csv'))
if ($x86Vertices.Count -ne $x64Vertices.Count) {
    throw "Different forced-macro vertex-row count: x86=$($x86Vertices.Count) x64=$($x64Vertices.Count)"
}
$maximumVertex = 0.0
$vertexMismatches = 0
for ($i = 0; $i -lt $x86Vertices.Count; ++$i) {
    $a = $x86Vertices[$i]
    $b = $x64Vertices[$i]
    if ($a.case -ne $b.case -or $a.time -ne $b.time -or $a.vertex -ne $b.vertex) {
        throw "Different forced-macro vertex identity at row $i"
    }
    foreach ($axis in 'x','y','z') {
        $delta = [Math]::Abs([double]::Parse($a.$axis, $culture) -
                             [double]::Parse($b.$axis, $culture))
        $maximumVertex = [Math]::Max($maximumVertex, $delta)
        if ($delta -gt $Tolerance) { ++$vertexMismatches }
    }
}
if ($RequireVertexParity -and $vertexMismatches) {
    throw "Forced-macro vertex parity failed: $MacroName mismatches=$vertexMismatches max=$maximumVertex"
}
Write-Output "MMTREE MACRO EFFECTS PASS: $MacroName expression=$expressionText, $matchedEffects matched effects, $affected affected muscles, evaluator delta=$maximum, x64 animator delta=$maximumNative; vertices mismatches=$vertexMismatches max=$maximumVertex."
