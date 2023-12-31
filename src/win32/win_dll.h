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
/// \brief load/unload a DLL at run-time

//#define WIN32_LEAN_AND_MEAN
#define RPC_NO_WINDOWS_H
#include <windows.h>

// this is made extern so win_dbg.c can log the load address
// of the dynamically loaded DLL functions
typedef struct loadfunc_s {
	LPCSTR fnName;
	LPVOID fnPointer;
} loadfunc_t;
#ifdef HWRENDER
extern HINSTANCE  hwdInstance;
extern loadfunc_t hwdFuncTable[];
#endif

#ifdef HW3SOUND
extern HINSTANCE  hwsInstance;
extern loadfunc_t hwsFuncTable[];
#endif


HINSTANCE LoadDLL (LPCSTR dllName, loadfunc_t *funcTable);
VOID UnloadDLL (HINSTANCE* pInstance);

#ifdef HWRENDER
BOOL Init3DDriver (LPCSTR dllName);
VOID Shutdown3DDriver (VOID);
#endif

#ifdef HW3SOUND
BOOL Init3DSDriver(LPCSTR dllName);
VOID Shutdown3DSDriver(VOID);
#endif
