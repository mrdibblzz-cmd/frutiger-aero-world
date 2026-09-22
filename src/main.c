
FRUTIGER AERO WORLD - PSP EDITION
=================================

What this is
------------
A large scrolling 2D open-world PSP homebrew prototype built around the
Frutiger Aero look from the reference images you supplied:
bright sky, clean futuristic buildings, green parks, water/reef areas,
wind turbines, bubbles, colorful blob characters, and a central Aero Dome.

The world is 3200 x 2400, so the camera scrolls as you explore rather than
loading separate stages.

PSP runtime
-----------
The game is written for the classic LuaPlayer / LuaPlayerHM environment.
LuaPlayer is a PSP Lua script player; its documentation describes installing
apps under the LuaPlayer Applications folder.

This package contains the game source, not an EBOOT.PBP, because the PSP
development compiler/runtime was not available in the build environment.

Install with an existing LuaPlayer
-----------------------------------
1. Put the `FrutigerAeroWorld` folder into:
   PSP/GAME/luaplayer/Applications/

2. Launch LuaPlayer from the PSP XMB.

3. Open the FrutigerAeroWorld application / index.lua.

For LuaPlayer variants that use a different Applications location, use the
same application folder structure shown by that LuaPlayer's readme.

Controls
--------
D-pad / analog : move
Square         : sprint
Cross          : collect / interact
Triangle       : toggle minimap
Start          : quit the game

Gameplay
--------
Explore all four broad regions:
- SKYLINE: futuristic towers and green city space
- REEF: underwater/coral-style zone
- AERO: central bright eco-city
- MEADOW: huge sunny park space

Collect Aero Orbs, talk to roaming blob friends, and visit landmarks.

Performance
-----------
The game deliberately uses simple rectangles and lightweight procedural
graphics instead of large textures, so it is much friendlier to PSP memory
and rendering limits.

Next upgrade ideas
------------------
- true 3D PSP renderer
- music and ambient water sounds
- buildings you can enter
- quests and NPC dialogue
- day/night cycle
- save/load
- more character customization
- title screen and XMB-style icon/audio
