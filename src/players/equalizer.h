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

//Equalizzatori per libMad:
struct equalizer{
	char name[32];
    int index;
	char shortName[4];
	double filter[32];
};

struct equalizersList{
	char name[20];
	struct equalizer EQ[10];
};


void EQ_init();
int EQ_getEqualizersNumber();
struct equalizer EQ_get(char *name);
struct equalizer EQ_getShort(char *shortName);
struct equalizer EQ_getIndex(int index);
