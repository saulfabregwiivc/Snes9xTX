/****************************************************************************
 * Snes9x Nintendo Wii/GameCube Port
 *
 * softdev July 2006
 * crunchy2 May 2007
 * Michniewski 2008
 * Tantric 2008-2022
 * Tanooki 2019-2023
 *
 * video.cpp
 *
 * Video routines
 ***************************************************************************/

#include <gccore.h>
#include <ogcsys.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <math.h>
#include <sys/stat.h>
#include <ogc/texconv.h>
#include <ogc/machine/processor.h>

#include "snes9xtx.h"
#include "menu.h"
#include "filter.h"
#include "filelist.h"
#include "audio.h"
#include "gui/gui.h"
#include "input.h"

#include "snes9x/snes9x.h"
#include "snes9x/memmap.h"
#include "snes9x/gfx.h"
#include "snes9x/ppu.h"
#include "filebrowser.h"
#include "utils/autocrop.h"
#include "utils/bezel.h"
#include "bezel_default_png.h"

extern void UpdatePlaybackRate(void);

/*** Snes9x GFX Buffer ***/
#define SNES9XGFX_SIZE 		(EXT_PITCH*EXT_HEIGHT)
#define FILTERMEM_SIZE 		(512*MAX_SNES_HEIGHT*4)

static unsigned char * snes9xgfx = NULL;
unsigned char * filtermem = NULL; // only want ((512*2) X (239*2))

/*** 2D Video ***/
static u32 *xfb[2] = { NULL, NULL }; // Double buffered
static int whichfb = 0; // Switch
GXRModeObj *vmode = NULL; // Current video mode
int screenheight = 480;
int screenwidth = 640;
static int oldRenderMode = -1; // set to GCSettings.render when changing (temporarily) to another mode
int CheckVideo = 0; // for forcing video reset

/*** GX ***/
#define TEX_WIDTH 512
#define TEX_HEIGHT 478
#define TEXTUREMEM_SIZE 	TEX_WIDTH*(TEX_HEIGHT+8)*2
static unsigned char texturemem[TEXTUREMEM_SIZE] ATTRIBUTE_ALIGN (32);

#define DEFAULT_FIFO_SIZE 256 * 1024
static unsigned int copynow = GX_FALSE;
static unsigned char gp_fifo[DEFAULT_FIFO_SIZE] ATTRIBUTE_ALIGN (32);
static GXTexObj texobj;
static Mtx view;
static Mtx GXmodelView2D;
static int vwidth, vheight, oldvwidth, oldvheight;

u8 * gameScreenPng = NULL;
int gameScreenPngSize = 0;

u32 FrameTimer = 0;

bool vmode_60hz = true;
int timerstyle = 0;
bool progressive = 0;

#define HASPECT 320
#define VASPECT 240

/* New texture based scaler */
typedef struct tagcamera
{
	guVector pos;
	guVector up;
	guVector view;
}
camera;

/*** Square Matrix
     This structure controls the size of the image on the screen.
	 Think of the output as a -80 x 80 by -60 x 60 graph.
***/
static s16 square[] ATTRIBUTE_ALIGN (32) =
{
  /*
   * X,   Y,  Z
   * Values set are for roughly 4:3 aspect
   */
	-HASPECT,  VASPECT, 0,		// 0
	 HASPECT,  VASPECT, 0,	// 1
	 HASPECT, -VASPECT, 0,	// 2
	-HASPECT, -VASPECT, 0	// 3
};

/*** Auto crop of black borders ***/
static autocrop_state autoCrop;
static int autoCropMode = AUTOCROP_OFF; // mode the current crop belongs to
static int autoCropScale = AUTOCROP_SCALE_SMOOTH; // scaling the quad was set for
static bool autoCropDirty = false;      // the quad must be updated
static u8 autoCropRows[MAX_SNES_HEIGHT];
static int quadX = HASPECT, quadY = VASPECT; // half size of the whole frame on screen
static f32 texS0 = 0.0f, texS1 = 1.0f, texT0 = 0.0f, texT1 = 1.0f; // visible part of the texture

/*** Sharp scaling of the auto crop (bilinear filtering off) ***
 Enlarging the crop by a fraction with nearest sampling makes some pixels
 and lines bigger than others, which shows most in 240p. Instead the visible
 part is first enlarged by whole numbers with nearest sampling, in a corner
 of the EFB, then copied to a texture and smoothed to its size on screen:
 the picture stays sharp, only the rows and columns where pixels meet are
 blended. */
#define SHARP_MAX_WIDTH  640 // EFB size
#define SHARP_MAX_HEIGHT 528
static u8 sharpTex[SHARP_MAX_WIDTH * SHARP_MAX_HEIGHT * 2] ATTRIBUTE_ALIGN (32);
static GXTexObj sharpTexObj;
static s16 sharpSquare[12] ATTRIBUTE_ALIGN (32);
static int sharpW = 0, sharpH = 0;       // enlarged size; 0 = draw directly
static int sharpTexW = 0, sharpTexH = 0; // texture size, multiples of 4
static bool sharpTexReady = false;
extern int fscale; // texture scale of the video filter

/*** Bezels (utils/bezel.h) ***
 A PNG from snes9xtx/bezels covers the screen in the default rendering
 (480i/480p, not 240p). The game fills its transparent window and the
 bezel is drawn over it; a PNG without a window is a background, with the
 game in front of it at its usual size. */
#define BEZEL_FOLDER APPFOLDER "/bezels"
#define BEZEL_WIDTH 640    // EFB width with a bezel
#define BEZEL_VI_WIDTH 704 // the whole picture of the TV (4:3 or 16:9)
static bezel_image bezel;
static GXTexObj bezelTexObj;
static char bezelPath[MAXPATHLEN]; // the file tried last
static int bezelHeight = 0;        // and the screen height it was for
static int quadCX = 0, quadCY = 0; // centre of the quad on screen (y up)
static f32 bezelXScale = 1.0f;     // game width with a background bezel / without
static u16 viWidthSet = 0;         // VI width the current mode was set up with
static const int bezelDevices[] = { DEVICE_SD, DEVICE_USB, DEVICE_SD_SLOTA, DEVICE_SD_SLOTB, DEVICE_SD_PORT2 };

static camera cam = {
	{0.0F, 0.0F, 0.0F},
	{0.0F, 0.5F, 0.0F},
	{0.0F, 0.0F, -0.5F}
};

/*** custom Video modes (used to emulate original console video modes) ***/
/* 239 lines progressive (PAL 50Hz) */
static GXRModeObj TV50hz_239p =
{
	VI_TVMODE_PAL_DS,       // viDisplayMode
	512,             // fbWidth
	239,             // efbHeight
	239,             // xfbHeight
	(VI_MAX_WIDTH_PAL - 644)/2,         // viXOrigin
	(VI_MAX_HEIGHT_PAL/2 - 478/2)/2,        // viYOrigin
	644,             // viWidth
	478,             // viHeight
	VI_XFBMODE_SF,   // xFBmode
	GX_FALSE,        // field_rendering
	GX_FALSE,        // aa

	// sample points arranged in increasing Y order
	{
		{6,6},{6,6},{6,6},  // pix 0, 3 sample points, 1/12 units, 4 bits each
		{6,6},{6,6},{6,6},  // pix 1
		{6,6},{6,6},{6,6},  // pix 2
		{6,6},{6,6},{6,6}   // pix 3
	},

	// vertical filter[7], 1/64 units, 6 bits each
	{
		0,         // line n-1
		0,         // line n-1
		21,         // line n
		22,         // line n
		21,         // line n
		0,         // line n+1
		0          // line n+1
	}
};

/* 478 lines interlaced (PAL 50Hz, Deflicker) */
static GXRModeObj TV50hz_478i =
{
	VI_TVMODE_PAL_INT,      // viDisplayMode
	512,             // fbWidth
	478,             // efbHeight
	478,             // xfbHeight
	(VI_MAX_WIDTH_PAL - 644)/2,         // viXOrigin
	(VI_MAX_HEIGHT_PAL - 478)/2,        // viYOrigin
	644,             // viWidth
	478,             // viHeight
	VI_XFBMODE_DF,   // xFBmode
	GX_FALSE,         // field_rendering
	GX_FALSE,        // aa

	// sample points arranged in increasing Y order
	{
		{6,6},{6,6},{6,6},  // pix 0, 3 sample points, 1/12 units, 4 bits each
		{6,6},{6,6},{6,6},  // pix 1
		{6,6},{6,6},{6,6},  // pix 2
		{6,6},{6,6},{6,6}   // pix 3
	},

	// vertical filter[7], 1/64 units, 6 bits each
	{
		8,         // line n-1
		8,         // line n-1
    	10,         // line n
		12,         // line n
		10,         // line n
		8,         // line n+1
		8          // line n+1
	}
};

/* 224 lines progressive (NTSC or PAL 60Hz) */
static GXRModeObj TV60hz_224p =
{
	VI_TVMODE_EURGB60_DS,      // viDisplayMode
	512,             // fbWidth
	224,             // efbHeight
	224,             // xfbHeight
	(VI_MAX_WIDTH_NTSC - 644)/2,        // viXOrigin
	(VI_MAX_HEIGHT_NTSC/2 - 448/2)/2,       // viYOrigin
	644,             // viWidth
	448,             // viHeight
	VI_XFBMODE_SF,   // xFBmode
	GX_FALSE,        // field_rendering
	GX_FALSE,        // aa

	// sample points arranged in increasing Y order
	{
		{6,6},{6,6},{6,6},  // pix 0, 3 sample points, 1/12 units, 4 bits each
		{6,6},{6,6},{6,6},  // pix 1
		{6,6},{6,6},{6,6},  // pix 2
		{6,6},{6,6},{6,6}   // pix 3
	},

	// vertical filter[7], 1/64 units, 6 bits each
	{
		0,         // line n-1
		0,         // line n-1
		21,         // line n
		22,         // line n
		21,         // line n
		0,         // line n+1
		0          // line n+1
	}
};

/* 448 lines interlaced (NTSC or PAL 60Hz, Deflicker) */
static GXRModeObj TV60hz_448i =
{
	VI_TVMODE_EURGB60_INT,     // viDisplayMode
	512,             // fbWidth
	448,             // efbHeight
	448,             // xfbHeight
	(VI_MAX_WIDTH_NTSC - 644)/2,        // viXOrigin
	(VI_MAX_HEIGHT_NTSC - 448)/2,       // viYOrigin
	644,             // viWidth
	448,             // viHeight
	VI_XFBMODE_DF,   // xFBmode
	GX_FALSE,         // field_rendering
	GX_FALSE,        // aa

	// sample points arranged in increasing Y order
	{
		{6,6},{6,6},{6,6},  // pix 0, 3 sample points, 1/12 units, 4 bits each
		{6,6},{6,6},{6,6},  // pix 1
		{6,6},{6,6},{6,6},  // pix 2
		{6,6},{6,6},{6,6}   // pix 3
	},

	// vertical filter[7], 1/64 units, 6 bits each
	{
		8,         // line n-1
		8,         // line n-1
		10,         // line n
		12,         // line n
		10,         // line n
		8,         // line n+1
		8          // line n+1
	}
};

static GXRModeObj TV_Custom;

/* TV Modes table */
static GXRModeObj *tvmodes[4] = {
	&TV60hz_224p, &TV60hz_448i,			/* 60hz video modes */
	&TV50hz_239p, &TV50hz_478i			/* 50hz video modes */
};

/****************************************************************************
 * VideoThreading
 ***************************************************************************/
#define TSTACK 16384
static lwp_t vbthread = LWP_THREAD_NULL;
static unsigned char vbstack[TSTACK];

/****************************************************************************
 * vbgetback
 *
 * This callback enables the emulator to keep running while waiting for a
 * vertical blank.
 *
 * Putting LWP to good use :)
 ***************************************************************************/
static void *
vbgetback (void *arg)
{
	while (1)
	{
		VIDEO_WaitVSync ();	 /**< Wait for video vertical blank */
		LWP_SuspendThread (vbthread);
	}
	return NULL;
}

/****************************************************************************
 * copy_to_xfb
 *
 * Stock code to copy the GX buffer to the current display mode.
 * Also increments the frameticker, as it's called for each vb.
 ***************************************************************************/
static inline void
copy_to_xfb (u32 arg)
{
	if (copynow == GX_TRUE)
	{
		GX_CopyDisp (xfb[whichfb], GX_TRUE);
		GX_Flush ();
		copynow = GX_FALSE;
	}
	++FrameTimer;
}

/****************************************************************************
 * Scaler Support Functions
 ***************************************************************************/
static inline void
draw_init ()
{
	GX_ClearVtxDesc ();
	GX_SetVtxDesc (GX_VA_POS, GX_INDEX8);
	GX_SetVtxDesc (GX_VA_CLR0, GX_INDEX8);
	GX_SetVtxDesc (GX_VA_TEX0, GX_DIRECT);

	GX_SetVtxAttrFmt (GX_VTXFMT0, GX_VA_POS, GX_POS_XYZ, GX_S16, 0);
	GX_SetVtxAttrFmt (GX_VTXFMT0, GX_VA_CLR0, GX_CLR_RGBA, GX_RGBA8, 0);
	GX_SetVtxAttrFmt (GX_VTXFMT0, GX_VA_TEX0, GX_TEX_ST, GX_F32, 0);

	GX_SetArray (GX_VA_POS, square, 3 * sizeof (s16));

	GX_SetNumTexGens (1);
	GX_SetNumChans (0);

	GX_SetTexCoordGen (GX_TEXCOORD0, GX_TG_MTX2x4, GX_TG_TEX0, GX_IDENTITY);

	GX_SetTevOp (GX_TEVSTAGE0, GX_REPLACE);
	GX_SetTevOrder (GX_TEVSTAGE0, GX_TEXCOORD0, GX_TEXMAP0, GX_COLORNULL);

	memset (&view, 0, sizeof (Mtx));
	guLookAt(view, &cam.pos, &cam.up, &cam.view);
	GX_LoadPosMtxImm (view, GX_PNMTX0);

	GX_InvVtxCache ();	// update vertex cache
}

static inline void
draw_vert (u8 pos, u8 c, f32 s, f32 t)
{
	GX_Position1x8 (pos);
	GX_Color1x8 (c);
	GX_TexCoord2f32 (s, t);
}

static inline void
draw_square_uv (Mtx v, f32 s0, f32 t0, f32 s1, f32 t1)
{
	Mtx m;			// model matrix.
	Mtx mv;			// modelview matrix.

	guMtxIdentity (m);
	guMtxTransApply (m, m, 0, 0, -100);
	guMtxConcat (v, m, mv);

	GX_LoadPosMtxImm (mv, GX_PNMTX0);
	GX_Begin (GX_QUADS, GX_VTXFMT0, 4);
	draw_vert (0, 0, s0, t0);
	draw_vert (1, 0, s1, t0);
	draw_vert (2, 0, s1, t1);
	draw_vert (3, 0, s0, t1);
	GX_End ();
}

static inline void
draw_square (Mtx v)
{
	draw_square_uv (v, texS0, texT0, texS1, texT1);
}

/****************************************************************************
 * Bezels
 ***************************************************************************/
static bool bezelShown = false; // drawn in the current video mode

static bool BezelHole()
{
	return bezelShown && bezel.hole_w > 0;
}

// Finds a bezel file on the devices (SD first).
static bool BezelFind(const char *name, char *path)
{
	struct stat st;
	for (unsigned int i = 0; i < sizeof(bezelDevices) / sizeof(bezelDevices[0]); i++)
	{
		snprintf(path, MAXPATHLEN, "%s%s/%s", pathPrefix[bezelDevices[i]], BEZEL_FOLDER, name);
		if (stat(path, &st) == 0 && !S_ISDIR(st.st_mode))
			return true;
	}
	path[0] = 0;
	return false;
}

// The file of the bezel setting, or BEZEL_DEFAULT for the built-in one.
// Auto takes <game>.png, or else default.png, or else the built-in one.
static bool BezelFile(char *path)
{
	path[0] = 0;
	if (GCSettings.Bezel[0] == 0)
		return false;
	if (strcmp(GCSettings.Bezel, BEZEL_DEFAULT) == 0)
	{
		strcpy(path, BEZEL_DEFAULT);
		return true;
	}
	if (strcmp(GCSettings.Bezel, BEZEL_AUTO) != 0)
		return BezelFind(GCSettings.Bezel, path);

	char name[MAXJOLIET + 8];
	if (Memory.ROMFilename[0])
	{
		snprintf(name, sizeof(name), "%s.png", Memory.ROMFilename);
		if (BezelFind(name, path))
			return true;
	}
	if (BezelFind("default.png", path))
		return true;
	strcpy(path, BEZEL_DEFAULT);
	return true;
}

static GXRModeObj * FindVideoMode();

// Before the game runs again (menu -> game, with nothing on screen to keep
// going): loads the bezel of the setting for the default rendering, unless it
// was tried already. In 240p the one loaded is kept for later.
void BezelPrepare()
{
	char path[MAXPATHLEN];
	bezelShown = false; // until ResetVideo_Emu
	if (GCSettings.render != 0)
		return;

	GXRModeObj *rmode = FindVideoMode();
	if (rmode->aa || !BezelFile(path))
	{
		bezel_free(&bezel);
		bezelPath[0] = 0;
		return;
	}
	if (strcmp(path, bezelPath) == 0 && bezelHeight == rmode->efbHeight)
		return; // loaded already (or not a PNG)

	snprintf(bezelPath, sizeof(bezelPath), "%s", path);
	bezelHeight = rmode->efbHeight;
	const bool loaded = (strcmp(path, BEZEL_DEFAULT) == 0) ?
		bezel_load_memory(&bezel, bezel_default_png, bezel_default_png_size, BEZEL_WIDTH, rmode->efbHeight) :
		bezel_load(&bezel, path, BEZEL_WIDTH, rmode->efbHeight);
	if (loaded)
		GX_InitTexObj(&bezelTexObj, bezel.texture, bezel.tex_width, bezel.tex_height, GX_TF_RGBA8, GX_CLAMP, GX_CLAMP, GX_FALSE);
}

// Draws the bezel over the whole screen, blended by its alpha.
static void DrawBezel()
{
	Mtx m, mv;
	const s16 left = -(vmode->fbWidth / 2), right = vmode->fbWidth - vmode->fbWidth / 2;
	const s16 top = vmode->efbHeight / 2, bottom = vmode->efbHeight / 2 - vmode->efbHeight;
	const f32 s1 = (f32) bezel.width / bezel.tex_width, t1 = (f32) bezel.height / bezel.tex_height;

	GX_ClearVtxDesc ();
	GX_SetVtxDesc (GX_VA_POS, GX_DIRECT);
	GX_SetVtxDesc (GX_VA_TEX0, GX_DIRECT);
	// format 2: format 1 is FreeTypeGX's (the menu text), set up only once
	GX_SetVtxAttrFmt (GX_VTXFMT2, GX_VA_POS, GX_POS_XYZ, GX_S16, 0);
	GX_SetVtxAttrFmt (GX_VTXFMT2, GX_VA_TEX0, GX_TEX_ST, GX_F32, 0);
	GX_InvVtxCache ();
	GX_SetBlendMode (GX_BM_BLEND, GX_BL_SRCALPHA, GX_BL_INVSRCALPHA, GX_LO_CLEAR);
	GX_LoadTexObj (&bezelTexObj, GX_TEXMAP0);

	guMtxIdentity (m);
	guMtxTransApply (m, m, 0, 0, -100);
	guMtxConcat (view, m, mv);
	GX_LoadPosMtxImm (mv, GX_PNMTX0);

	GX_Begin (GX_QUADS, GX_VTXFMT2, 4);
	GX_Position3s16 (left, top, 0);
	GX_TexCoord2f32 (0.0f, 0.0f);
	GX_Position3s16 (right, top, 0);
	GX_TexCoord2f32 (s1, 0.0f);
	GX_Position3s16 (right, bottom, 0);
	GX_TexCoord2f32 (s1, t1);
	GX_Position3s16 (left, bottom, 0);
	GX_TexCoord2f32 (0.0f, t1);
	GX_End ();

	GX_SetBlendMode (GX_BM_NONE, GX_BL_SRCALPHA, GX_BL_INVSRCALPHA, GX_LO_CLEAR);
	draw_init (); // back to the game's quad
}

int BezelList(char (*names)[BEZEL_NAME_MAX], int max)
{
	char dir[MAXPATHLEN];
	int count = 0;
	for (unsigned int i = 0; i < sizeof(bezelDevices) / sizeof(bezelDevices[0]); i++)
	{
		snprintf(dir, sizeof(dir), "%s%s", pathPrefix[bezelDevices[i]], BEZEL_FOLDER);
		count = bezel_list(dir, names, count, max);
	}
	return count;
}

/****************************************************************************
 * StopGX
 *
 * Stops GX (when exiting)
 ***************************************************************************/
void StopGX()
{
	GX_AbortFrame();
	GX_Flush();

	VIDEO_SetBlack(TRUE);
	VIDEO_Flush();
}

/****************************************************************************
 * FindVideoMode
 *
 * Finds the optimal video mode, or uses the user-specified one
 * Also configures original video modes
 ***************************************************************************/
static GXRModeObj * FindVideoMode()
{
	GXRModeObj * mode;

	// choose the desired video mode
	switch(GCSettings.videomode)
	{
		case 1: // NTSC (480i)
			mode = &TVNtsc480IntDf;
			break;
		case 2: // Progressive (480p)
			mode = &TVNtsc480Prog;
			break;
		case 3: // Progressive (576p)
			mode = &TVPal576ProgScale;
			break;
		case 4: // PAL (50Hz)
			mode = &TVPal576IntDfScale;
			break;
		case 5: // PAL (60Hz)
			mode = &TVEurgb60Hz480IntDf;
			break;
		default:
			mode = VIDEO_GetPreferredMode(NULL);

			#ifdef HW_DOL
			/* we have component cables, but the preferred mode is interlaced
			 * why don't we switch into progressive?
			 * on the Wii, the user can do this themselves on their Wii Settings */
			if(VIDEO_HaveComponentCable())
				mode = &TVNtsc480Prog;
			else
				mode = &TVPal576ProgScale;
			#endif

			break;
	}

	// configure original modes
	switch (mode->viTVMode >> 2)
	{
		case VI_PAL:
			// 576 lines (PAL 50Hz)
			vmode_60hz = false;

			// Original Video modes (forced to PAL 50Hz)
			// set video signal mode
			TV50hz_239p.viTVMode = VI_TVMODE_PAL_DS;
			TV50hz_478i.viTVMode = VI_TVMODE_PAL_INT;
			TV60hz_224p.viTVMode = VI_TVMODE_PAL_DS;
			TV60hz_448i.viTVMode = VI_TVMODE_PAL_INT;
			// set VI position
			TV50hz_239p.viYOrigin = (VI_MAX_HEIGHT_PAL/2 - 478/2)/2;
			TV50hz_478i.viYOrigin = (VI_MAX_HEIGHT_PAL - 478)/2;
			TV60hz_224p.viYOrigin = (VI_MAX_HEIGHT_PAL/2 - 448/2)/2;
			TV60hz_448i.viYOrigin = (VI_MAX_HEIGHT_PAL - 448)/2;
			break;

		case VI_NTSC:
			// 480 lines (NTSC 60Hz)
			vmode_60hz = true;

			// Original Video modes (forced to NTSC 60hz)
			// set video signal mode
			TV50hz_239p.viTVMode = VI_TVMODE_NTSC_DS;
			TV50hz_478i.viTVMode = VI_TVMODE_NTSC_INT;
			TV60hz_224p.viTVMode = VI_TVMODE_NTSC_DS;
			TV60hz_448i.viTVMode = VI_TVMODE_NTSC_INT;
			// set VI position
			TV50hz_239p.viYOrigin = (VI_MAX_HEIGHT_NTSC/2 - 478/2)/2;
			TV50hz_478i.viYOrigin = (VI_MAX_HEIGHT_NTSC - 478)/2;
			TV60hz_224p.viYOrigin = (VI_MAX_HEIGHT_NTSC/2 - 448/2)/2;
			TV60hz_448i.viYOrigin = (VI_MAX_HEIGHT_NTSC - 448)/2;
			break;

		default:
			// 480 lines (PAL 60Hz)
			vmode_60hz = true;

			// Original Video modes (forced to PAL 60hz)
			// set video signal mode
			TV50hz_239p.viTVMode = VI_TVMODE(mode->viTVMode >> 2, VI_NON_INTERLACE);
			TV50hz_478i.viTVMode = VI_TVMODE(mode->viTVMode >> 2, VI_INTERLACE);
			TV60hz_224p.viTVMode = VI_TVMODE(mode->viTVMode >> 2, VI_NON_INTERLACE);
			TV60hz_448i.viTVMode = VI_TVMODE(mode->viTVMode >> 2, VI_INTERLACE);
			// set VI position
			TV50hz_239p.viYOrigin = (VI_MAX_HEIGHT_NTSC/2 - 478/2)/2;
			TV50hz_478i.viYOrigin = (VI_MAX_HEIGHT_NTSC - 478)/2;
			TV60hz_224p.viYOrigin = (VI_MAX_HEIGHT_NTSC/2 - 448/2)/2;
			TV60hz_448i.viYOrigin = (VI_MAX_HEIGHT_NTSC - 448)/2;
			break;
	}

	// check for progressive scan
	if (mode->viTVMode == VI_TVMODE_NTSC_PROG || VI_TVMODE_PAL_PROG)
		progressive = true;
	else
		progressive = false;

	#ifdef HW_RVL
	if (CONF_GetAspectRatio() == CONF_ASPECT_16_9)
		mode->viWidth = 678;
	else
		mode->viWidth = 672;

	if (vmode_60hz)
	{
		mode->viXOrigin = (VI_MAX_WIDTH_NTSC - mode->viWidth) / 2;
		mode->viYOrigin = (VI_MAX_HEIGHT_NTSC - mode->viHeight) / 2;
	}
	else
	{
		mode->viXOrigin = (VI_MAX_WIDTH_PAL - mode->viWidth) / 2;
		mode->viYOrigin = (VI_MAX_HEIGHT_PAL - mode->viHeight) / 2;
	}
	#endif
	return mode;
}

/****************************************************************************
 * SetupVideoMode
 *
 * Sets up the given video mode
 ***************************************************************************/
static void SetupVideoMode(GXRModeObj * mode)
{
	if(vmode == mode && mode->viWidth == viWidthSet)
		return; // (a bezel changes the width of the same mode)

	VIDEO_SetPostRetraceCallback (NULL);
	copynow = GX_FALSE;
	VIDEO_Configure (mode);
	VIDEO_Flush();
	viWidthSet = mode->viWidth;

	// Clear framebuffers etc.
	VIDEO_ClearFrameBuffer (mode, xfb[0], COLOR_BLACK);
	VIDEO_ClearFrameBuffer (mode, xfb[1], COLOR_BLACK);
	VIDEO_SetNextFramebuffer (xfb[0]);

	VIDEO_SetBlack (FALSE);
	VIDEO_Flush ();
	VIDEO_WaitVSync ();
		
	if (mode->viTVMode & VI_NON_INTERLACE)
		VIDEO_WaitVSync();
	else
		while (VIDEO_GetNextField())
			VIDEO_WaitVSync();
	
	VIDEO_SetPostRetraceCallback ((VIRetraceCallback)copy_to_xfb);
	vmode = mode;
}

/****************************************************************************
 * InitGCVideo
 *
 * This function MUST be called at startup.
 * - also sets up menu video mode
 ***************************************************************************/
void
InitGCVideo ()
{
	VIDEO_Init();

	// Allocate the video buffers
	xfb[0] = (u32 *) memalign(32, 640*576*2);
	xfb[1] = (u32 *) memalign(32, 640*576*2);
	DCInvalidateRange(xfb[0], 640*576*2);
	DCInvalidateRange(xfb[1], 640*576*2);
	xfb[0] = (u32 *) MEM_K0_TO_K1 (xfb[0]);
	xfb[1] = (u32 *) MEM_K0_TO_K1 (xfb[1]);

	GXRModeObj *rmode = FindVideoMode();

    #ifdef HW_RVL
    if (CONF_GetAspectRatio() == CONF_ASPECT_16_9 && (*(u32*)(0xCD8005A0) >> 16) == 0xCAFE) // Wii U
    {
	    write32(0xd8006a0, 0x30000004), mask32(0xd8006a8, 0, 2);
    }
    #endif

	SetupVideoMode(rmode);
	LWP_CreateThread (&vbthread, vbgetback, NULL, vbstack, TSTACK, 68);
	
	// Initialize GX
	GXColor background = { 0, 0, 0, 0xff };
	memset (&gp_fifo, 0, DEFAULT_FIFO_SIZE);
	GX_Init (&gp_fifo, DEFAULT_FIFO_SIZE);
	GX_SetCopyClear (background, 0x00ffffff);
	GX_SetDispCopyGamma (GX_GM_1_0);
	GX_SetCullMode (GX_CULL_NONE);

	vwidth = 100;
	vheight = 100;
}

void ResetFbWidth(int width, GXRModeObj *rmode)
{
	if(rmode->fbWidth == width)
		return;
	
	rmode->fbWidth = width;
	
	if(rmode != vmode)
		return;
	
	GX_InvVtxCache();
	VIDEO_Configure(rmode);
	VIDEO_Flush();
	viWidthSet = rmode->viWidth;
}

/****************************************************************************
 * ResetVideo_Emu
 *
 * Reset the video/rendering mode for the emulator rendering
****************************************************************************/
void
ResetVideo_Emu ()
{
	GXRModeObj *rmode;
	Mtx44 p;
	int i = -1;

	// original or video filter
	if (GCSettings.render == 1)
	{
		for (int j=0; j<4; j++)
		{
			if (tvmodes[j]->efbHeight == vheight)
			{
				i = j;
				break;
			}
		}
	}

	if(i >= 0)
	{
		rmode = tvmodes[i];
		bezelShown = false; // no bezel in 240p

		// hack to fix video output for filters (only when actually filtering; h<=239, w<=256)
		if (GCSettings.VideoFilter != FILTER_NONE && vheight <= 239 && vwidth <= 256)
		{
			memcpy(&TV_Custom, tvmodes[i], sizeof(TV_Custom));
			rmode = &TV_Custom;

			rmode->fbWidth = 512;
			rmode->efbHeight *= 2;
			rmode->xfbHeight *= 2;
			rmode->xfbMode = VI_XFBMODE_DF;
			rmode->viTVMode |= VI_INTERLACE;
		}

		if (Settings.PAL == 1)
			Settings.SoundInputRate = 32090;
		else
			Settings.SoundInputRate = 31894;
		UpdatePlaybackRate();
	}
	else
	{
		rmode = FindVideoMode();

		// the bezel loaded by BezelPrepare, not for a screenshot taken in
		// original rendering (see update_video)
		bezelShown = GCSettings.render == 0 && oldRenderMode == -1 && bezel.texture != NULL &&
			bezel.height == rmode->efbHeight && !rmode->aa;

		const int fbWidth = GCSettings.aspect ? 640 : 512;
		bezelXScale = 1.0f;
		if (bezelShown)
		{
			// the bezel covers the whole picture of the TV; in front of a
			// background the game keeps its width on screen
			bezelXScale = ((f32) rmode->viWidth / fbWidth) * ((f32) BEZEL_WIDTH / BEZEL_VI_WIDTH);
			rmode->viWidth = BEZEL_VI_WIDTH;
			rmode->viXOrigin = (VI_MAX_WIDTH_NTSC - BEZEL_VI_WIDTH) / 2;
			ResetFbWidth(BEZEL_WIDTH, rmode);
		}
		else
		{
			ResetFbWidth(fbWidth, rmode);
		}
		
		Settings.SoundInputRate = 31920;
		UpdatePlaybackRate();
	}

	SetupVideoMode(rmode); // reconfigure VI

	GXColor background = {0, 0, 0, 255};
	GX_SetCopyClear (background, 0x00ffffff);

	GX_SetViewport (0, 0, rmode->fbWidth, rmode->efbHeight, 0, 1);
	GX_SetDispCopyYScale ((f32) rmode->xfbHeight / (f32) rmode->efbHeight);
	GX_SetScissor (0, 0, rmode->fbWidth, rmode->efbHeight);

	GX_SetDispCopySrc (0, 0, rmode->fbWidth, rmode->efbHeight);
	GX_SetDispCopyDst (rmode->fbWidth, rmode->xfbHeight);
	GX_SetCopyFilter(rmode->aa, rmode->sample_pattern, (rmode->xfbMode == VI_XFBMODE_SF) ? GX_FALSE : GX_TRUE, rmode->vfilter);

	GX_SetFieldMode (rmode->field_rendering, ((rmode->viHeight == 2 * rmode->xfbHeight) ? GX_ENABLE : GX_DISABLE));

	if (rmode->aa)
		GX_SetPixelFmt(GX_PF_RGB565_Z16, GX_ZC_LINEAR);
	else
		GX_SetPixelFmt(GX_PF_RGB8_Z24, GX_ZC_LINEAR);

	GX_SetZMode (GX_TRUE, GX_LEQUAL, GX_TRUE);
	GX_SetColorUpdate (GX_TRUE);

	guOrtho(p, rmode->efbHeight/2, -(rmode->efbHeight/2), -(rmode->fbWidth/2), rmode->fbWidth/2, 100, 1000);	// matrix, t, b, l, r, n, f
	GX_LoadProjectionMtx (p, GX_ORTHOGRAPHIC);

	draw_init ();
}

/****************************************************************************
 * MakeTexture
 *
 * Modified for a buffer with an offset (border)
 ***************************************************************************/
void
MakeTexture (const void *src, void *dst, s32 width, s32 height)
{
	register u32 tmp0=0,tmp1=0,tmp2=0,tmp3=0;

	__asm__ __volatile__ (
		"	srwi		%6,%6,2\n"
		"	srwi		%7,%7,2\n"
		"	subi		%3,%4,4\n"
		"	mr			%4,%3\n"
		"	subi		%4,%4,4\n"

		"2: mtctr		%6\n"
		"	mr			%0,%5\n"
		//
		"1: lwz			%1,0(%5)\n"			//1
		"	stwu		%1,8(%4)\n"
		"	lwz			%2,4(%5)\n"			//1
		"	stwu		%2,8(%3)\n"
		"	lwz			%1,1032(%5)\n"		//2
		"	stwu		%1,8(%4)\n"
		"	lwz			%2,1036(%5)\n"		//2
		"	stwu		%2,8(%3)\n"
		"	lwz			%1,2064(%5)\n"		//3
		"	stwu		%1,8(%4)\n"
		"	lwz			%2,2068(%5)\n"		//3
		"	stwu		%2,8(%3)\n"
		"	lwz			%1,3096(%5)\n"		//4
		"	stwu		%1,8(%4)\n"
		"	lwz			%2,3100(%5)\n"		//4
		"	stwu		%2,8(%3)\n"
		"	addi		%5,%5,8\n"
		"	bdnz		1b\n"
		"	addi		%5,%0,4128\n"		//5
		"	subic.		%7,%7,1\n"
		"	bne			2b"
		//		0			 1			  2			   3		   4		  5		    6		    7
		: "=&b"(tmp0), "=&b"(tmp1), "=&b"(tmp2), "=&b"(tmp3), "+b"(dst) : "b"(src), "b"(width), "b"(height)
	);
}

/****************************************************************************
 * Auto crop
 *
 * Same rules as the Auto Crop Black Borders option of the snes9x-ntsc-lie-pal
 * Libretro core, see utils/autocrop.h. The crop is drawn by showing part of
 * the texture on the quad: Stretch keeps the quad, Fit resizes it evenly.
 ***************************************************************************/
static int AutoCropLines()
{
	return vheight > SNES_HEIGHT_EXTENDED ? vheight / 2 : vheight;
}

static void AutoCropView(f32 *u0, f32 *u1, f32 *v0, f32 *v1, f32 *xsize, f32 *ysize)
{
	f32 xroom = 1.0f, yroom = 1.0f;
	if (vmode && quadX > 0 && quadY > 0 && !BezelHole()) // the window of a bezel is all the room
	{
		xroom = (f32) vmode->fbWidth / (2 * quadX);
		yroom = (f32) vmode->efbHeight / (2 * quadY);
	}
	autocrop_view(&autoCrop, autoCropMode, vwidth, AutoCropLines(), xroom, yroom, u0, u1, v0, v1, xsize, ysize);
}

// Chooses the sharp scaling for a crop drawn at width x height EFB pixels.
static void UpdateSharp(int width, int height)
{
	sharpW = sharpH = 0;

	if (GCSettings.bilinear || !vmode || vmode->aa)
		return;
	if (!bezelShown && (autoCropMode == AUTOCROP_OFF ||
		(texS0 == 0.0f && texS1 == 1.0f && texT0 == 0.0f && texT1 == 1.0f)))
		return; // nothing cropped: the original scaling (with a bezel it is sharp too)

	// visible part in texels (whole texels: the crop edges are whole pixels)
	const int cw = (int) ((texS1 - texS0) * vwidth * fscale + 0.5f);
	const int ch = (int) ((texT1 - texT0) * vheight * fscale + 0.5f);
	if (cw <= 0 || ch <= 0)
		return;

	const f32 sx = (f32) width / cw, sy = (f32) height / ch;
	const bool xwhole = fabsf(sx - (int) (sx + 0.5f)) < 0.01f;
	const bool ywhole = fabsf(sy - (int) (sy + 0.5f)) < 0.01f;
	if (xwhole && ywhole)
		return; // whole-number scaling is already even

	// enlarge by the next whole number, as long as it fits the EFB
	int nx = (int) ceilf(sx - 0.01f), ny = (int) ceilf(sy - 0.01f);
	if (nx < 1) nx = 1;
	if (ny < 1) ny = 1;
	while (nx > 1 && cw * nx > SHARP_MAX_WIDTH) nx--;
	while (ny > 1 && ch * ny > SHARP_MAX_HEIGHT) ny--;
	if (nx == 1 && ny == 1)
		return;
	if (cw * nx > SHARP_MAX_WIDTH || ch * ny > SHARP_MAX_HEIGHT)
		return;

	sharpW = cw * nx;
	sharpH = ch * ny;
	sharpTexW = (sharpW + 3) & ~3;
	sharpTexH = (sharpH + 3) & ~3;

	// the quad covers the viewport exactly (see DrawSharp)
	sharpSquare[0] = sharpSquare[9]  = -(sharpW / 2);
	sharpSquare[6] = sharpSquare[3]  = sharpW - sharpW / 2;
	sharpSquare[4] = sharpSquare[1]  = sharpH / 2;
	sharpSquare[7] = sharpSquare[10] = sharpH / 2 - sharpH;
	sharpSquare[2] = sharpSquare[5] = sharpSquare[8] = sharpSquare[11] = 0;
	DCFlushRange (sharpSquare, sizeof(sharpSquare));
	GX_InvVtxCache ();

	if (!sharpTexReady)
	{
		// nothing of it may stay in the CPU cache once the GPU writes it
		DCFlushRange (sharpTex, sizeof(sharpTex));
		sharpTexReady = true;
	}
	GX_InitTexObj (&sharpTexObj, sharpTex, sharpTexW, sharpTexH, GX_TF_RGB565, GX_CLAMP, GX_CLAMP, GX_FALSE);
}

// Draws the crop enlarged by whole numbers (nearest), then smoothed to the
// quad (DrawSharpFinal). See UpdateSharp.
static void DrawSharpEnlarge()
{
	Mtx44 p;

	// 1. enlarge, nearest sampling, into the top left corner of the EFB
	GX_SetViewport (0, 0, sharpW, sharpH, 0, 1);
	GX_SetScissor (0, 0, sharpW, sharpH);
	guOrtho (p, sharpSquare[1], sharpSquare[7], sharpSquare[0], sharpSquare[3], 100, 1000);
	GX_LoadProjectionMtx (p, GX_ORTHOGRAPHIC);
	GX_SetArray (GX_VA_POS, sharpSquare, 3 * sizeof (s16));
	GX_LoadTexObj (&texobj, GX_TEXMAP0);
	draw_square (view);

	// copy it to a texture, clearing that part of the EFB
	GX_SetCopyFilter (GX_FALSE, NULL, GX_FALSE, NULL);
	GX_SetTexCopySrc (0, 0, sharpTexW, sharpTexH);
	GX_SetTexCopyDst (sharpTexW, sharpTexH, GX_TF_RGB565, GX_FALSE);
	GX_CopyTex (sharpTex, GX_TRUE);
	GX_PixModeSync ();
	GX_InvalidateTexAll ();

	// back to the whole screen
	GX_SetCopyFilter (vmode->aa, vmode->sample_pattern, (vmode->xfbMode == VI_XFBMODE_SF) ? GX_FALSE : GX_TRUE, vmode->vfilter);
	GX_SetViewport (0, 0, vmode->fbWidth, vmode->efbHeight, 0, 1);
	GX_SetScissor (0, 0, vmode->fbWidth, vmode->efbHeight);
	guOrtho (p, vmode->efbHeight / 2, -(vmode->efbHeight / 2), -(vmode->fbWidth / 2), vmode->fbWidth / 2, 100, 1000);
	GX_LoadProjectionMtx (p, GX_ORTHOGRAPHIC);
}

static void DrawSharpFinal()
{
	// 2. smoothed (bilinear) to the size of the quad
	GX_SetArray (GX_VA_POS, square, 3 * sizeof (s16));
	GX_LoadTexObj (&sharpTexObj, GX_TEXMAP0);
	draw_square_uv (view, 0.0f, 0.0f, (f32) sharpW / sharpTexW, (f32) sharpH / sharpTexH);
}

// Sets the quad and texture coordinates for the whole frame (quadX x quadY)
// or its auto crop.
static void UpdateQuad()
{
	f32 u0, u1, v0, v1, xsize, ysize;
	AutoCropView(&u0, &u1, &v0, &v1, &xsize, &ysize);

	const int x = (int) (quadX * xsize + 0.5f);
	const int y = (int) (quadY * ysize + 0.5f);
	int left = -x, right = x, top = y, bottom = -y; // y up

	const bool cropped = !(u0 == 0.0f && u1 == 1.0f && v0 == 0.0f && v1 == 1.0f);
	autoCropScale = GCSettings.AutoCropScale;
	if (cropped && autoCropScale == AUTOCROP_SCALE_INTEGER)
	{
		// whole-number scaling: every texel covers the same number of pixels
		const int cw = (int) ((u1 - u0) * vwidth * fscale + 0.5f);
		const int ch = (int) ((v1 - v0) * vheight * fscale + 0.5f);
		if (cw > 0 && ch > 0)
		{
			int nx = (int) ((f32) (2 * x) / cw + 0.01f);
			int ny = (int) ((f32) (2 * y) / ch + 0.01f);
			if (nx < 1) nx = 1;
			if (ny < 1) ny = 1;
			const int w = cw * nx, h = ch * ny;
			left = -(w / 2);
			right = left + w;
			top = h / 2;
			bottom = top - h;
		}
	}

	square[6] = square[3]  = right + quadCX;
	square[0] = square[9]  = left + quadCX;
	square[4] = square[1]  = top + quadCY;
	square[7] = square[10] = bottom + quadCY;
	DCFlushRange (square, 32); // update memory BEFORE the GPU accesses it!
	GX_InvVtxCache ();

	texS0 = u0;
	texS1 = u1;
	texT0 = v0;
	texT1 = v1;
	autoCropDirty = false;

	if (cropped && autoCropScale == AUTOCROP_SCALE_INTEGER)
		sharpW = sharpH = 0; // already even
	else
		UpdateSharp(right - left, top - bottom);
}

// Measures the frame in GFX.Screen. Returns true when the crop changed.
static bool AutoCropMeasure()
{
	const int vscale = vheight > SNES_HEIGHT_EXTENDED ? 2 : 1;
	for (int row = 0; row < vheight && row < MAX_SNES_HEIGHT; row++)
	{
		const int line = row / vscale;
		// Rows below the emulated picture count as display-off lines.
		autoCropRows[row] = (line >= PPU.ScreenHeight || line >= SNES_HEIGHT_EXTENDED) ? 1 : GFX.LineBlank[line];
	}

	autocrop_frame frame;
	frame.pixels = GFX.Screen;
	frame.pitch = GFX.Pitch >> 1;
	frame.tiled = 0;
	frame.width = vwidth;
	frame.height = vheight < MAX_SNES_HEIGHT ? vheight : MAX_SNES_HEIGHT;
	frame.vscale = vscale;
	frame.hardware_rows = autoCropRows;
	return autocrop_process(&autoCrop, &frame) != 0;
}

// The learned layouts are kept next to the game's SRAM, as <game>.autocrop.
static bool AutoCropPath(char *path)
{
	if (SNESROMSize == 0 || !MakeFilePath(path, FILE_SRAM, Memory.ROMFilename, -1))
		return false;
	char *ext = strrchr(path, '.');
	if (!ext || (ext - path) + sizeof(".autocrop") > MAXPATHLEN)
		return false;
	strcpy(ext, ".autocrop");
	return true;
}

void AutoCropLoadGame()
{
	char path[MAXPATHLEN];
	autocrop_reset(&autoCrop);
	autoCropMode = GCSettings.AutoCrop;
	autoCropDirty = true;
	if (AutoCropPath(path))
		autocrop_load_layouts(&autoCrop, path);
}

void AutoCropSaveGame()
{
	char path[MAXPATHLEN];
	if (autoCrop.layouts_changed && AutoCropPath(path))
		autocrop_save_layouts(&autoCrop, path);
}

void AutoCropReset()
{
	autocrop_reset_tracking(&autoCrop);
	autoCropDirty = true;
}

void AutoCropMapPointer(float fx, float fy, int *x, int *y)
{
	f32 u0, u1, v0, v1, xsize, ysize;
	AutoCropView(&u0, &u1, &v0, &v1, &xsize, &ysize);
	if (!vmode || (!BezelHole() && u0 == 0.0f && u1 == 1.0f && v0 == 0.0f && v1 == 1.0f))
	{
		// Nothing cropped: the whole screen is the SNES picture, as before.
		*x = (int) (fx * 256);
		*y = (int) (fy * 224);
		return;
	}

	// Screen position on the quad as drawn, then in the visible texture.
	const f32 ex = (fx - 0.5f) * vmode->fbWidth;
	const f32 ey = (0.5f - fy) * vmode->efbHeight;
	const f32 left = square[0], top = square[1];
	const f32 width = square[3] - square[0], height = square[1] - square[7];
	const f32 qx = width > 0 ? (ex - left) / width : fx;
	const f32 qy = height > 0 ? (top - ey) / height : fy;
	*x = (int) ((u0 + qx * (u1 - u0)) * 256);
	*y = (int) ((v0 + qy * (v1 - v0)) * AutoCropLines());
}

/****************************************************************************
 * Update Video
 ***************************************************************************/
uint32 prevRenderedFrameCount = 0;
int fscale = 1;

void
update_video (int width, int height)
{
	vwidth = width;
	vheight = height;

	if(CheckVideo == 2 && IPPU.RenderedFramesCount == prevRenderedFrameCount)
		return; // we haven't rendered any frames yet, so we can't draw anything!

	// Ensure previous vb has complete
	while ((LWP_ThreadIsSuspended (vbthread) == 0) || (copynow == GX_TRUE))
		usleep (50);

	whichfb ^= 1;

	if (oldvheight != vheight || oldvwidth != vwidth)	 // if rendered width/height changes, update scaling
		CheckVideo = 1;

	if (CheckVideo)	// if we get back from the menu, and have rendered at least 1 frame
	{
		int xscale, yscale;
#ifdef HW_RVL
		if(vwidth <= 256)
			fscale = GetFilterScale((RenderFilter)GCSettings.VideoFilter);
		else
			fscale = 1;
#endif
		ResetVideo_Emu ();	// reset video to emulator rendering settings
#ifdef HW_RVL
		memset(filtermem, 0, FILTERMEM_SIZE);
#endif
		/** Update scaling **/
		if (GCSettings.render == 1)	// original rendering mode
		{
			if (GCSettings.VideoFilter != FILTER_NONE && vheight <= 239 && vwidth <= 256)
			{	// filters; normal operation
				xscale = vwidth;
				yscale = vheight;
			}
			else
			{	// no filtering
				fscale = 1;
				xscale = 256;
				yscale = vheight / 2;
			}
		}
		else // default rendering mode
		{
			xscale = 256;

			if(vheight == 224 || vheight == 448)
				yscale = 224;
			else
				yscale = 239;
		}

		if (GCSettings.aspect)
		{
			if(GCSettings.render == 1)
				xscale = (3*xscale)/4;
			else
				xscale = 256; // match the original console's width for 16:9 to prevent flickering
		}

		xscale *= GCSettings.zoomHor;
		yscale *= GCSettings.zoomVert;

		if (BezelHole())
		{
			// the window of the bezel, whatever the zoom and position
			quadX = (bezel.hole_w + 1) / 2;
			quadY = (bezel.hole_h + 1) / 2;
			quadCX = bezel.hole_x + quadX - vmode->fbWidth / 2;
			quadCY = vmode->efbHeight / 2 - bezel.hole_y - quadY;
		}
		else
		{
			quadX = (int) (xscale * bezelXScale + 0.5f);
			quadY = yscale;
			quadCX = GCSettings.xshift;
			quadCY = -GCSettings.yshift;
		}
		UpdateQuad(); // the whole frame, or its auto crop
    	draw_init ();

		// initialize the texture obj we are going to use
		GX_InitTexObj (&texobj, texturemem, vwidth*fscale, vheight*fscale, GX_TF_RGB565, GX_CLAMP, GX_CLAMP, GX_FALSE);

		if (GCSettings.bilinear == 0)
			GX_InitTexObjLOD(&texobj,GX_NEAR,GX_NEAR_MIP_NEAR,2.5,9.0,0.0,GX_FALSE,GX_FALSE,GX_ANISO_1); // bilinear filtering

		GX_LoadTexObj (&texobj, GX_TEXMAP0);	// load texture object so its ready to use

		oldvwidth = vwidth;
		oldvheight = vheight;
		CheckVideo = 0;
	}

	if (GCSettings.AutoCrop != autoCropMode)
	{
		autoCropMode = GCSettings.AutoCrop;
		autocrop_reset_tracking(&autoCrop);
		autoCropDirty = true;
	}
	if (GCSettings.AutoCropScale != autoCropScale)
		autoCropDirty = true;
	if (autoCropMode != AUTOCROP_OFF && AutoCropMeasure())
		autoCropDirty = true;
	if (autoCropDirty)
		UpdateQuad();
#ifdef HW_RVL
	// convert image to texture
	if (GCSettings.VideoFilter != FILTER_NONE && vheight <= 239 && vwidth <= 256) // don't do filtering on game textures > 256 x 239
	{
		FilterMethod ((uint8*) GFX.Screen, EXT_PITCH, (uint8*) filtermem, vwidth*fscale*2, vwidth, vheight);
		MakeTexture565((char *) filtermem, (char *) texturemem, vwidth*fscale, vheight*fscale);
	}
	else
#endif
	{
		MakeTexture((char *) GFX.Screen, (char *) texturemem, vwidth, vheight);
	}

	DCFlushRange (texturemem, TEXTUREMEM_SIZE);	// update the texture memory
	GX_InvalidateTexAll ();

	// the enlarged picture uses a corner of the EFB first, so it comes before
	// a bezel behind the game
	if (sharpW > 0)
		DrawSharpEnlarge ();
	if (bezelShown && !BezelHole())
		DrawBezel ();	// background
	if (sharpW > 0)
	{
		DrawSharpFinal ();	// auto crop or bezel window, enlarged sharply
	}
	else
	{
		GX_LoadTexObj (&texobj, GX_TEXMAP0);
		draw_square (view);		// draw the quad
	}
	if (BezelHole())
		DrawBezel ();	// around the game, over its edges

	GX_DrawDone ();

	if(ScreenshotRequested)
	{
		if(GCSettings.render == 1) // we can't take a screenshot in original rendering mode
		{
			oldRenderMode = 1;
			GCSettings.render = 0; // switch to default rendering mode
			CheckVideo = 1; // request the switch
		}
		else
		{
			ScreenshotRequested = 0;
			TakeScreenshot();
			if(oldRenderMode != -1)
			{
				GCSettings.render = oldRenderMode;
				oldRenderMode = -1;
			}
			ConfigRequested = 1;
		}
	}

	VIDEO_SetNextFramebuffer (xfb[whichfb]);
	VIDEO_Flush ();
	copynow = GX_TRUE;

	// Return to caller, don't waste time waiting for vb
	LWP_ResumeThread (vbthread);
}

void AllocGfxMem()
{
	snes9xgfx = (unsigned char *)memalign(32, SNES9XGFX_SIZE);
	memset(snes9xgfx, 0, SNES9XGFX_SIZE);

#ifdef HW_RVL
	filtermem = (unsigned char *)memalign(32, FILTERMEM_SIZE);
	memset(filtermem, 0, FILTERMEM_SIZE);
#endif

	GFX.Pitch = EXT_PITCH;
	GFX.Screen = (uint16*)(snes9xgfx + EXT_OFFSET);
}

/****************************************************************************
 * setGFX
 *
 * Setup the global GFX information for Snes9x
 ***************************************************************************/
void
setGFX ()
{
	GFX.Pitch = EXT_PITCH;
}

/****************************************************************************
 * TakeScreenshot
 *
 * Copies the current screen into a GX texture
 ***************************************************************************/
void TakeScreenshot()
{
	IMGCTX pngContext = PNGU_SelectImageFromBuffer(savebuffer);

	if (pngContext != NULL)
	{
		gameScreenPngSize = PNGU_EncodeFromEFB(pngContext, vmode->fbWidth, vmode->efbHeight);
		PNGU_ReleaseImageContext(pngContext);
		gameScreenPng = (u8 *)malloc(gameScreenPngSize);
		memcpy(gameScreenPng, savebuffer, gameScreenPngSize);
	}
}

void ClearScreenshot()
{
	if(gameScreenPng)
	{
		gameScreenPngSize = 0;
		free(gameScreenPng);
		gameScreenPng = NULL;
	}
}

/****************************************************************************
 * ResetVideo_Menu
 *
 * Reset the video/rendering mode for the menu
****************************************************************************/
void
ResetVideo_Menu ()
{
	Mtx44 p;
	f32 yscale;
	u32 xfbHeight;
	GXRModeObj * rmode = FindVideoMode();

	SetupVideoMode(rmode); // reconfigure VI

	// clears the bg to color and clears the z buffer
	GXColor background = {0, 0, 0, 255};
	GX_SetCopyClear (background, 0x00ffffff);

	yscale = GX_GetYScaleFactor(vmode->efbHeight,vmode->xfbHeight);
	xfbHeight = GX_SetDispCopyYScale(yscale);
	GX_SetScissor(0,0,vmode->fbWidth,vmode->efbHeight);
	GX_SetDispCopySrc(0,0,vmode->fbWidth,vmode->efbHeight);
	GX_SetDispCopyDst(vmode->fbWidth,xfbHeight);
	GX_SetCopyFilter(vmode->aa,vmode->sample_pattern,GX_TRUE,vmode->vfilter);
	GX_SetFieldMode(vmode->field_rendering,((vmode->viHeight==2*vmode->xfbHeight)?GX_ENABLE:GX_DISABLE));

	if (vmode->aa)
		GX_SetPixelFmt(GX_PF_RGB565_Z16, GX_ZC_LINEAR);
	else
		GX_SetPixelFmt(GX_PF_RGB8_Z24, GX_ZC_LINEAR);

	// setup the vertex descriptor
	// tells the flipper to expect direct data
	GX_ClearVtxDesc();
	GX_InvVtxCache ();
	GX_InvalidateTexAll();

	GX_SetVtxDesc(GX_VA_TEX0, GX_NONE);
	GX_SetVtxDesc(GX_VA_POS, GX_DIRECT);
	GX_SetVtxDesc (GX_VA_CLR0, GX_DIRECT);

	GX_SetVtxAttrFmt (GX_VTXFMT0, GX_VA_POS, GX_POS_XYZ, GX_F32, 0);
	GX_SetVtxAttrFmt (GX_VTXFMT0, GX_VA_CLR0, GX_CLR_RGBA, GX_RGBA8, 0);
	GX_SetVtxAttrFmt(GX_VTXFMT0, GX_VA_TEX0, GX_TEX_ST, GX_F32, 0);
	GX_SetZMode (GX_FALSE, GX_LEQUAL, GX_TRUE);

	GX_SetNumChans(1);
	GX_SetNumTexGens(1);
	GX_SetTevOp (GX_TEVSTAGE0, GX_PASSCLR);
	GX_SetTevOrder(GX_TEVSTAGE0, GX_TEXCOORD0, GX_TEXMAP0, GX_COLOR0A0);
	GX_SetTexCoordGen(GX_TEXCOORD0, GX_TG_MTX2x4, GX_TG_TEX0, GX_IDENTITY);

	guMtxIdentity(GXmodelView2D);
	guMtxTransApply (GXmodelView2D, GXmodelView2D, 0.0F, 0.0F, -50.0F);
	GX_LoadPosMtxImm(GXmodelView2D,GX_PNMTX0);

	guOrtho(p,0,479,0,639,0,300);
	GX_LoadProjectionMtx(p, GX_ORTHOGRAPHIC);

	GX_SetViewport(0,0,vmode->fbWidth,vmode->efbHeight,0,1);
	GX_SetBlendMode(GX_BM_BLEND, GX_BL_SRCALPHA, GX_BL_INVSRCALPHA, GX_LO_CLEAR);
	GX_SetAlphaUpdate(GX_TRUE);
}

/****************************************************************************
 * Menu_Render
 *
 * Renders everything current sent to GX, and flushes video
 ***************************************************************************/
void Menu_Render()
{
	whichfb ^= 1; // flip framebuffer
	GX_SetZMode(GX_TRUE, GX_LEQUAL, GX_TRUE);
	GX_SetColorUpdate(GX_TRUE);
	GX_CopyDisp(xfb[whichfb],GX_TRUE);
	GX_DrawDone();
	VIDEO_SetNextFramebuffer(xfb[whichfb]);
	VIDEO_Flush();
	VIDEO_WaitVSync();
}

/****************************************************************************
 * Menu_DrawImg
 *
 * Draws the specified image on screen using GX
 ***************************************************************************/
void Menu_DrawImg(f32 xpos, f32 ypos, u16 width, u16 height, u8 data[],
	f32 degrees, f32 scaleX, f32 scaleY, u8 alpha)
{
	if(data == NULL)
		return;

	GXTexObj texObj;

	GX_InitTexObj(&texObj, data, width,height, GX_TF_RGBA8,GX_CLAMP, GX_CLAMP,GX_FALSE);
	GX_LoadTexObj(&texObj, GX_TEXMAP0);
	GX_InvalidateTexAll();

	GX_SetTevOp (GX_TEVSTAGE0, GX_MODULATE);
	GX_SetVtxDesc (GX_VA_TEX0, GX_DIRECT);

	Mtx m,m1,m2, mv;
	width  >>= 1;
	height >>= 1;

	guMtxIdentity (m1);
	guMtxScaleApply(m1,m1,scaleX,scaleY,1.0);
	guVector axis = (guVector) {0 , 0, 1 };
	guMtxRotAxisDeg (m2, &axis, degrees);
	guMtxConcat(m2,m1,m);

	guMtxTransApply(m,m, xpos+width,ypos+height,0);
	guMtxConcat (GXmodelView2D, m, mv);
	GX_LoadPosMtxImm (mv, GX_PNMTX0);

	GX_Begin(GX_QUADS, GX_VTXFMT0,4);
	GX_Position3f32(-width, -height,  0);
	GX_Color4u8(0xFF,0xFF,0xFF,alpha);
	GX_TexCoord2f32(0, 0);

	GX_Position3f32(width, -height,  0);
	GX_Color4u8(0xFF,0xFF,0xFF,alpha);
	GX_TexCoord2f32(1, 0);

	GX_Position3f32(width, height,  0);
	GX_Color4u8(0xFF,0xFF,0xFF,alpha);
	GX_TexCoord2f32(1, 1);

	GX_Position3f32(-width, height,  0);
	GX_Color4u8(0xFF,0xFF,0xFF,alpha);
	GX_TexCoord2f32(0, 1);
	GX_End();
	GX_LoadPosMtxImm (GXmodelView2D, GX_PNMTX0);

	GX_SetTevOp (GX_TEVSTAGE0, GX_PASSCLR);
	GX_SetVtxDesc (GX_VA_TEX0, GX_NONE);
}

/****************************************************************************
 * Menu_DrawRectangle
 *
 * Draws a rectangle at the specified coordinates using GX
 ***************************************************************************/
void Menu_DrawRectangle(f32 x, f32 y, f32 width, f32 height, GXColor color, u8 filled)
{
	long n = 4;
	f32 x2 = x+width;
	f32 y2 = y+height;
	guVector v[] = {{x,y,0.0f}, {x2,y,0.0f}, {x2,y2,0.0f}, {x,y2,0.0f}, {x,y,0.0f}};
	u8 fmt = GX_TRIANGLEFAN;

	if(!filled)
	{
		fmt = GX_LINESTRIP;
		n = 5;
	}

	GX_Begin(fmt, GX_VTXFMT0, n);
	for(long i=0; i<n; ++i)
	{
		GX_Position3f32(v[i].x, v[i].y,  v[i].z);
		GX_Color4u8(color.r, color.g, color.b, color.a);
	}
	GX_End();
}
