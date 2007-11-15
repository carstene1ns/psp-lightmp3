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
#include <stdio.h>
#include <string.h>
#include <pspkernel.h>
#include "id3.h"

struct genre
{
	int code;
	char *text;
};

struct genre genreList[] = 
{
   {0 , "Blues"}, {1 , "Classic Rock"}, {2 , "Country"}, {3 , "Dance"}, {4 , "Disco"}, {5 , "Funk"}, {6 , "Grunge"}, {7 , "Hip-Hop"}, {8 , "Jazz"}, {9 , "Metal"}, {10 , "New Age"},
   {11 , "Oldies"}, {12 , "Other"}, {13 , "Pop"}, {14 , "R&B"}, {15 , "Rap"}, {16 , "Reggae"}, {17 , "Rock"}, {18 , "Techno"}, {19 , "Industrial"}, {20 , "Alternative"},
   {21 , "Ska"}, {22 , "Death Metal"}, {23 , "Pranks"}, {24 , "Soundtrack"}, {25 , "Euro-Techno"}, {26 , "Ambient"}, {27 , "Trip-Hop"}, {28 , "Vocal"}, {29 , "Jazz+Funk"}, {30 , "Fusion"},
   {31 , "Trance"}, {32 , "Classical"}, {33 , "Instrumental"}, {34 , "Acid"}, {35 , "House"}, {36 , "Game"}, {37 , "Sound Clip"}, {38 , "Gospel"}, {39 , "Noise"}, {40 , "Alternative Rock"},
   {41 , "Bass"}, {42 , "Soul"}, {43 , "Punk"}, {44 , "Space"}, {45 , "Meditative"}, {46 , "Instrumental Pop"}, {47 , "Instrumental Rock"}, {48 , "Ethnic"}, {49 , "Gothic"}, {50 , "Darkwave"},
   {51 , "Techno-Industrial"}, {52 , "Electronic"}, {53 , "Pop-Folk"}, {54 , "Eurodance"}, {55 , "Dream"}, {56 , "Southern Rock"}, {57 , "Comedy"}, {58 , "Cult"}, {59 , "Gangsta"}, {60 , "Top 40"},
   {61 , "Christian Rap"}, {62 , "Pop/Funk"}, {63 , "Jungle"}, {64 , "Native US"}, {65 , "Cabaret"}, {66 , "New Wave"}, {67 , "Psychadelic"}, {68 , "Rave"}, {69 , "Showtunes"}, {70 , "Trailer"},
   {71 , "Lo-Fi"}, {72 , "Tribal"}, {73 , "Acid Punk"}, {74 , "Acid Jazz"}, {75 , "Polka"}, {76 , "Retro"}, {77 , "Musical"}, {78 , "Rock & Roll"}, {79 , "Hard Rock"}, {80 , "Folk"},
   {81 , "Folk-Rock"}, {82 , "National Folk"}, {83 , "Swing"}, {84 , "Fast Fusion"}, {85 , "Bebob"}, {86 , "Latin"}, {87 , "Revival"}, {88 , "Celtic"}, {89 , "Bluegrass"}, {90 , "Avantgarde"},
   {91 , "Gothic Rock"}, {92 , "Progressive Rock"}, {93 , "Psychedelic Rock"}, {94 , "Symphonic Rock"}, {95 , "Slow Rock"}, {96 , "Big Band"}, {97 , "Chorus"}, {98 , "Easy Listening"}, {99 , "Acoustic"},
   {100 , "Humour"}, {101 , "Speech"}, {102 , "Chanson"}, {103 , "Opera"}, {104 , "Chamber Music"}, {105 , "Sonata"}, {106 , "Symphony"}, {107 , "Booty Bass"}, {108 , "Primus"}, {109 , "Porn Groove"},
   {110 , "Satire"}, {111 , "Slow Jam"}, {112 , "Club"}, {113 , "Tango"}, {114 , "Samba"}, {115 , "Folklore"}, {116 , "Ballad"}, {117 , "Power Ballad"}, {118 , "Rhytmic Soul"}, {119 , "Freestyle"}, {120 , "Duet"},
   {121 , "Punk Rock"}, {122 , "Drum Solo"}, {123 , "Acapella"}, {124 , "Euro-House"}, {125 , "Dance Hall"}, {126 , "Goa"}, {127 , "Drum & Bass"}, {128 , "Club-House"}, {129 , "Hardcore"}, {130 , "Terror"},
   {131 , "Indie"}, {132 , "BritPop"}, {133 , "Negerpunk"}, {134 , "Polsk Punk"}, {135 , "Beat"}, {136 , "Christian Gangsta"}, {137 , "Heavy Metal"}, {138 , "Black Metal"}, {139 , "Crossover"}, {140 , "Contemporary C"},
   {141 , "Christian Rock"}, {142 , "Merengue"}, {143 , "Salsa"}, {144 , "Thrash Metal"}, {145 , "Anime"}, {146 , "JPop"}, {147 , "SynthPop"}
};
int genreNumber = sizeof (genreList) / sizeof (struct genre);

struct ID3Tag ParseID3(char *mp3path)
{
    char id3buffer[512];
    struct ID3Tag TmpID3;

    strcpy(TmpID3.ID3Title,"");
    strcpy(TmpID3.ID3Artist,"");
    strcpy(TmpID3.ID3Album,"");
    strcpy(TmpID3.ID3Year,"");
    strcpy(TmpID3.ID3Comment,"");
    strcpy(TmpID3.ID3GenreCode,"");
    strcpy(TmpID3.ID3GenreText,"");

    int id3fd; //our local file descriptor
    id3fd = sceIoOpen(mp3path, 0x0001, 0777);

    if (id3fd < 0)
    {
        return TmpID3;
    }

    //search for ID3v1 tags first, easier to find and parse
    sceIoLseek(id3fd, -128, SEEK_END);
    sceIoRead(id3fd,id3buffer,128);

    if (strstr(id3buffer,"TAG") != NULL)
    {
        goto ID3v1_FOUND;
    }
    else //look for ID3v2 tags when i implement it
    {
		sceIoClose(id3fd);
        return TmpID3;  //this is where the search for ID3v2 tags code will go
                        //when it gets implemented.
    }
   
    //**** An ID3v1 tag was found ****

    ID3v1_FOUND:
    sceIoLseek(id3fd, -125, SEEK_END);
    sceIoRead(id3fd,TmpID3.ID3Title,30);
	TmpID3.ID3Title[30] = '\0';

    sceIoLseek(id3fd, -95, SEEK_END);
    sceIoRead(id3fd,TmpID3.ID3Artist,30);
	TmpID3.ID3Artist[30] = '\0';

    sceIoLseek(id3fd, -65, SEEK_END);
    sceIoRead(id3fd,TmpID3.ID3Album,30);
	TmpID3.ID3Album[30] = '\0';

    sceIoLseek(id3fd, -35, SEEK_END);
    sceIoRead(id3fd,TmpID3.ID3Year,4);
	TmpID3.ID3Year[4] = '\0';

    sceIoLseek(id3fd, -31, SEEK_END);
    sceIoRead(id3fd,TmpID3.ID3Comment,30);
	TmpID3.ID3Comment[30] = '\0';

    sceIoLseek(id3fd, -1, SEEK_END);
    sceIoRead(id3fd,TmpID3.ID3GenreCode,1);
	TmpID3.ID3GenreCode[1] = '\0';

	if (((int)TmpID3.ID3GenreCode[0] >= 0) & ((int)TmpID3.ID3GenreCode[0] < genreNumber)){
		strcpy(TmpID3.ID3GenreText, genreList[(int)TmpID3.ID3GenreCode[0]].text);
	}
	else{
		strcpy(TmpID3.ID3GenreText, "");
	}
	TmpID3.ID3GenreText[30] = '\0';

	TmpID3.versionfound = 1;
    sceIoClose(id3fd);
   
    return TmpID3;
}
