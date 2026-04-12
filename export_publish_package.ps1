param(
    [string]$OutputDir = "publish",
    [switch]$IncludeBuiltDll
)

$ErrorActionPreference = "Stop"

$pluginRoot = Split-Path -Parent $MyInvocation.MyCommand.Path
$outPath = Join-Path $pluginRoot $OutputDir

if (Test-Path $outPath) {
    Remove-Item -Recurse -Force $outPath
}

New-Item -ItemType Directory -Path $outPath | Out-Null

$files = @(
    "CMakeLists.txt",
    "IdleProfilePlugin.cpp",
    "IdleProfilePlugin.h",
    "IdleProfileWidget.cpp",
    "IdleProfileWidget.h",
    "build_and_stage.ps1",
    "BUILD_TEST.md",
    "README.md",
    "RELEASE.md",
    "OPENRGB_MR_TEMPLATE.md",
    "GITLAB_UPLOAD_STEPS.md"
)

foreach ($file in $files) {
    $src = Join-Path $pluginRoot $file
    if (Test-Path $src) {
        Copy-Item $src (Join-Path $outPath $file) -Force
    }
}

if ($IncludeBuiltDll) {
    $dll = Join-Path $pluginRoot "build\Release\IdleProfilePlugin.dll"
    if (Test-Path $dll) {
        Copy-Item $dll (Join-Path $outPath "IdleProfilePlugin.dll") -Force
    }
}

$zipPath = Join-Path $pluginRoot "IdleProfilePlugin_publish.zip"
if (Test-Path $zipPath) {
    Remove-Item $zipPath -Force
}

Compress-Archive -Path (Join-Path $outPath "*") -DestinationPath $zipPath -Force

Write-Host "Created publish package: $zipPath"
