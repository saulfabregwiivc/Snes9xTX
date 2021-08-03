Snes9x TX
===================

Snes9x TX is based on original Snes9x GX: https://github.com/dborth/snes9xgx
(Under GPL License)

Update History
--------------

---Snes9x TX 1.0.7---
---April 22 2021---

- Changed Filtering setting name to CRT Filter with On/Off option
- Replaced Scanlines filter per CRT filter (lighter scanlines that do not darken image)
- Removed hq2x filters (these filters take too many resources for Wii)
- Removed Soft and Sharp video filtering (not work with Snes9x GX)
- Removed "Auto" of name of saves and option to activate/disable it

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
- Reverted the app version in menu
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
- Disabled BS-X bios when loading Satellaview games (by fledge68)

---Snes9x TX 1.0.3---
---June 5 2020---

- Updated to the latest Snes9x GX (git bb341fe)
- Updated UI PNG images
- Added new Forwarder Channel (based on qwertymodo's channel)
- Moved the app version in menu to credits box info
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
  work with SNES Classic Mini controller
- Removed turbo mode on right joystick
- Changed UI and Forwarder Channel colors
- Based on the latest commit of Snes9x GX
