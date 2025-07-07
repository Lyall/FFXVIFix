## ⚠️ This repository has been migrated
Active development has moved to [Codeberg](https://codeberg.org/Lyall/FFXVIFix).  
This repo is archived and will no longer receive updates.

# Final Fantasy XVI Fix
[![Patreon-Button](https://github.com/user-attachments/assets/4f074cf5-3a94-4fe8-b915-35270f762b72)](https://www.patreon.com/Wintermance) [![ko-fi](https://ko-fi.com/img/githubbutton_sm.svg)](https://ko-fi.com/W7W01UAI9)<br />
[![Github All Releases](https://img.shields.io/github/downloads/Lyall/FFXVIFix/total.svg)](https://github.com/Lyall/FFXVIFix/releases)

This is a fix for Final Fantasy XVI that adds ultrawide/narrower support, uncaps framerate in cutscenes, lets you adjust gameplay FOV and much more.

## Features
### General
- Adjust gameplay/lock-on FOV, camera distance and camera horizontal position.
- JXL screenshot quality option and fixes hitching while taking screenshots.
- Allow the use of motion blur + frame generation.
- Disable depth of field.
- Enable background audio.
- Resize game window.
- Disable display sleep/screensaver while the game is running.

### Gameplay Tweaks
- Adjust stagger timers.
- Adjust damage scale.

### Performance
- Disable 30FPS cap in cutscenes/photo mode or set your own framerate limit.
- Allow frame generation in cutscenes.
- Disable graphics debugger checks.
- Adjust upper and lower bounds of dynamic resolution.
- Adjust level of detail distance.
 
### Ultrawide/Narrower
- Remove pillarboxing/letterboxing.
- Fixed HUD scaling with configurable HUD size.
- Fixed FOV scaling at <16:9.

## Installation
- Grab the latest release of FFXVIFix from [here.](https://github.com/Lyall/FFXVIFix/releases)
- Extract the contents of the release zip in to the the game folder. e.g. ("**steamapps\common\FINAL FANTASY XVI**" for Steam).

### Steam Deck/Linux Additional Instructions
🚩**You do not need to do this if you are using Windows!**
- Open up the game properties in Steam and add `WINEDLLOVERRIDES="dinput8=n,b" %command%` to the launch options.

<details>
<summary>Installing FFXVIFix as a Reloaded II Mod</summary>
  
*This applies to both Windows and Steam Deck/Linux*

Before starting, make sure to **delete any FFXVIFix files** inside of the game's files **if you have already have used this fix** previously (*FFXVIFix.ini*, *FFXVIFix.asi* and *dinput8.dll*)

To make sure FFXVIFix loads alongside any Reloaded II mods you are using, follow these steps:

- Download the file marked `FFXVIFix_Reloaded-II.zip` from the the latest release.

- Click "Download Mods" in Reloaded-II, then drag and drop `FFXVIFix_Reloaded-II.zip` onto the window. (Alternatively: [Manual Install](https://reloaded-project.github.io/Reloaded-II/QuickStart/))

- Enable it in your `Reloaded-II` mod list.
- You should now be able to start the game and see both FFXVIFix and Reloaded II mods working.

</details>

## Configuration
- See **FFXVIFix.ini** to adjust settings for the fix.

## Known Issues
Please report any issues you see.
This list will contain bugs which may or may not be fixed.

- Some movie cutscenes may appear stretched for some users. Try enabling the "Alternate" option in `FFXVIFix.ini` if this occurs for you. ([#82](https://github.com/Lyall/FFXVIFix/issues/82))

## Screenshots
| ![ezgif-4-40bff8440e](https://github.com/user-attachments/assets/74416ddf-43fe-4607-b608-c2e499cbe78b) |
|:--:|
| Gameplay |

## Credits
[Ultimate ASI Loader](https://github.com/ThirteenAG/Ultimate-ASI-Loader) for ASI loading. <br />
[inipp](https://github.com/mcmtroffaes/inipp) for ini reading. <br />
[spdlog](https://github.com/gabime/spdlog) for logging. <br />
[safetyhook](https://github.com/cursey/safetyhook) for hooking.
