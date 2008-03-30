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
#include "../players/m3u.h"
#include "../players/player.h"
#include "../system/osk.h"
#include "../system/clock.h"
#include "gui_playlistEditor.h"
#include "common.h"
#include "languages.h"
#include "settings.h"
#include "skinsettings.h"
#include "menu.h"

#define STATUS_CONFIRM_NONE 0
#define STATUS_CONFIRM_CLEAR 1
#define STATUS_CONFIRM_REMOVE 2

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Functions imported from prx:
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void readButtons(SceCtrlData *pad_data, int count);


///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Globals:
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
static int playlistEditorRetValue = 0;
static int exitFlagPlaylistEditor = 0;
OSL_IMAGE *trackInfoBkg;

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Build menu from current playlist:
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
int buildMenuFromPlaylist(struct menuElements *menu){
    struct menuElement tMenuEl;
    struct M3U_songEntry *entry;
    int i = 0;

    menu->first = 0;
    menu->selected = 0;
    menu->numberOfElements = M3U_getSongCount();
    for (i=0; i<menu->numberOfElements; i++){
        entry = M3U_getSong(i);
        strcpy(tMenuEl.text, entry->title);
        tMenuEl.triggerFunction = NULL;
        menu->elements[i] = tMenuEl;
    }
    return 0;
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Draw file info:
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
int drawTrackInfo(struct fileInfo *info){
    OSL_FONT *font = fontNormal;

    skinGetPosition("POS_PLAYLIST_EDITOR_TRACK_INFO_BKG", tempPos);
    oslDrawImageXY(trackInfoBkg, tempPos[0], tempPos[1]);
    oslSetFont(font);

    skinGetColor("RGBA_LABEL_TEXT", tempColor);
    oslSetTextColor(RGBA(tempColor[0], tempColor[1], tempColor[2], tempColor[3]));
    skinGetPosition("POS_PLAYLIST_TITLE_LABEL", tempPos);
    oslDrawString(tempPos[0], tempPos[1], langGetString("TITLE"));
    skinGetPosition("POS_PLAYLIST_ARTIST_LABEL", tempPos);
    oslDrawString(tempPos[0], tempPos[1], langGetString("ARTIST"));
    skinGetPosition("POS_PLAYLIST_ALBUM_LABEL", tempPos);
    oslDrawString(tempPos[0], tempPos[1], langGetString("ALBUM"));

    skinGetColor("RGBA_TEXT", tempColor);
    oslSetTextColor(RGBA(tempColor[0], tempColor[1], tempColor[2], tempColor[3]));
    skinGetPosition("POS_PLAYLIST_TITLE_VALUE", tempPos);
    oslDrawString(tempPos[0], tempPos[1], info->title);
    skinGetPosition("POS_PLAYLIST_ARTIST_VALUE", tempPos);
    oslDrawString(tempPos[0], tempPos[1], info->artist);
    skinGetPosition("POS_PLAYLIST_ALBUM_VALUE", tempPos);
    oslDrawString(tempPos[0], tempPos[1], info->album);

    return 0;
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Playlist Editor:
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
int gui_playlistEditor(){
    char buffer[264] = "";
    int oldSelected = 0;
    int oldFirst = 0;
    struct M3U_songEntry *song;
    struct fileInfo tagInfo;
    SceCtrlData pad;
    int confirmStatus = STATUS_CONFIRM_NONE;

    //Load images:
    sprintf(buffer, "%s/trackinfobkg.png", userSettings->skinImagesPath);
    trackInfoBkg = oslLoadImageFilePNG(buffer, OSL_IN_RAM | OSL_SWIZZLED, OSL_PF_8888);
    if (!trackInfoBkg)
        errorLoadImage(buffer);

    M3U_open(tempM3Ufile);
    if (M3U_getSongCount()){
        song = M3U_getSong(0);
        setAudioFunctions(song->fileName, userSettings->MP3_ME);
        tagInfo = (*getTagInfoFunct)(song->fileName);
        unsetAudioFunctions();
    }else{
        strcpy(tagInfo.title, "");
        strcpy(tagInfo.artist, "");
        strcpy(tagInfo.album, "");
    }


    //Build menu:
    skinGetPosition("POS_PLAYLIST_EDITOR", tempPos);
    commonMenu.yPos = tempPos[1];
    commonMenu.xPos = tempPos[0];
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
    buildMenuFromPlaylist(&commonMenu);

    exitFlagPlaylistEditor = 0;
    while(!osl_quit && !exitFlagPlaylistEditor){
        oslStartDrawing();
        drawCommonGraphics();
        drawButtonBar(MODE_PLAYLIST_EDITOR);
        drawMenu(&commonMenu);
        drawTrackInfo(&tagInfo);

        switch (confirmStatus){
            case STATUS_CONFIRM_CLEAR:
                drawConfirm(langGetString("CONFIRM_CLEAR_PLAYLIST_TITLE"), langGetString("CONFIRM_CLEAR_PLAYLIST"));
                break;
            case STATUS_CONFIRM_REMOVE:
                drawConfirm(langGetString("CONFIRM_REMOVE_FROM_PLAYLIST_TITLE"), langGetString("CONFIRM_REMOVE_FROM_PLAYLIST"));
                break;
        }

        oslReadKeys();
        readButtons(&pad, 1);
        if (!osl_pad.pressed.hold){
            if (confirmStatus == STATUS_CONFIRM_CLEAR){
                if(osl_pad.released.cross){
                    M3U_clear();
                    strcpy(userSettings->currentPlaylistName, "");
                    buildMenuFromPlaylist(&commonMenu);
                    confirmStatus = STATUS_CONFIRM_NONE;
                }else if(osl_pad.pressed.circle){
                    confirmStatus = STATUS_CONFIRM_NONE;
                }
            }else if (confirmStatus == STATUS_CONFIRM_REMOVE){
                if(osl_pad.released.cross){
                    M3U_removeSong(commonMenu.selected);
                    oldSelected = commonMenu.selected;
                    oldFirst = commonMenu.first;
                    buildMenuFromPlaylist(&commonMenu);
                    if (oldSelected < M3U_getSongCount())
        				commonMenu.selected = oldSelected;
                    else
                        commonMenu.selected = oldSelected - 1;
                    if (oldSelected < M3U_getSongCount())
        				commonMenu.first = oldFirst;
                    else
        				commonMenu.first = oldFirst - 1;
                    confirmStatus = STATUS_CONFIRM_NONE;
                }else if(osl_pad.released.circle){
                    confirmStatus = STATUS_CONFIRM_NONE;
                }
            }else{
                oldSelected = commonMenu.selected;
                processMenuKeys(&commonMenu);
                //Check if user changed track:
                if (commonMenu.selected != oldSelected){
                    song = M3U_getSong(commonMenu.selected);
                    setAudioFunctions(song->fileName, userSettings->MP3_ME);
                    tagInfo = (*getTagInfoFunct)(song->fileName);
                    unsetAudioFunctions();
                }

                if(pad.Buttons & PSP_CTRL_NOTE){
					sprintf(userSettings->selectedBrowserItem, "%s", tempM3Ufile);
                    userSettings->playlistStartIndex = -1;
                    playlistEditorRetValue = MODE_PLAYER;
                    userSettings->previousMode = MODE_PLAYLIST_EDITOR;
                    exitFlagPlaylistEditor = 1;
                }else if(osl_pad.pressed.square){
        			if (M3U_moveSongUp(commonMenu.selected) == 0){
                        oldSelected = commonMenu.selected;
                        oldFirst = commonMenu.first;
                        buildMenuFromPlaylist(&commonMenu);
        				commonMenu.selected = oldSelected - 1;
        				commonMenu.first = oldFirst;
        				if (commonMenu.selected == oldFirst - 1)
        					commonMenu.first--;
        			}
                }else if(osl_pad.pressed.cross){
        			if (M3U_moveSongDown(commonMenu.selected) == 0){
                        oldSelected = commonMenu.selected;
                        oldFirst = commonMenu.first;
                        buildMenuFromPlaylist(&commonMenu);
        				commonMenu.selected = oldSelected + 1;
        				commonMenu.first = oldFirst;
        				if (commonMenu.selected == oldFirst + commonMenu.maxNumberVisible)
        					commonMenu.first++;
        			}
                }else if(osl_pad.released.circle && M3U_getSongCount()){
                    confirmStatus = STATUS_CONFIRM_REMOVE;
                }else if(osl_pad.released.start && M3U_getSongCount()){
                    char onlyName[264] = "";
                	oslEndDrawing();
                    oslEndFrame();
                	oslSyncFrame();
                	setCpuClock(222);
                	getFileName(userSettings->currentPlaylistName, onlyName);
                    char *newName = requestString(langGetString("ASK_PLAYLIST_NAME"), onlyName);
                    char ext[4] = "";
                    getExtension(newName, ext, 3);
                    oslInitGfx(OSL_PF_8888, 1); //Re-init OSLib to avoid gaphics corruption!
                    setCpuClock(userSettings->CLOCK_GUI);
                    if (strlen(newName)){
                        if (strcmp(ext, "M3U"))
                            sprintf(buffer, "%s%s/%s.m3u", userSettings->ebootPath, "playLists", newName);
                        else
                        sprintf(buffer, "%s%s/%s", userSettings->ebootPath, "playLists", newName);
                        strcpy(userSettings->currentPlaylistName, buffer);
                        M3U_save(userSettings->currentPlaylistName);
                    }
                    continue;
                }else if(osl_pad.released.select){
                    confirmStatus = STATUS_CONFIRM_CLEAR;
                }else if(osl_pad.released.R){
                    playlistEditorRetValue = nextAppMode(MODE_PLAYLIST_EDITOR);
                    exitFlagPlaylistEditor = 1;
                }else if(osl_pad.released.L){
                    playlistEditorRetValue = previousAppMode(MODE_PLAYLIST_EDITOR);
                    exitFlagPlaylistEditor = 1;
                }
            }
        }
    	oslEndDrawing();
        oslEndFrame();
    	oslSyncFrame();
    }

    M3U_save(tempM3Ufile);
    //unLoad images:
    clearMenu(&commonMenu);
    oslDeleteImage(trackInfoBkg);

    return playlistEditorRetValue;
}
