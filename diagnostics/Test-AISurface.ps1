param(
    [string]$RetailDirectory = 'G:\SS\Silent-Storm\Soft\Andy\RussianGold',
    [string]$CurrentBuildDirectory,
    [string]$Debugger = 'C:\Program Files (x86)\Windows Kits\10\Debuggers\x86\cdb.exe'
)

$ErrorActionPreference = 'Stop'
$repoRoot = Split-Path -Parent $PSScriptRoot
$logicSource = Join-Path $repoRoot 'Main\aiCombatLogic.cpp'
$placeSource = Join-Path $repoRoot 'Main\aiActionPlaceSource.cpp'

foreach ($required in @($Debugger, (Join-Path $RetailDirectory 'Game.exe'), $logicSource, $placeSource)) {
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
    'x Game!*CAICombatLogic*DoJob*'
    'x Game!*CAICombatLogic*GenerateCommand*'
    'x Game!*CAICombatLogic*IsEndOfTurn*'
    'x Game!*CAICombatLogic*OnNewTurn*'
    'x Game!*CreateAttackPlaceSource*'
) -join '; '

$retailSymbols = Invoke-Cdb $RetailDirectory $symbolCommand
foreach ($symbol in @('DoJob', 'GenerateCommand', 'IsEndOfTurn', 'OnNewTurn', 'CreateAttackPlaceSource')) {
    Assert-Matches $retailSymbols ([regex]::Escape($symbol)) "retail $symbol"
}

# RussianGold CAIAttackLogic::CAIAttackLogic(IAIUnit*) @0x00420ce0: the ordinary attack source
# is the (unit,14,true) overload, not the area-gated overload used by guard logic.
$retailAttackCtor = Invoke-Cdb $RetailDirectory 'uf 00420ce0'
Assert-Matches $retailAttackCtor 'mov\s+edx,0Eh[\s\S]*call\s+Game!NAI::CreateAttackPlaceSource \(0048d3e0\)' 'retail 14-AP attack-place overload'
Assert-Matches $retailAttackCtor 'push\s+ebp[\s\S]*mov\s+edx,0Eh' 'retail friendly-fire check flag'

$logic = Get-Content -LiteralPath $logicSource -Raw
$place = Get-Content -LiteralPath $placeSource -Raw

foreach ($stage in @('case PS_INIT:', 'case PS_PLACESOURCE:', 'case PS_ACTION:', 'case PS_THINK:')) {
    Assert-Matches $logic ([regex]::Escape($stage)) "AI pipeline stage $stage"
}
Assert-Matches $logic 'GetAIJobManager\s*\(\s*\)->Add|pMgr->Add\s*\(' 'AI job scheduling'
Assert-Matches $logic 'WaitForJob\s*\(' 'choose-place job dependency'
Assert-Matches $logic 'MakeDecision\s*\(\s*\)' 'AI decision dispatch'
Assert-Matches $logic 'GenerateCommand\s*\(\s*\)[\s\S]*pLog->GetWorldCommands' 'AI log-to-command drain'
Assert-Matches $logic 'OnNewTurn\s*\([^)]*CEventOnNewPlayerTurn' 'turn-change handler'
Assert-Matches $logic 'CreateAttackPlaceSource\s*\(\s*u\s*,\s*14\s*,\s*true\s*\)' 'ordinary 14-AP attack source'
if ($logic -match 'GetUnitArea\s*\(') {
    throw 'Fabricated GetUnitArea shim remains in aiCombatLogic.cpp'
}

Assert-Matches $place 'CreateAttackPlaceSource\s*\(\s*IAIUnit \*pUnit\s*,\s*int nMaxAP\s*,\s*bool bCheckDangerousForAllies\s*\)' 'explicit-budget attack-source overload'
Assert-Matches $place 'new CAIAttackPlaceSource\s*\(\s*pUnit\s*,\s*nMaxAP\s*,\s*0\s*,\s*bCheckDangerousForAllies\s*\)' 'ungated ordinary attack source'
Assert-Matches $place '!IsValid\s*\(\s*pUnit\s*\)\s*\|\|\s*!IsValid\s*\(\s*pArea\s*\)' 'guard-area validity gate'
Assert-Matches $place 'IsPosDangerousForAllies\s*\([^)]*\)[\s\S]*GetUnitsNear\s*\(' 'live friendly-fire candidate query'
Assert-Matches $place 'GetDiplomacyState\s*\(\s*pOther\s*\)\s*!=\s*NDb::DS_ALLY' 'friendly-fire ally gate'
if ($place -match 'CONSERVATIVE STUB|friendly-fire.*stub') {
    throw 'Obsolete friendly-fire stub remains in aiActionPlaceSource.cpp'
}

if ($CurrentBuildDirectory) {
    $currentSymbols = Invoke-Cdb $CurrentBuildDirectory $symbolCommand
    foreach ($symbol in @('DoJob', 'GenerateCommand', 'IsEndOfTurn', 'OnNewTurn', 'CreateAttackPlaceSource')) {
        Assert-Matches $currentSymbols ([regex]::Escape($symbol)) "current PDB $symbol"
    }
}

[pscustomobject]@{
    RetailSymbols = 5
    PipelineStages = 4
    AttackPlaceBudget = 14
    GuardAreaOverload = 'validated separately'
    CurrentPdbChecked = [bool]$CurrentBuildDirectory
} | Format-List

Write-Host 'AI-SURFACE PASS'
