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
#include <pspkernel.h>
#include <pspsdk.h>
#include <stdio.h>
#include <stdlib.h>
#include <oslib/oslib.h>

#include "../main.h"
#include "gui_playlistsBrowser.h"
#include "common.h"
#include "languages.h"
#include "settings.h"
#include "skinsettings.h"
#include "menu.h"
#include "../system/clock.h"
#include "../system/opendir.h"
#include "../players/m3u.h"


#define STATUS_CONFIRM_NONE 0
#define STATUS_CONFIRM_REMOVE 1
#define STATUS_CONFIRM_LOAD 2
#define STATUS_CONFIRM_ADD 3
#define STATUS_HELP 4

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Globals:
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
static int playlistsBrowserRetValue = 0;
static int exitFlagPlaylistsBrowser = 0;
OSL_IMAGE *playlistInfoBkg;

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Draw playlist's info:
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
int drawPlaylistInfo(){
    OSL_FONT *font = fontNormal;
    char tBuffer[264] = "";

    skinGetPosition("POS_PLAYLIST_BROWSER_INFO_BKG", tempPos);
    oslDrawImageXY(playlistInfoBkg, tempPos[0], tempPos[1]);
    oslSetFont(font);

    skinGetColor("RGBA_LABEL_TEXT", tempColor);
    skinGetColor("RGBA_LABEL_TEXT_SHADOW", tempColorShadow);
    setFontStyle(fontNormal, defaultTextSize, RGBA(tempColor[0], tempColor[1], tempColor[2], tempColor[3]), RGBA(tempColorShadow[0], tempColorShadow[1], tempColorShadow[2], tempColorShadow[3]), INTRAFONT_ALIGN_LEFT);
    skinGetPosition("POS_PLAYLIST_NAME_LABEL", tempPos);
    oslDrawString(tempPos[0], tempPos[1], langGetString("PLAYLIST_NAME"));
    skinGetPosition("POS_PLAYLIST_TOTAL_TRACKS_LABEL", tempPos);
    oslDrawString(tempPos[0], tempPos[1], langGetString("TOTAL_TRACKS"));
    skinGetPosition("POS_PLAYLIST_TOTAL_TIME_LABEL", tempPos);
    oslDrawString(tempPos[0], tempPos[1], langGetString("TOTAL_TIME"));

    if (commonMenu.numberOfElements){
        skinGetColor("RGBA_TEXT", tempColor);
        skinGetColor("RGBA_TEXT_SHADOW", tempColorShadow);
        setFontStyle(fontNormal, defaultTextSize, RGBA(tempColor[0], tempColor[1], tempColor[2], tempColor[3]), RGBA(tempColorShadow[0], tempColorShadow[1], tempColorShadow[2], tempColorShadow[3]), INTRAFONT_ALIGN_LEFT);
        skinGetPosition("POS_PLAYLIST_NAME_VALUE", tempPos);
        oslDrawString(tempPos[0], tempPos[1], commonMenu.elements[commonMenu.selected]->text);
        skinGetPosition("POS_PLAYLIST_TOTAL_TRACKS_VALUE", tempPos);
        snprintf(tBuffer, sizeof(tBuffer), "%i", M3U_getSongCount());
        oslDrawString(tempPos[0], tempPos[1], tBuffer);

        formatHHMMSS(M3U_getTotalLength(), tBuffer, sizeof(tBuffer));
        skinGetPosition("POS_PLAYLIST_TOTAL_TIME_VALUE", tempPos);
        oslDrawString(tempPos[0], tempPos[1], tBuffer);
    }
    return 0;
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Playlists browser:
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
int gui_playlistsBrowser(){
    struct opendir_struct directory;
    char *result;
    char buffer[264];
    char playlistsDir[264] = "";
    char ext[1][5] = {"M3U"};
    int confirmStatus = STATUS_CONFIRM_NONE;

    //Create plylists directory:
    snprintf(playlistsDir, sizeof(playlistsDir), "%s%s", userSettings->ebootPath, "playLists");
	sceIoMkdir(playlistsDir, 0777);

    //Load images:
    snprintf(buffer, sizeof(buffer), "%s/playlistinfobkg.png", userSettings->skinImagesPath);
    playlistInfoBkg = oslLoadImageFilePNG(buffer, OSL_IN_RAM | OSL_SWIZZLED, OSL_PF_8888);
    if (!playlistInfoBkg)
        errorLoadImage(buffer);

    //Build menu:
    skinGetPosition("POS_PLAYLIST_BROWSER", tempPos);
    commonMenu.xPos = tempPos[0];
    commonMenu.yPos = tempPos[1];
    commonMenu.fastScrolling = 1;
    snprintf(buffer, sizeof(buffer), "%s/menuplaylistbkg.png", userSettings->skinImagesPath);
    commonMenu.background = oslLoadImageFilePNG(buffer, OSL_IN_RAM | OSL_SWIZZLED, OSL_PF_8888);
    if (!commonMenu.background)
        errorLoadImage(buffer);
    commonMenu.highlight = commonMenuHighlight;
    commonMenu.width = commonMenu.background->sizeX;
    commonMenu.height = commonMenu.background->sizeY;
    fixMenuSize(&commonMenu);
    commonMenu.interline = skinGetParam("MENU_INTERLINE");
    commonMenu.maxNumberVisible = commonMenu.background->sizeY / (fontNormal->charHeight + commonMenu.interline);
    commonMenu.cancelFunction = NULL;

    result = opendir_open(&directory, playlistsDir, playlistsDir, ext, 1, 1);
    buildMenuFromDirectory(&commonMenu, &directory, "");

    strcpy(buffer, "");
    exitFlagPlaylistsBrowser = 0;
	int skip = 0;
	cpuRestore();
    while(!osl_quit && !exitFlagPlaylistsBrowser){
		if (!skip){
			oslStartDrawing();

			if (commonMenu.selected >= 0 && strcmp(buffer, commonMenu.elements[commonMenu.selected]->text)){
				snprintf(buffer, sizeof(buffer), "%s/%s", playlistsDir, commonMenu.elements[commonMenu.selected]->text);
				M3U_clear();
				M3U_open(buffer);
				strcpy(buffer, commonMenu.elements[commonMenu.selected]->text);
			}

			drawCommonGraphics();
			drawButtonBar(MODE_PLAYLISTS);
			drawMenu(&commonMenu);
			drawPlaylistInfo();

			switch (confirmStatus){
				case STATUS_CONFIRM_LOAD:
					drawConfirm(langGetString("CONFIRM_LOAD_TITLE"), langGetString("CONFIRM_LOAD"));
					break;
				case STATUS_CONFIRM_REMOVE:
					drawConfirm(langGetString("CONFIRM_REMOVE_TITLE"), langGetString("CONFIRM_REMOVE"));
					break;
				case STATUS_CONFIRM_ADD:
					drawConfirm(langGetString("CONFIRM_ADD_PLAYLIST_TITLE"), langGetString("CONFIRM_ADD_PLAYLIST"));
					break;
				case STATUS_HELP:
					drawHelp("PLAYLIST_BROWSER");
					break;
			}
	    	oslEndDrawing();
		}
        oslEndFrame();
    	skip = oslSyncFrame();


        oslReadKeys();
        if (!confirmStatus){
            processMenuKeys(&commonMenu);
            if(getConfirmButton() && commonMenu.numberOfElements){
				snprintf(userSettings->selectedBrowserItem, sizeof(userSettings->selectedBrowserItem), "%s/%s", playlistsDir, directory.directory_entry[commonMenu.selected].longname);
				snprintf(userSettings->selectedBrowserItemShort, sizeof(userSettings->selectedBrowserItemShort), "%s/%s", playlistsDir, directory.directory_entry[commonMenu.selected].d_name);
				userSettings->playlistStartIndex = -1;
                playlistsBrowserRetValue = MODE_PLAYER;
                userSettings->previousMode = MODE_PLAYLISTS;
                exitFlagPlaylistsBrowser = 1;
            }else if(osl_pad.released.square && commonMenu.numberOfElements){
                confirmStatus = STATUS_CONFIRM_LOAD;
            }else if(getCancelButton() && commonMenu.numberOfElements){
                confirmStatus = STATUS_CONFIRM_REMOVE;
            }else if(osl_pad.released.start && commonMenu.numberOfElements){
                confirmStatus = STATUS_CONFIRM_ADD;
            }else if(osl_pad.released.R){
                playlistsBrowserRetValue = nextAppMode(MODE_PLAYLISTS);
                exitFlagPlaylistsBrowser = 1;
            }else if(osl_pad.released.L){
                playlistsBrowserRetValue = previousAppMode(MODE_PLAYLISTS);
                exitFlagPlaylistsBrowser = 1;
            }else if (osl_pad.held.L && osl_pad.held.R){
                confirmStatus = STATUS_HELP;
            }
        }else if (confirmStatus == STATUS_HELP){
            if (getConfirmButton() || getCancelButton())
                confirmStatus = STATUS_CONFIRM_NONE;
        }else if (confirmStatus == STATUS_CONFIRM_LOAD){
            if(getConfirmButton()){
                snprintf(buffer, sizeof(buffer), "%s/%s", playlistsDir, commonMenu.elements[commonMenu.selected]->text);
                M3U_clear();
                M3U_open(buffer);
                M3U_save(tempM3Ufile);
                strcpy(userSettings->currentPlaylistName, buffer);
                confirmStatus = STATUS_CONFIRM_NONE;
            }else if(getCancelButton()){
                confirmStatus = STATUS_CONFIRM_NONE;
            }
        }else if (confirmStatus == STATUS_CONFIRM_ADD){
            if(getConfirmButton()){
                M3U_clear();
                M3U_open(tempM3Ufile);
                snprintf(buffer, sizeof(buffer), "%s/%s", playlistsDir, commonMenu.elements[commonMenu.selected]->text);
                M3U_open(buffer);
                M3U_save(tempM3Ufile);
                confirmStatus = STATUS_CONFIRM_NONE;
            }else if(getCancelButton()){
                confirmStatus = STATUS_CONFIRM_NONE;
            }
        }else if (confirmStatus == STATUS_CONFIRM_REMOVE){
            if(getConfirmButton()){
                snprintf(buffer, sizeof(buffer), "%s/%s", playlistsDir, commonMenu.elements[commonMenu.selected]->text);
                sceIoRemove(buffer);
                opendir_close(&directory);
				cpuBoost();
                result = opendir_open(&directory, playlistsDir, playlistsDir, ext, 1, 1);
                buildMenuFromDirectory(&commonMenu, &directory, "");
				cpuRestore();
                commonMenu.first = 0;
                commonMenu.selected = 0;
                confirmStatus = STATUS_CONFIRM_NONE;
            }else if(getCancelButton()){
                confirmStatus = STATUS_CONFIRM_NONE;
            }
        }
    }
    opendir_close(&directory);
    M3U_clear();
    //unLoad images:
    clearMenu(&commonMenu);
    oslDeleteImage(playlistInfoBkg);
    playlistInfoBkg = NULL;

    return playlistsBrowserRetValue;
}
