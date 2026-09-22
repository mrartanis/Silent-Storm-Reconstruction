param([string]$LabRoot='G:\SS\lab',[string]$Source='G:\SteamLibrary\steamapps\common\Silent Storm')
$ErrorActionPreference='Stop'
$baseline=Join-Path $LabRoot 'baseline'
if(Test-Path $baseline){throw 'Baseline exists; preserve it and select a new LabRoot'}
New-Item -ItemType Directory -Force "$LabRoot\evidence" | Out-Null
$root=(Resolve-Path $Source).Path
Get-ChildItem -LiteralPath $root -File -Recurse | ForEach-Object {
 [pscustomobject]@{Path=$_.FullName.Substring($root.Length+1);Length=$_.Length;SHA256=(Get-FileHash -LiteralPath $_.FullName).Hash}
} | Export-Csv "$LabRoot\evidence\steam-files.csv" -NoTypeInformation
& robocopy $root $baseline /E /XD save screenshots tools source AlwaysCritical APForInventoryUsage HeadshotShouldKill ImprovedBackstab /XF game.exe MapEdit.exe /R:1 /W:1 /NFL /NDL /NP "/LOG:$LabRoot\evidence\copy.log" | Out-Null
if($LASTEXITCODE -ge 8){throw 'Baseline copy failed'}
Get-ChildItem -LiteralPath $baseline -File -Recurse | ForEach-Object {
 [pscustomobject]@{Path=$_.FullName.Substring($baseline.Length+1);Length=$_.Length;SHA256=(Get-FileHash -LiteralPath $_.FullName).Hash}
} | Export-Csv "$LabRoot\evidence\baseline-files.csv" -NoTypeInformation
