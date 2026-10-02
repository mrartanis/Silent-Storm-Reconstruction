param(
    [Parameter(Mandatory)][string]$RunDirectory,
    [string]$Debugger,
    [string]$GameArguments,
    [string]$UserDataDirectory
)
$ErrorActionPreference='Stop'
$run=(Resolve-Path $RunDirectory).Path
if(!(Test-Path "$run\evidence\run.json")){throw 'Not a prepared lab run'}
if(!$UserDataDirectory){$UserDataDirectory=Join-Path $run 'user-data'}
$UserDataDirectory=[IO.Path]::GetFullPath($UserDataDirectory)
if(!$UserDataDirectory.StartsWith($run.TrimEnd('\')+'\',[StringComparison]::OrdinalIgnoreCase)){
    throw 'User data directory must stay inside this lab run'
}
$runMetadata = Get-Content "$run\evidence\run.json" -Raw | ConvertFrom-Json
if(!$GameArguments){$GameArguments=$runMetadata.Arguments}
if(!$GameArguments){$GameArguments='-windowed -800 -harness'}
if(!$Debugger){
    $buildMetadata = Get-Content "$run\evidence\build.json" -Raw | ConvertFrom-Json
    $binary=[IO.File]::ReadAllBytes((Join-Path $run 'game\Game.exe'))
    $peOffset=[BitConverter]::ToInt32($binary,0x3c)
    $debugArch = if([BitConverter]::ToUInt16($binary,$peOffset+4) -eq 0x8664){'x64'}else{'x86'}
    $Debugger = "C:\Program Files (x86)\Windows Kits\10\Debuggers\$debugArch\cdb.exe"
}
$runMetadata.Arguments = $GameArguments
$runMetadata | Add-Member -NotePropertyName UserDataDirectory -NotePropertyValue $UserDataDirectory -Force
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
$previousUserDataDir=$env:S2_USER_DATA_DIR
try {
    # The restored game writes user data outside its resource cwd. Keep each
    # lab run isolated, including runs started from the same clean archive.
    $env:S2_USER_DATA_DIR=$UserDataDirectory
    $p=Start-Process $Debugger -ArgumentList "-G -o -logo `"$run\evidence\debugger.log`" -cf `"$run\evidence\debugger.txt`" `"$run\game\Game.exe`" $GameArguments" -WorkingDirectory "$run\game" -WindowStyle Hidden -PassThru
} finally {
    $env:S2_USER_DATA_DIR=$previousUserDataDir
}
$p.Id | Set-Content "$run\evidence\debugger.pid"
Write-Output "Debugger PID $($p.Id); run $run"


