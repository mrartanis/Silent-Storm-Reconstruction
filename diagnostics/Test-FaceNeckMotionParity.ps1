param(
    [Parameter(Mandatory)][string]$FixtureDirectory,
    [Parameter(Mandatory)][string]$GameRoot,
    [Parameter(Mandatory)][string]$X86Probe,
    [Parameter(Mandatory)][string]$X64Probe,
    [Parameter(Mandatory)][string]$OutputDirectory
)
$ErrorActionPreference = 'Stop'
$fixtures = (Resolve-Path -LiteralPath $FixtureDirectory).Path
foreach ($name in 'head-11-0.bin','head-11-1.bin','sequence-6005-0.bin') {
    if (!(Test-Path -LiteralPath (Join-Path $fixtures $name))) {
        throw "Missing neck-motion fixture: $name"
    }
}
$priorTimes = $env:S2_FACE_SAMPLE_TIMES
$priorOverlay = $env:S2_FACE_OVERLAY_SEQUENCE_FILE
$priorOverlay2 = $env:S2_FACE_OVERLAY2_SEQUENCE_FILE
try {
    # 6005's Head_LR runs around 9.3-11.6 s and Head_shake at 14.2-15.2 s.
    # Keep neutral and out-of-interval frames to catch persistent state.
    $env:S2_FACE_SAMPLE_TIMES = '0,500,3500,9500,10000,11000,13000,14200,14500,15000,16000'
    $env:S2_FACE_OVERLAY_SEQUENCE_FILE = $null
    $env:S2_FACE_OVERLAY2_SEQUENCE_FILE = $null
    & "$PSScriptRoot\Test-FaceParity.ps1" -FixtureDirectory $fixtures `
        -GameRoot $GameRoot -X86Probe $X86Probe -X64Probe $X64Probe `
        -OutputDirectory $OutputDirectory
    if (!$?) { throw 'Neck-motion parity failed' }
    Write-Output 'FACE NECK MOTION PARITY PASS: both head-11 segments'
}
finally {
    $env:S2_FACE_SAMPLE_TIMES = $priorTimes
    $env:S2_FACE_OVERLAY_SEQUENCE_FILE = $priorOverlay
    $env:S2_FACE_OVERLAY2_SEQUENCE_FILE = $priorOverlay2
}
