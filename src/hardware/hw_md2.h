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
/// \brief MD2 Handling
///	Inspired from md2.h by Mete Ciragan (mete@swissquake.ch)

#ifndef _HW_MD2_H_
#define _HW_MD2_H_

#include "hw_glob.h"

#define MD2_MAX_TRIANGLES               4096
#define MD2_MAX_VERTICES                2048
#define MD2_MAX_TEXCOORDS               2048
#define MD2_MAX_FRAMES                  512
#define MD2_MAX_SKINS                   32
#define MD2_MAX_FRAMESIZE               (MD2_MAX_VERTICES * 4 + 128)

typedef struct 
{ 
	int magic; 
	int version; 
	int skinWidth; 
	int skinHeight; 
	int frameSize; 
	int numSkins; 
	int numVertices; 
	int numTexCoords; 
	int numTriangles; 
	int numGlCommands; 
	int numFrames; 
	int offsetSkins; 
	int offsetTexCoords; 
	int offsetTriangles; 
	int offsetFrames; 
	int offsetGlCommands; 
	int offsetEnd; 
} md2_header_t;

typedef struct
{
	unsigned char vertex[3];
	unsigned char lightNormalIndex;
} md2_alias_triangleVertex_t;

typedef struct
{
	float vertex[3];
	float normal[3];
} md2_triangleVertex_t;

typedef struct
{
	short vertexIndices[3];
	short textureIndices[3];
} md2_triangle_t;

typedef struct
{
	short s, t;
} md2_textureCoordinate_t;

typedef struct
{
	float scale[3];
	float translate[3];
	char name[16];
	md2_alias_triangleVertex_t alias_vertices[1];
} md2_alias_frame_t;

typedef struct
{
	char name[16];
	md2_triangleVertex_t *vertices;
} md2_frame_t;

typedef char md2_skin_t[64];

typedef struct
{
	float s, t;
 	int vertexIndex;
} md2_glCommandVertex_t;

typedef struct
{
	md2_header_t            header;
	md2_skin_t              *skins;
	md2_textureCoordinate_t *texCoords;
	md2_triangle_t          *triangles;
	md2_frame_t             *frames;
	int                     *glCommandBuffer;
} md2_model_t;

typedef struct
{
	char        filename[32];
	float       scale;
	float       offset;
	md2_model_t *model;
	void        *grpatch;
} md2_t;

extern md2_t md2_models[NUMSPRITES];

void HWR_InitMD2(void);
void HWR_DrawMD2(gr_vissprite_t *spr);

#endif // _HW_MD2_H_
