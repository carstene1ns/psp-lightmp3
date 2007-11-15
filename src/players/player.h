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
#define OPENING_OK 0
#define ERROR_OPENING -1
#define ERROR_INVALID_SAMPLE_RATE -2
#define ERROR_MEMORY -3

#define MP3_TYPE 0
#define OGG_TYPE 1

#define MUTED_VOLUME 0x800
#define FASTFORWARD_VOLUME 0x2200

extern int MAX_VOLUME_BOOST;
extern int MIN_VOLUME_BOOST;
extern int MIN_PLAYING_SPEED;
extern int MAX_PLAYING_SPEED;

struct fileInfo{
    int fileType;
	int fileSize;
	char layer[10];
	int kbit;
	long instantBitrate;
	long hz;
	char mode[50];
	char emphasis[10];
	long length;
	char strLength[10];
	long frames;
	long framesDecoded;

    //Tag/comments:
	char album[256];
	char title[256];
	char artist[256];
	char genre[256];
    char year[5];
    char trackNumber[5];
};


//Pointers for functions:
extern void (*initFunct)(int);
extern int (*loadFunct)(char *);
extern int (*playFunct)();
extern void (*pauseFunct)();
extern void (*endFunct)();
extern void (*setVolumeBoostTypeFunct)(char*);
extern void (*setVolumeBoostFunct)(int);
extern struct fileInfo (*getInfoFunct)();
extern struct fileInfo (*getTagInfoFunct)();
extern void (*getTimeStringFunct)();
extern int (*getPercentageFunct)();
extern int (*getPlayingSpeedFunct)();
extern int (*setPlayingSpeedFunct)(int);
extern int (*endOfStreamFunct)();

extern int (*setMuteFunct)(int);
extern int (*setFilterFunct)(double[32], int copyFilter);
extern void (*enableFilterFunct)();
extern void (*disableFilterFunct)();
extern int (*isFilterEnabledFunct)();
extern int (*isFilterSupported)();
extern void setAudioFunctions(char *filename);

short volume_boost(short *Sample, unsigned int *boost);
