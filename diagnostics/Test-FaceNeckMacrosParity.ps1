param(
    [Parameter(Mandatory)][string]$FixtureDirectory,
    [Parameter(Mandatory)][string]$GameRoot,
    [Parameter(Mandatory)][string]$X86Probe,
    [Parameter(Mandatory)][string]$X64Probe,
    [Parameter(Mandatory)][string]$OutputDirectory,
    [double]$Tolerance = 0.0001
)
$ErrorActionPreference = 'Stop'
$fixtures = (Resolve-Path -LiteralPath $FixtureDirectory).Path
$game = (Resolve-Path -LiteralPath $GameRoot).Path
$x86 = (Resolve-Path -LiteralPath $X86Probe).Path
$x64 = (Resolve-Path -LiteralPath $X64Probe).Path
$output = [IO.Path]::GetFullPath($OutputDirectory)
New-Item -ItemType Directory -Path $output -Force | Out-Null
$head = Join-Path $fixtures 'head-11-0.bin'
$sequence = Join-Path $fixtures 'sequence-6005-0.bin'
$tree = Join-Path $game 'tree.mma'
foreach ($file in $head,$sequence,$tree) {
    if (!(Test-Path -LiteralPath $file)) { throw "Missing fixture: $file" }
}
$oldPath = $env:PATH
$oldTimes = $env:S2_FACE_SAMPLE_TIMES
$oldMacro = $env:S2_FACE_FORCE_MACRO_NAME
$oldValue = $env:S2_FACE_FORCE_MACRO_VALUE
try {
    $env:PATH = "$game;$oldPath"
    $env:S2_FACE_SAMPLE_TIMES = '0'
    $culture = [Globalization.CultureInfo]::InvariantCulture
    $maximum = 0.0
    $cases = 0
    foreach ($macro in 'HeadPitch','HeadYaw','Head_nod','Head_LR','Head_shake',
                     'NECK_UD','NECK_LR','NECK_ROTATE') {
        foreach ($value in '-1','-0.5','0.5','1') {
            $env:S2_FACE_FORCE_MACRO_NAME = $macro
            $env:S2_FACE_FORCE_MACRO_VALUE = $value
            $tag = "$macro-$($value.Replace('-','m').Replace('.','p'))"
            $leftPath = Join-Path $output "$tag-x86.csv"
            $rightPath = Join-Path $output "$tag-x64.csv"
            & $x86 $head $leftPath $sequence $tree
            if ($LASTEXITCODE -ne 0) { throw "x86 neck macro probe failed: ${tag}" }
            & $x64 $head $rightPath $sequence $tree
            if ($LASTEXITCODE -ne 0) { throw "x64 neck macro probe failed: ${tag}" }
            $left = @(Import-Csv -LiteralPath $leftPath | Where-Object case -eq 'sequence')
            $right = @(Import-Csv -LiteralPath $rightPath | Where-Object case -eq 'sequence')
            if ($left.Count -ne 416 -or $right.Count -ne $left.Count) {
                throw "Unexpected vertex count for ${tag}: x86=$($left.Count) x64=$($right.Count)"
            }
            for ($i = 0; $i -lt $left.Count; ++$i) {
                if ($left[$i].vertex -ne $right[$i].vertex) {
                    throw "Neck macro vertex identity mismatch: ${tag} row $i"
                }
                foreach ($axis in 'x','y','z') {
                    $delta = [Math]::Abs([double]::Parse($left[$i].$axis,$culture) -
                                         [double]::Parse($right[$i].$axis,$culture))
                    $maximum = [Math]::Max($maximum,$delta)
                    if ($delta -gt $Tolerance) {
                        throw "Neck macro mismatch: ${tag} vertex $i axis $axis delta $delta"
                    }
                }
            }
            ++$cases
            Write-Output "$tag pass: $($left.Count) vertices"
        }
    }
    Write-Output "FACE NECK MACRO PARITY PASS: $cases cases, max delta $maximum"
}
finally {
    $env:PATH = $oldPath
    $env:S2_FACE_SAMPLE_TIMES = $oldTimes
    $env:S2_FACE_FORCE_MACRO_NAME = $oldMacro
    $env:S2_FACE_FORCE_MACRO_VALUE = $oldValue
}
