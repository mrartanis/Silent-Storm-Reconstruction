param(
    [Parameter(Mandatory)][string]$FixtureDirectory,
    [Parameter(Mandatory)][string]$OverlaySequence,
    [Parameter(Mandatory)][string]$GameRoot,
    [Parameter(Mandatory)][string]$X86Probe,
    [Parameter(Mandatory)][string]$X64Probe,
    [Parameter(Mandatory)][string]$OutputDirectory,
    [int]$Frames = 100,
    [int]$StepMs = 100,
    [int]$OverlayShiftMs = -1500
)
$ErrorActionPreference = 'Stop'
if ($Frames -lt 2 -or $StepMs -lt 1 -or
    [long]($Frames - 1) * $StepMs -gt [int]::MaxValue) {
    throw 'Invalid frame grid'
}
$fixtures = (Resolve-Path -LiteralPath $FixtureDirectory).Path
$overlay = (Resolve-Path -LiteralPath $OverlaySequence).Path
$heads = @(Get-ChildItem -LiteralPath $fixtures -Filter 'head-*.bin' -File |
    Where-Object Name -Match '^head-\d+-\d+\.bin$')
$sequences = @(Get-ChildItem -LiteralPath $fixtures -Filter 'sequence-*.bin' -File |
    Where-Object Name -Match '^sequence-\d+-\d+\.bin$')
if (!$heads.Count -or $sequences.Count -ne 1) {
    throw 'Fixture directory must contain heads and exactly one primary sequence'
}
if ($overlay -eq $sequences[0].FullName) { throw 'Overlay must be a separate sequence' }
$out = [IO.Path]::GetFullPath($OutputDirectory)
$baselineOut = Join-Path $out 'primary-only'
$overlayOut = Join-Path $out 'overlaid'
$oldTimes = $env:S2_FACE_SAMPLE_TIMES
$oldOverlay = $env:S2_FACE_OVERLAY_SEQUENCE_FILE
$oldShift = $env:S2_FACE_OVERLAY_TIME_SHIFT
try {
    $env:S2_FACE_SAMPLE_TIMES = (0..($Frames - 1) | ForEach-Object { $_ * $StepMs }) -join ','
    $env:S2_FACE_OVERLAY_SEQUENCE_FILE = $null
    $env:S2_FACE_OVERLAY_TIME_SHIFT = $null
    & "$PSScriptRoot\Test-FaceParity.ps1" -FixtureDirectory $fixtures `
        -GameRoot $GameRoot -X86Probe $X86Probe -X64Probe $X64Probe `
        -OutputDirectory $baselineOut
    if (!$?) { throw 'Primary-only parity failed' }

    $env:S2_FACE_OVERLAY_SEQUENCE_FILE = $overlay
    $env:S2_FACE_OVERLAY_TIME_SHIFT = [string]$OverlayShiftMs
    & "$PSScriptRoot\Test-FaceParity.ps1" -FixtureDirectory $fixtures `
        -GameRoot $GameRoot -X86Probe $X86Probe -X64Probe $X64Probe `
        -OutputDirectory $overlayOut
    if (!$?) { throw 'Overlay parity failed' }

    foreach ($head in $heads) {
        $name = "$($head.BaseName)--$($sequences[0].BaseName)-x86"
        $baseTrace = @(Import-Csv -LiteralPath (Join-Path $baselineOut "$name-muscles.csv"))
        $overTrace = @(Import-Csv -LiteralPath (Join-Path $overlayOut "$name-muscles.csv"))
        if ($overTrace.Count -le $baseTrace.Count) {
            throw "Overlay produced no additional macro calls: $name"
        }
        $baseHash = (Get-FileHash -LiteralPath (Join-Path $baselineOut "$name.csv")).Hash
        $overHash = (Get-FileHash -LiteralPath (Join-Path $overlayOut "$name.csv")).Hash
        if ($baseHash -eq $overHash) {
            throw "Overlay did not change vertices: $name"
        }
        Write-Output "$name : $($baseTrace.Count) primary calls, $($overTrace.Count) overlaid calls; vertices changed"
    }
    Write-Output "FACE OVERLAY PARITY PASS: $($heads.Count) heads, $Frames frames, shift $OverlayShiftMs ms"
}
finally {
    $env:S2_FACE_SAMPLE_TIMES = $oldTimes
    $env:S2_FACE_OVERLAY_SEQUENCE_FILE = $oldOverlay
    $env:S2_FACE_OVERLAY_TIME_SHIFT = $oldShift
}
