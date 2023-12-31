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
/// \brief SDL specific part of the OpenGL API for SRB2

#ifdef _MSC_VER
#pragma warning(disable : 4214 4244)
#endif

#include "SDL.h"

#ifdef _MSC_VER
#pragma warning(default : 4214 4244)
#endif

#include "../doomdef.h"
#include "../hardware/r_opengl/r_opengl.h"
#include "ogl_sdl.h"
#include "../i_system.h"

#ifdef DEBUG_TO_FILE
#include <stdarg.h>
#if (defined (_WIN32) || defined (_WIN64)) && !defined (__CYGWIN__)
#include <direct.h>
#else
#include <unistd.h>
#endif
#include <sys/types.h>
#include <sys/stat.h>
#ifndef SDLIO
#include <fcntl.h>
#endif
#endif

#ifdef USE_WGL_SWAP
PFNWGLEXTSWAPCONTROLPROC wglSwapIntervalEXT = NULL;
#else
typedef int (*PFNGLXSWAPINTERVALPROC) (int);
PFNGLXSWAPINTERVALPROC glXSwapIntervalSGIEXT = NULL;
#endif

/**	\brief SDL video display surface
*/
SDL_Surface *vidSurface = NULL;
int oglflags = 0;

/**	\brief	The OglSdlSurface function

	\param	w	width
	\param	h	height
	\param	isFullscreen	if true, go fullscreen

	\return	if true, changed video mode
*/
boolean OglSdlSurface(int w, int h, boolean isFullscreen)
{
	int cbpp;
	const GLvoid *glvendor = NULL, *glrenderer = NULL, *glversion = NULL;

	cbpp = cv_scr_depth.value < 16 ? 16 : cv_scr_depth.value;

	if (vidSurface)
	{
		//Alam: SDL_Video system free vidSurface for me
#ifdef VOODOOSAFESWITCHING
		SDL_QuitSubSystem(SDL_INIT_VIDEO);
		SDL_InitSubSystem(SDL_INIT_VIDEO);
#endif
	}

	cbpp = SDL_VideoModeOK(w, h, cbpp, SDL_OPENGL|(isFullscreen?SDL_FULLSCREEN:SDL_RESIZABLE));
	if (cbpp < 16)
		return true; //Alam: Let just say we did, ok?
	vidSurface = SDL_SetVideoMode(w, h, cbpp, SDL_OPENGL|(isFullscreen?SDL_FULLSCREEN:SDL_RESIZABLE));
	if (!vidSurface)
		return false;

	glvendor = glGetString(GL_VENDOR);
	// Get info and extensions.
	//BP: why don't we make it earlier ?
	//Hurdler: we cannot do that before intialising gl context
	glrenderer = glGetString(GL_RENDERER);
	glversion = glGetString(GL_VERSION);
	gl_extensions = glGetString(GL_EXTENSIONS);

	DBG_Printf("Vendor     : %s\n", glvendor);
	DBG_Printf("Renderer   : %s\n", glrenderer);
	DBG_Printf("Version    : %s\n", glversion);
	DBG_Printf("Extensions : %s\n", gl_extensions);
	oglflags = 0;

#if defined (_WIN32) || defined (_WIN64)
	// BP: disable advenced feature that don't work on somes hardware
	// Hurdler: Now works on G400 with bios 1.6 and certified drivers 6.04
	if (strstr(glrenderer, "810")) oglflags |= GLF_NOZBUFREAD;
#elif defined (UNIXLIKE)
	// disable advanced features not working on somes hardware
	if (strstr(glrenderer, "G200")) oglflags |= GLF_NOTEXENV;
	if (strstr(glrenderer, "G400")) oglflags |= GLF_NOTEXENV;
#endif
	DBG_Printf("oglflags   : 0x%X\n", oglflags );

#ifdef USE_PALETTED_TEXTURE
	if (isExtAvailable("GL_EXT_paletted_texture", gl_extensions))
		glColorTableEXT = SDL_GL_GetProcAddress("glColorTableEXT");
	else
		glColorTableEXT = NULL;

#endif
#ifdef USE_WGL_SWAP
	if (isExtAvailable("WGL_EXT_swap_control", gl_extensions))
		wglSwapIntervalEXT = SDL_GL_GetProcAddress("wglSwapIntervalEXT");
	else
		wglSwapIntervalEXT = NULL;
#else
	if (isExtAvailable("SGI_swap_control", gl_extensions))
		glXSwapIntervalSGIEXT = SDL_GL_GetProcAddress("glXSwapIntervalSGI");
	else
		glXSwapIntervalSGIEXT = NULL;
#endif
	if (isExtAvailable("GL_EXT_texture_filter_anisotropic", gl_extensions))
		glGetIntegerv(GL_MAX_TEXTURE_MAX_ANISOTROPY_EXT, &maximumAnisotropy);
	else
		maximumAnisotropy = 0;

	granisotropicmode_cons_t[1].value = maximumAnisotropy;

	SetModelView(w, h);
	SetStates();
	glClear(GL_COLOR_BUFFER_BIT|GL_DEPTH_BUFFER_BIT);

	HWR_Startup();
#ifdef KOS_GL_COMPATIBILITY
	textureformatGL = GL_ARGB4444;
#else
	textureformatGL = cbpp > 16 ? GL_RGBA : GL_RGB5_A1;
#endif

#ifdef DEBUG_TO_FILE
	logstream = open("ogllog.txt",O_WRONLY|O_CREAT,0666);
#endif
	return true;
}

/**	\brief	The OglSdlFinishUpdate function

	\param	vidwait	wait for video sync

	\return	void
*/
void OglSdlFinishUpdate(boolean waitvbl)
{
	static boolean oldwaitvbl = false;
	if(oldwaitvbl != waitvbl)
	{
#ifdef USE_WGL_SWAP
		if (wglSwapIntervalEXT)
			wglSwapIntervalEXT(waitvbl);
#else
		if (glXSwapIntervalSGIEXT)
			glXSwapIntervalSGIEXT(waitvbl);
#endif
	}
	oldwaitvbl = waitvbl;

	SDL_GL_SwapWindow(window)
}

/**	\brief Shutdown OpenGL/SDL system
*/
void OglSdlShutdown(void)
{
	vidSurface = NULL; //Alam: SDL_Video system free vidSurface for me
#ifdef DEBUG_TO_FILE
	if (logstream != -1) close(logstream);
#endif
}

EXPORT void HWRAPI( OglSdlSetPalette) (RGBA_t *palette, RGBA_t *pgamma)
{
	int i = -1;
	byte redgamma = pgamma->s.red, greengamma = pgamma->s.green,
		bluegamma = pgamma->s.blue;

	i = SDL_SetGamma(byteasfloat(redgamma), byteasfloat(greengamma), byteasfloat(bluegamma));
	if (i == 0) redgamma = greengamma = bluegamma = 0x7F; //Alam: cool
	for (i = 0; i < 256; i++)
	{
		myPaletteData[i].s.red   = (byte)MIN((palette[i].s.red   * redgamma)  /127, 255);
		myPaletteData[i].s.green = (byte)MIN((palette[i].s.green * greengamma)/127, 255);
		myPaletteData[i].s.blue  = (byte)MIN((palette[i].s.blue  * bluegamma) /127, 255);
		myPaletteData[i].s.alpha = palette[i].s.alpha;
	}
#ifdef USE_PALETTED_TEXTURE
	if (glColorTableEXT)
	{
		for (i = 0; i < 256; i++)
		{
			palette_tex[(3*i)+0] = palette[i].s.red;
			palette_tex[(3*i)+1] = palette[i].s.green;
			palette_tex[(3*i)+2] = palette[i].s.blue;
		}
		glColorTableEXT(GL_TEXTURE_2D, GL_RGB8, 256, GL_RGB, GL_UNSIGNED_BYTE, palette_tex);
	}
#endif
	// on a chang�de palette, il faut recharger toutes les textures
	// jaja, und noch viel mehr ;-)
	Flush();
}
