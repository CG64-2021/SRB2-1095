// Emacs style mode select   -*- C++ -*-
//-----------------------------------------------------------------------------
//
// Copyright (C) 1993-1996 by id Software, Inc.
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
//-----------------------------------------------------------------------------
/// \file
/// \brief Commonly used routines
///
///	Default Config File.
///	Screenshots.
/// Moviemode.
///	File i/o

#ifdef __GNUC__
#include <unistd.h>
#endif
// Extended map support.
#include <ctype.h>
#ifdef SDLIO
#include "SDL_rwops.h"
#else
#ifndef _WIN32_WCE
#include <fcntl.h>
#endif
#endif

#ifdef HAVE_PNG
 #include <png.h>
 #ifdef PNG_WRITE_SUPPORTED
  #define USE_PNG // Only actually use PNG if write is supported.
  // See hardware/hw_draw.c for a similar check to this one.
 #endif
#endif

#ifdef HAVE_MNG
#define MNG_NO_INCLUDE_JNG
#define MNG_SUPPORT_WRITE
#define MNG_ACCESS_CHUNKS
#include <libmng.h>
#endif

#include "doomdef.h"
#include "g_game.h"
#include "m_misc.h"
#include "m_menu.h"
#include "hu_stuff.h"
#include "st_stuff.h"
#include "v_video.h"
#include "z_zone.h"
#include "g_input.h"
#include "i_video.h"
#include "d_main.h"
#include "m_argv.h"
#include "i_system.h"
#include "m_anigif.h"

#ifdef _WIN32_WCE
#include "sdl/SRB2CE/cehelp.h"
#endif

#ifdef _XBOX
#include "sdl/SRB2XBOX/xboxhelp.h"
#endif

#ifdef HWRENDER
#include "hardware/hw_main.h"
#endif

#ifdef HAVE_SDL
#include "sdl/hwsym_sdl.h"
#ifdef __linux__
#ifndef _LARGEFILE64_SOURCE
typedef off_t off64_t;
#endif
#endif
#endif

#if defined(__MINGW32__) && ((__GNUC__ > 7) || (__GNUC__ == 6 && __GNUC_MINOR__ >= 3)) && (__GNUC__ < 8)
#define PRIdS "u"
#elif defined (_WIN32)
#define PRIdS "Iu"
#elif defined (DJGPP)
#define PRIdS "u"
#else
#define PRIdS "zu"
#endif

#ifdef HAVE_PNG

#ifndef _MSC_VER
#ifndef _LARGEFILE64_SOURCE
#define _LARGEFILE64_SOURCE
#endif
#endif

#ifndef _LFS64_LARGEFILE
#define _LFS64_LARGEFILE
#endif

#ifndef _FILE_OFFSET_BITS
#define _FILE_OFFSET_BITS 0
#endif

 #include "zlib.h"
 #include "png.h"
 #if (PNG_LIBPNG_VER_MAJOR > 1) || (PNG_LIBPNG_VER_MAJOR == 1 && PNG_LIBPNG_VER_MINOR >= 4)
  #define NO_PNG_DEBUG // 1.4.0 move png_debug to pngpriv.h
 #endif
 #ifdef PNG_WRITE_SUPPORTED
  #define USE_PNG // Only actually use PNG if write is supported.
  #if defined (PNG_WRITE_APNG_SUPPORTED) //|| !defined(PNG_STATIC)
    #include "apng.h"
    #define USE_APNG
  #endif
  // See hardware/hw_draw.c for a similar check to this one.
 #endif
#endif

static CV_PossibleValue_t screenshot_cons_t[] = {{0, "Default"}, {1, "HOME"}, {2, "SRB2"}, {3, "CUSTOM"}, {0, NULL}};
consvar_t cv_screenshot_option = {"screenshot_option", "Default", CV_SAVE|CV_CALL, screenshot_cons_t, Screenshot_option_Onchange, 0, NULL, NULL, 0, 0, NULL};
consvar_t cv_screenshot_folder = {"screenshot_folder", "", CV_SAVE, NULL, NULL, 0, NULL, NULL, 0, 0, NULL};

static CV_PossibleValue_t moviemode_cons_t[] = {{MM_GIF, "GIF"}, {MM_APNG, "aPNG"}, {MM_SCREENSHOT, "Screenshots"}, {MM_MNG, "MNG"}, {0, NULL}};
consvar_t cv_moviemode = {"moviemode_mode", "GIF", CV_SAVE|CV_CALL, moviemode_cons_t, Moviemode_mode_Onchange, 0, NULL, NULL, 0, 0, NULL};

consvar_t cv_movie_option = {"movie_option", "Default", CV_SAVE|CV_CALL, screenshot_cons_t, Moviemode_option_Onchange, 0, NULL, NULL, 0, 0, NULL};
consvar_t cv_movie_folder = {"movie_folder", "", CV_SAVE, NULL, NULL, 0, NULL, NULL, 0, 0, NULL};

static CV_PossibleValue_t zlib_mem_level_t[] = {
	{1, "(Min Memory) 1"},
	{2, "2"}, {3, "3"}, {4, "4"}, {5, "5"}, {6, "6"}, {7, "7"},
	{8, "(Optimal) 8"}, //libpng Default
	{9, "(Max Memory) 9"}, {0, NULL}};

static CV_PossibleValue_t zlib_level_t[] = {
	{0, "No Compression"},  //Z_NO_COMPRESSION
	{1, "(Fastest) 1"}, //Z_BEST_SPEED
	{2, "2"}, {3, "3"}, {4, "4"}, {5, "5"},
	{6, "(Optimal) 6"}, //Zlib Default
	{7, "7"}, {8, "8"},
	{9, "(Maximum) 9"}, //Z_BEST_COMPRESSION
	{0, NULL}};

static CV_PossibleValue_t zlib_strategy_t[] = {
	{0, "Normal"}, //Z_DEFAULT_STRATEGY
	{1, "Filtered"}, //Z_FILTERED
	{2, "Huffman Only"}, //Z_HUFFMAN_ONLY
	{3, "RLE"}, //Z_RLE
	{4, "Fixed"}, //Z_FIXED
	{0, NULL}};

static CV_PossibleValue_t zlib_window_bits_t[] = {
#ifdef WBITS_8_OK
	{8, "256"},
#endif
	{9, "512"}, {10, "1k"}, {11, "2k"}, {12, "4k"}, {13, "8k"},
	{14, "16k"}, {15, "32k"},
	{0, NULL}};

static CV_PossibleValue_t apng_delay_t[] = {
	{1, "1x"},
	{2, "1/2x"},
	{3, "1/3x"},
	{4, "1/4x"},
	{0, NULL}};

// zlib memory usage is as follows:
// (1 << (zlib_window_bits+2)) +  (1 << (zlib_level+9))
consvar_t cv_zlib_memory = {"png_memory_level", "7", CV_SAVE, zlib_mem_level_t, NULL, 0, NULL, NULL, 0, 0, NULL};
consvar_t cv_zlib_level = {"png_compress_level", "(Optimal) 6", CV_SAVE, zlib_level_t, NULL, 0, NULL, NULL, 0, 0, NULL};
consvar_t cv_zlib_strategy = {"png_strategy", "Normal", CV_SAVE, zlib_strategy_t, NULL, 0, NULL, NULL, 0, 0, NULL};
consvar_t cv_zlib_window_bits = {"png_window_size", "32k", CV_SAVE, zlib_window_bits_t, NULL, 0, NULL, NULL, 0, 0, NULL};

consvar_t cv_zlib_memorya = {"apng_memory_level", "(Max Memory) 9", CV_SAVE, zlib_mem_level_t, NULL, 0, NULL, NULL, 0, 0, NULL};
consvar_t cv_zlib_levela = {"apng_compress_level", "4", CV_SAVE, zlib_level_t, NULL, 0, NULL, NULL, 0, 0, NULL};
consvar_t cv_zlib_strategya = {"apng_strategy", "RLE", CV_SAVE, zlib_strategy_t, NULL, 0, NULL, NULL, 0, 0, NULL};
consvar_t cv_zlib_window_bitsa = {"apng_window_size", "32k", CV_SAVE, zlib_window_bits_t, NULL, 0, NULL, NULL, 0, 0, NULL};
consvar_t cv_apng_delay = {"apng_speed", "1/2x", CV_SAVE, apng_delay_t, NULL, 0, NULL, NULL, 0, 0, NULL};

boolean takescreenshot = false; // Take a screenshot this tic

moviemode_t moviemode = MM_OFF;


/** Returns the map number for a map identified by the last two characters in
  * its name.
  *
  * \param first  The first character after MAP.
  * \param second The second character after MAP.
  * \return The map number, or 0 if no map corresponds to these characters.
  * \sa G_BuildMapName
  */
int M_MapNumber(char first, char second)
{
	if (isdigit(first))
	{
		if (isdigit(second))
			return ((int)first - '0') * 10 + ((int)second - '0');
		return 0;
	}

	if (!isalpha(first))
		return 0;
	if (!isalnum(second))
		return 0;

	return 100 + ((int)tolower(first) - 'a') * 36 + (isdigit(second) ? ((int)second - '0') :
		((int)tolower(second) - 'a') + 10);
}


/** Set of functions to take in a size_t as an argument,
  * put the argument in a character buffer, and return the
  * pointer to that buffer.
  * This is to eliminate usage of PRIdS, so gettext can work
  * with *all* of SRB2's strings.
  */
char *sizeu1(size_t num)
{
	static char sizeu1_buf[28];
	sprintf(sizeu1_buf, "%"PRIdS, num);
	return sizeu1_buf;
}

char *sizeu2(size_t num)
{
	static char sizeu2_buf[28];
	sprintf(sizeu2_buf, "%"PRIdS, num);
	return sizeu2_buf;
}

char *sizeu3(size_t num)
{
	static char sizeu3_buf[28];
	sprintf(sizeu3_buf, "%"PRIdS, num);
	return sizeu3_buf;
}

char *sizeu4(size_t num)
{
	static char sizeu4_buf[28];
	sprintf(sizeu4_buf, "%"PRIdS, num);
	return sizeu4_buf;
}

char *sizeu5(size_t num)
{
	static char sizeu5_buf[28];
	sprintf(sizeu5_buf, "%"PRIdS, num);
	return sizeu5_buf;
}

// ==========================================================================
//                         FILE INPUT / OUTPUT
// ==========================================================================

//
// FIL_WriteFile
//
#ifndef O_BINARY
#define O_BINARY 0
#endif

/** Writes out a file.
  *
  * \param name   Name of the file to write.
  * \param source Memory location to write from.
  * \param length How many bytes to write.
  * \return True on success, false on failure.
  */
boolean FIL_WriteFile(char const *name, const void *source, size_t length)
{
#ifdef SDLIO
	SDL_RWops *handle;
#else
	int handle;
#endif
	size_t count;

#ifdef SDLIO
	handle = SDL_RWFromFile(name, "w+b");
#else
	handle = open(name, O_WRONLY|O_CREAT|O_TRUNC|O_BINARY, 0666);
#endif

#ifdef SDLIO
	if (!handle)
#else
	if (handle == -1)
#endif
		return false;

#ifdef SDLIO
	count = SDL_RWwrite(handle, source, 1, (int)length);
	SDL_RWclose(handle);
#else
	count = write(handle, source, (unsigned int)length);
	close(handle);
#endif

	if (count < length)
		return false;

	return true;
}

/** Reads in a file, appending a zero byte at the end.
  *
  * \param name   Filename to read.
  * \param buffer Pointer to a pointer, which will be set to the location of a
  *               newly allocated buffer holding the file's contents.
  * \return Number of bytes read, not counting the zero byte added to the end,
  *         or 0 on error.
  */
int FIL_ReadFile(char const *name, byte **buffer)
{
#ifdef SDLIO
	SDL_RWops *handle;
#else
	int handle;
#endif
	int count, length;
#if !defined (SDLIO) && !defined (_arch_dreamcast)
	struct stat fileinfo;
#endif
	byte *buf;

#ifdef SDLIO
	handle = SDL_RWFromFile(name, "rb");
	if (!handle)
#else
	handle = open(name, O_RDONLY | O_BINARY, 0666);
	if (handle == -1)
#endif
		return 0;

#if !defined (SDLIO) && !defined (_arch_dreamcast)
	if (fstat(handle, &fileinfo) == -1)
		return 0;
#endif

#ifdef SDLIO
	{
		int currpos = SDL_RWtell(handle);
		SDL_RWseek(handle,0,SEEK_END);
		length = SDL_RWtell(handle);
		SDL_RWseek(handle,currpos,SEEK_SET);
	}
#elif defined (_arch_dreamcast)
	length = fs_total(handle);
#else
	length = fileinfo.st_size;
#endif
	buf = Z_Malloc(length + 1, PU_STATIC, NULL);
#ifdef SDLIO
	count = SDL_RWread(handle, buf, 1, length);
	SDL_RWclose(handle);
#else
	count = read(handle, buf, length);
	close(handle);
#endif

	if (count < length)
	{
		Z_Free(buf);
		return 0;
	}

	// append 0 byte for script text files
	buf[length] = 0;

	*buffer = buf;
	return length;
}

/** Check if the filename exists
  *
  * \param name   Filename to check.
  * \return true if file exists, false if it doesn't.
  */
boolean FIL_FileExists(char const *name)
{
	return access(name,0)+1;
}


/** Check if the filename OK to write
  *
  * \param name   Filename to check.
  * \return true if file write-able, false if it doesn't.
  */
boolean FIL_WriteFileOK(char const *name)
{
	return access(name,2)+1;
}


/** Check if the filename OK to read
  *
  * \param name   Filename to check.
  * \return true if file read-able, false if it doesn't.
  */
boolean FIL_ReadFileOK(char const *name)
{
	return access(name,4)+1;
}

/** Check if the filename OK to read/write
  *
  * \param name   Filename to check.
  * \return true if file (read/write)-able, false if it doesn't.
  */
boolean FIL_FileOK(char const *name)
{
	return access(name,6)+1;
}


/** Checks if a pathname has a file extension and adds the extension provided
  * if not.
  *
  * \param path      Pathname to check.
  * \param extension Extension to add if no extension is there.
  */
void FIL_DefaultExtension(char *path, const char *extension)
{
	char *src;

	// search for '.' from end to begin, add .EXT only when not found
	src = path + strlen(path) - 1;

	while (*src != '/' && src != path)
	{
		if (*src == '.')
			return; // it has an extension
		src--;
	}

	strcat(path, extension);
}

void FIL_ForceExtension(char *path, const char *extension)
{
	char *src;

	// search for '.' from end to begin, add .EXT only when not found
	src = path + strlen(path) - 1;

	while (*src != '/' && src != path)
	{
		if (*src == '.')
		{
			*src = '\0';
			break; // it has an extension
		}
		src--;
	}

	strcat(path, extension);
}

/** Checks if a filename extension is found.
  * Lump names do not contain dots.
  *
  * \param in String to check.
  * \return True if an extension is found, otherwise false.
  */
boolean FIL_CheckExtension(const char *in)
{
	while (*in++)
		if (*in == '.')
			return true;

	return false;
}

// ==========================================================================
//                        CONFIGURATION FILE
// ==========================================================================

//
// DEFAULTS
//

char configfile[MAX_WADPATH];

// ==========================================================================
//                          CONFIGURATION
// ==========================================================================
static boolean gameconfig_loaded = false; // true once config.cfg loaded AND executed

/** Saves a player's config, possibly to a particular file.
  *
  * \sa Command_LoadConfig_f
  */
void Command_SaveConfig_f(void)
{
	char tmpstr[MAX_WADPATH];

	if (COM_Argc() < 2)
	{
		CONS_Printf("saveconfig <filename[.cfg]> [-silent] : save config to a file\n");
		return;
	}
	strcpy(tmpstr, COM_Argv(1));
	FIL_ForceExtension(tmpstr, ".cfg");

	M_SaveConfig(tmpstr);
	if (stricmp(COM_Argv(2), "-silent"))
		CONS_Printf("config saved as %s\n", configfile);
}

/** Loads a game config, possibly from a particular file.
  *
  * \sa Command_SaveConfig_f, Command_ChangeConfig_f
  */
void Command_LoadConfig_f(void)
{
	if (COM_Argc() != 2)
	{
		CONS_Printf("loadconfig <filename[.cfg]> : load config from a file\n");
		return;
	}

	strcpy(configfile, COM_Argv(1));
	FIL_ForceExtension(configfile, ".cfg");
	COM_BufInsertText(va("exec \"%s\"\n", configfile));
}

/** Saves the current configuration and loads another.
  *
  * \sa Command_LoadConfig_f, Command_SaveConfig_f
  */
void Command_ChangeConfig_f(void)
{
	if (COM_Argc() != 2)
	{
		CONS_Printf("changeconfig <filename[.cfg]> : save current config and load another\n");
		return;
	}

	COM_BufAddText(va("saveconfig \"%s\"\n", configfile));
	COM_BufAddText(va("loadconfig \"%s\"\n", COM_Argv(1)));
}

/** Loads the default config file.
  *
  * \sa Command_LoadConfig_f
  */
void M_FirstLoadConfig(void)
{
	// configfile is initialised by d_main when searching for the wad?

	// check for a custom config file
	if (M_CheckParm("-config") && M_IsNextParm())
	{
		strcpy(configfile, M_GetNextParm());
		CONS_Printf("config file: %s\n",configfile);
	}

	// load default control
	G_Controldefault();

	// load config, make sure those commands doesnt require the screen..
	CONS_Printf("\n");
	COM_BufInsertText(va("exec \"%s\"\n", configfile));
	// no COM_BufExecute() needed; that does it right away

	// make sure I_Quit() will write back the correct config
	// (do not write back the config if it crash before)
	gameconfig_loaded = true;
}

/** Saves the game configuration.
  *
  * \sa Command_SaveConfig_f
  */
void M_SaveConfig(char *filename)
{
	FILE *f;

	// make sure not to write back the config until it's been correctly loaded
	if (!gameconfig_loaded)
		return;

	// can change the file name
	if (filename)
	{
		f = fopen(filename, "w");
		// change it only if valid
		if (f)
			strcpy(configfile, filename);
		else
		{
			CONS_Printf("Couldn't save game config file %s\n", filename);
			return;
		}
	}
	else
	{
		f = fopen(configfile, "w");
		if (!f)
		{
			CONS_Printf("Couldn't save game config file %s\n", configfile);
			return;
		}
	}

	// header message
	fprintf(f, "// SRB2 configuration file.\n");

	// FIXME: save key aliases if ever implemented..

	CV_SaveVariables(f);
	if (!dedicated) G_SaveKeySetting(f);

	fclose(f);
}

// ==========================================================================
//                              SCREENSHOTS
// ==========================================================================
static UINT8 screenshot_palette[768];
static void M_CreateScreenShotPalette(void)
{
	size_t i, j;
	for (i = 0, j = 0; i < 768; i += 3, j++)
	{
		RGBA_t locpal = pLocalPalette[(max(st_palette,0)*256)+j];
		screenshot_palette[i] = locpal.s.red;
		screenshot_palette[i+1] = locpal.s.green;
		screenshot_palette[i+2] = locpal.s.blue;
	}
}

#if NUMSCREENS > 2
static const char *Newsnapshotfile(const char *pathname, const char *ext)
{
	static char freename[13] = "srb2XXXX.ext";
	int i = 5000; // start in the middle: num screenshots divided by 2
	int add = i; // how much to add or subtract if wrong; gets divided by 2 each time
	int result; // -1 = guess too high, 0 = correct, 1 = guess too low

	// find a file name to save it to
	strcpy(freename+9,ext);

	for (;;)
	{
		freename[4] = (char)('0' + (char)(i/1000));
		freename[5] = (char)('0' + (char)((i/100)%10));
		freename[6] = (char)('0' + (char)((i/10)%10));
		freename[7] = (char)('0' + (char)(i%10));

		if (FIL_WriteFileOK(va(pandf,pathname,freename))) // access succeeds
			result = 1; // too low
		else // access fails: equal or too high
		{
			if (!i)
				break; // not too high, so it must be equal! YAY!

			freename[4] = (char)('0' + (char)((i-1)/1000));
			freename[5] = (char)('0' + (char)(((i-1)/100)%10));
			freename[6] = (char)('0' + (char)(((i-1)/10)%10));
			freename[7] = (char)('0' + (char)((i-1)%10));
			if (!FIL_WriteFileOK(va(pandf,pathname,freename))) // access fails
				result = -1; // too high
			else
				break; // not too high, so equal, YAY!
		}

		add /= 2;

		if (!add) // don't get stuck at 5 due to truncation!
			add = 1;

		i += add * result;

		if (i < 0 || i > 9999)
			return NULL;
	}

	freename[4] = (char)('0' + (char)(i/1000));
	freename[5] = (char)('0' + (char)((i/100)%10));
	freename[6] = (char)('0' + (char)((i/10)%10));
	freename[7] = (char)('0' + (char)(i%10));

	return freename;
}
#endif

#ifdef HAVE_PNG
FUNCNORETURN static void PNG_error(png_structp PNG, png_const_charp pngtext)
{
	//CONS_Debug(DBG_RENDER, "libpng error at %p: %s", PNG, pngtext);
	I_Error("libpng error at %p: %s", PNG, pngtext);
}

static void PNG_warn(png_structp PNG, png_const_charp pngtext)
{
	CONS_Printf("libpng warning at %p: %s", PNG, pngtext);
}

static void M_PNGhdr(png_structp png_ptr, png_infop png_info_ptr, PNG_CONST png_uint_32 width, PNG_CONST png_uint_32 height, PNG_CONST png_byte *palette)
{
	const png_byte png_interlace = PNG_INTERLACE_NONE; //PNG_INTERLACE_ADAM7
	if (palette)
	{
		png_colorp png_PLTE = png_malloc(png_ptr, sizeof(png_color)*256); //palette
		const png_byte *pal = palette;
		png_uint_16 i;
		for (i = 0; i < 256; i++)
		{
			png_PLTE[i].red   = *pal; pal++;
			png_PLTE[i].green = *pal; pal++;
			png_PLTE[i].blue  = *pal; pal++;
		}
		png_set_IHDR(png_ptr, png_info_ptr, width, height, 8, PNG_COLOR_TYPE_PALETTE,
		 png_interlace, PNG_COMPRESSION_TYPE_BASE, PNG_FILTER_TYPE_BASE);
		png_write_info_before_PLTE(png_ptr, png_info_ptr);
		png_set_PLTE(png_ptr, png_info_ptr, png_PLTE, 256);
		png_free(png_ptr, (png_voidp)png_PLTE); // safe in libpng-1.2.1+
		png_set_filter(png_ptr, PNG_FILTER_TYPE_BASE, PNG_FILTER_NONE);
		png_set_compression_strategy(png_ptr, Z_DEFAULT_STRATEGY);
	}
	else
	{
		png_set_IHDR(png_ptr, png_info_ptr, width, height, 8, PNG_COLOR_TYPE_RGB,
		 png_interlace, PNG_COMPRESSION_TYPE_BASE, PNG_FILTER_TYPE_BASE);
		png_write_info_before_PLTE(png_ptr, png_info_ptr);
		png_set_compression_strategy(png_ptr, Z_FILTERED);
	}
}

static void M_PNGText(png_structp png_ptr, png_infop png_info_ptr, PNG_CONST png_byte movie)
{
#ifdef PNG_TEXT_SUPPORTED
#define SRB2PNGTXT 10 //PNG_KEYWORD_MAX_LENGTH(79) is the max
	png_text png_infotext[SRB2PNGTXT];
	char keytxt[SRB2PNGTXT][12] = {
	"Title", "Description", "Playername", "Mapnum", "Mapname",
	"Location", "Interface", "Render Mode", "Build Date", "Build Time"};
	char titletxt[] = "Sonic Robo Blast 2 " VERSIONSTRING;
	png_charp playertxt =  cv_playername.zstring;
	char desctxt[] = "SRB2 Screenshot";
	char Movietxt[] = "SRB2 Movie";
	size_t i;
	char interfacetxt[] =
#ifdef HAVE_SDL
	 "SDL";
#elif defined (_WINDOWS)
	 "DirectX";
#elif defined (PC_DOS)
	 "Allegro";
#else
	 "Unknown";
#endif
	char rendermodetxt[9];
	char maptext[8];
	char lvlttltext[48];
	char locationtxt[40];
	char ctdate[40];
	char cttime[40];

	switch (rendermode)
	{
		case render_soft:
			strcpy(rendermodetxt, "Software");
			break;
		case render_opengl:
			strcpy(rendermodetxt, "OpenGL");
			break;
		default: // Just in case
			strcpy(rendermodetxt, "None");
			break;
	}

	if (gamestate == GS_LEVEL)
		snprintf(maptext, 8, "%s", G_BuildMapName(gamemap));
	else
		snprintf(maptext, 8, "Unknown");

	if (gamestate == GS_LEVEL && mapheaderinfo[gamemap-1].lvlttl[0] != '\0')
		snprintf(lvlttltext, 48, "%s%s%s",
			mapheaderinfo[gamemap-1].lvlttl,
			(mapheaderinfo[gamemap-1].nozone) ? "" : " Zone",
			(mapheaderinfo[gamemap-1].actnum > 0) ? va(" %d",mapheaderinfo[gamemap-1].actnum) : "");
	else
		snprintf(lvlttltext, 48, "Unknown");

	if (gamestate == GS_LEVEL && players[displayplayer].mo)
		snprintf(locationtxt, 40, "X:%d Y:%d Z:%d A:%d",
			players[displayplayer].mo->x>>FRACBITS,
			players[displayplayer].mo->y>>FRACBITS,
			players[displayplayer].mo->z>>FRACBITS,
			FixedInt(AngleFixed(players[displayplayer].mo->angle)));
	else
		snprintf(locationtxt, 40, "Unknown");

	memset(png_infotext,0x00,sizeof (png_infotext));

	for (i = 0; i < SRB2PNGTXT; i++)
		png_infotext[i].key  = keytxt[i];

	png_infotext[0].text = titletxt;
	if (movie)
		png_infotext[1].text = Movietxt;
	else
		png_infotext[1].text = desctxt;
	png_infotext[2].text = playertxt;
	png_infotext[3].text = maptext;
	png_infotext[4].text = lvlttltext;
	png_infotext[5].text = locationtxt;
	png_infotext[6].text = interfacetxt;
	png_infotext[7].text = rendermodetxt;
	png_infotext[8].text = strncpy(ctdate, compdate, sizeof(ctdate)-1);
	png_infotext[9].text = strncpy(cttime, comptime, sizeof(cttime)-1);

	png_set_text(png_ptr, png_info_ptr, png_infotext, SRB2PNGTXT);
#undef SRB2PNGTXT
#endif
}

static inline void M_PNGImage(png_structp png_ptr, png_infop png_info_ptr, PNG_CONST png_uint_32 height, png_bytep png_buf)
{
	png_uint_32 pitch = png_get_rowbytes(png_ptr, png_info_ptr);
	png_bytepp row_pointers = png_malloc(png_ptr, height* sizeof (png_bytep));
	png_uint_32 y;
	for (y = 0; y < height; y++)
	{
		row_pointers[y] = png_buf;
		png_buf += pitch;
	}
	png_write_image(png_ptr, row_pointers);
	png_free(png_ptr, (png_voidp)row_pointers);
}

#ifdef USE_APNG
static png_structp apng_ptr = NULL;
static png_infop   apng_info_ptr = NULL;
static apng_infop  apng_ainfo_ptr = NULL;
static png_FILE_p  apng_FILE = NULL;
static png_uint_32 apng_frames = 0;
#ifdef PNG_STATIC // Win32 build have static libpng
#define aPNG_set_acTL png_set_acTL
#define aPNG_write_frame_head png_write_frame_head
#define aPNG_write_frame_tail png_write_frame_tail
#else // outside libpng may not have apng support

#ifndef PNG_WRITE_APNG_SUPPORTED // libpng header may not have apng patch

#ifndef PNG_INFO_acTL
#define PNG_INFO_acTL 0x10000L
#endif
#ifndef PNG_INFO_fcTL
#define PNG_INFO_fcTL 0x20000L
#endif
#ifndef PNG_FIRST_FRAME_HIDDEN
#define PNG_FIRST_FRAME_HIDDEN       0x0001
#endif
#ifndef PNG_DISPOSE_OP_NONE
#define PNG_DISPOSE_OP_NONE        0x00
#endif
#ifndef PNG_DISPOSE_OP_BACKGROUND
#define PNG_DISPOSE_OP_BACKGROUND  0x01
#endif
#ifndef PNG_DISPOSE_OP_PREVIOUS
#define PNG_DISPOSE_OP_PREVIOUS    0x02
#endif
#ifndef PNG_BLEND_OP_SOURCE
#define PNG_BLEND_OP_SOURCE        0x00
#endif
#ifndef PNG_BLEND_OP_OVER
#define PNG_BLEND_OP_OVER          0x01
#endif
#ifndef PNG_HAVE_acTL
#define PNG_HAVE_acTL             0x4000
#endif
#ifndef PNG_HAVE_fcTL
#define PNG_HAVE_fcTL             0x8000L
#endif

#endif
typedef png_uint_32 (*P_png_set_acTL) (png_structp png_ptr,
   png_infop info_ptr, png_uint_32 num_frames, png_uint_32 num_plays);
typedef void (*P_png_write_frame_head) (png_structp png_ptr,
   png_infop info_ptr, png_bytepp row_pointers,
   png_uint_32 width, png_uint_32 height,
   png_uint_32 x_offset, png_uint_32 y_offset,
   png_uint_16 delay_num, png_uint_16 delay_den, png_byte dispose_op,
   png_byte blend_op);

typedef void (*P_png_write_frame_tail) (png_structp png_ptr,
   png_infop info_ptr);
static P_png_set_acTL aPNG_set_acTL = NULL;
static P_png_write_frame_head aPNG_write_frame_head = NULL;
static P_png_write_frame_tail aPNG_write_frame_tail = NULL;
#endif

static inline boolean M_PNGLib(void)
{
#ifdef PNG_STATIC // Win32 build have static libpng
	return true;
#else
	static void *pnglib = NULL;
	if (aPNG_set_acTL && aPNG_write_frame_head && aPNG_write_frame_tail)
		return true;
	if (pnglib)
		return false;
#ifdef _WIN32
	pnglib = GetModuleHandleA("libpng.dll");
	if (!pnglib)
		pnglib = GetModuleHandleA("libpng12.dll");
	if (!pnglib)
		pnglib = GetModuleHandleA("libpng13.dll");
#elif defined (HAVE_SDL)
#ifdef __APPLE__
	pnglib = hwOpen("libpng.dylib");
#else
	pnglib = hwOpen("libpng.so");
#endif
#endif
	if (!pnglib)
		return false;
#ifdef HAVE_SDL
	aPNG_set_acTL = hwSym("png_set_acTL", pnglib);
	aPNG_write_frame_head = hwSym("png_write_frame_head", pnglib);
	aPNG_write_frame_tail = hwSym("png_write_frame_tail", pnglib);
#endif
#ifdef _WIN32
	aPNG_set_acTL = GetProcAddress("png_set_acTL", pnglib);
	aPNG_write_frame_head = GetProcAddress("png_write_frame_head", pnglib);
	aPNG_write_frame_tail = GetProcAddress("png_write_frame_tail", pnglib);
#endif
	return (aPNG_set_acTL && aPNG_write_frame_head && aPNG_write_frame_tail);
#endif
}

static void M_PNGFrame(png_structp png_ptr, png_infop png_info_ptr, png_bytep png_buf)
{
	png_uint_32 pitch = png_get_rowbytes(png_ptr, png_info_ptr);
	PNG_CONST png_uint_32 height = vid.height;
	png_bytepp row_pointers = png_malloc(png_ptr, height* sizeof (png_bytep));
	png_uint_32 y;
	png_uint_16 framedelay = (png_uint_16)cv_apng_delay.value;

	apng_frames++;

	for (y = 0; y < height; y++)
	{
		row_pointers[y] = png_buf;
		png_buf += pitch;
	}

#ifndef PNG_STATIC
	if (aPNG_write_frame_head)
#endif
		aPNG_write_frame_head(apng_ptr, apng_info_ptr, row_pointers,
			vid.width, /* width */
			height,    /* height */
			0,         /* x offset */
			0,         /* y offset */
			framedelay, TICRATE,/* delay numerator and denominator */
			PNG_DISPOSE_OP_BACKGROUND, /* dispose */
			PNG_BLEND_OP_SOURCE        /* blend */
		                     );

	png_write_image(png_ptr, row_pointers);

#ifndef PNG_STATIC
	if (aPNG_write_frame_tail)
#endif
		aPNG_write_frame_tail(apng_ptr, apng_info_ptr);

	png_free(png_ptr, (png_voidp)row_pointers);
}

static void M_PNGfix_acTL(png_structp png_ptr, png_infop png_info_ptr,
		apng_infop png_ainfo_ptr)
{
	apng_set_acTL(png_ptr, png_info_ptr, png_ainfo_ptr, apng_frames, 0);

#ifndef NO_PNG_DEBUG
	png_debug(1, "in png_write_acTL\n");
#endif
}

static boolean M_SetupaPNG(png_const_charp filename, png_bytep pal)
{
	apng_FILE = fopen(filename,"wb+"); // + mode for reading
	if (!apng_FILE)
	{
		CONS_Printf("M_StartMovie: Error on opening %s for write\n", filename);
		return false;
	}

	apng_ptr = png_create_write_struct(PNG_LIBPNG_VER_STRING, NULL,
	 PNG_error, PNG_warn);
	if (!apng_ptr)
	{
		CONS_Printf("M_StartMovie: Error on initialize libpng\n");
		fclose(apng_FILE);
		remove(filename);
		return false;
	}

	apng_info_ptr = png_create_info_struct(apng_ptr);
	if (!apng_info_ptr)
	{
		CONS_Printf("M_StartMovie: Error on allocate for libpng\n");
		png_destroy_write_struct(&apng_ptr,  NULL);
		fclose(apng_FILE);
		remove(filename);
		return false;
	}

	apng_ainfo_ptr = apng_create_info_struct(apng_ptr);
	if (!apng_ainfo_ptr)
	{
		CONS_Printf("M_StartMovie: Error on allocate for apng\n");
		png_destroy_write_struct(&apng_ptr, &apng_info_ptr);
		fclose(apng_FILE);
		remove(filename);
		return false;
	}

	png_init_io(apng_ptr, apng_FILE);

#ifdef PNG_SET_USER_LIMITS_SUPPORTED
	png_set_user_limits(apng_ptr, MAXVIDWIDTH, MAXVIDHEIGHT);
#endif

	//png_set_filter(apng_ptr, 0, PNG_ALL_FILTERS);

	png_set_compression_level(apng_ptr, cv_zlib_levela.value);
	png_set_compression_mem_level(apng_ptr, cv_zlib_memorya.value);
	png_set_compression_strategy(apng_ptr, cv_zlib_strategya.value);
	png_set_compression_window_bits(apng_ptr, cv_zlib_window_bitsa.value);

	M_PNGhdr(apng_ptr, apng_info_ptr, vid.width, vid.height, pal);

	M_PNGText(apng_ptr, apng_info_ptr, true);

	apng_set_set_acTL_fn(apng_ptr, apng_ainfo_ptr, aPNG_set_acTL);

	apng_set_acTL(apng_ptr, apng_info_ptr, apng_ainfo_ptr, PNG_UINT_31_MAX, 0);

	apng_write_info(apng_ptr, apng_info_ptr, apng_ainfo_ptr);

	apng_frames = 0;

	return true;
}
#endif
#endif

// ==========================================================================
//                             MOVIE MODE
// ==========================================================================

#ifdef HAVE_MNG // SRB2 MNG Movies

static mng_handle MNG_ptr;

typedef struct MNG_user_struct
{
		FILE *fh;                 /* file handle */
		mng_palette8ep pal;        /* 256RGB pal*/
} MNG_userdata;
typedef MNG_userdata *MNG_userdatap;

static mng_ptr MNG_DECL MNG_memalloc(mng_size_t iLen)
{return (mng_ptr)calloc (1,iLen);}

static void MNG_DECL MNG_free(mng_ptr iPtr, mng_size_t iLen)
{ iLen = 0; free (iPtr);}

static mng_bool MNG_DECL MNG_open (mng_handle hHandle)
{ hHandle = NULL; return MNG_TRUE;}

static mng_bool MNG_DECL MNG_close (mng_handle hHandle)
{
	MNG_userdatap privdata = (MNG_userdatap)mng_get_userdata(hHandle);
	if (privdata->fh)
		fclose(privdata->fh);
	privdata->fh = 0;
	return MNG_TRUE;
}

static mng_bool MNG_DECL MNG_write (mng_handle  hHandle,
                                    mng_ptr     pBuf,
                                    mng_uint32  iBuflen,
                                    mng_uint32p pWritten)
{
  MNG_userdatap privdata = (MNG_userdatap)mng_get_userdata(hHandle);
  *pWritten = fwrite(pBuf, 1, iBuflen, privdata->fh);
  return MNG_TRUE;
}

static boolean M_SetupMNG(const mng_pchar filename, byte *pal)
{
	MNG_userdatap MNG_shared;
	mng_retcode ret;

	MNG_shared = malloc(sizeof (MNG_userdata));
	if (!MNG_shared)
	{
		return false;
	}

	MNG_shared->fh = fopen(filename,"wb");
	if (!MNG_shared->fh)
	{
		free(MNG_shared);
		return false;
	}
	MNG_shared->pal = (mng_palette8ep)pal;

	MNG_ptr = mng_initialize ((mng_ptr)MNG_shared, MNG_memalloc, MNG_free, MNG_NULL);
	if (!MNG_ptr)
	{
		fclose(MNG_shared->fh);
		free(MNG_shared);
		return false;
	}

	ret = mng_setcb_openstream  (MNG_ptr, MNG_open);
	ret = mng_setcb_closestream (MNG_ptr, MNG_close);
	ret = mng_setcb_writedata   (MNG_ptr, MNG_write);
	ret = mng_create(MNG_ptr);
	ret = mng_putchunk_mhdr(MNG_ptr, vid.width, vid.height, TICRATE, 0, 0, 0,
	      MNG_SIMPLICITY_VALID | MNG_SIMPLICITY_SIMPLEFEATURES);
	ret = mng_putchunk_term(MNG_ptr, MNG_TERMACTION_REPEAT,
	      MNG_ITERACTION_FIRSTFRAME, TICRATE, 0x7FFFFFFF);

	// iTXT

	if (pal)
	{
		ret = mng_putchunk_plte(MNG_ptr, 256, MNG_shared->pal);
	}

	ret = mng_putchunk_back(MNG_ptr, 0,0,0, 0, 0, MNG_BACKGROUNDIMAGE_NOTILE);
	ret = mng_putchunk_fram(MNG_ptr, MNG_FALSE, MNG_FRAMINGMODE_1, 0,MNG_NULL,
			MNG_CHANGEDELAY_DEFAULT, MNG_CHANGETIMOUT_NO, MNG_CHANGECLIPPING_NO, MNG_CHANGESYNCID_NO,
			100/TICRATE, 0,0,0,0,0,0, MNG_NULL,0);

	ret = mng_putchunk_defi(MNG_ptr, 0, MNG_DONOTSHOW_NOTVISIBLE, MNG_CONCRETE, MNG_FALSE, 0,0, MNG_FALSE, 0,0,0,0);

	return true;
}

boolean M_OpenMNG(const char *filename)
{
	boolean ret = false;

	if (rendermode == render_none)
		I_Error("Can't take a MNG movie without a render system");
	else
		freename = Newsnapshotfile(pathname,"mng");

	if (rendermode == render_soft)
		ret = M_SetupMNG(filename), W_CacheLumpName("PLAYPAL", PU_CACHE);
	else
		ret = M_SetupMNG(filename, NULL);

	return ret;
}

void MNG_frame(void)
{
	MNG_userdatap privdata;
	mng_retcode ret;
	mng_uint8 mColortype;
	if (!MNG_ptr) return;

	privdata = (MNG_userdatap)mng_get_userdata(MNG_ptr);

	if (privdata->pal)
	{
		mColortype = MNG_COLORTYPE_INDEXED;
	}
	else
	{
		mColortype = MNG_COLORTYPE_RGB;
	}

	ret = mng_putchunk_ihdr(MNG_ptr, vid.width, vid.height,
	      MNG_BITDEPTH_8, mColortype, MNG_COMPRESSION_DEFLATE,
		   MNG_FILTER_ADAPTIVE, MNG_INTERLACE_NONE);

	if (privdata->pal)
		ret = mng_putchunk_plte (MNG_ptr, 0, privdata->pal);

	ret = mng_putchunk_idat(MNG_ptr,0,NULL); //RAW?
	ret = mng_putchunk_iend(MNG_ptr);
}

boolean M_CloseMNG(void)
{
	mng_ptr privdata;
	if (!MNG_ptr) return false;

	privdata = mng_get_userdata(MNG_ptr);
	if (privdata)
		free(privdata);

	mng_putchunk_mend(MNG_ptr);
	mng_write(MNG_ptr);
	mng_cleanup(&MNG_ptr);

	CONS_Printf("MNG movie saved\n");
	return false;
}

#endif

#if NUMSCREENS > 2
static inline moviemode_t M_StartMovieAPNG(const char *pathname)
{
#ifdef USE_APNG
	UINT8 *palette = NULL;
	const char *freename = NULL;
	boolean ret = false;

	if (!M_PNGLib())
	{
		CONS_Printf("Couldn't create aPNG: libpng not found\n");
		return MM_OFF;
	}

	if (!(freename = Newsnapshotfile(pathname,"png")))
	{
		CONS_Printf("Couldn't create aPNG: no slots open in %s\n", pathname);
		return MM_OFF;
	}

	if (rendermode == render_soft)
	{
		M_CreateScreenShotPalette();
		palette = screenshot_palette;
	}

	ret = M_SetupaPNG(va(pandf,pathname,freename), palette);

	if (!ret)
	{
		CONS_Printf("ERROR: Couldn't create aPNG: error creating %s in %s\n", freename, pathname);
		return MM_OFF;
	}
	return MM_APNG;
#else
	// no APNG support exists
	(void)pathname;
	CONS_Printf("ERROR: Couldn't create aPNG: this build lacks aPNG support\n");
	return MM_OFF;
#endif
}

static inline moviemode_t M_StartMovieGIF(const char *pathname)
{
#ifdef HAVE_ANIGIF
	const char *freename;

	if (!(freename = Newsnapshotfile(pathname,"gif")))
	{
		CONS_Printf("ERROR: Couldn't create GIF: no slots open in %s\n", pathname);
		return MM_OFF;
	}

	if (!GIF_open(va(pandf,pathname,freename)))
	{
		CONS_Printf("ERROR: Couldn't create GIF: error creating %s in %s\n", freename, pathname);
		return MM_OFF;
	}
	return MM_GIF;
#else
	// no GIF support exists
	(void)pathname;
	CONS_Printf("ERROR: Couldn't create GIF: this build lacks GIF support\n");
	return MM_OFF;
#endif
}

static inline moviemode_t M_StartMovieMNG(const char *pathname)
{
#ifdef HAVE_MNG
	const char *freename;

	if (!(freename = Newsnapshotfile(pathname,"mng")))
	{
		CONS_Printf("ERROR: Couldn't create MNG: no slots open in %s\n", pathname);
		return MM_OFF;
	}

	if (!M_OpenMNG(va(pandf,pathname,freename)))
	{
		CONS_Printf("ERROR: Couldn't create MNG: error creating %s in %s\n", freename, pathname);
		return MM_OFF;
	}
	return MM_MNG;
#else
	// no MNG support exists
	(void)pathname;
	CONS_Printf("ERROR: Couldn't create MNG: this build lacks MNG support\n");
	return MM_OFF;
#endif
}
#endif

void M_StartMovie(void)
{
#if NUMSCREENS > 2
	char pathname[MAX_WADPATH];

	if (moviemode)
		return;

	if (cv_movie_option.value == 0)
		strcpy(pathname, usehome ? srb2home : srb2path);
	else if (cv_movie_option.value == 1)
		strcpy(pathname, srb2home);
	else if (cv_movie_option.value == 2)
		strcpy(pathname, srb2path);
	else if (cv_movie_option.value == 3 && *cv_movie_folder.string != '\0')
		strcpy(pathname, cv_movie_folder.string);

	if (cv_movie_option.value != 3)
	{
		strcat(pathname, PATHSEP"movies"PATHSEP);
		I_mkdir(pathname, 0755);
	}

	if (rendermode == render_none)
		I_Error("Can't make a movie without a render system\n");

	switch (cv_moviemode.value)
	{
		case MM_GIF:
			moviemode = M_StartMovieGIF(pathname);
			break;
		case MM_APNG:
			moviemode = M_StartMovieAPNG(pathname);
			break;
		case MM_SCREENSHOT:
			moviemode = MM_SCREENSHOT;
			break;
		case MM_MNG:
			moviemode = M_StartMovieMNG(pathname);
			break;
		default: //???
			return;
	}

	if (moviemode == MM_APNG)
		CONS_Printf(M_GetText("Movie mode enabled (%s).\n"), "aPNG");
	else if (moviemode == MM_GIF)
		CONS_Printf(M_GetText("Movie mode enabled (%s).\n"), "GIF");
	else if (moviemode == MM_SCREENSHOT)
		CONS_Printf(M_GetText("Movie mode enabled (%s).\n"), "screenshots");
	else if (moviemode == MM_MNG)
		CONS_Printf(M_GetText("Movie mode enabled (%s).\n"), "MNG");

	//singletics = (moviemode != MM_OFF);
#endif
}

void M_SaveFrame(void)
{
#if NUMSCREENS > 2
	// paranoia: should be unnecessary without singletics
	static tic_t oldtic = 0;

	if (oldtic == I_GetTime())
		return;
	else
		oldtic = I_GetTime();

	switch (moviemode)
	{
		case MM_SCREENSHOT:
			takescreenshot = true;
			return;
		case MM_GIF:
			GIF_frame();
			return;
		case MM_APNG:
#ifdef USE_APNG
			{
				UINT8 *linear = NULL;
				if (!apng_FILE) // should not happen!!
				{
					moviemode = MM_OFF;
					return;
				}

				if (rendermode == render_soft)
				{
					// munge planar buffer to linear
					linear = screens[2];
					I_ReadScreen(linear);
				}
#ifdef HWRENDER
				else
					linear = HWR_GetScreenshot();
#endif
				M_PNGFrame(apng_ptr, apng_info_ptr, (png_bytep)linear);
#ifdef HWRENDER
				if (rendermode != render_soft && linear)
					free(linear);
#endif

				if (apng_frames == PNG_UINT_31_MAX)
				{
					CONS_Printf(M_GetText("Max movie size reached\n"));
					M_StopMovie();
				}
			}
#else
			moviemode = MM_OFF;
#endif
			return;
		case MM_MNG:
#ifdef HAVE_MNG
			MNG_frame();
#else
			moviemode = MM_OFF;
#endif
		default:
			return;
	}
#endif
}

void M_StopMovie(void)
{
#if NUMSCREENS > 2
	switch (moviemode)
	{
		case MM_GIF:
			if (!GIF_close())
				return;
			break;
		case MM_APNG:
#ifdef USE_APNG
			if (!apng_FILE)
				return;

			if (apng_frames)
			{
				M_PNGfix_acTL(apng_ptr, apng_info_ptr, apng_ainfo_ptr);
				apng_write_end(apng_ptr, apng_info_ptr, apng_ainfo_ptr);
			}

			png_destroy_write_struct(&apng_ptr, &apng_info_ptr);

			fclose(apng_FILE);
			apng_FILE = NULL;
			CONS_Printf("aPNG closed; wrote %u frames\n", (UINT32)apng_frames);
			apng_frames = 0;
			break;
#else
			return;
#endif
		case MM_SCREENSHOT:
			break;
		case MM_MNG:
#ifdef HAVE_MNG
			if (!M_CloseMNG())
				return;
#else
			return;
#endif
		default:
			return;
	}
	moviemode = MM_OFF;
	CONS_Printf(M_GetText("Movie mode disabled.\n"));
#endif
}

// ==========================================================================
//                            SCREEN SHOTS
// ==========================================================================
#ifdef USE_PNG
/** Writes a PNG file to disk.
  *
  * \param filename Filename to write to.
  * \param data     The image data.
  * \param width    Width of the picture.
  * \param height   Height of the picture.
  * \param palette  Palette of image data.
  *  \note if palette is NULL, BGR888 format
  */
boolean M_SavePNG(const char *filename, void *data, int width, int height, const UINT8 *palette)
{
	png_structp png_ptr;
	png_infop png_info_ptr;
	PNG_CONST png_byte *PLTE = (const png_byte *)palette;
#ifdef PNG_SETJMP_SUPPORTED
#ifdef USE_FAR_KEYWORD
	jmp_buf jmpbuf;
#endif
#endif
	png_FILE_p png_FILE;

	png_FILE = fopen(filename,"wb");
	if (!png_FILE)
	{
		CONS_Printf("M_SavePNG: Error on opening %s for write\n", filename);
		return false;
	}

	png_ptr = png_create_write_struct(PNG_LIBPNG_VER_STRING, NULL, PNG_error, PNG_warn);
	if (!png_ptr)
	{
		CONS_Printf("M_SavePNG: Error on initialize libpng\n");
		fclose(png_FILE);
		remove(filename);
		return false;
	}

	png_info_ptr = png_create_info_struct(png_ptr);
	if (!png_info_ptr)
	{
		CONS_Printf("M_SavePNG: Error on allocate for libpng\n");
		png_destroy_write_struct(&png_ptr,  NULL);
		fclose(png_FILE);
		remove(filename);
		return false;
	}

#ifdef USE_FAR_KEYWORD
	if (setjmp(jmpbuf))
#else
	if (setjmp(png_jmpbuf(png_ptr)))
#endif
	{
		//CONS_Printf("libpng write error on %s\n", filename);
		png_destroy_write_struct(&png_ptr, &png_info_ptr);
		fclose(png_FILE);
		remove(filename);
		return false;
	}
#ifdef USE_FAR_KEYWORD
	png_memcpy(png_jmpbuf(png_ptr),jmpbuf, sizeof (jmp_buf));
#endif
	png_init_io(png_ptr, png_FILE);

#ifdef PNG_SET_USER_LIMITS_SUPPORTED
	png_set_user_limits(png_ptr, MAXVIDWIDTH, MAXVIDHEIGHT);
#endif

	//png_set_filter(png_ptr, 0, PNG_ALL_FILTERS);

	png_set_compression_level(png_ptr, cv_zlib_level.value);
	png_set_compression_mem_level(png_ptr, cv_zlib_memory.value);
	png_set_compression_strategy(png_ptr, cv_zlib_strategy.value);
	png_set_compression_window_bits(png_ptr, cv_zlib_window_bits.value);

	M_PNGhdr(png_ptr, png_info_ptr, width, height, PLTE);

	M_PNGText(png_ptr, png_info_ptr, false);

	png_write_info(png_ptr, png_info_ptr);

	M_PNGImage(png_ptr, png_info_ptr, height, data);

	png_write_end(png_ptr, png_info_ptr);
	png_destroy_write_struct(&png_ptr, &png_info_ptr);

	fclose(png_FILE);
	return true;
}
#else
/** PCX file structure.
  */
typedef struct
{
	UINT8 manufacturer;
	UINT8 version;
	UINT8 encoding;
	UINT8 bits_per_pixel;

	UINT16 xmin, ymin;
	UINT16 xmax, ymax;
	UINT16 hres, vres;
	UINT8  palette[48];

	UINT8 reserved;
	UINT8 color_planes;
	UINT16 bytes_per_line;
	UINT16 palette_type;

	char filler[58];
	UINT8 data; ///< Unbounded; used for all picture data.
} pcx_t;

/** Writes a PCX file to disk.
  *
  * \param filename Filename to write to.
  * \param data     The image data.
  * \param width    Width of the picture.
  * \param height   Height of the picture.
  * \param palette  Palette of image data
  */
#if NUMSCREENS > 2
static boolean WritePCXfile(const char *filename, const UINT8 *data, int width, int height, const UINT8 *pal)
{
	int i;
	size_t length;
	pcx_t *pcx;
	UINT8 *pack;

	pcx = Z_Malloc(width*height*2 + 1000, PU_STATIC, NULL);

	pcx->manufacturer = 0x0a; // PCX id
	pcx->version = 5; // 256 color
	pcx->encoding = 1; // uncompressed
	pcx->bits_per_pixel = 8; // 256 color
	pcx->xmin = pcx->ymin = 0;
	pcx->xmax = SHORT(width - 1);
	pcx->ymax = SHORT(height - 1);
	pcx->hres = SHORT(width);
	pcx->vres = SHORT(height);
	memset(pcx->palette, 0, sizeof (pcx->palette));
	pcx->reserved = 0;
	pcx->color_planes = 1; // chunky image
	pcx->bytes_per_line = SHORT(width);
	pcx->palette_type = SHORT(1); // not a grey scale
	memset(pcx->filler, 0, sizeof (pcx->filler));

	// pack the image
	pack = &pcx->data;

	for (i = 0; i < width*height; i++)
	{
		if ((*data & 0xc0) != 0xc0)
			*pack++ = *data++;
		else
		{
			*pack++ = 0xc1;
			*pack++ = *data++;
		}
	}

	// write the palette
	*pack++ = 0x0c; // palette ID byte

	// write color table
	{
		for (i = 0; i < 256; i++)
		{
			*pack++ = *pal; pal++;
			*pack++ = *pal; pal++;
			*pack++ = *pal; pal++;
		}
	}

	// write output file
	length = pack - (UINT8 *)pcx;
	i = FIL_WriteFile(filename, pcx, length);

	Z_Free(pcx);
	return i;
}
#endif
#endif

void M_ScreenShot(void)
{
	takescreenshot = true;
}

/** Takes a screenshot.
  * The screenshot is saved as "srb2xxxx.png" where xxxx is the lowest
  * four-digit number for which a file does not already exist.
  *
  * \sa HWR_ScreenShot
  */
void M_DoScreenShot(void)
{
#if NUMSCREENS > 2
	const char *freename = NULL;
	char pathname[MAX_WADPATH];
	boolean ret = false;
	UINT8 *linear = NULL;

	// Don't take multiple screenshots, obviously
	takescreenshot = false;

	// how does one take a screenshot without a render system?
	if (rendermode == render_none)
		return;

	if (cv_screenshot_option.value == 0)
		strcpy(pathname, usehome ? srb2home : srb2path);
	else if (cv_screenshot_option.value == 1)
		strcpy(pathname, srb2home);
	else if (cv_screenshot_option.value == 2)
		strcpy(pathname, srb2path);
	else if (cv_screenshot_option.value == 3 && *cv_screenshot_folder.string != '\0')
		strcpy(pathname, cv_screenshot_folder.string);

	if (cv_screenshot_option.value != 3)
	{
		strcat(pathname, PATHSEP"screenshots"PATHSEP);
		I_mkdir(pathname, 0755);
	}

#ifdef USE_PNG
	freename = Newsnapshotfile(pathname,"png");
#else
	if (rendermode == render_soft)
		freename = Newsnapshotfile(pathname,"pcx");
	else if (rendermode == render_opengl)
		freename = Newsnapshotfile(pathname,"tga");
#endif

	if (rendermode == render_soft)
	{
		// munge planar buffer to linear
		linear = screens[2];
		I_ReadScreen(linear);
	}

	if (!freename)
		goto failure;

	// save the pcx file
#ifdef HWRENDER
	if (rendermode == render_opengl)
		ret = HWR_Screenshot(va(pandf,pathname,freename));
	else
#endif
	{
		M_CreateScreenShotPalette();
#ifdef USE_PNG
		ret = M_SavePNG(va(pandf,pathname,freename), linear, vid.width, vid.height, screenshot_palette);
#else
		ret = WritePCXfile(va(pandf,pathname,freename), linear, vid.width, vid.height, screenshot_palette);
#endif
	}

failure:
	if (ret)
	{
		if (moviemode != MM_SCREENSHOT)
			CONS_Printf(M_GetText("Screen shot %s saved in %s\n"), freename, pathname);
	}
	else
	{
		if (freename)
			CONS_Printf(M_GetText("Couldn't create screen shot %s in %s\n"), freename, pathname);
		else
			CONS_Printf(M_GetText("Couldn't create screen shot in %s (all 10000 slots used!)\n"), pathname);

		if (moviemode == MM_SCREENSHOT)
			M_StopMovie();
	}
#endif
}

boolean M_ScreenshotResponder(event_t *ev)
{
	INT32 ch = -1;
	if (dedicated || ev->type != ev_keydown)
		return false;

	ch = ev->data1;

	if (ch == KEY_F8)
		M_ScreenShot();
	else if (ch == KEY_F9)
		((moviemode) ? M_StopMovie : M_StartMovie)();
	else
		return false;
	return true;
}

// ==========================================================================
//                        MISC STRING FUNCTIONS
// ==========================================================================

/** Returns a temporary string made out of varargs.
  * For use with CONS_Printf().
  *
  * \param format Format string.
  * \return Pointer to a static buffer of 1024 characters, containing the
  *         resulting string.
  */
char *va(const char *format, ...)
{
	va_list argptr;
	static char string[1024];

	va_start(argptr, format);
	vsprintf(string, format, argptr);
	va_end(argptr);

	return string;
}

/** Creates a string in the first argument that is the second argument followed
  * by the third argument followed by the first argument.
  * Useful for making filenames with full path. s1 = s2+s3+s1
  *
  * \param s1 First string, suffix, and destination.
  * \param s2 Second string. Ends up first in the result.
  * \param s3 Third string. Ends up second in the result.
  */
void strcatbf(char *s1, const char *s2, const char *s3)
{
	char tmp[1024];

	strcpy(tmp, s1);
	strcpy(s1, s2);
	strcat(s1, s3);
	strcat(s1, tmp);
}
