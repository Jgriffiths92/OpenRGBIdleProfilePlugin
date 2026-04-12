# How To Upload This Plugin To GitLab

## A) Create your plugin repository

1. In GitLab, create a new project (for example `OpenRGBIdleProfilePlugin`).
2. Make it Public.

## B) Push this plugin code to your new repo

From this folder in terminal:

`plugins/idle profile plugin`

Run:

```powershell
git init
git add .
git commit -m "Initial release: OpenRGB Idle Profile Plugin"
git branch -M main
git remote add origin https://gitlab.com/j.griffiths92/openrgbidleprofileplugin.git
git push -u origin main
```

If this folder is already inside another git repository and you do not want history from parent repo, create a clean local folder and copy plugin files there first.

## C) Create a release with binary

1. Build DLL:

```powershell
./build_and_stage.ps1 -QtPrefixPath C:\Qt\5.15.2\msvc2019_64 -Clean
```

2. In GitLab project, open `Deploy -> Releases -> New release`.
3. Tag: `v0.1.0`
4. Title: `v0.1.0`
5. Notes: copy from `RELEASE.md`
6. Attach `build/Release/IdleProfilePlugin.dll` as a release asset.

## D) Add plugin to OpenRGB plugin list

1. Fork OpenRGB: https://gitlab.com/CalcProgrammer1/OpenRGB
2. In your fork, edit `README.md` under `OpenRGB Plugins` and add:

```md
- [OpenRGB Idle Profile Plugin](https://gitlab.com/<your-namespace>/<your-plugin-repo>) (by jgrif)
- [OpenRGB Idle Profile Plugin](https://gitlab.com/j.griffiths92/openrgbidleprofileplugin) (by jgrif)
```

3. Commit this one-line change.
4. Open Merge Request to upstream OpenRGB.
5. Use content in `OPENRGB_MR_TEMPLATE.md` for title/description.

## E) After merge

- Your plugin will be listed in OpenRGB README plugin section.
- Keep publishing new tagged releases with updated DLL assets.
