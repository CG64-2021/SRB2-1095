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
/// \brief 3D render mode functions

#ifndef __HWR_MAIN_H__
#define __HWR_MAIN_H__

#include "hw_data.h"
#include "hw_defs.h"

#include "../am_map.h"
#include "../d_player.h"
#include "../r_defs.h"

// Startup & Shutdown the hardware mode renderer
void HWR_Startup(void);
void HWR_Shutdown(void);

void HWR_clearAutomap(void);
void HWR_drawAMline(const fline_t *fl, int color);
void HWR_FadeScreenMenuBack(unsigned long color, int height);
void HWR_RenderPlayerView(int viewnumber, player_t *player);
void HWR_DrawViewBorder(int clearlines);
void HWR_DrawFlatFill(int x, int y, int w, int h, int flatlumpnum);
UINT8 *HWR_GetScreenshot(void);
boolean HWR_Screenshot(const char *lbmname);
void HWR_InitTextureMapping(void);
void HWR_SetViewSize(int blocks);
void HWR_DrawPatch(GlidePatch_t *gpatch, int x, int y, int option);
void HWR_DrawClippedPatch(GlidePatch_t *gpatch, int x, int y, int option);
void HWR_DrawTranslucentPatch(GlidePatch_t *gpatch, int x, int y, int option);
void HWR_DrawSmallPatch(GlidePatch_t *gpatch, int x, int y, int option, const byte *colormap);
void HWR_DrawMappedPatch(GlidePatch_t *gpatch, int x, int y, int option, const byte *colormap);
void HWR_MakePatch(const patch_t *patch, GlidePatch_t *grPatch, GlideMipmap_t *grMipmap);
void HWR_CreatePlanePolygons(int bspnum);
void HWR_CreateStaticLightmaps(int bspnum);
void HWR_PrepLevelCache(int numtextures);
void HWR_DrawFill(int x, int y, int w, int h, int color);
void HWR_DrawPic(int x,int y,int lumpnum);

void HWR_AddCommands(void);
void HWR_CorrectSWTricks(void);
void transform(float *cx, float *cy, float *cz);
FBITFIELD HWR_TranstableToAlpha(int transtablenum, FSurfaceInfo *pSurf);
void HWR_SetPaletteColor(int palcolor);
int HWR_GetTextureUsed(void);
void HWR_MakeScreenFinalTexture(void);
void HWR_DrawScreenFinalTexture(int width, int height);

extern CV_PossibleValue_t granisotropicmode_cons_t[];

extern consvar_t cv_grcrappymlook;
extern consvar_t cv_grdynamiclighting;
extern consvar_t cv_grstaticlighting;
extern consvar_t cv_grcoronas;
extern consvar_t cv_grcoronasize;
extern consvar_t cv_grfov;
extern consvar_t cv_grmd2;
extern consvar_t cv_grtranswall;
extern consvar_t cv_grfog;
extern consvar_t cv_grfogcolor;
extern consvar_t cv_grfogdensity;
extern consvar_t cv_grgammared;
extern consvar_t cv_grgammagreen;
extern consvar_t cv_grgammablue;
extern consvar_t cv_grfiltermode;
extern consvar_t cv_granisotropicmode;
extern consvar_t cv_grcorrecttricks;
extern consvar_t cv_voodoocompatibility;
extern consvar_t cv_grfovchange;
extern consvar_t cv_grsolvetjoin;

extern float gr_viewwidth, gr_viewheight, gr_baseviewwindowy;

extern float gr_viewwindowx, grfovadjust, gr_basewindowcentery;

// BP: big hack for a test in lighting ref : 1249753487AB
extern fixed_t *hwbbox;
extern FTransform atransform;

#endif
