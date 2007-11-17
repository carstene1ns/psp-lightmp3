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

#include "player.h"
#include "mp3player.h"
#include "oggplayer.h"

//shared global vars
int MAX_VOLUME_BOOST=15;
int MIN_VOLUME_BOOST=-15;
int MIN_PLAYING_SPEED=0;
int MAX_PLAYING_SPEED=9;

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

//Set pointer to audio functions based on filename:
void setAudioFunctions(char *filename){
	char ext[5];
	memcpy(ext, filename + strlen(filename) - 4, 5);
	if (!stricmp(ext, ".ogg")){
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
    } else if (!stricmp(ext, ".mp3")){
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
    }
}

//Unset pointer to audio functions:
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
