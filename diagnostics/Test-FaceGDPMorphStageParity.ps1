param(
    [Parameter(Mandatory)][string]$GameRoot,
    [Parameter(Mandatory)][string]$X86GDPProbe,
    [Parameter(Mandatory)][string]$X86FaceProbe,
    [Parameter(Mandatory)][string]$X64FaceProbe,
    [Parameter(Mandatory)][string]$SequenceFile,
    [Parameter(Mandatory)][string]$OutputDirectory,
    [double]$Tolerance = 0.0001
)
$ErrorActionPreference = 'Stop'
$culture = [Globalization.CultureInfo]::InvariantCulture
$game = (Resolve-Path -LiteralPath $GameRoot).Path
$gdp = Join-Path $game 'Res\FaceGenHead.gdp'
$tree = Join-Path $game 'Res\FaceGenHead.mmt'
$x86GDP = (Resolve-Path -LiteralPath $X86GDPProbe).Path
$x86Face = (Resolve-Path -LiteralPath $X86FaceProbe).Path
$x64Face = (Resolve-Path -LiteralPath $X64FaceProbe).Path
$machine = @{}
foreach ($executable in @($x86GDP,$x86Face,$x64Face)) {
    $pe = [IO.File]::ReadAllBytes($executable)
    if ($pe.Length -lt 0x40 -or [BitConverter]::ToUInt16($pe,0) -ne 0x5a4d) {
        throw "Invalid PE executable: $executable"
    }
    $header = [BitConverter]::ToInt32($pe,0x3c)
    if ($header -lt 0 -or $header -gt $pe.Length - 6 -or
        [BitConverter]::ToUInt32($pe,$header) -ne 0x4550) {
        throw "Invalid PE header: $executable"
    }
    $machine[$executable] = [BitConverter]::ToUInt16($pe,$header + 4)
}
if ($machine[$x86GDP] -ne 0x14c -or $machine[$x86Face] -ne 0x14c -or
    $machine[$x64Face] -ne 0x8664) {
    throw 'Morph-stage parity requires original x86 probes and a native x64 FaceProbe'
}
$sequence = (Resolve-Path -LiteralPath $SequenceFile).Path
$output = [IO.Path]::GetFullPath($OutputDirectory)
New-Item -ItemType Directory -Force -Path $output | Out-Null
$oldPath = $env:PATH
$oldDump = $env:S2_FACE_DUMP_TRANSFORMER_INPUT_PREFIX
$oldMacro = $env:S2_FACE_FORCE_MACRO_NAME
$oldValue = $env:S2_FACE_FORCE_MACRO_VALUE
try {
    $env:PATH = "$game;$oldPath"
    $neutral = Join-Path $output 'neutral'
    $nose = Join-Path $output 'nose'
    foreach ($case in @(
        [pscustomobject]@{ Prefix=$neutral; Args=@() },
        [pscustomobject]@{ Prefix=$nose; Args=@('Nose','0.5') }
    )) {
        $env:S2_FACE_DUMP_TRANSFORMER_INPUT_PREFIX = $case.Prefix
        & $x86GDP $gdp $tree $case.Prefix @($case.Args)
        if ($LASTEXITCODE -ne 0) { throw "x86 transformer failed: $($case.Prefix)" }
    }
    if ((Get-FileHash -LiteralPath "$neutral-morphed-head.bin").Hash -ne
        (Get-FileHash -LiteralPath "$nose-morphed-head.bin").Hash) {
        throw 'Nose unexpectedly changed the serialized morph-head base'
    }
    $env:S2_FACE_DUMP_TRANSFORMER_INPUT_PREFIX = $oldDump
    $env:S2_FACE_FORCE_MACRO_NAME = 'Nose'
    $env:S2_FACE_FORCE_MACRO_VALUE = '0.5'
    $head = "$neutral-morphed-head.bin"
    $x86Csv = Join-Path $output 'replayed-x86.csv'
    $x64Csv = Join-Path $output 'replayed-x64.csv'
    & $x86Face $head $x86Csv $sequence $tree
    if ($LASTEXITCODE -ne 0) { throw 'x86 morph replay failed' }
    & $x64Face $head $x64Csv $sequence $tree
    if ($LASTEXITCODE -ne 0) { throw 'x64 morph replay failed' }

    $neutralReference = @(Import-Csv -LiteralPath "$neutral-morph-processed.csv")
    $noseReference = @(Import-Csv -LiteralPath "$nose-morph-processed.csv")
    $left = @(Import-Csv -LiteralPath $x86Csv)
    $right = @(Import-Csv -LiteralPath $x64Csv)
    if ($neutralReference.Count -ne 419 -or $noseReference.Count -ne 419 -or
        $left.Count -ne 2514 -or $right.Count -ne $left.Count) {
        throw 'Unexpected morph-stage vertex count'
    }
    $maximumNativeDelta = 0.0
    $maximumReplayDelta = 0.0
    $changed = 0
    for ($i = 0; $i -lt 419; ++$i) {
        if ($neutralReference[$i].vertex -ne "$i" -or $noseReference[$i].vertex -ne "$i") {
            throw "Morph reference vertex identity mismatch: $i"
        }
        $moving = $false
        foreach ($axis in 'x','y','z') {
            $a = [double]::Parse($neutralReference[$i].$axis, $culture)
            $b = [double]::Parse($noseReference[$i].$axis, $culture)
            if ([Math]::Abs($a - $b) -gt $Tolerance) { $moving = $true }
        }
        if ($moving) { ++$changed }
    }
    if ($changed -eq 0) { throw 'Nose morph did not move any vertices' }
    $neutralRows = 0
    $sequenceRows = 0
    for ($i = 0; $i -lt $left.Count; ++$i) {
        $a = $left[$i]
        $b = $right[$i]
        if ($a.case -ne $b.case -or $a.time -ne $b.time -or $a.vertex -ne $b.vertex) {
            throw "Morph replay row identity mismatch: $i"
        }
        $vertex = [int]$a.vertex
        if ($vertex -lt 0 -or $vertex -ge 419) { throw "Invalid vertex: $i" }
        if ($a.case -eq 'neutral') {
            ++$neutralRows
            $reference = $neutralReference[$vertex]
        } elseif ($a.case -eq 'sequence') {
            ++$sequenceRows
            $reference = $noseReference[$vertex]
        } else {
            throw "Unexpected replay case: $($a.case)"
        }
        foreach ($axis in 'x','y','z') {
            $expected = [double]::Parse($reference.$axis, $culture)
            $original = [double]::Parse($a.$axis, $culture)
            $native = [double]::Parse($b.$axis, $culture)
            if (![double]::IsFinite($expected) -or ![double]::IsFinite($original) -or
                ![double]::IsFinite($native)) { throw "Nonfinite morph coordinate: $i $axis" }
            $maximumReplayDelta = [Math]::Max($maximumReplayDelta, [Math]::Abs($expected - $original))
            $maximumNativeDelta = [Math]::Max($maximumNativeDelta, [Math]::Abs($expected - $native))
        }
    }
    if ($neutralRows -ne 419 -or $sequenceRows -ne 2095) {
        throw "Unexpected morph replay cases: neutral=$neutralRows sequence=$sequenceRows"
    }
    if ($maximumReplayDelta -gt $Tolerance -or $maximumNativeDelta -gt $Tolerance) {
        throw "Morph-stage parity failed: x86 replay=$maximumReplayDelta native=$maximumNativeDelta"
    }
    Write-Output "FACEGEN MORPH-STAGE PARITY PASS: rows=$($left.Count), moved=$changed, x86 replay delta=$($maximumReplayDelta.ToString('G9',$culture)), native delta=$($maximumNativeDelta.ToString('G9',$culture)), tolerance=$Tolerance"
}
finally {
    $env:PATH = $oldPath
    $env:S2_FACE_DUMP_TRANSFORMER_INPUT_PREFIX = $oldDump
    $env:S2_FACE_FORCE_MACRO_NAME = $oldMacro
    $env:S2_FACE_FORCE_MACRO_VALUE = $oldValue
}
