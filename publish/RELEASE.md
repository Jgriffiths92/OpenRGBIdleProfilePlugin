# Release Notes Template

## Version

1.0.0

## Summary

Initial public release of the OpenRGB Idle Profile Plugin.

## Highlights

- Idle-to-profile switching after configurable inactivity timeout
- Active profile restore on user input resume
- Top-level OpenRGB tab interface
- Profile refresh button
- Startup active profile apply option
- Resume cooldown/debounce option
- Debug logging toggle
- Live state and last-switch status

## Compatibility

- OpenRGB plugin API version: 4
- Tested OpenRGB version: 1.0rc2wr0
- Tested platform: Windows
- Build toolchain: Qt 5.15.2 (msvc2019_64), MSVC Build Tools, CMake

## Installation

1. Download `IdleProfilePlugin.dll`
2. Copy to `%APPDATA%\\OpenRGB\\plugins`
3. Restart OpenRGB

## Known Issues

- Windows-only validation so far
- Build must match OpenRGB Qt major version (Qt5 for Qt5 OpenRGB)

## Checks Before Publishing

- [ ] Bump version in plugin metadata if changed
- [ ] Build release DLL
- [ ] Verify plugin loads in OpenRGB
- [ ] Verify idle -> active switching
- [ ] Attach DLL to GitLab Release assets
- [ ] Update README compatibility notes if needed
