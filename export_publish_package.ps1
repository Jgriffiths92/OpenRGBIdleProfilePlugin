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
    "plugins\CMakeLists.txt",
    "plugins\IdleProfilePlugin\CMakeLists.txt",
    "plugins\IdleProfilePlugin\IdleProfilePlugin.cpp",
    "plugins\IdleProfilePlugin\IdleProfilePlugin.h",
    "plugins\IdleProfilePlugin\IdleProfileWidget.cpp",
    "plugins\IdleProfilePlugin\IdleProfileWidget.h",
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
        $dst = Join-Path $outPath $file
        $dstDir = Split-Path -Parent $dst
        if ($dstDir -and -not (Test-Path $dstDir)) {
            New-Item -ItemType Directory -Path $dstDir -Force | Out-Null
        }
        Copy-Item $src $dst -Force
    }
}

if ($IncludeBuiltDll) {
    $dll = Get-ChildItem -Path (Join-Path $pluginRoot "build") -Recurse -Filter "IdleProfilePlugin.dll" -ErrorAction SilentlyContinue | Select-Object -First 1
    if ($dll) {
        Copy-Item $dll.FullName (Join-Path $outPath "IdleProfilePlugin.dll") -Force
    }
}

$zipPath = Join-Path $pluginRoot "IdleProfilePlugin_publish.zip"
if (Test-Path $zipPath) {
    Remove-Item $zipPath -Force
}

Compress-Archive -Path (Join-Path $outPath "*") -DestinationPath $zipPath -Force

Write-Host "Created publish package: $zipPath"
