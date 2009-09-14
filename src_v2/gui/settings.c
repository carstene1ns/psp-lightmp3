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
#include "settings.h"

struct settings localSettings;

//Read a RGB color from settings (rrr,ggg,bbb,aaaa)
int readRGBA(int *RGBA, char *string){
    char *result = NULL;
    int count = 0;

    result = strtok(string, ",");
    while(result != NULL){
        RGBA[count] = atoi(result);
        if (++count > 3)
            break;
        result = strtok(NULL, ",");
    }
    return 0;
}

//Load settings from file:
int SETTINGS_load(char *fileName){
	FILE *f;
	char lineText[256];

	f = fopen(fileName, "rt");
	if (f == NULL){
		//Error opening file:
		return -1;
	}

    SETTINGS_default();
	while(fgets(lineText, 256, f) != NULL){
		int element = 0;
		char parName[10] = "";
		if (strlen(lineText) > 0){
			if (lineText[0] != '#'){
                //Tolgo caratteri di termine riga:
                if ((int)lineText[strlen(lineText) - 1] == 10 || (int)lineText[strlen(lineText) - 1] == 13)
                    lineText[strlen(lineText) - 1] = '\0';
                if ((int)lineText[strlen(lineText) - 1] == 10 || (int)lineText[strlen(lineText) - 1] == 13)
                    lineText[strlen(lineText) - 1] = '\0';
				//Split line:
				element = 0;
				char *result = NULL;
				result = strtok(lineText, "=");
				while(result != NULL){
					if (strlen(result) > 0){
						if (element == 0){
							strcpy(parName, result);
						}else if (element == 1){
                            if (strcmp(parName, "LANG") == 0){
								strcpy(localSettings.lang, result);
                            }else if (strcmp(parName, "CPU") == 0){
								localSettings.CPU = atoi(result);
							}else if (strcmp(parName, "BUS") == 0){
								localSettings.BUS = atoi(result);
							}else if (strcmp(parName, "EQ") == 0){
								strcpy(localSettings.EQ, result);
							}else if (strcmp(parName, "BOOST") == 0){
								strcpy(localSettings.BOOST, result);
							}else if (strcmp(parName, "BOOST_VALUE") == 0){
								localSettings.BOOST_VALUE = atoi(result);
							}else if (strcmp(parName, "SCROBBLER") == 0){
								localSettings.SCROBBLER = atoi(result);
							}else if (strcmp(parName, "VOLUME") == 0){
								localSettings.VOLUME = atoi(result);
							}else if (strcmp(parName, "MP3_ME") == 0){
								localSettings.MP3_ME = atoi(result);
							}else if (strcmp(parName, "FADE_OUT") == 0){
								localSettings.FADE_OUT = atoi(result);
							}else if (strcmp(parName, "PLAY_MODE") == 0){
								localSettings.playMode = atoi(result);
                            }else if (strcmp(parName, "BRIGHTNESS_CHECK") == 0){
								localSettings.BRIGHTNESS_CHECK = atoi(result);
							}else if (strcmp(parName, "MUTED_VOLUME") == 0){
								localSettings.MUTED_VOLUME = atoi(result);
							}else if (strcmp(parName, "CLOCK_GUI") == 0){
								localSettings.CLOCK_GUI = atoi(result);
							}else if (strcmp(parName, "CLOCK_AUTO") == 0){
								localSettings.CLOCK_AUTO = atoi(result);
							}else if (strcmp(parName, "CLOCK_MP3") == 0){
								localSettings.CLOCK_MP3 = atoi(result);
							}else if (strcmp(parName, "CLOCK_MP3ME") == 0){
								localSettings.CLOCK_MP3ME = atoi(result);
							}else if (strcmp(parName, "CLOCK_OGG") == 0){
								localSettings.CLOCK_OGG = atoi(result);
							}else if (strcmp(parName, "CLOCK_FLAC") == 0){
								localSettings.CLOCK_FLAC = atoi(result);
							}else if (strcmp(parName, "CLOCK_AA3") == 0){
								localSettings.CLOCK_AA3 = atoi(result);
							}else if (strcmp(parName, "CLOCK_AAC") == 0){
								localSettings.CLOCK_AAC = atoi(result);
							}else if (strcmp(parName, "CLOCK_WMA") == 0){
								localSettings.CLOCK_WMA = atoi(result);
                            }else if (strcmp(parName, "CLOCK_DELTA_ECONOMY_MODE") == 0){
								localSettings.CLOCK_DELTA_ECONOMY_MODE = atoi(result);
							}else if (strcmp(parName, "SKIN") == 0){
								strcpy(localSettings.skinName, result);
							}else if (strcmp(parName, "KEY_AUTOREPEAT_PLAYER") == 0){
								localSettings.KEY_AUTOREPEAT_PLAYER = atoi(result);
							}else if (strcmp(parName, "KEY_AUTOREPEAT_GUI") == 0){
								localSettings.KEY_AUTOREPEAT_GUI = atoi(result);
							}else if (strcmp(parName, "SHOW_SPLASH") == 0){
								localSettings.SHOW_SPLASH = atoi(result);
							}else if (strcmp(parName, "ML_ROOT") == 0){
								strcpy(localSettings.mediaLibraryRoot, result);
							}else if (strcmp(parName, "SHUTDOWN_AFTER_BOOKMARK") == 0){
								localSettings.shutDownAfterBookmark = atoi(result);
							}else if (strcmp(parName, "HOLD_DISPLAYOFF") == 0){
								localSettings.HOLD_DISPLAYOFF = atoi(result);
							}else if (strcmp(parName, "FRAMESKIP") == 0){
								localSettings.frameskip = atoi(result);
							}
						}
						element++;
					}
					result = strtok(NULL, "=");
				}
			}
		}
	}
	strcpy(localSettings.fileName, fileName);
	fclose(f);
	return 0;
}


//Get actual settings:
struct settings *SETTINGS_get(){
	return &localSettings;
}


//Get default settings:
struct settings *SETTINGS_default(){
	strcpy(localSettings.lang, "English");
	localSettings.CPU = 65;
	localSettings.BUS = 54;
	strcpy(localSettings.EQ, "NO");
	strcpy(localSettings.BOOST, "NEW");
	localSettings.BOOST_VALUE = 0;
	localSettings.SCROBBLER = 0;
	localSettings.VOLUME = 20;
	localSettings.MP3_ME = 1;
	localSettings.FADE_OUT = 0;
	localSettings.BRIGHTNESS_CHECK = 1;
	localSettings.MUTED_VOLUME = 800;
	localSettings.CLOCK_AUTO = 1;
	localSettings.CLOCK_GUI = 50;
	localSettings.CLOCK_MP3 = 90;
	localSettings.CLOCK_MP3ME = 40;
	localSettings.CLOCK_OGG = 70;
	localSettings.CLOCK_FLAC = 166;
	localSettings.CLOCK_AA3 = 40;
	localSettings.CLOCK_AAC = 60;
    localSettings.CLOCK_WMA = 40;
	localSettings.CLOCK_DELTA_ECONOMY_MODE = 1;
    localSettings.playMode = 0;
    localSettings.KEY_AUTOREPEAT_GUI = 10;
    localSettings.KEY_AUTOREPEAT_PLAYER = 10;
	localSettings.SHOW_SPLASH = 1;
    localSettings.frameskip = 0;
    localSettings.startTab = 0;

	//Skin's settings:
    strcpy(localSettings.skinName, "default");

    //Player's settings:
    localSettings.displayStatus = 1;

    //Media Library settings:
    strcpy(localSettings.mediaLibraryRoot, "ms0:/");

    localSettings.shutDownAfterBookmark = 0;
    localSettings.HOLD_DISPLAYOFF = 0;
	return &localSettings;
}


//Save settings:
int SETTINGS_save(struct settings tSettings){
	FILE *f;
	char testo[256];

	f = fopen(tSettings.fileName, "w");
	if (f == NULL){
		//Error opening file:
		return(-1);
	}

    snprintf(testo, sizeof(testo), "LANG=%s\n", tSettings.lang);
    fwrite(testo, 1, strlen(testo), f);
    snprintf(testo, sizeof(testo), "SKIN=%s\n", tSettings.skinName);
    fwrite(testo, 1, strlen(testo), f);
    snprintf(testo, sizeof(testo), "PLAY_MODE=%i\n", tSettings.playMode);
    fwrite(testo, 1, strlen(testo), f);
    snprintf(testo, sizeof(testo), "BRIGHTNESS_CHECK=%i\n", tSettings.BRIGHTNESS_CHECK);
    fwrite(testo, 1, strlen(testo), f);
    snprintf(testo, sizeof(testo), "CPU=%i\n", tSettings.CPU);
    fwrite(testo, 1, strlen(testo), f);
    snprintf(testo, sizeof(testo), "BUS=%i\n", tSettings.BUS);
    fwrite(testo, 1, strlen(testo), f);
    snprintf(testo, sizeof(testo), "EQ=%s\n", tSettings.EQ);
    fwrite(testo, 1, strlen(testo), f);
    snprintf(testo, sizeof(testo), "BOOST=%s\n", tSettings.BOOST);
    fwrite(testo, 1, strlen(testo), f);
    snprintf(testo, sizeof(testo), "BOOST_VALUE=%i\n", tSettings.BOOST_VALUE);
    fwrite(testo, 1, strlen(testo), f);
    snprintf(testo, sizeof(testo), "MP3_ME=%i\n", tSettings.MP3_ME);
    fwrite(testo, 1, strlen(testo), f);
    snprintf(testo, sizeof(testo), "SCROBBLER=%i\n", tSettings.SCROBBLER);
    fwrite(testo, 1, strlen(testo), f);
    snprintf(testo, sizeof(testo), "VOLUME=%i\n", tSettings.VOLUME);
    fwrite(testo, 1, strlen(testo), f);
    snprintf(testo, sizeof(testo), "FADE_OUT=%i\n", tSettings.FADE_OUT);
    fwrite(testo, 1, strlen(testo), f);
    snprintf(testo, sizeof(testo), "MUTED_VOLUME=%i\n", tSettings.MUTED_VOLUME);
    fwrite(testo, 1, strlen(testo), f);
    snprintf(testo, sizeof(testo), "CLOCK_AUTO=%i\n", tSettings.CLOCK_AUTO);
    fwrite(testo, 1, strlen(testo), f);
    snprintf(testo, sizeof(testo), "CLOCK_GUI=%i\n", tSettings.CLOCK_GUI);
    fwrite(testo, 1, strlen(testo), f);
    snprintf(testo, sizeof(testo), "CLOCK_MP3=%i\n", tSettings.CLOCK_MP3);
    fwrite(testo, 1, strlen(testo), f);
    snprintf(testo, sizeof(testo), "CLOCK_MP3ME=%i\n", tSettings.CLOCK_MP3ME);
    fwrite(testo, 1, strlen(testo), f);
    snprintf(testo, sizeof(testo), "CLOCK_OGG=%i\n", tSettings.CLOCK_OGG);
    fwrite(testo, 1, strlen(testo), f);
    snprintf(testo, sizeof(testo), "CLOCK_FLAC=%i\n", tSettings.CLOCK_FLAC);
    fwrite(testo, 1, strlen(testo), f);
    snprintf(testo, sizeof(testo), "CLOCK_AA3=%i\n", tSettings.CLOCK_AA3);
    fwrite(testo, 1, strlen(testo), f);
    snprintf(testo, sizeof(testo), "CLOCK_AAC=%i\n", tSettings.CLOCK_AAC);
    fwrite(testo, 1, strlen(testo), f);
    snprintf(testo, sizeof(testo), "CLOCK_WMA=%i\n", tSettings.CLOCK_WMA);
    fwrite(testo, 1, strlen(testo), f);
    snprintf(testo, sizeof(testo), "CLOCK_DELTA_ECONOMY_MODE=%i\n", tSettings.CLOCK_DELTA_ECONOMY_MODE);
    fwrite(testo, 1, strlen(testo), f);
    snprintf(testo, sizeof(testo), "KEY_AUTOREPEAT_PLAYER=%i\n", tSettings.KEY_AUTOREPEAT_PLAYER);
    fwrite(testo, 1, strlen(testo), f);
    snprintf(testo, sizeof(testo), "KEY_AUTOREPEAT_GUI=%i\n", tSettings.KEY_AUTOREPEAT_GUI);
    fwrite(testo, 1, strlen(testo), f);
    snprintf(testo, sizeof(testo), "SHOW_SPLASH=%i\n", tSettings.SHOW_SPLASH);
    fwrite(testo, 1, strlen(testo), f);
    snprintf(testo, sizeof(testo), "ML_ROOT=%s\n", tSettings.mediaLibraryRoot);
    fwrite(testo, 1, strlen(testo), f);
    snprintf(testo, sizeof(testo), "SHUTDOWN_AFTER_BOOKMARK=%i\n", tSettings.shutDownAfterBookmark);
    fwrite(testo, 1, strlen(testo), f);
    snprintf(testo, sizeof(testo), "HOLD_DISPLAYOFF=%i\n", tSettings.HOLD_DISPLAYOFF);
    fwrite(testo, 1, strlen(testo), f);
    snprintf(testo, sizeof(testo), "FRAMESKIP=%i\n", tSettings.frameskip);
    fwrite(testo, 1, strlen(testo), f);
    snprintf(testo, sizeof(testo), "START_TAB=%i\n", tSettings.startTab);
    fwrite(testo, 1, strlen(testo), f);

	fclose(f);
    return(0);
}

