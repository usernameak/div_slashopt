Divine Divinity Borderless Fullscreen Mod
=========================================

Improves game performance on modern Windows.

Requires [ThirteenAG's Ultimate ASI Loader](https://github.com/ThirteenAG/Ultimate-ASI-Loader/releases) to load the mod (name the loader's dll as `winmm.dll`, otherwise it will not work).

Installation
------------

Supported game versions:
* 1.0.0.62
* 1.0.0.61 (Russian)

1. Download the package from [Releases](https://github.com/usernameak/div_slashopt/releases) (the package already contains the loader, so you don't have to download it separately).
2. Unpack it to the game root folder.
3. Run `configtool.exe`.
4. Set the renderer to `Direct3D` and the resolution to the native resolution of your display (or you can set it to a lower resolution, if you want to play the game in windowed mode).
5. Hit `Apply & Close`; if the configuration tool says `No ddraw devices found`, just press OK.
6. If you see a blank screen right after hitting `Apply & Close` (sometimes it happens), try killing the game with task manager and restarting the game from `div.exe`, pressing `No` when asked whether you want to reset the configuration.

Known issues
============

* Only the left 320 pixels of the Credits screen are visible.
* Savegame previews (in the save/load menu and on loading screen) are glitched.
