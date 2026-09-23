param(
    [Parameter(Mandatory)][string]$X86GameRoot,
    [Parameter(Mandatory)][string]$X64GameRoot,
    [int]$TimeoutSeconds = 120
)
$ErrorActionPreference = 'Stop'
if ($TimeoutSeconds -lt 10) { throw 'TimeoutSeconds must be at least 10' }
$roots = @(
    [pscustomobject]@{ Name='x86'; Path=(Resolve-Path -LiteralPath $X86GameRoot).Path },
    [pscustomobject]@{ Name='x64'; Path=(Resolve-Path -LiteralPath $X64GameRoot).Path }
)
if ($roots[0].Path -eq $roots[1].Path) { throw 'Paired runs need separate game directories' }

function Send-HarnessCommand([string]$directory, [string]$command) {
    $target = Join-Path $directory '_harness_cmd.txt'
    if (Test-Path -LiteralPath $target) { throw "Pending harness command: $target" }
    # Write the bytes elsewhere first. The game opens the final path every
    # frame and can otherwise observe an empty, partially written command.
    $temp = Join-Path $directory ("_harness_cmd.{0}.tmp" -f [guid]::NewGuid().ToString('N'))
    try {
        [IO.File]::WriteAllBytes($temp, [Text.Encoding]::ASCII.GetBytes("$command`n"))
        [IO.File]::Move($temp,$target)
    } finally {
        if (Test-Path -LiteralPath $temp) { Remove-Item -LiteralPath $temp }
    }
}

$results = @{}
foreach ($run in $roots) {
    $exe = Join-Path $run.Path 'Game.exe'
    $db = Join-Path $run.Path 'game.db'
    if (!(Test-Path -LiteralPath $exe) -or !(Test-Path -LiteralPath $db)) {
        throw "Missing staged game executable or database: $($run.Path)"
    }
    if (Test-Path -LiteralPath (Join-Path $run.Path '_harness_cmd.txt')) {
        throw "Stale harness command in $($run.Path)"
    }
    $launchUtc = [DateTime]::UtcNow
    $process = Start-Process -FilePath $exe -ArgumentList '-windowed -800 -harness' `
        -WorkingDirectory $run.Path -WindowStyle Hidden -PassThru
    $log = Join-Path $run.Path '_saveload.log'
    $deadline = [DateTime]::UtcNow.AddSeconds($TimeoutSeconds)
    try {
        while ([DateTime]::UtcNow -lt $deadline) {
            if ((Test-Path -LiteralPath $log) -and
                (Get-Item -LiteralPath $log).LastWriteTimeUtc -ge $launchUtc) {
                $content = Get-Content -LiteralPath $log -Raw
                if ($content -match 'BOOT harness') { break }
            }
            if ($process.HasExited) { throw "$($run.Name) game exited before harness boot" }
            Start-Sleep -Milliseconds 250
        }
        if ([DateTime]::UtcNow -ge $deadline) { throw "$($run.Name) harness boot timed out" }
        Send-HarnessCommand $run.Path 'facegenbake'
        $lines = @()
        while ([DateTime]::UtcNow -lt $deadline) {
            if (Test-Path -LiteralPath $log) {
                $lines = @(Get-Content -LiteralPath $log | Where-Object {
                    $_ -match '^\[harness\] facegen bake case='
                })
                if ($lines.Count -ge 3) { break }
            }
            if ($process.HasExited) { throw "$($run.Name) game exited before three bake results" }
            Start-Sleep -Milliseconds 250
        }
        if ($lines.Count -ne 3) { throw "$($run.Name) bake timed out after $($lines.Count) results" }
        $cases = @{}
        foreach ($line in $lines) {
            if ($line -notmatch '^\[harness\] facegen bake case=(\d+) ok=(\d+) head=(-?\d+) static=(\d+) textured=(\d+) roundtrip=(\d+) animator=(\d+) pixels=(\d+) hash=([0-9a-f]{16})$') {
                throw "Invalid $($run.Name) bake result: $line"
            }
            $id = [int]$Matches[1]
            if ($cases.ContainsKey($id) -or $id -lt 0 -or $id -gt 2) { throw "Unexpected bake case: $id" }
            if ($Matches[2] -ne '1' -or $Matches[4] -ne '1' -or $Matches[5] -ne '1' -or
                $Matches[6] -ne '1' -or [int]$Matches[7] -le 0 -or
                [int]$Matches[8] -le 0) { throw "Incomplete $($run.Name) bake: $line" }
            $cases[$id] = [pscustomobject]@{
                Head=[int]$Matches[3]; Animator=[int]$Matches[7]
                Pixels=[int]$Matches[8]; Hash=$Matches[9]
            }
        }
        $results[$run.Name] = $cases
    } finally {
        if (!$process.HasExited) {
            if (!(Test-Path -LiteralPath (Join-Path $run.Path '_harness_cmd.txt'))) {
                Send-HarnessCommand $run.Path 'quit'
            }
            if (!$process.WaitForExit(10000)) {
                # Only the process started by this test is stopped on timeout.
                Stop-Process -Id $process.Id -Force -ErrorAction SilentlyContinue
            }
        }
    }
}

for ($case = 0; $case -lt 3; ++$case) {
    $left = $results.x86[$case]
    $right = $results.x64[$case]
    if ($left.Head -ne $right.Head -or $left.Animator -ne $right.Animator -or
        $left.Pixels -ne $right.Pixels -or
        $left.Hash -ne $right.Hash) {
        throw "Bake parity mismatch case=$case x86=$($left.Hash) x64=$($right.Hash)"
    }
    Write-Host "case=$case head=$($left.Head) animator=$($left.Animator) pixels=$($left.Pixels) hash=$($left.Hash)"
}
if (@($results.x86.Values | Select-Object -ExpandProperty Hash -Unique).Count -ne 3) {
    throw 'The three bake presets did not produce three distinct textures'
}
Write-Host 'FACEGEN BAKE PARITY PASS: 3 DB-backed game cases, exact x86/x64 CPU texture hashes and CHeadInfo round-trips'
