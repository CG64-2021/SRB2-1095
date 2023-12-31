// Emacs style mode select   -*- C++ -*- 
//-----------------------------------------------------------------------------
//
// Copyright (C) 1998-2000 by DooM Legacy Team.
//
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License
// as published by the Free Software Foundation; either version 2
// of the License, or (at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
//-----------------------------------------------------------------------------
/// \file
/// \brief globals (shared data & code) for hw_ modules

#ifndef _HWR_GLOB_H_
#define _HWR_GLOB_H_

#include "hw_defs.h"
#include "hw_main.h"

// the original aspect ratio of Doom graphics isn't square
#define ORIGINAL_ASPECT (320.0f/200.0f)

// -----------
// structures
// -----------

// a vertex of a Doom 'plane' polygon
typedef struct
{
	float x;
	float y;
//#ifdef SLOPENESS
	float z;
//#endif
} polyvertex_t;

#ifdef _MSC_VER
#pragma warning(disable :  4200)
#endif

// a convex 'plane' polygon, clockwise order
typedef struct
{
	int numpts;
	polyvertex_t pts[0];
} poly_t;

#ifdef _MSC_VER
#pragma warning(default :  4200)
#endif

// holds extra info for 3D render, for each subsector in subsectors[]
typedef struct
{
	poly_t *planepoly;  // the generated convex polygon
} extrasubsector_t;

typedef struct
{ 
	extrasubsector_t *xsub;
	fixed_t fixedheight;
	int lightlevel;
	int lumpnum;
	int alpha;
	sector_t *FOFSector;
} planeinfo_t;

// needed for sprite rendering
// equivalent of the software renderer's vissprites
typedef struct gr_vissprite_s
{
	// Doubly linked list
	struct gr_vissprite_s *prev;
	struct gr_vissprite_s *next;
	float x1, x2;
	float tz, ty;
	int patchlumpnum;
	boolean flip;
	unsigned char translucency;       //alpha level 0-255
	unsigned char sectorlight;        // ...
	mobj_t *mobj; 
	boolean precip; // Tails 08-25-2002
   //Hurdler: 25/04/2000: now support colormap in hardware mode
	byte *colormap;
} gr_vissprite_t;

// --------
// hw_bsp.c
// --------
extern extrasubsector_t *extrasubsectors;
extern int addsubsector;

void HWR_InitPolyPool(void);
void HWR_FreePolyPool(void);

// --------
// hw_cache.c
// --------
void HWR_InitTextureCache(void);
void HWR_FreeTextureCache(void);
void HWR_FreeExtraSubsectors(void);

void HWR_GetFlat(int flatlumpnum);
GlideTexture_t *HWR_GetTexture(int tex);
void HWR_GetPatch(GlidePatch_t *gpatch);
void HWR_GetMappedPatch(GlidePatch_t *gpatch, const byte *colormap);
GlidePatch_t *HWR_GetPic(int lumpnum);
void HWR_SetPalette(RGBA_t *palette);

// --------
// hw_draw.c
// --------
extern float gr_patch_scalex;
extern float gr_patch_scaley;

extern consvar_t cv_grrounddown; // on/off

extern int patchformat;
extern int textureformat;
extern boolean firetranslucent;

#endif //_HW_GLOB_
