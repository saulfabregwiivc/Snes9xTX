/****************************************************************************
 * Snes9x Nintendo Wii/GameCube Port
 *
 * softdev July 2006
 * crunchy2 May 2007-July 2007
 * Michniewski 2008
 * Tantric 2008-2022
 * Tanooki 2019-2023
 *
 * snes9xtx.h
 *
 * This file controls overall program flow. Most things start and end here!
 ***************************************************************************/

#ifndef _SNES9XTX_H_
#define _SNES9XTX_H_

#include "utils/FreeTypeGX.h"
#include "snes9x.h"
#include "filter.h"
#include "filelist.h"

#define APPNAME 			"Snes9x TX"
#define APPVERSION 			"1.1.3"
#define APPFOLDER 			"snes9xtx"
#define PREF_FILE_NAME		"settings.xml"

#define MAXPATHLEN 1024
#define NOTSILENT 0
#define SILENT 1

const char pathPrefix[9][8] =
{ "", "sd:/", "usb:/", "dvd:/", "carda:/", "cardb:/", "port2:/" };

enum {
	DEVICE_AUTO,
	DEVICE_SD,
	DEVICE_USB,
	DEVICE_DVD,
	DEVICE_SD_SLOTA,
	DEVICE_SD_SLOTB,
	DEVICE_SD_PORT2
};

enum {
	FILE_SRAM,
	FILE_SNAPSHOT,
	FILE_ROM,
	FILE_CHEAT
};

enum
{
	CTRL_PAD,
	CTRL_SCOPE,
	CTRL_JUST,
	CTRL_MOUSE,
	CTRL_MOUSE_PORT2,
	CTRL_MOUSE_BOTH_PORTS,
	CTRL_PAD2,
	CTRL_PAD4,
	CTRL_LENGTH
};

const char ctrlName[8][24] =
{ 
	"SNES Controller", 
	"Super Scope", 
	"Konami Justifier", 
	"SNES Mouse", 
	"SNES Mouse (Port 2)", 
	"SNES Mouse (2)", 
	"SNES Controllers (2)", 
	"SNES Controllers (4)"
};

enum {
	FASTFORWARD_BUTTON_RSTICK = 0,
	FASTFORWARD_BUTTON_A,
	FASTFORWARD_BUTTON_B,
	FASTFORWARD_BUTTON_X,
	FASTFORWARD_BUTTON_Y,
	FASTFORWARD_BUTTON_L,
	FASTFORWARD_BUTTON_R,
	FASTFORWARD_BUTTON_ZL,
	FASTFORWARD_BUTTON_ZR,
	FASTFORWARD_BUTTON_Z,
	FASTFORWARD_BUTTON_C,
	FASTFORWARD_BUTTON_1,
	FASTFORWARD_BUTTON_2,
	FASTFORWARD_BUTTON_PLUS,
	FASTFORWARD_BUTTON_MINUS
};

enum {
	LANG_JAPANESE = 0,
	LANG_ENGLISH,
	LANG_GERMAN,
	LANG_FRENCH,
	LANG_SPANISH,
	LANG_ITALIAN,
	LANG_DUTCH,
	LANG_SIMP_CHINESE,
	LANG_TRAD_CHINESE,
	LANG_KOREAN,
	LANG_PORTUGUESE,
	LANG_BRAZILIAN_PORTUGUESE,
	LANG_TURKISH,
	LANG_LENGTH
};

struct SGCSettings{
	int		AutoLoad;
	int		AutoSave;
	int		LoadMethod; // For ROMS: Auto, SD, DVD, USB
	int		SaveMethod; // For SRAM, Freeze, Prefs: Auto, SD, USB
	char	LoadFolder[MAXPATHLEN]; 	// Path to game files
	char	LastFileLoaded[MAXPATHLEN]; // Last file loaded filename
	char	SaveFolder[MAXPATHLEN]; 	// Path to save files
	char	CheatFolder[MAXPATHLEN]; 	// Path to cheat files
	char	ScreenshotsFolder[MAXPATHLEN]; // Path to screenshots files
	char	CoverFolder[MAXPATHLEN]; 	// Path to cover files
	char	ArtworkFolder[MAXPATHLEN]; 	// Path to artwork files
	int		HideSRAMSaving;
	int		AutoloadGame;

	float	zoomHor; // Horizontal zoom amount
	float	zoomVert; // Vertical zoom amount
	int		videomode; // 0 - Automatic, 1 - NTSC (480i), 2 - Progressive (480p), 3 - Progressive (576p), 4 - PAL (50Hz), 5 - PAL (60Hz)
	int		render;		// 0 - Default, 1 - Original (240p)
	int		bilinear;    // Bilinear filtering
	int		VideoFilter;
	int		Controller;
	int		FastForward;
	int		FastForwardButton;
	int		HiResMode;
	int		FrameSkip;
	int		ShowFrameRate;
	int		crosshair;
	int		aspect;
	int		xshift;		// Video output shift
	int		yshift;
	int		MuteSound;
	int		Interpolation;
	int		sfxOverclock;
	int		cpuOverclock;
	int		NoSpriteLimit;
	int		Satellaview;
	int		Region;
	int		WiimoteOrientation;
	int		ExitAction;
	int		MusicVolume;
	int		SFXVolume;
	int		language;
	int		PreviewImage;
};

void ExitApp();
void ShutdownWii();
bool SupportedIOS(u32 ios);
bool SaneIOS(u32 ios);
extern struct SGCSettings GCSettings;
extern int ScreenshotRequested;
extern int ConfigRequested;
extern int ShutdownRequested;
extern int ExitRequested;
extern char appPath[];
extern FreeTypeGX *fontSystem[];
extern bool isWiiVC;
static inline bool IsWiiU(void)
{
	return ((*(vu16*)0xCD8005A0 == 0xCAFE) || isWiiVC);
}
static inline bool IsWiiUFastCPU(void)
{
	return ((*(vu16*)0xCD8005A0 == 0xCAFE) && ((*(vu32*)0xCD8005B0 & 0x20) == 0));
}
#endif
