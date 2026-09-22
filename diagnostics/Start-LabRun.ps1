param([Parameter(Mandatory)][string]$RunDirectory,[string]$Debugger='C:\Program Files (x86)\Windows Kits\10\Debuggers\x86\cdb.exe')
$ErrorActionPreference='Stop'
$run=(Resolve-Path $RunDirectory).Path
if(!(Test-Path "$run\evidence\run.json")){throw 'Not a prepared lab run'}
$debugPath=$run.Replace('\','/')
$commands=@"
.sympath $run\game
.lines -e
sxd -c2 \".echo SECOND_CHANCE_EXCEPTION; .exr -1; .ecxr; kv; .dump /ma $debugPath/evidence/crash.dmp; q\" av
sxd -c2 \".echo SECOND_CHANCE_EXCEPTION; .exr -1; .ecxr; kv; .dump /ma $debugPath/evidence/crash.dmp; q\" eh
sxd -c2 \".echo SECOND_CHANCE_EXCEPTION; .exr -1; .ecxr; kv; .dump /ma $debugPath/evidence/crash.dmp; q\" sov
g
"@
$commands=$commands.Replace('\"','"')
$commands | Set-Content "$run\evidence\debugger.txt" -Encoding ascii
$p=Start-Process $Debugger -ArgumentList "-G -o -logo `"$run\evidence\debugger.log`" -cf `"$run\evidence\debugger.txt`" `"$run\game\Game.exe`" -windowed -800 -harness" -WorkingDirectory "$run\game" -WindowStyle Hidden -PassThru
$p.Id | Set-Content "$run\evidence\debugger.pid"
Write-Output "Debugger PID $($p.Id); run $run"


