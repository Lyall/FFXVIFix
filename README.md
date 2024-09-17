# Final Fantasy XVI Fix
[![Patreon-Button](https://github.com/user-attachments/assets/4f074cf5-3a94-4fe8-b915-35270f762b72)](https://www.patreon.com/Wintermance) [![ko-fi](https://ko-fi.com/img/githubbutton_sm.svg)](https://ko-fi.com/W7W01UAI9)<br />
[![Github All Releases](https://img.shields.io/github/downloads/Lyall/FFXVIFix/total.svg)](https://github.com/Lyall/FFXVIFix/releases)

This is a fix for Final Fantasy XVI that adds ultrawide/narrow display support, uncapped framerate in cutscenes and more.

## Features
### General
- Disable 30FPS cap in cutscenes/photo mode.
- Adjust gameplay FOV.
- Allow frame generation in cutscenes.
- JXL screenshot quality option and fixes hitching while taking screenshot.

### Ultrawide/Narrower
- Remove pillarboxing/letterboxing in borderless/fullscreen.
- Fixed HUD scaling.
- Fixed FOV scaling at <16:9.

## Installation
- Grab the latest release of FFXVIFix from [here.](https://github.com/Lyall/FFXVIFix/releases)
- Extract the contents of the release zip in to the the game folder. e.g. ("**steamapps\common\FINAL FANTASY XVI**" for Steam).

### Steam Deck/Linux Additional Instructions
🚩**You do not need to do this if you are using Windows!**
- Open up the game properties in Steam and add `WINEDLLOVERRIDES="dinput8=n,b" %command%` to the launch options.

## Configuration
- See **FFXVIFix.ini** to adjust settings for the fix.

## Known Issues
Please report any issues you see.
This list will contain bugs which may or may not be fixed.

- At ultrawide/narrower, the game world is visible in the background during some movie sequences.
- FOV is cropped at <16:9.

## Screenshots
| ![ezgif-4-40bff8440e](https://github.com/user-attachments/assets/74416ddf-43fe-4607-b608-c2e499cbe78b) |
|:--:|
| Gameplay |

## Credits
[Ultimate ASI Loader](https://github.com/ThirteenAG/Ultimate-ASI-Loader) for ASI loading. <br />
[inipp](https://github.com/mcmtroffaes/inipp) for ini reading. <br />
[spdlog](https://github.com/gabime/spdlog) for logging. <br />
[safetyhook](https://github.com/cursey/safetyhook) for hooking.
