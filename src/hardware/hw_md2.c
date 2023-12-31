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
//-----------------------------------------------------------------------------
/// \file
/// \brief MD2 Handling
///	Inspired from md2.c by Mete Ciragan (mete@swissquake.ch)


#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

#ifdef HAVE_PNG
 #include <png.h>
 #ifndef PNG_READ_SUPPORTED
 #undef HAVE_PNG
 #endif
 #if PNG_LIBPNG_VER < 100207
 //#undef HAVE_PNG
 #endif
#endif

#include "hw_drv.h"
#include "hw_light.h"
#include "hw_md2.h"

#include "../doomdef.h"
#include "../r_main.h"
#include "../m_misc.h"
#include "../w_wad.h"
#include "../z_zone.h"

md2_t md2_models[NUMSPRITES];

/*
 * free model
 */
static void md2_freeModel (md2_model_t *model)
{
	if (model)
	{
		if (model->skins)
			free(model->skins);

		if (model->texCoords)
			free(model->texCoords);

		if (model->triangles)
			free(model->triangles);

		if (model->frames)
		{
			int i;

			for (i = 0; i < model->header.numFrames; i++)
			{
				if (model->frames[i].vertices)
					free(model->frames[i].vertices);
			}
			free(model->frames);
		}

		if (model->glCommandBuffer)
			free(model->glCommandBuffer);

		free(model);
	}
}


//
// load model
//
// Hurdler: the current path is the Legacy.exe path
static md2_model_t *md2_readModel(const char *filename)
{
	FILE *file;
	md2_model_t *model;
	byte buffer[MD2_MAX_FRAMESIZE];
	int i;

	model = malloc(sizeof *model);
	if (!model)
		return 0;

	file = fopen(filename, "rb");
	if (!file)
	{
		free(model);
		return 0;
	}

	// initialize model and read header 
	memset(model, 0, sizeof *model);
	fread(&model->header, sizeof model->header, 1, file);

	if (model->header.magic !=
		(int)(('2' << 24) + ('P' << 16) + ('D' << 8) + 'I'))
	{
		fclose(file);
		free(model);
		return 0;
	}

	model->header.numSkins = 1;

	// read skins
	fseek(file, model->header.offsetSkins, SEEK_SET);
	if (model->header.numSkins > 0)
	{
		model->skins = (md2_skin_t *)malloc(sizeof (md2_skin_t) * model->header.numSkins);
		if (!model->skins)
		{
			md2_freeModel (model);
			return 0;
		}

		for (i = 0; i < model->header.numSkins; i++)
			fread(&model->skins[i], sizeof (md2_skin_t), 1, file);
	}

	// read texture coordinates 
	fseek(file, model->header.offsetTexCoords, SEEK_SET);
	if (model->header.numTexCoords > 0)
	{
		model->texCoords = (md2_textureCoordinate_t *)malloc(sizeof (md2_textureCoordinate_t) * model->header.numTexCoords);
		if (!model->texCoords)
		{
			md2_freeModel (model);
			return 0;
		}

		for (i = 0; i < model->header.numTexCoords; i++)
			fread(&model->texCoords[i], sizeof (md2_textureCoordinate_t), 1, file);
	}

	// read triangles 
	fseek(file, model->header.offsetTriangles, SEEK_SET);
	if (model->header.numTriangles > 0)
	{
		model->triangles = (md2_triangle_t *)malloc(sizeof (md2_triangle_t) * model->header.numTriangles);
		if (!model->triangles)
		{
			md2_freeModel (model);
			return 0;
		}

		for (i = 0; i < model->header.numTriangles; i++)
			fread(&model->triangles[i], sizeof (md2_triangle_t), 1, file);
	}

	// read alias frames 
	fseek(file, model->header.offsetFrames, SEEK_SET);
	if (model->header.numFrames > 0)
	{
		model->frames = (md2_frame_t *)malloc(sizeof (md2_frame_t) * model->header.numFrames);
		if (!model->frames)
		{
			md2_freeModel (model);
			return 0;
		}

		for (i = 0; i < model->header.numFrames; i++)
		{
			md2_alias_frame_t *frame = (md2_alias_frame_t *) buffer;
			int j;

			model->frames[i].vertices = (md2_triangleVertex_t *)malloc(sizeof (md2_triangleVertex_t) * model->header.numVertices);
			if (!model->frames[i].vertices)
			{
				md2_freeModel (model);
				return 0;
			}

			fread(frame, 1, model->header.frameSize, file);
			strcpy(model->frames[i].name, frame->name);
			for (j = 0; j < model->header.numVertices; j++)
			{
				model->frames[i].vertices[j].vertex[0] = (float) ((int) frame->alias_vertices[j].vertex[0]) * frame->scale[0] + frame->translate[0];
				model->frames[i].vertices[j].vertex[2] = -1* ((float) ((int) frame->alias_vertices[j].vertex[1]) * frame->scale[1] + frame->translate[1]);
				model->frames[i].vertices[j].vertex[1] = (float) ((int) frame->alias_vertices[j].vertex[2]) * frame->scale[2] + frame->translate[2];
			}
		}
	}

	// read gl commands 
	fseek(file, model->header.offsetGlCommands, SEEK_SET);
	if (model->header.numGlCommands)
	{
		model->glCommandBuffer = (int *)malloc(sizeof (int) * model->header.numGlCommands);
		if (!model->glCommandBuffer)
		{
			md2_freeModel (model);
			return 0;
		}

		fread(model->glCommandBuffer, sizeof (int), model->header.numGlCommands, file);
	}

	fclose(file);

	return model;
}

/*
 * center model
 */
static inline void md2_getBoundingBox (md2_model_t *model, float *minmax)
{
	int i;
	float minx, maxx;
	float miny, maxy;
	float minz, maxz;

	minx = miny = minz = 999999.0f;
	maxx = maxy = maxz = -999999.0f;

	/* get bounding box */
	for (i = 0; i < model->header.numVertices; i++)
	{
		md2_triangleVertex_t *v = &model->frames[0].vertices[i];

		if (v->vertex[0] < minx)
			minx = v->vertex[0];
		else if (v->vertex[0] > maxx)
			maxx = v->vertex[0];

		if (v->vertex[1] < miny)
			miny = v->vertex[1];
		else if (v->vertex[1] > maxy)
			maxy = v->vertex[1];

		if (v->vertex[2] < minz)
			minz = v->vertex[2];
		else if (v->vertex[2] > maxz)
			maxz = v->vertex[2];
	}

	minmax[0] = minx;
	minmax[1] = maxx;
	minmax[2] = miny;
	minmax[3] = maxy;
	minmax[4] = minz;
	minmax[5] = maxz;
}

static inline int md2_getAnimationCount(md2_model_t *model)
{
	int i, j = 0;
	size_t pos;
	int count;
	int lastId;
	char name[16], last[16];

	strcpy(last, model->frames[0].name);
	pos = strlen(last) - 1;
	while (last[pos] >= '0' && last[pos] <= '9' && j < 2)
	{
		pos--;
		j++;
	}
	last[pos + 1] = '\0';

	lastId = 0;
	count = 0;

	for (i = 0; i <= model->header.numFrames; i++)
	{
		if (i == model->header.numFrames)
			strcpy(name, ""); // some kind of a sentinel
		else
			strcpy(name, model->frames[i].name);
		pos = strlen(name) - 1;
		j = 0;
		while (name[pos] >= '0' && name[pos] <= '9' && j < 2)
		{
			pos--;
			j++;
		}
		name[pos + 1] = '\0';

		if (strcmp(last, name))
		{
			strcpy(last, name);
			count++;
		}
	}

	return count;
}

static inline const char * md2_getAnimationName (md2_model_t *model, int animation)
{
	int i, j = 0;
	size_t pos;
	int count;
	int lastId;
	static char last[32];
	char name[32];

	strcpy(last, model->frames[0].name);
	pos = strlen(last) - 1;
	while (last[pos] >= '0' && last[pos] <= '9' && j < 2)
	{
		pos--;
		j++;
	}
	last[pos + 1] = '\0';

	lastId = 0;
	count = 0;

	for (i = 0; i <= model->header.numFrames; i++)
	{
		if (i == model->header.numFrames)
			strcpy(name, ""); // some kind of a sentinel
		else
			strcpy(name, model->frames[i].name);
		pos = strlen(name) - 1;
		j = 0;
		while (name[pos] >= '0' && name[pos] <= '9' && j < 2)
		{
			pos--;
			j++;
		}
		name[pos + 1] = '\0';

		if (strcmp(last, name))
		{
			if (count == animation)
				return last;

			strcpy(last, name);
			count++;
		}
	}

	return 0;
}

static inline void md2_getAnimationFrames(md2_model_t *model,
	int animation, int *startFrame, int *endFrame)
{
	int i, j = 0;
	size_t pos;
	int count, numFrames, frameCount;
	int lastId;
	char name[16], last[16];

	strcpy(last, model->frames[0].name);
	pos = strlen(last) - 1;
	while (last[pos] >= '0' && last[pos] <= '9' && j < 2)
	{
		pos--;
		j++;
	}
	last[pos + 1] = '\0';

	lastId = 0;
	count = 0;
	numFrames = 0;
	frameCount = 0;

	for (i = 0; i <= model->header.numFrames; i++)
	{
		if (i == model->header.numFrames)
			strcpy(name, ""); // some kind of a sentinel
		else
			strcpy(name, model->frames[i].name);
		pos = strlen(name) - 1;
		j = 0;
		while (name[pos] >= '0' && name[pos] <= '9' && j < 2)
		{
			pos--;
			j++;
		}
		name[pos + 1] = '\0';

		if (strcmp(last, name))
		{
			strcpy(last, name);

			if (count == animation)
			{
				*startFrame = frameCount - numFrames;
				*endFrame = frameCount - 1;
				return;
			}

			count++;
			numFrames = 0;
		}
		frameCount++;
		numFrames++;
	}
	*startFrame = *endFrame = 0;
}

static inline void md2_printModelInfo (md2_model_t *model)
{
#if 0
	int i;

	CONS_Printf("magic:\t\t\t%c%c%c%c\n", model->header.magic>>24, 
                (model->header.magic>>16)&0xff, 
                (model->header.magic>>8)&0xff, 
                model->header.magic&0xff);
	CONS_Printf("version:\t\t%d\n", model->header.version);
	CONS_Printf("skinWidth:\t\t%d\n", model->header.skinWidth);
	CONS_Printf("skinHeight:\t\t%d\n", model->header.skinHeight);
	CONS_Printf("frameSize:\t\t%d\n", model->header.frameSize);
	CONS_Printf("numSkins:\t\t%d\n", model->header.numSkins);
	CONS_Printf("numVertices:\t\t%d\n", model->header.numVertices);
	CONS_Printf("numTexCoords:\t\t%d\n", model->header.numTexCoords);
	CONS_Printf("numTriangles:\t\t%d\n", model->header.numTriangles);
	CONS_Printf("numGlCommands:\t\t%d\n", model->header.numGlCommands);
	CONS_Printf("numFrames:\t\t%d\n", model->header.numFrames);
	CONS_Printf("offsetSkins:\t\t%d\n", model->header.offsetSkins);
	CONS_Printf("offsetTexCoords:\t%d\n", model->header.offsetTexCoords);
	CONS_Printf("offsetTriangles:\t%d\n", model->header.offsetTriangles);
	CONS_Printf("offsetFrames:\t\t%d\n", model->header.offsetFrames);
	CONS_Printf("offsetGlCommands:\t%d\n", model->header.offsetGlCommands);
	CONS_Printf("offsetEnd:\t\t%d\n", model->header.offsetEnd);

	for (i = 0; i < model->header.numFrames; i++)
		CONS_Printf("%s ", model->frames[i].name);
	CONS_Printf("\n");
#else
	model = NULL;
#endif
}

#ifdef HAVE_PNG
static void PNG_error(png_structp PNG, png_const_charp text)
{
	CONS_Printf("libpng error at %p: %s", PNG,text);
	//I_Error("libpng error at %p: %s", PNG,text);
}

static void PNG_warn(png_structp PNG, png_const_charp text)
{
	CONS_Printf("libpng warning at %p: %s", PNG, text);
}

static GrTextureFormat_t PNG_Load(const char *filename, int *w, int *h, GlidePatch_t *grpatch)
{
	png_structp png_ptr;
	png_infop png_info_ptr;
	png_uint_32 width, height;
	int bit_depth, color_type;
#ifdef PNG_SETJMP_SUPPORTED
#ifdef USE_FAR_KEYWORD
	jmp_buf jmpbuf;
#endif
#endif
	FILE *png_FILE;
	char *pngfilename = va("md2/%s", filename);

	FIL_ForceExtension(pngfilename, ".png");
	png_FILE = fopen(pngfilename, "rb");
	if (!png_FILE)
	{
		//CONS_Printf("M_SavePNG: Error on opening %s for loading\n", filename);
		return 0;
	}

	png_ptr = png_create_read_struct(PNG_LIBPNG_VER_STRING, NULL,
		PNG_error, PNG_warn);
	if (!png_ptr)
	{
		CONS_Printf("PNG_Load: Error on initialize libpng\n");
		fclose(png_FILE);
		return 0;
	}

	png_info_ptr = png_create_info_struct(png_ptr);
	if (!png_info_ptr)
	{
		CONS_Printf("PNG_Load: Error on allocate for libpng\n");
		png_destroy_read_struct(&png_ptr, NULL, NULL);
		fclose(png_FILE);
		return 0;
	}

#ifdef USE_FAR_KEYWORD
	if (setjmp(jmpbuf))
#else
	if (setjmp(png_jmpbuf(png_ptr)))
#endif
	{
		//CONS_Printf("libpng load error on %s\n", filename);
		png_destroy_read_struct(&png_ptr, &png_info_ptr, NULL);
		fclose(png_FILE);
		if (grpatch->mipmap.grInfo.data)
			Z_Free(grpatch->mipmap.grInfo.data);
		return 0;
	}
#ifdef USE_FAR_KEYWORD
	png_memcpy(png_jmpbuf(png_ptr), jmpbuf, sizeof (jmp_buf));
#endif

	png_init_io(png_ptr, png_FILE);

#ifdef PNG_SET_USER_LIMITS_SUPPORTED
	png_set_user_limits(png_ptr, 2048, 2048);
#endif

	png_read_info(png_ptr, png_info_ptr);

	png_get_IHDR(png_ptr, png_info_ptr, &width, &height, &bit_depth, &color_type,
	 NULL, NULL, NULL);

	if (bit_depth == 16)
		png_set_strip_16(png_ptr);

	if (color_type == PNG_COLOR_TYPE_GRAY || color_type == PNG_COLOR_TYPE_GRAY_ALPHA)
		png_set_gray_to_rgb(png_ptr);
	else if (color_type == PNG_COLOR_TYPE_PALETTE)
		png_set_palette_to_rgb(png_ptr);

	if (png_get_valid(png_ptr, png_info_ptr, PNG_INFO_tRNS))
		png_set_tRNS_to_alpha(png_ptr);
	else if (color_type != PNG_COLOR_TYPE_RGB_ALPHA && color_type != PNG_COLOR_TYPE_GRAY_ALPHA)
	{
#if PNG_LIBPNG_VER < 10207
		png_set_filler(png_ptr, 0xFF, PNG_FILLER_AFTER);
#else
		png_set_add_alpha(png_ptr, 0xFF, PNG_FILLER_AFTER);
#endif
	}

	png_read_update_info(png_ptr, png_info_ptr);
	
	{
		png_uint_32 i, pitch = png_get_rowbytes(png_ptr, png_info_ptr);
		png_bytep PNG_image = Z_Malloc(pitch*height, PU_HWRCACHE, &grpatch->mipmap.grInfo.data);
		png_bytep *row_pointers = png_malloc(png_ptr, height * sizeof (png_bytep));
		for (i = 0; i < height; i++)
			row_pointers[i] = PNG_image + i*pitch;
		png_read_image(png_ptr, row_pointers);
		png_free(png_ptr, (png_voidp)row_pointers);
	}

	png_destroy_read_struct(&png_ptr, &png_info_ptr, NULL);

	fclose(png_FILE);
	*w = width;
	*h = height;
	return GR_RGBA;
}
#endif

#define word short
typedef struct
{
	byte manufacturer;
	byte version;
	byte encoding;
	byte bitsPerPixel;
	word xmin;
	word ymin;
	word xmax;
	word ymax;
	word hDpi;
	word vDpi;
	byte colorMap[48];
	byte reserved;
	byte numPlanes;
	word bytesPerLine;
	word paletteInfo;
	word hScreenSize;
	word vScreenSize;
	byte filler[54];
} PcxHeader;
#undef word

static GrTextureFormat_t PCX_Load(const char *filename, int *w, int *h,
	GlidePatch_t *grpatch)
{
	PcxHeader header;
	byte palette[768];
	const byte *pal;
	RGBA_t *image;
	size_t pw, ph, size, ptr = 0;
	int ch, rep;
	FILE *file; 
	char *pcxfilename = va("md2/%s", filename);

	FIL_ForceExtension(pcxfilename, ".pcx");
	file = fopen(pcxfilename, "rb");
	if (!file)
		return 0;

	if (fread(&header, sizeof (PcxHeader), 1, file) == (size_t)-1)
	{
		fclose(file);
		return 0;
	}

	if (header.bitsPerPixel != 8)
	{
		fclose(file);
		return 0;
	}

	fseek(file, -768, SEEK_END);

	pw = *w = header.xmax - header.xmin + 1;
	ph = *h = header.ymax - header.ymin + 1;
	image = Z_Malloc(pw*ph*4, PU_HWRCACHE, &grpatch->mipmap.grInfo.data);

	if (fread(palette, sizeof (byte), 768, file) == (size_t)-1)
	{
		Z_Free(image);
		fclose(file);
		return 0;
	}
	fseek(file, sizeof (PcxHeader), SEEK_SET);

	size = pw * ph;
	while (ptr < size)
	{
		ch = fgetc(file);  //Hurdler: beurk
		if (ch >= 192)
		{
			rep = ch - 192;
			ch = fgetc(file);
		}
		else
		{
			rep = 1;
		}
		while (rep--)
		{
			pal = palette + ch*3;
			image[ptr].s.red   = *pal++;
			image[ptr].s.green = *pal++;
			image[ptr].s.blue  = *pal++;
			image[ptr].s.alpha = 0xFF;
			ptr++;
		}
	}
	fclose(file);
	return GR_RGBA;
}

// -----------------+
// md2_loadTexture  : Download a pcx texture for MD2 models
// -----------------+
static void md2_loadTexture(md2_t *model)
{
	GlidePatch_t *grpatch;
	const char *filename = model->filename;

	if (model->grpatch)
	{
		grpatch = model->grpatch;
		Z_Free(grpatch->mipmap.grInfo.data);
	}
	else
	{
		grpatch = Z_Malloc(sizeof *grpatch, PU_HWRPATCHINFO,
			&(model->grpatch));
		memset(grpatch, 0, sizeof *grpatch);
	}

	if (!grpatch->mipmap.downloaded && !grpatch->mipmap.grInfo.data)
	{
		int w = 0, h = 0;
#ifdef HAVE_PNG
		grpatch->mipmap.grInfo.format = PNG_Load(filename, &w, &h, grpatch);
		if (grpatch->mipmap.grInfo.format == 0)
#endif
		grpatch->mipmap.grInfo.format = PCX_Load(filename, &w, &h, grpatch);
		if (grpatch->mipmap.grInfo.format == 0)
			return;

		grpatch->mipmap.downloaded = 0;
		grpatch->mipmap.flags = 0;

		grpatch->width = (short)w;
		grpatch->height = (short)h;
		grpatch->mipmap.width = (unsigned short)w;
		grpatch->mipmap.height = (unsigned short)h;

		// not correct!
		grpatch->mipmap.grInfo.smallLodLog2 = GR_LOD_LOG2_256;
		grpatch->mipmap.grInfo.largeLodLog2 = GR_LOD_LOG2_256;
		grpatch->mipmap.grInfo.aspectRatioLog2 = GR_ASPECT_LOG2_1x1;
	}
	HWD.pfnSetTexture(&grpatch->mipmap);
}

void HWR_InitMD2(void)
{
	size_t i;
	FILE *f;
	char name[5], filename[32];
	float scale, offset;

	CONS_Printf("InitMD2()...\n");
	for (i = 0; i < NUMSPRITES; i++)
	{
		md2_models[i].scale = -1.0f;
		md2_models[i].model = NULL;
		md2_models[i].grpatch = NULL;
	}
	// read the md2.dat file

	f = fopen("md2.dat", "rt");
	if (!f)
	{
		CONS_Printf("Error while loading md2.dat\n");
		return;
	}
	while (fscanf(f, "%4s %31s %f %f", name, filename, &scale, &offset) == 4)
	{
		for (i = 0; i < NUMSPRITES; i++)
		{
			if (strcmp(name, sprnames[i]) == 0)
			{
				//CONS_Printf("  Found: %s %s %f %f\n", name, filename, scale, offset);
				md2_models[i].scale = scale;
				md2_models[i].offset = offset;
				strcpy(md2_models[i].filename, filename);
				break;
			}
		}
		if (i == NUMSPRITES)
			CONS_Printf("    Not found: %s\n", name);
	}
	fclose(f);
}



// -----------------+
// HWR_DrawMD2      : Draw MD2
//                  : (monsters, bonuses, weapons, lights, ...)
// Returns          :
// -----------------+
	/*
	wait/stand
	death
	pain
	walk
	shoot/fire

	die?
	atka?
	atkb?
	attacka/b/c/d?
	res?
	run?
	*/
void HWR_DrawMD2(gr_vissprite_t *spr)
{
	GlidePatch_t *gpatch; // sprite patch converted to hardware
	FSurfaceInfo Surf;

	char filename[64];
	int frame;
	FTransform p;
	md2_t *md2;

	// cache sprite graphics
	//12/12/99: Hurdler:
	//          OK, I don't change anything for MD2 support because I want to be
	//          sure to do it the right way. So actually, we keep normal sprite
	//          in memory and we add the md2 model if it exists for that sprite

	// convert srpite differently when fxtranslucent is detected
	if ((spr->mobj->frame & FF_TRANSMASK) == tr_transfx1<<FF_TRANSSHIFT)
	{
		firetranslucent = true;
		gpatch = W_CachePatchNum(spr->patchlumpnum, PU_CACHE);
		firetranslucent = false;
	}
	else
		gpatch = W_CachePatchNum(spr->patchlumpnum, PU_CACHE);

	/// \todo manage spr->flip

	//12/12/99: Hurdler: same comment as above (for md2)
	HWR_GetMappedPatch(gpatch, spr->colormap);

	// sprite lighting by modulating the RGB components
	/// \todo coloured
	Surf.FlatColor.s.red = Surf.FlatColor.s.green = Surf.FlatColor.s.blue = spr->sectorlight;

	// Look at HWR_ProjetctSprite for more
	if (cv_grmd2.value && (md2_models[spr->mobj->sprite].scale > 0) && !spr->precip)
	{
		int blend = 0;
		GlidePatch_t *oldgpatch = gpatch;

		if (spr->mobj->frame & FF_TRANSMASK)
			blend = HWR_TranstableToAlpha((spr->mobj->frame & FF_TRANSMASK)>>FF_TRANSSHIFT, &Surf);
		else if (spr->mobj->frame & FF_SMOKESHADE)
		{
			Surf.FlatColor.s.alpha = 0x80;
			blend = PF_Translucent;
		}
		else if (spr->mobj->flags2 & MF2_SHADOW)
		{
			Surf.FlatColor.s.alpha = 0x40;
			blend = PF_Translucent;
		}
		else
		{
			Surf.FlatColor.s.alpha = 0xFF;
			blend = PF_Translucent|PF_Occlude;
		}
		// hack for updating the light level before drawing the md2
		HWD.pfnDrawPolygon(&Surf, NULL, 4, blend|PF_Modulated|PF_Clip|PF_MD2);

		// dont forget to enabled the depth test because we can't do this like
		// before: polygons models are not sorted

		// 1. load model+texture if not already loaded
		// 2. draw model with correct position, rotation,...
		md2 = &md2_models[spr->mobj->sprite];
		if (!md2->model) 
		{
			//CONS_Printf("Loading MD2... (%s)", sprnames[spr->mobj->sprite]);
			sprintf(filename, "md2/%s", md2->filename);
			md2->model = md2_readModel(filename);

			if (md2->model)
			{
				md2_printModelInfo(md2->model);
			}
			else
			{
				//CONS_Printf(" FAILED\n");
				return;
			}
		}
		//Hurdler: arf, I don't like that implementation at all... too much crappy
		gpatch = md2->grpatch;
		if (!gpatch || !gpatch->mipmap.grInfo.format ||!gpatch->mipmap.downloaded)
			md2_loadTexture(md2);
		else if (gpatch->mipmap.grInfo.format)
			HWD.pfnSetTexture(&gpatch->mipmap);
		else
			HWD.pfnSetTexture(&oldgpatch->mipmap);

		//FIXME: this is not yet correct
		frame = spr->mobj->frame % md2->model->header.numFrames;

		//Hurdler: it seems there is still a small problem with mobj angle 
		p.x = FIXED_TO_FLOAT(spr->mobj->x);
		p.y = FIXED_TO_FLOAT(spr->mobj->y)+md2->offset;
		p.z = FIXED_TO_FLOAT(spr->mobj->z);
		{
			const fixed_t anglef = AngleFixed(spr->mobj->angle);
			p.angley = FIXED_TO_FLOAT(anglef);
		}
		p.anglex = 0.0f;

		HWD.pfnDrawMD2(md2->model->glCommandBuffer, &md2->model->frames[frame], &p, md2->scale);
	}
}
