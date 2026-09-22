param(
    [Parameter(Mandatory)][int]$ProcessId,
    [Parameter(Mandatory)][string]$OutputPath,
    [int]$ResizeWidth = 0,
    [int]$ResizeHeight = 0
)

$ErrorActionPreference = 'Stop'
Add-Type -AssemblyName System.Drawing
Add-Type @'
using System;
using System.Runtime.InteropServices;
public static class LabWindowCapture {
    [StructLayout(LayoutKind.Sequential)]
    public struct RECT { public int Left, Top, Right, Bottom; }
    [DllImport("user32.dll")]
    public static extern bool GetWindowRect(IntPtr hwnd, out RECT rect);
    [DllImport("user32.dll")]
    public static extern bool MoveWindow(IntPtr hwnd, int x, int y, int width, int height, bool repaint);
}
'@

$process = Get-Process -Id $ProcessId
$hwnd = $process.MainWindowHandle
if ($hwnd -eq [IntPtr]::Zero) { throw "Process $ProcessId has no main window" }
$rect = New-Object LabWindowCapture+RECT
if (-not [LabWindowCapture]::GetWindowRect($hwnd, [ref]$rect)) { throw 'GetWindowRect failed' }
if ($ResizeWidth -gt 0 -and $ResizeHeight -gt 0) {
    if (-not [LabWindowCapture]::MoveWindow($hwnd, $rect.Left, $rect.Top, $ResizeWidth, $ResizeHeight, $true)) {
        throw 'MoveWindow failed'
    }
    if (-not [LabWindowCapture]::GetWindowRect($hwnd, [ref]$rect)) { throw 'GetWindowRect after resize failed' }
}
$width = $rect.Right - $rect.Left
$height = $rect.Bottom - $rect.Top
if ($width -le 0 -or $height -le 0) { throw "Invalid window bounds ${width}x${height}" }

$bitmap = New-Object Drawing.Bitmap $width, $height
$graphics = [Drawing.Graphics]::FromImage($bitmap)
try {
    $graphics.CopyFromScreen($rect.Left, $rect.Top, 0, 0, $bitmap.Size)
    $parent = Split-Path -Parent $OutputPath
    if ($parent -and -not (Test-Path -LiteralPath $parent)) {
        New-Item -ItemType Directory -Path $parent | Out-Null
    }
    $bitmap.Save($OutputPath, [Drawing.Imaging.ImageFormat]::Png)
} finally {
    $graphics.Dispose()
    $bitmap.Dispose()
}

Write-Output $OutputPath
