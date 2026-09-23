param(
    [Parameter(Mandatory)][string]$GameRoot,
    [Parameter(Mandatory)][string]$X86GDPProbe,
    [Parameter(Mandatory)][string]$X64WeightCheck,
    [Parameter(Mandatory)][string]$OutputDirectory,
    [switch]$NativeEthnicity
)
$ErrorActionPreference = 'Stop'
$game = (Resolve-Path -LiteralPath $GameRoot).Path
$x86 = (Resolve-Path -LiteralPath $X86GDPProbe).Path
$x64 = (Resolve-Path -LiteralPath $X64WeightCheck).Path
$gdp = Join-Path $game 'Res\FaceGenHead.gdp'
$tree = Join-Path $game 'Res\FaceGenHead.mmt'
$output = [IO.Path]::GetFullPath($OutputDirectory)
New-Item -ItemType Directory -Force -Path $output | Out-Null
$oldPath = $env:PATH
$oldWeights = $env:S2_FACE_SELECTOR_WEIGHTS_PATH
$oldParameters = $env:S2_FACE_SELECTOR_PARAMS_PATH
$oldDump = $env:S2_FACE_DUMP_TRANSFORMER_INPUT_PREFIX
$maximum = 0.0
$cases = 0
try {
    $env:PATH = "$game;$oldPath"
    $env:S2_FACE_SELECTOR_PARAMS_PATH = $null
    $env:S2_FACE_DUMP_TRANSFORMER_INPUT_PREFIX = $null
    if ($NativeEthnicity) {
        $culture = [Globalization.CultureInfo]::InvariantCulture
        for ($position = 0; $position -le 100; ++$position) {
            $amplitude = ([single]($position * 0.02 - 1.0)).ToString('G9', $culture)
            $prefix = Join-Path $output ('slider-{0:D3}' -f $position)
            $env:S2_FACE_SELECTOR_WEIGHTS_PATH = "$prefix-weights.csv"
            & $x86 $gdp $tree $prefix Nationality $amplitude 2>&1 | Out-Null
            if ($LASTEXITCODE -ne 0) { throw "x86 slider position failed: $position" }
            $result = & $x64 $tree --native "$prefix-weights.csv" Nationality $amplitude
            if ($LASTEXITCODE -ne 0) { throw "Native slider position failed: $position $result" }
            if ($result -notmatch 'max-delta=([0-9.eE+-]+)') { throw "Bad check output: $result" }
            $maximum = [Math]::Max($maximum,
                [double]::Parse($Matches[1], [Globalization.CultureInfo]::InvariantCulture))
            ++$cases
        }
        foreach ($slider in @('Age','Gender')) {
            for ($position = 0; $position -le 100; ++$position) {
                $amplitude = ([single]($position * 0.02 - 1.0)).ToString('G9', $culture)
                $prefix = Join-Path $output ("$slider-{0:D3}" -f $position)
                $env:S2_FACE_SELECTOR_WEIGHTS_PATH = "$prefix-weights.csv"
                $macroArgs = @($slider,$amplitude,'Nationality','0')
                & $x86 $gdp $tree $prefix @macroArgs 2>&1 | Out-Null
                if ($LASTEXITCODE -ne 0) { throw "x86 $slider slider failed: $position" }
                $result = & $x64 $tree --native "$prefix-weights.csv" @macroArgs
                if ($LASTEXITCODE -ne 0) { throw "Native $slider slider failed: $position $result" }
                if ($result -notmatch 'max-delta=([0-9.eE+-]+)') { throw "Bad check output: $result" }
                $maximum = [Math]::Max($maximum,
                    [double]::Parse($Matches[1], [Globalization.CultureInfo]::InvariantCulture))
                ++$cases
            }
        }
    }
    foreach ($nationality in @('-1','0','1')) {
        $stem = $nationality.Replace('-','m')
        $base = Join-Path $output "nationality-$stem"
        $env:S2_FACE_SELECTOR_WEIGHTS_PATH = "$base-weights.csv"
        & $x86 $gdp $tree $base Nationality $nationality 2>&1 | Out-Null
        if ($LASTEXITCODE -ne 0) { throw "x86 nationality baseline failed: $nationality" }
        foreach ($age in @('-1','0','1')) {
            foreach ($gender in @('-1','0','1')) {
                $prefix = Join-Path $output "n$stem-a$($age.Replace('-','m'))-g$($gender.Replace('-','m'))"
                $env:S2_FACE_SELECTOR_WEIGHTS_PATH = "$prefix-weights.csv"
                $macroArgs = @('Age',$age,'Gender',$gender,'Nationality',$nationality)
                & $x86 $gdp $tree $prefix @macroArgs 2>&1 | Out-Null
                if ($LASTEXITCODE -ne 0) {
                    throw "x86 game weight case failed: nationality=$nationality age=$age gender=$gender"
                }
                $nationalityInput = if ($NativeEthnicity) { '--native' } else { "$base-weights.csv" }
                $result = & $x64 $tree $nationalityInput "$prefix-weights.csv" @macroArgs
                if ($LASTEXITCODE -ne 0) {
                    throw "Native game weight factorization failed: nationality=$nationality age=$age gender=$gender $result"
                }
                if ($result -notmatch 'max-delta=([0-9.eE+-]+)') { throw "Bad check output: $result" }
                $delta = [double]::Parse($Matches[1], [Globalization.CultureInfo]::InvariantCulture)
                $maximum = [Math]::Max($maximum, $delta)
                ++$cases
            }
        }
    }
    if ($NativeEthnicity) {
        Write-Output "FACEGEN NATIVE GAME WEIGHTS PASS: cases=$cases, max-raw-weight-delta=$maximum, tolerance=0.0001"
    } else {
        Write-Output "FACEGEN GAME WEIGHT FACTORIZATION PASS: cases=$cases, max-raw-weight-delta=$maximum, tolerance=0.0001"
    }
}
finally {
    $env:PATH = $oldPath
    $env:S2_FACE_SELECTOR_WEIGHTS_PATH = $oldWeights
    $env:S2_FACE_SELECTOR_PARAMS_PATH = $oldParameters
    $env:S2_FACE_DUMP_TRANSFORMER_INPUT_PREFIX = $oldDump
}
