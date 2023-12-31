// Emacs style mode select   -*- C++ -*-
//
//-----------------------------------------------------------------------------
//
// Copyright (C) 1993-1996 by id Software, Inc.
// Portions Copyright (C) 1998-2000 by DooM Legacy Team.
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
// Changes by Graue <graue@oceanbase.org> are in the public domain.
//
//-----------------------------------------------------------------------------
/// \file
/// \brief SRB2 system stuff for SDL

#ifndef _WIN32_WCE
#include <signal.h>
#endif

#ifdef _XBOX
#include "SRB2XBOX/xboxhelp.h"
#endif

#if (defined (_WIN32) || defined (_WIN64)) && !defined (_XBOX)
#define RPC_NO_WINDOWS_H
#include <windows.h>
typedef BOOL (WINAPI *MyFunc)(LPCSTR RootName, PULARGE_INTEGER pulA, PULARGE_INTEGER pulB, PULARGE_INTEGER pulFreeBytes);
typedef DWORD (WINAPI *MyFunc2) (void);
#endif	
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#ifdef __GNUC__
#include <unistd.h>
#elif defined (_MSC)
#include <direct.h>
#endif
#ifndef _WIN32_WCE
#if !defined (SDLIO) || defined (UNIXLIKE)
#include <fcntl.h>
#endif
#endif

#ifdef _arch_dreamcast
#include <arch/gdb.h>
#include <arch/timer.h>
#include <conio/conio.h>
#include <dc/pvr.h>
void __set_fpscr(long); // in libgcc / kernel's startup.s?
#else
#include <stdio.h>
#if (defined (_WIN32) || defined (_WIN64)) && !defined (_WIN32_WCE)
#include <conio.h>
#endif
#endif

#ifdef _MSC_VER
#pragma warning(disable : 4214 4244)
#endif

#include "SDL.h"

#ifdef _MSC_VER
#pragma warning(default : 4214 4244)
#endif

#if SDL_VERSION_ATLEAST(1,2,7) && !defined (__APPLE_CC__) && !defined (DC)
#include "SDL_cpuinfo.h" // 1.2.7 or greater
#define HAVE_SDLCPUINFO
#endif

#ifdef _PSP_
#include <pspiofilemgr.h>
#else
#if defined (UNIXLIKE) && !defined (_arch_dreamcast)
#if defined (LINUX) && !defined (FREEBSD)
#include <sys/vfs.h>
#else
#include <sys/param.h>
#include <sys/mount.h>
/*For meminfo*/
#include <sys/types.h>
#include <kvm.h>
#include <nlist.h>
#include <sys/vmmeter.h>
#endif
#endif
#endif

#if defined (UNIXLIKE) && !defined (_arch_dreamcast) && !defined (_PSP_)
#include <termios.h>
#include <sys/ioctl.h> // ioctl
#define HAVE_TERMIOS
#endif

#ifdef _WIN32_WCE
#include "SRB2CE/cehelp.h"
#endif

#ifndef O_BINARY
#define O_BINARY 0
#endif

// Locations for searching the srb2.srb
#ifdef _arch_dreamcast
#define DEFAULTWADLOCATION1 "/cd"
#define DEFAULTWADLOCATION2 "/pc"
#define DEFAULTWADLOCATION3 "/pc/home/alam/srb2code/data"
#define DEFAULTSEARCHPATH1 "/cd"
#define DEFAULTSEARCHPATH2 "/pc"
//#define DEFAULTSEARCHPATH3 "/pc/home/alam/srb2code/data"
#elif defined (UNIXLIKE) || defined (__APPLE__)
#define DEFAULTWADLOCATION1 "/usr/local/games/srb2"
#define DEFAULTWADLOCATION2 "/usr/games/srb2"
#define DEFAULTSEARCHPATH1 "/usr/local"
#define DEFAULTSEARCHPATH2 "/usr/games"
#elif defined (_XBOX)
#define NOCWD
#ifdef __GNUC__
#include <openxdk/debug.h>
#endif
#define DEFAULTWADLOCATION1 "c:\\srb2"
#define DEFAULTWADLOCATION2 "d:\\srb2"
#define DEFAULTWADLOCATION3 "e:\\srb2"
#define DEFAULTWADLOCATION4 "f:\\srb2"
#define DEFAULTWADLOCATION5 "g:\\srb2"
#define DEFAULTWADLOCATION6 "h:\\srb2"
#define DEFAULTWADLOCATION7 "i:\\srb2"
#elif defined (_WIN32_WCE)
#define NOCWD
#define NOHOME
#define DEFAULTWADLOCATION1 "\\Storage Card\\SRB2DEMO"
#define DEFAULTSEARCHPATH1 "\\Storage Card"
#elif defined (_WIN32)  || defined (_WIN64)
#define DEFAULTWADLOCATION1 "c:\\games\\srb2"
#define DEFAULTWADLOCATION2 "\\games\\srb2"
#define DEFAULTSEARCHPATH1 "c:\\games"
#define DEFAULTSEARCHPATH2 "\\games"
#endif

/**	\brief WAD file to look for
*/
#define WADKEYWORD1 "srb2.srb"
#define WADKEYWORD2 "srb2.wad"
/**	\brief holds wad path
*/
static char returnWadPath[256];

//Alam_GBC: SDL

#include "../doomdef.h"
#include "../m_misc.h"
#include "../i_video.h"
#include "../i_sound.h"
#include "../i_system.h"
#include "../screen.h" //vid.WndParent
#include "../d_net.h"
#include "../g_game.h"
#include "../filesrch.h"
#include "endtxt.h"
#include "sdlmain.h"

#include "../i_joy.h"

#include "../m_argv.h"

#ifdef MAC_ALERT
#include "macosx/mac_alert.h"
#endif

#include "../d_main.h"

/**	\brief	The JoyReset function

	\param	JoySet	Joystick info to reset

	\return	void

	
*/
static void JoyReset(SDLJoyInfo_t *JoySet)
{
	if (JoySet->dev) SDL_JoystickClose(JoySet->dev);
	JoySet->dev = NULL;
	JoySet->oldjoy = -1;
	JoySet->axises = JoySet->buttons = JoySet->hats = JoySet->balls = 0;
	//JoySet->scale
}

/**	\brief First joystick up and running
*/
static int joystick_started  = 0;

/**	\brief SDL info about joystick 1
*/
SDLJoyInfo_t JoyInfo;


/**	\brief Second joystick up and running
*/
static int joystick2_started = 0;

/**	\brief SDL inof about joystick 2
*/
SDLJoyInfo_t JoyInfo2;

#ifdef HAVE_TERMIOS
static int fdmouse2 = -1;
static int mouse2_started = 0;
#endif

SDL_bool consolevent = SDL_TRUE;


/// \brief max number of joystick buttons
#define JOYBUTTONS_MIN JOYBUTTONS
/// \brief max number of joystick button events
#define JOYBUTTONS_MAX JOYBUTTONS
/// \brief max number of joystick axies
#define JOYAXISES_MIN  JOYAXISSET
/// \brief max number ofjoystick axis events
#define JOYAXISES_MAX  JOYAXISSET
/// \brief max number of joystick hats
#define JOYHATS_MIN    JOYHATS
/// \brief max number of joystick hat events
#define JOYHATS_MAX    JOYHATS

byte keyboard_started = false;

#if 0

static void signal_handler(int num)
{
	//static char msg[] = "oh no! back to reality!\r\n";
	char *      sigmsg;
	char        sigdef[32];

	switch (num)
	{
	case SIGINT:
		sigmsg = "interrupt";
		break;
	case SIGILL:
		sigmsg = "illegal instruction - invalid function image";
		break;
	case SIGFPE:
		sigmsg = "floating point exception";
		break;
	case SIGSEGV:
		sigmsg = "segment violation";
		break;
	case SIGTERM:
		sigmsg = "Software termination signal from kill";
		break;
#if !defined (UNIXLIKE)
	case SIGBREAK:
		sigmsg = "Ctrl-Break sequence";
		break;
#endif
	case SIGABRT:
		sigmsg = "abnormal termination triggered by abort call";
		break;
	default:
		sprintf(sigdef,"signal number %d", num);
		sigmsg = sigdef;
	}
	
#ifdef LOGMESSAGES
	if (logstream != INVALID_HANDLE_VALUE)
	{
		I_OutputMsg ("signal_handler() error: %s\n", sigmsg);
	}
#endif
	signal(num, SIG_DFL);               //default signal action
	raise(num);
	I_Quit();
}
#endif

#if defined (NDEBUG) && !defined (DC) && !defined (_WIN32_WCE)
FUNCNORETURN static ATTRNORETURN void quit_handler(int num)
{
	signal(num, SIG_DFL);               //default signal action
	raise(num);
	I_Quit();
}
#endif

#ifdef HAVE_TERMIOS
// TERMIOS console code from Quake3: thank you!
SDL_bool stdin_active = SDL_TRUE;

typedef struct
{
	size_t cursor;
	char buffer[256];
} feild_t;

feild_t tty_con;

// when printing general stuff to stdout stderr (Sys_Printf)
//   we need to disable the tty console stuff
// this increments so we can recursively disable
static int ttycon_hide = 0;
// some key codes that the terminal may be using
// TTimo NOTE: I'm not sure how relevant this is
static int tty_erase;
static int tty_eof;

static struct termios tty_tc;

// =============================================================
// tty console routines
// NOTE: if the user is editing a line when something gets printed to the early console then it won't look good
//   so we provide tty_Clear and tty_Show to be called before and after a stdout or stderr output
// =============================================================

// flush stdin, I suspect some terminals are sending a LOT of shit
// FIXME TTimo relevant?
#if 0
static inline void tty_FlushIn(void)
{
	char key;
	while (read(STDIN_FILENO, &key, 1)!=-1);
}
#endif

// do a backspace
// TTimo NOTE: it seems on some terminals just sending '\b' is not enough
//   so for now, in any case we send "\b \b" .. yeah well ..
//   (there may be a way to find out if '\b' alone would work though)
static void tty_Back(void)
{
	char key;
	key = '\b';
	write(STDOUT_FILENO, &key, 1);
	key = ' ';
	write(STDOUT_FILENO, &key, 1);
	key = '\b';
	write(STDOUT_FILENO, &key, 1);
}

static void tty_Clear(void)
{
	size_t i;
	if (tty_con.cursor>0)
	{
		for (i=0; i<tty_con.cursor; i++)
		{
			tty_Back();
		}
	}

}

// clear the display of the line currently edited
// bring cursor back to beginning of line
static inline void tty_Hide(void)
{
	//I_Assert(consolevent);
	if (ttycon_hide)
	{
		ttycon_hide++;
		return;
	}
	tty_Clear();
	ttycon_hide++;
}

// show the current line
// FIXME TTimo need to position the cursor if needed??
static inline void tty_Show(void)
{
	size_t i;
	//I_Assert(consolevent);
	I_Assert(ttycon_hide>0);
	ttycon_hide--;
	if (ttycon_hide == 0 && tty_con.cursor)
	{
		for (i=0; i<tty_con.cursor; i++)
		{
			write(STDOUT_FILENO, tty_con.buffer+i, 1);
		}
	}
}

// never exit without calling this, or your terminal will be left in a pretty bad state
static void I_ShutdownConsole(void)
{
	if (consolevent)
	{
		I_OutputMsg("Shutdown tty console\n");
		consolevent = SDL_FALSE;
		tcsetattr (STDIN_FILENO, TCSADRAIN, &tty_tc);
	}
}

static void I_StartupConsole(void)
{
	struct termios tc;

	// TTimo 
	// https://zerowing.idsoftware.com/bugzilla/show_bug.cgi?id=390
	// then SIGTTIN or SIGTOU is emitted, if not catched, turns into a SIGSTP
	signal(SIGTTIN, SIG_IGN);
	signal(SIGTTOU, SIG_IGN);

	consolevent = !M_CheckParm("-noconsole");

	if (!consolevent) return;

	if (isatty(STDIN_FILENO)!=1)
	{
		I_OutputMsg("stdin is not a tty, tty console mode failed\n");
		consolevent = SDL_FALSE;
		return;
	}
	memset(&tty_con, 0x00, sizeof(tty_con));
	tcgetattr (0, &tty_tc);
	tty_erase = tty_tc.c_cc[VERASE];
	tty_eof = tty_tc.c_cc[VEOF];
	tc = tty_tc;
	/*
	 ECHO: don't echo input characters
	 ICANON: enable canonical mode.  This  enables  the  special
	  characters  EOF,  EOL,  EOL2, ERASE, KILL, REPRINT,
	  STATUS, and WERASE, and buffers by lines.
	 ISIG: when any of the characters  INTR,  QUIT,  SUSP,  or
	  DSUSP are received, generate the corresponding signal
	*/			  
	tc.c_lflag &= ~(ECHO | ICANON);
	/*
	 ISTRIP strip off bit 8
	 INPCK enable input parity checking
	 */
	tc.c_iflag &= ~(ISTRIP | INPCK);
	tc.c_cc[VMIN] = 0; //1?
	tc.c_cc[VTIME] = 0;
	tcsetattr (0, TCSADRAIN, &tc);	
}

void I_GetConsoleEvents(void)
{
	// we use this when sending back commands
	event_t ev = {0,0,0,0};
	int avail;
	char key;

	if(!consolevent)
		return;
	
	ev.type = ev_console;
	avail = read(STDIN_FILENO, &key, 1);
	if (avail == -1)
		return;

	// we have something
	// backspace?
	// NOTE TTimo testing a lot of values .. seems it's the only way to get it to work everywhere
	if ((key == tty_erase) || (key == 127) || (key == 8))
	{
		if (tty_con.cursor > 0)
		{
			tty_con.cursor--;
			tty_con.buffer[tty_con.cursor] = '\0';
			tty_Back();
		}
		ev.data1 = KEY_BACKSPACE;
	}
	else if ((key) && (key) < ' ') // check if this is a control char
	{
		if (key == '\n')
		{
			tty_Clear();
			tty_con.cursor = 0;
			ev.data1 = KEY_ENTER;
		}
		else return;
	}
	else
	{
		// push regular character
		ev.data1 = tty_con.buffer[tty_con.cursor] = key;
		tty_con.cursor++;
		// print the current line (this is differential)
		write(STDOUT_FILENO, &key, 1);
	}
	if (ev.data1) D_PostEvent(&ev);
	//tty_FlushIn();

}

#elif (defined (_WIN32) || defined (_WIN64)) && !(defined (_XBOX) || defined (_WIN32_WCE))
static inline BOOL I_ReadyConsole(HANDLE ci)
{
	DWORD gotinput;
	if (ci == (HANDLE)-1) return FALSE;
	if (WaitForSingleObject(ci,0) != WAIT_OBJECT_0) return FALSE;
	if (GetFileType(ci) != FILE_TYPE_CHAR) return FALSE;
	return (GetNumberOfConsoleInputEvents(ci, &gotinput) && gotinput);
}

static boolean entering_con_command = false;

void I_GetConsoleEvents(void)
{
	event_t ev = {0,0,0,0};
	HANDLE ci = GetStdHandle(STD_INPUT_HANDLE);
	HANDLE co = GetStdHandle(STD_OUTPUT_HANDLE);
	CONSOLE_SCREEN_BUFFER_INFO CSBI;
	INPUT_RECORD input;
	DWORD t;

	while (I_ReadyConsole(ci) && ReadConsoleInput(ci, &input, 1, &t) && t)
	{
		memset(&ev,0x00,sizeof (ev));
		switch (input.EventType)
		{
			case KEY_EVENT:
				if (input.Event.KeyEvent.bKeyDown)
				{
					ev.type = ev_console;
					entering_con_command = true;
					switch (input.Event.KeyEvent.wVirtualKeyCode)
					{						
						case VK_ESCAPE:
						case VK_TAB:
							ev.data1 = KEY_NULL;
							break;
						case VK_SHIFT:
							ev.data1 = KEY_SHIFT;
							break;
						case VK_RETURN:
							entering_con_command = false;
							// Fall through.
						default:
							ev.data1 = MapVirtualKey(input.Event.KeyEvent.wVirtualKeyCode,2); // convert in to char
					}
					if (co != (HANDLE)-1 && GetFileType(co) == FILE_TYPE_CHAR)
					{
						if (ev.data1 && ev.data1 != KEY_SHIFT)
						{
#ifdef _UNICODE
							WriteConsole(co, &input.Event.KeyEvent.uChar.UnicodeChar, 1, &t, NULL);
#else
							WriteConsole(co, &input.Event.KeyEvent.uChar.AsciiChar, 1 , &t, NULL);
#endif
						}
						if (input.Event.KeyEvent.wVirtualKeyCode == VK_BACK
							&& GetConsoleScreenBufferInfo(co,&CSBI))
						{
							WriteConsoleOutputCharacterA(co, " ",1, CSBI.dwCursorPosition, &t);
						}
					}
				}
				else
				{
					ev.type = ev_keyup;
					switch (input.Event.KeyEvent.wVirtualKeyCode)
					{
						case VK_SHIFT:
							ev.data1 = KEY_SHIFT;
							break;
						default:
							break;
					}
				}
				if (ev.data1) D_PostEvent(&ev);
				break;
			case MOUSE_EVENT:
			case WINDOW_BUFFER_SIZE_EVENT:
			case MENU_EVENT:
			case FOCUS_EVENT:
				break;
		}
	}
}

void I_StartupConsole(void)
{
	HANDLE ci, co;
	const int ded = M_CheckParm("-dedicated");
#ifdef SDLMAIN
	if (M_CheckParm("-console") || ded)
		AllocConsole();
#else
	if (M_CheckParm("-detachconsole"))
	{
		FreeConsole();
		AllocConsole();
	}
	else if (M_CheckParm("-noconsole") && !ded)
		FreeConsole();
#endif
	//Let get the real console HANDLE, because Mingw's Bash is bad!
	ci = CreateFile(TEXT("CONIN$") ,               GENERIC_READ, FILE_SHARE_READ,  NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
	co = CreateFile(TEXT("CONOUT$"), GENERIC_WRITE|GENERIC_READ, FILE_SHARE_WRITE, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
	if (ci != (HANDLE)-1)
	{
		const DWORD CM = ENABLE_LINE_INPUT|ENABLE_ECHO_INPUT|ENABLE_PROCESSED_INPUT;
		SetStdHandle(STD_INPUT_HANDLE,ci);
		if(GetFileType(ci) == FILE_TYPE_CHAR)
			SetConsoleMode(ci,CM); //default mode but no ENABLE_MOUSE_INPUT
	}
	if(co != (HANDLE)-1)
	{
		SetStdHandle(STD_OUTPUT_HANDLE,co);
		SetStdHandle(STD_ERROR_HANDLE,co); //maybe logstream?
	}
}
static inline void I_ShutdownConsole(void){}
#else
void I_GetConsoleEvents(void){}
static inline void I_StartupConsole(void){}
static inline void I_ShutdownConsole(void){}
#endif

//
// StartupKeyboard
//
void I_StartupKeyboard (void)
{
#if defined (NDEBUG) && !defined (DC)
#ifdef SIGILL
//	signal(SIGILL , signal_handler);
#endif
#ifdef SIGINT
	signal(SIGINT , quit_handler);
#endif
#ifdef SIGSEGV
//	signal(SIGSEGV , signal_handler);
#endif
#ifdef SIGBREAK
	signal(SIGBREAK , quit_handler);
#endif
#ifdef SIGABRT
//	signal(SIGABRT , signal_handler);
#endif
#ifdef SIGTERM
	signal(SIGTERM , quit_handler);
#endif
#endif
}

//
//I_OutputMsg
//
void I_OutputMsg(const char *fmt, ...)
{
	size_t len;
	XBOXSTATIC char txt[128];
	va_list  argptr;

#ifdef _arch_dreamcast
	if (!keyboard_started) conio_printf(fmt);
#endif

	va_start(argptr,fmt);
	vsprintf(txt, fmt, argptr);
	va_end(argptr);

#if (defined (_WIN32) || defined (_WIN64)) && !defined (_XBOX)
	OutputDebugStringA(txt);
#endif

	len = strlen(txt);
#if (defined (_WIN32) || defined (_WIN64)) && !defined (_XBOX) && !defined(_WIN32_WCE)
#ifdef DEBUGFILE
	if (debugfile != stderr)
#endif
	{
		HANDLE co = GetStdHandle(STD_OUTPUT_HANDLE);
		DWORD bytesWritten;
		if (co != (HANDLE)-1)
		{
			if (GetFileType(co) == FILE_TYPE_CHAR)
			{
				static COORD coordNextWrite = {0,0};
				char *oldLines = NULL;
				DWORD oldLength = 0;
				CONSOLE_SCREEN_BUFFER_INFO csbi;
			
				// Save the lines that we're going to obliterate.
				GetConsoleScreenBufferInfo(co, &csbi);
				oldLength = csbi.dwSize.X * (csbi.dwCursorPosition.Y - coordNextWrite.Y) + csbi.dwCursorPosition.X - coordNextWrite.X;

				if(oldLength > 0)
				{
					char *blank = malloc(oldLength);
					oldLines = malloc(oldLength);
					if(!oldLines || !blank) return;

					ReadConsoleOutputCharacterA(co, oldLines, oldLength, coordNextWrite, &bytesWritten);

					// Move to where we what to print - which is where we would've been,
					// had console input not been in the way,
					SetConsoleCursorPosition(co, coordNextWrite);

					// Blank out.
					memset(blank, ' ', oldLength);
					WriteConsoleA(co, blank, oldLength, &bytesWritten, NULL);
					free(blank);

					// And back to where we want to print again.
					SetConsoleCursorPosition(co, coordNextWrite);
				}

				// Actually write the string now!
				WriteConsoleA(co, txt, (DWORD)strlen(txt), &bytesWritten, NULL);

				// Next time, output where we left off.
				GetConsoleScreenBufferInfo(co, &csbi);
				coordNextWrite = csbi.dwCursorPosition;

				// Restore what was overwritten.
				if(oldLines && entering_con_command)
				{
					WriteConsoleA(co, oldLines, oldLength, &bytesWritten, NULL);
					free(oldLines);
				}
			}
			else	// Redirected to a file.
				WriteFile(co, txt, (DWORD)len, &bytesWritten, NULL);
		}
	}
#else
#ifdef HAVE_TERMIOS
	if (consolevent)
	{
		tty_Hide();
	}
#endif

	fprintf(stderr, "%s", txt);
#ifdef HAVE_TERMIOS
	if (consolevent)
	{
		tty_Show();
	}
#endif
	
#endif

#ifdef LOGMESSAGES
	if (logstream != INVALID_HANDLE_VALUE)
	{
#ifdef SDLIO
		SDL_RWwrite(logstream, txt, (int)len, 1);
#else
		write(logstream, txt, (unsigned int)len);
#endif
	}
#endif

	// 2004-03-03 AJR Since not all messages end in newline, some were getting displayed late.	
	fflush(stderr);
}

//
// I_GetKey
//
int I_GetKey (void)
{
	// Warning: I_GetKey empties the event queue till next keypress
	event_t *ev;
	int rc = 0;

	// return the first keypress from the event queue
	for (; eventtail != eventhead; eventtail = (eventtail+1)&(MAXEVENTS-1))
	{
		ev = &events[eventtail];
		if (ev->type == ev_keydown || ev->type == ev_console)
		{
			rc = ev->data1;
			continue;
		}
	}

	return rc;
}

//
// I_JoyScale
//
void I_JoyScale(void)
{
	Joystick.bGamepadStyle = !cv_joyscale.value;
	JoyInfo.scale = (Joystick.bGamepadStyle)?1:cv_joyscale.value;
}

void I_JoyScale2(void)
{
	Joystick2.bGamepadStyle = !cv_joyscale2.value;
	JoyInfo2.scale = (Joystick2.bGamepadStyle)?1:cv_joyscale2.value;
}

/**	\brief Joystick 1 buttons states
*/
static INT64 lastjoybuttons = 0;

/**	\brief Joystick 1 hats state
*/
static INT64 lastjoyhats = 0;

/**	\brief	Shuts down joystick 1


	\return void

	
*/
static void I_ShutdownJoystick(void)
{
	int i;
	event_t event;
	event.type=ev_keyup;
	event.data2 = 0;
	event.data3 = 0;

	lastjoybuttons = lastjoyhats = 0;

	// emulate the up of all joystick buttons
	for (i=0;i<JOYBUTTONS;i++)
	{
		event.data1=KEY_JOY1+i;
		D_PostEvent(&event);
	}

	// emulate the up of all joystick hats
	for (i=0;i<JOYHATS*4;i++)
	{
		event.data1=KEY_HAT1+i;
		D_PostEvent(&event);
	}

	// reset joystick position
	event.type = ev_joystick;
	for (i=0;i<JOYAXISSET; i++)
	{
		event.data1 = i;
		D_PostEvent(&event);
	}

	joystick_started = 0;
	JoyReset(&JoyInfo);
	if (!joystick_started && !joystick2_started && SDL_WasInit(SDL_INIT_JOYSTICK)!=0)
	{
		SDL_QuitSubSystem(SDL_INIT_JOYSTICK);
		if (cv_usejoystick.value==0) CONS_Printf("I_Joystick: SDL's Joystick system has been shutdown\n");
	}
}

void I_GetJoystickEvents(void)
{
	static event_t event = {0,0,0,0};
	int i = 0;
	INT64 joybuttons = 0;
	INT64 joyhats = 0;
	int axisx, axisy;

	if (!joystick_started) return;

	if (!JoyInfo.dev) //I_ShutdownJoystick();
		return;

	//faB: look for as much buttons as g_input code supports,
	//  we don't use the others
	for (i = JoyInfo.buttons - 1; i >= 0; i--)
	{
		joybuttons <<= 1;
		if (SDL_JoystickGetButton(JoyInfo.dev,i))
			joybuttons |= 1;
	}

	for (i = JoyInfo.hats - 1; i >= 0; i--)
	{
		int hat = SDL_JoystickGetHat(JoyInfo.dev, i);

		if (hat & SDL_HAT_UP   ) joyhats|=1<<(0 + 4*i);
		if (hat & SDL_HAT_DOWN ) joyhats|=1<<(1 + 4*i);
		if (hat & SDL_HAT_LEFT ) joyhats|=1<<(2 + 4*i);
		if (hat & SDL_HAT_RIGHT) joyhats|=1<<(3 + 4*i);
	}

	if (joybuttons != lastjoybuttons)
	{
		INT64 j = 1; // keep only bits that changed since last time
		INT64 newbuttons = joybuttons ^ lastjoybuttons;
		lastjoybuttons = joybuttons;

		for (i = 0; i < JOYBUTTONS; i++, j <<= 1)
		{
			if (newbuttons & j) // button changed state?
			{
				if (joybuttons & j)
					event.type = ev_keydown;
				else
					event.type = ev_keyup;
#ifdef _PSP_
				if (i == 12)
					event.data1 = KEY_ESCAPE;
				else
#endif
				event.data1 = KEY_JOY1 + i;
				D_PostEvent(&event);
			}
		}
	}

	if (joyhats != lastjoyhats)
	{
		INT64 j = 1; // keep only bits that changed since last time
		INT64 newhats = joyhats ^ lastjoyhats;
		lastjoyhats = joyhats;

		for (i = 0; i < JOYHATS*4; i++, j <<= 1)
		{
			if (newhats & j) // hat changed state?
			{
				if (joyhats & j)
					event.type = ev_keydown;
				else
					event.type = ev_keyup;
				event.data1 = KEY_HAT1 + i;
				D_PostEvent(&event);
			}
		}
	}

	// send joystick axis positions
	event.type = ev_joystick;

	for (i = JOYAXISSET - 1; i >= 0; i--)
	{
		event.data1 = i;
		if (i*2 + 1 <= JoyInfo.axises)
			axisx = SDL_JoystickGetAxis(JoyInfo.dev, i*2 + 0);
		else axisx = 0;
		if (i*2 + 2 <= JoyInfo.axises)
			axisy = SDL_JoystickGetAxis(JoyInfo.dev, i*2 + 1);
		else axisy = 0;

#ifdef _arch_dreamcast // -128 to 127
		axisx = axisx*8;
		axisy = axisy*8;
#else // -32768 to 32767
		axisx = axisx/32;
		axisy = axisy/32;
#endif

		if (Joystick.bGamepadStyle)
		{
			// gamepad control type, on or off, live or die
			if (axisx < -(JOYAXISRANGE/2))
				event.data2 = -1;
			else if (axisx > (JOYAXISRANGE/2))
				event.data2 = 1;
			else event.data2 = 0;
			if (axisy < -(JOYAXISRANGE/2))
				event.data3 = -1;
			else if (axisy > (JOYAXISRANGE/2))
				event.data3 = 1;
			else event.data3 = 0;
		}
		else
		{

			axisx = JoyInfo.scale?((axisx/JoyInfo.scale)*JoyInfo.scale):axisx;
			axisy = JoyInfo.scale?((axisy/JoyInfo.scale)*JoyInfo.scale):axisy;

			if (-SDL_JDEADZONE <= axisx && axisx <= SDL_JDEADZONE) axisx = 0;
			if (-SDL_JDEADZONE <= axisy && axisy <= SDL_JDEADZONE) axisy = 0;

			// analog control style , just send the raw data
			event.data2 = axisx; // x axis
			event.data3 = axisy; // y axis
		}
		D_PostEvent(&event);
	}
}

/**	\brief	Open joystick handle

	\param	fname	name of joystick

	\return	axises

	
*/
static int joy_open(const char *fname)
{
	int joyindex = atoi(fname);
	int num_joy = 0;
	int i;

	if (joystick_started == 0 && joystick2_started == 0)
	{
		if (SDL_InitSubSystem(SDL_INIT_JOYSTICK) == -1)
		{
			CONS_Printf("Couldn't initialize SDL Joystick: %s\n", SDL_GetError());
			return -1;
		}
		else
		{
			num_joy = SDL_NumJoysticks();
		}

		if (num_joy < joyindex)
		{
			CONS_Printf("Unable to use that joystick #%d/(%s), it doesn't exist\n",joyindex,fname);
			for (i = 0; i < num_joy; i++)
				CONS_Printf("#: %d, Name: %s\n", i, SDL_JoystickName(i));
			I_ShutdownJoystick();
			return -1;
		}
	}
	else
	{
		JoyReset(&JoyInfo);
		//I_ShutdownJoystick();
		//joy_open(fname);
	}
	
	num_joy = SDL_NumJoysticks();

	if (joyindex <= 0 || num_joy == 0 || JoyInfo.oldjoy == joyindex)
	{
//		CONS_Printf("Unable to use that joystick #(%s), non-number\n",fname);
		if (num_joy != 0)
		{
			CONS_Printf("Hmmm, I was able to find %d joysticks on this system\n", num_joy);
			for (i = 0; i < num_joy; i++)
				CONS_Printf("#: %d, Name: %s\n", i+1, SDL_JoystickName(i));
		}
		else
			CONS_Printf("Hmm, I was unable to found any joysticks on this system\n");
		if (joyindex <= 0 || num_joy == 0) return 0;
	}

	JoyInfo.dev = SDL_JoystickOpen(joyindex-1);
	CONS_Printf("Joystick: %s\n",SDL_JoystickName(joyindex-1));

	if (JoyInfo.dev == NULL)
	{
		CONS_Printf("Couldn't open joystick: %s\n", SDL_GetError());
		I_ShutdownJoystick();
		return -1;
	}
	else
	{
		JoyInfo.axises = SDL_JoystickNumAxes(JoyInfo.dev);
		if (JoyInfo.axises > JOYAXISSET*2)
			JoyInfo.axises = JOYAXISSET*2;
/*		if (joyaxes<2)
		{
			CONS_Printf("Not enought axes?\n");
			I_ShutdownJoystick();
			return 0;
		}*/

		JoyInfo.buttons = SDL_JoystickNumButtons(JoyInfo.dev);
		if (JoyInfo.buttons > JOYBUTTONS)
			JoyInfo.buttons = JOYBUTTONS;

#ifdef DC
		JoyInfo.hats = 0;
#else
		JoyInfo.hats = SDL_JoystickNumHats(JoyInfo.dev);
		if (JoyInfo.hats > JOYHATS)
			JoyInfo.hats = JOYHATS;

		JoyInfo.balls = SDL_JoystickNumBalls(JoyInfo.dev);
#endif

		//Joystick.bGamepadStyle = !strcmp(SDL_JoystickName(SDL_JoystickIndex(JoyInfo.dev)), "Pad");

		return JoyInfo.axises;
	}
}

//Joystick2

/**	\brief Joystick 2 buttons states
*/
static INT64 lastjoy2buttons = 0;

/**	\brief Joystick 2 hats state
*/
static INT64 lastjoy2hats = 0;

/**	\brief	Shuts down joystick 2


	\return	void

	
*/
static void I_ShutdownJoystick2(void)
{
	int i;
	event_t event;
	event.type = ev_keyup;
	event.data2 = 0;
	event.data3 = 0;

	lastjoy2buttons = lastjoy2hats = 0;

	// emulate the up of all joystick buttons
	for (i = 0; i < JOYBUTTONS; i++)
	{
		event.data1 = KEY_2JOY1 + i;
		D_PostEvent(&event);
	}

	// emulate the up of all joystick hats
	for (i = 0; i < JOYHATS*4; i++)
	{
		event.data1 = KEY_2HAT1 + i;
		D_PostEvent(&event);
	}

	// reset joystick position
	event.type = ev_joystick2;
	for (i = 0; i < JOYAXISSET; i++)
	{
		event.data1 = i;
		D_PostEvent(&event);
	}

	JoyReset(&JoyInfo2);
	if (!joystick_started && !joystick2_started && SDL_WasInit(SDL_INIT_JOYSTICK)!=0)
	{
		SDL_QuitSubSystem(SDL_INIT_JOYSTICK);
		if (cv_usejoystick2.value == 0) CONS_Printf("I_Joystick2: SDL's Joystick system has been shutdown\n");
	}
}

void I_GetJoystick2Events(void)
{
	static event_t event = {0,0,0,0};
	int i = 0;
	INT64 joybuttons = 0;
	INT64 joyhats = 0;
	int axisx, axisy;

	if (!joystick2_started)
		return;

	if (!JoyInfo2.dev) //I_ShutdownJoystick2();
		return;
	

	//faB: look for as much buttons as g_input code supports,
	//  we don't use the others
	for (i = JoyInfo2.buttons - 1; i >= 0; i--)
	{
		joybuttons <<= 1;
		if (SDL_JoystickGetButton(JoyInfo2.dev,i))
			joybuttons |= 1;
	}

	for (i = JoyInfo2.hats - 1; i >= 0; i--)
	{
		int hat = SDL_JoystickGetHat(JoyInfo2.dev, i);

		if (hat & SDL_HAT_UP   ) joyhats|=1<<(0 + 4*i);
		if (hat & SDL_HAT_DOWN ) joyhats|=1<<(1 + 4*i);
		if (hat & SDL_HAT_LEFT ) joyhats|=1<<(2 + 4*i);
		if (hat & SDL_HAT_RIGHT) joyhats|=1<<(3 + 4*i);
	}

	if (joybuttons != lastjoy2buttons)
	{
		INT64 j = 1; // keep only bits that changed since last time
		INT64 newbuttons = joybuttons ^ lastjoy2buttons;
		lastjoy2buttons = joybuttons;

		for (i = 0; i < JOYBUTTONS; i++, j <<= 1)
		{
			if (newbuttons & j) // button changed state?
			{
				if (joybuttons & j)
					event.type = ev_keydown;
				else
					event.type = ev_keyup;
				event.data1 = KEY_2JOY1 + i;
				D_PostEvent(&event);
			}
		}
	}

	if (joyhats != lastjoy2hats)
	{
		INT64 j = 1; // keep only bits that changed since last time
		INT64 newhats = joyhats ^ lastjoy2hats;
		lastjoy2hats = joyhats;

		for (i = 0; i < JOYHATS*4; i++, j <<= 1)
		{
			if (newhats & j) // hat changed state?
			{
				if (joyhats & j)
					event.type = ev_keydown;
				else
					event.type = ev_keyup;
				event.data1 = KEY_2HAT1 + i;
				D_PostEvent(&event);
			}
		}
	}

	// send joystick axis positions
	event.type = ev_joystick2;

	for (i = JOYAXISSET - 1; i >= 0; i--)
	{
		event.data1 = i;
		if (i*2 + 1 <= JoyInfo2.axises)
			axisx = SDL_JoystickGetAxis(JoyInfo2.dev, i*2 + 0);
		else axisx = 0;
		if (i*2 + 2 <= JoyInfo2.axises)
			axisy = SDL_JoystickGetAxis(JoyInfo2.dev, i*2 + 1);
		else axisy = 0;

#ifdef _arch_dreamcast // -128 to 127
		axisx = axisx*8;
		axisy = axisy*8;
#else // -32768 to 32767
		axisx = axisx/32;
		axisy = axisy/32;
#endif

		if (Joystick2.bGamepadStyle)
		{
			// gamepad control type, on or off, live or die
			if (axisx < -(JOYAXISRANGE/2))
				event.data2 = -1;
			else if (axisx > (JOYAXISRANGE/2))
				event.data2 = 1;
			else
				event.data2 = 0;
			if (axisy < -(JOYAXISRANGE/2))
				event.data3 = -1;
			else if (axisy > (JOYAXISRANGE/2))
				event.data3 = 1;
			else
				event.data3 = 0;
		}
		else
		{

			axisx = JoyInfo2.scale?((axisx/JoyInfo2.scale)*JoyInfo2.scale):axisx;
			axisy = JoyInfo2.scale?((axisy/JoyInfo2.scale)*JoyInfo2.scale):axisy;

			if (-SDL_JDEADZONE <= axisx && axisx <= SDL_JDEADZONE) axisx = 0;
			if (-SDL_JDEADZONE <= axisy && axisy <= SDL_JDEADZONE) axisy = 0;

			// analog control style , just send the raw data
			event.data2 = axisx; // x axis
			event.data3 = axisy; // y axis
		}
		D_PostEvent(&event);
	}

}

/**	\brief	Open joystick handle

	\param	fname	name of joystick

	\return	axises

	
*/
static int joy_open2(const char *fname)
{
	int joyindex = atoi(fname);
	int num_joy = 0;
	int i;

	if (joystick_started == 0 && joystick2_started == 0)
	{
		if (SDL_InitSubSystem(SDL_INIT_JOYSTICK) == -1)
		{
			CONS_Printf("Couldn't initialize SDL Joystick: %s\n", SDL_GetError());
			return -1;
		}
		else
			num_joy = SDL_NumJoysticks();

		if (num_joy < joyindex)
		{
			CONS_Printf("Unable to use that joystick #%d/(%s), it doesn't exist\n",joyindex,fname);
			for (i = 0; i < num_joy; i++)
				CONS_Printf("#: %d, Name: %s\n", i, SDL_JoystickName(i));
			I_ShutdownJoystick2();
			return -1;
		}
	}
	else
	{
		JoyReset(&JoyInfo2);
		//I_ShutdownJoystick();
		//joy_open(fname);
	}

	num_joy = SDL_NumJoysticks();

	if (joyindex <= 0 || num_joy == 0 || JoyInfo2.oldjoy == joyindex)
	{
//		CONS_Printf("Unable to use that joystick #(%s), non-number\n",fname);
		if (num_joy != 0)
		{
			CONS_Printf("Hmmm, I was able to find %d joysticks on this system\n", num_joy);
			for (i = 0; i < num_joy; i++)
				CONS_Printf("#: %d, Name: %s\n", i+1, SDL_JoystickName(i));
		}
		else
			CONS_Printf("Hmm, I was unable to found any joysticks on this system\n");
		if (joyindex <= 0 || num_joy == 0) return 0;
	}

	JoyInfo2.dev = SDL_JoystickOpen(joyindex-1);
	CONS_Printf("Joystick2: %s\n", SDL_JoystickName(joyindex-1));

	if (!JoyInfo2.dev)
	{
		CONS_Printf("Couldn't open joystick2: %s\n", SDL_GetError());
		I_ShutdownJoystick2();
		return -1;
	}
	else
	{
		JoyInfo2.axises = SDL_JoystickNumAxes(JoyInfo2.dev);
		if (JoyInfo2.axises > JOYAXISSET*2)
			JoyInfo2.axises = JOYAXISSET*2;
/*		if (joyaxes < 2)
		{
			CONS_Printf("Not enought axes?\n");
			I_ShutdownJoystick2();
			return 0;
		}*/

		JoyInfo2.buttons = SDL_JoystickNumButtons(JoyInfo2.dev);
		if (JoyInfo2.buttons > JOYBUTTONS)
			JoyInfo2.buttons = JOYBUTTONS;

#ifdef DC
		JoyInfo2.hats = 0;
#else
		JoyInfo2.hats = SDL_JoystickNumHats(JoyInfo2.dev);
		if (JoyInfo2.hats > JOYHATS)
			JoyInfo2.hats = JOYHATS;

		JoyInfo2.balls = SDL_JoystickNumBalls(JoyInfo2.dev);
#endif

		//Joystick.bGamepadStyle = !strcmp(SDL_JoystickName(SDL_JoystickIndex(JoyInfo2.dev)), "Pad");

		return JoyInfo2.axises;
	}
}

//
// I_InitJoystick
//
void I_InitJoystick(void)
{
	I_ShutdownJoystick();
	if (!strcmp(cv_usejoystick.string, "0") || M_CheckParm("-nojoy"))
		return;
	if (joy_open(cv_usejoystick.string) != -1)
		JoyInfo.oldjoy = atoi(cv_usejoystick.string);
	else
	{
		cv_usejoystick.value = 0;
		return;
	}
	joystick_started = 1;
}

void I_InitJoystick2 (void)
{
	I_ShutdownJoystick2();
	if (!strcmp(cv_usejoystick2.string, "0") || M_CheckParm("-nojoy"))
		return;
	if (joy_open2(cv_usejoystick2.string) != -1)
		JoyInfo2.oldjoy = atoi(cv_usejoystick2.string);
	else
	{
		cv_usejoystick2.value = 0;
		return;
	}
	joystick2_started = 1;
}

int I_NumJoys(void)
{
	int numjoy = 0;
	if (SDL_WasInit(SDL_INIT_JOYSTICK) == 0)
	{
		if (SDL_InitSubSystem(SDL_INIT_JOYSTICK) != -1)
			numjoy = SDL_NumJoysticks();
		SDL_QuitSubSystem(SDL_INIT_JOYSTICK);
	}
	else
		numjoy = SDL_NumJoysticks();
	return numjoy;
}

const char *I_GetJoyName(int joyindex)
{
	const char *joyname = "NA";
	joyindex--; //SDL's Joystick System starts at 0, not 1
	if (SDL_WasInit(SDL_INIT_JOYSTICK) == 0)
	{
		if (SDL_InitSubSystem(SDL_INIT_JOYSTICK) != -1)
			joyname = SDL_JoystickName(joyindex);
		SDL_QuitSubSystem(SDL_INIT_JOYSTICK);
	}
	else
		joyname = SDL_JoystickName(joyindex);
	return joyname;
}

#ifdef HAVE_TERMIOS

void I_GetMouseEvents(void)
{
	static unsigned char mdata[5];
	static int i = 0,om2b = 0;
	int di, j, mlp, button;
	event_t event;
	const int mswap[8] = {0, 4, 1, 5, 2, 6, 3, 7};

	if (!mouse2_started) return;
	for (mlp = 0; mlp < 20; mlp++)
	{
		for (; i < 5; i++)
		{
			di = read(fdmouse2, mdata+i, 1);
			if (di == -1) return;
		}
		if ((mdata[0] & 0xf8) != 0x80)
		{
			for (j = 1; j < 5; j++)
				if ((mdata[j] & 0xf8) == 0x80)
					for (i = 0; i < 5-j; i++) // shift
						mdata[i] = mdata[i+j];
			if (i < 5) continue;
		}
		else
		{
			button = mswap[~mdata[0] & 0x07];
			for (j = 0; j < MOUSEBUTTONS; j++)
			{
				if (om2b & (1<<j))
				{
					if (!(button & (1<<j))) //keyup
					{
						event.type = ev_keyup;
						event.data1 = KEY_2MOUSE1+j;
						D_PostEvent(&event);
						om2b ^= 1 << j;
					}
				}
				else
				{
					if (button & (1<<j))
					{
						event.type = ev_keydown;
						event.data1 = KEY_2MOUSE1+j;
						D_PostEvent(&event);
						om2b ^= 1 << j;
					}
				}
			}
			event.data2 = ((signed char)mdata[1])+((signed char)mdata[3]);
			event.data3 = ((signed char)mdata[2])+((signed char)mdata[4]);
			if (event.data2 && event.data3)
			{
				event.type = ev_mouse2;
				event.data1 = 0;
				D_PostEvent(&event);
			}
		}
		i = 0;
	}
}

//
// I_ShutdownMouse2
//
static void I_ShutdownMouse2(void)
{
	if (fdmouse2 != -1) close(fdmouse2);
	mouse2_started = 0;
}
#elif (defined (_WIN32) || defined (_WIN64)) && !defined (_XBOX)

static HANDLE mouse2filehandle = (HANDLE)(-1);

static void I_ShutdownMouse2(void)
{
	event_t event;
	int i;

	if (mouse2filehandle == (HANDLE)(-1))
		return;

	SetCommMask(mouse2filehandle, 0);

	EscapeCommFunction(mouse2filehandle, CLRDTR);
	EscapeCommFunction(mouse2filehandle, CLRRTS);

	PurgeComm(mouse2filehandle, PURGE_TXABORT | PURGE_RXABORT |
		 PURGE_TXCLEAR | PURGE_RXCLEAR);

	CloseHandle(mouse2filehandle);

	// emulate the up of all mouse buttons
	for (i = 0; i < MOUSEBUTTONS; i++)
	{
		event.type = ev_keyup;
		event.data1 = KEY_2MOUSE1+i;
		D_PostEvent(&event);
	}

	mouse2filehandle = (HANDLE)(-1);
}

#define MOUSECOMBUFFERSIZE 256
static int handlermouse2x,handlermouse2y,handlermouse2buttons;

static void I_PoolMouse2(void)
{
	byte buffer[MOUSECOMBUFFERSIZE];
	COMSTAT ComStat;
	DWORD dwErrorFlags;
	DWORD dwLength;
	char dx,dy;

	static int bytenum;
	static byte combytes[4];
	DWORD i;

	ClearCommError(mouse2filehandle, &dwErrorFlags, &ComStat);
	dwLength = min(MOUSECOMBUFFERSIZE, ComStat.cbInQue);

	if (dwLength <= 0)
		return;

	if (!ReadFile(mouse2filehandle, buffer, dwLength, &dwLength, NULL))
	{
		CONS_Printf("\2Read Error on secondary mouse port\n");
		return;
	}

	// parse the mouse packets
	for (i = 0; i < dwLength; i++)
	{
		if ((buffer[i] & 64)== 64)
			bytenum = 0;
			
		if (bytenum < 4)
			combytes[bytenum] = buffer[i];
		bytenum++;

		if (bytenum == 1)
		{
			handlermouse2buttons &= ~3;
			handlermouse2buttons |= ((combytes[0] & (32+16)) >> 4);
		}
		else if (bytenum == 3)
		{
			dx = (char)((combytes[0] &  3) << 6);
			dy = (char)((combytes[0] & 12) << 4);
			dx = (char)(dx + combytes[1]);
			dy = (char)(dy + combytes[2]);
			handlermouse2x+= dx;
			handlermouse2y+= dy;
		}
		else if (bytenum == 4) // fourth byte (logitech mouses)
		{
			if (buffer[i] & 32)
				handlermouse2buttons |= 4;
			else
				handlermouse2buttons &= ~4;
		}
	}
}

void I_GetMouseEvents(void)
{
	static byte lastbuttons2 = 0; //mouse movement
	event_t event;

	if (mouse2filehandle == (HANDLE)(-1))
		return;

	I_PoolMouse2();
	// post key event for buttons
	if (handlermouse2buttons != lastbuttons2)
	{
		int i, j = 1, k;
		k = (handlermouse2buttons ^ lastbuttons2); // only changed bit to 1
		lastbuttons2 = (byte)handlermouse2buttons;

		for (i = 0; i < MOUSEBUTTONS; i++, j <<= 1)
			if (k & j)
			{
				if (handlermouse2buttons & j)
					event.type = ev_keydown;
				else
					event.type = ev_keyup;
				event.data1 = KEY_2MOUSE1+i;
				D_PostEvent(&event);
			}
	}

	if (handlermouse2x != 0 || handlermouse2y != 0)
	{
		event.type = ev_mouse2;
		event.data1 = 0;
//		event.data1 = buttons; // not needed
		event.data2 = handlermouse2x << 1;
		event.data3 = -handlermouse2y << 1;
		handlermouse2x = 0;
		handlermouse2y = 0;

		D_PostEvent(&event);
	}
}
#else
void I_GetMouseEvents(void){};
#endif

//
// I_StartupMouse2
// 
void I_StartupMouse2(void)
{
#ifdef HAVE_TERMIOS
	struct termios m2tio;
	size_t i;
	int dtr = -1, rts = -1;;
	I_ShutdownMouse2();
	if (cv_usemouse2.value == 0) return;
	if ((fdmouse2 = open(cv_mouse2port.string, O_RDONLY|O_NONBLOCK|O_NOCTTY)) == -1)
	{
		CONS_Printf("Error opening %s!\n", cv_mouse2port.string);
		return;
	}
	tcflush(fdmouse2, TCIOFLUSH);
	m2tio.c_iflag = IGNBRK;
	m2tio.c_oflag = 0;
	m2tio.c_cflag = CREAD|CLOCAL|HUPCL|CS8|CSTOPB|B1200;
	m2tio.c_lflag = 0;
	m2tio.c_cc[VTIME] = 0;
	m2tio.c_cc[VMIN] = 1;
	tcsetattr(fdmouse2, TCSANOW, &m2tio);
	for (i = 0; i < strlen(cv_mouse2opt.string); i++)
	{
		if (toupper(cv_mouse2opt.string[i]) == 'D')
		{
			if (cv_mouse2opt.string[i+1] == '-')
				dtr = 0;
			else
				dtr = 1;
		}
		if (toupper(cv_mouse2opt.string[i]) == 'R')
		{
			if (cv_mouse2opt.string[i+1] == '-')
				rts = 0;
			else
				rts = 1;
		}
		if (dtr != -1 || rts != -1)
		{
			int c;
			if (!ioctl(fdmouse2, TIOCMGET, &c))
			{
				if (!dtr)
					c &= ~TIOCM_DTR;
				else if (dtr > 0)
					c |= TIOCM_DTR;
			}
			if (!rts)
				c &= ~TIOCM_RTS;
			else if (rts > 0)
				c |= TIOCM_RTS;
			ioctl(fdmouse2, TIOCMSET, &c);
		}
	}
	mouse2_started = 1;
	I_AddExitFunc(I_ShutdownMouse2);
#elif (defined (_WIN32) || defined (_WIN64)) && !defined (_XBOX)
	DCB dcb;

	if (mouse2filehandle != (HANDLE)(-1))
		I_ShutdownMouse2();

	if (cv_usemouse2.value == 0)
		return;

	if (mouse2filehandle == (HANDLE)(-1))
	{
		// COM file handle
		mouse2filehandle = CreateFileA(cv_mouse2port.string, GENERIC_READ | GENERIC_WRITE,
		                               0,                     // exclusive access
		                               NULL,                  // no security attrs
		                               OPEN_EXISTING,
		                               FILE_ATTRIBUTE_NORMAL, 
		                               NULL);
		if (mouse2filehandle == (HANDLE)(-1))
		{
			int e = GetLastError();
			if (e == 5)
				CONS_Printf("\2Can't open %s: Access denied\n"
				            "The port is probably already used by one other device (mouse, modem,...)\n", cv_mouse2port.string);
			else
				CONS_Printf("\2Can't open %s: error %d\n", cv_mouse2port.string, e);
			return;
		}
	}

	// getevent when somthing happens
	//SetCommMask(mouse2filehandle, EV_RXCHAR);

	// buffers
	SetupComm(mouse2filehandle, MOUSECOMBUFFERSIZE, MOUSECOMBUFFERSIZE);

	// purge buffers
	PurgeComm(mouse2filehandle, PURGE_TXABORT | PURGE_RXABORT
	          | PURGE_TXCLEAR | PURGE_RXCLEAR);

	// setup port to 1200 7N1
	dcb.DCBlength = sizeof (DCB);

	GetCommState(mouse2filehandle, &dcb);

	dcb.BaudRate = CBR_1200;
	dcb.ByteSize = 7;
	dcb.Parity = NOPARITY;
	dcb.StopBits = ONESTOPBIT;

	dcb.fDtrControl = DTR_CONTROL_ENABLE;
	dcb.fRtsControl = RTS_CONTROL_ENABLE;

	dcb.fBinary = TRUE;
	dcb.fParity = TRUE;

	SetCommState(mouse2filehandle, &dcb);
	I_AddExitFunc(I_ShutdownMouse2);
#endif
}

//
// I_Tactile
//
void I_Tactile(FFType pFFType, const JoyFF_t *FFEffect)
{
	// UNUSED.
	pFFType = EvilForce;
	FFEffect = NULL;
}

void I_Tactile2(FFType pFFType, const JoyFF_t *FFEffect)
{
	// UNUSED.
	pFFType = EvilForce;
	FFEffect = NULL;
}

/**	\brief empty ticcmd for player 1
*/
static ticcmd_t emptycmd;

ticcmd_t *I_BaseTiccmd(void)
{
	return &emptycmd;
}

/**	\brief empty ticcmd for player 2
*/
static ticcmd_t emptycmd2;

ticcmd_t *I_BaseTiccmd2(void)
{
	return &emptycmd2;
}

#if ((defined (_WIN32) && !defined (_WIN32_WCE)) || defined (_WIN64)) && !defined (_XBOX)
static DWORD starttickcount = 0; // hack for win2k time bug
static MyFunc2 pfntimeGetTime = NULL;

// ---------
// I_GetTime
// Use the High Resolution Timer if available,
// else use the multimedia timer which has 1 millisecond precision on Windowz 95,
// but lower precision on Windows NT
// ---------

tic_t I_GetTime(void)
{
	tic_t newtics = 0;

	if (!starttickcount) // high precision timer
	{
		LARGE_INTEGER currtime; // use only LowPart if high resolution counter is not available
		static LARGE_INTEGER basetime = {{0, 0}};

		// use this if High Resolution timer is found
		static LARGE_INTEGER frequency;

		if (!basetime.LowPart)
		{
			if (!QueryPerformanceFrequency(&frequency))
				frequency.QuadPart = 0;
			else
				QueryPerformanceCounter(&basetime);
		}

		if (frequency.LowPart && QueryPerformanceCounter(&currtime))
		{
			newtics = (int)((currtime.QuadPart - basetime.QuadPart) * TICRATE
				/ frequency.QuadPart);
		}
		else if (pfntimeGetTime)
		{
			currtime.LowPart = pfntimeGetTime();
			if (!basetime.LowPart)
				basetime.LowPart = currtime.LowPart;
			newtics = ((currtime.LowPart - basetime.LowPart)/(1000/TICRATE));
		}
	}
	else
		newtics = (GetTickCount() - starttickcount)/(1000/TICRATE);

	return newtics;
}
#else
//
// I_GetTime
// returns time in 1/TICRATE second tics
//
tic_t I_GetTime (void)
{
#ifdef _arch_dreamcast
	static Uint64 basetime = 0;
	       Uint64 ticks = timer_ms_gettime64(); //using timer_ms_gettime64 instand of SDL_GetTicks for the Dreamcast
#else
	static Uint32 basetime = 0;
	       Uint32 ticks = SDL_GetTicks();
#endif

	if (!basetime)
		basetime = ticks;

	ticks -= basetime;

	ticks = (ticks*TICRATE);

#ifdef _WIN32_WCE
	ticks = (ticks/10);
#else
	ticks = (ticks/1000);
#endif

	return (tic_t)ticks;
}
#endif

//
//I_StartupTimer
//
void I_StartupTimer(void)
{
#if ((defined (_WIN32) && !defined (_WIN32_WCE)) || defined (_WIN64)) && !defined (_XBOX)
	// for win2k time bug
	if (M_CheckParm("-gettickcount"))
	{
		starttickcount = GetTickCount();
		CONS_Printf("Using GetTickCount()\n");
	}
	{
		HINSTANCE h = LoadLibraryA("winmm.dll");
		if (h)
		{
			pfntimeGetTime = (MyFunc2)GetProcAddress(h, "timeGetTime");
			FreeLibrary(h);
		}
	}
#elif defined (_arch_dreamcast)

#else
	if (SDL_InitSubSystem(SDL_INIT_TIMER) < 0)
		I_Error("SRB2: Needs SDL_Timer, Error: %s", SDL_GetError());
#endif
}



void I_Sleep(void)
{
#if !(defined (_arch_dreamcast) || defined (_XBOX))
	if (cv_sleep.value != -1)
		SDL_Delay(cv_sleep.value);
#endif
}

//
// I_Init
//
#if 0
void I_Init(void)
{
	char title[30];

	I_StartupSound();
	I_InitMusic();

	sprintf(title, "SRB2 %s", VERSIONSTRING);
	SDL_WM_SetCaption(title, "SRB2"); // Window title
}
#endif

//
//
//

int I_StartupSystem(void)
{
	SDL_version SDLcompiled;
	const SDL_version *SDLlinked;
#ifdef _XBOX
#ifdef __GNUC__
	char DP[] ="      Sonic Robo Blast 2!\n";
	debugPrint(DP);
#endif
	unlink("e:/Games/SRB2/stdout.txt");
	freopen("e:/Games/SRB2/stdout.txt", "w+", stdout);
	unlink("e:/Games/SRB2/stderr.txt");
	freopen("e:/Games/SRB2/stderr.txt", "w+", stderr);
#endif
#ifdef _arch_dreamcast
#ifdef _DEBUG
	//gdb_init();
#endif
	pvr_init_defaults(); //CONS_Printf(__FILE__":%i\n",__LINE__);
#ifdef _DEBUG
	//gdb_breakpoint();
#endif
	{
		char title[] = "SRB2 for Dreamcast!\n";
		__set_fpscr(0x00040000); /* ignore FPU underflow */
		//printf("\nHello world!\n\n");
		conio_init(CONIO_TTY_PVR, CONIO_INPUT_LINE);
		conio_set_theme(CONIO_THEME_MATRIX);
		conio_clear();
		conio_putstr(title);
		//printf("\nHello world!\n\n");
	}
#endif
	SDL_VERSION(&SDLcompiled)
	SDLlinked = SDL_Linked_Version();
	CONS_Printf("Compiled for SDL version: %d.%d.%d\n",
                        SDLcompiled.major, SDLcompiled.minor, SDLcompiled.patch);
	CONS_Printf("Linked with SDL version: %d.%d.%d\n",
                        SDLlinked->major, SDLlinked->minor, SDLlinked->patch);
	if (SDL_Init(SDL_INIT_NOPARACHUTE) < 0)
		I_Error("SRB2: SDL System Error: %s", SDL_GetError()); //Alam: Oh no....
	I_StartupConsole();
	return 0;
}


//
// I_Quit
//
void I_Quit(void)
{
	static int quiting = 0;

	/* prevent recursive I_Quit() */
	if (quiting) exit(1);
	quiting = true;
	I_ShutdownConsole();
	M_SaveConfig(NULL); //save game config, cvars..
	G_SaveGameData(); // Tails 12-08-2002
	//added:16-02-98: when recording a demo, should exit using 'q' key,
	//        but sometimes we forget and use 'F10'.. so save here too.
	if (demorecording)
		G_CheckDemoStatus();
	D_QuitNetGame();
	I_ShutdownMusic();
	I_ShutdownSound();
	I_ShutdownCD();
	// use this for 1.28 19990220 by Kin
	I_ShutdownGraphics();
#ifndef _arch_dreamcast
	SDL_Quit();
#endif
	I_ShutdownSystem();
	/* if option -noendtxt is set, don't print the text */
	if (!M_CheckParm("-noendtxt") && W_CheckNumForName("ENDOOM") != -1)
	{
		printf("\r");
		ShowEndTxt();
	}
	exit(0);
}

void I_WaitVBL(int count)
{
	count = 1;
	SDL_Delay(count);
}

void I_BeginRead(void)
{
}

void I_EndRead(void)
{
}

byte *I_AllocLow(int length)
{
	byte *mem;

	mem = (byte *)malloc(length);
	memset(mem, 0, length);
	return mem;
}

//
// I_Error
//
/**	\brief phuck recursive errors
*/
static int errorcount = 0;

/**	\brief recursive error detecting
*/
static boolean shutdowning = false;

void I_Error(const char *error, ...)
{
	va_list argptr;
#if (defined (MAC_ALERT) || defined (_WIN32) || defined (_WIN64) || (defined (_WIN32_WCE) && !defined (__GNUC__))) && !defined (_XBOX)
	char buffer[8192];
#endif

	// recursive error detecting
	if (shutdowning)
	{
		errorcount++;
		// try to shutdown each subsystem separately
		if (errorcount == 2)
			I_ShutdownMusic();
		if (errorcount == 3)
			I_ShutdownSound();
		if (errorcount == 4)
			I_ShutdownCD();
		if (errorcount == 5)
			I_ShutdownGraphics();
#ifndef _arch_dreamcast
		if (errorcount == 6)
			SDL_Quit();
#endif
		if (errorcount == 7)
			I_ShutdownSystem();
		if (errorcount == 8)
		{
			M_SaveConfig(NULL);
			G_SaveGameData();
		}
		if (errorcount > 20)
		{
#ifdef MAC_ALERT
			va_start(argptr, error);
			vsprintf(buffer, error, argptr);
			va_end(argptr);
			// 2004-03-03 AJR Since the Mac user is most likely double clicking to run the game, give them a panel.
			MacShowAlert("Recursive Error", buffer, "Quit", NULL, NULL);
#elif (defined (_WIN32) || defined (_WIN64) || (defined (_WIN32_WCE)) && !defined (__GNUC__)) && !defined (_XBOX)
			va_start(argptr,error);
			vsprintf(buffer, error, argptr);
			va_end(argptr);
#ifndef _WIN32_WCE
			{
				HANDLE co = GetStdHandle(STD_OUTPUT_HANDLE);
				DWORD bytesWritten;
				if (co != (HANDLE)-1)
				{
					if (GetFileType(co) == FILE_TYPE_CHAR)
						WriteConsoleA(co, buffer, (DWORD)strlen(buffer), NULL, NULL);
					else
						WriteFile(co, buffer, (DWORD)strlen(buffer), &bytesWritten, NULL);
				}
			}
#endif
			MessageBoxA(vid.WndParent, buffer, "SRB2 Recursive Error", MB_OK|MB_ICONERROR);
#else
			// Don't print garbage
			va_start(argptr, error);
			vfprintf (stderr, error, argptr);
			va_end(argptr);
#endif
			exit(-1); // recursive errors detected
		}
	}
	shutdowning = true;
	I_ShutdownConsole();
#ifndef MAC_ALERT
	// Message first.
	va_start(argptr,error);
	fprintf(stderr, "Error: ");
	vfprintf(stderr,error,argptr);
	fprintf(stderr, "\n");
	va_end(argptr);

	fflush(stderr);
#endif
	M_SaveConfig(NULL); // save game config, cvars..
	G_SaveGameData(); // Tails 12-08-2002

	// Shutdown. Here might be other errors.
	if (demorecording)
		G_CheckDemoStatus();

	D_QuitNetGame();
	I_ShutdownMusic();
	I_ShutdownSound();
	I_ShutdownCD();
	// use this for 1.28 19990220 by Kin
	I_ShutdownGraphics();
#ifndef _arch_dreamcast
	SDL_Quit();
#endif
	I_ShutdownSystem();
#ifdef MAC_ALERT
	va_start(argptr, error);
	vsprintf(buffer, error, argptr);
	va_end(argptr);
	// 2004-03-03 AJR Since the Mac user is most likely double clicking to run the game, give them a panel.
	MacShowAlert("Critical Error", buffer, "Quit", NULL, NULL);
#endif
#if defined (PARANOIA) && defined (__CYGWIN__)
		*(int *)2 = 4; //Alam: Debug!
#endif
	exit(-1);
}

/**	\brief quit function table
*/
static quitfuncptr quit_funcs[MAX_QUIT_FUNCS]; /* initialized to all bits 0 */

//
//  Adds a function to the list that need to be called by I_SystemShutdown().
//
void I_AddExitFunc(void (*func)())
{
	int c;

	for (c = 0; c < MAX_QUIT_FUNCS; c++)
	{
		if (!quit_funcs[c])
		{
			quit_funcs[c] = func;
			break;
		}
	}
}


//
//  Removes a function from the list that need to be called by
//   I_SystemShutdown().
//
void I_RemoveExitFunc(void (*func)())
{
	int c;

	for (c = 0; c < MAX_QUIT_FUNCS; c++)
	{
		if (quit_funcs[c] == func)
		{
			while (c < MAX_QUIT_FUNCS-1)
			{
				quit_funcs[c] = quit_funcs[c+1];
				c++;
			}
			quit_funcs[MAX_QUIT_FUNCS-1] = NULL;
			break;
		}
	}
}

//
//  Closes down everything. This includes restoring the initial
//  palette and video mode, and removing whatever mouse, keyboard, and
//  timer routines have been installed.
//
//  NOTE: Shutdown user funcs are effectively called in reverse order.
//
void I_ShutdownSystem(void)
{
	int c;

	for (c = MAX_QUIT_FUNCS-1; c >= 0; c--)
		if (quit_funcs[c])
			(*quit_funcs[c])();
#if defined (SDLIO) && defined (LOGMESSAGES)
	if (logstream)
		SDL_RWclose(logstream);
#endif

}

void I_GetDiskFreeSpace(INT64 *freespace)
{
#if defined (_arch_dreamcast) || defined (_PSP_)
	*freespace = 0;
#elif defined (UNIXLIKE)
#ifdef SOLARIS
	*freespace = MAXINT;
	return;
#else // Both Linux and BSD have this, apparently.
	struct statfs stfs;
	if (statfs(".", &stfs) == -1)
	{
		*freespace = MAXINT;
		return;
	}
	*freespace = stfs.f_bavail * stfs.f_bsize;
#endif
#elif ((defined (_WIN32) && !defined (_WIN32_WCE)) || defined (_WIN64)) && !defined (_XBOX)

	static MyFunc pfnGetDiskFreeSpaceEx = NULL;
	static boolean testwin95 = false;

	INT64 usedbytes;

	if (!testwin95)
	{
		HINSTANCE h = LoadLibraryA("kernel32.dll");

		if (h)
		{
			pfnGetDiskFreeSpaceEx = (MyFunc)GetProcAddress(h, "GetDiskFreeSpaceExA");
			FreeLibrary(h);
		}
		testwin95 = true;
	}
	if (pfnGetDiskFreeSpaceEx)
	{
		if (!pfnGetDiskFreeSpaceEx(NULL, (PULARGE_INTEGER)freespace, (PULARGE_INTEGER)&usedbytes, NULL))
			*freespace = MAXINT;
	}
	else
	{
		DWORD SectorsPerCluster, BytesPerSector, NumberOfFreeClusters, TotalNumberOfClusters;
		GetDiskFreeSpace(NULL, &SectorsPerCluster, &BytesPerSector,
							&NumberOfFreeClusters, &TotalNumberOfClusters);
		*freespace = BytesPerSector*SectorsPerCluster*NumberOfFreeClusters;
	}
#else // Dummy for platform independent; 1GB should be enough
	*freespace = 1024*1024*1024;
#endif
}

char *I_GetUserName(void)
{
#if !(defined (_WIN32_WCE) || defined (_XBOX))
	static char username[MAXPLAYERNAME];
	char *p;
#if defined (_WIN32) || defined (_WIN64)
	DWORD i = MAXPLAYERNAME;

	if (!GetUserNameA(username, &i))
#endif
	{
		p = I_GetEnv("USER");
		if (!p)
		{
			p = I_GetEnv("user");
			if (!p)
			{
				p = I_GetEnv("USERNAME");
				if (!p)
				{
					p = I_GetEnv("username");
					if (!p)
					{
						return NULL;
					}
				}
			}
		}
		strncpy(username, p, MAXPLAYERNAME);
	}


	if (strcmp(username, "") != 0)
		return username;
#endif
	return NULL; // dummy for platform independent version
}

int I_mkdir(const char *dirname, int unixright)
{
//[segabor]
#if defined (UNIXLIKE) || defined (__CYGWIN__) || defined (__OS2__) || (defined (_XBOX) && defined (__GNUC__))
	return mkdir(dirname, unixright);
#elif (defined (_WIN32) || (defined (_WIN32_WCE) && !defined (__GNUC__)) || defined (_WIN64)) && !defined (_XBOX)
	unixright = 0; /// \todo should implement ntright under nt...
	return CreateDirectoryA(dirname, NULL);
#else
	dirname = NULL;
	unixright = 0;
	return false;
#endif
}

char *I_GetEnv(const char *name)
{
#ifdef NEED_SDL_GETENV
	return SDL_getenv(name);
#elif defined(_WIN32_WCE)
	name = NULL;
	return NULL;
#else
	return getenv(name);
#endif
}

int I_PutEnv(char *variable)
{
#ifdef NEED_SDL_GETENV
	return SDL_putenv(variable);
#elif defined(_WIN32_WCE)
	return ((variable)?-1:0);
#else
	return putenv(variable);
#endif
}

/**	\brief	The isWadPathOk function

	\param	path	string path to check

	\return if true, wad file found

	
*/
static boolean isWadPathOk(const char *path)
{
	char *wad3path = malloc(256);

	if (!wad3path)
		return false;

	sprintf(wad3path, pandf, path, WADKEYWORD1);

	if (FIL_ReadFileOK(wad3path))
	{
		free(wad3path);
		return true;
	}

	sprintf(wad3path, pandf, path, WADKEYWORD2);

	if (FIL_ReadFileOK(wad3path))
	{
		free(wad3path);
		return true;
	}

	free(wad3path);
	return false;
}

static void pathonly(char *s)
{
	size_t j;

	for (j = strlen(s); j != (size_t)-1; j--)
		if ((s[j] == '\\') || (s[j] == ':') || (s[j] == '/'))
		{
			if (s[j] == ':') s[j+1] = 0;
			else s[j] = 0;
			return;
		}
}

/**	\brief	search for srb2.srb in the given path

	\param	searchDir	starting path

	\return	WAD path if not NULL

	
*/
static const char *searchWad(const char *searchDir) 
{
	static char tempsw[256] = "";
	filestatus_t fstemp;

	strcpy(tempsw, WADKEYWORD1);
	fstemp = filesearch(tempsw,searchDir,NULL,true,20);
	if (fstemp == FS_FOUND)
	{
		pathonly(tempsw);
		return tempsw;
	}

	strcpy(tempsw, WADKEYWORD2);
	fstemp = filesearch(tempsw, searchDir, NULL, true, 20);
	if (fstemp == FS_FOUND)
	{
		pathonly(tempsw);
		return tempsw;
	}
	return NULL;
}

/**	\brief go through all possible paths and look for srb2.srb

  \return path to srb2.srb if any
*/
static const char *locateWad(void)
{
	const char *envstr;
	const char *WadPath;

	I_OutputMsg("SRB2WADDIR");
	// does SRB2WADDIR exist?
	if (((envstr = I_GetEnv("SRB2WADDIR")) != NULL) && isWadPathOk(envstr))
		return envstr;

#ifndef NOCWD
	I_OutputMsg(",.");
	// examine current dir
	strcpy(returnWadPath, ".");
	if (isWadPathOk(returnWadPath))
		return NULL;
#endif

	// examine default dirs
#ifdef DEFAULTWADLOCATION1
	I_OutputMsg(","DEFAULTWADLOCATION1);
	strcpy(returnWadPath, DEFAULTWADLOCATION1);
	if (isWadPathOk(returnWadPath))
		return returnWadPath;
#endif
#ifdef DEFAULTWADLOCATION2
	I_OutputMsg(","DEFAULTWADLOCATION2);
	strcpy(returnWadPath, DEFAULTWADLOCATION2);
	if (isWadPathOk(returnWadPath))
		return returnWadPath;
#endif
#ifdef DEFAULTWADLOCATION3
	I_OutputMsg(","DEFAULTWADLOCATION3);
	strcpy(returnWadPath, DEFAULTWADLOCATION3);
	if (isWadPathOk(returnWadPath))
		return returnWadPath;
#endif
#ifdef DEFAULTWADLOCATION4
	I_OutputMsg(","DEFAULTWADLOCATION4);
	strcpy(returnWadPath, DEFAULTWADLOCATION4);
	if (isWadPathOk(returnWadPath))
		return returnWadPath;
#endif
#ifdef DEFAULTWADLOCATION5
	I_OutputMsg(","DEFAULTWADLOCATION5);
	strcpy(returnWadPath, DEFAULTWADLOCATION5);
	if (isWadPathOk(returnWadPath))
		return returnWadPath;
#endif
#ifdef DEFAULTWADLOCATION6
	I_OutputMsg(","DEFAULTWADLOCATION6);
	strcpy(returnWadPath, DEFAULTWADLOCATION6);
	if (isWadPathOk(returnWadPath))
		return returnWadPath;
#endif
#ifdef DEFAULTWADLOCATION7
	I_OutputMsg(","DEFAULTWADLOCATION7);
	strcpy(returnWadPath, DEFAULTWADLOCATION7);
	if (isWadPathOk(returnWadPath))
		return returnWadPath;
#endif
#ifndef NOHOME
	// find in $HOME
	I_OutputMsg(",HOME");
	if ((envstr = I_GetEnv("HOME")) != NULL)
	{
		WadPath = searchWad(envstr);
		if (WadPath)
			return WadPath;
	}
#endif
#ifdef DEFAULTSEARCHPATH1
	// find in /usr/local
	I_OutputMsg(", in:"DEFAULTSEARCHPATH1);
	WadPath = searchWad(DEFAULTSEARCHPATH1);
	if (WadPath)
		return WadPath;
#endif
#ifdef DEFAULTSEARCHPATH2
	// find in /usr/games
	I_OutputMsg(", in:"DEFAULTSEARCHPATH2);
	WadPath = searchWad(DEFAULTSEARCHPATH2);
	if (WadPath)
		return WadPath;
#endif
#ifdef DEFAULTSEARCHPATH3
	// find in ???
	I_OutputMsg(", in:"DEFAULTSEARCHPATH3);
	WadPath = searchWad(DEFAULTSEARCHPATH3);
	if (WadPath)
		return WadPath;
#endif
	// if nothing was found
	return NULL;
}

const char *I_LocateWad(void)
{
	const char *waddir;

	I_OutputMsg("Looking for WADs in: ");
	waddir = locateWad();
	I_OutputMsg("\n");

	if (waddir)
	{
		// change to the directory where we found srb2.srb
#if ((defined (_WIN32) && !defined (_WIN32_WCE)) || defined (_WIN64)) && !defined (_XBOX)
		SetCurrentDirectoryA(waddir);
#elif !defined (_WIN32_WCE)
		chdir(waddir);
#endif
	}
	return waddir;
}

#ifdef LINUX
#define MEMINFO_FILE "/proc/meminfo"
#define MEMTOTAL "MemTotal:"
#define MEMFREE "MemFree:"
#endif

// quick fix for compil
ULONG I_GetFreeMem(ULONG *total)
{
#if defined (_arch_dreamcast)
	//Dreamcast!
	if (total)
		*total = 16<<20;
	return 8<<20;
#elif defined (FREEBSD)
	struct vmmeter sum;
	kvm_t *kd;
	struct nlist namelist[] =
	{
#define X_SUM   0
		{"_cnt"},
		{NULL}
	};
	if ((kd = kvm_open(NULL, NULL, NULL, O_RDONLY, "kvm_open")) == NULL)
	{
		*total = 0L;
		return 0;
	}
	if (kvm_nlist(kd, namelist) != 0)
	{
		kvm_close (kd);
		*total = 0L;
		return 0;
	}
	if (kvm_read(kd, namelist[X_SUM].n_value, &sum,
		sizeof (sum)) != sizeof (sum))
	{
		kvm_close(kd);
		*total = 0L;
		return 0;
	}
	kvm_close(kd);

	if (total)
		*total = sum.v_page_count * sum.v_page_size;
	return sum.v_free_count * sum.v_page_size;
#elif defined (SOLARIS)
	/* Just guess */
	*total = 32 << 20;
	return 32 << 20;
#elif defined (LINUX)
	/* Linux */
	char buf[1024];
	char *memTag;
	ULONG freeKBytes;
	ULONG totalKBytes;
	int n;
	int meminfo_fd = -1;

	meminfo_fd = open(MEMINFO_FILE, O_RDONLY);
	n = read(meminfo_fd, buf, 1023);
	close(meminfo_fd);

	if (n < 0)
	{
		// Error
		*total = 0L;
		return 0;
	}

	buf[n] = '\0';
	if (NULL == (memTag = strstr(buf, MEMTOTAL)))
	{
		// Error
		*total = 0L;
		return 0;
	}

	memTag += sizeof (MEMTOTAL);
	totalKBytes = atoi(memTag);

	if (NULL == (memTag = strstr(buf, MEMFREE)))
	{
		// Error
		*total = 0L;
		return 0;
	}

	memTag += sizeof (MEMFREE);
	freeKBytes = atoi(memTag);

	if (total)
		*total = totalKBytes << 10;
	return freeKBytes << 10;
#elif (defined (_WIN32) || (defined (_WIN32_WCE) && !defined (__GNUC__)) || defined (_WIN64)) && !defined (_XBOX)
	MEMORYSTATUS info;

	info.dwLength = sizeof (MEMORYSTATUS);
	GlobalMemoryStatus( &info );
	if (total)
		*total = (ULONG)info.dwTotalPhys;
	return (ULONG)info.dwAvailPhys;
#elif defined (__OS2__)
	ULONG pr_arena;

	if (total)
		DosQuerySysInfo( QSV_TOTPHYSMEM, QSV_TOTPHYSMEM,
							(PVOID) total, sizeof (ULONG));
	DosQuerySysInfo( QSV_MAXPRMEM, QSV_MAXPRMEM,
				(PVOID) &pr_arena, sizeof (ULONG));

	return pr_arena;
#else
	// Guess 48 MB.
	if (total)
		*total = 48<<20;
	return 48<<20;
#endif /* LINUX */
}

const CPUInfoFlags *I_CPUInfo(void)
{
	static CPUInfoFlags SDL_CPUInfo;
	memset(&SDL_CPUInfo,0,sizeof (CPUInfoFlags));
#ifdef HAVE_SDLCPUINFO
	SDL_CPUInfo.RDTSC       = SDL_HasRDTSC();
	SDL_CPUInfo.MMX         = SDL_HasMMX();
	SDL_CPUInfo.MMXExt      = SDL_HasMMXExt();
	SDL_CPUInfo.AMD3DNow    = SDL_Has3DNow();
	SDL_CPUInfo.AMD3DNowExt = SDL_Has3DNowExt();
	SDL_CPUInfo.SSE         = SDL_HasSSE();
	SDL_CPUInfo.SSE2        = SDL_HasSSE2();
	SDL_CPUInfo.AltiVec     = SDL_HasAltiVec();
	return &SDL_CPUInfo;
#else
	return NULL; /// \todo CPUID asm
#endif
}

UINT64 I_FileSize(const char *filename)
{
#ifdef _PSP_
	SceIoStat fs;
	fs.st_attr = (unsigned int)-1;
	sceIoGetstat(filename, &fs);
	return fs.st_attr;
#else
#if defined (SDLIO) && !defined (_arch_dreamcast)
	SDL_RWops *handle;
#else
	int handle;
#ifndef _arch_dreamcast
	struct stat bufstat;
#endif
#endif
	UINT64 filesize;
	
#if defined (SDLIO) && !defined (_arch_dreamcast)
	if (NULL != (handle = SDL_RWFromFile(filename, "rb")))
#else
	if ((handle = open(filename, O_RDONLY|O_BINARY, 0666)) != -1)
#endif
	{
#if defined (SDLIO) && !defined (_arch_dreamcast)
		SDL_RWseek(handle, 0, SEEK_END);
		filesize = SDL_RWtell(handle);
		SDL_RWclose(handle);
#else
#ifdef _arch_dreamcast
		filesize = fs_total(handle);
#else
		fstat(handle, &bufstat);
		filesize = bufstat.st_size;
#endif
		close(handle);
#endif
		return filesize;
	}
#endif
	return (UINT64)-1;
}
