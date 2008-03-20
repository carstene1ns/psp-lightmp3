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
int readRGBA(int RGBA[], char *string){
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
							}else if (strcmp(parName, "USE_OSK") == 0){
								localSettings.USE_OSK = atoi(result);
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
                            }else if (strcmp(parName, "CLOCK_DELTA_ECONOMY_MODE") == 0){
								localSettings.CLOCK_DELTA_ECONOMY_MODE = atoi(result);
							}else if (strcmp(parName, "SKIN") == 0){
								strcpy(localSettings.skinName, result);
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

//Load settings from file:
int SETTINGS_loadSkin(char *fileName){
	FILE *f;
	char lineText[256];

	f = fopen(fileName, "rt");
	if (f == NULL){
		//Error opening file:
		return -1;
	}

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
                            if (strcmp(parName, "FILE_INFO_X") == 0){
								localSettings.fileInfoX = atoi(result);
                            }else if (strcmp(parName, "FILE_INFO_Y") == 0){
								localSettings.fileInfoY = atoi(result);
                            }else if (strcmp(parName, "FILE_SPECS_X") == 0){
								localSettings.fileSpecsX = atoi(result);
                            }else if (strcmp(parName, "FILE_SPECS_Y") == 0){
								localSettings.fileSpecsY = atoi(result);
                            }else if (strcmp(parName, "COVERART_X") == 0){
								localSettings.coverArtX = atoi(result);
                            }else if (strcmp(parName, "COVERART_Y") == 0){
								localSettings.coverArtY = atoi(result);
                            }else if (strcmp(parName, "PROGRESS_X") == 0){
								localSettings.progressX = atoi(result);
                            }else if (strcmp(parName, "PROGRESS_Y") == 0){
								localSettings.progressY = atoi(result);
                            }else if (strcmp(parName, "BUTTON_1_X") == 0){
								localSettings.buttonsX[0] = atoi(result);
                            }else if (strcmp(parName, "BUTTON_2_X") == 0){
								localSettings.buttonsX[1] = atoi(result);
                            }else if (strcmp(parName, "BUTTON_3_X") == 0){
								localSettings.buttonsX[2] = atoi(result);
                            }else if (strcmp(parName, "BUTTON_4_X") == 0){
								localSettings.buttonsX[3] = atoi(result);
                            }else if (strcmp(parName, "BUTTON_5_X") == 0){
								localSettings.buttonsX[4] = atoi(result);
                            }else if (strcmp(parName, "BUTTON_1_Y") == 0){
								localSettings.buttonsY[0] = atoi(result);
                            }else if (strcmp(parName, "BUTTON_2_Y") == 0){
								localSettings.buttonsY[1] = atoi(result);
                            }else if (strcmp(parName, "BUTTON_3_Y") == 0){
								localSettings.buttonsY[2] = atoi(result);
                            }else if (strcmp(parName, "BUTTON_4_Y") == 0){
								localSettings.buttonsY[3] = atoi(result);
                            }else if (strcmp(parName, "BUTTON_5_Y") == 0){
								localSettings.buttonsY[4] = atoi(result);
                            }else if (strcmp(parName, "PLAYER_STATUS_X") == 0){
								localSettings.playerStatusX = atoi(result);
                            }else if (strcmp(parName, "PLAYER_STATUS_Y") == 0){
								localSettings.playerStatusY = atoi(result);
                            }else if (strcmp(parName, "FILE_BROWSER_X") == 0){
								localSettings.fileBrowserX = atoi(result);
                            }else if (strcmp(parName, "FILE_BROWSER_Y") == 0){
								localSettings.fileBrowserY = atoi(result);
                            }else if (strcmp(parName, "PLAYLIST_BROWSER_X") == 0){
								localSettings.playlistBrowserX = atoi(result);
                            }else if (strcmp(parName, "PLAYLIST_BROWSER_Y") == 0){
								localSettings.playlistBrowserY = atoi(result);
                            }else if (strcmp(parName, "PLAYLIST_BROWSER_INFO_X") == 0){
								localSettings.playlistInfoX = atoi(result);
                            }else if (strcmp(parName, "PLAYLIST_BROWSER_INFO_Y") == 0){
								localSettings.playlistInfoY = atoi(result);
                            }else if (strcmp(parName, "PLAYLIST_EDITOR_X") == 0){
								localSettings.playlistEditorX = atoi(result);
                            }else if (strcmp(parName, "PLAYLIST_EDITOR_Y") == 0){
								localSettings.playlistEditorY = atoi(result);
                            }else if (strcmp(parName, "PLAYLIST_EDITOR_TRACK_INFO_X") == 0){
								localSettings.playlistTrackInfoX = atoi(result);
                            }else if (strcmp(parName, "PLAYLIST_EDITOR_TRACK_INFO_Y") == 0){
								localSettings.playlistTrackInfoY = atoi(result);
                            }else if (strcmp(parName, "SETTINGS_X") == 0){
								localSettings.settingsX = atoi(result);
                            }else if (strcmp(parName, "SETTINGS_Y") == 0){
								localSettings.settingsY = atoi(result);
                            }else if (strcmp(parName, "MEDIALIBRARY_X") == 0){
								localSettings.medialibraryX = atoi(result);
                            }else if (strcmp(parName, "MEDIALIBRARY_Y") == 0){
								localSettings.medialibraryY = atoi(result);
                            }else if (strcmp(parName, "TOOLBAR_FG_COLOR") == 0){
								readRGBA(localSettings.colorToolbar, result);
                            }else if (strcmp(parName, "BUTTONBAR_FG_COLOR") == 0){
								readRGBA(localSettings.colorButtonbar, result);
                            }else if (strcmp(parName, "BUTTONBAR_SELECTED_FG_COLOR") == 0){
								readRGBA(localSettings.colorButtonbarSelected, result);
                            }else if (strcmp(parName, "POPUP_TITLE_FG_COLOR") == 0){
								readRGBA(localSettings.colorPopupTitle, result);
                            }else if (strcmp(parName, "POPUP_MESSAGE_FG_COLOR") == 0){
								readRGBA(localSettings.colorPopupMessage, result);
                            }else if (strcmp(parName, "MENU_TEXT_FG_COLOR") == 0){
								readRGBA(localSettings.colorMenu, result);
                            }else if (strcmp(parName, "MENU_SELECTED_TEXT_FG_COLOR") == 0){
								readRGBA(localSettings.colorMenuSelected, result);
                            }else if (strcmp(parName, "LABEL_FG_COLOR") == 0){
								readRGBA(localSettings.colorLabel, result);
                            }else if (strcmp(parName, "TEXT_FG_COLOR") == 0){
								readRGBA(localSettings.colorText, result);
                            }else if (strcmp(parName, "HELP_TITLE_FG_COLOR") == 0){
								readRGBA(localSettings.colorHelpTitle, result);
                            }else if (strcmp(parName, "HELP_FG_COLOR") == 0){
								readRGBA(localSettings.colorHelp, result);
							}
						}
						element++;
					}
					result = strtok(NULL, "=");
				}
			}
		}
	}
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
	localSettings.CLOCK_DELTA_ECONOMY_MODE = 1;
	localSettings.USE_OSK = 1;
    localSettings.playMode = 0;

	//Skin's settings:
    strcpy(localSettings.skinName, "default");

    //Colors:
    int black[4] = {0,0,0,255};
    memcpy(localSettings.colorButtonbar, black, sizeof(black));
    memcpy(localSettings.colorPopupMessage, black, sizeof(black));
    memcpy(localSettings.colorPopupTitle, black, sizeof(black));
    memcpy(localSettings.colorToolbar, black, sizeof(black));
    memcpy(localSettings.colorLabel, black, sizeof(black));
    memcpy(localSettings.colorMenu, black, sizeof(black));
    memcpy(localSettings.colorMenuSelected, black, sizeof(black));
    memcpy(localSettings.colorText, black, sizeof(black));
    memcpy(localSettings.colorLabel, black, sizeof(black));
    memcpy(localSettings.colorHelpTitle, black, sizeof(black));
    memcpy(localSettings.colorHelp, black, sizeof(black));

    //Player's settings:
    localSettings.displayStatus = 1;

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
    snprintf(testo, sizeof(testo), "USE_OSK=%i\n", tSettings.USE_OSK);
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
    snprintf(testo, sizeof(testo), "CLOCK_DELTA_ECONOMY_MODE=%i\n", tSettings.CLOCK_DELTA_ECONOMY_MODE);
    fwrite(testo, 1, strlen(testo), f);

	fclose(f);
    return(0);
}

