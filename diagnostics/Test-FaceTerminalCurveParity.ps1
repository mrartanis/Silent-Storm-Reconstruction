param(
    [Parameter(Mandatory)][string]$FixtureDirectory,
    [Parameter(Mandatory)][string]$HeadFile,
    [Parameter(Mandatory)][string]$GameRoot,
    [Parameter(Mandatory)][string]$X86Probe,
    [Parameter(Mandatory)][string]$X64Probe,
    [Parameter(Mandatory)][string]$NativeSequenceDecode,
    [Parameter(Mandatory)][string]$NativeSequenceEvaluate,
    [Parameter(Mandatory)][string]$OutputDirectory
)
$ErrorActionPreference = 'Stop'
$fixtures = (Resolve-Path -LiteralPath $FixtureDirectory).Path
$head = (Resolve-Path -LiteralPath $HeadFile).Path
$game = (Resolve-Path -LiteralPath $GameRoot).Path
$x86 = (Resolve-Path -LiteralPath $X86Probe).Path
$x64 = (Resolve-Path -LiteralPath $X64Probe).Path
$decoder = (Resolve-Path -LiteralPath $NativeSequenceDecode).Path
$evaluator = (Resolve-Path -LiteralPath $NativeSequenceEvaluate).Path
$output = [IO.Path]::GetFullPath($OutputDirectory)
if (Test-Path -LiteralPath $output) { throw "Output already exists: $output" }
$files = @(Get-ChildItem -LiteralPath $fixtures -Filter 'sequence-*.bin' -File | Sort-Object Name)
if (!$files.Count) { throw 'No sequence fixtures' }
$anomalies = [System.Collections.Generic.List[object]]::new()
for ($i = 0; $i -lt $files.Count; $i += 100) {
    $last = [Math]::Min($i + 99,$files.Count - 1)
    $paths = @($files[$i..$last] | ForEach-Object FullName)
    $lines = @(& $decoder --curve-audit @paths)
    if ($LASTEXITCODE -ne 0) { throw "Curve audit failed at input $i" }
    foreach ($line in $lines) {
        if ($line -notmatch '^curve-anomaly,file=(.*),track=(.*),event=([^,]+),start=(\d+),duration=(\d+),sequence=(\d+),index=(\d+),prev=([^,]+),current=([^,]+)$') {
            if ($line -notmatch '^curve-audit,') { throw "Unexpected curve audit row: $line" }
            continue
        }
        $anomalies.Add([pscustomobject]@{
            File=$Matches[1]; Track=$Matches[2]; Event=$Matches[3]
            Start=[int]$Matches[4]; Duration=[int]$Matches[5]
            SequenceDuration=[int]$Matches[6]; Index=[int]$Matches[7]
            Previous=$Matches[8]; Current=$Matches[9]
        })
    }
}
if (!$anomalies.Count) { throw 'No nonincreasing terminal curves found' }
New-Item -ItemType Directory -Path $output | Out-Null
$anomalies | Export-Csv -LiteralPath (Join-Path $output 'anomalies.csv') -NoTypeInformation
$results = [System.Collections.Generic.List[object]]::new()
$priorTimes = $env:S2_FACE_SAMPLE_TIMES
try {
    foreach ($group in ($anomalies | Group-Object File | Sort-Object Name)) {
        $sequence = (Resolve-Path -LiteralPath $group.Name).Path
        $stem = [IO.Path]::GetFileNameWithoutExtension($sequence)
        $caseRoot = Join-Path $output $stem
        $caseFixtures = Join-Path $caseRoot 'fixtures'
        New-Item -ItemType Directory -Path $caseFixtures | Out-Null
        Copy-Item -LiteralPath $head,$sequence -Destination $caseFixtures
        $times = [System.Collections.Generic.HashSet[int]]::new()
        [void]$times.Add(0)
        foreach ($anomaly in $group.Group) {
            foreach ($fraction in 0.0,0.25,0.5,0.75,1.0) {
                $time = $anomaly.Start + [int][Math]::Floor(($anomaly.Duration - 1) * $fraction)
                if ($time -ge 0 -and $time -lt $anomaly.SequenceDuration) {
                    [void]$times.Add($time)
                }
            }
        }
        $env:S2_FACE_SAMPLE_TIMES = (@($times | Sort-Object) -join ',')
        $parityOutput = Join-Path $caseRoot 'comparison'
        $report = & (Join-Path $PSScriptRoot 'Test-FaceParity.ps1') `
            -FixtureDirectory $caseFixtures -GameRoot $game -X86Probe $x86 -X64Probe $x64 `
            -NativeSequenceDecode $decoder -NativeSequenceEvaluate $evaluator `
            -OutputDirectory $parityOutput -AllowStaticCorpus
        if ($LASTEXITCODE -and $LASTEXITCODE -ne 0) { throw "Parity subprocess failed: $stem" }
        if (($report -join "`n") -notmatch 'FACE PARITY PASS: 1 cases') {
            throw "Terminal curve parity failed: $stem`n$($report -join "`n")"
        }
        $prefix = Join-Path $parityOutput "$([IO.Path]::GetFileNameWithoutExtension($head))--$stem"
        $trace = @(Import-Csv -LiteralPath "$prefix-x86-muscles.csv")
        $map = @{}
        foreach ($row in @(Import-Csv -LiteralPath "$prefix-x86-muscle-names.csv")) {
            $map[$row.id] = $row.name
        }
        foreach ($anomaly in $group.Group) {
            $seen = @($trace | Where-Object {
                $map[$_.muscle] -ceq $anomaly.Event -and
                [int]$_.time -ge $anomaly.Start -and
                [int]$_.time -lt ($anomaly.Start + $anomaly.Duration)
            }).Count -gt 0
            $results.Add([pscustomobject]@{
                Sequence=$stem; Event=$anomaly.Event; Start=$anomaly.Start
                Duration=$anomaly.Duration; SampleTimes=$times.Count
                X86EventObserved=$seen
            })
        }
        Write-Host "${stem}: $($group.Count) terminal curve(s), $($times.Count) sample times, parity pass"
    }
} finally { $env:S2_FACE_SAMPLE_TIMES = $priorTimes }
$results | Export-Csv -LiteralPath (Join-Path $output 'results.csv') -NoTypeInformation
$unobserved = @($results | Where-Object { !$_.X86EventObserved })
Write-Host "TERMINAL CURVE PARITY PASS: $($results.Count) anomalies across $(($results | Select-Object -ExpandProperty Sequence -Unique).Count) sequences; $($unobserved.Count) target events not seen in x86 sampled calls"
if ($unobserved.Count) { throw 'Some anomalous events were not exercised by the x86 sample trace' }
