param(
    [Parameter(Mandatory)][string]$TreeFile,
    [Parameter(Mandatory)][string]$HeadFile,
    [Parameter(Mandatory)][string]$SequenceFile,
    [Parameter(Mandatory)][string]$GameRoot,
    [Parameter(Mandatory)][string]$X86Probe,
    [Parameter(Mandatory)][string]$NativeDecoder,
    [string]$OutputDirectory
)
$ErrorActionPreference = 'Stop'
$tree = (Resolve-Path -LiteralPath $TreeFile).Path
$head = (Resolve-Path -LiteralPath $HeadFile).Path
$sequence = (Resolve-Path -LiteralPath $SequenceFile).Path
$game = (Resolve-Path -LiteralPath $GameRoot).Path
$x86 = (Resolve-Path -LiteralPath $X86Probe).Path
$native = (Resolve-Path -LiteralPath $NativeDecoder).Path
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
$nativeNodes = @(& $native $tree --graph)
if ($LASTEXITCODE -ne 0) { throw 'Native MMLF graph decode failed' }
$nativeSummary = @(& $native $tree)
if ($LASTEXITCODE -ne 0) { throw 'Native MMLF summary decode failed' }
$recordLine = @($nativeSummary | Where-Object { $_ -match '^records=' })
if ($recordLine.Count -ne 1 -or $recordLine[0] -notmatch '^records=(\d+),') {
    throw 'Native MMLF record count is missing'
}
$nativeRecords = [int]$Matches[1]
if ($x86Operations.Count -ne $nativeRecords) {
    throw "MMLF operation count differs: x86=$($x86Operations.Count) native=$nativeRecords"
}
if ($x86Nodes.Count -ne $nativeNodes.Count) {
    throw "MMLF macro count differs: x86=$($x86Nodes.Count) native=$($nativeNodes.Count)"
}
for ($i = 0; $i -lt $x86Nodes.Count; ++$i) {
    if ($x86Nodes[$i] -cne $nativeNodes[$i]) {
        throw "MMLF graph mismatch at row $i`: x86='$($x86Nodes[$i])' native='$($nativeNodes[$i])'"
    }
}
Write-Output "MMTREE PARITY PASS: $($x86Nodes.Count - 1) macro nodes, $nativeRecords operations; x86 repeat byte-identical."
