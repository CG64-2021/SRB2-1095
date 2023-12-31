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
/// \brief imports/exports for the 3D hardware low-level interface API

#ifndef __HWR_DRV_H__
#define __HWR_DRV_H__

// this must be here 19991024 by Kin
#include "../screen.h"
#include "hw_data.h"
#include "hw_defs.h"
#include "hw_md2.h"

#include "hw_dll.h"

// ==========================================================================
//                                                       STANDARD DLL EXPORTS
// ==========================================================================

#ifdef SDL
#undef VID_X11
#endif

EXPORT boolean HWRAPI(Init) (I_Error_t ErrorFunction);
#ifndef SDL
EXPORT void HWRAPI(Shutdown) (void);
#endif
#ifdef _WINDOWS
EXPORT void HWRAPI(GetModeList) (vmode_t **pvidmodes, int *numvidmodes);
#endif
#ifdef VID_X11
EXPORT Window HWRAPI(HookXwin) (Display *, int, int, boolean);
#endif
#if defined (PURESDL) || defined (macintosh)
EXPORT void HWRAPI(SetPalette) (int *, RGBA_t *gamma);
#else
EXPORT void HWRAPI(SetPalette) (RGBA_t *pal, RGBA_t *gamma);
#endif
EXPORT void HWRAPI(FinishUpdate) (int waitvbl);
EXPORT void HWRAPI(Draw2DLine) (F2DCoord *v1, F2DCoord *v2, RGBA_t Color);
EXPORT void HWRAPI(DrawPolygon) (FSurfaceInfo *pSurf, FOutVector *pOutVerts, FUINT iNumPts, FBITFIELD PolyFlags);
EXPORT void HWRAPI(SetBlend) (FBITFIELD PolyFlags);
EXPORT void HWRAPI(ClearBuffer) (FBOOLEAN ColorMask, FBOOLEAN DepthMask, FRGBAFloat *ClearColor);
EXPORT void HWRAPI(SetTexture) (FTextureInfo *TexInfo);
EXPORT void HWRAPI(ReadRect) (int x, int y, int width, int height, int dst_stride, unsigned short *dst_data);
EXPORT void HWRAPI(GClipRect) (int minx, int miny, int maxx, int maxy, float nearclip);
EXPORT void HWRAPI(ClearMipMapCache) (void);

//Hurdler: added for backward compatibility
EXPORT void HWRAPI(SetSpecialState) (hwdspecialstate_t IdState, int Value);

//Hurdler: added for new development
EXPORT void HWRAPI(DrawMD2) (int *gl_cmd_buffer, md2_frame_t *frame, FTransform *pos, float scale);
EXPORT void HWRAPI(SetTransform) (FTransform *transform);
EXPORT int HWRAPI(GetTextureUsed) (void);
EXPORT int HWRAPI(GetRenderVersion) (void);

EXPORT void HWRAPI(MakeScreenFinalTexture) (void);
EXPORT void HWRAPI(DrawScreenFinalTexture) (int width, int height);
EXPORT void HWRAPI(FlushScreenTextures) (void);

#ifdef VID_X11 // ifdef to be removed as soon as windoze supports that as well
// metzgermeister: added for Voodoo detection
EXPORT char *HWRAPI(GetRenderer) (void);
#endif

// ==========================================================================
//                                      HWR DRIVER OBJECT, FOR CLIENT PROGRAM
// ==========================================================================

#if !defined (_CREATE_DLL_)

struct hwdriver_s
{
	Init                pfnInit;
	SetPalette          pfnSetPalette;
	FinishUpdate        pfnFinishUpdate;
	Draw2DLine          pfnDraw2DLine;
	DrawPolygon         pfnDrawPolygon;
	SetBlend            pfnSetBlend;
	ClearBuffer         pfnClearBuffer;
	SetTexture          pfnSetTexture;
	ReadRect            pfnReadRect;
	GClipRect           pfnGClipRect;
	ClearMipMapCache    pfnClearMipMapCache;
	SetSpecialState     pfnSetSpecialState;//Hurdler: added for backward compatibility
	DrawMD2             pfnDrawMD2;
	SetTransform        pfnSetTransform;
	GetTextureUsed      pfnGetTextureUsed;
	GetRenderVersion    pfnGetRenderVersion;
	MakeScreenFinalTexture  pfnMakeScreenFinalTexture;
	DrawScreenFinalTexture  pfnDrawScreenFinalTexture;
	FlushScreenTextures pfnFlushScreenTextures;
#ifdef _WINDOWS
	GetModeList         pfnGetModeList;
#endif
#ifdef VID_X11
	HookXwin            pfnHookXwin;
	GetRenderer         pfnGetRenderer;
#endif
#ifndef SDL
	Shutdown            pfnShutdown;
#endif
};

extern struct hwdriver_s hwdriver;

//Hurdler: 16/10/99: added for OpenGL gamma correction
//extern RGBA_t  gamma_correction;

#define HWD hwdriver

#endif //not defined _CREATE_DLL_

#endif //__HWR_DRV_H__

