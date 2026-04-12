param(
    [string]$QtCMakeBin = "",
    [string]$QtPrefixPath = "",
    [string]$BuildDir = "build",
    [string]$Config = "Release",
    [string]$OpenRGBExeDir = "",
    [switch]$Clean
)

$ErrorActionPreference = "Stop"

$pluginRoot = Split-Path -Parent $MyInvocation.MyCommand.Path
$repoRoot = Split-Path -Parent (Split-Path -Parent $pluginRoot)
$buildPath = Join-Path $pluginRoot $BuildDir

if ($Clean -and (Test-Path $buildPath)) {
    Remove-Item -Recurse -Force $buildPath
}

if (-not $QtCMakeBin) {
    $cmake = Get-Command cmake -ErrorAction SilentlyContinue
} else {
    $cmakePath = Join-Path $QtCMakeBin "cmake.exe"
    if (Test-Path $cmakePath) {
        $cmake = Get-Item $cmakePath
    }
}

if (-not $cmake) {
    throw "cmake.exe not found. Install CMake or pass -QtCMakeBin <path to bin containing cmake.exe>."
}

if (-not $QtPrefixPath) {
    throw "QtPrefixPath is required. Example: -QtPrefixPath C:\\Qt\\6.8.0\\msvc2022_64"
}

New-Item -ItemType Directory -Force -Path $buildPath | Out-Null
Push-Location $buildPath

try {
    & $cmake.FullName "-S" $pluginRoot "-B" $buildPath "-DCMAKE_BUILD_TYPE=$Config" "-DCMAKE_PREFIX_PATH=$QtPrefixPath"
    & $cmake.FullName "--build" $buildPath "--config" $Config
}
finally {
    Pop-Location
}

$dll = Get-ChildItem -Path $buildPath -Recurse -Filter "*.dll" | Where-Object { $_.Name -match "IdleProfilePlugin" } | Select-Object -First 1

if (-not $dll) {
    throw "Build finished but IdleProfilePlugin DLL was not found under $buildPath"
}

Write-Host "Built plugin: $($dll.FullName)"

if ($OpenRGBExeDir) {
    $pluginsDir = Join-Path $OpenRGBExeDir "plugins"
    New-Item -ItemType Directory -Force -Path $pluginsDir | Out-Null
    Copy-Item -Force $dll.FullName (Join-Path $pluginsDir $dll.Name)
    Write-Host "Staged to: $(Join-Path $pluginsDir $dll.Name)"
}
