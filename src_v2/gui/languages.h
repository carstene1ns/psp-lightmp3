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
#ifndef __languages_h
#define __languages_h (1)

#define MAX_LANGSTRINGS 500

struct strLangString{
    char name[52];
    char value[100];
};

struct strLangStrings{
    int stringsCount;
    struct strLangString strings[MAX_LANGSTRINGS];
};

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Public functions:
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
int langLoad(char *fileName);
void langLoadList(char *dirName);
char *langGetString(char *stringName);

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Globals:
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
extern int languagesCount;
char languagesList[100][100];

#endif
