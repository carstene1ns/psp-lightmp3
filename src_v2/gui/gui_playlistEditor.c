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
#define STATUS_HELP 3

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Globals:
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
static int playlistEditorRetValue = 0;
static int exitFlagPlaylistEditor = 0;
OSL_IMAGE *trackInfoBkg;
struct fileInfo tagInfo;


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
        limitString(entry->title, menu->width, tMenuEl.text);
        //strcpy(tMenuEl.text, entry->title);
        tMenuEl.triggerFunction = NULL;
        tMenuEl.icon = musicIcon;
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
    skinGetColor("RGBA_LABEL_TEXT_SHADOW", tempColorShadow);
    setFontStyle(fontNormal, defaultTextSize, RGBA(tempColor[0], tempColor[1], tempColor[2], tempColor[3]), RGBA(tempColorShadow[0], tempColorShadow[1], tempColorShadow[2], tempColorShadow[3]), INTRAFONT_ALIGN_LEFT);
    skinGetPosition("POS_PLAYLIST_TITLE_LABEL", tempPos);
    oslDrawString(tempPos[0], tempPos[1], langGetString("TITLE"));
    skinGetPosition("POS_PLAYLIST_ARTIST_LABEL", tempPos);
    oslDrawString(tempPos[0], tempPos[1], langGetString("ARTIST"));
    skinGetPosition("POS_PLAYLIST_ALBUM_LABEL", tempPos);
    oslDrawString(tempPos[0], tempPos[1], langGetString("ALBUM"));

    skinGetColor("RGBA_TEXT", tempColor);
    skinGetColor("RGBA_TEXT_SHADOW", tempColorShadow);
    setFontStyle(fontNormal, defaultTextSize, RGBA(tempColor[0], tempColor[1], tempColor[2], tempColor[3]), RGBA(tempColorShadow[0], tempColorShadow[1], tempColorShadow[2], tempColorShadow[3]), INTRAFONT_ALIGN_LEFT);
    skinGetPosition("POS_PLAYLIST_TITLE_VALUE", tempPos);
    oslDrawString(tempPos[0], tempPos[1], info->title);
    skinGetPosition("POS_PLAYLIST_ARTIST_VALUE", tempPos);
    oslDrawString(tempPos[0], tempPos[1], info->artist);
    skinGetPosition("POS_PLAYLIST_ALBUM_VALUE", tempPos);
    oslDrawString(tempPos[0], tempPos[1], info->album);

    return 0;
}


///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Ask playlist name:
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void askPlaylistName(char *message, char *initialValue, char *target){
	int skip = 0;
	int done = 0;

	cpuBoost();
	oslInitOsk(message, initialValue, 128, 1, getOSKlang());
    while(!osl_quit && !done){
		if (!skip){
			oslStartDrawing();
			drawCommonGraphics();
			drawButtonBar(MODE_PLAYLIST_EDITOR);
			drawMenu(&commonMenu);
			drawTrackInfo(&tagInfo);

			if (oslOskIsActive())
				oslDrawOsk();
			if (oslGetOskStatus() == PSP_UTILITY_DIALOG_NONE){
				if (oslOskGetResult() == OSL_OSK_CANCEL){
					strcpy(target, "");
					done = 1;
				}else{
					oslOskGetText(target);
					done = 1;
				}
				oslEndOsk();
			}
			oslEndDrawing();
		}
        oslEndFrame();
        skip = oslSyncFrame();
	}
	cpuRestore();
}


///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Playlist Editor:
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
int gui_playlistEditor(){
    char buffer[264] = "";
    int oldSelected = 0;
    int oldFirst = 0;
    struct M3U_songEntry *song;
    int confirmStatus = STATUS_CONFIRM_NONE;

    //Load images:
    snprintf(buffer, sizeof(buffer), "%s/trackinfobkg.png", userSettings->skinImagesPath);
    trackInfoBkg = oslLoadImageFilePNG(buffer, OSL_IN_RAM | OSL_SWIZZLED, OSL_PF_8888);
    if (!trackInfoBkg)
        errorLoadImage(buffer);

	M3U_clear();
    M3U_open(tempM3Ufile);
    if (M3U_getSongCount()){
        song = M3U_getSong(0);
        if (!setAudioFunctions(song->fileName, userSettings->MP3_ME))
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
    snprintf(buffer, sizeof(buffer), "%s/menuplaylistbkg.png", userSettings->skinImagesPath);
    commonMenu.background = oslLoadImageFilePNG(buffer, OSL_IN_RAM | OSL_SWIZZLED, OSL_PF_8888);
    if (!commonMenu.background)
        errorLoadImage(buffer);
    commonMenu.highlight = commonMenuHighlight;
    commonMenu.width = commonMenu.background->sizeX;
    commonMenu.height = commonMenu.background->sizeY;
    commonMenu.interline = skinGetParam("MENU_INTERLINE");
    commonMenu.maxNumberVisible = commonMenu.background->sizeY / (fontMenuNormal->charHeight + commonMenu.interline);
    commonMenu.cancelFunction = NULL;
    buildMenuFromPlaylist(&commonMenu);

    exitFlagPlaylistEditor = 0;
	int skip = 0;
	cpuRestore();

    while(!osl_quit && !exitFlagPlaylistEditor){
		if (!skip){
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
				case STATUS_HELP:
					drawHelp("PLAYLIST_EDITOR");
					break;
			}
			oslEndDrawing();
		}
        oslEndFrame();
    	skip = oslSyncFrame();

        oslReadKeys();
        if (confirmStatus == STATUS_CONFIRM_CLEAR){
            if(osl_pad.released.cross){
                M3U_clear();
                strcpy(userSettings->currentPlaylistName, "");
                memset(&tagInfo, 0, sizeof(tagInfo));
                buildMenuFromPlaylist(&commonMenu);
                confirmStatus = STATUS_CONFIRM_NONE;
            }else if(osl_pad.released.circle){
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
                if (oldFirst < M3U_getSongCount())
                    commonMenu.first = oldFirst;
                else
                    commonMenu.first = oldFirst - 1;
                confirmStatus = STATUS_CONFIRM_NONE;
            }else if(osl_pad.released.circle){
                confirmStatus = STATUS_CONFIRM_NONE;
            }
        }else if (confirmStatus == STATUS_HELP){
            if (osl_pad.released.cross || osl_pad.released.circle)
                confirmStatus = STATUS_CONFIRM_NONE;
        }else{
            oldSelected = commonMenu.selected;
            processMenuKeys(&commonMenu);
            if (commonMenu.selected != oldSelected){
                song = M3U_getSong(commonMenu.selected);
                setAudioFunctions(song->fileName, userSettings->MP3_ME);
                tagInfo = (*getTagInfoFunct)(song->fileName);
                unsetAudioFunctions();
            }

            if (osl_pad.held.L && osl_pad.held.R){
                confirmStatus = STATUS_HELP;
            }else if(osl_pad.pressed.note && M3U_getSongCount()){
                snprintf(userSettings->selectedBrowserItem, sizeof(userSettings->selectedBrowserItem), "%s", tempM3Ufile);
                snprintf(userSettings->selectedBrowserItemShort, sizeof(userSettings->selectedBrowserItemShort), "%s", tempM3Ufile);
                userSettings->playlistStartIndex = commonMenu.selected;
                playlistEditorRetValue = MODE_PLAYER;
                userSettings->previousMode = MODE_PLAYLIST_EDITOR;
                exitFlagPlaylistEditor = 1;
            }else if(osl_pad.pressed.triangle && M3U_getSongCount()){
                drawWait(langGetString("WAIT"), langGetString("CHECKING_PLAYLIST"));
                M3U_checkFiles();
                oldSelected = commonMenu.selected;
                oldFirst = commonMenu.first;
                buildMenuFromPlaylist(&commonMenu);
                if (oldSelected < M3U_getSongCount())
                    commonMenu.selected = oldSelected;
                else
                    commonMenu.selected = oldSelected - 1;
                if (oldFirst < M3U_getSongCount())
                    commonMenu.first = oldFirst;
                else
                    commonMenu.first = oldFirst - 1;
                continue;
            }else if(osl_pad.pressed.square && M3U_getSongCount()){
                if (M3U_moveSongUp(commonMenu.selected) == 0){
                    oldSelected = commonMenu.selected;
                    oldFirst = commonMenu.first;
                    buildMenuFromPlaylist(&commonMenu);
                    commonMenu.selected = oldSelected - 1;
                    commonMenu.first = oldFirst;
                    if (commonMenu.selected == oldFirst - 1)
                        commonMenu.first--;
                }
            }else if(osl_pad.pressed.cross && M3U_getSongCount()){
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
                getFileName(userSettings->currentPlaylistName, onlyName);
                char newName[129] = "";
				askPlaylistName(langGetString("ASK_PLAYLIST_NAME"), onlyName, newName);
				if (strlen(newName)){
					char ext[4] = "";
					getExtension(newName, ext, 3);
					if (strlen(newName)){
						if (strcmp(ext, "M3U"))
							snprintf(buffer, sizeof(buffer), "%s%s/%s.m3u", userSettings->ebootPath, "playLists", newName);
						else
						snprintf(buffer, sizeof(buffer), "%s%s/%s", userSettings->ebootPath, "playLists", newName);
						strcpy(userSettings->currentPlaylistName, buffer);
						M3U_save(userSettings->currentPlaylistName);
					}
				}
                continue;
            }else if(osl_pad.released.select && M3U_getSongCount()){
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

    M3U_save(tempM3Ufile);
    //unLoad images:
    clearMenu(&commonMenu);
    oslDeleteImage(trackInfoBkg);
    trackInfoBkg = NULL;

    return playlistEditorRetValue;
}
