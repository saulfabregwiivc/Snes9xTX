/****************************************************************************
 * Snes9x Nintendo Wii/GameCube Port
 *
 * Tantric 2008-2022
 * Tanooki 2019-2022
 *
 * preferences.h
 *
 * Preferences save/load to XML file
 ***************************************************************************/

void FixInvalidSettings();
void DefaultSettings();
bool SavePrefs (bool silent);
bool LoadPrefs ();
