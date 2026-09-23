param(
    [Parameter(Mandatory)][string]$X86Stream,
    [Parameter(Mandatory)][string]$X64Stream,
    [Parameter(Mandatory)][string]$SequenceFile,
    [Parameter(Mandatory)][string]$TreeFile,
    [Parameter(Mandatory)][string]$GameRoot,
    [Parameter(Mandatory)][string]$X86Probe,
    [Parameter(Mandatory)][string]$X64Probe,
    [Parameter(Mandatory)][string]$OutputDirectory,
    [string]$SampleTimes = '9500,10000,11000,12000,13000,14500,16000',
    [switch]$UseDefaultTimes,
    [switch]$AllowStatic,
    [switch]$PassThru,
    [double]$Tolerance = 0.0001
)
$ErrorActionPreference = 'Stop'
$x86StreamPath = (Resolve-Path -LiteralPath $X86Stream).Path
$x64StreamPath = (Resolve-Path -LiteralPath $X64Stream).Path
if ([string]::Equals($x86StreamPath, $x64StreamPath, [StringComparison]::OrdinalIgnoreCase)) {
    throw 'Committed-head parity requires separate x86 and x64 saved streams'
}
$sequence = (Resolve-Path -LiteralPath $SequenceFile).Path
$tree = (Resolve-Path -LiteralPath $TreeFile).Path
$game = (Resolve-Path -LiteralPath $GameRoot).Path
$x86 = (Resolve-Path -LiteralPath $X86Probe).Path
$x64 = (Resolve-Path -LiteralPath $X64Probe).Path
$out = [IO.Path]::GetFullPath($OutputDirectory)
New-Item -ItemType Directory -Force -Path $out | Out-Null

$priorPath = $env:PATH
$priorTimes = $env:S2_FACE_SAMPLE_TIMES
try {
    $env:PATH = "$game;$priorPath"
    $env:S2_FACE_SAMPLE_TIMES = if ($UseDefaultTimes) { $null } else { $SampleTimes }
    $left = Join-Path $out 'committed-x86.csv'
    $repeat = Join-Path $out 'committed-x86-repeat.csv'
    $right = Join-Path $out 'committed-x64.csv'
    & $x86 $x86StreamPath $left $sequence $tree
    if ($LASTEXITCODE -ne 0) { throw "x86 FaceProbe failed: $LASTEXITCODE" }
    & $x86 $x86StreamPath $repeat $sequence $tree
    if ($LASTEXITCODE -ne 0) { throw "x86 repeat FaceProbe failed: $LASTEXITCODE" }
    if ((Get-FileHash -LiteralPath $left).Hash -ne (Get-FileHash -LiteralPath $repeat).Hash) {
        throw 'x86 committed-head probe is nondeterministic'
    }
    & $x64 $x64StreamPath $right $sequence $tree
    if ($LASTEXITCODE -ne 0) { throw "x64 FaceProbe failed: $LASTEXITCODE" }
} finally {
    $env:PATH = $priorPath
    $env:S2_FACE_SAMPLE_TIMES = $priorTimes
}

$a = @(Import-Csv -LiteralPath $left)
$b = @(Import-Csv -LiteralPath $right)
if (!$a.Count -or $a.Count -ne $b.Count) { throw "Vertex row count mismatch: x86=$($a.Count) x64=$($b.Count)" }
$culture = [Globalization.CultureInfo]::InvariantCulture
$maximum = 0.0
$moving = 0
$firstByVertex = @{}
for ($i = 0; $i -lt $a.Count; ++$i) {
    $l = $a[$i]; $r = $b[$i]
    if ($l.case -ne $r.case -or $l.time -ne $r.time -or $l.vertex -ne $r.vertex) {
        throw "Vertex row identity mismatch at $i"
    }
    $coordinates = @{}
    foreach ($axis in 'x','y','z') {
        $x = [double]::Parse($l.$axis,$culture)
        $y = [double]::Parse($r.$axis,$culture)
        if ([double]::IsNaN($x) -or [double]::IsInfinity($x) -or
            [double]::IsNaN($y) -or [double]::IsInfinity($y)) {
            throw "Nonfinite committed-head coordinate at $($l.case) time=$($l.time) vertex=$($l.vertex) $axis : x86=$x x64=$y"
        }
        $delta = [Math]::Abs($x - $y)
        if ($delta -gt $maximum) { $maximum = $delta }
        if ($delta -gt $Tolerance) {
            throw "Committed head mismatch at $($l.case) time=$($l.time) vertex=$($l.vertex) $axis delta=$delta"
        }
        $coordinates[$axis] = $x
    }
    if ($l.case -eq 'neutral') {
        $firstByVertex[$l.vertex] = $coordinates
    } elseif ($firstByVertex.ContainsKey($l.vertex)) {
        $base = $firstByVertex[$l.vertex]
        if ([Math]::Abs($coordinates.x - $base.x) -gt 0.0001 -or
            [Math]::Abs($coordinates.y - $base.y) -gt 0.0001 -or
            [Math]::Abs($coordinates.z - $base.z) -gt 0.0001) { $moving++ }
    }
}
if (!$moving -and !$AllowStatic) { throw 'Sampled sequence did not move any x86 committed-head vertex' }
if ($PassThru) {
    [pscustomobject]@{ Rows=$a.Count; MovingRows=$moving; MaxDelta=$maximum }
} else {
    Write-Host "PASS: committed FaceGen head x86/x64 animated vertex parity; $($a.Count) rows, $moving moving rows, max delta $maximum (tolerance $Tolerance)"
}
