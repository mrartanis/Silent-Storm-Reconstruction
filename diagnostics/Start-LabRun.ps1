param(
    [Parameter(Mandatory)][string]$RunDirectory,
    [string]$Debugger,
    [string]$GameArguments='-windowed -800 -harness'
)
$ErrorActionPreference='Stop'
$run=(Resolve-Path $RunDirectory).Path
if(!(Test-Path "$run\evidence\run.json")){throw 'Not a prepared lab run'}
$runMetadata = Get-Content "$run\evidence\run.json" -Raw | ConvertFrom-Json
if(!$Debugger){
    $buildMetadata = Get-Content "$run\evidence\build.json" -Raw | ConvertFrom-Json
    $debugArch = if($buildMetadata.Architecture -eq 'x64'){'x64'}else{'x86'}
    $Debugger = "C:\Program Files (x86)\Windows Kits\10\Debuggers\$debugArch\cdb.exe"
}
$runMetadata.Arguments = $GameArguments
$runMetadata | ConvertTo-Json | Set-Content "$run\evidence\run.json"
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
$p=Start-Process $Debugger -ArgumentList "-G -o -logo `"$run\evidence\debugger.log`" -cf `"$run\evidence\debugger.txt`" `"$run\game\Game.exe`" $GameArguments" -WorkingDirectory "$run\game" -WindowStyle Hidden -PassThru
$p.Id | Set-Content "$run\evidence\debugger.pid"
Write-Output "Debugger PID $($p.Id); run $run"


