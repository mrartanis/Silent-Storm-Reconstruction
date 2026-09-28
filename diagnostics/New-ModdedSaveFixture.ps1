param(
    [Parameter(Mandatory)][string]$SourceSave,
    [Parameter(Mandatory)][string]$TargetSave,
    [Parameter(Mandatory)][ValidatePattern('^[A-Za-z0-9_-]{1,63}$')][string]$ModDirectory
)
$ErrorActionPreference = 'Stop'
$source = [IO.Path]::GetFullPath($SourceSave)
$target = [IO.Path]::GetFullPath($TargetSave)
if ($source -eq $target) { throw 'Source and target must differ' }
if ([IO.File]::Exists($target)) { throw 'Target save already exists' }
$bytes = [IO.File]::ReadAllBytes($source)
$headerSize = 256008
if ($bytes.Length -le $headerSize) { throw 'Source save is truncated' }
$magic = [BitConverter]::ToUInt32($bytes, 0)
if ($magic -ne 2190254114 -and $magic -ne 2173411105) { throw 'Unsupported save magic' }
if ([BitConverter]::ToInt32($bytes, 4) -ne 0) { throw 'Source save already names mods' }
$name = [Text.Encoding]::ASCII.GetBytes($ModDirectory)
if ($name.Length -ge 128) { throw 'Mod directory is too long' }
$result = [byte[]]::new($bytes.Length + 1 + $name.Length)
[Array]::Copy($bytes, 0, $result, 0, $headerSize)
[BitConverter]::GetBytes([int]1).CopyTo($result, 4)
$result[$headerSize] = [byte]($name.Length * 2)
[Array]::Copy($name, 0, $result, $headerSize + 1, $name.Length)
[Array]::Copy($bytes, $headerSize, $result, $headerSize + 1 + $name.Length,
    $bytes.Length - $headerSize)
$parent = [IO.Path]::GetDirectoryName($target)
[IO.Directory]::CreateDirectory($parent) | Out-Null
[IO.File]::WriteAllBytes($target, $result)
Write-Output "modded-save=$target bytes=$($result.Length) mod=$ModDirectory sha256=$((Get-FileHash -LiteralPath $target -Algorithm SHA256).Hash)"
