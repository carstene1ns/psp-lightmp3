//    LightMP3
//    Copyright (C) 2007,2008 Sakya
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
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "../players/player.h"
#include "audioscrobbler.h"
#include "../main.h"

char SCROBBLER_fileName[264];

//Init scrobbler logging:
int SCROBBLER_init(char *fileName){
	int f = 0;

	strcpy(SCROBBLER_fileName, fileName);
    f = sceIoOpen(SCROBBLER_fileName, PSP_O_RDONLY, 0777);
	if (f < 0){
		//Create empty file:
		SCROBBLER_writeHeader();
	}else{
        sceIoClose(f);
    }
	return 0;
}


//Write header:
int SCROBBLER_writeHeader(){
	int f = 0;
	char buffer[260] = "";

	f = sceIoOpen(SCROBBLER_fileName, PSP_O_WRONLY | PSP_O_CREAT, 0777);
	if (f < 0){
		//Error opening file:
		return(-1);
	}
	sceIoWrite(f, "#AUDIOSCROBBLER/1.0\n", strlen("#AUDIOSCROBBLER/1.0\n"));
	sceIoWrite(f, "#TZ/UTC\n", strlen("#TZ/UTC\n"));
	snprintf(buffer, sizeof(buffer), "#CLIENT/PSP LightMP3 v%s\n", VERSION);
	sceIoWrite(f, buffer, strlen(buffer));
	sceIoClose(f);
	return(0);
}


//Add a track to logfile:
int SCROBBLER_addTrack(struct fileInfo tag, long duration, char *rating, long timestamp){
	int f = 0;
	char testo[1024*2] = "";

    SCROBBLER_init(SCROBBLER_fileName);
	f = sceIoOpen(SCROBBLER_fileName, PSP_O_WRONLY | PSP_O_APPEND, 0777);
	if (f < 0){
		//Error opening file:
		return(-1);
	}

	snprintf(testo, sizeof(testo), "%s\t%s\t%s\t%s\t%li\t%s\t%li\n", tag.artist, tag.album, tag.title, "", duration, rating, timestamp);
	sceIoWrite(f, testo, strlen(testo));
	sceIoClose(f);
    return(0);
}

