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
/// \brief map utility functions

#ifndef __P_MAPUTL__
#define __P_MAPUTL__

#include "doomtype.h"
#include "r_defs.h"
#include "m_fixed.h"

//
// P_MAPUTL
//
typedef struct
{
	fixed_t x, y, dx, dy;
} divline_t;

typedef struct
{
	fixed_t frac; // along trace line
	boolean isaline;
	union
	{
		mobj_t *thing;
		line_t *line;
	} d;
} intercept_t;

extern intercept_t *intercepts;
extern intercept_t *intercept_p;

typedef boolean (*traverser_t)(intercept_t *in);

boolean P_PathTraverse(fixed_t x1, fixed_t y1, fixed_t x2, fixed_t y2,
	int flags, traverser_t trav);

fixed_t P_AproxDistance(fixed_t dx, fixed_t dy);
int P_PointOnLineSide(fixed_t x, fixed_t y, line_t *line);
int P_PointOnDivlineSide(fixed_t x, fixed_t y, divline_t *line);
void P_MakeDivline(line_t *li, divline_t *dl);
void P_CameraLineOpening(line_t *linedef);
fixed_t P_InterceptVector(divline_t *v2, divline_t *v1);
int P_BoxOnLineSide(fixed_t *tmbox, line_t *ld);
void P_UnsetPrecipThingPosition(precipmobj_t *thing);
void P_SetPrecipitationThingPosition(precipmobj_t *thing);
void P_CreatePrecipSecNodeList(precipmobj_t *thing, fixed_t x,fixed_t y);
boolean P_SceneryTryMove(mobj_t *thing, fixed_t x, fixed_t y);

extern fixed_t opentop, openbottom, openrange, lowfloor;

void P_LineOpening(line_t *linedef);

boolean P_BlockLinesIterator(int x, int y, boolean(*func)(line_t *));
boolean P_BlockThingsIterator(int x, int y, boolean(*func)(mobj_t *));

#define PT_ADDLINES     1
#define PT_ADDTHINGS    2
#define PT_EARLYOUT     4

extern divline_t trace;

extern fixed_t tmbbox[4]; // p_map.c

// call your user function for each line of the blockmap in the
// bbox defined by the radius
//boolean P_RadiusLinesCheck(fixed_t radius, fixed_t x, fixed_t y,
//	boolean (*func)(line_t *));
#endif // __P_MAPUTL__
