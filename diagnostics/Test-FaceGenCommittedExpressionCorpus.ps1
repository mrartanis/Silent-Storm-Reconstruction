param(
    [Parameter(Mandatory)][string]$X86Stream,
    [Parameter(Mandatory)][string]$X64Stream,
    [Parameter(Mandatory)][string]$SequenceDirectory,
    [Parameter(Mandatory)][string]$TreeFile,
    [Parameter(Mandatory)][string]$GameRoot,
    [Parameter(Mandatory)][string]$X86Probe,
    [Parameter(Mandatory)][string]$X64Probe,
    [Parameter(Mandatory)][string]$OutputDirectory,
    [string]$SampleTimes,
    [double]$Tolerance = 0.0001
)
$ErrorActionPreference = 'Stop'
$sequenceRoot = (Resolve-Path -LiteralPath $SequenceDirectory).Path
$output = [IO.Path]::GetFullPath($OutputDirectory)
$headMotion = Join-Path $sequenceRoot 'sequence-6005-0.bin'
$speech = Join-Path $sequenceRoot 'sequence-371-0.bin'
if (!(Test-Path -LiteralPath $headMotion) -or !(Test-Path -LiteralPath $speech)) {
    throw 'Missing DB-exported head-motion or speech sequence'
}
# Non-neutral FaceExpression2Sequences records returned by the game DB.
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
if (!$SampleTimes) {
    $times = @(13999,14000,14099,14100,14101,15099,15100,15101,17998,17999) +
             @(14000..17999 | Where-Object { ($_ - 14000) % 25 -eq 0 })
    $SampleTimes = (@($times | Sort-Object -Unique) -join ',')
}
foreach ($kind in $masks.Keys) {
    $expression = Join-Path $sequenceRoot "sequence-$($masks[$kind])-0.bin"
    if (!(Test-Path -LiteralPath $expression)) { throw "Missing $kind expression sequence: $expression" }
    try {
        & (Join-Path $PSScriptRoot 'Test-FaceGenCommittedLayeredParity.ps1') `
            -X86Stream $X86Stream -X64Stream $X64Stream `
            -HeadMotionSequence $headMotion -SpeechSequence $speech `
            -ExpressionSequence $expression -TreeFile $TreeFile -GameRoot $GameRoot `
            -X86Probe $X86Probe -X64Probe $X64Probe `
            -OutputDirectory (Join-Path $output $kind) `
            -SampleTimes $SampleTimes -Tolerance $Tolerance
        if (!$?) { throw 'Layered parity command failed' }
    } catch {
        throw "$kind ($($masks[$kind])): $_"
    }
    Write-Host "$kind ($($masks[$kind])): PASS"
}
Write-Host "PASS: all $($masks.Count) game expression masks layered over speech/head motion on committed FaceGen streams"
