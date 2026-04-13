# Release Notes Template

## Version

1.0.2

## Summary

Media-aware idle suppression update for the OpenRGB Idle Profile Plugin.

## Highlights

- Idle-to-profile switching after configurable inactivity timeout
- Active profile restore on user input resume
- Optional screen-off detection to trigger idle profile when display powers down
- Optional active profile restore when display powers back on
- Optional media playback guard to prevent idle switch while media is playing
- Hybrid media detection: Global Media Session API with audio-session fallback
- Media playback guard is enabled by default
- Monitor compatibility note in plugin UI for display power events
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

- [x] Bump version in plugin metadata if changed
- [x] Build release DLL
- [X] Verify plugin loads in OpenRGB
- [X] Verify idle -> active switching
- [x] Attach DLL to GitLab Release assets
- [x] Attach DLL to GitHub Release assets
- [x] Update README compatibility notes if needed
