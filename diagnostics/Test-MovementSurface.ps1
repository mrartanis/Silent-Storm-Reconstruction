param(
    [string]$RetailDirectory = 'G:\SS\Silent-Storm\Soft\Andy\RussianGold',
    [string]$CurrentBuildDirectory,
    [string]$Debugger = 'C:\Program Files (x86)\Windows Kits\10\Debuggers\x86\cdb.exe'
)

$ErrorActionPreference = 'Stop'
$repoRoot = Split-Path -Parent $PSScriptRoot
$moveSource = Join-Path $repoRoot 'Main\wUnitMove.cpp'
$scriptCommon = Join-Path $repoRoot 'Main\scriptCommon.cpp'
$scriptSequence = Join-Path $repoRoot 'Main\scriptSequence.cpp'

foreach ($required in @($Debugger, (Join-Path $RetailDirectory 'Game.exe'), $moveSource, $scriptCommon, $scriptSequence)) {
    if (-not (Test-Path -LiteralPath $required)) {
        throw "Required input not found: $required"
    }
}

function Get-MovementSymbols([string]$directory) {
    $exe = Join-Path $directory 'Game.exe'
    if (-not (Test-Path -LiteralPath $exe)) {
        throw "Game.exe not found in $directory"
    }
    (& $Debugger -z $exe -y $directory -c 'dt Game!NWorld::CPathConflictsRemover; x Game!NWorld::CPathConflictsRemover::*; dt Game!NWorld::CPathAPCalcer; q' 2>&1) -join "`n"
}

function Assert-Matches([string]$text, [string]$pattern, [string]$description) {
    if ($text -notmatch $pattern) {
        throw "Missing $description ($pattern)"
    }
}

$retail = Get-MovementSymbols $RetailDirectory
$expectedMembers = @(
    'GetWhoLocks',
    'TryToSetNewPath',
    'UseAnotherPCR',
    'CheckLockerState',
    'CheckCanDoMove',
    'Segment'
)
foreach ($member in $expectedMembers) {
    Assert-Matches $retail ([regex]::Escape("CPathConflictsRemover::$member")) "retail $member"
}

# Retail ABI: CCommandExecute data ends at +0x14, IExecMove contributes the second vtable at +0x18,
# then the serialized wait-state begins at +0x1c. This is the boundary that was lost in the dev tree.
foreach ($layout in @(
    '\+0x018 __VFN_table',
    '\+0x01c bWaiting',
    '\+0x01d bAfterWaiting',
    '\+0x020 nTimeToWait',
    '\+0x024 posToWait'
)) {
    Assert-Matches $retail $layout "retail path-conflict layout $layout"
}

$source = Get-Content -LiteralPath $moveSource -Raw
Assert-Matches $source 'class\s+CPathConflictsRemover\s*:\s*public\s+CCommandExecute\s*,\s*public\s+IExecMove' 'IExecMove on CPathConflictsRemover'
foreach ($member in $expectedMembers) {
    Assert-Matches $source ([regex]::Escape("CPathConflictsRemover::$member")) "current $member implementation"
}
Assert-Matches $source 'class\s+CPathAPCalcer' 'path AP accumulator'
Assert-Matches $source 'GetMoveActionType\s*\(' 'movement action pricing'
Assert-Matches $source 'pRPG->GetActionAP\s*\(' 'RPG AP pricing'

$commonText = Get-Content -LiteralPath $scriptCommon -Raw
$sequenceText = Get-Content -LiteralPath $scriptSequence -Raw
Assert-Matches $commonText 'BEGIN_SCRIPT_COMMAND\(\s*PassCalcerIsActive' 'PassCalcerIsActive Lua command'
Assert-Matches $commonText 'HasPassCalcerJobs\s*\(' 'live pass-calculation state query'
Assert-Matches $sequenceText 'BEGIN_SCRIPT_COMMAND\(\s*SlowSyncAIMap' 'SlowSyncAIMap Lua command'
Assert-Matches $sequenceText 'GetAIMap\(\)->Sync\s*\(' 'AI-map synchronization'

if ($CurrentBuildDirectory) {
    $current = Get-MovementSymbols $CurrentBuildDirectory
    foreach ($layout in @(
        '\+0x018 __VFN_table',
        '\+0x01c bWaiting',
        '\+0x01d bAfterWaiting',
        '\+0x020 nTimeToWait',
        '\+0x024 posToWait'
    )) {
        Assert-Matches $current $layout "current path-conflict layout $layout"
    }
    foreach ($member in $expectedMembers) {
        Assert-Matches $current ([regex]::Escape("CPathConflictsRemover::$member")) "current PDB $member"
    }
}

[pscustomobject]@{
    RetailHandlers = $expectedMembers.Count
    RetailLayout = 'IExecMove@0x18 wait-state@0x1c..0x24'
    CurrentPdbChecked = [bool]$CurrentBuildDirectory
    APAccumulator = 'GetMoveActionType + RPG GetActionAP'
    ScriptBarriers = 'PassCalcerIsActive + SlowSyncAIMap'
} | Format-List

Write-Host 'MOVEMENT-SURFACE PASS'
