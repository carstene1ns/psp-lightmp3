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
#include <psprtc.h>
#include <oslib/oslib.h>

#include "../main.h"
#include "gui_mediaLibrary.h"
#include "common.h"
#include "languages.h"
#include "settings.h"
#include "skinsettings.h"
#include "menu.h"
#include "../others/strreplace.h"
#include "../others/medialibrary.h"
#include "../system/clock.h"
#include "../players/player.h"
#include "../players/m3u.h"

#define STATUS_MAINMENU 0
#define STATUS_QUERYMENU 1

#define STATUS_CONFIRM_NONE 0
#define STATUS_CONFIRM_SCAN 1
#define STATUS_HELP 3

#define QUERY_SINGLE_ENTRY 0
#define QUERY_COUNT 1
#define QUERY_COUNT_RATING 2

int buildMainMenu();
int buildQueryMenu(char *select, int (*cancelFunction)());
void drawMLinfo();

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Globals:
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
static int mediaLibraryRetValue = 0;
static int exitFlagMediaLibrary = 0;
static struct menuElement tMenuEl;
static char buffer[264];
static int mediaLibraryStatus = STATUS_MAINMENU;
static int confirmStatus = STATUS_CONFIRM_NONE;

static int mlQueryType = QUERY_SINGLE_ENTRY;
static int mlPrevQueryType = 0;
static int mlQueryCount = -1;
static int mlBufferPosition = 0;
static OSL_IMAGE *scanBkg;
static OSL_IMAGE *infoBkg;

static char currentSql[ML_SQLMAXLENGTH] = "";
static char previousSql[ML_SQLMAXLENGTH] = "";
static char currentWhere[ML_SQLMAXLENGTH] = "";
static char currentOrderBy[ML_SQLMAXLENGTH] = "";
static char previousWhere[ML_SQLMAXLENGTH] = "";
static char previousOrderBy[ML_SQLMAXLENGTH] = "";
static char tempSql[ML_SQLMAXLENGTH] = "";

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Add selection to playlist:
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
int addSelectionToPlaylist(char *where, char *orderBy, int fastMode, char *m3uName){
    int offset = 0;
    struct libraryEntry localResult[ML_BUFFERSIZE];
    struct fileInfo info;
    char onlyName[264] = "";
    char message[10] = "";

	oslEndDrawing();
	oslEndFrame();
	oslSyncFrame();

	setCpuClock(222);
	setBusClock(111);

    int i = 0;
	int count = ML_countRecords(where);
    if (count){
        M3U_clear();
        M3U_open(m3uName);
        ML_queryDB(where, orderBy, offset, ML_BUFFERSIZE, localResult);
        for (i=0; i<count; i++){
            if (i >= offset + ML_BUFFERSIZE){
                offset += ML_BUFFERSIZE;
                ML_queryDB(where, orderBy, offset, ML_BUFFERSIZE, localResult);
            }

            oslStartDrawing();
            drawCommonGraphics();
            drawButtonBar(MODE_MEDIA_LIBRARY);
            drawMenu(&commonMenu);
            drawMLinfo();

            snprintf(message, sizeof(message), "%3.3i%% %s", (int)((i+1.0)/count*100.0), langGetString("DONE"));
            if (!fastMode){
                drawWait(langGetString("ADDING_PLAYLIST"), message);

        	    if (localResult[i - offset].seconds > 0){
                    M3U_addSong(localResult[i - offset].path, localResult[i - offset].seconds, localResult[i - offset].title);
                }else{
                    setAudioFunctions(localResult[i - offset].path, userSettings->MP3_ME);
            	    (*initFunct)(0);
                	if ((*loadFunct)(localResult[i - offset].path) == OPENING_OK){
                		info = (*getInfoFunct)();
                		if (strlen(info.title)){
                			M3U_addSong(localResult[i - offset].path, info.length, info.title);
                		}else{
                			getFileName(localResult[i - offset].path, onlyName);
                			M3U_addSong(localResult[i - offset].path, info.length, onlyName);
                		}
                    	localResult[i - offset].seconds = info.length;
                        ML_updateEntry(localResult[i - offset]);
                	}
                	(*endFunct)();
                    unsetAudioFunctions();
                }
            }else{
                drawWait(langGetString("WORKING"), message);

                getFileName(localResult[i - offset].path, onlyName);
                M3U_addSong(localResult[i - offset].path, 0, onlyName);
            }
        	oslEndDrawing();
            oslEndFrame();
        	oslSyncFrame();
        }
        M3U_save(m3uName);
    }
	setBusClock(userSettings->BUS);
	setCpuClock(userSettings->CLOCK_GUI);
	oslStartDrawing();
	return 0;
}


///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Back to main menu:
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
int backToMainMenu(){
    clearMenu(&commonMenu);
    buildMainMenu();
    return 0;
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Scan Memory stick for media:
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
int checkFileCallback(char *fileName){
    oslStartDrawing();
    drawCommonGraphics();
    drawButtonBar(MODE_MEDIA_LIBRARY);

    int startX = (480 - scanBkg->sizeX) / 2;
    int startY = (272 - scanBkg->sizeY) / 2;
    oslDrawImageXY(scanBkg, startX, startY);
    skinGetColor("RGBA_POPUP_TITLE_TEXT", tempColor);
    skinGetColor("RGBA_POPUP_TITLE_TEXT_SHADOW", tempColorShadow);
    oslIntraFontSetStyle(fontNormal, 0.5f, RGBA(tempColor[0], tempColor[1], tempColor[2], tempColor[3]), RGBA(tempColorShadow[0], tempColorShadow[1], tempColorShadow[2], tempColorShadow[3]), INTRAFONT_ALIGN_LEFT);
    //oslSetTextColor(RGBA(tempColor[0], tempColor[1], tempColor[2], tempColor[3]));
    oslDrawString((480 - oslGetStringWidth(langGetString("CHECKING_FILE"))) / 2, startY + 3, langGetString("CHECKING_FILE"));
    skinGetColor("RGBA_POPUP_MESSAGE_TEXT", tempColor);
    skinGetColor("RGBA_POPUP_MESSAGE_TEXT_SHADOW", tempColorShadow);
    oslIntraFontSetStyle(fontNormal, 0.5f, RGBA(tempColor[0], tempColor[1], tempColor[2], tempColor[3]), RGBA(tempColorShadow[0], tempColorShadow[1], tempColorShadow[2], tempColorShadow[3]), INTRAFONT_ALIGN_LEFT);
    //oslSetTextColor(RGBA(tempColor[0], tempColor[1], tempColor[2], tempColor[3]));
    getFileName(fileName, buffer);
    if (strlen(buffer) > 70)
       buffer[70] = '\0';
    oslDrawString((480 - oslGetStringWidth(buffer)) / 2, startY + 30, buffer);

    oslReadKeys();
    oslEndDrawing();
    oslEndFrame();
    oslSyncFrame();
    return 0;
}

int scanDirCallback(char *dirName){
    oslStartDrawing();
    drawCommonGraphics();
    drawButtonBar(MODE_MEDIA_LIBRARY);

    int startX = (480 - scanBkg->sizeX) / 2;
    int startY = (272 - scanBkg->sizeY) / 2;
    oslDrawImageXY(scanBkg, startX, startY);
    skinGetColor("RGBA_POPUP_TITLE_TEXT", tempColor);
    skinGetColor("RGBA_POPUP_TITLE_TEXT_SHADOW", tempColorShadow);
    oslIntraFontSetStyle(fontNormal, 0.5f, RGBA(tempColor[0], tempColor[1], tempColor[2], tempColor[3]), RGBA(tempColorShadow[0], tempColorShadow[1], tempColorShadow[2], tempColorShadow[3]), INTRAFONT_ALIGN_LEFT);
    //oslSetTextColor(RGBA(tempColor[0], tempColor[1], tempColor[2], tempColor[3]));
    oslDrawString((480 - oslGetStringWidth(langGetString("SCANNING"))) / 2, startY + 3, langGetString("SCANNING"));
    skinGetColor("RGBA_POPUP_MESSAGE_TEXT", tempColor);
    skinGetColor("RGBA_POPUP_MESSAGE_TEXT_SHADOW", tempColorShadow);
    oslIntraFontSetStyle(fontNormal, 0.5f, RGBA(tempColor[0], tempColor[1], tempColor[2], tempColor[3]), RGBA(tempColorShadow[0], tempColorShadow[1], tempColorShadow[2], tempColorShadow[3]), INTRAFONT_ALIGN_LEFT);
    //oslSetTextColor(RGBA(tempColor[0], tempColor[1], tempColor[2], tempColor[3]));
    getFileName(dirName, buffer);
    if (strlen(buffer) > 70)
       buffer[70] = '\0';
    oslDrawString((480 - oslGetStringWidth(buffer)) / 2, startY + 30, buffer);

    oslReadKeys();
    oslEndDrawing();
    oslEndFrame();
    oslSyncFrame();
    return 0;
}

int scanMS(){
    char strFound[10] = "";
    char *scannedMsg;

    setCpuClock(222);
    setBusClock(111);
    ML_checkFiles(checkFileCallback);
    int found = ML_scanMS(fileExt, fileExtCount-1, scanDirCallback, NULL);
    setBusClock(userSettings->BUS);
    setCpuClock(userSettings->CLOCK_GUI);

    sprintf(strFound, "%i", found);

    while(!osl_quit && !exitFlagMediaLibrary){
        oslStartDrawing();
        drawCommonGraphics();
        drawButtonBar(MODE_MEDIA_LIBRARY);

        strcpy(buffer, langGetString("MEDIA_FOUND"));
        scannedMsg = replace(buffer, "XX", strFound);
        drawMessageBox(langGetString("SCAN_FINISHED"), scannedMsg);

        oslReadKeys();
        if(osl_pad.released.cross){
            oslEndDrawing();
            oslEndFrame();
            oslSyncFrame();
            break;
        }

        oslEndDrawing();
        oslEndFrame();
        oslSyncFrame();
    }
    return 0;
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Draw the whole video with query running message:
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void drawQueryRunning(){
    oslStartDrawing();
    drawCommonGraphics();
    drawButtonBar(MODE_MEDIA_LIBRARY);
    drawMenu(&commonMenu);
    drawMLinfo();
    drawWait(langGetString("QUERY_RUNNING_TITLE"), langGetString("QUERY_RUNNING"));
    oslEndDrawing();
    oslEndFrame();
    oslSyncFrame();
}


///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Draw info on selected item:
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void drawMLinfo(){
	static OSL_IMAGE *coverArt = NULL;
	static u64 lastMenuChange = 0;
	static int lastSelected = -1;

	OSL_FONT *font = fontNormal;

    if (mediaLibraryStatus != STATUS_QUERYMENU)
        return;

    skinGetPosition("POS_MEDIALIBRARY_INFO_BKG", tempPos);
    oslDrawImageXY(infoBkg, tempPos[0], tempPos[1]);
    oslSetFont(font);
    skinGetColor("RGBA_LABEL_TEXT", tempColor);
    skinGetColor("RGBA_LABEL_TEXT_SHADOW", tempColorShadow);
    oslIntraFontSetStyle(fontNormal, 0.5f, RGBA(tempColor[0], tempColor[1], tempColor[2], tempColor[3]), RGBA(tempColorShadow[0], tempColorShadow[1], tempColorShadow[2], tempColorShadow[3]), INTRAFONT_ALIGN_LEFT);
    //oslSetTextColor(RGBA(tempColor[0], tempColor[1], tempColor[2], tempColor[3]));

    if (mlQueryType == QUERY_COUNT || mlQueryType == QUERY_COUNT_RATING){
        skinGetPosition("POS_MEDIALIBRARY_TOTAL_TRACKS_LABEL", tempPos);
        oslDrawString(tempPos[0], tempPos[1], langGetString("TOTAL_TRACKS"));
        skinGetPosition("POS_MEDIALIBRARY_TOTAL_TIME_LABEL", tempPos);
        oslDrawString(tempPos[0], tempPos[1], langGetString("TOTAL_TIME"));
        skinGetColor("RGBA_TEXT", tempColor);
        skinGetColor("RGBA_TEXT_SHADOW", tempColorShadow);
        oslIntraFontSetStyle(fontNormal, 0.5f, RGBA(tempColor[0], tempColor[1], tempColor[2], tempColor[3]), RGBA(tempColorShadow[0], tempColorShadow[1], tempColorShadow[2], tempColorShadow[3]), INTRAFONT_ALIGN_LEFT);
        //oslSetTextColor(RGBA(tempColor[0], tempColor[1], tempColor[2], tempColor[3]));
        sprintf(buffer, "%.f", MLresult[commonMenu.selected - mlBufferPosition].intField01);
        skinGetPosition("POS_MEDIALIBRARY_TOTAL_TRACKS_VALUE", tempPos);
        oslDrawString(tempPos[0], tempPos[1], buffer);

        formatHHMMSS(MLresult[commonMenu.selected - mlBufferPosition].intField02, buffer);
        skinGetPosition("POS_MEDIALIBRARY_TOTAL_TIME_VALUE", tempPos);
        oslDrawString(tempPos[0], tempPos[1], buffer);
    }else if (mlQueryType == QUERY_SINGLE_ENTRY){
        skinGetPosition("POS_MEDIALIBRARY_GENRE_LABEL", tempPos);
        oslDrawString(tempPos[0], tempPos[1], langGetString("GENRE"));
        skinGetPosition("POS_MEDIALIBRARY_YEAR_LABEL", tempPos);
        oslDrawString(tempPos[0], tempPos[1], langGetString("YEAR"));
        skinGetPosition("POS_MEDIALIBRARY_TIME_LABEL", tempPos);
        oslDrawString(tempPos[0], tempPos[1], langGetString("TIME"));
        skinGetPosition("POS_MEDIALIBRARY_RATING_LABEL", tempPos);
        oslDrawString(tempPos[0], tempPos[1], langGetString("RATING"));
        skinGetPosition("POS_MEDIALIBRARY_PLAYED_LABEL", tempPos);
        oslDrawString(tempPos[0], tempPos[1], langGetString("PLAYED"));

        skinGetColor("RGBA_TEXT", tempColor);
        skinGetColor("RGBA_TEXT_SHADOW", tempColorShadow);
        oslIntraFontSetStyle(fontNormal, 0.5f, RGBA(tempColor[0], tempColor[1], tempColor[2], tempColor[3]), RGBA(tempColorShadow[0], tempColorShadow[1], tempColorShadow[2], tempColorShadow[3]), INTRAFONT_ALIGN_LEFT);
        //oslSetTextColor(RGBA(tempColor[0], tempColor[1], tempColor[2], tempColor[3]));
        skinGetPosition("POS_MEDIALIBRARY_GENRE_VALUE", tempPos);
        oslDrawString(tempPos[0], tempPos[1], MLresult[commonMenu.selected - mlBufferPosition].genre);
        skinGetPosition("POS_MEDIALIBRARY_YEAR_VALUE", tempPos);
        oslDrawString(tempPos[0], tempPos[1], MLresult[commonMenu.selected - mlBufferPosition].year);

        formatHHMMSS(MLresult[commonMenu.selected - mlBufferPosition].seconds, buffer);
        skinGetPosition("POS_MEDIALIBRARY_TIME_VALUE", tempPos);
        oslDrawString(tempPos[0], tempPos[1], buffer);

        skinGetPosition("POS_MEDIALIBRARY_RATING_VALUE", tempPos);
        drawRating(tempPos[0], tempPos[1], MLresult[commonMenu.selected - mlBufferPosition].rating);

		skinGetPosition("POS_MEDIALIBRARY_PLAYED_VALUE", tempPos);
		sprintf(buffer, "%i", MLresult[commonMenu.selected - mlBufferPosition].played);
		oslDrawString(tempPos[0], tempPos[1], buffer);

		if (commonMenu.selected != lastSelected){
			sceRtcGetCurrentTick(&lastMenuChange);
			lastSelected = commonMenu.selected;
			if (coverArt){
				oslDeleteImage(coverArt);
				coverArt = NULL;
			}
		}else if (!coverArt){
			u64 currentTime;
			sceRtcGetCurrentTick(&currentTime);
			if (currentTime - lastMenuChange > 500000){
				char dirName[264];
				int size = 0;

				sprintf(dirName, "%s", MLresult[commonMenu.selected - mlBufferPosition].path);
				directoryUp(dirName);
				//Look for folder.jpg in the same directory:
				sprintf(buffer, "%s/%s.jpg", dirName, "folder.jpg");
				size = fileExists(buffer);
				if (size > 0 && size <= MAX_IMAGE_DIMENSION)
					coverArt = oslLoadImageFileJPG(buffer, OSL_IN_RAM | OSL_SWIZZLED, OSL_PF_8888);
				else{
					//Look for cover.jpg in same directory:
					sprintf(buffer, "%s/%s", dirName, "cover.jpg");
					size = fileExists(buffer);
					if (size > 0 && size <= MAX_IMAGE_DIMENSION)
						coverArt = oslLoadImageFileJPG(buffer, OSL_IN_RAM | OSL_SWIZZLED, OSL_PF_8888);
				}
				if (coverArt){
					coverArt->stretchX = skinGetParam("MEDIALIBRARY_COVERART_WIDTH");
					coverArt->stretchY = skinGetParam("MEDIALIBRARY_COVERART_HEIGHT");
				}
			}
		}

		if (coverArt){
			skinGetPosition("POS_MEDIALIBRARY_COVERART", tempPos);
			oslDrawImageXY(coverArt, tempPos[0], tempPos[1]);
		}
	}
}


///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Exit from current selection:
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
int exitSelection(){
    strcpy(tempSql, previousSql);
    buildQueryMenu(tempSql, backToMainMenu);
    strcpy(currentWhere, previousWhere);
    strcpy(currentOrderBy, previousOrderBy);
	mlQueryType = mlPrevQueryType;
    return 0;
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Enter in current selection:
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
int enterSelection(){
    sprintf(tempSql, "Select media.*, \
                             title || ' - ' || artist || ' (' || album || ')' as strfield,  \
                             'path = ''' || replace(path, '''', '''''') || '''' as datafield \
                      From media \
                      Where %s \
                      Order by title ", commonMenu.elements[commonMenu.selected].data);
    strcpy(previousSql, currentSql);
    strcpy(previousWhere, currentWhere);
    strcpy(previousOrderBy, currentOrderBy);
	strcpy(currentWhere, commonMenu.elements[commonMenu.selected].data);
    strcpy(currentOrderBy, "title");
	buildQueryMenu(tempSql, exitSelection);
    mlPrevQueryType = mlQueryType;
    mlQueryType = QUERY_SINGLE_ENTRY;
    oslReadKeys(); //To avoid reread the CROSS button after entering selection
    return 0;
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Query Menu:
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void queryDataFeed(int index, struct menuElement *element){
    if ((index >= mlBufferPosition + ML_BUFFERSIZE && index < commonMenu.numberOfElements) || index < mlBufferPosition){
        if (index >= mlBufferPosition + ML_BUFFERSIZE)
            mlBufferPosition = commonMenu.first;
        else{
            mlBufferPosition = commonMenu.first + commonMenu.maxNumberVisible - ML_BUFFERSIZE;
            if (mlBufferPosition < 0)
                mlBufferPosition = 0;
        }
		setCpuClock(222);
        setBusClock(111);
        ML_queryDBSelect(currentSql, mlBufferPosition, ML_BUFFERSIZE, MLresult);
        setBusClock(userSettings->BUS);
        setCpuClock(userSettings->CLOCK_GUI);
    }

    if (mlQueryType == QUERY_COUNT){
        sprintf(buffer, "%s (%.f)", MLresult[index - mlBufferPosition].strField, MLresult[index - mlBufferPosition].intField01);
        strcpy(element->text, buffer);
        strcpy(element->data, MLresult[index - mlBufferPosition].dataField);
        element->triggerFunction = enterSelection;
    }else if (mlQueryType == QUERY_SINGLE_ENTRY){
        strcpy(element->text, MLresult[index - mlBufferPosition].strField);
        //sprintf(element->text, "%i.%s", index, MLresult[index - mlBufferPosition].strField);
		strcpy(element->data, MLresult[index - mlBufferPosition].dataField);
        element->triggerFunction = NULL;
    }else if (mlQueryType == QUERY_COUNT_RATING){
	    int startY = commonMenu.yPos + (float)(commonMenu.height -  commonMenu.maxNumberVisible * (fontMenuNormal->charHeight + commonMenu.interline)) / 2.0;
        drawRating(commonMenu.xPos + 4, startY + (fontMenuNormal->charHeight * index + commonMenu.interline * index), atoi(MLresult[index - mlBufferPosition].strField));
        sprintf(buffer, "(%.f)", MLresult[index - mlBufferPosition].intField01);
        oslSetFont(fontMenuNormal);
        if (index == commonMenu.selected){
            skinGetColor("RGBA_MENU_SELECTED_TEXT", tempColor);
            skinGetColor("RGBA_MENU_SELECTED_TEXT_SHADOW", tempColorShadow);
            oslIntraFontSetStyle(fontMenuNormal, 0.5f, RGBA(tempColor[0], tempColor[1], tempColor[2], tempColor[3]), RGBA(tempColorShadow[0], tempColorShadow[1], tempColorShadow[2], tempColorShadow[3]), INTRAFONT_ALIGN_LEFT);
            //oslSetTextColor(RGBA(tempColor[0], tempColor[1], tempColor[2], tempColor[3]));
        }else{
            skinGetColor("RGBA_MENU_TEXT", tempColor);
            skinGetColor("RGBA_MENU_TEXT_SHADOW", tempColorShadow);
            oslIntraFontSetStyle(fontMenuNormal, 0.5f, RGBA(tempColor[0], tempColor[1], tempColor[2], tempColor[3]), RGBA(tempColorShadow[0], tempColorShadow[1], tempColorShadow[2], tempColorShadow[3]), INTRAFONT_ALIGN_LEFT);
            //oslSetTextColor(RGBA(tempColor[0], tempColor[1], tempColor[2], tempColor[3]));
        }
        oslDrawString(commonMenu.xPos + star->sizeX*ML_MAX_RATING + 8, startY + (fontMenuNormal->charHeight * index + commonMenu.interline * index), buffer);
		strcpy(element->text, "");
		strcpy(element->data, MLresult[index - mlBufferPosition].dataField);
        element->triggerFunction = enterSelection;
    }
}

int buildQueryMenu(char *select, int (*cancelFunction)()){
    drawQueryRunning();
    mediaLibraryStatus = STATUS_QUERYMENU;

    setCpuClock(222);
    setBusClock(111);

    mlQueryCount = ML_countRecordsSelect(select);
	if (mlQueryCount > MENU_MAX_ELEMENTS)
		mlQueryCount = MENU_MAX_ELEMENTS;
    ML_queryDBSelect(select, 0, ML_BUFFERSIZE, MLresult);
    setBusClock(userSettings->BUS);
    setCpuClock(userSettings->CLOCK_GUI);
    strcpy(currentSql, select);

    clearMenu(&commonMenu);
    commonMenu.numberOfElements = mlQueryCount;
    commonMenu.first = 0;
    commonMenu.selected = 0;
    skinGetPosition("POS_MEDIALIBRARY_QUERY", tempPos);
    commonMenu.xPos = tempPos[0];
    commonMenu.yPos = tempPos[1];
    commonMenu.fastScrolling = 1;
    sprintf(buffer, "%s/medialibraryquerybkg.png", userSettings->skinImagesPath);
    commonMenu.background = oslLoadImageFilePNG(buffer, OSL_IN_RAM | OSL_SWIZZLED, OSL_PF_8888);
    if (!commonMenu.background)
        errorLoadImage(buffer);
    commonMenu.highlight = commonMenuHighlight;
    commonMenu.width = commonMenu.background->sizeX;
    commonMenu.height = commonMenu.background->sizeY;
    commonMenu.dataFeedFunction = queryDataFeed;
    commonMenu.align = ALIGN_LEFT;
    commonMenu.interline = 0;
    commonMenu.maxNumberVisible = commonMenu.background->sizeY / (fontNormal->charHeight + commonMenu.interline);
    commonMenu.cancelFunction = (cancelFunction);

    return 0;
}


///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Browse all:
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
int browseAll(){
    mlQueryType = QUERY_SINGLE_ENTRY;
    mlQueryCount = -1;
    mlBufferPosition = 0;
    sprintf(tempSql, "Select media.*, \
                             title || ' - ' || artist || ' (' || album || ')' as strfield,  \
                             'path = ''' || replace(path, '''', '''''') || '''' as datafield \
                      From media \
                      Order by title, artist");

	strcpy(currentWhere, "1=1");
	strcpy(currentOrderBy, "title, artist");
	buildQueryMenu(tempSql, backToMainMenu);
    oslReadKeys(); //To avoid reread the CROSS button after entering selection
	return 0;
}


///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Browse by artist:
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
int browseByArtist(){
    mlQueryType = QUERY_COUNT;
    mlQueryCount = -1;
    mlBufferPosition = 0;
    buildQueryMenu("Select artist as strfield, 'artist = ''' || replace(artist, '''', '''''') || '''' as datafield, count(*) as intfield01, sum(seconds) as intfield02 from media group by artist order by upper(artist)", backToMainMenu);
    return 0;
}


///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Browse by ALBUM:
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
int browseByAlbum(){
    mlQueryType = QUERY_COUNT;
    mlQueryCount = -1;
    mlBufferPosition = 0;
    buildQueryMenu("Select album || ' - ' || artist as strfield, 'artist = ''' || replace(artist, '''', '''''') || ''' and album = ''' || replace(album, '''', '''''') || '''' as datafield, count(*) as intfield01, sum(seconds) as intfield02 from media group by artist, album order by upper(album), upper(artist)", backToMainMenu);
    return 0;
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Browse by GENRE:
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
int browseByGenre(){
    mlQueryType = QUERY_COUNT;
    mlQueryCount = -1;
    mlBufferPosition = 0;
    buildQueryMenu("Select genre as strfield, 'genre = ''' || replace(genre, '''', '''''') || '''' as datafield, count(*) as intfield01, sum(seconds) as intfield02 from media group by genre order by upper(genre)", backToMainMenu);
    return 0;
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Browse by rating:
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
int browseByRating(){
    mlQueryType = QUERY_COUNT_RATING;
    mlQueryCount = -1;
    mlBufferPosition = 0;
    buildQueryMenu("Select cast(coalesce(rating, 0) as varchar) as strfield, 'rating = ' || cast(coalesce(rating, 0) as varchar) as datafield, count(*) as intfield01, sum(seconds) as intfield02 from media group by coalesce(rating, 0) order by coalesce(rating, 0) desc", backToMainMenu);
    return 0;
}


///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Browse TOP 100:
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
int browseTop100(){
    mlQueryType = QUERY_SINGLE_ENTRY;
    mlQueryCount = -1;
    mlBufferPosition = 0;
    sprintf(tempSql, "Select media.*, \
                             title || ' - ' || artist || ' (' || album || ')' as strfield,  \
                             'path = ''' || replace(path, '''', '''''') || '''' as datafield \
                      From media \
                      Order by played desc, rating desc, title LIMIT 100");

	sprintf(currentWhere, "path in (Select path \
						   From media \
						   Order by played desc, rating desc, title LIMIT 100)");
	strcpy(currentOrderBy, "played desc, rating desc, title");
	buildQueryMenu(tempSql, backToMainMenu);
    oslReadKeys(); //To avoid reread the CROSS button after entering selection
    return 0;
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Ask for Search string:
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void askSearchString(char *message, char *initialValue, char *target){
	int skip = 0;
	int done = 0;
    int oldClock = getCpuClock();
    int oldBus = getBusClock();
    setCpuClock(222);
    setBusClock(111);

	oslInitOsk(message, initialValue, 128, 1);
    while(!osl_quit && !done){
		if (!skip){
			oslStartDrawing();
			drawCommonGraphics();
			drawButtonBar(MODE_MEDIA_LIBRARY);
			drawMenu(&commonMenu);

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
	oslReadKeys();
    setBusClock(oldBus);
    setCpuClock(oldClock);
}


///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Search:
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
int search(){
	oslEndDrawing();
    oslEndFrame();
	oslSyncFrame();
    char searchString[129] = "";
	askSearchString(langGetString("ASK_SEARCH_STRING"), "", searchString);
	if (strlen(searchString)){
		oslStartDrawing();
		drawCommonGraphics();
		drawButtonBar(MODE_MEDIA_LIBRARY);
		oslEndDrawing();
		oslSyncFrame();


		if (strlen(searchString)){
			mlQueryType = QUERY_SINGLE_ENTRY;
			mlQueryCount = -1;
			mlBufferPosition = 0;
			sprintf(tempSql, "Select media.*, title || ' - ' || artist as strfield, \
									'path = ''' || replace(path, '''', '''''') || '''' as datafield \
						 	  From media \
							  Where title like '%%%s%%' \
								 or artist like '%%%s%%' \
								 or album like '%%%s%%' \
							  Order by title, artist, album",
							  searchString, searchString, searchString);
			buildQueryMenu(tempSql, backToMainMenu);
		    sprintf(currentWhere, " title like '%%%s%%' \
								    or artist like '%%%s%%' \
									or album like '%%%s%%' ",
								   searchString, searchString, searchString);
			strcpy(currentOrderBy, "title, artist, album");
		}
	}
	oslStartDrawing();
    return 0;
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Main Menu:
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
int buildMainMenu(){
    mediaLibraryStatus = STATUS_MAINMENU;
    skinGetPosition("POS_MEDIALIBRARY", tempPos);
    commonMenu.yPos = tempPos[1];
    commonMenu.xPos = tempPos[0];
    commonMenu.fastScrolling = 0;
    commonMenu.align = ALIGN_CENTER;
    sprintf(buffer, "%s/medialibrarybkg.png", userSettings->skinImagesPath);
    commonMenu.background = oslLoadImageFilePNG(buffer, OSL_IN_RAM | OSL_SWIZZLED, OSL_PF_8888);
    if (!commonMenu.background)
        errorLoadImage(buffer);
    commonMenu.highlight = commonMenuHighlight;
    commonMenu.width = commonMenu.background->sizeX;
    commonMenu.height = commonMenu.background->sizeY;
    commonMenu.interline = 0;
    commonMenu.maxNumberVisible = commonMenu.background->sizeY / (fontNormal->charHeight + commonMenu.interline);
    commonMenu.cancelFunction = NULL;

    commonMenu.first = 0;
    commonMenu.selected = 0;

    strcpy(tMenuEl.text, langGetString("BROWSE_ALL"));
    tMenuEl.triggerFunction = browseAll;
    commonMenu.elements[0] = tMenuEl;
    strcpy(tMenuEl.text, langGetString("BROWSE_ARTIST"));
    tMenuEl.triggerFunction = browseByArtist;
    commonMenu.elements[1] = tMenuEl;
    strcpy(tMenuEl.text, langGetString("BROWSE_ALBUM"));
    tMenuEl.triggerFunction = browseByAlbum;
    commonMenu.elements[2] = tMenuEl;
    strcpy(tMenuEl.text, langGetString("BROWSE_GENRE"));
    tMenuEl.triggerFunction = browseByGenre;
    commonMenu.elements[3] = tMenuEl;
    strcpy(tMenuEl.text, langGetString("BROWSE_RATING"));
    tMenuEl.triggerFunction = browseByRating;
    commonMenu.elements[4] = tMenuEl;
    strcpy(tMenuEl.text, langGetString("BROWSE_TOP100"));
    tMenuEl.triggerFunction = browseTop100;
    commonMenu.elements[5] = tMenuEl;
	strcpy(tMenuEl.text, langGetString("SEARCH"));
    tMenuEl.triggerFunction = search;
    commonMenu.elements[6] = tMenuEl;
    strcpy(tMenuEl.text, langGetString("SCAN_MS"));
    tMenuEl.triggerFunction = NULL;
    commonMenu.elements[7] = tMenuEl;

    commonMenu.numberOfElements = 8;
    return 0;
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Media Library:
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
int gui_mediaLibrary(){
    int ratingChangedUpDown = 0;

    //Load images:
    sprintf(buffer, "%s/medialibraryscanbkg.png", userSettings->skinImagesPath);
    scanBkg = oslLoadImageFilePNG(buffer, OSL_IN_RAM | OSL_SWIZZLED, OSL_PF_8888);
    if (!scanBkg)
        errorLoadImage(buffer);

    sprintf(buffer, "%s/medialibraryinfobkg.png", userSettings->skinImagesPath);
    infoBkg = oslLoadImageFilePNG(buffer, OSL_IN_RAM | OSL_SWIZZLED, OSL_PF_8888);
    if (!infoBkg)
        errorLoadImage(buffer);

    //Build menu:
    buildMainMenu();
    exitFlagMediaLibrary = 0;
    while(!osl_quit && !exitFlagMediaLibrary){
        oslStartDrawing();
        drawCommonGraphics();
        drawButtonBar(MODE_MEDIA_LIBRARY);
        drawMenu(&commonMenu);
        if (mediaLibraryStatus == STATUS_QUERYMENU)
            drawMLinfo();

        switch (confirmStatus){
            case STATUS_CONFIRM_SCAN:
                drawConfirm(langGetString("CONFIRM_SCAN_MS_TITLE"), langGetString("CONFIRM_SCAN_MS"));
                break;
            case STATUS_HELP:
                drawHelp("MEDIALIBRARY");
                break;
        }

        oslReadKeys();
        if (confirmStatus == STATUS_CONFIRM_SCAN){
            if(osl_pad.released.cross){
                confirmStatus = STATUS_CONFIRM_NONE;
                scanMS();
            }else if(osl_pad.pressed.circle)
                confirmStatus = STATUS_CONFIRM_NONE;
        }else if (confirmStatus == STATUS_HELP){
            if (osl_pad.released.cross || osl_pad.released.circle)
                confirmStatus = STATUS_CONFIRM_NONE;
        }else{
            if (ratingChangedUpDown && (osl_pad.released.cross || osl_pad.released.up || osl_pad.released.down))
                ratingChangedUpDown = 0;

            if (osl_pad.held.L && osl_pad.held.R){
                confirmStatus = STATUS_HELP;
            }else if(mediaLibraryStatus == STATUS_MAINMENU && osl_pad.released.cross && commonMenu.selected == 7){
                confirmStatus = STATUS_CONFIRM_SCAN;
            }else if(osl_pad.released.start && mediaLibraryStatus == STATUS_QUERYMENU){
                addSelectionToPlaylist(commonMenu.elements[commonMenu.selected].data, "title", 0, tempM3Ufile);
            }else if(!ratingChangedUpDown && osl_pad.released.cross && mediaLibraryStatus == STATUS_QUERYMENU && mlQueryType == QUERY_SINGLE_ENTRY){
                M3U_clear();
                M3U_save(MLtempM3Ufile);
                addSelectionToPlaylist(currentWhere, currentOrderBy, 1, MLtempM3Ufile);
                sprintf(userSettings->selectedBrowserItem, "%s", MLtempM3Ufile);
                userSettings->playlistStartIndex = commonMenu.selected;
                mediaLibraryRetValue = MODE_PLAYER;
                userSettings->previousMode = MODE_MEDIA_LIBRARY;
                exitFlagMediaLibrary = 1;
            }else if(osl_pad.released.square && mediaLibraryStatus == STATUS_QUERYMENU && (mlQueryType == QUERY_COUNT || mlQueryType == QUERY_COUNT_RATING)){
                M3U_clear();
                M3U_save(MLtempM3Ufile);
                addSelectionToPlaylist(commonMenu.elements[commonMenu.selected].data, "title", 1, MLtempM3Ufile);
                sprintf(userSettings->selectedBrowserItem, "%s", MLtempM3Ufile);
                userSettings->playlistStartIndex = -1;
                mediaLibraryRetValue = MODE_PLAYER;
                userSettings->previousMode = MODE_MEDIA_LIBRARY;
                exitFlagMediaLibrary = 1;
            }else if (osl_pad.held.cross && osl_pad.held.up && mediaLibraryStatus == STATUS_QUERYMENU && mlQueryType == QUERY_SINGLE_ENTRY){
                if (++MLresult[commonMenu.selected - mlBufferPosition].rating > ML_MAX_RATING)
                    MLresult[commonMenu.selected - mlBufferPosition].rating = ML_MAX_RATING;
                ratingChangedUpDown = 1;
                ML_updateEntry(MLresult[commonMenu.selected - mlBufferPosition]);
                sceKernelDelayThread(userSettings->KEY_AUTOREPEAT_PLAYER*15000);
            }else if (osl_pad.held.cross && osl_pad.held.down  && mediaLibraryStatus == STATUS_QUERYMENU && mlQueryType == QUERY_SINGLE_ENTRY){
                if (--MLresult[commonMenu.selected - mlBufferPosition].rating < 0)
                    MLresult[commonMenu.selected - mlBufferPosition].rating = 0;
                ratingChangedUpDown = 1;
                ML_updateEntry(MLresult[commonMenu.selected - mlBufferPosition]);
                sceKernelDelayThread(userSettings->KEY_AUTOREPEAT_PLAYER*15000);
            }else if(osl_pad.released.R){
                mediaLibraryRetValue = nextAppMode(MODE_MEDIA_LIBRARY);
                exitFlagMediaLibrary = 1;
            }else if(osl_pad.released.L){
                mediaLibraryRetValue = previousAppMode(MODE_MEDIA_LIBRARY);
                exitFlagMediaLibrary = 1;
            }

            if (!ratingChangedUpDown)
                processMenuKeys(&commonMenu);
        }
    	oslEndDrawing();
        oslEndFrame();
    	oslSyncFrame();
    }

    clearMenu(&commonMenu);
    oslDeleteImage(scanBkg);
    oslDeleteImage(infoBkg);
    return mediaLibraryRetValue;
}
