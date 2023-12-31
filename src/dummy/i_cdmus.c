#include "../command.h"
#include "../s_sound.h"
#include "../i_sound.h"

//
// CD MUSIC I/O
//

byte cdaudio_started = 0;

consvar_t cd_volume = {"cd_volume","31",CV_SAVE,soundvolume_cons_t, NULL, 0, NULL, NULL, 0, 0, NULL};
consvar_t cdUpdate  = {"cd_update","1",CV_SAVE, NULL, NULL, 0, NULL, NULL, 0, 0, NULL};


void I_InitCD(void){}

void I_StopCD(void){}

void I_PauseCD(void){}

void I_ResumeCD(void){}

void I_ShutdownCD(void){}

void I_UpdateCD(void){}

void I_PlayCD(int track, boolean looping)
{
	track = looping = 0;
}

int I_SetVolumeCD(int volume)
{
	return volume;
}

