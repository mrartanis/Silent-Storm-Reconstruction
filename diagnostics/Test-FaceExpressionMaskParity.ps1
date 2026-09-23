param(
    [Parameter(Mandatory)][string]$FixtureDirectory,
    [Parameter(Mandatory)][string]$SequenceDirectory,
    [Parameter(Mandatory)][string]$GameRoot,
    [Parameter(Mandatory)][string]$X86Probe,
    [Parameter(Mandatory)][string]$X64Probe,
    [Parameter(Mandatory)][string]$OutputDirectory
)
$ErrorActionPreference = 'Stop'
$sequences = (Resolve-Path -LiteralPath $SequenceDirectory).Path
$output = [IO.Path]::GetFullPath($OutputDirectory)
# These are the non-neutral FaceExpression2Sequences mappings returned by the
# baseline game.db through the game's GetSequenceByExpression lookup.
$masks = [ordered]@{
    Smile = 6010
    Anger = 7548
    Worry = 7550
    Fear = 7551
    Sad = 7552
    Happy = 7553
    Smirk = 7554
    Disgust = 7555
}
foreach ($kind in $masks.Keys) {
    $file = Join-Path $sequences "sequence-$($masks[$kind])-0.bin"
    if (!(Test-Path -LiteralPath $file)) { throw "Missing $kind mask stream: $file" }
}
foreach ($kind in $masks.Keys) {
    $file = Join-Path $sequences "sequence-$($masks[$kind])-0.bin"
    $lines = @(& "$PSScriptRoot\Test-FaceOverlayParity.ps1" `
        -FixtureDirectory $FixtureDirectory -OverlaySequence $file `
        -GameRoot $GameRoot -X86Probe $X86Probe -X64Probe $X64Probe `
        -OutputDirectory (Join-Path $output $kind) `
        -Frames 10 -StepMs 100 -OverlayShiftMs 0)
    if (!$? -or !$lines.Count -or $lines[-1] -notmatch '^FACE OVERLAY PARITY PASS:') {
        throw "Expression-mask parity did not pass: $kind"
    }
    Write-Output "$kind ($($masks[$kind])): PASS"
}
Write-Output "FACE EXPRESSION MASK PARITY PASS: $($masks.Count) game DB masks"
