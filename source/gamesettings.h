/****************************************************************************
 * Snes9x Nintendo Wii/GameCube Port
 *
 * VrepliroidV 2026
 *
 * gamesettings.h
 *
 * Settings of one game: the video, audio and emulation options can be kept
 * for a game (Game Settings > Per-Game). They are stored next to the game's
 * saves as <game>.cfg and replace the global options while it is loaded.
 ***************************************************************************/

#ifndef _GAMESETTINGS_H_
#define _GAMESETTINGS_H_

// The loaded game uses its own settings.
bool GameSettingsActive();

// After Memory.ROMFilename is set, before the game is loaded: puts back the
// global options, then the game's own ones if it has them.
void GameSettingsLoad();

// Keeps the current options for the loaded game from now on.
bool GameSettingsEnable();

// The loaded game uses the global options again; its file is deleted.
void GameSettingsDisable();

// Writes the game's file when it uses its own settings.
void GameSettingsSave();

// Around saving the global preferences: the global values of the per-game
// options are put in GCSettings, then the game's values back.
void GameSettingsBeginGlobalSave();
void GameSettingsEndGlobalSave();

#endif
