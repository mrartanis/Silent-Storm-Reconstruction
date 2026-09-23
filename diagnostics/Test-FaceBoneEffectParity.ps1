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
    [double]$Tolerance = 0.0001,
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
if (!$OutputDirectory) { $OutputDirectory = Join-Path (Split-Path -Parent $head) 'bone-effect-comparison' }
$output = [IO.Path]::GetFullPath($OutputDirectory)
New-Item -ItemType Directory -Path $output -Force | Out-Null
$culture = [Globalization.CultureInfo]::InvariantCulture
$expressionText = $Expression.ToString('G9', $culture)
$prior = @{
    Path = $env:PATH
    Times = $env:S2_FACE_SAMPLE_TIMES
    SelectedTime = $env:S2_FACE_STATE_SNAPSHOT_TIME
    BonePrefix = $env:S2_FACE_BONE_SNAPSHOT_PREFIX
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
        $env:S2_FACE_BONE_SNAPSHOT_PREFIX = Join-Path $output "$run-bones"
        & $x86 $head (Join-Path $output "$run-face.csv") $sequence $tree
        if ($LASTEXITCODE -ne 0) { throw "x86 bone-effect probe failed: $run" }
    }
    $env:S2_FACE_BONE_SNAPSHOT_PREFIX = $null
    & $x64 $head (Join-Path $output 'native-face.csv') $sequence $tree
    if ($LASTEXITCODE -ne 0) { throw 'x64 bone-effect probe failed' }
} finally {
    $env:PATH = $prior.Path
    $env:S2_FACE_SAMPLE_TIMES = $prior.Times
    $env:S2_FACE_STATE_SNAPSHOT_TIME = $prior.SelectedTime
    $env:S2_FACE_BONE_SNAPSHOT_PREFIX = $prior.BonePrefix
    $env:S2_FACE_FORCE_MACRO_NAME = $prior.ForcedName
    $env:S2_FACE_FORCE_MACRO_VALUE = $prior.ForcedValue
}
$bones = @(& $headDecoder $head --bones | ConvertFrom-Csv |
    Where-Object { $_.matrix -eq 'A' -and $_.element -eq '0' })
if ($LASTEXITCODE -ne 0 -or !$bones.Count) { throw 'Native bone-name decode failed' }
$effects = @(& $macroEvaluator $tree $MacroName $expressionText | ConvertFrom-Csv)
if ($LASTEXITCODE -ne 0) { throw 'Native macro-effect evaluation failed' }
if (@($effects | Where-Object kind -eq '3').Count) {
    throw 'This test requires an isolated bone-only macro'
}
$expectedY = [double[]]::new($bones.Count)
$expectedZ = [double[]]::new($bones.Count)
$signedY = [double[]]::new($bones.Count)
$signedZ = [double[]]::new($bones.Count)
$absoluteY = [double[]]::new($bones.Count)
$absoluteZ = [double[]]::new($bones.Count)
$matched = 0
foreach ($effect in $effects) {
    if ([int]$effect.kind -ne 4) { continue }
    $runtimeType = [int]$effect.'runtime-type'
    if ($runtimeType -notin 1,2 -and [Math]::Abs([double]::Parse($effect.value, $culture)) -gt 0.000001) {
        throw "Unsupported active bone runtime type: $runtimeType"
    }
    foreach ($bone in $bones) {
        if ($effect.target -eq $bone.name) {
            $index = [int]$bone.index
            $value = [double]::Parse($effect.value, $culture)
            if ($runtimeType -eq 2) {
                $signedY[$index] += [Math]::Sign($value) * $value * $value
                $absoluteY[$index] += [Math]::Abs($value)
            } elseif ($runtimeType -eq 1) {
                $signedZ[$index] += [Math]::Sign($value) * $value * $value
                $absoluteZ[$index] += [Math]::Abs($value)
            }
            ++$matched
            break
        }
    }
}
if (!$matched) { throw 'Macro produced no bone effect for this head' }
for ($i = 0; $i -lt $bones.Count; ++$i) {
    if ($absoluteY[$i] -ge 0.0001) { $expectedY[$i] = $signedY[$i] / $absoluteY[$i] }
    if ($absoluteZ[$i] -ge 0.0001) { $expectedZ[$i] = $signedZ[$i] / $absoluteZ[$i] }
}
$first = [IO.File]::ReadAllBytes((Join-Path $output 'first-bones-sequence-0-post-physics.bin'))
$repeat = [IO.File]::ReadAllBytes((Join-Path $output 'repeat-bones-sequence-0-post-physics.bin'))
if ($first.Length -ne 4 + 556 * $bones.Count -or $repeat.Length -ne $first.Length -or
    [BitConverter]::ToInt32($first, 0) -ne $bones.Count) {
    throw 'Invalid x86 bone-state snapshot'
}
$maximumAmplitude = 0.0
for ($i = 0; $i -lt $bones.Count; ++$i) {
    foreach ($channel in 'Y','Z') {
        $offset = 4 + 556 * $i + $(if ($channel -eq 'Y') { 528 } else { 524 })
        for ($byte = 0; $byte -lt 4; ++$byte) {
            if ($first[$offset + $byte] -ne $repeat[$offset + $byte]) {
                throw "x86 bone amplitude is nondeterministic: bone[$i] channel=$channel"
            }
        }
        $actual = [BitConverter]::ToSingle($first, $offset)
        $expected = if ($channel -eq 'Y') { $expectedY[$i] } else { $expectedZ[$i] }
        $delta = [Math]::Abs([double]$actual - $expected)
        $maximumAmplitude = [Math]::Max($maximumAmplitude, $delta)
        if ($delta -gt 0.00001) {
            throw "Bone effect mismatch: bone[$i] channel=$channel x86=$actual native=$expected"
        }
    }
}
$x86Vertices = @(Import-Csv -LiteralPath (Join-Path $output 'first-face.csv'))
$repeatVertices = @(Import-Csv -LiteralPath (Join-Path $output 'repeat-face.csv'))
$x64Vertices = @(Import-Csv -LiteralPath (Join-Path $output 'native-face.csv'))
if ($x86Vertices.Count -ne $x64Vertices.Count -or $x86Vertices.Count -ne $repeatVertices.Count) {
    throw 'Different bone-effect vertex-row counts'
}
$maximumVertex = 0.0
$movingVertices = 0
$neutralByVertex = @{}
foreach ($row in $x86Vertices) {
    if ($row.case -eq 'neutral') { $neutralByVertex[$row.vertex] = $row }
}
for ($i = 0; $i -lt $x86Vertices.Count; ++$i) {
    $a = $x86Vertices[$i]
    $b = $x64Vertices[$i]
    $r = $repeatVertices[$i]
    if ($a.case -ne $b.case -or $a.time -ne $b.time -or $a.vertex -ne $b.vertex -or
        $a.case -ne $r.case -or $a.time -ne $r.time -or $a.vertex -ne $r.vertex) {
        throw "Bone-effect vertex identity differs at row $i"
    }
    foreach ($axis in 'x','y','z') {
        if ($a.$axis -ne $r.$axis) { throw "x86 bone-effect vertex is nondeterministic: row $i" }
        $delta = [Math]::Abs([double]::Parse($a.$axis, $culture) -
                             [double]::Parse($b.$axis, $culture))
        $maximumVertex = [Math]::Max($maximumVertex, $delta)
        if ($delta -gt $Tolerance) {
            throw "Bone-effect vertex mismatch: vertex=$($a.vertex) axis=$axis delta=$delta"
        }
        if ($a.case -eq 'sequence' -and $neutralByVertex.ContainsKey($a.vertex)) {
            $neutral = $neutralByVertex[$a.vertex]
            if ([Math]::Abs([double]::Parse($a.$axis, $culture) -
                            [double]::Parse($neutral.$axis, $culture)) -gt $Tolerance) {
                ++$movingVertices
            }
        }
    }
}
if (!$movingVertices) { throw 'Bone effect produced no moving head vertex; geometry parity is vacuous' }
Write-Output "FACE BONE EFFECT PARITY PASS: $MacroName expression=$expressionText, matched effects=$matched, moving components=$movingVertices, amplitude delta=$maximumAmplitude, vertex delta=$maximumVertex."
