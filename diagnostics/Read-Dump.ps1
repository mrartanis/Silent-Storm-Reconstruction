param([Parameter(Mandatory)][string]$Dump,[Parameter(Mandatory)][string]$Symbols,[Parameter(Mandatory)][string]$Log,[string]$Debugger='C:\Program Files (x86)\Windows Kits\10\Debuggers\x86\cdb.exe')
$ErrorActionPreference='Stop'
& $Debugger -z $Dump -y $Symbols -logo $Log -c '.lines -e; .exr -1; .ecxr; kv; ~* kv; lmvm Game; lmvm DumpProbe; q'
if($LASTEXITCODE){throw 'Dump analysis failed'}
