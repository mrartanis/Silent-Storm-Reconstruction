param(
    [Parameter(Mandatory)][string]$X86Stream,
    [Parameter(Mandatory)][string]$X64Stream,
    [Parameter(Mandatory)][string]$HeadMotionSequence,
    [Parameter(Mandatory)][string]$SpeechSequence,
    [Parameter(Mandatory)][string]$ExpressionSequence,
    [Parameter(Mandatory)][string]$TreeFile,
    [Parameter(Mandatory)][string]$GameRoot,
    [Parameter(Mandatory)][string]$X86Probe,
    [Parameter(Mandatory)][string]$X64Probe,
    [Parameter(Mandatory)][string]$OutputDirectory,
    [string]$SampleTimes = '14000,14200,14500,15000,15100,15500',
    [double]$Tolerance = 0.0001
)
$ErrorActionPreference = 'Stop'
$speech = (Resolve-Path -LiteralPath $SpeechSequence).Path
$expression = (Resolve-Path -LiteralPath $ExpressionSequence).Path
$oldOverlay = $env:S2_FACE_OVERLAY_SEQUENCE_FILE
$oldOverlayShift = $env:S2_FACE_OVERLAY_TIME_SHIFT
$oldOverlay2 = $env:S2_FACE_OVERLAY2_SEQUENCE_FILE
$oldOverlay2Shift = $env:S2_FACE_OVERLAY2_TIME_SHIFT
$oldHold = $env:S2_FACE_OVERLAY2_HOLD_LAST
try {
    $env:S2_FACE_OVERLAY_SEQUENCE_FILE = $speech
    $env:S2_FACE_OVERLAY_TIME_SHIFT = '-14100'
    $env:S2_FACE_OVERLAY2_SEQUENCE_FILE = $expression
    $env:S2_FACE_OVERLAY2_TIME_SHIFT = '-14100'
    $env:S2_FACE_OVERLAY2_HOLD_LAST = '1'
    & (Join-Path $PSScriptRoot 'Test-FaceGenCommittedMotionParity.ps1') `
        -X86Stream $X86Stream -X64Stream $X64Stream -SequenceFile $HeadMotionSequence `
        -TreeFile $TreeFile -GameRoot $GameRoot -X86Probe $X86Probe -X64Probe $X64Probe `
        -OutputDirectory $OutputDirectory -SampleTimes $SampleTimes -Tolerance $Tolerance
} finally {
    $env:S2_FACE_OVERLAY_SEQUENCE_FILE = $oldOverlay
    $env:S2_FACE_OVERLAY_TIME_SHIFT = $oldOverlayShift
    $env:S2_FACE_OVERLAY2_SEQUENCE_FILE = $oldOverlay2
    $env:S2_FACE_OVERLAY2_TIME_SHIFT = $oldOverlay2Shift
    $env:S2_FACE_OVERLAY2_HOLD_LAST = $oldHold
}
