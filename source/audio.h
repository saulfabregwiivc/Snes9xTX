/****************************************************************************
 * Snes9x Nintendo Wii/Gamecube Port
 *
 * Tantric 2008-2021
 *
 * audio.h
 *
 * Audio driver
 * Audio is fixed to 32Khz/16bit/Stereo
 ***************************************************************************/

void InitAudio ();
void AudioStart ();
void AudioMode(int mode);
void ShutdownAudio();
