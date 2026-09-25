param([string]$LabRoot='G:\SS\lab',[string]$ArchiveRoot,[string]$RunRoot,[Parameter(Mandatory)][string]$BuildId,[Parameter(Mandatory)][ValidatePattern('^[a-zA-Z0-9_-]+$')][string]$RunId,[switch]$SkipIntro,[switch]$LinkResources)
$ErrorActionPreference='Stop'
if(!$ArchiveRoot){$ArchiveRoot=Join-Path $LabRoot 'builds'}
if(!$RunRoot){$RunRoot=Join-Path $LabRoot 'runs'}
$archive=Join-Path ([IO.Path]::GetFullPath($ArchiveRoot)) $BuildId
$run=Join-Path ([IO.Path]::GetFullPath($RunRoot)) $RunId
$baseline=Join-Path ([IO.Path]::GetFullPath($LabRoot)) 'baseline'
$resSource=Join-Path $baseline 'res'
if(Test-Path $run){throw 'Run already exists; use a fresh ID to preserve evidence'}
if(!(Test-Path "$archive\build.json")){throw 'Incomplete build archive'}
if($LinkResources){
 $resItem=Get-Item -LiteralPath $resSource -ErrorAction Stop
 if(!$resItem.PSIsContainer -or ($resItem.Attributes -band [IO.FileAttributes]::ReparsePoint)){throw 'Linked resources must be a real baseline directory'}
}
$buildMetadata=Get-Content "$archive\build.json" -Raw | ConvertFrom-Json
New-Item -ItemType Directory "$run\game","$run\evidence" -Force | Out-Null
$copyArgs=@($baseline,"$run\game",'/E','/R:1','/W:1','/NFL','/NDL','/NP',"/LOG:$run\evidence\copy.log")
if($LinkResources){$copyArgs+=@('/XD',$resSource)}
if($buildMetadata.NativeSFX){$copyArgs+=@('/XF',(Join-Path $baseline 'fmod.dll'))}
& robocopy @copyArgs | Out-Null
if($LASTEXITCODE -ge 8){throw 'Copy failed'}
if($LinkResources){
 $resLink=Join-Path "$run\game" 'res'
 if(Test-Path -LiteralPath $resLink){throw 'Resource exclusion failed; refusing to replace copied data'}
 New-Item -ItemType Junction -Path $resLink -Target $resSource | Out-Null
}
foreach($file in 'Game.exe','Game.pdb','zlib.dll','zlib.pdb'){Copy-Item "$archive\$file" "$run\game"}
if($buildMetadata.Architecture -eq 'x64'){
 foreach($file in 'binkw32.dll'){Copy-Item "$archive\$file" "$run\game"}
 if($buildMetadata.NativeSFX){
  if(Test-Path -LiteralPath "$run\game\fmod.dll"){throw 'Native SFX run must not contain fmod.dll'}
 } elseif(Test-Path -LiteralPath "$archive\fmod.dll"){
  Copy-Item "$archive\fmod.dll" "$run\game"
 }
 Get-ChildItem -LiteralPath $archive -File | Where-Object Name -Match '^(avcodec|avformat|avutil|swscale|swresample)-[0-9]+\.dll$' |
  Copy-Item -Destination "$run\game"
}
Copy-Item "$archive\build.json" "$run\evidence"
Copy-Item "$LabRoot\evidence\baseline-files.csv" "$run\evidence"
Get-ChildItem "$run\game" -File | Get-FileHash | Export-Csv "$run\evidence\runtime-hashes.csv" -NoTypeInformation
$arguments='-windowed -800 -harness'
if($SkipIntro){
 # Only the boot sequence is bypassed; game_skipvideo would also skip mission cinematics.
 $fastStart='cfg\lab-no-intro.cfg'
 Set-Content -LiteralPath (Join-Path "$run\game" $fastStart) -Value 'mainmenu' -Encoding ascii
 $arguments+=' -cfg .\cfg\lab-no-intro.cfg'
}
@{RunId=$RunId;BuildId=$BuildId;WorkingDirectory="$run\game";Arguments=$arguments;SkipIntro=[bool]$SkipIntro;LinkedResources=[bool]$LinkResources;ResourceSource=$(if($LinkResources){$resSource}else{$null});CreatedUtc=[DateTime]::UtcNow.ToString('o')} | ConvertTo-Json | Set-Content "$run\evidence\run.json"
Write-Output $run

