param(
    [string]$RetailDirectory = 'G:\SS\Silent-Storm\Soft\Andy\RussianGold',
    [string]$CurrentBuildDirectory,
    [string]$Debugger = 'C:\Program Files (x86)\Windows Kits\10\Debuggers\x86\cdb.exe'
)

$ErrorActionPreference = 'Stop'
$repoRoot = Split-Path -Parent $PSScriptRoot
$globalSource = Join-Path $repoRoot 'Main\iGlobalMap.cpp'
$globalUiSource = Join-Path $repoRoot 'Main\iGlobalMapUI.h'
$missionSource = Join-Path $repoRoot 'Main\iMission.h'
$scenarioSource = Join-Path $repoRoot 'Main\scScenarioTracker.cpp'
$inventorySource = Join-Path $repoRoot 'Main\RPGInventory.cpp'

foreach ($required in @($Debugger, (Join-Path $RetailDirectory 'Game.exe'), $globalSource,
        $globalUiSource, $missionSource, $scenarioSource, $inventorySource)) {
    if (-not (Test-Path -LiteralPath $required)) {
        throw "Required input not found: $required"
    }
}

function Invoke-Cdb([string]$directory, [string]$commands) {
    $exe = Join-Path $directory 'Game.exe'
    if (-not (Test-Path -LiteralPath $exe)) {
        throw "Game.exe not found in $directory"
    }
    (& $Debugger -z $exe -y $directory -c "$commands; q" 2>&1) -join "`n"
}

function Assert-Matches([string]$text, [string]$pattern, [string]$description) {
    if ($text -notmatch $pattern) {
        throw "Missing $description ($pattern)"
    }
}

$symbolCommand = @(
    'dt Game!NGame::CGlobalMap'
    'dt Game!NUI::CGlobalMapUI'
    'x Game!NGame::CGlobalMap::*'
    'x Game!NScenario::*GetGoalByID*'
    'x Game!NScenario::CScenarioTracker::ScriptGoalSetComplete'
    'x Game!NScenario::CScenarioTracker::ScriptTaskSetComplete'
    'x Game!NRPG::CInventory::*'
) -join '; '

$retail = Invoke-Cdb $RetailDirectory $symbolCommand
foreach ($layout in @(
    '\+0x108 bindClose',
    '\+0x138 bShowMode',
    '\+0x13c pGlobalMap',
    '\+0x140 pGlobalInfo',
    '\+0x148 pGlobalMapUI',
    'pGlobal\s+: CPtr<NGame::IMission>'
)) {
    Assert-Matches $retail $layout "retail global-map layout $layout"
}
foreach ($symbol in @('IsGlobalMapShowMode', 'GetGlobalMap', 'GetGlobalInfo', 'GetGoalByID',
        'ScriptGoalSetComplete', 'ScriptTaskSetComplete', 'CanPlace', 'FindPlace', 'Equip', 'TakeOff')) {
    Assert-Matches $retail ([regex]::Escape($symbol)) "retail campaign/inventory symbol $symbol"
}

$global = Get-Content -LiteralPath $globalSource -Raw
$globalUi = Get-Content -LiteralPath $globalUiSource -Raw
$mission = Get-Content -LiteralPath $missionSource -Raw
$scenario = Get-Content -LiteralPath $scenarioSource -Raw
$inventory = Get-Content -LiteralPath $inventorySource -Raw

Assert-Matches $global 'class\s+CGlobalMap\s*:\s*public\s+CMissionBase' 'global map CMissionBase inheritance'
Assert-Matches $global 'f\.Add\(1,\(CMissionBase\*\)this\)' 'complete global-map base serialization'
if ($global -match 'struct\s+SBaseChunk') {
    throw 'Obsolete partial global-map SBaseChunk remains'
}
Assert-Matches $globalUi 'CPtr<NGame::IMission>\s+pGlobal' 'global-map UI mission pointer'
foreach ($method in @('IsGlobalMapShowMode', 'GetGlobalMap', 'GetGlobalInfo')) {
    Assert-Matches $mission ([regex]::Escape("$method() const")) "common mission method $method"
}
Assert-Matches $scenario 'FindGoal\s*\([^)]*\)[\s\S]*FindScriptGoal[\s\S]*FindClueGoal' 'script-to-clue goal fallback'
Assert-Matches $scenario 'FindClueGoal[\s\S]*GetClues\s*\([\s\S]*GetGoal\s*\(' 'clue goal lookup'
Assert-Matches $scenario 'ScriptGoalSetComplete[\s\S]*InvalidateLeaveZoneCache\s*\(\)[\s\S]*FindGoal' 'goal cache invalidation and lookup order'
Assert-Matches $scenario 'ScriptTaskSetComplete[\s\S]*InvalidateLeaveZoneCache\s*\(\)[\s\S]*FindGoal' 'task cache invalidation and lookup order'
Assert-Matches $inventory 'f\.Add\(2,&nActiveSlot\)[\s\S]*f\.Add\(8,&pPK\)' 'retail inventory wire'
Assert-Matches $inventory 'CanEquip[\s\S]*CDynamicCast<IClipItem>[\s\S]*CDynamicCast<IDummyItem>' 'inventory equip gates'

if ($CurrentBuildDirectory) {
    $current = Invoke-Cdb $CurrentBuildDirectory $symbolCommand
    foreach ($symbol in @('CGlobalMap::IsGlobalMapShowMode', 'CGlobalMap::GetGlobalMap',
            'CGlobalMap::GetGlobalInfo', 'CScenarioTracker::ScriptGoalSetComplete',
            'CScenarioTracker::ScriptTaskSetComplete', 'CInventory::CanPlace', 'CInventory::Equip')) {
        Assert-Matches $current ([regex]::Escape($symbol)) "current PDB $symbol"
    }
    Assert-Matches $current 'pGlobal\s+: CPtr<NGame::IMission>' 'current global-map UI mission pointer'
    Assert-Matches $current 'pGlobalGame\s+: CPtr<NRPG::CGlobalGame>' 'current inherited global-game storage'
}

[pscustomobject]@{
    GlobalMapBaseWire = 'CMissionBase tags 2..38'
    GlobalMapOwnWire = 'show/map/info/ui tags 2..5'
    GoalLookup = 'script goal, then clue goal'
    InventorySurface = 'place/equip/take-off + retail wire'
    CurrentPdbChecked = [bool]$CurrentBuildDirectory
} | Format-List

Write-Host 'CAMPAIGN-SURFACE PASS'
