param([string]$OutputDirectory='G:\SS\lab\input-tools',[string]$ToolRoot='G:\SS\lab\tools\VS2022')
$ErrorActionPreference='Stop'
if(Test-Path $OutputDirectory){throw 'Output already exists; use a fresh directory'}
$out=(New-Item -ItemType Directory $OutputDirectory).FullName
$source=Join-Path $PSScriptRoot 'LabInput.cpp'
$cmd=@"
@echo off
call "$ToolRoot\VC\Auxiliary\Build\vcvarsall.bat" x64_x86
if errorlevel 1 exit /b 1
set "INCLUDE=C:\Program Files (x86)\Windows Kits\10\Include\10.0.26100.0\ucrt;%INCLUDE%"
set "LIB=C:\Program Files (x86)\Windows Kits\10\Lib\10.0.26100.0\ucrt\x86;%LIB%"
cd /d "$out"
cl /nologo /Zi /Od /Oy- /EHsc "$source" /Fe:LabInput.exe user32.lib /link /DEBUG:FULL /PDB:LabInput.pdb
exit /b %errorlevel%
"@
$cmd | Set-Content "$out\build.cmd" -Encoding ascii
& $env:ComSpec /d /c "$out\build.cmd" 2>&1 | Tee-Object "$out\build.log"
if($LASTEXITCODE){throw 'LabInput build failed'}
Copy-Item $source $out
Get-ChildItem $out -File | Where-Object Name -ne 'hashes.csv' | Get-FileHash | Select-Object Path,Hash | Export-Csv "$out\hashes.csv" -NoTypeInformation
Write-Output "$out\LabInput.exe"
