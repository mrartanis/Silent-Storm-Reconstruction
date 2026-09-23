param(
    [Parameter(Mandatory)][string]$GameRoot,
    [Parameter(Mandatory)][string]$X86GDPProbe,
    [Parameter(Mandatory)][string]$X64BlendCheck,
    [string]$X64SelectorParamCheck,
    [string]$X64GameWeightCheck,
    [string]$X64OutputTransferCheck,
    [Parameter(Mandatory)][string]$OutputDirectory
)
$ErrorActionPreference = 'Stop'
$game = (Resolve-Path -LiteralPath $GameRoot).Path
$x86 = (Resolve-Path -LiteralPath $X86GDPProbe).Path
$x64 = (Resolve-Path -LiteralPath $X64BlendCheck).Path
$nativeParameters = if ($X64SelectorParamCheck) {
    (Resolve-Path -LiteralPath $X64SelectorParamCheck).Path
} else { $null }
$nativeWeights = if ($X64GameWeightCheck) {
    (Resolve-Path -LiteralPath $X64GameWeightCheck).Path
} else { $null }
$nativeTransfer = if ($X64OutputTransferCheck) {
    (Resolve-Path -LiteralPath $X64OutputTransferCheck).Path
} else { $null }
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
if ($nativeParameters -and (Get-PEMachine $nativeParameters) -ne 0x8664) {
    throw 'Selector-parameter parity requires a native x64 check'
}
if ($nativeWeights -and (Get-PEMachine $nativeWeights) -ne 0x8664) {
    throw 'Game-weight parity requires a native x64 check'
}
if ($nativeTransfer -and (Get-PEMachine $nativeTransfer) -ne 0x8664) {
    throw 'Output-transfer parity requires a native x64 check'
}
$output = [IO.Path]::GetFullPath($OutputDirectory)
New-Item -ItemType Directory -Force -Path $output | Out-Null
$oldPath = $env:PATH
$oldWeights = $env:S2_FACE_SELECTOR_WEIGHTS_PATH
$oldParameters = $env:S2_FACE_SELECTOR_PARAMS_PATH
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
        [pscustomobject]@{ Name='nose-negative'; Macro='Nose'; Value='-0.5' },
        [pscustomobject]@{ Name='nationality'; Macro='Nationality'; Value='1' },
        [pscustomobject]@{ Name='nationality-negative'; Macro='Nationality'; Value='-1' },
        [pscustomobject]@{
            Name='game-editor-mid'; Macro=$null; Value=$null
            Extra=@('Age','0', 'Gender','0', 'Nationality','0', 'Nose','0.5')
        },
        [pscustomobject]@{
            Name='game-editor-default'; Macro=$null; Value=$null
            Extra=@(
                'Age','0', 'Gender','0', 'Nationality','-1',
                'Lips','0', 'Chin','0', 'Nose','0', 'Brows','0', 'Cheeks','0',
                'HairColor','1', 'WomanHair','1', 'EyesColor','-1',
                'FaceDamage','-1', 'FacialColor','-1'
            )
        }
    )) {
        $prefix = Join-Path $output $case.Name
        $weights = "$prefix-weights.csv"
        $parameters = "$prefix-params.csv"
        $head = "$prefix-morphed-head.bin"
        $env:S2_FACE_SELECTOR_WEIGHTS_PATH = $weights
        $env:S2_FACE_SELECTOR_PARAMS_PATH = if ($nativeParameters) { $parameters } else { $null }
        $env:S2_FACE_DUMP_TRANSFORMER_INPUT_PREFIX = $prefix
        $probeArgs = @($gdp, $tree, $prefix)
        $macroArgs = @()
        if ($case.Macro) { $macroArgs += @($case.Macro, $case.Value) }
        if ($case.Extra) { $macroArgs += $case.Extra }
        $probeArgs += $macroArgs
        & $x86 @probeArgs
        if ($LASTEXITCODE -ne 0) { throw "x86 GDP probe failed: $($case.Name)" }
        $rows = @(Import-Csv -LiteralPath $weights)
        if ($rows.Count -ne 16) { throw "Expected 16 selector weights: $($case.Name)" }
        & $x64 $gdp $weights $head --morph-fields
        if ($LASTEXITCODE -ne 0) { throw "Native morph-head parity failed: $($case.Name)" }
        & $x64 $gdp $weights "$prefix-base-animation.bin" --animation-fields
        if ($LASTEXITCODE -ne 0) { throw "Native animation-head parity failed: $($case.Name)" }
        if ($nativeTransfer) {
            & $nativeTransfer "$prefix-base-animation.bin" $head `
                "$prefix-morph-processed.csv" "$prefix-generated.bin"
            if ($LASTEXITCODE -ne 0) { throw "Native output transfer parity failed: $($case.Name)" }
        }
        if ($nativeParameters) {
            & $nativeParameters $tree $parameters @macroArgs
            if ($LASTEXITCODE -ne 0) { throw "Native selector-parameter parity failed: $($case.Name)" }
        }
        if ($nativeWeights) {
            & $nativeWeights $tree --native $weights @macroArgs
            if ($LASTEXITCODE -ne 0) { throw "Native game-weight parity failed: $($case.Name)" }
        }
    }
    if ((Get-FileHash -LiteralPath (Join-Path $output 'neutral-morphed-head.bin')).Hash -ne
        (Get-FileHash -LiteralPath (Join-Path $output 'nose-morphed-head.bin')).Hash) {
        throw 'Nose unexpectedly changed the serialized morph-head base'
    }
    Write-Output 'FACEGEN MORPH-HEAD PARITY PASS: 11 x86 cases, vertices, 129 muscles, influences and 6 bones, tolerance=0.0001'
    Write-Output 'FACEGEN ANIMATION-HEAD PARITY PASS: 11 x86 cases, vertices, 36 muscles, influences and 7 bones, tolerance=0.0001'
    if ($nativeTransfer) {
        Write-Output 'FACEGEN OUTPUT-TRANSFER PARITY PASS: 11 x86 cases, final head fields, tolerance=0.0001'
    }
    if ($nativeParameters) {
        Write-Output 'FACEGEN SELECTOR-PARAMETER PARITY PASS: 11 x86 cases, 5 inputs each, tolerance=0.0001'
    }
    if ($nativeWeights) {
        Write-Output 'FACEGEN NATIVE GAME WEIGHT PARITY PASS: 11 x86 cases, 16 weights each, tolerance=0.0001'
    }
}
finally {
    $env:PATH = $oldPath
    $env:S2_FACE_SELECTOR_WEIGHTS_PATH = $oldWeights
    $env:S2_FACE_SELECTOR_PARAMS_PATH = $oldParameters
    $env:S2_FACE_DUMP_TRANSFORMER_INPUT_PREFIX = $oldDump
}
