/****************************************************************************
 * Snes9x Nintendo Wii/GameCube Port
 *
 * Michniewski 2008
 * Tanooki 2019-2023
 *
 * Scale2x filter
 * (c) Copyright 2001         Andrea Mazzoleni (amadvance@gmail.com)
 *
 * Adapted from AdvanceMAME
 * Video Filter Code (scale2x)
 * http://www.scale2x.it/
 *
 * filter.cpp
 *
 * Video filtering
 ****************************************************************************/

#include <gccore.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ogcsys.h>
#include <unistd.h>
#include <malloc.h>

#include "filter.h"
#include "video.h"
#include "menu.h"
#include "snes9xtx.h"
#include "snes9x/memmap.h"

#define	Mask_2	0x07E0	// 00000 111111 00000
#define	Mask13	0xF81F	// 11111 000000 11111

TFilterMethod FilterMethod;

template<int GuiScale> void RenderTVMode (uint8 *srcPtr, uint32 srcPitch, uint8 *dstPtr, uint32 dstPitch, int width, int height);
template<int GuiScale> void RenderScale2X (uint8 *srcPtr, uint32 srcPitch, uint8 *dstPtr, uint32 dstPitch, int width, int height);

const char* GetFilterName (RenderFilter filterID)
{
	switch(filterID)
	{
		default: return "Unknown";
		case FILTER_NONE: return "None";
		case FILTER_TVMODE: return "TV Mode";
		case FILTER_SCALE2X: return "Scale2x";
	}
}

// Return pointer to appropriate function
static TFilterMethod FilterToMethod (RenderFilter filterID)
{
	switch(filterID)
	{
		case FILTER_TVMODE:   return RenderTVMode<FILTER_TVMODE>;
		case FILTER_SCALE2X:  return RenderScale2X<FILTER_SCALE2X>;
		default: return 0;
	}
}

int GetFilterScale(RenderFilter filterID)
{
	switch(filterID)
	{
		case FILTER_NONE:
			return 1;
		default:
		case FILTER_TVMODE:
		case FILTER_SCALE2X:
			return 2;
	}
}

void SelectFilterMethod ()
{
	FilterMethod = FilterToMethod((RenderFilter)GCSettings.VideoFilter);
}

template<int GuiScale>
void RenderTVMode (uint8 *srcPtr, uint32 srcPitch, uint8 *dstPtr, uint32 dstPitch, int width, int height)
{
	unsigned int nextlineSrc = srcPitch / sizeof(uint16);
	uint16 *p = (uint16 *)srcPtr;

	unsigned int nextlineDst = dstPitch / sizeof(uint16);
	uint16 *q = (uint16 *)dstPtr;

	while(height--) {
		for (int i = 0, j = 0; i < width; ++i, j += 2) {
			uint16 p1 = *(p + i);
			uint32 pi;

			pi = (((p1 & Mask_2) * 6) >> 3) & Mask_2;
			pi |= (((p1 & Mask13) * 6) >> 3) & Mask13;

			*(q + j) = p1;
			*(q + j + 1) = p1;
			*(q + j + nextlineDst) = (uint16)pi;
			*(q + j + nextlineDst + 1) = (uint16)pi;
		}
		p += nextlineSrc;
		q += nextlineDst << 1;
	}
}

template<int GuiScale>
void RenderScale2X (uint8 *srcPtr, uint32 srcPitch, uint8 *dstPtr, uint32 dstPitch, int width, int height)
{
	unsigned int nextlineSrc = srcPitch / sizeof(uint16);
	uint16 *p = (uint16 *)srcPtr;

	unsigned int nextlineDst = dstPitch / sizeof(uint16);
	uint16 *q = (uint16 *)dstPtr;

	while (height--) {
		for (int i = 0; i < width; ++i) {
			uint16 B = *(p + i - nextlineSrc);
			uint16 D = *(p + i - 1);
			uint16 E = *(p + i);
			uint16 F = *(p + i + 1);
			uint16 H = *(p + i + nextlineSrc);

			*(q + (i << 1)) = D == B && B != F && D != H ? D : E;
			*(q + (i << 1) + 1) = B == F && B != D && F != H ? F : E;
			*(q + (i << 1) + nextlineDst) = D == H && D != B && H != F ? D : E;
			*(q + (i << 1) + nextlineDst + 1) = H == F && D != H && B != F ? F : E;
		}
		p += nextlineSrc;
		q += nextlineDst << 1;
	}
}
