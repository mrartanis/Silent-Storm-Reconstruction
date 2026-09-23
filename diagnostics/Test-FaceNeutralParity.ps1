param(
    [Parameter(Mandatory)][string]$FixtureDirectory,
    [Parameter(Mandatory)][string]$GameRoot,
    [Parameter(Mandatory)][string]$X86Probe,
    [Parameter(Mandatory)][string]$X64Probe,
    [string]$OutputDirectory,
    [double]$Tolerance = 0.0001
)
$ErrorActionPreference = 'Stop'
$fixtures = (Resolve-Path -LiteralPath $FixtureDirectory).Path
$game = (Resolve-Path -LiteralPath $GameRoot).Path
$x86 = (Resolve-Path -LiteralPath $X86Probe).Path
$x64 = (Resolve-Path -LiteralPath $X64Probe).Path
if (!$OutputDirectory) { $OutputDirectory = Join-Path $fixtures 'neutral-comparison' }
$output = [IO.Path]::GetFullPath($OutputDirectory)
New-Item -ItemType Directory -Path $output -Force | Out-Null
$heads = @(Get-ChildItem -LiteralPath $fixtures -Filter 'head-*.bin' -File |
    Where-Object Name -Match '^head-\d+-\d+\.bin$' | Sort-Object Name)
if (!$heads.Count) { throw 'No original head streams found' }
$culture = [Globalization.CultureInfo]::InvariantCulture
$priorPath = $env:PATH
$results = @()
try {
    $env:PATH = "$game;$priorPath"
    foreach ($head in $heads) {
        $name = $head.BaseName
        $reference = Join-Path $output "$name-x86.csv"
        $repeat = Join-Path $output "$name-x86-repeat.csv"
        $candidate = Join-Path $output "$name-x64.csv"
        & $x86 $head.FullName $reference
        if ($LASTEXITCODE -ne 0) { throw "x86 neutral oracle failed: $name" }
        & $x86 $head.FullName $repeat
        if ($LASTEXITCODE -ne 0) { throw "x86 neutral oracle repeat failed: $name" }
        if ((Get-FileHash -LiteralPath $reference).Hash -ne (Get-FileHash -LiteralPath $repeat).Hash) {
            throw "x86 neutral oracle is nondeterministic: $name"
        }
        & $x64 $head.FullName $candidate
        if ($LASTEXITCODE -ne 0) { throw "x64 native neutral probe failed: $name" }
        $left = @(Import-Csv -LiteralPath $reference)
        $right = @(Import-Csv -LiteralPath $candidate)
        if (!$left.Count -or $left.Count -ne $right.Count) {
            throw "Different neutral vertex-row count: $name x86=$($left.Count) x64=$($right.Count)"
        }
        $max = 0.0
        $bad = 0
        for ($i = 0; $i -lt $left.Count; ++$i) {
            $a = $left[$i]
            $b = $right[$i]
            if ($a.case -ne 'neutral' -or $b.case -ne 'neutral' -or
                $a.time -ne $b.time -or $a.vertex -ne $b.vertex) {
                throw "Different neutral row identity: $name row $i"
            }
            foreach ($axis in 'x','y','z') {
                $value = [double]::Parse($b.$axis, $culture)
                if ([double]::IsNaN($value) -or [double]::IsInfinity($value)) {
                    throw "Nonfinite x64 coordinate: $name row $i axis $axis"
                }
                $delta = [Math]::Abs([double]::Parse($a.$axis, $culture) - $value)
                $max = [Math]::Max($max, $delta)
                if ($delta -gt $Tolerance) { ++$bad }
            }
        }
        $results += [pscustomobject]@{
            Head=$name
            Vertices=$left.Count
            MaximumDelta=$max.ToString('G9', $culture)
            Mismatches=$bad
        }
    }
}
finally {
    $env:PATH = $priorPath
}
Write-Output (($results | Format-Table -AutoSize | Out-String).TrimEnd())
if (($results | Where-Object Mismatches -GT 0).Count) { throw "Neutral face parity failed: tolerance $Tolerance" }
Write-Output "NEUTRAL FACE PARITY PASS: $($results.Count) heads, tolerance $Tolerance; animated parity not implied."
