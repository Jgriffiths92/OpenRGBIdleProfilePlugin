# Idle Profile Plugin Build and Test (Windows)

## 1) Prerequisites

Install these tools first:

- Visual Studio 2022 Build Tools (Desktop development with C++)
- Qt 5.15.x with MSVC kit (for example: msvc2019_64)
- CMake 3.22+

If you want package-manager installs:

- `winget install Kitware.CMake`
- `winget install Microsoft.VisualStudio.2022.BuildTools`

Note: Qt via winget may vary by package availability. Installing Qt with the Qt Online Installer is usually most reliable.

## 2) Open a developer shell

Use "x64 Native Tools Command Prompt for VS 2022" or open PowerShell after VS build tools are installed and available.

## 3) Build the plugin

From this folder:

`plugins/idle profile plugin`

Run:

- `./build_and_stage.ps1 -QtPrefixPath C:\Qt\5.15.2\msvc2019_64 -Clean`

Optional (copy DLL directly next to OpenRGB.exe):

- `./build_and_stage.ps1 -QtPrefixPath C:\Qt\5.15.2\msvc2019_64 -OpenRGBExeDir "C:\Program Files\OpenRGB" -Clean`

Expected output includes a line like:

- `Built plugin: ...\IdleProfilePlugin.dll`

## 4) Stage plugin manually (if needed)

Copy the built DLL to one of these plugin folders:

- `%APPDATA%\OpenRGB\plugins`
- `<OpenRGB install folder>\plugins`

## 5) Run OpenRGB and verify load

1. Start OpenRGB.
2. Open the Plugins page.
3. Verify "Idle Profile Plugin" appears and can be enabled.
4. Open Settings tab and verify an "Idle Profile" plugin panel appears.

If the plugin does not appear:

- Check OpenRGB log file under `%APPDATA%\OpenRGB\logs`
- Look for lines from PluginManager about loading/casting/API mismatch.

## 6) Functional test

1. In the plugin widget, set:
   - Enable Idle RGB Switching: on
   - Idle time: 10 seconds (for quick testing)
   - Idle profile: a valid existing OpenRGB profile name/path
   - Active profile: a valid existing OpenRGB profile name/path
2. Save by changing any field (auto-saves).
3. Stop keyboard/mouse input for >10 seconds.
4. Confirm idle profile applies.
5. Move mouse or press key.
6. Confirm active profile applies within ~2 seconds.

## 7) Quick troubleshooting

- "cannot open source file QWidget/Qt*": Qt kit not installed or wrong QtPrefixPath.
- CMake configure fails to find Qt: ensure `-QtPrefixPath` points to the kit root containing `lib/cmake`.
- "Invalid metadata version" in OpenRGB logs: plugin was built with the wrong Qt major version; rebuild against Qt5 and use `-Clean`.
- Plugin loads but does nothing: verify profile names are valid and OpenRGB CLI profile loading works.
