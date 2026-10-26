/*****************************************************************************\
     Snes9x - Portable Super Nintendo Entertainment System (TM) emulator.
                This file is licensed under the Snes9x License.
   For further information, consult the LICENSE file in the root directory.
\*****************************************************************************/

/* Automatic CPU overclock (Snes9x TX, VrepliroidV 2026), see autooc.cpp */

#ifndef _AUTOOC_H_
#define _AUTOOC_H_

// a taken branch 1 to this many bytes back may be a loop waiting for something
// (a flag set by NMI, a hardware register)
#define AUTOOC_SPIN_BYTES	32

#define AUTOOC_LEVELS		4	// off, low, medium, max

struct SAutoOC
{
	bool8	enabled;	// CPU Overclock is Auto
	bool8	waiting;	// the CPU is in WAI since waitStart
	int32	waitStart;
	uint32	spinTarget;	// last short loop: where it goes back to,
	uint16	spinA, spinX, spinY;	// the registers then
	int32	spinLast;	// and when
	int32	idle;		// master cycles the CPU waited this frame
	int32	apuWrites;	// writes to the sound chip ports this frame
	int32	level;		// overclock level in use, 0 to AUTOOC_LEVELS - 1
	int32	busy;		// per mille of the last frame the CPU worked
};

extern struct SAutoOC	AutoOC;

// Turns the automatic overclock on or off. On: the game starts again from
// no overclock; already on: the level in use is set again.
void S9xAutoOCSetEnabled (bool8 on);
// After a game is loaded (the ROM fixes chose its cycle timing).
void S9xAutoOCLoadGame (void);
// After a reset or a loaded state: no overclock until the game settles.
void S9xAutoOCReset (void);
// Once a frame, after S9xMainLoop.
void S9xAutoOCEndFrame (void);

// from the CPU core
void S9xAutoOCSpin (uint32 target);
void S9xAutoOCWaitBegin (void);
void S9xAutoOCWaitEnd (void);
void S9xAutoOCApuWrite (void);
void S9xAutoOCSample (void);

#define AUTOOC_SAMPLE_LINE	112	// S9xAutoOCSample is called at this line

#endif
