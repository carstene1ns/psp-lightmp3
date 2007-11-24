//    player.c
//    Copyright (C) 2007 Sakya
//    sakya_tg@yahoo.it
//
//    This program is free software; you can redistribute it and/or modify
//    it under the terms of the GNU General Public License as published by
//    the Free Software Foundation; either version 2 of the License, or
//    (at your option) any later version.
//
//    This program is distributed in the hope that it will be useful,
//    but WITHOUT ANY WARRANTY; without even the implied warranty of
//    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
//    GNU General Public License for more details.
//
//    You should have received a copy of the GNU General Public License
//    along with this program; if not, write to the Free Software
//    Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
//
//    CREDITS:
//    This file contains functions to play aa3 files through the PSP's Media Engine.
//    This code is based upon this sample code from ps2dev.org
//    http://forums.ps2dev.org/viewtopic.php?t=8469
//    and the source code of Music prx by joek2100
#include <pspkernel.h>
#include <pspsdk.h>
#include <string.h>
#include <psputility_avmodules.h>
#include <pspaudio.h>

#include "player.h"
#include "mp3player.h"
#include "mp3playerME.h"
#include "aa3playerME.h"
#include "oggplayer.h"

//shared global vars
int MAX_VOLUME_BOOST=15;
int MIN_VOLUME_BOOST=-15;
int MIN_PLAYING_SPEED=0;
int MAX_PLAYING_SPEED=9;
int currentVolume = 0;

//shared global vars for ME
int HW_ModulesInit = 0;
SceUID fd;
u16 data_align;
u32 sample_per_frame;
u16 channel_mode;
u32 samplerate;
long data_start;
long data_size;
u8 getEDRAM;
u32 channels;
SceUID data_memid;
volatile int OutputBuffer_flip;
//shared between at3+aa3
u16 at3_type;
u8* at3_data_buffer;
u8 at3_at3plus_flagdata[2];
unsigned char   AT3_OutputBuffer[2][AT3_OUTPUT_BUFFER_SIZE]__attribute__((aligned(64))),
                *AT3_OutputPtr=AT3_OutputBuffer[0];

//Pointers for functions:
void (*initFunct)(int);
int (*loadFunct)(char *);
int (*playFunct)();
void (*pauseFunct)();
void (*endFunct)();
void (*setVolumeBoostTypeFunct)(char*);
void (*setVolumeBoostFunct)(int);
struct fileInfo (*getInfoFunct)();
struct fileInfo (*getTagInfoFunct)();
void (*getTimeStringFunct)();
int (*getPercentageFunct)();
int (*getPlayingSpeedFunct)();
int (*setPlayingSpeedFunct)(int);
int (*endOfStreamFunct)();

int (*setMuteFunct)(int);
int (*setFilterFunct)(double[32], int copyFilter);
void (*enableFilterFunct)();
void (*disableFilterFunct)();
int (*isFilterEnabledFunct)();
int (*isFilterSupportedFunct)();
int (*suspendFunct)();
int (*resumeFunct)();
void (*fadeOutFunct)(float seconds);

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Functions for ME
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//Load a module:
SceUID LoadStartAudioModule(char *modname, int partition){
    SceKernelLMOption option;
    SceUID modid;

    memset(&option, 0, sizeof(option));
    option.size = sizeof(option);
    option.mpidtext = partition;
    option.mpiddata = partition;
    option.position = 0;
    option.access = 1;

    modid = sceKernelLoadModule(modname, 0, &option);
    if (modid < 0)
        return modid;

    return sceKernelStartModule(modid, 0, NULL, NULL, NULL);
}

//Load and start needed modules:
int initMEAudioModules(){
   if (!HW_ModulesInit){
        if (sceKernelDevkitVersion() == 0x01050001)
        {
            LoadStartAudioModule("flash0:/kd/me_for_vsh.prx", PSP_MEMORY_PARTITION_KERNEL);
            LoadStartAudioModule("flash0:/kd/audiocodec.prx", PSP_MEMORY_PARTITION_KERNEL);
        }
        else
        {
            sceUtilityLoadAvModule(PSP_AV_MODULE_AVCODEC);
        }
       HW_ModulesInit = 1;
   }
   return 0;
}

int GetID3TagSize(char *fname)
{
    SceUID fd;
    char header[10];
    int size = 0;
    fd = sceIoOpen(fname, PSP_O_RDONLY, 0777);
    if (fd < 0)
        return 0;

    sceIoRead(fd, header, sizeof(header));
    sceIoClose(fd);

    if (!strncmp((char*)header, "ea3", 3) || !strncmp((char*)header, "EA3", 3)
      ||!strncmp((char*)header, "ID3", 3))
    {
        //get the real size from the syncsafe int
        size = header[6];
        size = (size<<7) | header[7];
        size = (size<<7) | header[8];
        size = (size<<7) | header[9];

        size += 10;

        if (header[5] & 0x10) //has footer
            size += 10;
         return size;
    }
    return 0;
}

char GetOMGFileType(char *fname)
{
    SceUID fd;
    int size;
    char ea3_header[0x60];

    size = GetID3TagSize(fname);

    fd = sceIoOpen(fname, PSP_O_RDONLY, 0777);
    if (fd < 0)
        return UNK_TYPE;

    sceIoLseek32(fd, size, PSP_SEEK_SET);

    if (sceIoRead(fd, ea3_header, 0x60) != 0x60)
        return UNK_TYPE;

    sceIoClose(fd);

    if (strncmp(ea3_header, "EA3", 3) != 0)
        return UNK_TYPE;

    switch (ea3_header[3])
    {
        case 1:
        case 3:
            return AT3_TYPE;
        case 2:
            return MP3_TYPE;
        default:
            return UNK_TYPE;
    }
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//Set pointer to audio functions based on filename:
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void setAudioFunctions(char *filename, int useME){
	char ext[5];
	memcpy(ext, filename + strlen(filename) - 4, 5);
	if (!stricmp(ext, ".ogg")){
        //OGG Vorbis
		initFunct = OGG_Init;
		loadFunct = OGG_Load;
		playFunct = OGG_Play;
		pauseFunct = OGG_Pause;
		endFunct = OGG_End;
        setVolumeBoostTypeFunct = OGG_setVolumeBoostType;
        setVolumeBoostFunct = OGG_setVolumeBoost;
        getInfoFunct = OGG_GetInfo;
        getTagInfoFunct = OGG_GetTagInfoOnly;
        getTimeStringFunct = OGG_GetTimeString;
        getPercentageFunct = OGG_GetPercentage;
        getPlayingSpeedFunct = OGG_getPlayingSpeed;
        setPlayingSpeedFunct = OGG_setPlayingSpeed;
        endOfStreamFunct = OGG_EndOfStream;

        setMuteFunct = OGG_setMute;
        setFilterFunct = OGG_setFilter;
        enableFilterFunct = OGG_enableFilter;
        disableFilterFunct = OGG_disableFilter;
        isFilterEnabledFunct = OGG_isFilterEnabled;
        isFilterSupportedFunct = OGG_isFilterSupported;

        suspendFunct = OGG_suspend;
        resumeFunct = OGG_resume;
        fadeOutFunct = OGG_fadeOut;
    } else if (!stricmp(ext, ".mp3") && useME){
        //MP3 via Media Engine
		initFunct = MP3ME_Init;
		loadFunct = MP3ME_Load;
		playFunct = MP3ME_Play;
		pauseFunct = MP3ME_Pause;
		endFunct = MP3ME_End;
        setVolumeBoostTypeFunct = MP3ME_setVolumeBoostType;
        setVolumeBoostFunct = MP3ME_setVolumeBoost;
        getInfoFunct = MP3ME_GetInfo;
        getTagInfoFunct = MP3ME_GetTagInfoOnly;
        getTimeStringFunct = MP3ME_GetTimeString;
        getPercentageFunct = MP3ME_GetPercentage;
        getPlayingSpeedFunct = MP3ME_getPlayingSpeed;
        setPlayingSpeedFunct = MP3ME_setPlayingSpeed;
        endOfStreamFunct = MP3ME_EndOfStream;

        setMuteFunct = MP3ME_setMute;
        setFilterFunct = MP3ME_setFilter;
        enableFilterFunct = MP3ME_enableFilter;
        disableFilterFunct = MP3ME_disableFilter;
        isFilterEnabledFunct = MP3ME_isFilterEnabled;
        isFilterSupportedFunct = MP3ME_isFilterSupported;

        suspendFunct = MP3ME_suspend;
        resumeFunct = MP3ME_resume;
        fadeOutFunct = MP3ME_fadeOut;
    } else if (!stricmp(ext, ".mp3")){
        //MP3 via LibMad
		initFunct = MP3_Init;
		loadFunct = MP3_Load;		 
		playFunct = MP3_Play;
		pauseFunct = MP3_Pause;
		endFunct = MP3_End;
        setVolumeBoostTypeFunct = MP3_setVolumeBoostType;
        setVolumeBoostFunct = MP3_setVolumeBoost;
        getInfoFunct = MP3_GetInfo;
        getTagInfoFunct = MP3_GetTagInfoOnly;
        getTimeStringFunct = MP3_GetTimeString;
        getPercentageFunct = MP3_GetPercentage;
        getPlayingSpeedFunct = MP3_getPlayingSpeed;
        setPlayingSpeedFunct = MP3_setPlayingSpeed;
        endOfStreamFunct = MP3_EndOfStream;

        setMuteFunct = MP3_setMute;
        setFilterFunct = MP3_setFilter;
        enableFilterFunct = MP3_enableFilter;
        disableFilterFunct = MP3_disableFilter;
        isFilterEnabledFunct = MP3_isFilterEnabled;
        isFilterSupportedFunct = MP3_isFilterSupported;

        suspendFunct = MP3_suspend;
        resumeFunct = MP3_resume;        
        fadeOutFunct = MP3_fadeOut;
    } else if (!stricmp(ext, ".aa3") || !stricmp(ext, ".oma") || !stricmp(ext, ".omg")){
        //AA3
		initFunct = AA3ME_Init;
		loadFunct = AA3ME_Load;
		playFunct = AA3ME_Play;
		pauseFunct = AA3ME_Pause;
		endFunct = AA3ME_End;
        setVolumeBoostTypeFunct = AA3ME_setVolumeBoostType;
        setVolumeBoostFunct = AA3ME_setVolumeBoost;
        getInfoFunct = AA3ME_GetInfo;
        getTagInfoFunct = AA3ME_GetTagInfoOnly;
        getTimeStringFunct = AA3ME_GetTimeString;
        getPercentageFunct = AA3ME_GetPercentage;
        getPlayingSpeedFunct = AA3ME_getPlayingSpeed;
        setPlayingSpeedFunct = AA3ME_setPlayingSpeed;
        endOfStreamFunct = AA3ME_EndOfStream;

        setMuteFunct = AA3ME_setMute;
        setFilterFunct = AA3ME_setFilter;
        enableFilterFunct = AA3ME_enableFilter;
        disableFilterFunct = AA3ME_disableFilter;
        isFilterEnabledFunct = AA3ME_isFilterEnabled;
        isFilterSupportedFunct = AA3ME_isFilterSupported;

        suspendFunct = AA3ME_suspend;
        resumeFunct = AA3ME_resume;
        fadeOutFunct = AA3ME_fadeOut;
    }
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//Unset pointer to audio functions:
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void unsetAudioFunctions(){
    initFunct = NULL;
    loadFunct = NULL;
    playFunct = NULL;
    pauseFunct = NULL;
    endFunct = NULL;
    setVolumeBoostTypeFunct = NULL;
    setVolumeBoostFunct = NULL;
    getInfoFunct = NULL;
    getTagInfoFunct = NULL;
    getTimeStringFunct = NULL;
    getPercentageFunct = NULL;
    getPlayingSpeedFunct = NULL;
    setPlayingSpeedFunct = NULL;
    endOfStreamFunct = NULL;
    
    setMuteFunct = NULL;
    setFilterFunct = NULL;
    enableFilterFunct = NULL;
    disableFilterFunct = NULL;
    isFilterEnabledFunct = NULL;
    isFilterSupportedFunct = NULL;
    
    suspendFunct = NULL;
    resumeFunct = NULL;
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//Volume boost for a single sample:
//OLD METHOD
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
short volume_boost(short *Sample, unsigned int *boost){
	int intSample = *Sample * (*boost + 1);
	if (intSample > 32767)
		return 32767;
	else if (intSample < -32768)
		return -32768;
	else
    	return intSample;
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//Set volume:
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
int setVolume(int channel, int volume){
    pspAudioSetVolume(channel, volume, volume);
    currentVolume = volume;
    return 0;
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//Set mute:
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
int setMute(int channel, int onOff){
	if (onOff)
        setVolume(channel, MUTED_VOLUME);
	else
        setVolume(channel, 0x8000);
	return 0;
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//Fade out:
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void fadeOut(int channel, float seconds){
    int i = 0;
    long timeToWait = (seconds * 1000) / (float)currentVolume;
    for (i=currentVolume; i>=0; i--){
        pspAudioSetVolume(channel, i, i);
        sceKernelDelayThread(timeToWait);
    }
}
