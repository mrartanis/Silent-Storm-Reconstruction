param([string]$LabRoot='G:\SS\lab', [string]$SteamRoot='G:\SteamLibrary\steamapps\common\Silent Storm', [Parameter(Mandatory=$true)][string]$OutputDirectory)
$ErrorActionPreference='Stop'
if(Test-Path -LiteralPath $OutputDirectory){throw 'Choose a fresh evidence directory'}
New-Item -ItemType Directory -Path $OutputDirectory | Out-Null
foreach($target in @(@{Name='baseline';Root=(Join-Path $LabRoot 'baseline');Manifest='baseline-files.csv'},@{Name='steam';Root=$SteamRoot;Manifest='steam-files.csv'})){
    $manifest=Import-Csv -LiteralPath (Join-Path $LabRoot ('evidence\'+$target.Manifest))
    $failures=@()
    foreach($entry in $manifest){
        $path=Join-Path $target.Root $entry.Path
        if(!(Test-Path -LiteralPath $path -PathType Leaf)){$failures+='Missing: '+$entry.Path;continue}
        $file=Get-Item -LiteralPath $path
        if($file.Length -ne [long]$entry.Length){$failures+='Length: '+$entry.Path;continue}
        if((Get-FileHash -LiteralPath $path -Algorithm SHA256).Hash -ne $entry.SHA256){$failures+='SHA256: '+$entry.Path}
    }
    $configurationDifferences=@($failures | Where-Object {$_ -match '^(Length|SHA256): cfg\\config\.cfg$'})
    $immutableFailures=@($failures | Where-Object {$_ -notin $configurationDifferences})
    $config=Get-Item -LiteralPath (Join-Path $target.Root 'cfg\config.cfg')
    [ordered]@{VerifiedAt=[DateTime]::UtcNow.ToString('o');Root=$target.Root;Manifest=$target.Manifest;Checked=$manifest.Count;Failures=$failures;ImmutableFailures=$immutableFailures;RuntimeConfigurationDifferences=$configurationDifferences;ConfigurationLastWriteUtc=$config.LastWriteTimeUtc.ToString('o');ConfigurationSHA256=(Get-FileHash -LiteralPath $config.FullName).Hash} | ConvertTo-Json -Depth 4 | Set-Content -LiteralPath (Join-Path $OutputDirectory ($target.Name+'.json')) -Encoding utf8
    Write-Output ($target.Name+': '+$manifest.Count+' checked, '+$failures.Count+' failures')
    if($immutableFailures.Count){throw 'Original game data verification failed'}
}
