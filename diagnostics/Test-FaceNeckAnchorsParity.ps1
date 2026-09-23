param(
    [Parameter(Mandatory)][string]$FixtureDirectory,
    [Parameter(Mandatory)][string]$GameRoot,
    [Parameter(Mandatory)][string]$X86Probe,
    [Parameter(Mandatory)][string]$X64Probe,
    [Parameter(Mandatory)][string]$NativeHeadDecode,
    [Parameter(Mandatory)][string]$OutputDirectory,
    [double]$Tolerance = 0.0001
)
$ErrorActionPreference = 'Stop'
$fixtures = (Resolve-Path -LiteralPath $FixtureDirectory).Path
$game = (Resolve-Path -LiteralPath $GameRoot).Path
$x86 = (Resolve-Path -LiteralPath $X86Probe).Path
$x64 = (Resolve-Path -LiteralPath $X64Probe).Path
$decoder = (Resolve-Path -LiteralPath $NativeHeadDecode).Path
$output = [IO.Path]::GetFullPath($OutputDirectory)
New-Item -ItemType Directory -Path $output -Force | Out-Null
$head = Join-Path $fixtures 'head-11-0.bin'
$sequence = Join-Path $fixtures 'sequence-6005-0.bin'
$tree = Join-Path $game 'tree.mma'
foreach ($file in $head,$sequence,$tree) {
    if (!(Test-Path -LiteralPath $file)) { throw "Missing fixture: $file" }
}
$metadata = @(& $decoder $head --neck | ConvertFrom-Csv)
if ($LASTEXITCODE -ne 0 -or $metadata.Count -eq 0) { throw 'Cannot decode neck metadata' }
$lower = @($metadata | Select-Object -ExpandProperty lower -Unique)
if ($lower.Count -ne 8) { throw "Expected eight distinct lower neck landmarks, got $($lower.Count)" }
$lowerSet = [Collections.Generic.HashSet[int]]::new()
foreach ($id in $lower) { [void]$lowerSet.Add([int]$id) }
$oldPath = $env:PATH
$oldTimes = $env:S2_FACE_SAMPLE_TIMES
$oldOverlay = $env:S2_FACE_OVERLAY_SEQUENCE_FILE
$oldOverlay2 = $env:S2_FACE_OVERLAY2_SEQUENCE_FILE
try {
    $env:PATH = "$game;$oldPath"
    $env:S2_FACE_SAMPLE_TIMES = '0,500,3500,9500,10000,11000,13000,14200,14500,15000,16000'
    $env:S2_FACE_OVERLAY_SEQUENCE_FILE = $null
    $env:S2_FACE_OVERLAY2_SEQUENCE_FILE = $null
    $leftPath = Join-Path $output 'head-11-neck-anchors-x86.csv'
    $rightPath = Join-Path $output 'head-11-neck-anchors-x64.csv'
    & $x86 $head $leftPath $sequence $tree
    if ($LASTEXITCODE -ne 0) { throw 'x86 neck anchor probe failed' }
    & $x64 $head $rightPath $sequence $tree
    if ($LASTEXITCODE -ne 0) { throw 'x64 neck anchor probe failed' }
    $left = @(Import-Csv -LiteralPath $leftPath | Where-Object {
        $_.case -eq 'sequence' -and $lowerSet.Contains([int]$_.vertex)
    })
    $right = @(Import-Csv -LiteralPath $rightPath | Where-Object {
        $_.case -eq 'sequence' -and $lowerSet.Contains([int]$_.vertex)
    })
    if ($left.Count -ne 11 * 8 -or $right.Count -ne $left.Count) {
        throw "Unexpected neck anchor row counts: x86=$($left.Count) x64=$($right.Count)"
    }
    $culture = [Globalization.CultureInfo]::InvariantCulture
    $maximum = 0.0
    for ($i = 0; $i -lt $left.Count; ++$i) {
        if ($left[$i].time -ne $right[$i].time -or
            $left[$i].vertex -ne $right[$i].vertex) {
            throw "Neck anchor identity mismatch at row $i"
        }
        foreach ($axis in 'x','y','z') {
            $delta = [Math]::Abs([double]::Parse($left[$i].$axis,$culture) -
                                 [double]::Parse($right[$i].$axis,$culture))
            $maximum = [Math]::Max($maximum,$delta)
            if ($delta -gt $Tolerance) {
                throw "Neck anchor mismatch at time $($left[$i].time) vertex $($left[$i].vertex) axis $axis delta $delta"
            }
        }
    }
    Write-Output "FACE NECK ANCHOR PARITY PASS: $($left.Count) rows, max delta $maximum"
}
finally {
    $env:PATH = $oldPath
    $env:S2_FACE_SAMPLE_TIMES = $oldTimes
    $env:S2_FACE_OVERLAY_SEQUENCE_FILE = $oldOverlay
    $env:S2_FACE_OVERLAY2_SEQUENCE_FILE = $oldOverlay2
}
