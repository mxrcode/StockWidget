# Builds StockWidget in Release mode and packs it into a zip ready for a GitHub release.
#
#   .\deploy.ps1
#   .\deploy.ps1 -QtDir C:\Qt\6.12.0\mingw_64
#
# Result: build\StockWidget-<version>-win64.zip and its .sha256 file

param(
    # Qt kit (the folder with bin\windeployqt.exe)
    [string]$QtDir = "C:\Qt\6.12.0\mingw_64",
    [string]$BuildDir = "$PSScriptRoot\build"
)

$ErrorActionPreference = "Stop"

function Step($text) { Write-Host "`n== $text" -ForegroundColor Cyan }

function Invoke-Native {
    param([string]$Exe, [string[]]$Arguments)
    & $Exe @Arguments
    if ($LASTEXITCODE -ne 0) { throw "$Exe failed with exit code $LASTEXITCODE" }
}

# Newest folder matching the pattern in Qt\Tools, e.g. mingw1310_64
function Find-QtTool($pattern) {
    $tools = Join-Path (Split-Path (Split-Path $QtDir)) "Tools"
    $dir = Get-ChildItem $tools -Directory -Filter $pattern -ErrorAction SilentlyContinue | Sort-Object Name -Descending | Select-Object -First 1
    if (-not $dir) { throw "'$pattern' not found in $tools. Install it with the Qt Maintenance Tool." }
    return $dir.FullName
}

# --- Toolchain ---------------------------------------------------------------

Step "Toolchain"

if (-not (Test-Path "$QtDir\bin\windeployqt.exe")) { throw "Qt not found: $QtDir (use -QtDir)" }

$MinGW = Find-QtTool "mingw*_64"
$Ninja = Find-QtTool "Ninja"
$CMake = Join-Path (Find-QtTool "CMake_64") "bin"

# Qt and the compiler that built it must come first : other MinGW / Qt installs in PATH break the build
$env:PATH = "$QtDir\bin;$MinGW\bin;$Ninja;$CMake;$env:PATH"

Write-Host "Qt     : $QtDir"
Write-Host "MinGW  : $MinGW"
Write-Host "CMake  : $CMake"

# --- Version -----------------------------------------------------------------

$cmakeLists = Get-Content "$PSScriptRoot\CMakeLists.txt" -Raw
if ($cmakeLists -notmatch 'project\(StockWidget VERSION (\d+\.\d+\.\d+)') { throw "Version not found in CMakeLists.txt" }
$Version = $Matches[1]
$PackageName = "StockWidget-$Version-win64"

Write-Host "Version: $Version"

# --- Build -------------------------------------------------------------------

Step "Build (Release)"

$ReleaseDir = Join-Path $BuildDir "release"
if (Test-Path $ReleaseDir) { Remove-Item $ReleaseDir -Recurse -Force }

Invoke-Native cmake @("-S", $PSScriptRoot, "-B", $ReleaseDir, "-G", "Ninja", "-DCMAKE_BUILD_TYPE=Release", "-DCMAKE_PREFIX_PATH=$QtDir")
Invoke-Native cmake @("--build", $ReleaseDir)

# --- Package -----------------------------------------------------------------

Step "Package"

$PackageDir = Join-Path $BuildDir $PackageName
$ZipPath = Join-Path $BuildDir "$PackageName.zip"

if (Test-Path $PackageDir) { Remove-Item $PackageDir -Recurse -Force }
New-Item -ItemType Directory $PackageDir | Out-Null

Copy-Item "$ReleaseDir\StockWidget.exe" $PackageDir

# Qt and MinGW runtime libraries, plugins (platforms, tls, styles, ...).
# Only SVG images are used, no touch input.
Invoke-Native windeployqt @("--release", "--no-translations", "--no-opengl-sw", "--no-system-d3d-compiler",
    "--exclude-plugins", "qjpeg,qgif,qico,qtuiotouchplugin", "--verbose", "0", "$PackageDir\StockWidget.exe")

Copy-Item "$PSScriptRoot\LICENSE" "$PackageDir\LICENSE.txt"
Copy-Item "$PSScriptRoot\resources\fonts\LICENSE-Roboto.txt" $PackageDir

# Without these files the widget does not start or cannot reach the exchanges (HTTPS)
$required = @(
    "StockWidget.exe",
    "Qt6Core.dll", "Qt6Gui.dll", "Qt6Widgets.dll", "Qt6Network.dll", "Qt6Svg.dll",
    "libgcc_s_seh-1.dll", "libstdc++-6.dll", "libwinpthread-1.dll",
    "platforms\qwindows.dll",
    "tls\qschannelbackend.dll",
    "imageformats\qsvg.dll", "iconengines\qsvgicon.dll"
)
$missing = $required | Where-Object { -not (Test-Path (Join-Path $PackageDir $_)) }
if ($missing) { throw "Missing in the package: $($missing -join ', ')" }

if (Test-Path $ZipPath) { Remove-Item $ZipPath -Force }

# Entries are added one by one : Compress-Archive and ZipFile.CreateFromDirectory
# write "\" path separators in Windows PowerShell 5.1, which other unzip tools reject
Add-Type -AssemblyName System.IO.Compression, System.IO.Compression.FileSystem
$root = (Get-Item $PackageDir).Parent.FullName
$zip = [System.IO.Compression.ZipFile]::Open($ZipPath, [System.IO.Compression.ZipArchiveMode]::Create)
try {
    foreach ($file in Get-ChildItem $PackageDir -Recurse -File) {
        $entry = $file.FullName.Substring($root.Length + 1).Replace('\', '/')
        [System.IO.Compression.ZipFileExtensions]::CreateEntryFromFile($zip, $file.FullName, $entry, [System.IO.Compression.CompressionLevel]::Optimal) | Out-Null
    }
} finally {
    $zip.Dispose()
}

$hash = (Get-FileHash $ZipPath -Algorithm SHA256).Hash.ToLower()
Set-Content -Path "$ZipPath.sha256" -Value "$hash  $PackageName.zip" -Encoding ascii -NoNewline

# --- Summary -----------------------------------------------------------------

Step "Done"

$size = "{0:N1} MB" -f ((Get-Item $ZipPath).Length / 1MB)
Write-Host "Archive: $ZipPath ($size)"
Write-Host "SHA256 : $hash"
Write-Host "Folder : $PackageDir (can be run from there)"
