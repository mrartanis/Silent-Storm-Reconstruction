param(
    [Parameter(Mandatory)][string]$GameRoot,
    [Parameter(Mandatory)][string]$X86GDPProbe,
    [Parameter(Mandatory)][string]$X64GDPProbe,
    [Parameter(Mandatory)][string]$OutputDirectory,
    [double]$Tolerance = 0.0001
)
$ErrorActionPreference = 'Stop'
$culture = [Globalization.CultureInfo]::InvariantCulture
$game = (Resolve-Path -LiteralPath $GameRoot).Path
$x86 = (Resolve-Path -LiteralPath $X86GDPProbe).Path
$x64 = (Resolve-Path -LiteralPath $X64GDPProbe).Path
$output = [IO.Path]::GetFullPath($OutputDirectory)
New-Item -ItemType Directory -Force -Path $output | Out-Null
$gdp = Join-Path $game 'Res\FaceGenHead.gdp'
$mmt = Join-Path $game 'Res\FaceGenHead.mmt'
$names = @(
    'Eyes_Old','Eyes_01','Eyes_02','Eyes_03','Eyes_04','Eyes_05','Eyes_06','Eyes_07',
    'Hair_01','Hair_02','Hair_03','Hair_04','Hair_05','Hair_06','Hair_07','Hair_08','Hair_09','Hair_10',
    'Beards_01','Beards_02','Beards_03','Beards_04','Beards_05','Beards_06','Beards_07','Beards_08','Beards_09','Beards_10',
    'M01','M02','M03','F01','F02','F03','T01','T02','OLD',
    'European.0','Arab.0','African.0','Asian.0',
    '01','02','03','04','05','06','07','08','09','1_1','1_2','1_3','2_1','2_2','2_3','3_1','3_2','3_3',
    'missing'
)
$cases = @(
    @{ Name='neutral'; Sliders=@() },
    @{ Name='age'; Sliders=@('Age','0.5') },
    @{ Name='gender'; Sliders=@('Gender','0.5') },
    @{ Name='eyes'; Sliders=@('EyesColor','0.5') },
    @{ Name='hair'; Sliders=@('HairColor','0.5') },
    @{ Name='nationality'; Sliders=@('Nationality','0') },
    @{ Name='editor-default'; Sliders=@('Age','0','Gender','0','Nationality','-1','HairColor','1','WomanHair','1','EyesColor','-1','FaceDamage','-1','FacialColor','-1') }
)
foreach ($macro in @('EyesColor','WomanHair','FaceDamage','FacialColor',
                     'Nationality','HairColor','Gender','Age')) {
    foreach ($step in 0,25,50,75,100) {
        $expression = (($step - 50) / 50.0).ToString('G9',$culture)
        $cases += @{ Name="$macro-$step"; Sliders=@($macro,$expression) }
    }
}
$oldPath = $env:PATH
$oldCollect = $env:S2_FACE_COLLECT_USER_ITEMS
$oldNames = $env:S2_FACE_USER_ITEM_NAMES
try {
    $env:PATH = "$game;$oldPath"
    $env:S2_FACE_COLLECT_USER_ITEMS = '1'
    $env:S2_FACE_USER_ITEM_NAMES = $names -join ','
    foreach ($case in $cases) {
        $leftPrefix = Join-Path $output "$($case.Name)-x86"
        $rightPrefix = Join-Path $output "$($case.Name)-x64"
        & $x86 $gdp $mmt $leftPrefix @($case.Sliders)
        if ($LASTEXITCODE -ne 0) { throw "x86 UserItem probe failed: $($case.Name)" }
        & $x64 $gdp $mmt $rightPrefix @($case.Sliders)
        if ($LASTEXITCODE -ne 0) { throw "x64 UserItem probe failed: $($case.Name)" }
        $left = @(Import-Csv -LiteralPath "$leftPrefix-user-items.csv")
        $right = @(Import-Csv -LiteralPath "$rightPrefix-user-items.csv")
        if ($left.Count -ne $right.Count) { throw "UserItem row count mismatch: $($case.Name)" }
        $maximum = 0.0
        for ($i = 0; $i -lt $left.Count; ++$i) {
            $a = $left[$i]; $b = $right[$i]
            if ($a.source -ne $b.source -or $a.name -ne $b.name -or
                $a.count -ne $b.count -or $a.index -ne $b.index) {
                throw "UserItem identity mismatch: $($case.Name) row $i"
            }
            if ($a.value -ne '') {
                $delta = [Math]::Abs([double]::Parse($a.value,$culture) - [double]::Parse($b.value,$culture))
                $maximum = [Math]::Max($maximum,$delta)
                if ($delta -gt $Tolerance) { throw "UserItem value mismatch: $($case.Name) row $i delta $delta" }
            }
        }
        Write-Host "$($case.Name): $($left.Count) rows, max delta $($maximum.ToString('G9',$culture))"
    }
} finally {
    $env:PATH = $oldPath
    $env:S2_FACE_COLLECT_USER_ITEMS = $oldCollect
    $env:S2_FACE_USER_ITEM_NAMES = $oldNames
}
