param(
    [Parameter(Mandatory)][string]$FixtureDirectory,
    [Parameter(Mandatory)][string]$NativeHeadDecode,
    [double]$RawTolerance = 0.02,
    [double]$ImplicitTolerance = 0.0001
)
$ErrorActionPreference = 'Stop'
$fixtures = (Resolve-Path -LiteralPath $FixtureDirectory).Path
$decoder = (Resolve-Path -LiteralPath $NativeHeadDecode).Path
$referenceDirectory = Join-Path $fixtures 'comparison'
$heads = @(Get-ChildItem -LiteralPath $fixtures -Filter 'head-*.bin' -File |
    Where-Object Name -Match '^head-\d+-\d+\.bin$' | Sort-Object Name)
if (!$heads.Count) { throw 'No original head streams found' }
$summary = @()
foreach ($head in $heads) {
    $reference = Get-ChildItem -LiteralPath $referenceDirectory -Filter "$($head.BaseName)--sequence-*-x86.csv" -File |
        Sort-Object Name | Select-Object -First 1
    if (!$reference) { throw "No x86 reference for $($head.Name); run Test-FaceParity.ps1 -ReferenceOnly first" }
    $output = Join-Path $referenceDirectory "$($head.BaseName)-native-raw.csv"
    & $decoder $head.FullName $output
    if ($LASTEXITCODE -ne 0) { throw "Native decoder failed for $($head.Name)" }
    $native = @(Import-Csv -LiteralPath $output)
    $oracle = @(Import-Csv -LiteralPath $reference.FullName | Where-Object case -eq neutral)
    if ($native.Count -ne $oracle.Count) {
        throw "Vertex count differs for $($head.Name): native=$($native.Count), x86=$($oracle.Count)"
    }
    $seen = @{}
    $maxExplicit = 0.0
    $maxImplicit = 0.0
    $explicit = 0
    $implicit = 0
    foreach ($row in $native) {
        $index = [int]$row.vertex
        if ($index -lt 0 -or $index -ge $oracle.Count -or $seen.ContainsKey($index)) {
            throw "Invalid/duplicate vertex $index in $($head.Name)"
        }
        $seen[$index] = $true
        if ($row.kind -eq 'explicit') { ++$explicit }
        elseif ($row.kind -eq 'implicit') { ++$implicit }
        else { throw "Unknown native vertex kind: $($row.kind)" }
        foreach ($axis in 'x','y','z') {
            $left = [double]::Parse($row.$axis, [Globalization.CultureInfo]::InvariantCulture)
            $right = [double]::Parse($oracle[$index].$axis, [Globalization.CultureInfo]::InvariantCulture)
            if ([double]::IsNaN($left) -or [double]::IsInfinity($left)) { throw "Nonfinite native vertex $index" }
            $delta = [Math]::Abs($left - $right)
            if ($row.kind -eq 'explicit') { $maxExplicit = [Math]::Max($maxExplicit, $delta) }
            else { $maxImplicit = [Math]::Max($maxImplicit, $delta) }
        }
    }
    if ($maxExplicit -gt $RawTolerance -or $maxImplicit -gt $ImplicitTolerance) {
        throw "Raw decode mismatch for $($head.Name): explicit max=$maxExplicit implicit max=$maxImplicit"
    }
    $summary += [pscustomobject]@{
        Head = $head.Name
        Explicit = $explicit
        Implicit = $implicit
        MaximumExplicitDelta = $maxExplicit
        MaximumImplicitDelta = $maxImplicit
    }
}
Write-Output (($summary | Format-Table -AutoSize | Out-String).TrimEnd())
Write-Output "RAW HEAD DECODE PASS: $($summary.Count) heads. This is not animated x64 parity."
