param(
    [Parameter(Mandatory)][string]$TreeFile,
    [Parameter(Mandatory)][string]$HeadFile,
    [Parameter(Mandatory)][string]$SequenceFile,
    [Parameter(Mandatory)][string]$GameRoot,
    [Parameter(Mandatory)][string]$X86Probe,
    [Parameter(Mandatory)][string]$NativeDecoder,
    [string]$NativeApiCheck,
    [string]$OutputDirectory
)
$ErrorActionPreference = 'Stop'
$tree = (Resolve-Path -LiteralPath $TreeFile).Path
$head = (Resolve-Path -LiteralPath $HeadFile).Path
$sequence = (Resolve-Path -LiteralPath $SequenceFile).Path
$game = (Resolve-Path -LiteralPath $GameRoot).Path
$x86 = (Resolve-Path -LiteralPath $X86Probe).Path
$native = (Resolve-Path -LiteralPath $NativeDecoder).Path
if ($NativeApiCheck) { $nativeApi = (Resolve-Path -LiteralPath $NativeApiCheck).Path }
if (!$OutputDirectory) { $OutputDirectory = Join-Path (Split-Path -Parent $head) 'tree-comparison' }
$output = [IO.Path]::GetFullPath($OutputDirectory)
New-Item -ItemType Directory -Path $output -Force | Out-Null
$originalPath = Join-Path $output 'x86-tree.csv'
$repeatPath = Join-Path $output 'x86-tree-repeat.csv'
$vertexPath = Join-Path $output 'x86-vertices.csv'
$priorPath = $env:PATH
$priorDump = $env:S2_FACE_TREE_DUMP_PATH
try {
    $env:PATH = "$game;$priorPath"
    foreach ($dump in @($originalPath, $repeatPath)) {
        $env:S2_FACE_TREE_DUMP_PATH = $dump
        & $x86 $head $vertexPath $sequence $tree
        if ($LASTEXITCODE -ne 0 -or !(Test-Path -LiteralPath $dump)) {
            throw "x86 tree dump failed: $dump"
        }
    }
} finally {
    $env:PATH = $priorPath
    $env:S2_FACE_TREE_DUMP_PATH = $priorDump
}
if ((Get-FileHash -LiteralPath $originalPath).Hash -ne
    (Get-FileHash -LiteralPath $repeatPath).Hash) {
    throw 'x86 macro graph is not byte-identical on repeat'
}
$x86Lines = @(Get-Content -LiteralPath $originalPath)
$x86Nodes = @($x86Lines | Where-Object { $_ -match '^(depth,|[0-9]+,)' })
$x86Operations = @($x86Lines | Where-Object { $_ -match '^operation,' })
$x86Targets = @($x86Lines | Where-Object { $_ -match '^operation-target,' } |
    ForEach-Object { ($_ -split ',', 4)[3] })
$x86Lookups = @($x86Lines | Where-Object { $_ -match '^lookup,' })
if ($x86Lookups.Count -ne ($x86Nodes.Count - 1) -or
    @($x86Lookups | Where-Object { $_ -notmatch ',1$' }).Count) {
    throw 'Original x86 FindMacroMuscle lookup differs from visited macro graph'
}
$nativeNodes = @(& $native $tree --graph)
if ($LASTEXITCODE -ne 0) { throw 'Native MMLF graph decode failed' }
$nativeSummary = @(& $native $tree)
if ($LASTEXITCODE -ne 0) { throw 'Native MMLF summary decode failed' }
$recordLine = @($nativeSummary | Where-Object { $_ -match '^records=' })
if ($recordLine.Count -ne 1 -or $recordLine[0] -notmatch '^records=(\d+),') {
    throw 'Native MMLF record count is missing'
}
$nativeRecords = [int]$Matches[1]
$nativeRows = @(& $native $tree --records | ConvertFrom-Csv)
if ($LASTEXITCODE -ne 0) { throw 'Native MMLF records decode failed' }
if ($x86Operations.Count -ne $nativeRecords) {
    throw "MMLF operation count differs: x86=$($x86Operations.Count) native=$nativeRecords"
}
if ($nativeRows.Count -ne $nativeRecords) {
    throw "Native MMLF records list differs from summary: $($nativeRows.Count) vs $nativeRecords"
}
for ($i = 0; $i -lt $nativeRecords; ++$i) {
    if ($x86Operations[$i] -notmatch 'type=(\d+)') { throw "Missing x86 operation type at row $i" }
    $runtimeType = [int]$Matches[1]
    $expectedKind = if ($runtimeType -eq 4) { 1 } elseif ($runtimeType -eq 0) { 3 } else { 4 }
    if ($runtimeType -gt 4 -or [int]$nativeRows[$i].h0 -ne $expectedKind) {
        throw "MMLF operation kind mismatch at row $i`: x86=$runtimeType native=$($nativeRows[$i].h0)"
    }
}
$nativeTargets = @($nativeRows | Where-Object { $_.h0 -eq '1' } |
    ForEach-Object { $_.'resolved-name' })
if ($x86Targets.Count -ne $nativeTargets.Count) {
    throw "MMLF macro-target count differs: x86=$($x86Targets.Count) native=$($nativeTargets.Count)"
}
for ($i = 0; $i -lt $x86Targets.Count; ++$i) {
    if ($x86Targets[$i] -cne $nativeTargets[$i]) {
        throw "MMLF macro target mismatch at row $i`: x86='$($x86Targets[$i])' native='$($nativeTargets[$i])'"
    }
}
if ($nativeApi) {
    $apiResult = @(& $nativeApi $tree)
    if ($LASTEXITCODE -ne 0 -or $apiResult.Count -ne 1 -or
        $apiResult[0] -notmatch '^macro-nodes=(\d+),macro-targets=(\d+)$') {
        throw 'Native MMLF API lookup check failed'
    }
    if ([int]$Matches[1] -ne ($x86Nodes.Count - 1) -or
        [int]$Matches[2] -ne $x86Targets.Count) {
        throw "Native MMLF API lookup counts differ: $($apiResult[0])"
    }
}
if ($x86Nodes.Count -ne $nativeNodes.Count) {
    throw "MMLF macro count differs: x86=$($x86Nodes.Count) native=$($nativeNodes.Count)"
}
for ($i = 0; $i -lt $x86Nodes.Count; ++$i) {
    if ($x86Nodes[$i] -cne $nativeNodes[$i]) {
        throw "MMLF graph mismatch at row $i`: x86='$($x86Nodes[$i])' native='$($nativeNodes[$i])'"
    }
}
Write-Output "MMTREE PARITY PASS: $($x86Nodes.Count - 1) macro nodes, $nativeRecords operation kinds, $($x86Targets.Count) macro targets; x86 repeat byte-identical."
