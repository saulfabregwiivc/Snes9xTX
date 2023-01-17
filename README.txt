Snes9x TX
===================

Snes9x TX is a fork of Snes9x GX: https://github.com/dborth/snes9xgx
(Under GPL License)

Update History
--------------

---Snes9x TX 1.1.3---
---January 15 2023---

- Updated to the latest Snes9x GX (git 3940616)
- Disabled game info message when "show frame rate" option is enabled
- Added option to turn on/off frame skipping in video settings (enabled by default)
  frame skipping is highly recommended with some games using special chips (especially SuperFX)
  Disable it for other games if you prefer optimal frame rate despite some potential slowdowns
- Added option to force SNES region (experimental) and create emulation settings in main settings
  When launching Snes9x TX just choose a region and then launch a game
  For another game it will be necessary to launch any game in automatic region
  then go back to emulation settings to choose the desired region or restart Snes9x TX
  Another constraint is that you must load a save state with the same region
  than the one with which it was made (otherwise the timing will not be correct)
- Added separate PAL audio sample rate for original render (fix audio hiccups in 50Hz video mode)
- Fixed the PAL games resolution (PAL 50Hz video mode is recommended for have a correct ratio)
- Added progressive PAL video mode (576p)
- Reorganized menu options
- Renamed turbo mode option to "fast forward" (turbo mode name is reminiscent of auto fire)
- Removed catalan and updated all other language files
- Fixed sound effects volume setting and replace enter.ogg audio file
- Updated UI PNG images
- Added DualShock 3 (wired) support based on SickSaxis lib by xerpi (thanks niuus and JabuPL!)
  How to use:
  1. Start up Snes9x TX
  2. Start a game with your Wiimote/GC Controller/etc
  3. Press the PS button on your DualShock 3 and connect it to your Wii with a USB cable
  4. It should connect, one led on the controller should light up
  5. Done, you can play now on a DualShock 3 Controller

---Snes9x TX 1.1.2---
---September 26 2022---

- Added new setting button to turn on/off start Satellaview BIOS with BS games in menu settings
- Added option to turn on/off game sound in audio settings
- Updated french language file

---Snes9x TX 1.1.1---
---September 4 2022---

- Updated to the latest Snes9x (git 8685db4)
- Updated to the latest Snes9x GX (git 88deb03)
- Removed exit sound of UI (sometimes causes a small audio crackle when returning to game)
- Removed option to turn it on/off stereo inversion
- Removed filtered render and replaced render option by an option to turn on/off 240p output
- Added option to turn on/off bilinear filtering in video settings (works with 240p output)
- Added low/medium/max CPU overclocking option in emulation hacks settings
- Added new emulation hacks settings button and moved screenshot button to main game menu
- Updated all language files
- Updated and changed position of some text and UI PNG images

---Snes9x TX 1.1.0---
---June 23 2022---

- Updated to the latest Snes9x GX 4.5.3
- Disabled peripheral devices by default if not selected in controller settings
- Added new options to turn it on/off sprite limit and hi-res mode in video settings
  Disable hi-res mode to keep video filtering active with games using this mode
- Added credits button in main menu settings (thanks InfiniteBlue!)
- Updated credits
- Updated all language files
- Updated UI PNG images
- Compiled with latest devkitPPC and libfat with UStealth Mod (thanks SaulFabre!)

---Snes9x TX 1.0.9c---
---May 20 2022---

- Added option to turn it on/off stereo inversion in audio settings
  Enabled by default to fix Wii inverted L/R audio channels
- Updated all language files

---Snes9x TX 1.0.9b---
---May 19 2022---

- Fixed SuperFX overclock issues (thanks InfiniteBlue!)

---Snes9x TX 1.0.9---
---May 17 2022---

- Updated to the latest Snes9x GX (git 2e0df05)
- Restored turbo mode and added button remapping feature and submenu (thanks InfiniteBlue!)
- Added Cubic and Sinc sound interpolation methods
- Added option to overclock the SuperFX to 20 Mhz (thanks niuus!)
- Fixed not saving SuperFX overclock option in settings
- Added saving show frame rate option in settings
- Removed show local time option
- Removed all hq2x video filter code (keep the necessary for TV Mode and Scale2x filters)
- Added Scale2x video filter (takes less resources than hq2x filters)
- Restored video filtering setting and rename Scanlines filter to TV Mode
- Reorganized video rendering mode to Original/Unfiltered/Filtered (Unfiltered by default)
- Reorganized menu video settings
- Updated and cleaned up all language files
- Updated UI PNG images
- Compiled with latest devkitPPC

---Snes9x TX 1.0.8---
---September 1 2021---

- Updated to the latest Snes9x GX (git 5bff07b)
- Fixed 128KByte SRAM support
- Reorganized menu video settings
- Removed all old crosshairs (not used by Snes9x TX)
- Added custom crosshairs for Super Scope/Justifier
- Fixed not working option to disable crosshair
- Removed rumble function from menu (your batteries will last longer)
- Updated UI PNG images
- Compiled with latest devkitPPC/libogc
- Updated Forwarder Channel (1.3)

---Snes9x TX 1.0.7---
---April 22 2021---

- Changed Filtering setting name to CRT Filter with On/Off option
- Replaced Scanlines filter by tv2x filter (lighter scanlines that do not darken image)
- Removed hq2x filters (these filters take too many resources for Wii)
- Removed Soft and Sharp video filtering (not work with Snes9x GX)
- Removed "Auto" of name of saves and option to enable/disable it
- Updated Forwarder Channel (1.2)

---Snes9x TX 1.0.6---
---March 25 2021---

- Updated to the latest Snes9x GX 4.5.2 (git eb0e39d)
- Updated UI PNG images
- Updated French translation
- Fixed my error which broke hq2x filtering with PAL games
- Added new alternate Forwarder Channel (based on Flo_o's channel)
- Added compiled GameCube version

---Snes9x TX 1.0.5---
---February 22 2021---

- Updated to the latest Snes9x GX 4.5.1 (git 09149db)
- Removed SMB support (SMB 1.0 has a major security issue due to flaws in the protocol)
- Removed Retrode 2 and Hornet USB controller support (not really useful for Wii)
- Changed max game image dimensions to 640x512 to fix screenshots
- Changed UI stripes size
- Reorganized credits
- Reverted moved app version in credits box info
- Reworked UI PNG images (improved colors, shadow effects and logo)
- Removed menu music (use a bg_music.ogg file in root snes9xtx folder)
- Updated French translation
- Changed root folder name to snes9xtx
- Compiled with latest devkitPPC/libogc
- Updated Forwarder Channel (1.1)

---Snes9x TX 1.0.4---
---June 16 2020---

- Updated to the latest Snes9x GX (git cd66ff8)
- Updated UI PNG images
- Added support for .bs files (new extension of last update to No-Intro Satellaview roms)
- Disabled BS-X bios when loading Satellaview games (thanks fledge68!)

---Snes9x TX 1.0.3---
---June 5 2020---

- Updated to the latest Snes9x GX (git bb341fe)
- Updated UI PNG images
- Added new Forwarder Channel (based on qwertymodo's channel)
- Moved app version in main menu to credits box info
- Changed snes9xgx root folder name to snes9x
- Changed Mushroom name to Snes9x TX and add new logo

---Snes9x TX 1.0.2---
---February 26 2020---

- Updated Wii/vWii Forwarder Channel (1.1)
- Added a version in Snes US colors
- Updated UI PNG images

---Snes9x TX 1.0.1---
---February 20 2020---

- Updated to the latest Snes9x GX (git 8a120b0)
- Updated UI PNG images
- Changed Mod name: welcome to Snes9x GX Mushroom

---Snes9x TX 1.0.0---
---2019---

- Added vWii Forwarder Channel for Wii U
- Disabled Multitape by default
- Added L+R+START trigger to go back to menu with Classic Controller
  Work with SNES Classic Mini controller
- Removed turbo mode on right joystick
- Changed UI and Forwarder Channel colors
- Based on the latest commit of Snes9x GX
