param([string]$LabRoot='G:\SS\lab',[Parameter(Mandatory)][string]$BuildId,[Parameter(Mandatory)][ValidatePattern('^[a-zA-Z0-9_-]+$')][string]$RunId)
$ErrorActionPreference='Stop'
$archive=Join-Path $LabRoot "builds\$BuildId"
$run=Join-Path $LabRoot "runs\$RunId"
if(Test-Path $run){throw 'Run already exists; use a fresh ID to preserve evidence'}
if(!(Test-Path "$archive\build.json")){throw 'Incomplete build archive'}
$buildMetadata=Get-Content "$archive\build.json" -Raw | ConvertFrom-Json
New-Item -ItemType Directory "$run\game","$run\evidence" -Force | Out-Null
& robocopy "$LabRoot\baseline" "$run\game" /E /R:1 /W:1 /NFL /NDL /NP "/LOG:$run\evidence\copy.log" | Out-Null
if($LASTEXITCODE -ge 8){throw 'Copy failed'}
foreach($file in 'Game.exe','Game.pdb','zlib.dll','zlib.pdb'){Copy-Item "$archive\$file" "$run\game"}
if($buildMetadata.Architecture -eq 'x64'){
 foreach($file in 'fmod.dll','binkw32.dll'){Copy-Item "$archive\$file" "$run\game"}
 Get-ChildItem -LiteralPath $archive -File | Where-Object Name -Match '^(avcodec|avformat|avutil|swscale|swresample)-[0-9]+\.dll$' |
  Copy-Item -Destination "$run\game"
}
Copy-Item "$archive\build.json" "$run\evidence"
Copy-Item "$LabRoot\evidence\baseline-files.csv" "$run\evidence"
Get-ChildItem "$run\game" -File | Get-FileHash | Export-Csv "$run\evidence\runtime-hashes.csv" -NoTypeInformation
@{RunId=$RunId;BuildId=$BuildId;WorkingDirectory="$run\game";Arguments='-windowed -800 -harness';CreatedUtc=[DateTime]::UtcNow.ToString('o')} | ConvertTo-Json | Set-Content "$run\evidence\run.json"
Write-Output $run

