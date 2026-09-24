param(
 [string]$LabRoot='G:\SS\lab',
 [string]$ToolRoot='G:\SS\lab\tools\VS2022',
 [ValidateSet('Win32','x64')][string]$Architecture='Win32',
 [string]$BuildId=(Get-Date -Format 'yyyyMMdd-HHmmss'),
 [switch]$NativeMedia,
 [string]$FFmpegRoot,
 [string]$MiniaudioIncludeDir
)
$ErrorActionPreference='Stop'
$env:UCRTContentRoot='C:\Program Files (x86)\Windows Kits\10\'
$repo=Split-Path $PSScriptRoot -Parent
$cmake=Join-Path $ToolRoot 'Common7\IDE\CommonExtensions\Microsoft\CMake\CMake\bin\cmake.exe'
$archive=Join-Path $LabRoot "builds\$BuildId"
if(Test-Path $archive){throw "Build ID already exists: $archive"}
$sourceStatus=@(& git -C $repo status --porcelain=v1 --untracked-files=all)
if($LASTEXITCODE){throw 'Cannot inspect source worktree'}
if($sourceStatus.Count -ne 0){throw 'Reproducible build archive requires a clean source worktree; commit changes first'}
if($NativeMedia){
 if($Architecture -ne 'x64'){throw 'Native media build currently requires x64'}
 if(!(Test-Path -LiteralPath "$FFmpegRoot\include\libavformat\avformat.h")){throw 'Set FFmpegRoot to the shared development package root'}
 if(!(Test-Path -LiteralPath "$MiniaudioIncludeDir\miniaudio.h")){throw 'Set MiniaudioIncludeDir to miniaudio 0.11.25'}
}
New-Item -ItemType Directory $archive | Out-Null
$sha=(& git -C $repo rev-parse HEAD).Trim()
& git -C $repo archive --format=zip "--output=$archive\source.zip" HEAD
Set-Content -LiteralPath "$archive\git-status.txt" -Value '' -NoNewline
Set-Content -LiteralPath "$archive\source.patch" -Value '' -NoNewline
& $cmake --version | Set-Content "$archive\cmake-version.txt"
$build=Join-Path $LabRoot $(if($Architecture -eq 'x64'){'build-x64'}else{'build-x86'})
$configureArgs=@('--fresh','-S',$repo,'-B',$build,'-G','Visual Studio 17 2022','-A',$Architecture,"-DCMAKE_GENERATOR_INSTANCE=$ToolRoot","-DS2_GAME_DIR=$LabRoot\baseline")
if($NativeMedia){
 $configureArgs+=@('-DS2_BUILD_MEDIA_PROBE=ON','-DS2_BUILD_MINIAUDIO_MUSIC_PROBE=ON','-DS2_ENABLE_NATIVE_MUSIC=ON','-DS2_ENABLE_NATIVE_VIDEO=ON',"-DS2_FFMPEG_ROOT=$FFmpegRoot","-DS2_MINIAUDIO_INCLUDE_DIR=$MiniaudioIncludeDir")
}
& $cmake @configureArgs 2>&1 | Tee-Object "$archive\configure.log"
if($LASTEXITCODE){throw 'Configure failed'}
if(!(Select-String -Path "$build\CMakeCache.txt" -Pattern '^CMAKE_CXX_FLAGS_RELWITHDEBINFO:STRING=.*[/]Zi')){throw 'Debug compile flags missing'}
$targets=@('Game','zlib')
if($NativeMedia){$targets+='s2_bink_compat'}
& $cmake --build $build --config RelWithDebInfo --target @targets --parallel 6 -- /v:minimal 2>&1 | Tee-Object "$archive\build.log"
if($LASTEXITCODE){throw 'Build failed'}
foreach($file in 'Game.exe','Game.pdb','zlib.dll','zlib.pdb'){
 Copy-Item -LiteralPath "$build\RelWithDebInfo\$file" -Destination $archive
}
if($Architecture -eq 'x64'){
 Copy-Item -LiteralPath "$build\_implibs\fmod.dll" -Destination $archive
 $binkPath=if($NativeMedia){"$build\_media\RelWithDebInfo\binkw32.dll"}else{"$build\_implibs\binkw32.dll"}
 Copy-Item -LiteralPath $binkPath -Destination $archive
 if($NativeMedia){
  $runtime=@(Get-ChildItem -LiteralPath "$FFmpegRoot\bin" -File | Where-Object Name -Match '^(avcodec|avformat|avutil|swscale|swresample)-[0-9]+\.dll$')
  if($runtime.Count -ne 5){throw 'Expected five FFmpeg runtime DLLs'}
  $runtime | Copy-Item -Destination $archive
 }
}
Copy-Item "$build\CMakeCache.txt" $archive
Copy-Item $PSScriptRoot "$archive\diagnostics" -Recurse
[ordered]@{Commit=$sha;BuildId=$BuildId;Configuration='RelWithDebInfo';Architecture=$(if($Architecture -eq 'x64'){'x64'}else{'x86'});NativeMedia=[bool]$NativeMedia;ToolRoot=$ToolRoot;CreatedUtc=[DateTime]::UtcNow.ToString('o')} | ConvertTo-Json | Set-Content "$archive\build.json"
Get-ChildItem $archive -File | Where-Object Name -ne 'hashes.csv' | Get-FileHash -Algorithm SHA256 | Select-Object Path,Hash | Export-Csv "$archive\hashes.csv" -NoTypeInformation
Write-Output "Archived build: $archive"






