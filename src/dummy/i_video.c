#include "../doomdef.h"
#include "../command.h"
#include "../i_video.h"

rendermode_t rendermode = render_none;

boolean highcolor = false;

boolean allow_fullscreen = false;

consvar_t cv_vidwait = {"vid_wait", "On", CV_SAVE, CV_OnOff, NULL, 0, NULL, NULL, 0, 0, NULL};

void I_StartupGraphics(void){}

void I_ShutdownGraphics(void){}

void I_SetPalette(RGBA_t *palette)
{
	palette = NULL;
}

int VID_NumModes(void)
{
	return 0;
}

int VID_GetModeForSize(int w, int h)
{
	w = h = 0;
	return 0;
}

void VID_PrepareModeList(void){}

int VID_SetMode(int modenum)
{
	modenum = 0;
	return 0;
}

const char *VID_GetModeName(int modenum)
{
	modenum = 0;
	return NULL;
}

void I_UpdateNoBlit(void){}

void I_FinishUpdate(void){}

void I_WaitVBL(int count)
{
	count = 0;
}

void I_ReadScreen(byte *scr)
{
	scr = NULL;
}

void I_BeginRead(void){}

void I_EndRead(void){}

