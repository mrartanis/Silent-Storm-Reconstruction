param(
    [Parameter(Mandatory)][string]$GameRoot,
    [Parameter(Mandatory)][string]$X86GDPProbe,
    [Parameter(Mandatory)][string]$X64GDPProbe,
    [Parameter(Mandatory)][string]$OutputDirectory,
    [string]$MacroName = 'Nose',
    [double]$Amplitude = 0.5,
    [double]$Tolerance = 0.0001
)
$ErrorActionPreference = 'Stop'
$culture = [Globalization.CultureInfo]::InvariantCulture
$game = (Resolve-Path -LiteralPath $GameRoot).Path
$x86 = (Resolve-Path -LiteralPath $X86GDPProbe).Path
$x64 = (Resolve-Path -LiteralPath $X64GDPProbe).Path
$gdp = Join-Path $game 'Res\FaceGenHead.gdp'
$mmt = Join-Path $game 'Res\FaceGenHead.mmt'
$output = [IO.Path]::GetFullPath($OutputDirectory)
New-Item -ItemType Directory -Force -Path $output | Out-Null
$oldPath = $env:PATH
$results = @()
$cases = @(
    [pscustomobject]@{Name='neutral'; Macro=$null; Value=0.0},
    [pscustomobject]@{Name='morph-positive'; Macro=$MacroName; Value=$Amplitude},
    [pscustomobject]@{Name='morph-negative'; Macro=$MacroName; Value=-$Amplitude},
    [pscustomobject]@{Name='age-positive'; Macro='Age'; Value=1.0},
    [pscustomobject]@{Name='age-negative'; Macro='Age'; Value=-1.0},
    [pscustomobject]@{Name='gender-positive'; Macro='Gender'; Value=1.0},
    [pscustomobject]@{Name='gender-negative'; Macro='Gender'; Value=-1.0}
)
try {
    $env:PATH = "$game;$oldPath"
    foreach ($case in $cases) {
        $args = @($gdp, $mmt)
        $referencePrefix = Join-Path $output "$($case.Name)-x86"
        $nativePrefix = Join-Path $output "$($case.Name)-x64"
        $referenceArgs = $args + @($referencePrefix)
        $nativeArgs = $args + @($nativePrefix)
        if ($case.Macro) {
            $expression = $case.Value.ToString('G9', $culture)
            $referenceArgs += @($case.Macro, $expression)
            $nativeArgs += @($case.Macro, $expression)
        }
        & $x86 @referenceArgs
        if ($LASTEXITCODE -ne 0) { throw "x86 GDP transform failed: $($case.Name)" }
        & $x64 @nativeArgs
        if ($LASTEXITCODE -ne 0) { throw "x64 GDP transform failed: $($case.Name)" }
        $left = @(Import-Csv -LiteralPath "$referencePrefix-vertices.csv")
        $right = @(Import-Csv -LiteralPath "$nativePrefix-vertices.csv")
        if ($left.Count -ne 419 -or $right.Count -ne $left.Count) {
            throw "GDP transform vertex count mismatch: $($case.Name)"
        }
        $max = 0.0
        $bad = 0
        for ($i = 0; $i -lt $left.Count; ++$i) {
            if ($left[$i].vertex -ne $right[$i].vertex) {
                throw "GDP transform vertex identity mismatch: $($case.Name) row $i"
            }
            foreach ($axis in 'x','y','z') {
                $a = [double]::Parse($left[$i].$axis, $culture)
                $b = [double]::Parse($right[$i].$axis, $culture)
                if (![double]::IsFinite($a) -or ![double]::IsFinite($b)) {
                    throw "Nonfinite GDP transform coordinate: $($case.Name) row $i axis $axis"
                }
                $delta = [Math]::Abs($a - $b)
                $max = [Math]::Max($max, $delta)
                if ($delta -gt $Tolerance) { ++$bad }
            }
        }
        $results += [pscustomobject]@{
            Case=$case.Name
            Vertices=$left.Count
            MaximumDelta=$max.ToString('G9', $culture)
            Mismatches=$bad
        }
    }
}
finally {
    $env:PATH = $oldPath
}
Write-Output (($results | Format-Table -AutoSize | Out-String).TrimEnd())
if (($results | Where-Object Mismatches -GT 0).Count) {
    throw "GDP transform parity failed: tolerance $Tolerance"
}
Write-Output "GDP TRANSFORM PARITY PASS: $($results.Count) cases, tolerance $Tolerance"
