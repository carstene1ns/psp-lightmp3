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
#ifndef __skinsettings_h
#define __skinsettings_h (1)

#define MAX_SKINPARAMS  500
#define MAX_SKINCOLORS 500
#define MAX_SKINPOSITIONS 500

struct intSkinParam{
    char name[52];
    int  value;
};

struct intSkinParams{
    int paramCount;
    struct intSkinParam params[MAX_SKINPARAMS];
};


struct intSkinColor{
    char name[52];
    int  value[4];
};


struct intSkinPosition{
    char name[52];
    int  value[2];
};


struct intSkinColors{
    int colorCount;
    struct intSkinColor colors[MAX_SKINCOLORS];
};

struct intSkinPositions{
    int positionCount;
    struct intSkinPosition positions[MAX_SKINPOSITIONS];
};

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Public functions:
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
int skinLoad(char *fileName);
void skinLoadList(char *dirName);
int skinGetParam(char *paramName);
int skinGetColor(char *colorName, int *color);
int skinGetPosition(char *positionName, int *position);

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Globals:
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
extern int skinsCount;
char skinsList[100][100];
struct intSkinParams skinParams;
struct intSkinColors skinColors;
struct intSkinPositions skinPositions;

#endif
