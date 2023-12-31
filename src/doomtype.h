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
/// \brief SRB2 standard types
/// 
///	Simple basic typedefs, isolated here to make it easier
///	separating modules.

#ifndef __DOOMTYPE__
#define __DOOMTYPE__

#if (defined (_WIN32) && !defined (_XBOX)) || (defined (_WIN32_WCE) && !defined (__GNUC__)) || defined (_WIN64)
//#define WIN32_LEAN_AND_MEAN
#define RPC_NO_WINDOWS_H
#include <windows.h>
#endif
#ifndef _OS2EMX_H
#if defined (_XBOX) || !(defined (__MINGW32__) || defined (__MINGW64__))
typedef unsigned long ULONG;
typedef unsigned short USHORT;
#endif
#endif // _OS2EMX_H

/* 7.18.1.1  Exact-width integer types */
#ifdef _MSC_VER
#define UINT8 unsigned __int8
#define SINT8 signed __int8

#define UINT16 unsigned __int16
#define INT16 __int16

#define INT32 __int32
#define UINT32 unsigned __int32

#define INT64  __int64
#define UINT64 unsigned __int64

typedef long ssize_t;

/* Older Visual C++ headers don't have the Win64-compatible typedefs... */
#if (_MSC_VER <= 1200)
	#ifndef DWORD_PTR
		#define DWORD_PTR DWORD
	#endif
	#ifndef PDWORD_PTR
		#define PDWORD_PTR PDWORD
	#endif
#endif
#elif defined (_arch_dreamcast) // KOS Dreamcast
#include <arch/types.h>

#define UINT8 unsigned char
#define SINT8 signed char

#define UINT16 uint16
#define INT16 int16

#define INT32 int
#define UINT32 unsigned int
#define INT64  int64
#define UINT64 uint64
#elif defined (__DJGPP__)
#define UINT8 unsigned char
#define SINT8 signed char

#define UINT16 unsigned short int
#define INT16 signed short int

#define INT32 signed long
#define UINT32 unsigned long
#define INT64  signed long long
#define UINT64 unsigned long long
#else
#define __STDC_LIMIT_MACROS
#include <stdint.h>

#define UINT8 uint8_t
#define SINT8 int8_t

#define UINT16 uint16_t
#define INT16 int16_t

#define INT32 int32_t
#define UINT32 uint32_t
#define INT64  int64_t
#define UINT64 uint64_t
#endif

#ifdef __APPLE_CC__
#define DIRECTFULLSCREEN
#define DEBUG_LOG
#define HWRENDER
#define NOIPX
#endif

#if defined (_MSC_VER) || defined (__OS2__)
	// Microsoft VisualC++
#ifdef _MSC_VER
	#define vsnprintf               _vsnprintf
	#define snprintf                _snprintf
#endif
	#define strncasecmp             strnicmp
	#define strcasecmp              stricmp
	#define inline                  __inline
#elif defined (__WATCOMC__)
	#include <dos.h>
	#include <sys\types.h>
	#include <direct.h>
	#include <malloc.h>
	#define strncasecmp             strnicmp
	#define strcasecmp              strcmpi
#endif
#ifdef _PSP_
	#include <malloc.h>
#elif defined(UNIXLIKE)
	#undef stricmp
	#define stricmp(x,y) strcasecmp(x,y)
	#undef strnicmp
	#define strnicmp(x,y,n) strncasecmp(x,y,n)
#endif
#ifdef _WIN32_WCE
#ifndef __GNUC__
	#define stricmp(x,y)            _stricmp(x,y)
	#define strnicmp                _strnicmp
#endif
	#define strdup                  _strdup
	#define strupr                  _strupr
	#define strlwr                  _strlwr
#endif

#if defined (macintosh) //|| defined (__APPLE__) //skip all boolean/Boolean crap
	#define true 1
	#define false 0
	#define min(x,y) (((x)<(y)) ? (x) : (y))
	#define max(x,y) (((x)>(y)) ? (x) : (y))

#ifdef macintosh
	#define stricmp strcmp
	#define strnicmp strncmp
#endif

	#define __BYTEBOOL__
	typedef unsigned char byte;
	#define boolean int

	#ifndef O_BINARY
	#define O_BINARY 0
	#endif
#endif //macintosh

#if defined (PC_DOS) || defined (_WIN32) || defined (_WIN64) || defined (_PSP_) || defined (_arch_dreamcast)
#define HAVE_DOSSTR_FUNCS
#endif

#ifndef HAVE_DOSSTR_FUNCS
int strupr(char *n); // from dosstr.c
int strlwr(char *n); // from dosstr.c
#endif

#include <stddef.h> // for size_t

#ifndef __APPLE__
size_t strlcat(char *dst, const char *src, size_t siz);
size_t strlcpy(char *dst, const char *src, size_t siz);
#endif

// Macro for use with char foo[FOOSIZE+1] type buffers.
// Never use this with a buffer that is a "char *" or passed
// into the function as an argument.
//
// In those cases sizeof will return the size of the pointer,
// not the number of bytes in the buffer.
#define STRBUFCPY(dst,src) strlcpy(dst, src, sizeof dst)

#ifndef __BYTEBOOL__
	#define __BYTEBOOL__

	// Fixed to use builtin bool type with C++.
	//#ifdef __cplusplus
	//    typedef bool boolean;
	//#else
	#if defined (_XBOX) || !(defined (__MINGW32__) || defined (__MINGW64__))
		typedef unsigned char byte;
	#endif
	//faB: clean that up !!
	#if (defined (_WIN32) || (defined (_WIN32_WCE) && !defined (__GNUC__)) || defined (_WIN64)) && !defined (_XBOX)
		#define false   FALSE           // use windows types
		#define true    TRUE
		#define boolean BOOL
	#else
		typedef enum {false, true} boolean;
	#endif
	//#endif // __cplusplus
#endif // __BYTEBOOL__

typedef ULONG tic_t;

#include <limits.h>

#ifdef MAXCHAR
#undef MAXCHAR
#endif
#ifdef MAXSHORT
#undef MAXSHORT
#endif
#ifdef MAXINT
#undef MAXINT
#endif
#ifdef MAXLONG
#undef MAXLONG
#endif
#ifdef MINCHAR
#undef MINCHAR
#endif
#ifdef MINSHORT
#undef MINSHORT
#endif
#ifdef MININT
#undef MININT
#endif
#ifdef MINLONG
#undef MINLONG
#endif

#define MAXCHAR  ((char)0x7f)
#define MAXSHORT ((short)0x7fff)
#define MAXINT   ((int)0x7fffffff)
#define MAXLONG  ((long)0x7fffffff)
#define MINCHAR  ((char)0x80)
#define MINSHORT ((short)-0x8000)
#define MININT   ((int)0x80000000)
#define MINLONG  ((long)0x80000000)

union FColorRGBA
{
	ULONG rgba;
	struct
	{
		byte red;
		byte green;
		byte blue;
		byte alpha;
	} s;
};
typedef union FColorRGBA RGBA_t;

#ifdef __BIG_ENDIAN__
#define UINT2RGBA(a) a
#else
#define UINT2RGBA(a) ((a&0xff)<<24)|((a&0xff00)<<8)|((a&0xff0000)>>8)|(((ULONG)a&0xff000000)>>24)
#endif

#ifdef __GNUC__ // __attribute__ ((X))
#define FUNCPRINTF __attribute__ ((format(printf, 1, 0)))
#define FUNCNORETURN __attribute__ ((noreturn))
#define FUNCIERROR __attribute__ ((format(printf, 1, 0),noreturn))
#define FUNCDEAD __attribute__ ((deprecated))
#if (__GNUC__ > 3) || (__GNUC__ == 3 && __GNUC_MINOR__ >= 1)
#define FUNCINLINE __attribute__((always_inline))
#endif
#define FUNCNOINLINE __attribute__((noinline))
#ifndef __APPLE__
#define ATTRPACK __attribute__ ((packed))
#endif
#ifdef _XBOX
#define FILESTAMP I_OutputMsg("%s:%d\n",__FILE__,__LINE__);
#define XBOXSTATIC static
#endif
#elif defined (_MSC_VER)
#define ATTRNORETURN __declspec(noreturn)
#define ATTRINLINE __forceinline
#if _MSC_VER > 1200
#define ATTRNOINLINE __declspec(noinline)
#endif
#endif

#ifndef FUNCPRINTF
#define FUNCPRINTF
#endif
#ifndef FUNCNORETURN
#define FUNCNORETURN
#endif
#ifndef FUNCIERROR
#define FUNCIERROR
#endif
#ifndef FUNCDEAD
#define FUNCDEAD
#endif
#ifndef FUNCINLINE
#define FUNCINLINE
#endif
#ifndef FUNCNOINLINE
#define FUNCNOINLINE
#endif
#ifndef ATTRPACK
#define ATTRPACK
#endif
#ifndef ATTRNORETURN
#define ATTRNORETURN
#endif
#ifndef ATTRINLINE
#define ATTRINLINE inline
#endif
#ifndef ATTRNOINLINE
#define ATTRNOINLINE
#endif
#ifndef XBOXSTATIC
#define XBOXSTATIC
#endif
#ifndef FILESTAMP
#define FILESTAMP
#endif
#endif //__DOOMTYPE__
