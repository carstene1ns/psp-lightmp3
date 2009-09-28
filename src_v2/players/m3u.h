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

//M3U header file
#define MAX_SONGS 5000

struct M3U_songEntry{
	char fileName[264];
	int length;
	char title[264];
};

struct M3U_playList{
	char fileName[264];
	int songCount;
	struct M3U_songEntry *songs[MAX_SONGS];
	int modified;
};

int M3U_open(char *fileName);
int M3U_save(char *fileName);
int M3U_getSongCount();
int M3U_getTotalLength();
int M3U_moveSongUp();
int M3U_moveSongDown();
int M3U_removeSong(int index);
int M3U_addSong(char *fileName, int length, char *title);
struct M3U_songEntry *M3U_getSong(int index);
int M3U_isModified();
int M3U_forceModified(int modified);
int M3U_clear();
struct M3U_playList *M3U_getPlaylist();
int M3U_checkFiles();
