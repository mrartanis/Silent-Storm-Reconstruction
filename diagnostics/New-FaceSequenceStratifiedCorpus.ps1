param(
    [Parameter(Mandatory)][string]$FixtureDirectory,
    [Parameter(Mandatory)][string]$HeadFile,
    [Parameter(Mandatory)][string]$OutputDirectory
)
$ErrorActionPreference = 'Stop'
$source = (Resolve-Path -LiteralPath $FixtureDirectory).Path
$head = (Resolve-Path -LiteralPath $HeadFile).Path
$output = [IO.Path]::GetFullPath($OutputDirectory)
if (Test-Path -LiteralPath $output) { throw "Output already exists: $output" }
$headers = foreach ($file in Get-ChildItem -LiteralPath $source -Filter 'sequence-*.bin' -File) {
    $stream = [IO.File]::OpenRead($file.FullName)
    try {
        $bytes = New-Object byte[] 24
        if ($stream.Read($bytes,0,24) -ne 24 -or [BitConverter]::ToUInt32($bytes,0) -ne 0x46534d4d) {
            throw "Invalid sequence header: $($file.FullName)"
        }
        [pscustomobject]@{
            Name=$file.Name; Path=$file.FullName; Length=$file.Length
            Duration=[BitConverter]::ToUInt32($bytes,12)
            Tracks=[BitConverter]::ToUInt32($bytes,16)
            Prelude=[BitConverter]::ToUInt32($bytes,20)
        }
    } finally { $stream.Dispose() }
}
if ($headers.Count -lt 100) { throw 'Expected the expanded game sequence corpus' }
$chosen = [System.Collections.Generic.Dictionary[string,object]]::new([StringComparer]::Ordinal)
function Include-FaceSequences($items) {
    foreach ($item in $items) { $chosen[$item.Name] = $item }
}
Include-FaceSequences @($headers | Where-Object Prelude -eq 84)
Include-FaceSequences @($headers | Where-Object Prelude -eq 100 | Sort-Object Length -Descending | Select-Object -First 5)
Include-FaceSequences @($headers | Where-Object Tracks -eq 3 | Sort-Object Length | Select-Object -First 5)
Include-FaceSequences @($headers | Where-Object Tracks -eq 3 | Sort-Object Length -Descending | Select-Object -First 5)
Include-FaceSequences @($headers | Where-Object Tracks -eq 5 | Sort-Object Duration -Descending | Select-Object -First 5)
Include-FaceSequences @($headers | Where-Object Tracks -eq 5 | Sort-Object Length -Descending | Select-Object -First 5)
Include-FaceSequences @($headers | Where-Object Tracks -ge 7 | Sort-Object Tracks -Descending | Select-Object -First 5)
Include-FaceSequences @($headers | Where-Object Tracks -eq 5 | Get-Random -SetSeed 122676 -Count 5)
New-Item -ItemType Directory -Path $output | Out-Null
Copy-Item -LiteralPath $head -Destination (Join-Path $output (Split-Path -Leaf $head))
foreach ($item in $chosen.Values) {
    Copy-Item -LiteralPath $item.Path -Destination (Join-Path $output $item.Name)
}
$chosen.Values | Sort-Object Name | Select-Object Name,Length,Duration,Tracks,Prelude |
    Export-Csv -LiteralPath (Join-Path $output 'selection.csv') -NoTypeInformation
Write-Host "Selected $($chosen.Count) structurally varied sequences from $($headers.Count) game fixtures: $output"
