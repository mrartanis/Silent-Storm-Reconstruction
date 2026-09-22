param(
 [string]$LabRoot='G:\SS\lab',
 [string]$ToolRoot='G:\SS\lab\tools\VS2022',
 [string]$BuildId=(Get-Date -Format 'yyyyMMdd-HHmmss')
)
$ErrorActionPreference='Stop'
$env:UCRTContentRoot='C:\Program Files (x86)\Windows Kits\10\'
$repo=Split-Path $PSScriptRoot -Parent
$cmake=Join-Path $ToolRoot 'Common7\IDE\CommonExtensions\Microsoft\CMake\CMake\bin\cmake.exe'
$archive=Join-Path $LabRoot "builds\$BuildId"
if(Test-Path $archive){throw "Build ID already exists: $archive"}
New-Item -ItemType Directory $archive | Out-Null
$sha=(& git -C $repo rev-parse HEAD).Trim()
& git -C $repo status --porcelain=v1 | Set-Content "$archive\git-status.txt"
& git -C $repo archive --format=zip "--output=$archive\source.zip" HEAD
& git -C $repo diff --binary | Set-Content "$archive\source.patch"
& $cmake --version | Set-Content "$archive\cmake-version.txt"
$build=Join-Path $LabRoot 'build-x86'
& $cmake --fresh -S $repo -B $build -G 'Visual Studio 17 2022' -A Win32 "-DCMAKE_GENERATOR_INSTANCE=$ToolRoot" "-DS2_GAME_DIR=$LabRoot\baseline" 2>&1 | Tee-Object "$archive\configure.log"
if($LASTEXITCODE){throw 'Configure failed'}
if(!(Select-String -Path "$build\CMakeCache.txt" -Pattern '^CMAKE_CXX_FLAGS_RELWITHDEBINFO:STRING=.*[/]Zi')){throw 'Debug compile flags missing'}
& $cmake --build $build --config RelWithDebInfo --target Game zlib --parallel 6 -- /v:minimal 2>&1 | Tee-Object "$archive\build.log"
if($LASTEXITCODE){throw 'Build failed'}
foreach($file in 'Game.exe','Game.pdb','zlib.dll','zlib.pdb'){
 Copy-Item -LiteralPath "$build\RelWithDebInfo\$file" -Destination $archive
}
Copy-Item "$build\CMakeCache.txt" $archive
Copy-Item $PSScriptRoot "$archive\diagnostics" -Recurse
Get-ChildItem $archive -File | Where-Object Name -ne 'hashes.csv' | Get-FileHash -Algorithm SHA256 | Select-Object Path,Hash | Export-Csv "$archive\hashes.csv" -NoTypeInformation
[ordered]@{Commit=$sha;BuildId=$BuildId;Configuration='RelWithDebInfo';Architecture='x86';ToolRoot=$ToolRoot;CreatedUtc=[DateTime]::UtcNow.ToString('o')} | ConvertTo-Json | Set-Content "$archive\build.json"
Write-Output "Archived build: $archive"






