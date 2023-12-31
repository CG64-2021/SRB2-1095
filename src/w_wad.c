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
/// \brief Handles WAD file header, directory, lump I/O

#ifndef SDLIO
#ifndef _WIN32_WCE
#include <fcntl.h>
#endif
#endif
#ifdef __GNUC__
#include <unistd.h>
#endif

#include "doomdef.h"
#include "doomtype.h"
#include "w_wad.h"
#include "z_zone.h"

#include "i_video.h" // rendermode
#include "d_netfil.h"
#include "dehacked.h"
#include "r_defs.h"
#include "i_system.h"
#include "md5.h"

#ifdef HWRENDER
#include "r_data.h"
#include "hardware/hw_main.h"
#include "hardware/hw_glob.h"
#endif

#ifndef O_BINARY
#define O_BINARY 0
#endif

//===========================================================================
//                                                                    GLOBALS
//===========================================================================
int numwadfiles; // number of active wadfiles
wadfile_t *wadfiles[MAX_WADFILES]; // 0 to numwadfiles-1 are valid

// W_Shutdown
// Closes all of the WAD files before quitting
// If not done on a Mac then open wad files
// can prevent removable media they are on from
// being ejected
void W_Shutdown(void)
{
	while (numwadfiles--)
	{
#ifdef SDLIO
		SDL_RWclose(wadfiles[numwadfiles]->handle);
#else
		close(wadfiles[numwadfiles]->handle);
#endif
	}
}

//===========================================================================
//                                                        LUMP BASED ROUTINES
//===========================================================================

// W_AddFile
// All files are optional, but at least one file must be
//  found (PWAD, if all required lumps are present).
// Files with a .wad extension are wadlink files
//  with multiple lumps.
// Other files are single lumps with the base filename
//  for the lump name.

static char filenamebuf[MAX_WADPATH];

// search for all DEHACKED lump in all wads and load it
static inline void W_LoadDehackedLumps(int wadnum)
{
	int clump = 0;

	// Check for MAINCFG
	for (;;)
	{
		clump = W_CheckNumForNamePwad("MAINCFG", wadnum, clump);
		if (clump == -1)
			break;
		CONS_Printf("Loading main config from %s\n", wadfiles[wadnum]->filename);
		DEH_LoadDehackedLump(clump);
		clump++;
	}

	// Check for OBJCTCFG
	for (;;)
	{
		clump = W_CheckNumForNamePwad("OBJCTCFG", wadnum, clump);
		if (clump == -1)
			break;
		CONS_Printf("Loading object config from %s\n", wadfiles[wadnum]->filename);
		DEH_LoadDehackedLump(clump);
		clump++;
	}
}

//  Allocate a wadfile, setup the lumpinfo (directory) and
//  lumpcache, add the wadfile to the current active wadfiles
//
//  now returns index into wadfiles[], you can get wadfile_t *
//  with:
//       wadfiles[<return value>]
//
//  return -1 in case of problem
//
// Can now load dehacked files (.soc)
//
int W_LoadWadFile(char *filename)
{
#ifdef SDLIO
	SDL_RWops *handle;
#else
	int handle;
#endif
	lumpinfo_t *lumpinfo;
	lumpcache_t *lumpcache;
	wadfile_t *wadfile;
	int numlumps, i, length;
#ifdef HWRENDER
	GlidePatch_t *grPatch;
#endif

	//CONS_Printf("Loading %s\n", filename);
	//
	// check if limit of active wadfiles
	//
	if (numwadfiles >= MAX_WADFILES)
	{
		CONS_Printf("Maximum wad files reached\n");
		return -1;
	}

	strncpy(filenamebuf, filename, MAX_WADPATH);
	filename = filenamebuf;
	// open wad file
#ifdef SDLIO
	if (NULL == (handle = SDL_RWFromFile(filename, "rb")))
#else
	if ((handle = open(filename, O_RDONLY|O_BINARY, 0666)) == -1)
#endif
	{
		nameonly(filename); // leave full path here
		if (findfile(filename, NULL, true))
		{
#ifdef SDLIO
			if (NULL == (handle = SDL_RWFromFile(filename, "rb")))
#else
			if ((handle = open(filename, O_RDONLY|O_BINARY, 0666)) == -1)
#endif
			{
				CONS_Printf("Can't open %s\n", filename);
				return -1;
			}
		}
		else
		{
			CONS_Printf("File %s not found.\n", filename);
			return -1;
		}
	}

	// detect dehacked file with the "soc" extension
	if (!stricmp(&filename[strlen(filename) - 4], ".soc"))
	{
		// This code emulates a wadfile with one lump name "OBJCTCFG"
		// at position 0 and size of the whole file.
		// This allows soc files to be like all wads, copied by network and loaded at the console.
		numlumps = 1;
		lumpinfo = Z_Malloc(sizeof (lumpinfo_t), PU_STATIC, NULL);
		lumpinfo->position = 0;
		lumpinfo->size = (int)I_FileSize(filename);
		strncpy(lumpinfo->name, "OBJCTCFG", 8);
	}
	else
	{
		// assume wad file
		wadinfo_t header;
		lumpinfo_t *lump_p;
		filelump_t *fileinfo;
		void *fileinfov;

		// read the header
#ifdef SDLIO
		if (SDL_RWread(handle, &header, 1, sizeof (header)) < sizeof (header))
#else
		if (read(handle, &header, sizeof (header)) < (int)sizeof (header))
#endif
		{
			CONS_Printf("Can't read wad header from %s\n", filename);
			return -1;
		}

		if (strncmp(header.identification, "IWAD", 4)
			&& strncmp(header.identification, "PWAD", 4)
			&& strncmp(header.identification, "SDLL", 4))
		{
			CONS_Printf("%s doesn't have IWAD or PWAD id\n", filename);
			return -1;
		}
		header.numlumps = LONG(header.numlumps);
		header.infotableofs = LONG(header.infotableofs);

		// read wad file directory
		length = header.numlumps * sizeof (filelump_t);
		fileinfov = fileinfo = malloc(length);
#ifdef SDLIO
		if (SDL_RWseek(handle, header.infotableofs, SEEK_SET) == -1
			|| SDL_RWread(handle, fileinfo, 1, length) < length)
#else
		if (lseek(handle, header.infotableofs, SEEK_SET) == -1
			|| read(handle, fileinfo, length) < length)
#endif
		{
			CONS_Printf("%s wadfile directory is corrupt\n", filename);
			free(fileinfov);
			return -1;
		}

		numlumps = header.numlumps;

		// fill in lumpinfo for this wad
		lump_p = lumpinfo = Z_Malloc(numlumps * sizeof (lumpinfo_t), PU_STATIC, NULL);
		for (i = 0; i < numlumps; i++, lump_p++, fileinfo++)
		{
			lump_p->position = LONG(fileinfo->filepos);
			lump_p->size = LONG(fileinfo->size);
			strncpy(lump_p->name, fileinfo->name, 8);
		}
		free(fileinfov);
	}
	//
	// link wad file to search files
	//
	wadfile = Z_Malloc(sizeof (wadfile_t), PU_STATIC, NULL);
	wadfile->filename = strdup(filename);
	wadfile->handle = handle;
	wadfile->numlumps = numlumps;
	wadfile->lumpinfo = lumpinfo;
	wadfile->filesize = (int)I_FileSize(filename);

	//
	// generate md5sum
	//
	W_MakeFileMD5(filename, wadfile->md5sum);

	//
	// set up caching
	//
	length = numlumps * sizeof (lumpcache_t);
	lumpcache = Z_Malloc(length, PU_STATIC, NULL);

	memset(lumpcache, 0, length);
	wadfile->lumpcache = lumpcache;

#ifdef HWRENDER
	// allocates GlidePatch info structures STATIC from the start,
	// because these were causing a lot of fragmentation of the heap,
	// considering they are never freed.
	length = numlumps * sizeof (GlidePatch_t);
	grPatch = Z_Malloc(length, PU_HWRPATCHINFO, (void**)&wadfile->hwrcache); // never freed
	// set mipmap.downloaded to false
	memset(grPatch, 0, length);
	for (i = 0; i < numlumps; i++)
	{
		// store the software patch lump number for each GlidePatch
		grPatch[i].patchlump = (numwadfiles<<16) + i;
	}
#endif

	//
	// add the wadfile
	//
	wadfiles[numwadfiles++] = wadfile;

	CONS_Printf("Added file %s (%d lumps)\n", filename, numlumps);
	W_LoadDehackedLumps(numwadfiles - 1);
	return numwadfiles - 1;
}

void W_UnloadWadFile(int num)
{
	int i;
	wadfile_t *delwad = wadfiles[num];
	lumpcache_t *lumpcache;
	if (num == 0)
		CONS_Printf("You can't remove the IWAD %s!\n", wadfiles[0]->filename);
	else
		CONS_Printf("Removing WAD %s...", wadfiles[num]->filename);

	wadfiles[num] = NULL;
	lumpcache = delwad->lumpcache;
	numwadfiles--;
#ifdef HWRENDER
	if (rendermode != render_soft && rendermode != render_none)
	{
		HWR_PrepLevelCache(numtextures);
		HWR_CreateStaticLightmaps(numnodes - 1);
	}	
	Z_Free(delwad->hwrcache);
#endif
	if (*lumpcache)
	{
		for (i = 0;i < delwad->numlumps;i++)
			Z_ChangeTag(lumpcache[i],PU_LEVEL);
	}
	else
		Z_Free(lumpcache);
#ifdef SDLIO
	SDL_RWclose(delwad->handle);
#else
	close(delwad->handle);
#endif
	Z_Free(delwad->filename);
	Z_Free(delwad);
	CONS_Printf("done\n");
}

/** Compute MD5 message digest for bytes read from STREAM of this filname.
  *
  * The resulting message digest number will be written into the 16 bytes
  * beginning at RESBLOCK.  
  *
  * \param filename path of file
  * \param resblock resulting MD5 checksum 
  * \return 0 if MD5 checksum was made, and is at resblock, 1 if error was found
  */
int W_MakeFileMD5(const char *filename, void *resblock)
{
	FILE *fhandle;

	if ((fhandle = fopen(filename, "rb")) != NULL)
	{
#ifndef NOMD5
		int t = I_GetTime();
#ifdef _arch_dreamcast
		CONS_Printf("Making MD5 for %s\n",wadfile->filename);
#endif
		if (md5_stream(fhandle, resblock) == 1)
			return 1;
#ifndef _arch_dreamcast
		if (devparm)
#endif
		CONS_Printf("MD5 calc for %s took %f second\n",
			filename, (float)(I_GetTime() - t)/TICRATE);
#else
		memset(resblock, 0, 16);
#endif
		fclose(fhandle);
		return 0;
	}
	memset(resblock, 0, 16);
	return 1;
		
}

/** Tries to load a series of files.
  * All files are wads unless they have an extension of ".soc".
  *
  * Each file is optional, but at least one file must be found or an error will
  * result. Lump names can appear multiple times. The name searcher looks
  * backwards, so a later file overrides all earlier ones.
  *
  * \param filenames A null-terminated list of files to use.
  * \return 1 if all files were loaded, 0 if at least one was missing or
  *           invalid.
  */
int W_InitMultipleFiles(char **filenames)
{
	int rc = 1;

	// open all the files, load headers, and count lumps
	numwadfiles = 0;

	// will be realloced as lumps are added
	for (; *filenames; filenames++)
	{
		//CONS_Printf("Loading %s\n", *filenames);
		rc &= (W_LoadWadFile(*filenames) != -1) ? 1 : 0;
	}

	if (!numwadfiles)
		I_Error("W_InitMultipleFiles: no files found");

	return rc;
}

const char *W_CheckNameForNumPwad(int wadid, int lumpnum)
{
	const int lumpnuml = lumpnum & 0xFFFF;

	if (lumpnum < 0 || wadid < 0 || lumpnuml > lumpnum)
		return NULL;

	if (lumpnuml >= wadfiles[wadid]->numlumps)
		return NULL;

	return wadfiles[wadid]->lumpinfo[lumpnuml].name;
}

const char *W_CheckNameForNum(int lumpnum)
{
	return W_CheckNameForNumPwad(lumpnum>>16,lumpnum&0xFFFF);
}

//
// W_CheckNumForName
// Returns -1 if name not found.
//

int W_CheckNumForName(const char *name)
{
	union
	{
		char s[9];
		int x[2];
	} name8;
	int i, j, v1, v2;
	lumpinfo_t *lump_p;

	// make the name into two integers for easy compares
	strncpy(name8.s, name, 8);

	// in case the name was a full 8 chars
	name8.s[8] = 0;

	// case insensitive
	strupr(name8.s);

	v1 = name8.x[0];
	v2 = name8.x[1];

	// scan wad files backwards so patch lump files take precedence
	//
	for (i = numwadfiles - 1; i >= 0; i--)
	{
		lump_p = wadfiles[i]->lumpinfo;

		for (j = 0; j < wadfiles[i]->numlumps; j++, lump_p++)
		{
			if (*(int *)lump_p->name == v1 && *(int *)&lump_p->name[4] == v2)
			{
				// high word is the wad file number
				return (i<<16) + j;
			}
		}
	}

	// not found.
	return -1;
}

//
// Same as the original, but checks in one pwad only.
// wadid is a wad number
// (Used for sprites loading)
//
// 'startlump' is the lump number to start the search
//
int W_CheckNumForNamePwad(const char *name, int wadid, int startlump)
{
	union
	{
		char s[9];
		int x[2];
	} name8;

	int i, v1, v2;
	lumpinfo_t *lump_p;

	strncpy(name8.s, name, 8);
	name8.s[8] = 0;
	strupr(name8.s);

	v1 = name8.x[0];
	v2 = name8.x[1];

	//
	// scan forward
	// start at 'startlump', useful parameter when there are multiple
	//                       resources with the same name
	//
	if (startlump < wadfiles[wadid]->numlumps)
	{
		lump_p = wadfiles[wadid]->lumpinfo + startlump;
		for (i = startlump; i < wadfiles[wadid]->numlumps; i++, lump_p++)
		{
			if (*(int *)lump_p->name == v1 && *(int *)&lump_p->name[4] == v2)
				return (wadid<<16) + i;
		}
	}

	// not found.
	return -1;
}

//
// W_GetNumForName
//
// Calls W_CheckNumForName, but bombs out if not found.
//
int W_GetNumForName(const char *name)
{
	int i;

	i = W_CheckNumForName(name);

	if (i == -1)
		I_Error("W_GetNumForName: %s not found!\n", name);

	return i;
}

/** Make sure a lump number is valid.
  * Compiles away to nothing if PARANOIA is not defined.
  */
static void TestValidLump(int lump)
{
	// make sure the lump exists, if debugging
	I_Assert(lump >= 0);
	I_Assert((lump & 0xffff) < wadfiles[lump>>16]->numlumps);
	lump = 0;
}

/** Returns the buffer size needed to load the given lump.
  *
  * \param lump Lump number to look at.
  * \return Buffer size needed, in bytes.
  */
int W_LumpLength(int lump)
{
	TestValidLump(lump);
	return wadfiles[lump>>16]->lumpinfo[lump&0xFFFF].size;
}

/** Reads bytes from the head of a lump.
  *
  * \param lump Lump number to read from.
  * \param dest Buffer in memory to serve as destination.
  * \param size Number of bytes to read.
  * \return Number of bytes read (should equal size).
  * \sa W_ReadLump
  */
int W_ReadLumpHeader(int lump, void *dest, int size)
{
	int bytesread;
	lumpinfo_t *l;
#ifdef SDLIO
	SDL_RWops *handle;
#else
	int handle;
#endif

	TestValidLump(lump);
	l = wadfiles[lump>>16]->lumpinfo + (lump & 0xFFFF);

	// empty resource (usually markers like S_START, F_END ..)
	if (!l->size)
		return 0;

	handle = wadfiles[lump>>16]->handle;

	// zero size means read all the lump
	if (!size || size > l->size)
		size = l->size;

#ifdef SDLIO
	SDL_RWseek(handle, l->position, SEEK_SET);
	bytesread = SDL_RWread(handle, dest, 1, size);
#else
	lseek(handle, l->position, SEEK_SET);
	bytesread = read(handle, dest, size);
#endif

	return bytesread;
}

/** Reads a lump into memory.
  *
  * \param lump Lump number to read from.
  * \param dest Buffer in memory to serve as destination. Size must be >=
  *             W_LumpLength().
  * \sa W_ReadLumpHeader
  */
void W_ReadLump(int lump, void *dest)
{
	W_ReadLumpHeader(lump, dest, 0);
}

// ==========================================================================
// W_CacheLumpNum
// ==========================================================================
void *W_CacheLumpNum(int lump, int tag)
{
	byte *ptr;
	lumpcache_t *lumpcache;

	// Don't keep doing operations to the lump variable!
	const int llump = lump & 0xFFFF;
	const int lfile = lump >> 16;

	// check return value of a previous W_CheckNumForName()
	I_Assert(lfile < numwadfiles);
	I_Assert(llump < wadfiles[lfile]->numlumps);
	I_Assert(lump >= 0);

	lumpcache = wadfiles[lfile]->lumpcache;
	if (!lumpcache[llump])
	{
		// read the lump in
		ptr = Z_Malloc(W_LumpLength(lump), tag, &lumpcache[llump]);
		W_ReadLump(lump, lumpcache[llump]); // read full
	}
	else
		Z_ChangeTag(lumpcache[llump], tag);

	return lumpcache[llump];
}

// ==========================================================================
// W_CacheLumpName
// ==========================================================================
void *W_CacheLumpName(const char *name, int tag)
{
	return W_CacheLumpNum(W_GetNumForName(name), tag);
}

// ==========================================================================
//                                         CACHING OF GRAPHIC PATCH RESOURCES
// ==========================================================================

// Graphic 'patches' are loaded, and if necessary, converted into the format
// the most useful for the current rendermode. For software renderer, the
// graphic patches are kept as is. For the hardware renderer, graphic patches
// are 'unpacked', and are kept into the cache in that unpacked format, and
// the heap memory cache then acts as a 'level 2' cache just after the
// graphics card memory.

//
// Cache a patch into heap memory, convert the patch format as necessary
//

// Software-only compile cache the data without conversion
#ifdef HWRENDER

void *W_CachePatchNum(int lump, int tag)
{
	GlidePatch_t *grPatch;

	if (rendermode == render_soft || rendermode == render_none)
		return W_CacheLumpNum(lump, tag);

	// check the return value of a previous W_CheckNumForName()
	I_Assert(lump >= 0);
	I_Assert((lump & 0xffff) < wadfiles[lump>>16]->numlumps);

	grPatch = &(wadfiles[lump>>16]->hwrcache[lump & 0xffff]);

	if (grPatch->mipmap.grInfo.data)
	{
		if (tag == PU_CACHE)
			tag = PU_HWRCACHE;
		Z_ChangeTag(grPatch->mipmap.grInfo.data, tag);
	}
	else
	{
		// first time init grPatch fields
		// we need patch w,h,offset,...
		// this code will be executed latter in GetPatch, anyway
		// do it now
		patch_t *ptr = W_CacheLumpNum(grPatch->patchlump, PU_STATIC);
		HWR_MakePatch(ptr, grPatch, &grPatch->mipmap);
		Z_Free(ptr);
	}

	// return GlidePatch_t, which can be casted to (patch_t) with valid patch header info
	return (void *)grPatch;
}

#endif // HWRENDER Glide version

void *W_CachePatchName(const char *name, int tag)
{
	int num;

	num = W_CheckNumForName(name);

	if (num < 0)
		return W_CachePatchNum(W_GetNumForName("BRDR_MM"), tag);
	return W_CachePatchNum(num, tag);
}

#define MD5_LEN 16

/**
  * Prints an MD5 string into a human-readable textual format.
  *
  * \param md5 The md5 in binary form -- MD5_LEN (16) bytes.
  * \param buf Where to print the textual form. Needs 2*MD5_LEN+1 (33) bytes.
  * \author Graue <graue@oceanbase.org>
  */
#define MD5_FORMAT \
	"%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x"
static void PrintMD5String(const unsigned char *md5, char *buf)
{
	snprintf(buf, 2*MD5_LEN+1, MD5_FORMAT,
		md5[0], md5[1], md5[2], md5[3],
		md5[4], md5[5], md5[6], md5[7],
		md5[8], md5[9], md5[10], md5[11],
		md5[12], md5[13], md5[14], md5[15]);
}

/** Verifies a file's MD5 is as it should be.
  * For releases, used as cheat prevention -- if the MD5 doesn't match, a
  * fatal error is thrown. In debug mode, an MD5 mismatch only triggers a
  * warning.
  *
  * \param wadfilenum Number of the loaded wad file to check.
  * \param matchmd5   The MD5 sum this wad should have, expressed as a
  *                   textual string.
  * \author Graue <graue@oceanbase.org>
  */
void W_VerifyFileMD5(int wadfilenum, const char *matchmd5)
{
#ifndef NOMD5
	unsigned char realmd5[MD5_LEN];
	int ix;
#endif

	I_Assert(wadfilenum < numwadfiles);
	I_Assert(wadfilenum >= 0);
	I_Assert(strlen(matchmd5) == 2*MD5_LEN);
#ifdef NOMD5
	// unused parameters
	(void)wadfilenum;
	(void)matchmd5;
#else
	// Convert an md5 string like "7d355827fa8f981482246d6c95f9bd48"
	// into a real md5.
	for (ix = 0; ix < 2*MD5_LEN; ix++)
	{
		int n, c = matchmd5[ix];
		if (isdigit(c))
			n = c - '0';
		else
		{
			I_Assert(isxdigit(c));
			if (isupper(c)) n = c - 'A' + 10;
			else n = c - 'a' + 10;
		}
		if (ix & 1) realmd5[ix>>1] = (unsigned char)(realmd5[ix>>1]+n);
		else realmd5[ix>>1] = (unsigned char)(n<<4);
	}

	if (memcmp(realmd5, wadfiles[wadfilenum]->md5sum, 16))
	{
		char actualmd5text[2*MD5_LEN+1];
		PrintMD5String(wadfiles[wadfilenum]->md5sum, actualmd5text);
#ifdef _DEBUG
		CONS_Printf
#else
		I_Error
#endif
			("File is corrupt or has been modified: %s "
			"(found md5: %s, wanted: %s)\n",
			wadfiles[wadfilenum]->filename,
			actualmd5text, matchmd5);
	}
#endif
}

int W_VerifyFile(const char *filename, lumpchecklist_t *checklist, boolean status)
{
#ifdef SDLIO
	SDL_RWops *handle;
#else
	int handle;
#endif
	size_t j;
	int numlumps, i, length, goodfile = false;

	if (!checklist) I_Error("No checklist for %s\n", filename);
	strncpy(filenamebuf, filename, MAX_WADPATH);
	filename = filenamebuf;
	// open wad file
#ifdef SDLIO
	if (NULL == (handle = SDL_RWFromFile(filename,"rb")))
#else
	if ((handle = open(filename, O_RDONLY|O_BINARY, 0666)) == -1)
#endif
	{
		nameonly(filenamebuf); // leave full path here
		if (findfile(filenamebuf, NULL, true))
		{
#ifdef SDLIO
			if (NULL == (handle = SDL_RWFromFile(filename, "rb")))
#else
			if ((handle = open(filename, O_RDONLY|O_BINARY, 0666)) == -1)
#endif
				goodfile = true;
		}
		else
			goodfile = true;
	}

	// detect dehacked file with the "soc" extension
	if (!stricmp(&filename[strlen(filename) - 4], ".soc"))
	{
		goodfile = false; // no way!
	}
	else if (!goodfile)
	{
		// assume wad file
		wadinfo_t header;
		lumpinfo_t lump_p;
		filelump_t *fileinfo;
		void *fileinfov;

		// read the header
#ifdef SDLIO
		SDL_RWread(handle, &header, 1, sizeof (header));
#else
		read(handle, &header, sizeof (header));
#endif
		if (strncmp(header.identification, "IWAD", 4)
			&& strncmp(header.identification, "PWAD", 4)
			&& strncmp(header.identification, "SDLL", 4))
		{
#ifdef SDLIO
			SDL_RWclose(handle);
#else
			close(handle);
#endif
			return true;
		}
		header.numlumps = LONG(header.numlumps);
		header.infotableofs = LONG(header.infotableofs);

		// read wad file directory
		length = header.numlumps*sizeof (filelump_t);
		fileinfov = fileinfo = malloc(length);
#ifdef SDLIO
		SDL_RWseek(handle, header.infotableofs, SEEK_SET);
		SDL_RWread(handle, fileinfo, 1, length);
#else
		lseek(handle, header.infotableofs, SEEK_SET);
		read(handle, fileinfo, length);
#endif
		numlumps = header.numlumps;

		// fill in lumpinfo for this wad
		memset(&lump_p, 0, sizeof (lumpinfo_t));
		goodfile = true;
		for (i = 0; i < numlumps; i++, fileinfo++)
		{
			lump_p.position = LONG(fileinfo->filepos);
			lump_p.size = LONG(fileinfo->size);
			strncpy(lump_p.name, fileinfo->name, 8);
			goodfile = false;
			for (j = 0; checklist[j].len && checklist[j].name && !goodfile; j++)
				if ((strncmp(lump_p.name, checklist[j].name, checklist[j].len) != false) == status)
					goodfile = true;

			if (!goodfile)
				break;
		}
		free(fileinfov);
	}
#ifdef SDLIO
	SDL_RWclose(handle);
#else
	close(handle);
#endif
	return goodfile;
}


/** Checks a wad for lumps other than music and sound.
  * Used during game load to verify music.dta is a good file and during a
  * netgame join (on the server side) to see if a wad is important enough to
  * be sent.
  *
  * \param filename Filename of the wad to check.
  * \return 1 if file contains only music/sound lumps, 0 if it contains other
  *         stuff (maps, sprites, dehacked lumps, and so on).
  * \author Alam Arias
  */
int W_VerifyNMUSlumps(const char *filename)
{
	// MIDI, MOD/S3M/IT/XM/OGG/MP3/WAV, WAVE SFX and ENDOOM
	lumpchecklist_t NMUSlist[] =
	{
		{"D_", 2},
		{"O_", 2},
		{"DS", 2},
		{"ENDOOM", 6},
		{NULL, 0},
	};
	return W_VerifyFile(filename, NMUSlist, false);
}
