param(
    [Parameter(Mandatory)][string]$GameRoot,
    [string]$FFmpeg = 'ffmpeg',
    [string]$FFprobe = 'ffprobe'
)
$ErrorActionPreference = 'Stop'
$root = (Resolve-Path -LiteralPath $GameRoot).Path
$video = @(Get-ChildItem -LiteralPath (Join-Path $root 'res\video') -File -Filter '*.bik')
$music = @(Get-ChildItem -LiteralPath (Join-Path $root 'res\Music') -File -Filter '*.wav')
if (!$video.Count -or !$music.Count) { throw 'Missing original video or music fixtures' }

foreach ($file in $video) {
    $description = & $FFprobe -v error -show_entries stream=codec_name,codec_type `
        -of json -- $file.FullName | ConvertFrom-Json
    if ($LASTEXITCODE -ne 0 -or !$description.streams) { throw "Cannot probe $($file.Name)" }
    & $FFmpeg -hide_banner -nostdin -v error -xerror -i $file.FullName `
        -map '0:v:0' -frames:v 2 -f null NUL
    if ($LASTEXITCODE -ne 0) { throw "Cannot decode video $($file.Name)" }
    if (@($description.streams | Where-Object codec_type -eq 'audio').Count) {
        & $FFmpeg -hide_banner -nostdin -v error -xerror -i $file.FullName `
            -map '0:a:0' -t 0.5 -f null NUL
        if ($LASTEXITCODE -ne 0) { throw "Cannot decode movie audio $($file.Name)" }
    }
}
foreach ($file in $music) {
    & $FFmpeg -hide_banner -nostdin -v error -xerror -i $file.FullName `
        -map '0:a:0' -t 0.5 -f null NUL
    if ($LASTEXITCODE -ne 0) { throw "Cannot decode music $($file.Name)" }
}
Write-Output "Decoded $($video.Count) Bink movies and $($music.Count) music tracks with FFmpeg."
