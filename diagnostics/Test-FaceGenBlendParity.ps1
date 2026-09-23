param(
    [Parameter(Mandatory)][string]$GameRoot,
    [Parameter(Mandatory)][string]$X86GDPProbe,
    [Parameter(Mandatory)][string]$X64BlendCheck,
    [Parameter(Mandatory)][string]$OutputDirectory
)
$ErrorActionPreference = 'Stop'
$game = (Resolve-Path -LiteralPath $GameRoot).Path
$x86 = (Resolve-Path -LiteralPath $X86GDPProbe).Path
$x64 = (Resolve-Path -LiteralPath $X64BlendCheck).Path
$gdp = Join-Path $game 'Res\FaceGenHead.gdp'
$tree = Join-Path $game 'Res\FaceGenHead.mmt'
foreach ($path in @($gdp, $tree)) {
    if (!(Test-Path -LiteralPath $path -PathType Leaf)) { throw "Missing FaceGen resource: $path" }
}
function Get-PEMachine([string]$executable) {
    $bytes = [IO.File]::ReadAllBytes($executable)
    if ($bytes.Length -lt 0x40 -or [BitConverter]::ToUInt16($bytes, 0) -ne 0x5a4d) {
        throw "Invalid PE executable: $executable"
    }
    $header = [BitConverter]::ToInt32($bytes, 0x3c)
    if ($header -lt 0 -or $header -gt $bytes.Length - 6 -or
        [BitConverter]::ToUInt32($bytes, $header) -ne 0x4550) {
        throw "Invalid PE header: $executable"
    }
    return [BitConverter]::ToUInt16($bytes, $header + 4)
}
if ((Get-PEMachine $x86) -ne 0x14c -or (Get-PEMachine $x64) -ne 0x8664) {
    throw 'Blend parity requires the original x86 GDP probe and native x64 blend check'
}
$output = [IO.Path]::GetFullPath($OutputDirectory)
New-Item -ItemType Directory -Force -Path $output | Out-Null
$oldPath = $env:PATH
$oldWeights = $env:S2_FACE_SELECTOR_WEIGHTS_PATH
$oldDump = $env:S2_FACE_DUMP_TRANSFORMER_INPUT_PREFIX
try {
    $env:PATH = "$game;$oldPath"
    foreach ($case in @(
        [pscustomobject]@{ Name='neutral'; Macro=$null; Value=$null },
        [pscustomobject]@{ Name='age'; Macro='Age'; Value='1' },
        [pscustomobject]@{ Name='age-negative'; Macro='Age'; Value='-1' },
        [pscustomobject]@{ Name='gender'; Macro='Gender'; Value='1' },
        [pscustomobject]@{ Name='gender-negative'; Macro='Gender'; Value='-1' },
        [pscustomobject]@{ Name='nose'; Macro='Nose'; Value='0.5' },
        [pscustomobject]@{ Name='nose-negative'; Macro='Nose'; Value='-0.5' }
    )) {
        $prefix = Join-Path $output $case.Name
        $weights = "$prefix-weights.csv"
        $head = "$prefix-morphed-head.bin"
        $env:S2_FACE_SELECTOR_WEIGHTS_PATH = $weights
        $env:S2_FACE_DUMP_TRANSFORMER_INPUT_PREFIX = $prefix
        $probeArgs = @($gdp, $tree, $prefix)
        if ($case.Macro) { $probeArgs += @($case.Macro, $case.Value) }
        & $x86 @probeArgs
        if ($LASTEXITCODE -ne 0) { throw "x86 GDP probe failed: $($case.Name)" }
        $rows = @(Import-Csv -LiteralPath $weights)
        if ($rows.Count -ne 16) { throw "Expected 16 selector weights: $($case.Name)" }
        & $x64 $gdp $weights $head
        if ($LASTEXITCODE -ne 0) { throw "Native blend parity failed: $($case.Name)" }
    }
    if ((Get-FileHash -LiteralPath (Join-Path $output 'neutral-morphed-head.bin')).Hash -ne
        (Get-FileHash -LiteralPath (Join-Path $output 'nose-morphed-head.bin')).Hash) {
        throw 'Nose unexpectedly changed the serialized morph-head base'
    }
    Write-Output 'FACEGEN BLEND PARITY PASS: 7 x86 cases, 419 vertices and 129 muscle anchors each, tolerance=0.0001'
}
finally {
    $env:PATH = $oldPath
    $env:S2_FACE_SELECTOR_WEIGHTS_PATH = $oldWeights
    $env:S2_FACE_DUMP_TRANSFORMER_INPUT_PREFIX = $oldDump
}
