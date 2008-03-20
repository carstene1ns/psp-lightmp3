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

#ifndef __setting_h
#define __setting_h (1)

//Settings:
struct settings{
    char ebootPath[264];
	char fileName[264];
	char lang[32];
	int CPU;
	int BUS;
	char EQ[5];
	char BOOST[4];
	int BOOST_VALUE;
	int SCROBBLER;
	int VOLUME;
	int MP3_ME;
	int FADE_OUT;
    int MUTED_VOLUME;
    int BRIGHTNESS_CHECK;

	//CPU clock for filetype:
    int CLOCK_GUI;
    int CLOCK_AUTO;
    int CLOCK_MP3;
    int CLOCK_MP3ME;
    int CLOCK_OGG;
    int CLOCK_FLAC;
    int CLOCK_AA3;
    int CLOCK_AAC;
    int CLOCK_DELTA_ECONOMY_MODE;

    int USE_OSK;

	//Skin's settings:
    char skinName[52];
    char skinImagesPath[264];
    int coverArtX;
    int coverArtY;
    int fileInfoX;
    int fileInfoY;
    int fileSpecsX;
    int fileSpecsY;
    int progressX;
    int progressY;
    int playerStatusX;
    int playerStatusY;
    int buttonsX[5];
    int buttonsY[5];
    int fileBrowserX;
    int fileBrowserY;
    int playlistBrowserX;
    int playlistBrowserY;
    int playlistInfoX;
    int playlistInfoY;
    int playlistEditorX;
    int playlistEditorY;
    int playlistTrackInfoX;
    int playlistTrackInfoY;
    int settingsX;
    int settingsY;
    int medialibraryX;
    int medialibraryY;

    //Colors:
    int colorToolbar[4];
    int colorButtonbar[4];
    int colorButtonbarSelected[4];
    int colorPopupTitle[4];
    int colorPopupMessage[4];
    int colorMenu[4];
    int colorMenuSelected[4];
    int colorLabel[4];
    int colorText[4];
    int colorHelpTitle[4];
    int colorHelp[4];

    //Player Settings:
    int displayStatus;
    int curBrightness;
    int playMode;
    int sleepMode;
    int volumeBoost;
    int currentEQ;
    int muted;

	//Data to be passed between forms:
    int shutDown;
    char lastBrowserDir[264];
    char selectedBrowserItem[264];
    int previousMode;
    char currentPlaylistName[264];
};

int SETTINGS_load(char *fileName);
int SETTINGS_loadSkin(char *fileName);
struct settings *SETTINGS_get();
struct settings *SETTINGS_default();
int SETTINGS_save();
#endif
