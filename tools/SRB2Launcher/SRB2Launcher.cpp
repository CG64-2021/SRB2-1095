/////////////////////////////
//                         //
//    Sonic Robo Blast 2   //
// Official Win32 Launcher //
//                         //
//           By            //
//        SSNTails         //
//    ah518@tcnet.org      //
//  (Sonic Team Junior)    //
//  http://www.srb2.org    //
//                         //
/////////////////////////////
//
// This source code is released under
// Public Domain. I hope it helps you
// learn how to write exciting Win32
// applications in C!
//
// However, you may not alter this
// program and continue to call it
// the "Official Sonic Robo Blast 2
// Launcher".
//
// NOTE: Not all files in this project
// are released under this license.
// Any license mentioned in accompanying
// source files overrides the license
// mentioned here, sorry!
//
// SRB2Launcher.cpp : Defines the entry point for the application.
//

#include "stdafx.h"
#include <stdlib.h>
#include <stdio.h>
#include "SRB2Launcher.h"

char TempString[256];

char EXEName[1024] = "srb2win.exe";
char Arguments[16384];

HWND mainHWND;
HINSTANCE g_hInst;

#define APPTITLE "Official Sonic Robo Blast 2 Launcher"
#define APPVERSION "v0.1"
#define APPAUTHOR "SSNTails"
#define APPCOMPANY "Sonic Team Junior"

LRESULT CALLBACK MainProc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam);

//
// RunSRB2
//
// Runs SRB2
// returns true if successful
//
BOOL RunSRB2(void)
{
	SHELLEXECUTEINFO lpExecInfo;
	BOOL result;

	memset(&lpExecInfo, 0, sizeof(SHELLEXECUTEINFO));

	lpExecInfo.cbSize = sizeof(SHELLEXECUTEINFO);

	lpExecInfo.lpFile = EXEName;
	lpExecInfo.lpParameters = Arguments;
	lpExecInfo.nShow = SW_SHOWNORMAL;
	lpExecInfo.hwnd = mainHWND;
	lpExecInfo.fMask = SEE_MASK_NOCLOSEPROCESS;
	lpExecInfo.lpVerb = "open";

	result = ShellExecuteEx(&lpExecInfo);

	if(!result)
	{
		MessageBox(mainHWND, "Error starting the game!", "Error", MB_OK|MB_APPLMODAL|MB_ICONERROR);
		return false;
	}

	return true;
}

//
// CompileArguments
//
// Go through ALL of the settings
// and put them into a parameter
// string. Yikes!
//
void CompileArguments(void)
{
}

void RegisterDialogClass(char* name, WNDPROC callback)
{
	WNDCLASS wnd;

	wnd.style			= CS_HREDRAW | CS_VREDRAW;
	wnd.cbWndExtra		= DLGWINDOWEXTRA;
	wnd.cbClsExtra		= 0;
	wnd.hCursor			= LoadCursor(NULL,MAKEINTRESOURCE(IDC_ARROW));
	wnd.hIcon			= LoadIcon(NULL,MAKEINTRESOURCE(IDI_ICON1));
	wnd.hInstance		= g_hInst;
	wnd.lpfnWndProc		= callback;
	wnd.lpszClassName	= name;
	wnd.lpszMenuName	= NULL;
	wnd.hbrBackground	= (HBRUSH)(COLOR_WINDOW);

	if(!RegisterClass(&wnd))
	{
		return;
	}
}

int APIENTRY WinMain(HINSTANCE hInstance,
                     HINSTANCE hPrevInstance,
                     LPSTR     lpCmdLine,
                     int       nCmdShow)
{
	// Prevent multiples instances of this app.
	CreateMutex(NULL, true, APPTITLE);

	if(GetLastError() == ERROR_ALREADY_EXISTS)
		return 0;

	g_hInst = hInstance;

	RegisterDialogClass("SRB2Launcher", MainProc);

	DialogBox(g_hInst, (LPCTSTR)IDD_MAIN, NULL, (DLGPROC)MainProc);

	return 0;
}

LRESULT CALLBACK MainProc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam)
{
	switch(message)
	{
		case WM_INITDIALOG:
			mainHWND = hwnd;
			SendMessage(GetDlgItem(hwnd, IDC_EXENAME), WM_SETTEXT, 0, (LPARAM)(LPCTSTR)EXEName);
			break;
	
	    case WM_CREATE:
	    {
	            
	        break;
	    }

		case WM_DESTROY:
		{
			PostQuitMessage(0);
			break;
		}
			
		case WM_COMMAND:
		{
			switch(LOWORD(wParam))
			{
				case 2:
					PostMessage(hwnd, WM_DESTROY, 0, 0);
					break;
				case IDC_ABOUT: // The About button.
					sprintf(TempString, "%s %s by %s - %s", APPTITLE, APPVERSION, APPAUTHOR, APPCOMPANY);
					MessageBox(mainHWND, TempString, "About", MB_OK|MB_APPLMODAL);
					break;
				case IDC_GO:
					CompileArguments();
					RunSRB2();
					break;
			    default:
					break;
			}

			break;
		}

		case WM_PAINT:
		{
			break;
		}
	}

	return 0;
}

