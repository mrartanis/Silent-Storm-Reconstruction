param(
    [Parameter(Mandatory)][string]$GameRoot,
    [Parameter(Mandatory)][string]$X86GDPProbe,
    [Parameter(Mandatory)][string]$X86FaceProbe,
    [Parameter(Mandatory)][string]$X64FaceProbe,
    [Parameter(Mandatory)][string]$OutputDirectory,
    [string]$NativeHeadDecode,
    [string]$MacroName = 'Nose',
    [double]$Amplitude = 0.5,
    [string]$SequenceFile,
    [double]$Tolerance = 0.0001
)
$ErrorActionPreference = 'Stop'
$culture = [Globalization.CultureInfo]::InvariantCulture
$game = (Resolve-Path -LiteralPath $GameRoot).Path
$gdp = Join-Path $game 'Res\FaceGenHead.gdp'
$mmt = Join-Path $game 'Res\FaceGenHead.mmt'
$x86GDP = (Resolve-Path -LiteralPath $X86GDPProbe).Path
$x86Face = (Resolve-Path -LiteralPath $X86FaceProbe).Path
$x64Face = (Resolve-Path -LiteralPath $X64FaceProbe).Path
if ($NativeHeadDecode) { $nativeHead = (Resolve-Path -LiteralPath $NativeHeadDecode).Path }
if ($SequenceFile) { $sequence = (Resolve-Path -LiteralPath $SequenceFile).Path }
$output = [IO.Path]::GetFullPath($OutputDirectory)
New-Item -ItemType Directory -Force -Path $output | Out-Null
$oldPath = $env:PATH
$oldInfluences = $env:S2_FACE_VERTEX_INFLUENCES_PATH
$oldSnapshotTime = $env:S2_FACE_STATE_SNAPSHOT_TIME
$results = @()
try {
    $env:PATH = "$game;$oldPath"
    foreach ($case in 'neutral','morph') {
        $prefix = Join-Path $output $case
        $args = @($gdp, $mmt, $prefix)
        if ($case -eq 'morph') {
            $args += @($MacroName, $Amplitude.ToString('G9', $culture))
        }
        & $x86GDP @args
        if ($LASTEXITCODE -ne 0) { throw "x86 GDP oracle failed: $case" }
        $generated = "$prefix-generated.bin"
        $reference = "$prefix-vertices.csv"
        $reloaded = "$prefix-x86-reloaded.csv"
        $candidate = "$prefix-x64.csv"
        & $x86Face $generated $reloaded
        if ($LASTEXITCODE -ne 0) { throw "x86 generated stream reload failed: $case" }
        & $x64Face $generated $candidate
        if ($LASTEXITCODE -ne 0) { throw "x64 generated stream load failed: $case" }
        $left = @(Import-Csv -LiteralPath $reference)
        $repeat = @(Import-Csv -LiteralPath $reloaded)
        $right = @(Import-Csv -LiteralPath $candidate)
        if ($left.Count -ne 419 -or $repeat.Count -ne $left.Count -or
            $right.Count -ne $left.Count) {
            throw "GDP vertex count mismatch: $case x86=$($left.Count) reload=$($repeat.Count) x64=$($right.Count)"
        }
        $max = 0.0
        $bad = 0
        for ($i = 0; $i -lt $left.Count; ++$i) {
            if ($left[$i].vertex -ne $repeat[$i].vertex -or
                $left[$i].vertex -ne $right[$i].vertex) {
                throw "GDP vertex identity mismatch: $case row $i"
            }
            foreach ($axis in 'x','y','z') {
                $expected = [double]::Parse($left[$i].$axis, $culture)
                $x86Reload = [double]::Parse($repeat[$i].$axis, $culture)
                $native = [double]::Parse($right[$i].$axis, $culture)
                if (![double]::IsFinite($expected) -or ![double]::IsFinite($native) -or
                    $expected -ne $x86Reload) {
                    throw "Invalid x86 reload or nonfinite coordinate: $case row $i axis $axis"
                }
                $delta = [Math]::Abs($expected - $native)
                $max = [Math]::Max($max, $delta)
                if ($delta -gt $Tolerance) { ++$bad }
            }
        }
        $results += [pscustomobject]@{
            Case=$case
            Rows=$left.Count
            MaximumDelta=$max.ToString('G9', $culture)
            Mismatches=$bad
        }
        if ($case -eq 'morph' -and $sequence) {
            $tree = Join-Path $game 'tree.mma'
            $animatedX86 = "$prefix-animated-x86.csv"
            $animatedX64 = "$prefix-animated-x64.csv"
            if ($nativeHead) {
                $env:S2_FACE_VERTEX_INFLUENCES_PATH = "$prefix-x86-influences.csv"
                $env:S2_FACE_STATE_SNAPSHOT_TIME = '0'
            }
            & $x86Face $generated $animatedX86 $sequence $tree
            if ($LASTEXITCODE -ne 0) { throw 'x86 FaceGen animation oracle failed' }
            $env:S2_FACE_VERTEX_INFLUENCES_PATH = $null
            $env:S2_FACE_STATE_SNAPSHOT_TIME = $null
            & $x64Face $generated $animatedX64 $sequence $tree
            if ($LASTEXITCODE -ne 0) { throw 'x64 FaceGen animation probe failed' }
            $animatedLeft = @(Import-Csv -LiteralPath $animatedX86)
            $animatedRight = @(Import-Csv -LiteralPath $animatedX64)
            if (!$animatedLeft.Count -or $animatedLeft.Count -ne $animatedRight.Count) {
                throw 'FaceGen animated vertex count mismatch'
            }
            $animatedMax = 0.0
            $animatedBad = 0
            for ($i = 0; $i -lt $animatedLeft.Count; ++$i) {
                if ($animatedLeft[$i].case -ne $animatedRight[$i].case -or
                    $animatedLeft[$i].time -ne $animatedRight[$i].time -or
                    $animatedLeft[$i].vertex -ne $animatedRight[$i].vertex) {
                    throw "FaceGen animated row identity mismatch: $i"
                }
                foreach ($axis in 'x','y','z') {
                    $x86Value = [double]::Parse($animatedLeft[$i].$axis, $culture)
                    $x64Value = [double]::Parse($animatedRight[$i].$axis, $culture)
                    if (![double]::IsFinite($x86Value) -or ![double]::IsFinite($x64Value)) {
                        throw "Nonfinite FaceGen animation coordinate: row $i axis $axis"
                    }
                    $delta = [Math]::Abs($x86Value - $x64Value)
                    $animatedMax = [Math]::Max($animatedMax, $delta)
                    if ($delta -gt $Tolerance) { ++$animatedBad }
                }
            }
            $results += [pscustomobject]@{
                Case='morph+sequence'
                Rows=$animatedLeft.Count
                MaximumDelta=$animatedMax.ToString('G9', $culture)
                Mismatches=$animatedBad
            }
            if ($nativeHead) {
                $nativeRows = @(& $nativeHead $generated '--influences' | ConvertFrom-Csv)
                if ($LASTEXITCODE -ne 0) { throw 'Native FaceGen influence decode failed' }
                $referenceRows = @(Import-Csv -LiteralPath "$prefix-x86-influences.csv")
                if (!$referenceRows.Count -or $referenceRows.Count -ne $nativeRows.Count) {
                    throw 'FaceGen runtime influence count mismatch'
                }
                $weightMax = 0.0
                $weightBad = 0
                for ($i = 0; $i -lt $referenceRows.Count; ++$i) {
                    $a = $referenceRows[$i]
                    $b = $nativeRows[$i]
                    if ($a.vertex -ne $b.vertex -or $a.muscle -ne $b.muscle) {
                        throw "FaceGen influence identity mismatch: row $i"
                    }
                    foreach ($pair in @(@('componentA','component-a'), @('componentB','component-b'))) {
                        $delta = [Math]::Abs([double]::Parse($a.($pair[0]), $culture) -
                                             [double]::Parse($b.($pair[1]), $culture))
                        $weightMax = [Math]::Max($weightMax, $delta)
                        if ($delta -gt $Tolerance) { ++$weightBad }
                    }
                }
                $results += [pscustomobject]@{
                    Case='morph+weights'
                    Rows=$referenceRows.Count
                    MaximumDelta=$weightMax.ToString('G9', $culture)
                    Mismatches=$weightBad
                }
            }
        }
    }
}
finally {
    $env:PATH = $oldPath
    $env:S2_FACE_VERTEX_INFLUENCES_PATH = $oldInfluences
    $env:S2_FACE_STATE_SNAPSHOT_TIME = $oldSnapshotTime
}
Write-Output (($results | Format-Table -AutoSize | Out-String).TrimEnd())
if (($results | Where-Object Mismatches -GT 0).Count) {
    throw "FaceGen generated animator parity failed: tolerance $Tolerance"
}
Write-Output "FACEGEN SAVED-ANIMATOR PARITY PASS: $($results.Count) cases, tolerance $Tolerance"
