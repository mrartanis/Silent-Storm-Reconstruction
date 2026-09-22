param([Parameter(Mandatory)][string]$RunDirectory)
$ErrorActionPreference='Stop'
$run=(Resolve-Path $RunDirectory).Path
if(!(Test-Path "$run\evidence\run.json")){throw 'Not a prepared run'}
$game=Join-Path $run 'game'
$stamp=Get-Date -Format 'yyyyMMdd-HHmmss'
$dest=Join-Path $run "evidence\snapshot-$stamp"
New-Item -ItemType Directory $dest | Out-Null
Get-ChildItem $game -File | Where-Object Extension -In '.log','.txt','.cfg' | Copy-Item -Destination $dest
foreach($dir in 'save','screenshots','cfg') { if(Test-Path "$game\$dir"){Copy-Item "$game\$dir" $dest -Recurse} }
Get-ChildItem $dest -Recurse -File | Where-Object Name -ne 'hashes.csv' | Get-FileHash | Select-Object Path,Hash | Export-Csv "$dest\hashes.csv" -NoTypeInformation
Write-Output $dest

