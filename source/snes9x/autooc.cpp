/*****************************************************************************\
     Snes9x - Portable Super Nintendo Entertainment System (TM) emulator.
                This file is licensed under the Snes9x License.
   For further information, consult the LICENSE file in the root directory.
\*****************************************************************************/

/* Automatic CPU overclock (Snes9x TX, VrepliroidV 2026)

   Each frame the time the CPU spends waiting is measured: WAI, and short loops
   that keep branching back to the same place with the same registers (a game
   polling a flag or a register until the next frame). A game that does not
   wait at all within a frame is running late, and when that happens again the
   overclock goes up one level; when the frames would still have time left at
   the level below, it goes down again.

   The overclock is off while the screen is dark or fading, while NMI is off
   and while lots of data goes to the sound chip: screen changes and loading,
   where overclocked games tend to hang. Each game starts with it off, and it
   only goes up once the game was seen waiting (a game whose waiting is not
   recognised is left alone). */

#include "snes9x.h"
#include "memmap.h"
#include "ppu.h"
#include "autooc.h"

struct SAutoOC	AutoOC;

// one turn of a waiting loop takes less than this (master cycles); a longer
// turn ran something else (a routine called from the loop, DMA)
#define SPIN_TURN_MAX		512

// per mille of a frame the CPU worked
#define LAG_BUSY			985	// at least this: the frame was late
#define WAIT_BUSY			950	// at most this: the game waited
#define DOWN_BUSY			850	// at most this at the level below: go down

#define CALIBRATE_FRAMES	30	// frames the game must be seen waiting
#define UP_WAIT				10	// frames between two steps up
#define DOWN_FRAMES			120	// frames with time left before a step down
#define DOWN_HOLD			180	// frames without a step down after a step up,
#define DOWN_HOLD_MAX		1800	// doubled up to this when the level bounces
#define BOUNCE_FRAMES		300	// (a step up this soon after a step down)

#define HOLD_START			180	// frames off after a load, a reset or a state
#define HOLD_SCREEN			30	// frames off after the screen is back
#define HOLD_SOUND			60	// frames off after data went to the sound chip
#define SOUND_BURST			32	// writes to the sound ports in a frame

// cycles of the levels: one, slow one, two
static const int32	cycles[AUTOOC_LEVELS][3] =
{
	{ 6, 8, 12 },	// none (the game's own timing, see oneBase)
	{ 6, 6, 12 },	// low
	{ 4, 5,  6 },	// medium
	{ 3, 3,  3 }	// max
};

// how much more a frame takes one level below (per mille), a bit more than
// the usual mix of memory accesses gives
static const int32	slower[AUTOOC_LEVELS] = { 1000, 1350, 1450, 1700 };

static int32	oneBase = 6;		// one cycle of the game without overclock
static int32	hold;				// frames left at level 0
static int32	upWait, downWait, downHold = DOWN_HOLD;
static int32	sinceDown = BOUNCE_FRAMES, roomy;
static int32	waitedFrames;
static uint32	lagHistory;			// one bit a frame, 1 = late

static inline int32 Now (void)
{
	return (CPU.V_Counter * Timings.H_Max + CPU.Cycles);
}

static inline int32 Since (int32 then)
{
	int32	d = Now() - then;
	if (d < 0)
		d += Timings.V_Max * Timings.H_Max;
	return (d);
}

static void SetLevel (int32 level)
{
	AutoOC.level = level;
	Settings.OneClockCycle     = level ? cycles[level][0] : oneBase;
	Settings.OneSlowClockCycle = cycles[level][1];
	Settings.TwoClockCycles    = cycles[level][2];

	// the speeds the CPU keeps from them (as S9xSetPCBase sets them)
	CPU.FastROMSpeed = (Memory.FillRAM[0x420d] & 1) ? ONE_CYCLE : SLOW_ONE_CYCLE;
	CPU.MemSpeed = memory_speed(Registers.PBPC);
	CPU.MemSpeedx2 = CPU.MemSpeed << 1;

	lagHistory = 0;
	roomy = 0;
}

static void Calm (int32 frames)
{
	if (hold < frames)
		hold = frames;
	if (AutoOC.level != 0)
		SetLevel(0);
}

void S9xAutoOCReset (void)
{
	AutoOC.waiting = FALSE;
	AutoOC.spinTarget = 0xffffffff;
	AutoOC.idle = 0;
	AutoOC.apuWrites = 0;
	AutoOC.busy = 0;
	hold = HOLD_START;
	upWait = downWait = 0;
	downHold = DOWN_HOLD;
	sinceDown = BOUNCE_FRAMES;
	waitedFrames = 0;

	if (AutoOC.enabled)
		SetLevel(0);
	else
		AutoOC.level = 0;
}

void S9xAutoOCSetEnabled (bool8 on)
{
	if (!on)
	{
		// back to the speeds without overclock, as before Auto was chosen
		if (AutoOC.enabled && AutoOC.level != 0)
			SetLevel(0);
		AutoOC.enabled = FALSE;
		return;
	}

	if (AutoOC.enabled)
	{
		SetLevel(AutoOC.level);
		return;
	}

	AutoOC.enabled = TRUE;
	S9xAutoOCReset();
}

void S9xAutoOCLoadGame (void)
{
	// the ROM fixes slow a few games down (one cycle of 8)
	oneBase = (Settings.OneClockCycle == 8) ? 8 : 6;
	S9xAutoOCReset();
}

void S9xAutoOCSpin (uint32 target)
{
	const int32	now = Now();

	if (target == AutoOC.spinTarget && Registers.A.W == AutoOC.spinA &&
		Registers.X.W == AutoOC.spinX && Registers.Y.W == AutoOC.spinY)
	{
		int32	d = now - AutoOC.spinLast;
		if (d < 0)
			d += Timings.V_Max * Timings.H_Max;
		if (d <= SPIN_TURN_MAX)
			AutoOC.idle += d;
	}
	else
	{
		AutoOC.spinTarget = target;
		AutoOC.spinA = Registers.A.W;
		AutoOC.spinX = Registers.X.W;
		AutoOC.spinY = Registers.Y.W;
	}

	AutoOC.spinLast = now;
}

void S9xAutoOCWaitBegin (void)
{
	AutoOC.waiting = TRUE;
	AutoOC.waitStart = Now();
}

void S9xAutoOCWaitEnd (void)
{
	AutoOC.waiting = FALSE;
	AutoOC.idle += Since(AutoOC.waitStart);
}

void S9xAutoOCApuWrite (void)
{
	if (++AutoOC.apuWrites == SOUND_BURST)
		Calm(HOLD_SOUND);
}

void S9xAutoOCSample (void)
{
	// dark or fading screen, or no NMI: a screen change or loading
	if (PPU.ForcedBlanking || PPU.Brightness < 15 || !(Memory.FillRAM[0x4200] & 0x80))
		Calm(HOLD_SCREEN);
}

void S9xAutoOCEndFrame (void)
{
	const int32	frame = Timings.V_Max * Timings.H_Max;

	if (AutoOC.waiting)
	{
		AutoOC.idle += Since(AutoOC.waitStart);
		AutoOC.waitStart = Now();
	}

	int32	idle = AutoOC.idle;
	if (idle < 0)
		idle = 0;
	if (idle > frame)
		idle = frame;
	AutoOC.idle = 0;
	AutoOC.apuWrites = 0;

	const int32	busy = 1000 - (int32) ((int64) idle * 1000 / frame);
	AutoOC.busy = busy;

	if (hold > 0)
	{
		hold--;
		lagHistory = 0;
		roomy = 0;
		return;
	}

	const bool8	late = (busy >= LAG_BUSY);
	if (busy <= WAIT_BUSY && waitedFrames < CALIBRATE_FRAMES)
		waitedFrames++;

	lagHistory = (lagHistory << 1) | (late ? 1 : 0);
	if (upWait > 0)
		upWait--;
	if (downWait > 0)
		downWait--;
	if (sinceDown < BOUNCE_FRAMES)
		sinceDown++;

	// late again: one level up
	if (late && waitedFrames >= CALIBRATE_FRAMES && AutoOC.level < AUTOOC_LEVELS - 1 && upWait == 0)
	{
		int32	lateFrames = 0;
		for (uint32 bits = lagHistory & 0xff; bits; bits &= bits - 1)
			lateFrames++;

		if (lateFrames >= 2)
		{
			// gone down too soon: stay up longer from now on
			if (sinceDown < BOUNCE_FRAMES)
				downHold = (downHold * 2 > DOWN_HOLD_MAX) ? DOWN_HOLD_MAX : downHold * 2;
			else
				downHold = DOWN_HOLD;

			SetLevel(AutoOC.level + 1);
			upWait = UP_WAIT;
			downWait = downHold;
			return;
		}
	}

	// enough time left for the level below
	if (AutoOC.level > 0)
	{
		if ((int64) busy * slower[AutoOC.level] <= (int64) DOWN_BUSY * 1000)
			roomy++;
		else
			roomy = 0;

		if (roomy >= DOWN_FRAMES && downWait == 0)
		{
			SetLevel(AutoOC.level - 1);
			sinceDown = 0;
		}
	}
}
