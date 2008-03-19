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
#include <stdlib.h>
#include <string.h>
#include "settings.h"

struct settings localSettings;

//Load settings from file:
int SETTINGS_load(char *fileName){
	FILE *f;
	char lineText[256];

	f = fopen(fileName, "rt");
	if (f == NULL){
		//Error opening file:
		return(0);
	}

    SETTINGS_default();
	while(fgets(lineText, 256, f) != NULL){
		int element = 0;
		char parName[10] = "";
		if (strlen(lineText) > 0){
			if (lineText[0] != '#'){
                //Tolgo caratteri di termine riga:
                if ((int)lineText[strlen(lineText) - 1] == 10 || (int)lineText[strlen(lineText) - 1] == 13){
                    lineText[strlen(lineText) - 1] = '\0';
                }
                if ((int)lineText[strlen(lineText) - 1] == 10 || (int)lineText[strlen(lineText) - 1] == 13){
                    lineText[strlen(lineText) - 1] = '\0';
                }
				//Split line:
				element = 0;
				char *result = NULL;
				result = strtok(lineText, "=");
				while(result != NULL){
					if (strlen(result) > 0){
						if (element == 0){
							strcpy(parName, result);
						}else if (element == 1){
                            if (strcmp(parName, "CPU") == 0){
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
							}else if (strcmp(parName, "USE_OSK") == 0){
								localSettings.USE_OSK = atoi(result);
							}else if (strcmp(parName, "FADE_OUT") == 0){
								localSettings.FADE_OUT = atoi(result);
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
	return(1);
}


//Get actual settings:
struct settings SETTINGS_get(){
	return localSettings;
}


//Get default settings:
struct settings SETTINGS_default(){
	localSettings.CPU = 65;
	localSettings.BUS = 54;
	strcpy(localSettings.EQ, "NO");
	strcpy(localSettings.BOOST, "NEW");
	localSettings.BOOST_VALUE = 0;
	localSettings.SCROBBLER = 0;
	localSettings.VOLUME = 20;
	localSettings.MP3_ME = 0;
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
	localSettings.USE_OSK = 1;
	return localSettings;
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
    fwrite("#Brightness check at startup (0=No, 1=Yes with warning, 2=Yes without warning):\n", 1, strlen("#Brightness check at startup (0=No, 1=Yes with warning, 2=Yes without warning):\n"), f);
    snprintf(testo, sizeof(testo), "BRIGHTNESS_CHECK=%i\n\n", tSettings.BRIGHTNESS_CHECK);
    fwrite(testo, 1, strlen(testo), f);

    fwrite("#CPU speed at startup (10-222):\n", 1, strlen("#CPU speed at startup (33-333):\n"), f);
    snprintf(testo, sizeof(testo), "CPU=%i\n\n", tSettings.CPU);
    fwrite(testo, 1, strlen(testo), f);

    fwrite("#BUS speed at startup (54-111):\n", 1, strlen("#BUS speed at startup (54-166):\n"), f);
    snprintf(testo, sizeof(testo), "BUS=%i\n\n", tSettings.BUS);
    fwrite(testo, 1, strlen(testo), f);

    fwrite("#EQ set at startup (short name R/P/D/...):\n", 1, strlen("#EQ set at startup (short name R/P/D/...):\n"), f);
    snprintf(testo, sizeof(testo), "EQ=%s\n\n", tSettings.EQ);
    fwrite(testo, 1, strlen(testo), f);

    fwrite("#Volume boost method (OLD/NEW) and value:\n", 1, strlen("#Volume boost method (OLD/NEW) and value:\n"), f);
    snprintf(testo, sizeof(testo), "BOOST=%s\n", tSettings.BOOST);
    fwrite(testo, 1, strlen(testo), f);
    snprintf(testo, sizeof(testo), "BOOST_VALUE=%i\n\n", tSettings.BOOST_VALUE);
    fwrite(testo, 1, strlen(testo), f);

    fwrite("#Use ME for MP3 (0/1):\n", 1, strlen("#Use ME for MP3 (0/1):\n"), f);
    snprintf(testo, sizeof(testo), "MP3_ME=%i\n\n", tSettings.MP3_ME);
    fwrite(testo, 1, strlen(testo), f);

    fwrite("#Use OSK (1/0):\n", 1, strlen("#Use OSK (1/0):\n"), f);
    snprintf(testo, sizeof(testo), "USE_OSK=%i\n\n", tSettings.USE_OSK);
    fwrite(testo, 1, strlen(testo), f);

    fwrite("#SCROBBLER Log (1/0):\n", 1, strlen("#SCROBBLER Log (1/0):\n"), f);
    snprintf(testo, sizeof(testo), "SCROBBLER=%i\n\n", tSettings.SCROBBLER);
    fwrite(testo, 1, strlen(testo), f);

    fwrite("#Volume level (0-30):\n", 1, strlen("#Volume level (0-30):\n"), f);
    snprintf(testo, sizeof(testo), "VOLUME=%i\n\n", tSettings.VOLUME);
    fwrite(testo, 1, strlen(testo), f);

    fwrite("#Fade out when changing track (0/1):\n", 1, strlen("#Fade out when changing track (0/1):\n"), f);
    snprintf(testo, sizeof(testo), "FADE_OUT=%i\n\n", tSettings.FADE_OUT);
    fwrite(testo, 1, strlen(testo), f);

    fwrite("#Muted volume (0/8000):\n", 1, strlen("#Muted volume (0/8000):\n"), f);
    snprintf(testo, sizeof(testo), "MUTED_VOLUME=%i\n\n", tSettings.MUTED_VOLUME);
    fwrite(testo, 1, strlen(testo), f);

    fwrite("#Automatic CPU clock (0/1):\n", 1, strlen("#Automatic CPU clock (0/1):\n"), f);
    snprintf(testo, sizeof(testo), "CLOCK_AUTO=%i\n\n", tSettings.CLOCK_AUTO);
    fwrite(testo, 1, strlen(testo), f);

    fwrite("#CPU clock for GUI (10/222):\n", 1, strlen("#CPU clock for GUI (10/222):\n"), f);
    snprintf(testo, sizeof(testo), "CLOCK_GUI=%i\n\n", tSettings.CLOCK_GUI);
    fwrite(testo, 1, strlen(testo), f);

    fwrite("#Clock for filetype (10/222):\n", 1, strlen("#Clock for filetype (10/222):\n"), f);
    snprintf(testo, sizeof(testo), "CLOCK_MP3=%i\n", tSettings.CLOCK_MP3);
    fwrite(testo, 1, strlen(testo), f);
    snprintf(testo, sizeof(testo), "CLOCK_MP3ME=%i\n", tSettings.CLOCK_MP3ME);
    fwrite(testo, 1, strlen(testo), f);
    snprintf(testo, sizeof(testo), "CLOCK_OGG=%i\n", tSettings.CLOCK_OGG);
    fwrite(testo, 1, strlen(testo), f);
    snprintf(testo, sizeof(testo), "CLOCK_FLAC=%i\n", tSettings.CLOCK_FLAC);
    fwrite(testo, 1, strlen(testo), f);
    snprintf(testo, sizeof(testo), "CLOCK_AA3=%i\n\n", tSettings.CLOCK_AA3);
    fwrite(testo, 1, strlen(testo), f);

	fclose(f);
    return(0);
}

