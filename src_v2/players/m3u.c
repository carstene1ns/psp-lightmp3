//    LightMP3
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

//M3U parsing
#include <pspkernel.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

#include "m3u.h"
#include "../system/opendir.h"

static struct M3U_playList lPlayList;
static struct M3U_songEntry emptySong;

//Split song length and title:
int splitSongInfo(char *text, char *chrLength, char *title){
	strcpy(chrLength, "");
	strcpy(title, "");
	if (strstr(text, ",") != NULL){
		int i = 0;
		int posSep = -1;
		int countLength = 0;
		int countTitle = 0;

		for (i = 8; i < strlen(text); i++){
			if ((int)text[i] == 10 || (int)text[i] == 13){
				continue;
			}
			if (text[i] == ','){
				posSep = i;
			}else if (posSep == -1){
				if ((countLength) | (text[i] != ' ')){
					chrLength[countLength] = text[i];
					countLength++;
				}
			}else{
				if ((countTitle) | (text[i] != ' ')){
					title[countTitle] = text[i];
					countTitle++;
				}
			}
		}
		chrLength[countLength] = '\0';
		title[countTitle] = '\0';
		return(1);
	}
	return(0);
}

int M3U_getSongCountFromFile(char *fileName){
    int count = 0;
    char lineText[512];
	FILE *f = fopen(fileName, "rt");

    if (f == NULL)
        return -1;

    while(fgets(lineText, 256, f) != NULL){
        if (lineText[0] != '#')
            count++;
    }
    fclose(f);
    return count;
}

//Open and parse a M3U file:
int M3U_open(char *fileName){
	FILE *f;
	char lineText[512];
	char chrLength[20];
	char title[264];
	struct M3U_songEntry *singleEntry;
	int playListCount = lPlayList.songCount;

	f = fopen(fileName, "rt");
	if (f == NULL){
		//Error opening file:
		return(-1);
	}

	while(fgets(lineText, 256, f) != NULL){
		if (!strncmp(lineText, "#EXTINF:", 8)){
			//Length and title:
			splitSongInfo(lineText, chrLength, title);
		}else if (!strncmp(lineText, "#EXTM3U", 7)){
			//Nothing to do. :)
		}else if (strlen(lineText) > 2){
			//Store song info:
            if (!lPlayList.songs[playListCount])
                lPlayList.songs[playListCount] = malloc(sizeof(struct M3U_songEntry));
			singleEntry = lPlayList.songs[playListCount++];
            if (!singleEntry)
                break;
			strncpy(singleEntry->fileName, lineText, 263);
			singleEntry->fileName[263] = '\0';
			if ((int)singleEntry->fileName[strlen(singleEntry->fileName) - 1] == 10 || (int)singleEntry->fileName[strlen(singleEntry->fileName) - 1] == 13 ){
				singleEntry->fileName[strlen(singleEntry->fileName) - 1] = '\0';
			}
			if ((int)singleEntry->fileName[strlen(singleEntry->fileName) - 1] == 10 || (int)singleEntry->fileName[strlen(singleEntry->fileName) - 1] == 13 ){
				singleEntry->fileName[strlen(singleEntry->fileName) - 1] = '\0';
			}

			if (strlen(title)){
				strncpy(singleEntry->title, title, 263);
			}else{
				getFileName(singleEntry->fileName, singleEntry->title);
			}
			singleEntry->title[263] = '\0';
			singleEntry->length = atoi(chrLength);
			if (playListCount == MAX_SONGS){
				break;
			}
		}
	}
	fclose(f);

	lPlayList.modified = 0;
	lPlayList.songCount = playListCount;
	strcpy(lPlayList.fileName, fileName);
	return(0);
}

//Save to m3u file:
int M3U_save(char *fileName){
	FILE *f;
	int i;
	char testo[500];

	f = fopen(fileName, "w");
	if (f == NULL){
		//Error opening file:
		return(-1);
	}
	fwrite("#EXTM3U\n", 1, strlen("#EXTM3U\n"), f);

	for (i=0; i < lPlayList.songCount; i++){
		snprintf(testo, sizeof(testo), "#EXTINF:%i,%s\n", lPlayList.songs[i]->length, lPlayList.songs[i]->title);
		fwrite(testo, 1, strlen(testo), f);
		snprintf(testo, sizeof(testo), "%s\n", lPlayList.songs[i]->fileName);
		fwrite(testo, 1, strlen(testo), f);
	}
	fclose(f);
	lPlayList.modified = 0;
	return(0);
}

//Get song count:
int M3U_getSongCount(){
	return lPlayList.songCount;
}

//Get a song:
struct M3U_songEntry *M3U_getSong(int index){
	if (index >= 0 && index < lPlayList.songCount){
		return lPlayList.songs[index];
	}else{
		return &emptySong;
	}
}

//Get total length in seconds:
int M3U_getTotalLength(){
	int i;
	int total = 0;
	for (i = 0; i < lPlayList.songCount; i++)
		total += lPlayList.songs[i]->length;
	return(total);
}

//Move a song up in the playlist:
int M3U_moveSongUp(int index){
	if (index > 0){
		struct M3U_songEntry tSong;
		//Pass 1:
		strcpy(tSong.fileName, lPlayList.songs[index]->fileName);
		tSong.length = lPlayList.songs[index]->length;
		strcpy(tSong.title, lPlayList.songs[index]->title);
		//Pass 2:
		strcpy(lPlayList.songs[index]->fileName, lPlayList.songs[index-1]->fileName);
		lPlayList.songs[index]->length = lPlayList.songs[index-1]->length;
		strcpy(lPlayList.songs[index]->title, lPlayList.songs[index-1]->title);
		//Pass 3:
		strcpy(lPlayList.songs[index-1]->fileName, tSong.fileName);
		lPlayList.songs[index-1]->length = tSong.length;
		strcpy(lPlayList.songs[index-1]->title, tSong.title);
		lPlayList.modified = 1;
		return(0);
	}else{
		return(-1);
	}
}

//Move a song down in the playlist:
int M3U_moveSongDown(int index){
	if (index < lPlayList.songCount - 1){
		struct M3U_songEntry tSong;
		//Pass 1:
		strcpy(tSong.fileName, lPlayList.songs[index]->fileName);
		tSong.length = lPlayList.songs[index]->length;
		strcpy(tSong.title, lPlayList.songs[index]->title);
		//Pass 2:
		strcpy(lPlayList.songs[index]->fileName, lPlayList.songs[index+1]->fileName);
		lPlayList.songs[index]->length = lPlayList.songs[index+1]->length;
		strcpy(lPlayList.songs[index]->title, lPlayList.songs[index+1]->title);
		//Pass 3:
		strcpy(lPlayList.songs[index+1]->fileName, tSong.fileName);
		lPlayList.songs[index+1]->length = tSong.length;
		strcpy(lPlayList.songs[index+1]->title, tSong.title);
		lPlayList.modified = 1;
		return(0);
	}else{
		return(-1);
	}
}

//Remove a song from playlist:
int M3U_removeSong(int index){
	if (index >= 0 && index <= lPlayList.songCount - 1){
		int i;
		for (i = index; i <= lPlayList.songCount - 1; i++){
			if (i < lPlayList.songCount - 1){
				strcpy(lPlayList.songs[i]->fileName, lPlayList.songs[i + 1]->fileName);
				lPlayList.songs[i]->length = lPlayList.songs[i + 1]->length;
				strcpy(lPlayList.songs[i]->title, lPlayList.songs[i + 1]->title);
			}else{
				strcpy(lPlayList.songs[i]->fileName, "");
				lPlayList.songs[i]->length = 0;
				strcpy(lPlayList.songs[i]->title, "");
			}
		}
		lPlayList.songCount--;
		lPlayList.modified = 1;
		return(0);
	}else{
		return(-1);
	}
}

//Add a song to the playlist:
int M3U_addSong(char *fileName, int length, char *title){
	if (lPlayList.songCount < MAX_SONGS){
        if (!lPlayList.songs[lPlayList.songCount])
            lPlayList.songs[lPlayList.songCount] = malloc(sizeof(struct M3U_songEntry));
		struct M3U_songEntry *entry = lPlayList.songs[lPlayList.songCount];
        if (!entry)
            return -1;
		strcpy(entry->fileName, fileName);
		entry->length = length;
		strcpy(entry->title, title);
		lPlayList.songCount++;
		lPlayList.modified = 1;
		return(0);
	}else{
		return(-1);
	}
}

//Is modified?
int M3U_isModified(){
	return(lPlayList.modified);
}

//Force modified:
int M3U_forceModified(int modified){
	lPlayList.modified = modified;
	return(0);
}

//Clear playlist:
int M3U_clear(){
	int i;
	for (i=0; i < lPlayList.songCount; i++){
        if (lPlayList.songs[i])
            free(lPlayList.songs[i]);
            lPlayList.songs[i] = NULL;
	}
	lPlayList.songCount = 0;
	strcpy(lPlayList.fileName, "");
	lPlayList.modified = 0;
	return(0);
}

//Returns the whole playlist:
struct M3U_playList *M3U_getPlaylist(){
    return &lPlayList;
}

//Check for files existance:
int M3U_checkFiles(){
    int i = 0;
	for (i=0; i < lPlayList.songCount; i++){
        if (fileExists(lPlayList.songs[i]->fileName) < 0)
            M3U_removeSong(i--);
    }
    return 0;
}
