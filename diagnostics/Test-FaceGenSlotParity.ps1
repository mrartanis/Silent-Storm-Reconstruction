param(
    [Parameter(Mandatory)][string]$X86GameRoot,
    [Parameter(Mandatory)][string]$X64GameRoot,
    [string]$SourceSlot = 'AI_CTRL',
    [string]$OutputSlot = 'FACEGEN_PARITY_01',
    [ValidateRange(0,2)][int]$CaseIndex = 0,
    [int]$TimeoutSeconds = 120
)
$ErrorActionPreference = 'Stop'
if ($TimeoutSeconds -lt 10) { throw 'TimeoutSeconds must be at least 10' }

function Send-Command([string]$root, [string]$command) {
    $target = Join-Path $root '_harness_cmd.txt'
    if (Test-Path -LiteralPath $target) { throw "Pending harness command: $target" }
    $temp = Join-Path $root ("_harness_cmd.{0}.tmp" -f [guid]::NewGuid().ToString('N'))
    try {
        [IO.File]::WriteAllBytes($temp,[Text.Encoding]::ASCII.GetBytes("$command`n"))
        [IO.File]::Move($temp,$target)
    } finally {
        if (Test-Path -LiteralPath $temp) { Remove-Item -LiteralPath $temp }
    }
}

function Wait-For([System.Diagnostics.Process]$process, [scriptblock]$condition,
                  [string]$description, [int]$timeout) {
    $deadline = [DateTime]::UtcNow.AddSeconds($timeout)
    while ([DateTime]::UtcNow -lt $deadline) {
        if (. $condition) { return }
        if ($process.HasExited) { throw "Game exited while waiting for $description (code $($process.ExitCode))" }
        Start-Sleep -Milliseconds 200
    }
    throw "Timed out waiting for $description"
}

function Get-Status([string]$logPath, [string]$phase) {
    $lines = @(Get-Content -LiteralPath $logPath | Where-Object { $_ -match "\[harness\] facegen status phase=$phase " })
    if ($lines.Count -ne 1) { throw "Expected one $phase status, found $($lines.Count)" }
    $line = $lines[0]
    $match = [regex]::Match($line, 'merc=(\d+) head=(-?\d+) static=(\d+) textured=(\d+) animator=(\d+) animator_hash=([0-9a-f]{16}) pixels=(\d+) hash=([0-9a-f]{16})')
    if (!$match.Success) { throw "Invalid FaceGen status: $line" }
    return [pscustomobject]@{
        Merc = [int]$match.Groups[1].Value
        Head = [int]$match.Groups[2].Value
        Static = [int]$match.Groups[3].Value
        Textured = [int]$match.Groups[4].Value
        Animator = [int]$match.Groups[5].Value
        AnimatorHash = $match.Groups[6].Value
        Pixels = [int]$match.Groups[7].Value
        Hash = $match.Groups[8].Value
    }
}

function Get-Models([string]$logPath, [string]$phase) {
    $lines = @(Get-Content -LiteralPath $logPath | Where-Object { $_ -match "\[harness\] facegen models phase=$phase " })
    if ($lines.Count -ne 1) { throw "Expected one $phase models line, found $($lines.Count)" }
    $match = [regex]::Match($lines[0], 'hair=(-?\d+) body=(-?\d+) mesh=(-?\d+,-?\d+,-?\d+,-?\d+) ifmesh=(-?\d+,-?\d+,-?\d+,-?\d+)')
    if (!$match.Success) { throw "Invalid FaceGen models line: $($lines[0])" }
    return [pscustomobject]@{
        Hair = [int]$match.Groups[1].Value
        Body = [int]$match.Groups[2].Value
        Mesh = $match.Groups[3].Value
        IFMesh = $match.Groups[4].Value
    }
}

$roots = @(
    [pscustomobject]@{ Name='x86'; Root=(Resolve-Path -LiteralPath $X86GameRoot).Path },
    [pscustomobject]@{ Name='x64'; Root=(Resolve-Path -LiteralPath $X64GameRoot).Path }
)
if ($roots[0].Root -eq $roots[1].Root) { throw 'Paired runs need separate game directories' }
$results = @{}
$streams = @{}
foreach ($entry in $roots) {
    $root = $entry.Root
    $exe = Join-Path $root 'Game.exe'
    $db = Join-Path $root 'game.db'
    $source = Join-Path $root "save\default\$SourceSlot\game.sav"
    $saved = Join-Path $root "save\default\$OutputSlot\game.sav"
    $stream = Join-Path $root "_facegen_$OutputSlot.bin"
    $log = Join-Path $root '_saveload.log'
    if (!(Test-Path -LiteralPath $exe) -or !(Test-Path -LiteralPath $db) -or !(Test-Path -LiteralPath $source)) {
        throw "Missing staged game executable, DB or source slot in $root"
    }
    if (Test-Path -LiteralPath $saved) { throw "Refusing to overwrite existing save: $saved" }
    if (Test-Path -LiteralPath $stream) { throw "Refusing to overwrite existing stream: $stream" }
    if (Test-Path -LiteralPath (Join-Path $root '_harness_cmd.txt')) { throw "Pending command in $root" }
    $launchUtc = [DateTime]::UtcNow
    $priorStreamPath = $env:S2_FACE_SLOT_ANIMATOR_PATH
    try {
        $env:S2_FACE_SLOT_ANIMATOR_PATH = $stream
        $process = Start-Process -FilePath $exe -ArgumentList '-windowed -800 -harness -harness-active' `
            -WorkingDirectory $root -WindowStyle Hidden -PassThru
    } finally {
        $env:S2_FACE_SLOT_ANIMATOR_PATH = $priorStreamPath
    }
    try {
        Wait-For $process { (Test-Path -LiteralPath $log) -and
            (Get-Item -LiteralPath $log).LastWriteTimeUtc -ge $launchUtc -and
            ((Get-Content -LiteralPath $log -Raw) -match 'BOOT harness') } 'harness boot' $TimeoutSeconds
        Send-Command $root "load $SourceSlot"
        Wait-For $process { ((Get-Content -LiteralPath $log -Raw) -match 'LOAD-SLOT-DONE') } 'source slot load' $TimeoutSeconds
        Send-Command $root "facegencommit $CaseIndex"
        Wait-For $process { ((Get-Content -LiteralPath $log -Raw) -match '\[harness\] facegen models phase=commit ') } 'FaceGen commit' $TimeoutSeconds
        $before = Get-Status $log 'commit'
        $beforeModels = Get-Models $log 'commit'
        if ($beforeModels.Body -lt 0) { throw "$($entry.Name) committed Nationality has no per-unit body race" }
        if ($CaseIndex -eq 2 -and $beforeModels.Hair -lt 0) {
            throw "$($entry.Name) active WomanHair selection has no per-unit hair model"
        }
        $glassesModel = [int]($beforeModels.Mesh.Split(',')[0])
        if (($CaseIndex -eq 2 -and $glassesModel -lt 0) -or
            ($CaseIndex -ne 2 -and $glassesModel -ge 0)) {
            throw "$($entry.Name) unexpected glasses model for case $CaseIndex`: $glassesModel"
        }
        if ($before.Merc -ne 1 -or $before.Static -ne 1 -or $before.Textured -ne 1 -or
            $before.Animator -le 0 -or $before.Pixels -le 0) {
            throw "$($entry.Name) custom head did not commit: $($before | ConvertTo-Json -Compress)"
        }
        Send-Command $root "save $OutputSlot"
        Wait-For $process { (Test-Path -LiteralPath $saved) -and (Get-Item -LiteralPath $saved).Length -gt 0 } 'game slot save' $TimeoutSeconds
        # Wait for the save writer to close before asking the main loop to load it.
        $lastLength = -1
        $stable = 0
        Wait-For $process {
            $length = (Get-Item -LiteralPath $saved).Length
            if ($length -eq $lastLength) { $stable++ } else { $stable = 0; $lastLength = $length }
            $stable -ge 5
        } 'stable game slot' $TimeoutSeconds
        Send-Command $root "load $OutputSlot"
        Wait-For $process { ([regex]::Matches((Get-Content -LiteralPath $log -Raw),'LOAD-SLOT-DONE')).Count -ge 2 } 'custom slot reload' $TimeoutSeconds
        Send-Command $root 'facegenstatus'
        Wait-For $process { ((Get-Content -LiteralPath $log -Raw) -match '\[harness\] facegen models phase=query ') } 'reloaded head status' $TimeoutSeconds
        $after = Get-Status $log 'query'
        $afterModels = Get-Models $log 'query'
        if (!(Test-Path -LiteralPath $stream) -or (Get-Item -LiteralPath $stream).Length -ne $after.Animator) {
            throw "$($entry.Name) did not export the reloaded animator stream"
        }
        foreach ($field in 'Merc','Head','Static','Textured','Animator','AnimatorHash','Pixels','Hash') {
            if ($before.$field -ne $after.$field) {
                throw "$($entry.Name) $field changed across game slot: $($before.$field) -> $($after.$field)"
            }
        }
        foreach ($field in 'Hair','Body','Mesh','IFMesh') {
            if ($beforeModels.$field -ne $afterModels.$field) {
                throw "$($entry.Name) $field changed across game slot: $($beforeModels.$field) -> $($afterModels.$field)"
            }
        }
        $results[$entry.Name] = $after
        $results["$($entry.Name)Models"] = $afterModels
        $streams[$entry.Name] = $stream
        Write-Host "$($entry.Name): head $($after.Head), animator $($after.Animator) bytes/hash $($after.AnimatorHash), texture $($after.Pixels) pixels/hash $($after.Hash), hair/body $($afterModels.Hair)/$($afterModels.Body), meshes $($afterModels.Mesh), save/reload PASS; stream $stream"
    } finally {
        if (!$process.HasExited) {
            if (!(Test-Path -LiteralPath (Join-Path $root '_harness_cmd.txt'))) {
                Send-Command $root 'quit'
            }
            if (!$process.WaitForExit(10000)) { Stop-Process -Id $process.Id -Force }
        }
    }
}
foreach ($field in 'Head','Static','Textured','Animator','Pixels','Hash') {
    if ($results.x86.$field -ne $results.x64.$field) {
        throw "x86/x64 $field mismatch: $($results.x86.$field) != $($results.x64.$field)"
    }
}
foreach ($field in 'Hair','Body','Mesh','IFMesh') {
    if ($results.x86Models.$field -ne $results.x64Models.$field) {
        throw "x86/x64 $field mismatch: $($results.x86Models.$field) != $($results.x64Models.$field)"
    }
}
Write-Host 'PASS: x86/x64 committed texture/model parity and per-architecture ordinary slot integrity (animated mesh parity is a separate gate)'
