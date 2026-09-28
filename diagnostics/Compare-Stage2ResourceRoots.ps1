param(
    [Parameter(Mandatory = $true)][string]$GameDb,
    [Parameter(Mandatory = $true)][string]$ResourceDir,
    [Parameter(Mandatory = $true)][string]$BuildDir,
    [switch]$List,
    [switch]$StrictBaseline
)

$ErrorActionPreference = 'Stop'
$db = (Resolve-Path -LiteralPath $GameDb).Path
$res = (Resolve-Path -LiteralPath $ResourceDir).Path
$build = (Resolve-Path -LiteralPath $BuildDir).Path
$databaseProbe = Join-Path $build 'NativeMapDatabaseTests.exe'
$packageProbe = Join-Path $build 'PortablePackageProbe.exe'
if (-not (Test-Path -LiteralPath $databaseProbe -PathType Leaf) -or
    -not (Test-Path -LiteralPath $packageProbe -PathType Leaf)) {
    throw 'Build NativeMapDatabaseTests and PortablePackageProbe first.'
}

$families = @('Buildings', 'Terrain', 'Waypoints', 'Units', 'Groups', 'Animations-guard')
$packageNames = @{
    'Buildings' = 'Buildings'; 'Terrain' = 'Terrain'; 'Waypoints' = 'Waypoints'
    'Units' = 'Units'; 'Groups' = 'Groups'; 'Animations-guard' = 'Animations'
}
$baseline = @{
    'Buildings' = @(985, 898, 5, 0, 82, 6331)
    'Terrain' = @(985, 371, 0, 0, 614, 3671)
    'Waypoints' = @(1357, 1285, 0, 0, 72, 2364)
    'Units' = @(1117, 62, 8, 0, 1047, 169)
    'Groups' = @(97, 34, 0, 0, 63, 42)
    'Animations-guard' = @(4, 4, 0, 0, 0, 2855)
}
$roots = @{}
foreach ($family in $families) { $roots[$family] = @{} }
$dbLines = & $databaseProbe $db --resource-root-ids
if ($LASTEXITCODE -ne 0) { throw "NativeMapDatabaseTests failed with exit code $LASTEXITCODE" }
foreach ($line in $dbLines) {
    if ($line -match '^resource_root family=([^ ]+) id=(-?\d+)$' -and $roots.ContainsKey($Matches[1])) {
        $roots[$Matches[1]][[int]$Matches[2]] = $true
    }
}

foreach ($family in $families) {
    $packageName = $packageNames[$family]
    $packagePath = Join-Path $res ($packageName + '.res')
    $packageIDs = @{}
    $packageLines = & $packageProbe $packagePath --list
    if ($LASTEXITCODE -ne 0) { throw "PortablePackageProbe failed for $packageName" }
    foreach ($line in $packageLines) {
        if ($line -match '^(-?\d+) (\d+) (\d+)$') {
            $packageIDs[[int]$Matches[1]] = [long]$Matches[3]
        }
    }

    $matchingDirs = @(Get-ChildItem -LiteralPath $res -Directory |
        Where-Object { $_.Name -ieq $packageName })
    if ($matchingDirs.Count -gt 1) { throw "Ambiguous loose directory for $packageName" }
    $looseIDs = @{}
    if ($matchingDirs.Count -eq 1) {
        foreach ($file in Get-ChildItem -LiteralPath $matchingDirs[0].FullName -File) {
            [int]$id = 0
            if ([int]::TryParse($file.Name, [ref]$id)) {
                $looseIDs[$id] = $file
            }
        }
    }

    $fromPackage = 0
    $overrides = 0
    $looseOnly = 0
    $missing = 0
    foreach ($id in @($roots[$family].Keys | Sort-Object)) {
        if ($looseIDs.ContainsKey($id)) {
            $source = 'loose'
            $bytes = $looseIDs[$id].Length
            if ($packageIDs.ContainsKey($id)) { ++$overrides } else { ++$looseOnly }
        } elseif ($packageIDs.ContainsKey($id)) {
            $source = 'package'
            $bytes = $packageIDs[$id]
            ++$fromPackage
        } else {
            $source = 'missing'
            $bytes = 0
            ++$missing
        }
        if ($List) {
            "resource_effective family=$family id=$id source=$source bytes=$bytes"
        }
    }
    "resource_resolution family=$family candidates=$($roots[$family].Count) package=$fromPackage loose_override=$overrides loose_only=$looseOnly missing=$missing"
    if ($fromPackage + $overrides + $looseOnly + $missing -ne $roots[$family].Count) {
        throw "Resolution count mismatch for $family"
    }
    if ($StrictBaseline) {
        $actual = @($roots[$family].Count, $fromPackage, $overrides,
            $looseOnly, $missing, $packageIDs.Count)
        for ($i = 0; $i -lt $actual.Count; ++$i) {
            if ($actual[$i] -ne $baseline[$family][$i]) {
                throw "Steam baseline mismatch for $family field $i`: expected $($baseline[$family][$i]), got $($actual[$i])"
            }
        }
    }
}
