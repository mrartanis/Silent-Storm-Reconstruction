param(
    [string]$RetailDirectory = 'G:\SS\Silent-Storm\Soft\Andy\RussianGold',
    [string]$CurrentBuildDirectory,
    [string]$Debugger = 'C:\Program Files (x86)\Windows Kits\10\Debuggers\x86\cdb.exe'
)

$ErrorActionPreference = 'Stop'
$repoRoot = Split-Path -Parent $PSScriptRoot
$toHitSource = Join-Path $repoRoot 'Main\RPGToHit.cpp'
$attackSource = Join-Path $repoRoot 'Main\wUnitAttackExec.cpp'

foreach ($required in @($Debugger, (Join-Path $RetailDirectory 'Game.exe'), $toHitSource, $attackSource)) {
    if (-not (Test-Path -LiteralPath $required)) {
        throw "Required input not found: $required"
    }
}

function Get-CombatSymbols([string]$directory) {
    $exe = Join-Path $directory 'Game.exe'
    if (-not (Test-Path -LiteralPath $exe)) {
        throw "Game.exe not found in $directory"
    }
    (& $Debugger -z $exe -y $directory -c 'x Game!*GetAuraAdd*; x Game!*CheckAuraPerk*; x Game!*GetWeatherPenalty*; x Game!*GetAuraToHitAdd*; x Game!*GetTAuraEvasionAdd*; q' 2>&1) -join "`n"
}

function Assert-Matches([string]$text, [string]$pattern, [string]$description) {
    if ($text -notmatch $pattern) {
        throw "Missing $description ($pattern)"
    }
}

$retail = Get-CombatSymbols $RetailDirectory
foreach ($symbol in @('GetAuraAdd', 'CheckAuraPerk', 'GetWeatherPenalty', 'GetAuraToHitAdd', 'GetTAuraEvasionAdd')) {
    Assert-Matches $retail ([regex]::Escape($symbol)) "retail $symbol"
}

$retailDisassembly = (& $Debugger -z (Join-Path $RetailDirectory 'Game.exe') -y $RetailDirectory -c 'uf Game!GetAuraAdd; uf Game!NRPG::CToHitCalcer::GetWeatherPenalty; q' 2>&1) -join "`n"
foreach ($instruction in @(
    'push\s+40480000h', # radius 3.125f
    'push\s+1Ch',       # allied aura
    'push\s+42h',       # owner aura with same-player ally
    'push\s+5Dh'        # owner aura without same-player ally
)) {
    Assert-Matches $retailDisassembly $instruction "retail aura constant $instruction"
}
Assert-Matches $retailDisassembly 'GetWeatherPenalty[\s\S]*call\s+dword ptr \[edx\+1C0h\]' 'retail weather query'

$toHit = Get-Content -LiteralPath $toHitSource -Raw
Assert-Matches $toHit 'GetUnitsNear\s*\([^;]*3\.125f\s*\)' 'nearby-unit aura radius'
Assert-Matches $toHit 'GetDiplomacyState\s*\(\s*pOther\s*\)\s*!=\s*NDb::DS_ALLY' 'allied-unit aura gate'
foreach ($perk in @('0x1c', '0x42', '0x5d')) {
    Assert-Matches $toHit ([regex]::Escape($perk)) "aura perk $perk"
}
Assert-Matches $toHit 'GetWeather\s*\(\s*\)\s*!=\s*NWorld::IWorld::WEATHER_SUNNY' 'non-sunny weather penalty gate'
Assert-Matches $toHit 'return\s+5\.0f' 'five-point weather penalty'
Assert-Matches $toHit 'GetAuraAdd\s*\(\s*&fToHit\s*,\s*&fEvasion\s*,\s*pUnitServer\s*\)' 'attacker aura application'
Assert-Matches $toHit 'GetAuraAdd\s*\(\s*&fToHit\s*,\s*&fEvasion\s*,\s*pTarget\s*\)' 'target evasion aura application'
if ($toHit -match 'aura subsystem absent|weather subsystem absent') {
    throw 'Obsolete aura/weather stub remains in RPGToHit.cpp'
}

$attack = Get-Content -LiteralPath $attackSource -Raw
Assert-Matches $attack 'CExecShoot::CheckUnhide\s*\(\s*\)[\s\S]*?IsHiding\s*\(' 'concealed-shooter check'
Assert-Matches $attack 'fSilencer\s*>=\s*1\.0f' 'unsilenced-weapon reveal gate'
Assert-Matches $attack 'Hide\s*\(\s*false\s*,\s*false\s*\)' 'shooter reveal action'

if ($CurrentBuildDirectory) {
    $current = Get-CombatSymbols $CurrentBuildDirectory
    foreach ($symbol in @('GetAuraAdd', 'CheckAuraPerk', 'GetWeatherPenalty', 'GetAuraToHitAdd', 'GetTAuraEvasionAdd')) {
        Assert-Matches $current ([regex]::Escape($symbol)) "current PDB $symbol"
    }
}

[pscustomobject]@{
    RetailSymbols = 5
    AuraRadius = '3.125m'
    AuraPerks = '0x1c, 0x42, 0x5d'
    WeatherPenalty = '5.0 when non-sunny'
    RevealOnShoot = 'unsilenced concealed shooter'
    CurrentPdbChecked = [bool]$CurrentBuildDirectory
} | Format-List

Write-Host 'COMBAT-SURFACE PASS'
