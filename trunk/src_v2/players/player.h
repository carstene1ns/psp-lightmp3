//    player.h
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
#ifndef __player_h
#define __player_h (1)

#include <pspkernel.h>
#include <pspsdk.h>
#include <pspaudiocodec.h>
#include <pspaudio.h>
#include "pspaudiolib.h"

#include "info.h"
#include "mp3player.h"
#include "mp3playerME.h"
#include "aa3playerME.h"
#include "oggplayer.h"
#include "flacplayer.h"
#include "aacplayer.h"
#include "wmaplayerme.h"

#define OPENING_OK 0
#define ERROR_OPENING -1
#define ERROR_INVALID_SAMPLE_RATE -2
#define ERROR_MEMORY -3
#define ERROR_CREATE_THREAD -4

#define MP3_TYPE 0
#define OGG_TYPE 1
#define AT3_TYPE 2
#define FLAC_TYPE 3
#define AAC_TYPE 4
#define WMA_TYPE 5
#define UNK_TYPE -1

#define FASTFORWARD_VOLUME 0x2200
#define MAX_IMAGE_DIMENSION 300*1024
#define DEFAULT_THREAD_STACK_SIZE 256*1024


extern int CLOCK_WHEN_PAUSE;

extern char fileTypeDescription[6][20];

extern char audioCurrentDir[264];
extern int MUTED_VOLUME;
extern int currentVolume;
extern int MAX_VOLUME_BOOST;
extern int MIN_VOLUME_BOOST;
extern int MIN_PLAYING_SPEED;
extern int MAX_PLAYING_SPEED;

//shared global vars for ME
extern SceUID fd;
extern u16 data_align;
extern u32 sample_per_frame;
extern u16 channel_mode;
extern u32 samplerate;
extern long data_start;
extern long data_size;
extern u8 getEDRAM;
extern u32 channels;
extern SceUID data_memid;
extern volatile int OutputBuffer_flip;

//shared between at3+aa3
#define AT3_OUTPUT_BUFFER_SIZE	(2048*2)
extern u16 at3_type;
extern u8 at3_at3plus_flagdata[2];
short AT3_OutputBuffer[2][AT3_OUTPUT_BUFFER_SIZE];
short *AT3_OutputPtr;

int openAudio(int channel, int samplecount);
SceUID LoadStartAudioModule(char *modname, int partition);
int initMEAudioModules();
int GetID3TagSize(char *fname);
int SeekNextFrameMP3(SceUID fd);



//Pointers for functions:
extern void (*initFunct)(int);
extern int (*loadFunct)(char *);
extern int (*playFunct)();
extern void (*pauseFunct)();
extern void (*endFunct)();
extern void (*setVolumeBoostTypeFunct)(char*);
extern void (*setVolumeBoostFunct)(int);
extern struct fileInfo *(*getInfoFunct)();
extern struct fileInfo (*getTagInfoFunct)();
extern void (*getTimeStringFunct)();
extern float (*getPercentageFunct)();
extern int (*getPlayingSpeedFunct)();
extern int (*setPlayingSpeedFunct)(int);
extern int (*endOfStreamFunct)();

extern int (*setMuteFunct)(int);
extern int (*setFilterFunct)(double[32], int copyFilter);
extern void (*enableFilterFunct)();
extern void (*disableFilterFunct)();
extern int (*isFilterEnabledFunct)();
extern int (*isFilterSupportedFunct)();
extern int (*suspendFunct)();
extern int (*resumeFunct)();
extern void (*fadeOutFunct)(float seconds);

extern double (*getFilePositionFunct)();                     //Gets current file position in bytes
extern void (*setFilePositionFunct)(double position);          //Set current file position in butes

extern int setAudioFunctions(char *filename, int useME);
extern void unsetAudioFunctions();

short volume_boost(short *Sample, unsigned int *boost);
int setVolume(int channel, int volume);
int setMute(int channel, int onOff);
void fadeOut(int channel, float seconds);

int initAudioLib();
int endAudioLib();
int setAudioFrequency(unsigned short samples, unsigned short freq, char car);
int releaseAudio(void);
int audioOutput(int volume, void *buffer);
void initFileInfo(struct fileInfo *info);
void getCovertArtImageName(char *fileName, struct fileInfo *info);

#endif
