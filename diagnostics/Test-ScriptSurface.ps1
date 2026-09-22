param(
    [string]$RetailDirectory = 'G:\SS\Silent-Storm\Soft\Andy\RussianGold',
    [string]$Debugger = 'C:\Program Files (x86)\Windows Kits\10\Debuggers\x86\cdb.exe'
)

$ErrorActionPreference = 'Stop'
$repoRoot = Split-Path -Parent $PSScriptRoot
$retailExe = Join-Path $RetailDirectory 'Game.exe'
$source = Join-Path $repoRoot 'Main\ScriptFunctions.cpp'
$missionSource = Join-Path $repoRoot 'Main\iMission.cpp'

foreach ($required in @($Debugger, $retailExe, $source, $missionSource)) {
    if (-not (Test-Path -LiteralPath $required)) {
        throw "Required input not found: $required"
    }
}

$registryOutput = & $Debugger -z $retailExe -y $RetailDirectory -c '.for (r $t0=0; @$t0<0n216; r $t0=@$t0+1) { da poi(Game!NScript::pRegList+@$t0*8); }; q' 2>&1
$retailFunctions = @(
    $registryOutput | ForEach-Object {
        if ($_ -match '^[0-9a-f]+\s+"([A-Za-z_][A-Za-z0-9_]*)"') {
            $Matches[1]
        }
    }
)

$registryText = Get-Content -LiteralPath $source -Raw
$registryBody = [regex]::Match(
    $registryText,
    'Script::SRegFunction pRegList\[\]\s*=\s*\{(?<body>.*?)\{\s*0\s*,\s*0\s*\}',
    [Text.RegularExpressions.RegexOptions]::Singleline
).Groups['body'].Value
$currentFunctions = @(
    [regex]::Matches($registryBody, 'REG_FUNCTION\(\s*([A-Za-z_][A-Za-z0-9_]*)\s*\)|\{\s*"([A-Za-z_][A-Za-z0-9_]*)"\s*,') |
        ForEach-Object {
            if ($_.Groups[1].Success) { $_.Groups[1].Value } else { $_.Groups[2].Value }
        }
)

$missingFunctions = @(Compare-Object $retailFunctions $currentFunctions -PassThru | Where-Object SideIndicator -eq '<=')
$extraFunctions = @(Compare-Object $retailFunctions $currentFunctions -PassThru | Where-Object SideIndicator -eq '=>')

$dispatchOutput = & $Debugger -z $retailExe -y $RetailDirectory -c 'uf Game!NGame::CMissionBase::ExecWorldCommand; uf Game!NGame::CMission::ExecWorldCommand; q' 2>&1
$retailDispatchTypes = @(
    [regex]::Matches(($dispatchOutput -join "`n"), 'CDynamicCast<NWorld::(CUICmd[A-Za-z0-9_]+)>') |
        ForEach-Object { $_.Groups[1].Value } |
        Sort-Object -Unique
)
$currentDispatchTypes = @(
    [regex]::Matches((Get-Content -LiteralPath $missionSource -Raw), 'CDynamicCast<\s*NWorld::(CUICmd[A-Za-z0-9_]+)') |
        ForEach-Object { $_.Groups[1].Value } |
        Sort-Object -Unique
)
$missingDispatchTypes = @(Compare-Object $retailDispatchTypes $currentDispatchTypes -PassThru | Where-Object SideIndicator -eq '<=')

[pscustomobject]@{
    RetailFunctions = $retailFunctions.Count
    CurrentFunctions = $currentFunctions.Count
    MissingFunctions = $missingFunctions -join ', '
    ExtraFunctions = $extraFunctions -join ', '
    RetailCommandTypes = $retailDispatchTypes.Count
    MissingCommandHandlers = $missingDispatchTypes -join ', '
} | Format-List

if ($retailFunctions.Count -ne 215) {
    throw "Unexpected retail function count: $($retailFunctions.Count)"
}
if ($missingFunctions.Count -ne 0) {
    throw "Missing retail Lua functions: $($missingFunctions -join ', ')"
}
if ($missingDispatchTypes.Count -ne 0) {
    throw "Missing retail mission command handlers: $($missingDispatchTypes -join ', ')"
}

Write-Host 'SCRIPT-SURFACE PASS'
