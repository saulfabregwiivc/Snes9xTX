/****************************************************************************
 * Snes9x Nintendo Wii/GameCube Port
 *
 * softdev July 2006
 * crunchy2 May 2007
 * Michniewski 2008
 * Tantric 2008-2022
 * Tanooki 2019-2023
 *
 * video.h
 *
 * Video routines
 ***************************************************************************/

#ifndef _GCVIDEOH_
#define _GCVIDEOH_

#include <ogcsys.h>

#include "snes9x/snes9x.h"
#include "utils/bezel.h"

void AllocGfxMem();
void InitGCVideo ();
void StopGX();
void ResetVideo_Emu();
void setGFX();
void update_video (int width, int height);
void ResetVideo_Menu();
void TakeScreenshot();
void ClearScreenshot();
void Menu_Render();
void Menu_DrawImg(f32 xpos, f32 ypos, u16 width, u16 height, u8 data[], f32 degrees, f32 scaleX, f32 scaleY, u8 alphaF );
void Menu_DrawRectangle(f32 x, f32 y, f32 width, f32 height, GXColor color, u8 filled);

// Auto crop of black borders (GCSettings.AutoCrop)
void AutoCropLoadGame();   // after a game is loaded: learned layouts
void AutoCropSaveGame();   // writes them when they changed
void AutoCropReset();      // forgets the current crop (reset, mode change)
// Screen position (0 to 1 across the screen) to SNES pixel coordinates
void AutoCropMapPointer(float fx, float fy, int *x, int *y);

// Bezels (GCSettings.Bezel): the PNG files in snes9xtx/bezels, for the menu
int BezelList(char (*names)[BEZEL_NAME_MAX], int max);
void BezelPrepare(); // loads it before the game runs again (menu -> game)

extern GXRModeObj *vmode;
extern int screenheight;
extern int screenwidth;
extern bool progressive;
extern u8 * gameScreenPng;
extern int gameScreenPngSize;
extern u32 FrameTimer;
extern bool vmode_60hz;
extern int timerstyle;
extern int CheckVideo;

#endif
