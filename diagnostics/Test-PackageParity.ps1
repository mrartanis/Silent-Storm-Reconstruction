param(
 [string]$GameRoot='G:\SS\lab\baseline',
 [string]$OldX86Probe='G:\SS\lab\evidence\package-oracle-preportable-x86.exe',
 [string]$PortableProbe='G:\SS\lab\build-x64\RelWithDebInfo\PortablePackageProbe.exe',
 [string]$GamePathProbe='G:\SS\lab\build-x64\RelWithDebInfo\PackageOracleProbe.exe'
)
$ErrorActionPreference='Stop'
foreach($path in @($OldX86Probe,$PortableProbe,$GamePathProbe)){
 if(!(Test-Path -LiteralPath $path -PathType Leaf)){throw "Missing probe: $path"}
}
$files=@(Get-ChildItem -LiteralPath (Join-Path $GameRoot 'res') -Filter '*.res' -Recurse -File)
if($files.Count -eq 0){throw 'No resource archives found'}
$samples=0
foreach($file in $files){
 $lines=@(& $PortableProbe $file.FullName --list)
 if($LASTEXITCODE){throw "Cannot enumerate $($file.FullName)"}
 $entries=@($lines | Select-Object -Skip 1)
 if($entries.Count -lt 3){throw "Too few entries in $($file.Name)"}
 $last=$entries.Count-1
 $middle=[int][math]::Floor($last/2)
 $ids=@(0,$middle,$last | Select-Object -Unique | ForEach-Object {($entries[$_] -split ' ')[0]})
 $old=@(& $OldX86Probe $file.FullName @ids)
 if($LASTEXITCODE -or $old.Count -ne $ids.Count){throw "Old x86 read failed: $($file.Name)"}
 $portable=@(& $PortableProbe $file.FullName @ids | Select-Object -Skip 1)
 if($LASTEXITCODE -or $portable.Count -ne $ids.Count){throw "Portable read failed: $($file.Name)"}
 $game=@(& $GamePathProbe $file.FullName @ids)
 if($LASTEXITCODE -or $game.Count -ne $ids.Count){throw "Game FileIO read failed: $($file.Name)"}
 if(@(Compare-Object $old $portable).Count -or @(Compare-Object $old $game).Count){
  throw "Resource parity mismatch: $($file.Name) IDs $($ids -join ',')"
 }
 $samples+=$ids.Count
}
Write-Output "Package parity passed: $($files.Count) archives, $samples sampled resources, old x86 / portable x64 / game FileIO x64"
