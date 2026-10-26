/****************************************************************************
 * Snes9x Nintendo Wii/GameCube Port
 *
 * VrepliroidV 2026
 *
 * gamesettings.cpp
 *
 * Settings of one game, see gamesettings.h
 ***************************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stddef.h>

#include "snes9xtx.h"
#include "filebrowser.h"
#include "preferences.h"
#include "gamesettings.h"
#include "snes9x/memmap.h"

#define GAME_FILE_HEADER "snes9xtx-game 1"

// The options a game can have of its own: video, audio and emulation.
// Buttons, controllers, folders and menu options stay global.
enum { FIELD_INT, FIELD_FLOAT, FIELD_STRING };

struct GameField
{
	const char *name;
	int type;
	size_t offset;
	size_t size;
};

#define INT_FIELD(f)    { #f, FIELD_INT,    offsetof(SGCSettings, f), sizeof(int) }
#define FLOAT_FIELD(f)  { #f, FIELD_FLOAT,  offsetof(SGCSettings, f), sizeof(float) }
#define STRING_FIELD(f) { #f, FIELD_STRING, offsetof(SGCSettings, f), sizeof(((SGCSettings *) 0)->f) }

static const GameField fields[] =
{
	// video
	INT_FIELD(videomode),
	INT_FIELD(render),
	INT_FIELD(bilinear),
	INT_FIELD(aspect),
	FLOAT_FIELD(zoomHor),
	FLOAT_FIELD(zoomVert),
	INT_FIELD(xshift),
	INT_FIELD(yshift),
	INT_FIELD(VideoFilter),
	INT_FIELD(HiResMode),
	INT_FIELD(FrameSkip),
	INT_FIELD(ShowFrameRate),
	INT_FIELD(crosshair),
	INT_FIELD(AutoCrop),
	INT_FIELD(AutoCropScale),
	STRING_FIELD(Bezel),
	// audio
	INT_FIELD(MuteSound),
	INT_FIELD(Interpolation),
	// emulation
	INT_FIELD(Region),
	INT_FIELD(sfxOverclock),
	INT_FIELD(cpuOverclock),
	INT_FIELD(NoSpriteLimit),
};

#define FIELD_COUNT (sizeof(fields) / sizeof(fields[0]))

static SGCSettings globals; // global values of the fields while a game uses its own
static bool active = false;

static inline void *FieldIn(SGCSettings *s, const GameField &f)
{
	return (char *) s + f.offset;
}

static void CopyFields(SGCSettings *dst, SGCSettings *src)
{
	for(unsigned int i = 0; i < FIELD_COUNT; i++)
		memcpy(FieldIn(dst, fields[i]), FieldIn(src, fields[i]), fields[i].size);
}

static void SwapFields(SGCSettings *a, SGCSettings *b)
{
	for(unsigned int i = 0; i < FIELD_COUNT; i++)
	{
		char *x = (char *) FieldIn(a, fields[i]), *y = (char *) FieldIn(b, fields[i]);
		for(size_t j = 0; j < fields[i].size; j++)
		{
			const char tmp = x[j];
			x[j] = y[j];
			y[j] = tmp;
		}
	}
}

// <saves>/<game>.cfg
static bool GamePath(char *path)
{
	if(Memory.ROMFilename[0] == 0 || !MakeFilePath(path, FILE_SRAM, Memory.ROMFilename, -1))
		return false;
	char *ext = strrchr(path, '.');
	if(!ext || (ext - path) + sizeof(".cfg") > MAXPATHLEN)
		return false;
	strcpy(ext, ".cfg");
	return true;
}

static bool ReadGameFile(const char *path)
{
	FILE *file = fopen(path, "r");
	if(!file)
		return false;

	char line[256];
	if(!fgets(line, sizeof(line), file) || strncmp(line, GAME_FILE_HEADER, strlen(GAME_FILE_HEADER)) != 0)
	{
		fclose(file);
		return false;
	}

	while(fgets(line, sizeof(line), file))
	{
		char *eq = strchr(line, '=');
		if(!eq)
			continue;
		*eq = 0;
		for(unsigned int i = 0; i < FIELD_COUNT; i++)
		{
			if(strcmp(line, fields[i].name) != 0)
				continue;
			if(fields[i].type == FIELD_STRING)
			{
				char *value = eq + 1;
				value[strcspn(value, "\r\n")] = 0;
				snprintf((char *) FieldIn(&GCSettings, fields[i]), fields[i].size, "%s", value);
			}
			else if(fields[i].type == FIELD_FLOAT)
			{
				float v;
				if(sscanf(eq + 1, "%f", &v) == 1)
					*(float *) FieldIn(&GCSettings, fields[i]) = v;
			}
			else
			{
				int v;
				if(sscanf(eq + 1, "%d", &v) == 1)
					*(int *) FieldIn(&GCSettings, fields[i]) = v;
			}
			break;
		}
	}
	fclose(file);
	return true;
}

bool GameSettingsActive()
{
	return active;
}

void GameSettingsSave()
{
	char path[MAXPATHLEN];
	if(!active || !GamePath(path))
		return;

	FILE *file = fopen(path, "w");
	if(!file)
		return;
	fprintf(file, "%s\n", GAME_FILE_HEADER);
	for(unsigned int i = 0; i < FIELD_COUNT; i++)
	{
		if(fields[i].type == FIELD_STRING)
			fprintf(file, "%s=%s\n", fields[i].name, (char *) FieldIn(&GCSettings, fields[i]));
		else if(fields[i].type == FIELD_FLOAT)
			fprintf(file, "%s=%f\n", fields[i].name, *(float *) FieldIn(&GCSettings, fields[i]));
		else
			fprintf(file, "%s=%d\n", fields[i].name, *(int *) FieldIn(&GCSettings, fields[i]));
	}
	fclose(file);
}

void GameSettingsLoad()
{
	char path[MAXPATHLEN];

	// back to the global options
	if(active)
	{
		CopyFields(&GCSettings, &globals);
		active = false;
	}

	if(GamePath(path))
	{
		CopyFields(&globals, &GCSettings);
		if(ReadGameFile(path))
		{
			active = true;
			FixInvalidSettings();
		}
	}

	ApplyEmulationSettings();
}

bool GameSettingsEnable()
{
	if(active)
		return true;
	if(SNESROMSize == 0)
		return false;

	CopyFields(&globals, &GCSettings); // the current values are the global ones
	active = true;
	GameSettingsSave();
	return true;
}

void GameSettingsDisable()
{
	char path[MAXPATHLEN];
	if(!active)
		return;

	if(GamePath(path))
		remove(path);
	CopyFields(&GCSettings, &globals);
	active = false;
	ApplyEmulationSettings();
}

void GameSettingsBeginGlobalSave()
{
	if(active)
		SwapFields(&GCSettings, &globals);
}

void GameSettingsEndGlobalSave()
{
	if(active)
		SwapFields(&GCSettings, &globals);
}
