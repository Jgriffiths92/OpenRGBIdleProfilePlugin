param(
    [string]$QtCMakeBin = "",
    [string]$QtPrefixPath = "",
    [string]$OpenRGBSourceRoot = "",
    [string]$BuildDir = "build",
    [string]$Config = "Release",
    [string]$OpenRGBExeDir = "",
    [switch]$Clean
)

$ErrorActionPreference = "Stop"

$pluginRoot = Split-Path -Parent $MyInvocation.MyCommand.Path
$buildPath = Join-Path $pluginRoot $BuildDir

function Resolve-OpenRGBSourceRoot {
    param(
        [string]$ConfiguredPath,
        [string]$PluginPath
    )

    $candidates = @()

    if ($ConfiguredPath) {
        $candidates += $ConfiguredPath
    }

    $pluginParent = Split-Path -Parent $PluginPath
    $pluginGrandParent = Split-Path -Parent $pluginParent

    $candidates += @(
        (Join-Path $pluginGrandParent "OpenRGB"),
        (Join-Path $pluginGrandParent "openrgb"),
        (Join-Path $pluginParent "OpenRGB"),
        (Join-Path $pluginParent "openrgb"),
        (Join-Path $PluginPath "..\..")
    )

    foreach ($candidate in $candidates) {
        if (-not $candidate) {
            continue
        }

        $resolvedCandidate = [System.IO.Path]::GetFullPath($candidate)
        $pluginInterfaceHeader = Join-Path $resolvedCandidate "OpenRGB\OpenRGBPluginInterface.h"

        if (Test-Path $pluginInterfaceHeader) {
            return $resolvedCandidate
        }
    }

    throw "OpenRGBSourceRoot is required for standalone builds. Pass -OpenRGBSourceRoot <path to OpenRGB source root> so OpenRGB/OpenRGBPluginInterface.h is available."
}

function Resolve-CMakeExecutable {
    param(
        [string]$QtCMakeBin
    )

    if ($QtCMakeBin) {
        $cmakePath = Join-Path $QtCMakeBin "cmake.exe"
        if (Test-Path $cmakePath) {
            return Get-Item $cmakePath
        }
    }

    $cmake = Get-Command cmake -ErrorAction SilentlyContinue
    if ($cmake) {
        return $cmake
    }

    $commonCMakePaths = @(
        "C:\Program Files\CMake\bin\cmake.exe",
        "C:\Program Files (x86)\CMake\bin\cmake.exe"
    )

    foreach ($cmakePath in $commonCMakePaths) {
        if (Test-Path $cmakePath) {
            return Get-Item $cmakePath
        }
    }

    throw "cmake.exe not found. Install CMake, add it to PATH, or pass -QtCMakeBin <path to bin containing cmake.exe>."
}

if ($Clean -and (Test-Path $buildPath)) {
    Remove-Item -Recurse -Force $buildPath
}

$cmake = Resolve-CMakeExecutable -QtCMakeBin $QtCMakeBin

if (-not $QtPrefixPath) {
    throw "QtPrefixPath is required. Example: -QtPrefixPath C:\\Qt\\6.8.0\\msvc2022_64"
}

$resolvedOpenRGBSourceRoot = Resolve-OpenRGBSourceRoot -ConfiguredPath $OpenRGBSourceRoot -PluginPath $pluginRoot

New-Item -ItemType Directory -Force -Path $buildPath | Out-Null
Push-Location $buildPath

try {
    & $cmake.FullName "-S" $pluginRoot "-B" $buildPath "-DCMAKE_BUILD_TYPE=$Config" "-DCMAKE_PREFIX_PATH=$QtPrefixPath" "-DOPENRGB_SOURCE_ROOT=$resolvedOpenRGBSourceRoot"
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
