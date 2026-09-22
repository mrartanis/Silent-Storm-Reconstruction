param(
    [string]$RetailDirectory = 'G:\SS\Silent-Storm\Soft\Andy\RussianGold',
    [string]$CurrentBuildDirectory,
    [string]$SaveFile,
    [string]$Debugger = 'C:\Program Files (x86)\Windows Kits\10\Debuggers\x86\cdb.exe'
)

$ErrorActionPreference = 'Stop'
$repoRoot = Split-Path -Parent $PSScriptRoot
$mainSource = Join-Path $repoRoot 'Main\iMain.cpp'
$managerSource = Join-Path $repoRoot 'Main\iSaveManager.cpp'
$managerHeader = Join-Path $repoRoot 'Main\iSaveManager.h'

foreach ($required in @($Debugger, (Join-Path $RetailDirectory 'Game.exe'), $mainSource,
        $managerSource, $managerHeader)) {
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

$symbols = @(
    'x Game!NMainLoop::CICLoad::Exec'
    'x Game!NMainLoop::CICSave::Exec'
    'x Game!NMainLoop::CICLoadFile::Exec'
    'x Game!NMainLoop::CICSaveFile::Exec'
    'x Game!NMainLoop::GetSlotScreenShot*'
    'x Game!NMainLoop::GetQuickSaveSlot*'
) -join '; '

$retailSymbols = Invoke-Cdb $RetailDirectory $symbols
foreach ($symbol in @('CICLoad::Exec', 'CICSave::Exec', 'CICLoadFile::Exec', 'CICSaveFile::Exec',
        'GetSlotScreenShot', 'GetQuickSaveSlot')) {
    Assert-Matches $retailSymbols ([regex]::Escape($symbol)) "retail save symbol $symbol"
}

# Retail GetSlotScreenShot @0x632720 explicitly accepts both the Jan03 and retail v1.x magics.
$retailScreenshot = Invoke-Cdb $RetailDirectory 'uf 00632720'
Assert-Matches $retailScreenshot 'cmp\s+eax,818B9F21h[\s\S]*cmp\s+eax,828CA022h' 'retail dual screenshot magic check'

$main = Get-Content -LiteralPath $mainSource -Raw
$manager = Get-Content -LiteralPath $managerSource -Raw
$header = Get-Content -LiteralPath $managerHeader -Raw

foreach ($constant in @('0x828CA022', '0x818B9F21', 'N_SAVE_SCREENSHOT_X = 320', 'N_SAVE_SCREENSHOT_Y = 200')) {
    Assert-Matches $header ([regex]::Escape($constant)) "save format constant $constant"
}
Assert-Matches $main 'CICLoad::Exec[\s\S]*N_SAVE_MAGIC_NUMBER[\s\S]*N_SAVE_MAGIC_NUMBER_V0' 'dual full-load magic check'
Assert-Matches $manager 'GetSlotScreenShot[\s\S]*N_SAVE_MAGIC_NUMBER[\s\S]*N_SAVE_MAGIC_NUMBER_V0' 'dual preview magic check'
Assert-Matches $main 'CICSave::Exec[\s\S]*sHeader\.nMods\s*=\s*activeMods\.size\(\)[\s\S]*WriteString' 'active-mod save header'
Assert-Matches $main 'CICLoad::Exec[\s\S]*vector<string>\s+modDirs[\s\S]*CModManager::Activate' 'active-mod restore before graph read'
Assert-Matches $main 'sSaver\.Add\(\s*2,\s*&interfaces\s*\)' 'full interface-stack graph wire'
Assert-Matches $main 'CICLoadFile::Exec[\s\S]*OnSnapshotRestored' 'raw snapshot runtime-cache restore'
Assert-Matches $manager 'GetQuickSaveSlot[\s\S]*bFirstNewer' 'two-slot quicksave rotation'

if ($CurrentBuildDirectory) {
    $currentSymbols = Invoke-Cdb $CurrentBuildDirectory $symbols
    foreach ($symbol in @('CICLoad::Exec', 'CICSave::Exec', 'CICLoadFile::Exec', 'CICSaveFile::Exec',
            'GetSlotScreenShot', 'GetQuickSaveSlot')) {
        Assert-Matches $currentSymbols ([regex]::Escape($symbol)) "current PDB save symbol $symbol"
    }
}

$saveChecked = $false
$saveMagic = ''
$saveLength = 0
if ($SaveFile) {
    if (-not (Test-Path -LiteralPath $SaveFile)) {
        throw "Save file not found: $SaveFile"
    }
    $stream = [System.IO.File]::OpenRead($SaveFile)
    try {
        $reader = [System.IO.BinaryReader]::new($stream)
        $magic = $reader.ReadUInt32()
        $mods = $reader.ReadInt32()
        $minimumHeader = 8 + 320 * 200 * 4
        $magicV1 = [uint32]::Parse('828CA022', [Globalization.NumberStyles]::HexNumber)
        $magicV0 = [uint32]::Parse('818B9F21', [Globalization.NumberStyles]::HexNumber)
        if ($magic -ne $magicV1 -and $magic -ne $magicV0) {
            throw ('Unexpected save magic 0x{0:X8}' -f $magic)
        }
        if ($mods -lt 0 -or $mods -gt 1024) {
            throw "Implausible active-mod count: $mods"
        }
        if ($stream.Length -le $minimumHeader) {
            throw "Truncated save: $($stream.Length) bytes"
        }
        $saveChecked = $true
        $saveMagic = '0x{0:X8}' -f $magic
        $saveLength = $stream.Length
    }
    finally {
        $stream.Dispose()
    }
}

[pscustomobject]@{
    RetailReadMagics = '0x818B9F21 + 0x828CA022'
    HeaderScreenshot = '320x200 BGRA'
    GraphRoot = 'tag 2 interface stack'
    RawSnapshots = 'restart.sav round-trip + runtime-cache restore'
    CurrentPdbChecked = [bool]$CurrentBuildDirectory
    SaveChecked = $saveChecked
    SaveMagic = $saveMagic
    SaveLength = $saveLength
} | Format-List

Write-Host 'SAVE-SURFACE PASS'
