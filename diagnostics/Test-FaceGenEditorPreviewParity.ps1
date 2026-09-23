param(
    [Parameter(Mandatory)][string]$X86GameRoot,
    [Parameter(Mandatory)][string]$X64GameRoot,
    [string]$SourceSlot = 'AI_CTRL',
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

function Get-Log([string]$path) {
    if (!(Test-Path -LiteralPath $path)) { return '' }
    return Get-Content -LiteralPath $path -Raw
}

function Get-Preview([string]$line) {
    $match = [regex]::Match($line,'ok=(\d+) head=(-?\d+) static=(\d+) textured=(\d+) animator=(\d+) animator_hash=([0-9a-f]{16}) pixels=(\d+) hash=([0-9a-f]{16})')
    if (!$match.Success) { throw "Invalid editor preview: $line" }
    return [pscustomobject]@{
        Ok = [int]$match.Groups[1].Value
        Head = [int]$match.Groups[2].Value
        Static = [int]$match.Groups[3].Value
        Textured = [int]$match.Groups[4].Value
        Animator = [int]$match.Groups[5].Value
        AnimatorHash = $match.Groups[6].Value
        Pixels = [int]$match.Groups[7].Value
        Hash = $match.Groups[8].Value
    }
}

$roots = @(
    [pscustomobject]@{ Name='x86'; Root=(Resolve-Path -LiteralPath $X86GameRoot).Path },
    [pscustomobject]@{ Name='x64'; Root=(Resolve-Path -LiteralPath $X64GameRoot).Path }
)
if ($roots[0].Root -eq $roots[1].Root) { throw 'Paired runs need separate game directories' }
$results = @{}
foreach ($entry in $roots) {
    $root = $entry.Root
    $exe = Join-Path $root 'Game.exe'
    $db = Join-Path $root 'game.db'
    $source = Join-Path $root "save\default\$SourceSlot\game.sav"
    $log = Join-Path $root '_saveload.log'
    if (!(Test-Path -LiteralPath $exe) -or !(Test-Path -LiteralPath $db) -or !(Test-Path -LiteralPath $source)) {
        throw "Missing staged executable, DB or source slot in $root"
    }
    if (Test-Path -LiteralPath (Join-Path $root '_harness_cmd.txt')) { throw "Pending command in $root" }
    $launchUtc = [DateTime]::UtcNow
    $process = Start-Process -FilePath $exe -ArgumentList '-windowed -800 -harness -harness-active' `
        -WorkingDirectory $root -WindowStyle Hidden -PassThru
    try {
        Wait-For $process { (Test-Path -LiteralPath $log) -and
            (Get-Item -LiteralPath $log).LastWriteTimeUtc -ge $launchUtc -and
            ((Get-Log $log) -match 'BOOT harness') } 'harness boot' $TimeoutSeconds
        Send-Command $root "load $SourceSlot"
        Wait-For $process { (Get-Log $log) -match 'LOAD-SLOT-DONE' } 'mission load' $TimeoutSeconds
        Send-Command $root 'facegeneditor'
        Wait-For $process { (Get-Log $log) -match '\[harness\] facegen editor queued=1' } 'Advanced FaceGen editor' $TimeoutSeconds
        $steps = @(
            [pscustomobject]@{ Name='default'; Edit=$null },
            [pscustomobject]@{ Name='nose100'; Edit='Nose' },
            [pscustomobject]@{ Name='eyes100'; Edit='EyesColor' }
        )
        $previews = @{}
        $previousCount = 0
        foreach ($step in $steps) {
            if ($step.Edit) {
                Send-Command $root "facegenedit $($step.Edit) 100"
                $editPattern = "\[harness\] facegen edit name=$($step.Edit) requested=100 observed=100 ok=1"
                Wait-For $process { (Get-Log $log) -match $editPattern } "$($step.Edit) slider update" $TimeoutSeconds
            }
            Send-Command $root 'facegenpreview'
            Wait-For $process {
                ([regex]::Matches((Get-Log $log),'\[harness\] facegen preview ')).Count -gt $previousCount
            } "$($step.Name) preview" $TimeoutSeconds
            $lines = @((Get-Content -LiteralPath $log) | Where-Object { $_ -match '\[harness\] facegen preview ' })
            $previousCount = $lines.Count
            $preview = Get-Preview $lines[-1]
            if ($preview.Ok -ne 1 -or $preview.Static -ne 1 -or $preview.Textured -ne 1 -or
                $preview.Animator -le 0 -or $preview.Pixels -le 0) {
                throw "$($entry.Name) $($step.Name) did not bake a complete preview: $($lines[-1])"
            }
            $previews[$step.Name] = $preview
            Write-Host "$($entry.Name) $($step.Name): mesh $($preview.AnimatorHash), texture $($preview.Hash)"
        }
        if ($previews.default.AnimatorHash -eq $previews.nose100.AnimatorHash) {
            throw "$($entry.Name) Nose slider did not change the editor mesh"
        }
        if ($previews.nose100.Hash -eq $previews.eyes100.Hash) {
            throw "$($entry.Name) EyesColor slider did not change the editor texture"
        }
        $results[$entry.Name] = $previews
    } finally {
        if (!$process.HasExited) {
            if (!(Test-Path -LiteralPath (Join-Path $root '_harness_cmd.txt'))) { Send-Command $root 'quit' }
            if (!$process.WaitForExit(10000)) { Stop-Process -Id $process.Id -Force }
        }
    }
}
foreach ($step in 'default','nose100','eyes100') {
    foreach ($field in 'Head','Static','Textured','Animator','Pixels','Hash') {
        if ($results.x86[$step].$field -ne $results.x64[$step].$field) {
            throw "x86/x64 $step $field mismatch: $($results.x86[$step].$field) != $($results.x64[$step].$field)"
        }
    }
}
Write-Host 'PASS: real Advanced FaceGen UI preview x86/x64 texture parity, Nose mesh change, EyesColor texture change'
