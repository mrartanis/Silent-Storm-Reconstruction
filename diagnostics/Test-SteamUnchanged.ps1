param([string]$LabRoot='G:\SS\lab',[string]$Source='G:\SteamLibrary\steamapps\common\Silent Storm')
$ErrorActionPreference='Stop'
$expected=Import-Csv "$LabRoot\evidence\steam-files.csv"
$failures=@(foreach($row in $expected){$p=Join-Path $Source $row.Path;if(!(Test-Path -LiteralPath $p)){"Missing: $($row.Path)"}elseif((Get-FileHash -LiteralPath $p).Hash -ne $row.SHA256){"Changed: $($row.Path)"}})
$actualCount=(Get-ChildItem -LiteralPath $Source -Recurse -File | Measure-Object).Count
if($actualCount -ne $expected.Count){$failures+="File count changed: $actualCount vs $($expected.Count)"}
@{CheckedUtc=[DateTime]::UtcNow.ToString('o');ExpectedFiles=$expected.Count;ActualFiles=$actualCount;Failures=$failures} | ConvertTo-Json | Set-Content "$LabRoot\evidence\steam-verification.json"
if($failures.Count){throw ($failures -join "`n")}
Write-Output "Steam files unchanged: $actualCount"
