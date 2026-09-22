param([string]$LabRoot='G:\SS\lab',[string]$ToolRoot='G:\SS\lab\tools\VS2022',[string]$Debugger='C:\Program Files (x86)\Windows Kits\10\Debuggers\x86\cdb.exe')
$ErrorActionPreference='Stop'
$probe=Join-Path $LabRoot ('probe-'+(Get-Date -Format 'yyyyMMdd-HHmmss'))
New-Item -ItemType Directory $probe | Out-Null
$source=Join-Path $PSScriptRoot 'DumpProbe.cpp'
@"
@echo off
call "$ToolRoot\VC\Auxiliary\Build\vcvarsall.bat" x64_x86
set INCLUDE=C:\Program Files (x86)\Windows Kits\10\Include\10.0.26100.0\ucrt;%INCLUDE%
set LIB=C:\Program Files (x86)\Windows Kits\10\Lib\10.0.26100.0\ucrt\x86;%LIB%
cd /d "$probe"
cl /nologo /Zi /Od /Oy- /EHsc "$source" /Fe:DumpProbe.exe /link /DEBUG:FULL /PDB:DumpProbe.pdb
"@ | Set-Content "$probe\build.cmd" -Encoding ascii
& "$probe\build.cmd" *> "$probe\build.log"
if($LASTEXITCODE){throw 'Probe build failed'}
$dp=$probe.Replace('\','/')
@"
.sympath $probe
.lines -e
sxd -c2 ".exr -1; kv; .dump /ma $dp/crash.dmp; q" av
g
"@ | Set-Content "$probe\crash.txt" -Encoding ascii
& $Debugger -logo "$probe\capture.log" -cf "$probe\crash.txt" "$probe\DumpProbe.exe" *> "$probe\capture-console.log"
if(!(Test-Path "$probe\crash.dmp")){throw 'Crash dump missing'}
& "$PSScriptRoot\Read-Dump.ps1" -Dump "$probe\crash.dmp" -Symbols $probe -Log "$probe\analysis.log" *> "$probe\analysis-console.log"
if(!(Select-String -Path "$probe\analysis.log" -SimpleMatch 'DumpProbe!DiagnosticCrashLeaf')){throw 'Crash symbols not resolved'}
$p=Start-Process "$probe\DumpProbe.exe" -ArgumentList hang -WindowStyle Hidden -PassThru
try { & "$PSScriptRoot\Capture-Hang.ps1" -ProcessId $p.Id -OutputDirectory $probe *> "$probe\hang-capture.log" }
finally { if(!$p.HasExited){Stop-Process -Id $p.Id} }
$dump=Get-ChildItem $probe -Filter 'hang-*.dmp' | Select-Object -First 1
if(!$dump){throw 'Hang dump missing'}
& "$PSScriptRoot\Read-Dump.ps1" -Dump $dump.FullName -Symbols $probe -Log "$probe\hang-analysis.log" *> "$probe\hang-analysis-console.log"
if(!(Select-String -Path "$probe\hang-analysis.log" -SimpleMatch 'DumpProbe!main')){throw 'Hang symbols not resolved'}
Write-Output "Probe passed: $probe"
