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
#include "../system/opendir.h"
#include "../players/m3u.h"

#define STATUS_CONFIRM_NONE 0
#define STATUS_CONFIRM_REMOVE 1
#define STATUS_CONFIRM_LOAD 2

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
    oslSetTextColor(RGBA(tempColor[0], tempColor[1], tempColor[2], tempColor[3]));
    skinGetPosition("POS_PLAYLIST_NAME_LABEL", tempPos);
    oslDrawString(tempPos[0], tempPos[1], langGetString("PLAYLIST_NAME"));
    skinGetPosition("POS_PLAYLIST_TOTAL_TRACKS_LABEL", tempPos);
    oslDrawString(tempPos[0], tempPos[1], langGetString("TOTAL_TRACKS"));

    if (commonMenu.numberOfElements){
        skinGetColor("RGBA_TEXT", tempColor);
        oslSetTextColor(RGBA(tempColor[0], tempColor[1], tempColor[2], tempColor[3]));
        skinGetPosition("POS_PLAYLIST_NAME_VALUE", tempPos);
        oslDrawString(tempPos[0], tempPos[1], commonMenu.elements[commonMenu.selected].text);
        skinGetPosition("POS_PLAYLIST_TOTAL_TRACKS_VALUE", tempPos);
        sprintf(tBuffer, "%i", M3U_getSongCount());
        oslDrawString(tempPos[0], tempPos[1], tBuffer);

        formatHHMMSS(M3U_getTotalLength(), tBuffer);
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
    sprintf(playlistsDir, "%s%s", userSettings->ebootPath, "playLists");
	sceIoMkdir(playlistsDir, 0777);

    //Load images:
    sprintf(buffer, "%s/playlistinfobkg.png", userSettings->skinImagesPath);
    playlistInfoBkg = oslLoadImageFilePNG(buffer, OSL_IN_RAM | OSL_SWIZZLED, OSL_PF_8888);
    if (!playlistInfoBkg)
        errorLoadImage(buffer);

    //Build menu:
    skinGetPosition("POS_PLAYLIST_BROWSER", tempPos);
    commonMenu.xPos = tempPos[0];
    commonMenu.yPos = tempPos[1];
    commonMenu.fastScrolling = 1;
    sprintf(buffer, "%s/menuplaylistbkg.png", userSettings->skinImagesPath);
    commonMenu.background = oslLoadImageFilePNG(buffer, OSL_IN_RAM | OSL_SWIZZLED, OSL_PF_8888);
    if (!commonMenu.background)
        errorLoadImage(buffer);
    sprintf(buffer, "%s/menuhighlight.png", userSettings->skinImagesPath);
    commonMenu.highlight = oslLoadImageFilePNG(buffer, OSL_IN_RAM | OSL_SWIZZLED, OSL_PF_8888);
    if (!commonMenu.highlight)
        errorLoadImage(buffer);
    commonMenu.width = commonMenu.background->sizeX;
    commonMenu.height = commonMenu.background->sizeY;
    commonMenu.interline = 1;
    commonMenu.maxNumberVisible = commonMenu.background->sizeY / (fontNormal->charHeight + commonMenu.interline);
    commonMenu.cancelFunction = NULL;

    result = opendir_open(&directory, playlistsDir, ext, 1, 1);
    sortDirectory(&directory);
    buildMenuFromDirectory(&commonMenu, &directory, "");

    strcpy(buffer, "");
    exitFlagPlaylistsBrowser = 0;
    while(!osl_quit && !exitFlagPlaylistsBrowser){
        oslStartDrawing();

        if (commonMenu.selected >= 0 && strcmp(buffer, commonMenu.elements[commonMenu.selected].text)){
            sprintf(buffer, "%s/%s", playlistsDir, commonMenu.elements[commonMenu.selected].text);
			M3U_open(buffer);
			strcpy(buffer, commonMenu.elements[commonMenu.selected].text);
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
        }

        oslReadKeys();
        if (!osl_keys->pressed.hold){
            processMenuKeys(&commonMenu);

            if (!confirmStatus){
                if(osl_keys->released.cross && commonMenu.numberOfElements){
                    sprintf(userSettings->selectedBrowserItem, "%s/%s", playlistsDir, commonMenu.elements[commonMenu.selected].text);
                    userSettings->playlistStartIndex = -1;
                    playlistsBrowserRetValue = MODE_PLAYER;
                    userSettings->previousMode = MODE_PLAYLISTS;
                    exitFlagPlaylistsBrowser = 1;
                }else if(osl_keys->released.square && commonMenu.numberOfElements){
                    confirmStatus = STATUS_CONFIRM_LOAD;
                }else if(osl_keys->released.circle && commonMenu.numberOfElements){
                    confirmStatus = STATUS_CONFIRM_REMOVE;
                }else if(osl_keys->released.R){
                    playlistsBrowserRetValue = nextAppMode(MODE_PLAYLISTS);
                    exitFlagPlaylistsBrowser = 1;
                }else if(osl_keys->released.L){
                    playlistsBrowserRetValue = previousAppMode(MODE_PLAYLISTS);
                    exitFlagPlaylistsBrowser = 1;
                }
            }else if (confirmStatus == STATUS_CONFIRM_LOAD){
                if(osl_keys->released.cross){
                    sprintf(buffer, "%s/%s", playlistsDir, commonMenu.elements[commonMenu.selected].text);
                    M3U_open(buffer);
                    M3U_save(tempM3Ufile);
                    strcpy(userSettings->currentPlaylistName, buffer);
                    /*playlistsBrowserRetValue = MODE_PLAYLIST_EDITOR;
                    userSettings->previousMode = MODE_PLAYLISTS;
                    exitFlagPlaylistsBrowser = 1;*/
                    confirmStatus = STATUS_CONFIRM_NONE;
                }else if(osl_keys->released.circle){
                    confirmStatus = STATUS_CONFIRM_NONE;
                }
            }else if (confirmStatus == STATUS_CONFIRM_REMOVE){
                if(osl_keys->released.cross){
                    sprintf(buffer, "%s/%s", playlistsDir, commonMenu.elements[commonMenu.selected].text);
                    sceIoRemove(buffer);
                    opendir_close(&directory);
                    result = opendir_open(&directory, playlistsDir, ext, 1, 1);
                    sortDirectory(&directory);
                    buildMenuFromDirectory(&commonMenu, &directory, "");
                    commonMenu.first = 0;
                    commonMenu.selected = 0;
                    confirmStatus = STATUS_CONFIRM_NONE;
                }else if(osl_keys->released.circle){
                    confirmStatus = STATUS_CONFIRM_NONE;
                }
            }
        }
    	oslEndDrawing();
    	oslSyncFrame();
    }
    opendir_close(&directory);
    M3U_clear();
    //unLoad images:
    clearMenu(&commonMenu);
    oslDeleteImage(playlistInfoBkg);

    return playlistsBrowserRetValue;
}
