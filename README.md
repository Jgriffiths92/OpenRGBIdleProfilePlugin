# OpenRGB Idle Profile Plugin

Switch OpenRGB profiles automatically based on user inactivity.

## Features

- Switches to an idle profile after a configurable idle timeout
- Restores an active profile when input resumes
- Top-level OpenRGB tab UI
- Manual profile list refresh
- Optional startup behavior: apply active profile on OpenRGB launch
- Resume cooldown/debounce to prevent rapid toggles
- Optional debug logging
- Live status in UI (current state and last switch time)

## Compatibility

- OpenRGB plugin API: 4
- Tested OpenRGB version: 1.0rc2wr0 (update this to your tested release if needed)
- Tested platform: Windows
- Build/runtime note: Build with Qt5 for Qt5-based OpenRGB builds

## Install (Windows)

1. Download `IdleProfilePlugin.dll` from Releases.
2. Copy it to:
   - `%APPDATA%\\OpenRGB\\plugins`
3. Restart OpenRGB.
4. Enable the plugin if needed from the Plugins page.
5. Open the `Idle Profile` top tab.

## Build From Source (Windows)

Prerequisites:

- Visual Studio Build Tools with C++ workload
- CMake
- Qt 5.15.x MSVC kit (for Qt5 OpenRGB)

Build command from this folder:

```powershell
./build_and_stage.ps1 -QtPrefixPath C:\Qt\5.15.2\msvc2019_64 -Clean
```

## Configuration

- Enable Idle RGB Switching
- Idle time (seconds)
- Idle profile
- Active profile
- Apply active profile on OpenRGB launch
- Resume cooldown (seconds)
- Enable debug logging

## Troubleshooting

- Plugin not listed:
  - Confirm DLL is in `%APPDATA%\\OpenRGB\\plugins`
  - Restart OpenRGB
- "Invalid metadata version":
  - Plugin was built with wrong Qt major; rebuild against Qt5 if OpenRGB uses Qt5
- Profiles not visible:
  - Click `Refresh Profiles`
  - Ensure `.orp` profile files exist in OpenRGB config directory

## License

This project is licensed under the GNU General Public License v2.0 (GPL-2.0) - see the LICENSE file for details.
