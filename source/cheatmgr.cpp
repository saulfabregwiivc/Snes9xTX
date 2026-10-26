/****************************************************************************
 * Snes9x Nintendo Wii/GameCube Port
 *
 * Tantric 2008-2022
 *
 * cheatmgr.cpp
 *
 * Cheat handling
 *
 * VrepliroidV 2026: codes typed in the cheats menu (Game Genie, Pro Action
 * Replay or raw), saved to the game's cheat file; codes enabled in the file
 * are active again when the game loads.
 ***************************************************************************/

#include <ctype.h>
#include <sys/stat.h>

#include "snes9x/port.h"
#include "snes9x/cheats.h"
#include "snes9x/bml.h"

#include "snes9xtx.h"
#include "fileop.h"
#include "filebrowser.h"
#include "cheatmgr.h"

#define MAX_CHEATS      150

extern SCheatData Cheat;

/****************************************************************************
 * LoadCheatFile
 *
 * Loads cheat file from save buffer
 * Custom version of S9xLoadCheatFile()
 ***************************************************************************/
static bool LoadCheatFile (int length)
{
	uint8 data [28];
	int offset = 0;

	while (offset < length) {
		if(Cheat.g.size() >= MAX_CHEATS || (length - offset) < 28)
			break;

		memcpy (data, savebuffer+offset, 28);
		offset += 28;

		SCheat c;
		char name[21];
		char cheat[10];
		c.byte = data[1];
		c.address = data[2] | (data[3] << 8) |  (data[4] << 16);
		memcpy (name, &data[8], 20);
		name[20] = 0;

		snprintf (cheat, 10, "%x=%x", c.address, c.byte);
		S9xAddCheatGroup (name, cheat);
	}
	return true;
}


// Cheats are applied while at least one of them is on.
static void UpdateCheatsEnabled() {
	for(unsigned int i=0; i < Cheat.g.size(); i++) {
		if(Cheat.g[i].enabled) {
			Cheat.enabled = TRUE;
			return;
		}
	}
	Cheat.enabled = FALSE;
}

void ToggleCheat(uint32 num) {
	if(num >= Cheat.g.size())
		return;

	if(Cheat.g[num].enabled) {
		S9xDisableCheatGroup(num);
	}
	else {
		S9xEnableCheatGroup(num);
	}

	UpdateCheatsEnabled();
}

void DeleteCheat(uint32 num) {
	if(num >= Cheat.g.size())
		return;

	S9xDeleteCheatGroup(num);
	UpdateCheatsEnabled();
}

void RenameCheat(uint32 num, const char *name) {
	if(num >= Cheat.g.size() || !name || !name[0])
		return;

	// the cheat file keeps names on one line, without double quotes
	char clean[64];
	snprintf(clean, sizeof(clean), "%s", name);
	for(char *p = clean; *p; p++)
		if(*p == '"' || (unsigned char)*p < ' ')
			*p = '\'';

	free(Cheat.g[num].name); // the core makes names with strdup
	Cheat.g[num].name = strdup(clean);
}

int CheatCount() {
	return Cheat.g.size();
}

/****************************************************************************
 * AddCheatCode
 *
 * Adds and turns on the codes typed by the user. Accepted, one or more
 * separated by +, spaces, commas or semicolons:
 *   Game Genie         XXXX-XXXX
 *   Pro Action Replay  XXXXXXXX (also XXXXXX:XX)
 *   raw                address=byte, address=byte?compare, address/byte
 * Returns the index of the new cheat, or -1 with the first invalid code in
 * bad (empty when nothing was typed).
 ***************************************************************************/
int AddCheatCode(const char *text, char *bad, int badlen) {
	char code[256] = "";
	char token[64];
	int len = 0, tlen = 0;

	bad[0] = 0;

	for(const char *p = text; ; p++) {
		const char c = *p;
		if(c == 0 || c == '+' || c == ',' || c == ';' || isspace((unsigned char)c)) {
			if(tlen > 0) {
				token[tlen] = 0;
				// Pro Action Replay written as XXXXXX:XX
				if(tlen == 9 && token[6] == ':')
					memmove(&token[6], &token[7], 3);

				char *valid = S9xCheatValidate(token);
				if(!valid) {
					snprintf(bad, badlen, "%s", token);
					return -1;
				}
				free(valid);

				if(len + tlen + 4 >= (int)sizeof(code)) {
					snprintf(bad, badlen, "%s", token);
					return -1;
				}
				len += sprintf(code + len, "%s%s", len ? " + " : "", token);
				tlen = 0;
			}
			if(c == 0)
				break;
		}
		else if(tlen < (int)sizeof(token) - 1) {
			token[tlen++] = toupper((unsigned char)c);
		}
		else {
			token[tlen] = 0;
			snprintf(bad, badlen, "%s", token);
			return -1;
		}
	}

	if(len == 0 || Cheat.g.size() >= MAX_CHEATS)
		return -1;

	// The codes as typed are the name of the cheat.
	int index = S9xAddCheatGroup(code, code);
	if(index < 0) {
		snprintf(bad, badlen, "%s", code);
		return -1;
	}
	ToggleCheat(index);
	return index;
}

/****************************************************************************
 * SaveCheats
 *
 * Writes the cheats of the game, with their on/off state, to its cheat file
 * (same place the cheats are loaded from).
 ***************************************************************************/
bool SaveCheats() {
	char filepath[1024];

	if(!MakeFilePath(filepath, FILE_CHEAT))
		return false;

	// create the cheats folder if needed
	char folder[1024];
	snprintf(folder, sizeof(folder), "%s", filepath);
	char *slash = strrchr(folder, '/');
	if(slash) {
		*slash = 0;
		mkdir(folder, 0777);
	}

	return S9xSaveCheatFile(filepath);
}

/****************************************************************************
 * SetupCheats
 *
 * Erases any prexisting cheats, loads cheats from a cheat file
 * Called when a ROM is first loaded
 ***************************************************************************/
void
WiiSetupCheats()
{
	char filepath[1024];
	int offset = 0;

	if(!MakeFilePath(filepath, FILE_CHEAT))
		return;

	AllocSaveBuffer();

	offset = LoadFile(filepath, SILENT);

	// load cheat file if present
	if(offset > 0)
	{
		bml_node bml;
		if (!bml.parse_file(filepath))
		{
			LoadCheatFile (offset);
		}

		bml_node *n = bml.find_subnode("cheat");
		if (n)
		{
			S9xLoadCheatsFromBMLNode (&bml);
		}

		if (!n)
		{
			LoadCheatFile (offset);
		}
	}


	FreeSaveBuffer ();

	// codes saved as on are applied from the first frame
	UpdateCheatsEnabled();
}
