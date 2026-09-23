param(
    [Parameter(Mandatory)][string]$SourceDirectory,
    [Parameter(Mandatory)][string]$OutputDirectory,
    [string]$AlwaysIncludeDirectory,
    [ValidateRange(2,1000)][int]$Modulo = 13,
    [ValidateRange(0,999)][int]$Residue = 0
)
$ErrorActionPreference = 'Stop'
if ($Residue -ge $Modulo) { throw 'Residue must be smaller than Modulo' }
$source = (Resolve-Path -LiteralPath $SourceDirectory).Path
$output = [IO.Path]::GetFullPath($OutputDirectory)
if (Test-Path -LiteralPath $output) {
    if (@(Get-ChildItem -LiteralPath $output -Force).Count) {
        throw "Refusing to add fixtures to nonempty directory: $output"
    }
} else {
    New-Item -ItemType Directory -Path $output | Out-Null
}
$selected = @{}
$sha = [Security.Cryptography.SHA256]::Create()
try {
    $files = @(Get-ChildItem -LiteralPath $source -Filter 'sequence-*.bin' -File |
        Where-Object Name -Match '^sequence-\d+-\d+\.bin$' | Sort-Object Name)
    foreach ($file in $files) {
        $hash = $sha.ComputeHash([Text.Encoding]::UTF8.GetBytes($file.Name))
        $value = [BitConverter]::ToUInt32($hash,0)
        if ($value % $Modulo -eq $Residue) { $selected[$file.Name] = $file.FullName }
    }
} finally {
    $sha.Dispose()
}
if ($AlwaysIncludeDirectory) {
    $include = (Resolve-Path -LiteralPath $AlwaysIncludeDirectory).Path
    foreach ($file in (Get-ChildItem -LiteralPath $include -Filter 'sequence-*.bin' -File |
            Where-Object Name -Match '^sequence-\d+-\d+\.bin$')) {
        $selected[$file.Name] = $file.FullName
    }
}
if (!$selected.Count) { throw 'Hash selection produced no sequences' }
foreach ($name in ($selected.Keys | Sort-Object)) {
    Copy-Item -LiteralPath $selected[$name] -Destination (Join-Path $output $name)
}
Write-Host "Selected $($selected.Count) of $($files.Count) game sequences (SHA-256(name) modulo $Modulo = $Residue, plus always-included files)"
