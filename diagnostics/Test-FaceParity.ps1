param(
    [Parameter(Mandatory)][string]$FixtureDirectory,
    [Parameter(Mandatory)][string]$GameRoot,
    [Parameter(Mandatory)][string]$X86Probe,
    [string]$X64Probe,
    [string]$OutputDirectory,
    [double]$Tolerance = 0.0001,
    [switch]$ReferenceOnly
)
$ErrorActionPreference = 'Stop'
$fixtures = (Resolve-Path -LiteralPath $FixtureDirectory).Path
$game = (Resolve-Path -LiteralPath $GameRoot).Path
$x86 = (Resolve-Path -LiteralPath $X86Probe).Path
if (!$ReferenceOnly) {
    if (!$X64Probe) { throw 'X64Probe is required unless ReferenceOnly is set' }
    $x64 = (Resolve-Path -LiteralPath $X64Probe).Path
}
if (!$OutputDirectory) { $OutputDirectory = Join-Path $fixtures 'comparison' }
$output = [IO.Path]::GetFullPath($OutputDirectory)
New-Item -ItemType Directory -Path $output -Force | Out-Null
$tree = Join-Path $game 'tree.mma'
if (!(Test-Path -LiteralPath $tree)) { throw "Missing muscle tree: $tree" }
$heads = @(Get-ChildItem -LiteralPath $fixtures -Filter 'head-*.bin' -File |
    Where-Object Name -Match '^head-\d+-\d+\.bin$' | Sort-Object Name)
$sequences = @(Get-ChildItem -LiteralPath $fixtures -Filter 'sequence-*.bin' -File |
    Where-Object Name -Match '^sequence-\d+-\d+\.bin$' | Sort-Object Name)
if (!$heads.Count -or !$sequences.Count) { throw 'Need at least one extracted head and sequence stream' }
$culture = [Globalization.CultureInfo]::InvariantCulture
$priorPath = $env:PATH
$priorTracePath = $env:S2_FACE_MUSCLE_TRACE_PATH
$results = @()
$animatedCases = 0
$totalMuscleCalls = 0
try {
    # The x86 executable imports the owner's original DLL from the isolated
    # installation; no proprietary binary or fixture is checked into Git.
    $env:PATH = "$game;$priorPath"
    foreach ($head in $heads) {
        foreach ($sequence in $sequences) {
            $name = "$($head.BaseName)--$($sequence.BaseName)"
            $reference = Join-Path $output "$name-x86.csv"
            $trace = Join-Path $output "$name-x86-muscles.csv"
            $env:S2_FACE_MUSCLE_TRACE_PATH = $trace
            & $x86 $head.FullName $reference $sequence.FullName $tree
            if ($LASTEXITCODE -ne 0) { throw "x86 oracle failed ($LASTEXITCODE): $name" }
            $repeat = Join-Path $output "$name-x86-repeat.csv"
            $traceRepeat = Join-Path $output "$name-x86-muscles-repeat.csv"
            $env:S2_FACE_MUSCLE_TRACE_PATH = $traceRepeat
            & $x86 $head.FullName $repeat $sequence.FullName $tree
            if ($LASTEXITCODE -ne 0) { throw "x86 oracle repeat failed ($LASTEXITCODE): $name" }
            $env:S2_FACE_MUSCLE_TRACE_PATH = $null
            if ((Get-FileHash -LiteralPath $reference).Hash -ne (Get-FileHash -LiteralPath $repeat).Hash) {
                throw "x86 oracle is nondeterministic: $name"
            }
            if ((Get-FileHash -LiteralPath $trace).Hash -ne (Get-FileHash -LiteralPath $traceRepeat).Hash) {
                throw "x86 muscle trace is nondeterministic: $name"
            }
            $muscleRows = @(Import-Csv -LiteralPath $trace)
            $totalMuscleCalls += $muscleRows.Count
            $referenceRows = @(Import-Csv -LiteralPath $reference)
            if (!$referenceRows.Count) { throw "x86 oracle returned no vertices: $name" }
            $neutral = @{}
            foreach ($row in $referenceRows) {
                if ($row.case -eq 'neutral') { $neutral[$row.vertex] = $row }
            }
            $moves = $false
            foreach ($row in $referenceRows) {
                if ($row.case -ne 'sequence') { continue }
                $origin = $neutral[$row.vertex]
                if (!$origin) { throw "Missing neutral vertex: $name $($row.vertex)" }
                foreach ($axis in 'x','y','z') {
                    if ([Math]::Abs([double]::Parse($origin.$axis, $culture) -
                                    [double]::Parse($row.$axis, $culture)) -gt $Tolerance) {
                        $moves = $true
                    }
                }
            }
            if ($moves) { ++$animatedCases }
            $record = [ordered]@{
                Case = $name
                HeadSha256 = (Get-FileHash -LiteralPath $head.FullName).Hash
                SequenceSha256 = (Get-FileHash -LiteralPath $sequence.FullName).Hash
                ReferenceSha256 = (Get-FileHash -LiteralPath $reference).Hash
                MuscleTraceSha256 = (Get-FileHash -LiteralPath $trace).Hash
                MuscleCalls = $muscleRows.Count
                Rows = $referenceRows.Count
                MaximumDelta = $null
                Mismatches = $null
            }
            if (!$ReferenceOnly) {
                $candidate = Join-Path $output "$name-x64.csv"
                & $x64 $head.FullName $candidate $sequence.FullName $tree
                if ($LASTEXITCODE -ne 0) { throw "x64 native probe failed ($LASTEXITCODE): $name" }
                $candidateRows = @(Import-Csv -LiteralPath $candidate)
                if ($candidateRows.Count -ne $referenceRows.Count) {
                    throw "Different vertex-row count: $name x86=$($referenceRows.Count) x64=$($candidateRows.Count)"
                }
                $maximum = 0.0
                $mismatches = 0
                for ($i = 0; $i -lt $referenceRows.Count; ++$i) {
                    $a = $referenceRows[$i]
                    $b = $candidateRows[$i]
                    if ($a.case -ne $b.case -or $a.time -ne $b.time -or $a.vertex -ne $b.vertex) {
                        throw "Different row identity at $name row $i"
                    }
                    foreach ($axis in 'x','y','z') {
                        $left = [double]::Parse($a.$axis, $culture)
                        $right = [double]::Parse($b.$axis, $culture)
                        if ([double]::IsNaN($right) -or [double]::IsInfinity($right)) {
                            throw "Nonfinite x64 coordinate: $name row $i axis $axis"
                        }
                        $delta = [Math]::Abs($left - $right)
                        $maximum = [Math]::Max($maximum, $delta)
                        if ($delta -gt $Tolerance) { ++$mismatches }
                    }
                }
                $record.MaximumDelta = $maximum
                $record.Mismatches = $mismatches
            }
            $results += [pscustomobject]$record
        }
    }
}
finally {
    $env:PATH = $priorPath
    $env:S2_FACE_MUSCLE_TRACE_PATH = $priorTracePath
}
Write-Output (($results | Format-Table Case,Rows,MuscleCalls,MaximumDelta,Mismatches -AutoSize | Out-String).TrimEnd())
if (!$animatedCases) { throw 'Oracle corpus has no animated vertex; add a moving sequence' }
if (!$totalMuscleCalls) { throw 'Oracle corpus has no macro-muscle calls; add a sequence exercising the sequencer' }
if ($ReferenceOnly) {
    Write-Output "REFERENCE ONLY: $($results.Count) deterministic x86 cases, $animatedCases animated; no x64 parity claim."
} elseif (($results | Where-Object Mismatches -GT 0).Count) {
    throw "Face parity failed: tolerance $Tolerance"
} else {
    Write-Output "FACE PARITY PASS: $($results.Count) cases, tolerance $Tolerance"
}
