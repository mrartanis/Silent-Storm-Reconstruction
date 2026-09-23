param(
    [Parameter(Mandatory)][string]$X86Stream,
    [Parameter(Mandatory)][string]$X64Stream,
    [Parameter(Mandatory)][string]$SequenceDirectory,
    [Parameter(Mandatory)][string]$TreeFile,
    [Parameter(Mandatory)][string]$GameRoot,
    [Parameter(Mandatory)][string]$X86Probe,
    [Parameter(Mandatory)][string]$X64Probe,
    [Parameter(Mandatory)][string]$OutputDirectory,
    [ValidateRange(0,100000)][int]$ProgressEvery = 50,
    [double]$Tolerance = 0.0001
)
$ErrorActionPreference = 'Stop'
$sequences = @(Get-ChildItem -LiteralPath (Resolve-Path -LiteralPath $SequenceDirectory).Path `
    -Filter 'sequence-*.bin' -File | Where-Object Name -Match '^sequence-\d+-\d+\.bin$' | Sort-Object Name)
if (!$sequences.Count) { throw 'No exported game sequence streams found' }
$output = [IO.Path]::GetFullPath($OutputDirectory)
New-Item -ItemType Directory -Force -Path $output | Out-Null
$rows = 0
$moving = 0
$maximum = 0.0
$completed = 0
foreach ($sequence in $sequences) {
    try {
        $result = & (Join-Path $PSScriptRoot 'Test-FaceGenCommittedMotionParity.ps1') `
            -X86Stream $X86Stream -X64Stream $X64Stream -SequenceFile $sequence.FullName `
            -TreeFile $TreeFile -GameRoot $GameRoot -X86Probe $X86Probe -X64Probe $X64Probe `
            -OutputDirectory (Join-Path $output $sequence.BaseName) `
            -UseDefaultTimes -AllowStatic -PassThru -Tolerance $Tolerance
    } catch {
        throw "$($sequence.Name): $_"
    }
    if (!$result -or $result.Count -ne 1) { throw "No parity result for $($sequence.Name)" }
    $rows += $result.Rows
    $moving += $result.MovingRows
    $maximum = [Math]::Max($maximum, $result.MaxDelta)
    ++$completed
    if ($ProgressEvery -and $completed % $ProgressEvery -eq 0) {
        Write-Host "Committed-head corpus: $completed/$($sequences.Count) sequences passed; max delta $maximum"
    }
}
if (!$moving) { throw 'Corpus has no moving committed-head vertices at sampled times' }
Write-Host "PASS: $($sequences.Count) game sequences, $rows x86/x64 committed-head vertex rows, $moving moving rows, max delta $maximum (tolerance $Tolerance)"
