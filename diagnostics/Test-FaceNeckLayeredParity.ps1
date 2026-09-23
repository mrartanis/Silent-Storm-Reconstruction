param(
    [Parameter(Mandatory)][string]$FixtureDirectory,
    [Parameter(Mandatory)][string]$SpeechSequence,
    [Parameter(Mandatory)][string]$ExpressionSequence,
    [Parameter(Mandatory)][string]$GameRoot,
    [Parameter(Mandatory)][string]$X86Probe,
    [Parameter(Mandatory)][string]$X64Probe,
    [Parameter(Mandatory)][string]$OutputDirectory
)
$ErrorActionPreference = 'Stop'
$speech = (Resolve-Path -LiteralPath $SpeechSequence).Path
$expression = (Resolve-Path -LiteralPath $ExpressionSequence).Path
$oldTimes = $env:S2_FACE_SAMPLE_TIMES
$oldOverlay = $env:S2_FACE_OVERLAY_SEQUENCE_FILE
$oldOverlayShift = $env:S2_FACE_OVERLAY_TIME_SHIFT
$oldOverlay2 = $env:S2_FACE_OVERLAY2_SEQUENCE_FILE
$oldOverlay2Shift = $env:S2_FACE_OVERLAY2_TIME_SHIFT
$oldHold = $env:S2_FACE_OVERLAY2_HOLD_LAST
try {
    # At global 14.5 s the primary sequence has Head_shake and Distrust;
    # speech and expression both run at local 400 ms. Later frames exercise
    # the held expression after its serialized 1,002-ms duration.
    $env:S2_FACE_SAMPLE_TIMES = '14000,14200,14500,15000,15100,15500'
    $env:S2_FACE_OVERLAY_SEQUENCE_FILE = $speech
    $env:S2_FACE_OVERLAY_TIME_SHIFT = '-14100'
    $env:S2_FACE_OVERLAY2_SEQUENCE_FILE = $expression
    $env:S2_FACE_OVERLAY2_TIME_SHIFT = '-14100'
    $env:S2_FACE_OVERLAY2_HOLD_LAST = '1'
    & "$PSScriptRoot\Test-FaceParity.ps1" -FixtureDirectory $FixtureDirectory `
        -GameRoot $GameRoot -X86Probe $X86Probe -X64Probe $X64Probe `
        -OutputDirectory $OutputDirectory
    if (!$?) { throw 'Layered neck parity failed' }
    Write-Output 'FACE NECK LAYERED PARITY PASS: head motion + speech + held expression'
}
finally {
    $env:S2_FACE_SAMPLE_TIMES = $oldTimes
    $env:S2_FACE_OVERLAY_SEQUENCE_FILE = $oldOverlay
    $env:S2_FACE_OVERLAY_TIME_SHIFT = $oldOverlayShift
    $env:S2_FACE_OVERLAY2_SEQUENCE_FILE = $oldOverlay2
    $env:S2_FACE_OVERLAY2_TIME_SHIFT = $oldOverlay2Shift
    $env:S2_FACE_OVERLAY2_HOLD_LAST = $oldHold
}
