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
#ifndef __main_h
#define __main_h (1)

#include "others/medialibrary.h"

#define VERSION "2.0.0RC4"

#define ML_DB "mediaLibrary.db"

#define DISPLAY_FADE_TIME 0.2
#define FFWDREW_CPU_BOOST 100

#define MODE_FILEBROWSER 0
#define MODE_PLAYLISTS 1
#define MODE_PLAYLIST_EDITOR 2
#define MODE_MEDIA_LIBRARY 3
#define MODE_SETTINGS 4
#define MODE_PLAYER 99

//Shared globals:
extern struct settings *userSettings;
extern int fileExtCount;
extern char fileExt[10][5];
extern char tempM3Ufile[264];
extern char MLtempM3Ufile[264];
extern struct libraryEntry MLresult[ML_BUFFERSIZE];
extern struct menuElements commonMenu;
extern struct menuElements commonSubMenu;
extern int tempPos[2];
extern int tempColor[4];
extern int tempColorShadow[4];

extern float defaultTextSize;
#endif
