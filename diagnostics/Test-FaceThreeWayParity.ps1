param(
    [Parameter(Mandatory)][string]$FixtureDirectory,
    [Parameter(Mandatory)][string]$SpeechSequence,
    [Parameter(Mandatory)][string]$ExpressionSequence,
    [Parameter(Mandatory)][string]$GameRoot,
    [Parameter(Mandatory)][string]$X86Probe,
    [Parameter(Mandatory)][string]$X64Probe,
    [Parameter(Mandatory)][string]$OutputDirectory,
    [int]$SpeechStartMs = 1500,
    [int]$Frames = 100,
    [int]$StepMs = 100
)
$ErrorActionPreference = 'Stop'
if ($SpeechStartMs -lt 0 -or $Frames -lt 2 -or $StepMs -lt 1 -or
    [long]($Frames - 1) * $StepMs -gt [int]::MaxValue) {
    throw 'Invalid three-way frame grid'
}
$fixtures = (Resolve-Path -LiteralPath $FixtureDirectory).Path
$speech = (Resolve-Path -LiteralPath $SpeechSequence).Path
$expression = (Resolve-Path -LiteralPath $ExpressionSequence).Path
$heads = @(Get-ChildItem -LiteralPath $fixtures -Filter 'head-*.bin' -File |
    Where-Object Name -Match '^head-\d+-\d+\.bin$')
$primary = @(Get-ChildItem -LiteralPath $fixtures -Filter 'sequence-*.bin' -File |
    Where-Object Name -Match '^sequence-\d+-\d+\.bin$')
if (!$heads.Count -or $primary.Count -ne 1) {
    throw 'Fixture directory must contain heads and exactly one idle sequence'
}
$output = [IO.Path]::GetFullPath($OutputDirectory)
$twoOut = Join-Path $output 'idle-speech'
$threeOut = Join-Path $output 'idle-speech-mask'
$prior = @{
    Times = $env:S2_FACE_SAMPLE_TIMES
    Overlay = $env:S2_FACE_OVERLAY_SEQUENCE_FILE
    Shift = $env:S2_FACE_OVERLAY_TIME_SHIFT
    Overlay2 = $env:S2_FACE_OVERLAY2_SEQUENCE_FILE
    Shift2 = $env:S2_FACE_OVERLAY2_TIME_SHIFT
    Hold2 = $env:S2_FACE_OVERLAY2_HOLD_LAST
}
try {
    $env:S2_FACE_SAMPLE_TIMES = (0..($Frames - 1) | ForEach-Object { $_ * $StepMs }) -join ','
    $env:S2_FACE_OVERLAY_SEQUENCE_FILE = $speech
    $env:S2_FACE_OVERLAY_TIME_SHIFT = [string](-$SpeechStartMs)
    $env:S2_FACE_OVERLAY2_SEQUENCE_FILE = $null
    $env:S2_FACE_OVERLAY2_TIME_SHIFT = $null
    $env:S2_FACE_OVERLAY2_HOLD_LAST = $null
    & "$PSScriptRoot\Test-FaceParity.ps1" -FixtureDirectory $fixtures `
        -GameRoot $GameRoot -X86Probe $X86Probe -X64Probe $X64Probe `
        -OutputDirectory $twoOut
    if (!$?) { throw 'Idle+speech parity failed' }

    $env:S2_FACE_OVERLAY2_SEQUENCE_FILE = $expression
    $env:S2_FACE_OVERLAY2_TIME_SHIFT = [string](-$SpeechStartMs)
    $env:S2_FACE_OVERLAY2_HOLD_LAST = '1'
    & "$PSScriptRoot\Test-FaceParity.ps1" -FixtureDirectory $fixtures `
        -GameRoot $GameRoot -X86Probe $X86Probe -X64Probe $X64Probe `
        -OutputDirectory $threeOut
    if (!$?) { throw 'Idle+speech+mask parity failed' }

    foreach ($head in $heads) {
        $name = "$($head.BaseName)--$($primary[0].BaseName)-x86"
        $twoTrace = @(Import-Csv -LiteralPath (Join-Path $twoOut "$name-muscles.csv"))
        $threeTrace = @(Import-Csv -LiteralPath (Join-Path $threeOut "$name-muscles.csv"))
        if ($threeTrace.Count -le $twoTrace.Count) {
            throw "Mask added no calls: $name"
        }
        $twoTail = @($twoTrace | Where-Object { [int]$_.time -ge ($SpeechStartMs + 1500) }).Count
        $threeTail = @($threeTrace | Where-Object { [int]$_.time -ge ($SpeechStartMs + 1500) }).Count
        if ($threeTail -le $twoTail) {
            throw "Mask tail was not rendered after its duration: $name"
        }
        $twoHash = (Get-FileHash -LiteralPath (Join-Path $twoOut "$name.csv")).Hash
        $threeHash = (Get-FileHash -LiteralPath (Join-Path $threeOut "$name.csv")).Hash
        if ($twoHash -eq $threeHash) { throw "Mask did not change vertices: $name" }
        Write-Output "$name : $($twoTrace.Count) two-layer calls, $($threeTrace.Count) three-layer calls; held tail observed"
    }
    Write-Output "FACE THREE-WAY PARITY PASS: $($heads.Count) heads, $Frames frames"
}
finally {
    $env:S2_FACE_SAMPLE_TIMES = $prior.Times
    $env:S2_FACE_OVERLAY_SEQUENCE_FILE = $prior.Overlay
    $env:S2_FACE_OVERLAY_TIME_SHIFT = $prior.Shift
    $env:S2_FACE_OVERLAY2_SEQUENCE_FILE = $prior.Overlay2
    $env:S2_FACE_OVERLAY2_TIME_SHIFT = $prior.Shift2
    $env:S2_FACE_OVERLAY2_HOLD_LAST = $prior.Hold2
}
