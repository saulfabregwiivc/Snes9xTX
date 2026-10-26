/****************************************************************************
 * Snes9x Nintendo Wii/GameCube Port
 *
 * cheatmgr.h
 *
 * Cheat handling
 ***************************************************************************/

#ifndef _CHEATMGR_H_
#define _CHEATMGR_H_

#include <stdint.h>

void WiiSetupCheats();
void ToggleCheat(uint32_t num);
void DeleteCheat(uint32_t num);
void RenameCheat(uint32_t num, const char *name);
int CheatCount();
int AddCheatCode(const char *text, char *bad, int badlen);
bool SaveCheats();

#endif
