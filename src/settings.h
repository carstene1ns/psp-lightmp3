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

//Settings:
struct settings{
	char fileName[262];
	int CPU;
	int BUS;
	char EQ[5];
	char BOOST[4];
	int BOOST_VALUE;
	int SCROBBLER;
	int VOLUME;
	int MP3_ME;
	int FADE_OUT;
};

int SETTINGS_load(char *fileName);
struct settings SETTINGS_get();
struct settings SETTINGS_default();
int SETTINGS_save();
