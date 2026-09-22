param([Parameter(Mandatory)][int]$ProcessId,[Parameter(Mandatory)][string]$OutputDirectory,[string]$Debugger='C:\Program Files (x86)\Windows Kits\10\Debuggers\x86\cdb.exe')
$ErrorActionPreference='Stop'
New-Item -ItemType Directory -Force $OutputDirectory | Out-Null
$out=(Resolve-Path $OutputDirectory).Path.Replace('\','/')
$stamp=Get-Date -Format 'yyyyMMdd-HHmmss'
# Non-invasive attach: safe even when a crash debugger is already attached.
& $Debugger -y (Split-Path (Get-Process -Id $ProcessId).Path -Parent) -pv -p $ProcessId -logo "$out/hang-$stamp.log" -c ".lines -e; ~* kv; !runaway; .dump /ma $out/hang-$stamp.dmp; q"
if($LASTEXITCODE){throw 'Snapshot debugger failed'}

