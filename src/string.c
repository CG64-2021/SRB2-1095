// Emacs style mode select   -*- C++ -*- 
//-----------------------------------------------------------------------------
//
// This file is in the public domain.
// Written by Graue in 2006.
//
//-----------------------------------------------------------------------------
/// \file
/// \brief String functions that we need but are missing on some
///        operating systems.

#include <stddef.h>
#include <string.h>
#include "doomdef.h"

// Like the OpenBSD version, but it doesn't check for src not being a valid
// C string.
size_t strlcat(char *dst, const char *src, size_t siz)
{
	size_t dstlen, n = siz;
	char *p = dst;

	dstlen = strlen(dst);
	n -= dstlen;
	p += dstlen;

	while (n > 1 && *src != '\0')
	{
		*p++ = *src++;
		n--;
	}

	if (n >= 1)
		*p = '\0';

	return strlen(src) + dstlen;
}

size_t strlcpy(char *dst, const char *src, size_t siz)
{
	if (siz == 0)
		return strlen(dst);

	dst[0] = '\0';
	return strlcat(dst, src, siz);
}
