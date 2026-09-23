param(
    [Parameter(Mandatory)][string]$GameRoot,
    [Parameter(Mandatory)][string]$X86GDPProbe,
    [Parameter(Mandatory)][string]$X64GDPProbe,
    [Parameter(Mandatory)][string]$OutputDirectory,
    [string]$X86FaceProbe,
    [string]$X64FaceProbe,
    [string]$SequenceFile,
    [string]$MacroName = 'Nose',
    [double]$Amplitude = 0.5,
    [double]$Tolerance = 0.0001
)
$ErrorActionPreference = 'Stop'
$culture = [Globalization.CultureInfo]::InvariantCulture
$game = (Resolve-Path -LiteralPath $GameRoot).Path
$x86 = (Resolve-Path -LiteralPath $X86GDPProbe).Path
$x64 = (Resolve-Path -LiteralPath $X64GDPProbe).Path
if ($X86FaceProbe -or $X64FaceProbe -or $SequenceFile) {
    if (!$X86FaceProbe -or !$X64FaceProbe -or !$SequenceFile) {
        throw 'Animated output parity requires both FaceProbe executables and a sequence file'
    }
    $x86Face = (Resolve-Path -LiteralPath $X86FaceProbe).Path
    $x64Face = (Resolve-Path -LiteralPath $X64FaceProbe).Path
    $sequence = (Resolve-Path -LiteralPath $SequenceFile).Path
    $animationTree = Join-Path $game 'tree.mma'
}
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
    [pscustomobject]@{Name='gender-negative'; Macro='Gender'; Value=-1.0},
    [pscustomobject]@{Name='nationality-positive'; Macro='Nationality'; Value=1.0},
    [pscustomobject]@{Name='nationality-negative'; Macro='Nationality'; Value=-1.0},
    [pscustomobject]@{
        Name='game-editor-mid'; Macro=$null; Value=0.0
        Extra=@('Age','0', 'Gender','0', 'Nationality','0', 'Nose','0.5')
    },
    [pscustomobject]@{
        Name='game-editor-default'; Macro=$null; Value=0.0
        Extra=@(
            'Age','0', 'Gender','0', 'Nationality','-1',
            'Lips','0', 'Chin','0', 'Nose','0', 'Brows','0', 'Cheeks','0',
            'HairColor','1', 'WomanHair','1', 'EyesColor','-1',
            'FaceDamage','-1', 'FacialColor','-1'
        )
    }
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
        if ($case.Extra) {
            $referenceArgs += $case.Extra
            $nativeArgs += $case.Extra
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
        $animatedX86Max = 0.0
        $animatedX64Max = 0.0
        $animatedBad = 0
        if ($sequence) {
            $referenceAnimated = "$referencePrefix-animated.csv"
            $nativeOnX86 = "$nativePrefix-animated-x86.csv"
            $nativeOnX64 = "$nativePrefix-animated-x64.csv"
            & $x86Face "$referencePrefix-generated.bin" $referenceAnimated $sequence $animationTree
            if ($LASTEXITCODE -ne 0) { throw "x86 reference animation failed: $($case.Name)" }
            & $x86Face "$nativePrefix-generated.bin" $nativeOnX86 $sequence $animationTree
            if ($LASTEXITCODE -ne 0) { throw "x86 reload of native animator failed: $($case.Name)" }
            & $x64Face "$nativePrefix-generated.bin" $nativeOnX64 $sequence $animationTree
            if ($LASTEXITCODE -ne 0) { throw "x64 reload of native animator failed: $($case.Name)" }
            $animatedReference = @(Import-Csv -LiteralPath $referenceAnimated)
            $animatedReloadX86 = @(Import-Csv -LiteralPath $nativeOnX86)
            $animatedReloadX64 = @(Import-Csv -LiteralPath $nativeOnX64)
            if (!$animatedReference.Count -or
                $animatedReloadX86.Count -ne $animatedReference.Count -or
                $animatedReloadX64.Count -ne $animatedReference.Count) {
                throw "Animated row count mismatch: $($case.Name)"
            }
            for ($row = 0; $row -lt $animatedReference.Count; ++$row) {
                $a = $animatedReference[$row]
                $b = $animatedReloadX86[$row]
                $c = $animatedReloadX64[$row]
                if ($a.case -ne $b.case -or $a.case -ne $c.case -or
                    $a.time -ne $b.time -or $a.time -ne $c.time -or
                    $a.vertex -ne $b.vertex -or $a.vertex -ne $c.vertex) {
                    throw "Animated identity mismatch: $($case.Name) row $row"
                }
                foreach ($axis in 'x','y','z') {
                    $expected = [double]::Parse($a.$axis, $culture)
                    $fromX86 = [double]::Parse($b.$axis, $culture)
                    $fromX64 = [double]::Parse($c.$axis, $culture)
                    if (![double]::IsFinite($expected) -or
                        ![double]::IsFinite($fromX86) -or
                        ![double]::IsFinite($fromX64)) {
                        throw "Nonfinite animated coordinate: $($case.Name) row $row axis $axis"
                    }
                    $deltaX86 = [Math]::Abs($expected - $fromX86)
                    $deltaX64 = [Math]::Abs($expected - $fromX64)
                    $animatedX86Max = [Math]::Max($animatedX86Max, $deltaX86)
                    $animatedX64Max = [Math]::Max($animatedX64Max, $deltaX64)
                    if ($deltaX86 -gt $Tolerance -or $deltaX64 -gt $Tolerance) { ++$animatedBad }
                }
            }
        }
        $results += [pscustomobject]@{
            Case=$case.Name
            Vertices=$left.Count
            MaximumDelta=$max.ToString('G9', $culture)
            Mismatches=$bad
            AnimatedX86=$animatedX86Max.ToString('G9', $culture)
            AnimatedX64=$animatedX64Max.ToString('G9', $culture)
            AnimatedMismatches=$animatedBad
        }
    }
}
finally {
    $env:PATH = $oldPath
}
Write-Output (($results | Format-Table -AutoSize | Out-String).TrimEnd())
if (($results | Where-Object { $_.Mismatches -gt 0 -or $_.AnimatedMismatches -gt 0 }).Count) {
    throw "GDP transform parity failed: tolerance $Tolerance"
}
Write-Output "GDP TRANSFORM PARITY PASS: $($results.Count) cases, tolerance $Tolerance"
if ($sequence) {
    Write-Output "GDP ANIMATED OUTPUT PARITY PASS: $($results.Count) cases, x86 and x64 reload, tolerance $Tolerance"
}
