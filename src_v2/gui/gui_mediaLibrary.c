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
#include "../system/libminiconv.h"
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
#define QUERY_COUNT_ARTIST 3

int buildMainMenu();
int buildQueryMenu(char *select, char *where, char *orderBy, int (*cancelFunction)(), int selected);
void drawMLinfo();

int changeQuery(int queryType, char *select, char *where, char *orderBy,  int (*cancelFunction)(), int (*triggerFunction)());
int restoreQuery();

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
static int mlQueryCount = -1;
static int mlBufferPosition = 0;
static OSL_IMAGE *scanBkg;
static OSL_IMAGE *infoBkg;

static ML_status MlStatus[4];
static int currentStatus = -1;

static char tempSql[ML_SQLMAXLENGTH] = "";

static OSL_IMAGE *coverArt = NULL;
static int coverArtFailed = 0;
static struct libraryEntry localResult[ML_BUFFERSIZE];

#define MAX_ORDER_BY 9
static int selectedOrderBy = 0;
static char orderBy[MAX_ORDER_BY][100] = {
                                          "artist, album, tracknumber",
                                          "artist, album, title",
                                          "artist, title",
                                          "title",
                                          "year, title",
                                          "year, artist, album, tracknumber",
                                          "year, artist, album, title",
                                          "rating desc, artist, album, tracknumber",
                                          "rating desc, artist, album, title"
                                         };

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Add selection to playlist:
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
int addSelectionToPlaylist(char *where, char *orderBy, int fastMode, char *m3uName){
    int offset = 0;
    struct fileInfo *info = NULL;
    char onlyName[264] = "";
    char message[100] = "";

	cpuBoost();
    int i = 0;
	snprintf(tempSql, sizeof(tempSql), "%s Where %s Order By %s", "Select media.* from media", where, orderBy);
    int count = ML_countRecordsSelect(tempSql);

	if (count > 0){
        int perc = 0;
        int oldPerc = -1;

        M3U_clear();
        M3U_open(m3uName);
	    ML_queryDBSelect(tempSql, 0, ML_BUFFERSIZE, localResult);

        for (i=0; i<count; i++){
            if (i >= offset + ML_BUFFERSIZE){
                offset += ML_BUFFERSIZE;
				ML_queryDBSelect(tempSql, offset, ML_BUFFERSIZE, localResult);
            }

            perc = (int)((i+1.0) / count*100.0);

            if (oldPerc != perc){
                oldPerc = perc;
                oslStartDrawing();
                drawCommonGraphics();
                drawButtonBar(MODE_MEDIA_LIBRARY);
                drawMenu(&commonMenu);
                drawMLinfo();
                snprintf(message, sizeof(message), "%3.3i%% %s", perc, langGetString("DONE"));

                if (!fastMode)
                    drawWait(langGetString("ADDING_PLAYLIST"), message);
                else
                    drawWait(langGetString("WORKING"), message);

                oslEndDrawing();
                oslEndFrame();
                oslSyncFrame();
            }

            if (!fastMode){
        	    if (localResult[i - offset].seconds > 0){
                    M3U_addSong(localResult[i - offset].shortpath, localResult[i - offset].seconds, localResult[i - offset].title);
                }else{
                    if (setAudioFunctions(localResult[i - offset].shortpath, userSettings->MP3_ME))
						continue;
            	    (*initFunct)(0);
                	if ((*loadFunct)(localResult[i - offset].shortpath) == OPENING_OK){
                		info = (*getInfoFunct)();
                		if (strlen(info->title)){
                			M3U_addSong(localResult[i - offset].shortpath, info->length, info->title);
                		}else{
                			getFileName(localResult[i - offset].shortpath, onlyName);
                			M3U_addSong(localResult[i - offset].shortpath, info->length, onlyName);
                		}
                    	localResult[i - offset].seconds = info->length;
                        ML_updateEntry(localResult[i - offset], "");
                	}
                	(*endFunct)();
                    unsetAudioFunctions();
                }
            }else{
                getFileName(localResult[i - offset].shortpath, onlyName);
                M3U_addSong(localResult[i - offset].shortpath, 0, onlyName);
            }
        }
        M3U_save(m3uName);
    }
	cpuRestore();
	return 0;
}


///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Back to main menu:
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
int backToMainMenu(){
    clearMenu(&commonMenu);
    buildMainMenu();
	if (coverArt){
		oslDeleteImage(coverArt);
		coverArt = NULL;
	}
	coverArtFailed = 0;
    currentStatus = -1;
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
    setFontStyle(fontNormal, defaultTextSize, RGBA(tempColor[0], tempColor[1], tempColor[2], tempColor[3]), RGBA(tempColorShadow[0], tempColorShadow[1], tempColorShadow[2], tempColorShadow[3]), INTRAFONT_ALIGN_LEFT);
    oslDrawString((480 - oslGetStringWidth(langGetString("CHECKING_FILE"))) / 2, startY + 3, langGetString("CHECKING_FILE"));
    skinGetColor("RGBA_POPUP_MESSAGE_TEXT", tempColor);
    skinGetColor("RGBA_POPUP_MESSAGE_TEXT_SHADOW", tempColorShadow);
    setFontStyle(fontNormal, defaultTextSize, RGBA(tempColor[0], tempColor[1], tempColor[2], tempColor[3]), RGBA(tempColorShadow[0], tempColorShadow[1], tempColorShadow[2], tempColorShadow[3]), INTRAFONT_ALIGN_LEFT);
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
    setFontStyle(fontNormal, defaultTextSize, RGBA(tempColor[0], tempColor[1], tempColor[2], tempColor[3]), RGBA(tempColorShadow[0], tempColorShadow[1], tempColorShadow[2], tempColorShadow[3]), INTRAFONT_ALIGN_LEFT);
    oslDrawString((480 - oslGetStringWidth(langGetString("SCANNING"))) / 2, startY + 3, langGetString("SCANNING"));
    skinGetColor("RGBA_POPUP_MESSAGE_TEXT", tempColor);
    skinGetColor("RGBA_POPUP_MESSAGE_TEXT_SHADOW", tempColorShadow);
    setFontStyle(fontNormal, defaultTextSize, RGBA(tempColor[0], tempColor[1], tempColor[2], tempColor[3]), RGBA(tempColorShadow[0], tempColorShadow[1], tempColorShadow[2], tempColorShadow[3]), INTRAFONT_ALIGN_LEFT);
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

	cpuBoost();
	ML_checkFiles(checkFileCallback);
    int found = ML_scanMS(userSettings->mediaLibraryRoot, fileExt, fileExtCount-1, scanDirCallback, NULL);
    cpuRestore();

    snprintf(strFound, sizeof(strFound), "%i", found);

    while(!osl_quit && !exitFlagMediaLibrary){
        oslStartDrawing();
        drawCommonGraphics();
        drawButtonBar(MODE_MEDIA_LIBRARY);

        strcpy(buffer, langGetString("MEDIA_FOUND"));
        scannedMsg = replace(buffer, "XX", strFound);
        drawMessageBox(langGetString("SCAN_FINISHED"), scannedMsg);

        oslEndDrawing();
        oslEndFrame();
        oslSyncFrame();

        oslReadKeys();
        if(getConfirmButton())
            break;
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
	static u64 lastMenuChange = 0;
	static int lastSelected = -1;

	OSL_FONT *font = fontNormal;
	OSL_IMAGE *tmpCoverArt = NULL;

    if (mediaLibraryStatus != STATUS_QUERYMENU)
        return;

    skinGetPosition("POS_MEDIALIBRARY_INFO_BKG", tempPos);
    oslDrawImageXY(infoBkg, tempPos[0], tempPos[1]);
    oslSetFont(font);
    skinGetColor("RGBA_LABEL_TEXT", tempColor);
    skinGetColor("RGBA_LABEL_TEXT_SHADOW", tempColorShadow);
    setFontStyle(fontNormal, defaultTextSize, RGBA(tempColor[0], tempColor[1], tempColor[2], tempColor[3]), RGBA(tempColorShadow[0], tempColorShadow[1], tempColorShadow[2], tempColorShadow[3]), INTRAFONT_ALIGN_LEFT);

	switch (mlQueryType)
	{
	case QUERY_COUNT:
	case QUERY_COUNT_RATING:
	case QUERY_COUNT_ARTIST:
        skinGetPosition("POS_MEDIALIBRARY_TOTAL_TRACKS_LABEL", tempPos);
        oslDrawString(tempPos[0], tempPos[1], langGetString("TOTAL_TRACKS"));
        skinGetPosition("POS_MEDIALIBRARY_TOTAL_TIME_LABEL", tempPos);
        oslDrawString(tempPos[0], tempPos[1], langGetString("TOTAL_TIME"));
        skinGetColor("RGBA_TEXT", tempColor);
        skinGetColor("RGBA_TEXT_SHADOW", tempColorShadow);
        setFontStyle(fontNormal, defaultTextSize, RGBA(tempColor[0], tempColor[1], tempColor[2], tempColor[3]), RGBA(tempColorShadow[0], tempColorShadow[1], tempColorShadow[2], tempColorShadow[3]), INTRAFONT_ALIGN_LEFT);
        snprintf(buffer, sizeof(buffer), "%.f", MLresult[commonMenu.selected - mlBufferPosition].intField01);
        skinGetPosition("POS_MEDIALIBRARY_TOTAL_TRACKS_VALUE", tempPos);
        oslDrawString(tempPos[0], tempPos[1], buffer);

        formatHHMMSS(MLresult[commonMenu.selected - mlBufferPosition].intField02, buffer, sizeof(buffer));
        skinGetPosition("POS_MEDIALIBRARY_TOTAL_TIME_VALUE", tempPos);
        oslDrawString(tempPos[0], tempPos[1], buffer);
		break;
	case QUERY_SINGLE_ENTRY:
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
        setFontStyle(fontNormal, defaultTextSize, RGBA(tempColor[0], tempColor[1], tempColor[2], tempColor[3]), RGBA(tempColorShadow[0], tempColorShadow[1], tempColorShadow[2], tempColorShadow[3]), INTRAFONT_ALIGN_LEFT);
        skinGetPosition("POS_MEDIALIBRARY_GENRE_VALUE", tempPos);
        oslDrawString(tempPos[0], tempPos[1], MLresult[commonMenu.selected - mlBufferPosition].genre);
        skinGetPosition("POS_MEDIALIBRARY_YEAR_VALUE", tempPos);
        oslDrawString(tempPos[0], tempPos[1], MLresult[commonMenu.selected - mlBufferPosition].year);

        formatHHMMSS(MLresult[commonMenu.selected - mlBufferPosition].seconds, buffer, sizeof(buffer));
        skinGetPosition("POS_MEDIALIBRARY_TIME_VALUE", tempPos);
        oslDrawString(tempPos[0], tempPos[1], buffer);

        skinGetPosition("POS_MEDIALIBRARY_RATING_VALUE", tempPos);
        drawRating(tempPos[0], tempPos[1], MLresult[commonMenu.selected - mlBufferPosition].rating);

		skinGetPosition("POS_MEDIALIBRARY_PLAYED_VALUE", tempPos);
		snprintf(buffer, sizeof(buffer), "%i", MLresult[commonMenu.selected - mlBufferPosition].played);
		oslDrawString(tempPos[0], tempPos[1], buffer);

		if (commonMenu.selected != lastSelected){
			sceRtcGetCurrentTick (&lastMenuChange);
			lastSelected = commonMenu.selected;
			if (coverArt){
				oslDeleteImage(coverArt);
				coverArt = NULL;
			}
			coverArtFailed = 0;
		}else if (!coverArt && !coverArtFailed){
			u64 currentTime;
			sceRtcGetCurrentTick(&currentTime);
			if (currentTime - lastMenuChange > COVERTART_DELAY){
				char dirName[264];
				int size = 0;

				cpuBoost();
				snprintf(dirName, sizeof(dirName), "%s", MLresult[commonMenu.selected - mlBufferPosition].shortpath);
				directoryUp(dirName);
				//Look for folder.jpg in the same directory:
				snprintf(buffer, sizeof(buffer), "%s/%s", dirName, "folder.jpg");

				size = fileExists(buffer);
				if (size > 0 && size <= MAX_IMAGE_DIMENSION)
                {
					tmpCoverArt = oslLoadImageFileJPG(buffer, OSL_IN_RAM | OSL_SWIZZLED, OSL_PF_8888);
                }
				else
                {
					//Look for cover.jpg in same directory:
					snprintf(buffer, sizeof(buffer), "%s/%s", dirName, "cover.jpg");
					size = fileExists(buffer);
					if (size > 0 && size <= MAX_IMAGE_DIMENSION)
						tmpCoverArt = oslLoadImageFileJPG(buffer, OSL_IN_RAM | OSL_SWIZZLED, OSL_PF_8888);
				}
				if (tmpCoverArt){
    				int coverArtWidth  = skinGetParam("MEDIALIBRARY_COVERART_WIDTH");
    				int coverArtHeight = skinGetParam("MEDIALIBRARY_COVERART_HEIGHT");

					coverArt = oslScaleImageCreate(tmpCoverArt, OSL_IN_RAM | OSL_SWIZZLED, coverArtWidth, coverArtHeight, OSL_PF_8888);
					oslDeleteImage(tmpCoverArt);
					tmpCoverArt = NULL;

        			coverArt->stretchX = coverArtWidth;
        			coverArt->stretchY = coverArtHeight;
				}else{
					coverArtFailed = 1;
				}
				cpuRestore();
			}
		}

		if (coverArt){
			skinGetPosition("POS_MEDIALIBRARY_COVERART", tempPos);
			oslDrawImageXY(coverArt, tempPos[0], tempPos[1]);
		}
		break;
	}
}


///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Exit from current selection:
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
int exitSelection(){
    restoreQuery();
	if (coverArt){
		oslDeleteImage(coverArt);
		coverArt = NULL;
	}
    return 0;
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Enter in current selection:
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
int enterSelection(){
    changeQuery(QUERY_SINGLE_ENTRY,
                "Select media.*, \
                        title || ' - ' || artist || ' (' || album || ')' as strfield,  \
                        'path = ''' || replace(path, '''', '''''') || '''' as datafield \
                 From media ",
                 commonMenu.elements[commonMenu.selected].data,
				 orderBy[selectedOrderBy], exitSelection, NULL);
    oslReadKeys(); //To avoid reread the CROSS button after entering selection
    return 0;
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Enter in current artist:
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
int enterArtist(){
	snprintf(tempSql, sizeof(tempSql), "Select album || ' - ' || artist as strfield, 'artist = ''' || replace(artist, '''', '''''') || ''' and album = ''' || replace(album, '''', '''''') || '''' as datafield, count(*) as intfield01, sum(seconds) as intfield02 \
									    from media \
									    Where %s \
										group by album order by upper(album)", commonMenu.elements[commonMenu.selected].data);

    changeQuery(QUERY_COUNT, tempSql, "", "", exitSelection, enterSelection);
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

		cpuBoost();
        snprintf(tempSql, sizeof(tempSql), "%s Where %s Order By %s", MlStatus[currentStatus].select, MlStatus[currentStatus].where, MlStatus[currentStatus].orderBy);
		ML_queryDBSelect(tempSql, mlBufferPosition, ML_BUFFERSIZE, MLresult);
        cpuRestore();
    }

	int startX = 0;
	int startY = 0;
	switch (mlQueryType)
	{
	case QUERY_COUNT:
        if (!strlen(element->text)){
            snprintf(element->text, sizeof(buffer), "%s (%.f)", MLresult[index - mlBufferPosition].strField, MLresult[index - mlBufferPosition].intField01);
            strcpy(element->data, MLresult[index - mlBufferPosition].dataField);
            element->icon = folderIcon;
            element->triggerFunction = MlStatus[currentStatus].triggerFunction;
        }
		break;
	case QUERY_SINGLE_ENTRY:
        if (!strlen(element->text)){
            strcpy(element->data, MLresult[index - mlBufferPosition].dataField);
            strcpy(element->text, MLresult[index - mlBufferPosition].strField);
            element->icon = musicIcon;
            element->triggerFunction = MlStatus[currentStatus].triggerFunction;
        }
		break;
	case QUERY_COUNT_RATING:
	    startY = commonMenu.yPos + (float)(commonMenu.height -  commonMenu.maxNumberVisible * (fontMenuNormal->charHeight + commonMenu.interline)) / 2.0;
		startX = drawRating(commonMenu.xPos + 4, startY + (fontMenuNormal->charHeight * index + commonMenu.interline * index), atoi(MLresult[index - mlBufferPosition].strField));
        snprintf(buffer, sizeof(buffer), "(%.f)", MLresult[index - mlBufferPosition].intField01);
		strcpy(element->text, "");
		strcpy(element->data, MLresult[index - mlBufferPosition].dataField);
        element->triggerFunction = MlStatus[currentStatus].triggerFunction;
		break;
	case QUERY_COUNT_ARTIST:
        if (!strlen(element->text)){
            snprintf(element->text, sizeof(buffer), "%s (%.f)", MLresult[index - mlBufferPosition].strField, MLresult[index - mlBufferPosition].intField01);
            strcpy(element->data, MLresult[index - mlBufferPosition].dataField);
            element->icon = folderIcon;
            element->triggerFunction = MlStatus[currentStatus].triggerFunction;
        }
		break;
	}
}


int buildQueryMenu(char *select, char *where, char *orderBy, int (*cancelFunction)(), int selected){
    drawQueryRunning();
    mediaLibraryStatus = STATUS_QUERYMENU;

	cpuBoost();

	if (strlen(where) && strlen(orderBy))
		snprintf(tempSql, sizeof(tempSql), "%s Where %s Order By %s", select, where, orderBy);
	else
		snprintf(tempSql, sizeof(tempSql), "%s", select);
    mlQueryCount = ML_countRecordsSelect(tempSql);
	if (mlQueryCount > MENU_MAX_ELEMENTS)
		mlQueryCount = MENU_MAX_ELEMENTS;
    ML_queryDBSelect(tempSql, 0, ML_BUFFERSIZE, MLresult);
	cpuRestore();

    clearMenu(&commonMenu);
    commonMenu.numberOfElements = mlQueryCount;
    commonMenu.first = selected;
    commonMenu.selected = selected;
    skinGetPosition("POS_MEDIALIBRARY_QUERY", tempPos);
    commonMenu.xPos = tempPos[0];
    commonMenu.yPos = tempPos[1];
    commonMenu.fastScrolling = 1;
    snprintf(buffer, sizeof(buffer), "%s/medialibraryquerybkg.png", userSettings->skinImagesPath);
    commonMenu.background = oslLoadImageFilePNG(buffer, OSL_IN_RAM | OSL_SWIZZLED, OSL_PF_8888);
    if (!commonMenu.background)
        errorLoadImage(buffer);
    commonMenu.highlight = commonMenuHighlight;
    commonMenu.width = commonMenu.background->sizeX;
    commonMenu.height = commonMenu.background->sizeY;
    commonMenu.dataFeedFunction = queryDataFeed;
    commonMenu.align = ALIGN_LEFT;
    commonMenu.interline = skinGetParam("MENU_INTERLINE");
    commonMenu.maxNumberVisible = commonMenu.background->sizeY / (fontMenuNormal->charHeight + commonMenu.interline);
    if (commonMenu.selected < commonMenu.maxNumberVisible)
        commonMenu.first = 0;
    commonMenu.cancelFunction = (cancelFunction);

    return 0;
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Functions to change and restore the queries:
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
int applyCurrentQuery()
{
    if (currentStatus < 0)
        return -1;

    mlQueryCount = -1;
    mlBufferPosition = MlStatus[currentStatus].offset;

	buildQueryMenu(MlStatus[currentStatus].select,
                   MlStatus[currentStatus].where,
				   MlStatus[currentStatus].orderBy,
                   MlStatus[currentStatus].exitFunction,
                   MlStatus[currentStatus].selected);
    mlQueryType = MlStatus[currentStatus].queryType ;

    return 0;
}

int changeQuery(int queryType, char *select, char *where, char *orderBy,
                int (*cancelFunction)(), int (*triggerFunction)())
{
    if (currentStatus == 3)
        return -1;

    if (currentStatus >= 0){
        MlStatus[currentStatus].offset = mlBufferPosition;
        MlStatus[currentStatus].selected = commonMenu.selected;
        MlStatus[currentStatus].orderByMenuSelected = selectedOrderBy;
    }

    MlStatus[++currentStatus].queryType = queryType;
    strcpy(MlStatus[currentStatus].select, select);
    strcpy(MlStatus[currentStatus].where, where);
    strcpy(MlStatus[currentStatus].orderBy, orderBy);
    MlStatus[currentStatus].offset = 0;
    MlStatus[currentStatus].selected = 0;
    MlStatus[currentStatus].exitFunction = cancelFunction;
    MlStatus[currentStatus].triggerFunction = triggerFunction;
    MlStatus[currentStatus].orderByMenuSelected = 0;

    applyCurrentQuery();
    return 0;
}

int restoreQuery()
{
    if (currentStatus <= 0)
        return -1;

    MlStatus[currentStatus].offset = mlBufferPosition;
    MlStatus[currentStatus].selected = commonMenu.selected;

    currentStatus--;

    applyCurrentQuery();
    return 0;
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Browse all:
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
int browseAll(){
    changeQuery(QUERY_SINGLE_ENTRY, "Select media.*, \
                                            title || ' - ' || artist || ' (' || album || ')' as strfield,  \
                                            'path = ''' || replace(path, '''', '''''') || '''' as datafield \
                                     From media ",
                                    "1=1",
                                    orderBy[selectedOrderBy],
                                    backToMainMenu, NULL);
    oslReadKeys(); //To avoid reread the CROSS button after entering selection
	return 0;
}


///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Browse by artist:
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
int browseByArtist(){
    changeQuery(QUERY_COUNT_ARTIST, "Select artist as strfield, 'artist = ''' || replace(artist, '''', '''''') || '''' as datafield, count(*) as intfield01, sum(seconds) as intfield02 from media group by artist order by upper(artist)",
                                    "", "", backToMainMenu, enterArtist);
    oslReadKeys(); //To avoid reread the CROSS button after entering selection
    return 0;
}


///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Browse by ALBUM:
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
int browseByAlbum(){
    changeQuery(QUERY_COUNT, "Select album || ' - ' || artist as strfield, 'artist = ''' || replace(artist, '''', '''''') || ''' and album = ''' || replace(album, '''', '''''') || '''' as datafield, count(*) as intfield01, sum(seconds) as intfield02 from media group by artist, album order by upper(album), upper(artist)",
                             "", "", backToMainMenu, enterSelection);
    oslReadKeys(); //To avoid reread the CROSS button after entering selection
    return 0;
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Browse by GENRE:
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
int browseByGenre(){
    changeQuery(QUERY_COUNT, "Select genre as strfield, 'genre = ''' || replace(genre, '''', '''''') || '''' as datafield, count(*) as intfield01, sum(seconds) as intfield02 from media group by genre order by upper(genre)",
                             "", "", backToMainMenu, enterSelection);
    oslReadKeys(); //To avoid reread the CROSS button after entering selection
    return 0;
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Browse by rating:
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
int browseByRating(){
    changeQuery(QUERY_COUNT_RATING, "Select cast(coalesce(rating, 0) as varchar) as strfield, 'rating = ' || cast(coalesce(rating, 0) as varchar) as datafield, count(*) as intfield01, sum(seconds) as intfield02 from media group by coalesce(rating, 0) order by coalesce(rating, 0) desc",
                                    "", "", backToMainMenu, enterSelection);
    oslReadKeys(); //To avoid reread the CROSS button after entering selection
    return 0;
}


///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Browse TOP 100:
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
int browseTop100(){
    changeQuery(QUERY_SINGLE_ENTRY,
                "Select media.*, \
                        title || ' - ' || artist || ' (' || album || ')' as strfield,  \
                        'path = ''' || replace(path, '''', '''''') || '''' as datafield \
                 From media",
				 "1=1",
				 "played desc, rating desc, title LIMIT 100",
				 backToMainMenu, NULL);
    oslReadKeys(); //To avoid reread the CROSS button after entering selection
    return 0;
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Ask for Search string:
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void askSearchString(char *message, char *initialValue, char *target){
	int skip = 0;
	int done = 0;
	unsigned short carattere16[129];
	char* utf8Str;

	cpuBoost();
	oslInitOsk(message, initialValue, 128, 1, getOSKlang());
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
					oslOskGetTextUCS2(carattere16);
					utf8Str = miniConvUTF16LEConv(carattere16);
					target[0] = '\0';
					if ( utf8Str != NULL ) {
						strncpy(target, utf8Str, 128);
						target[128] = '\0';
					}
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
	cpuRestore();
}


///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Search:
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
int search(){
    char searchString[129] = "";
	askSearchString(langGetString("ASK_SEARCH_STRING"), "", searchString);
	if (strlen(searchString)){
		oslStartDrawing();
		drawCommonGraphics();
		drawButtonBar(MODE_MEDIA_LIBRARY);
		oslEndDrawing();
		oslSyncFrame();

        mlQueryType = QUERY_SINGLE_ENTRY;
        mlQueryCount = -1;
        mlBufferPosition = 0;

		char tempWhere[512] = "";
		snprintf(tempWhere, sizeof(tempWhere), "title like '%%%s%%' \
                            or artist like '%%%s%%' \
                            or album like '%%%s%%' ",
                            searchString, searchString, searchString);
        buildQueryMenu("Select media.*, title || ' - ' || artist as strfield, \
                               'path = ''' || replace(path, '''', '''''') || '''' as datafield \
                        From media",
						tempWhere,
						"title, artist, album",
						backToMainMenu, 0);
	}
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
    snprintf(buffer, sizeof(buffer), "%s/medialibrarybkg.png", userSettings->skinImagesPath);
    commonMenu.background = oslLoadImageFilePNG(buffer, OSL_IN_RAM | OSL_SWIZZLED, OSL_PF_8888);
    if (!commonMenu.background)
        errorLoadImage(buffer);
    commonMenu.highlight = commonMenuHighlight;
    commonMenu.width = commonMenu.background->sizeX;
    commonMenu.height = commonMenu.background->sizeY;
    commonMenu.interline = skinGetParam("MENU_INTERLINE");
    commonMenu.maxNumberVisible = commonMenu.background->sizeY / (fontMenuNormal->charHeight + commonMenu.interline);
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
// Build order by Menu:
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
int buildOrderByMenu(){
    skinGetPosition("POS_MEDIALIBRARY_QUERY", tempPos);
    commonSubMenu.yPos = tempPos[1];
    commonSubMenu.xPos = tempPos[0];
    commonSubMenu.fastScrolling = 0;
    commonSubMenu.align = ALIGN_LEFT;
    snprintf(buffer, sizeof(buffer), "%s/medialibraryquerybkg.png", userSettings->skinImagesPath);
    commonSubMenu.background = oslLoadImageFilePNG(buffer, OSL_IN_RAM | OSL_SWIZZLED, OSL_PF_8888);
    if (!commonSubMenu.background)
        errorLoadImage(buffer);
    commonSubMenu.highlight = commonMenuHighlight;
    commonSubMenu.width = commonMenu.background->sizeX;
    commonSubMenu.height = commonMenu.background->sizeY;
    commonSubMenu.interline = skinGetParam("MENU_INTERLINE");
    commonSubMenu.maxNumberVisible = commonSubMenu.background->sizeY / (fontMenuNormal->charHeight + commonSubMenu.interline);
    commonSubMenu.cancelFunction = NULL;

    commonSubMenu.first = 0;
    commonSubMenu.selected = 0;
    commonSubMenu.numberOfElements = MAX_ORDER_BY;

    int i = 0;
    for (i=0; i<MAX_ORDER_BY; i++){
        commonSubMenu.elements[i].icon = NULL;
        commonSubMenu.elements[i].triggerFunction = NULL;
        snprintf(buffer, sizeof(buffer), "ORDER_BY_%2.2i", i + 1);
        snprintf(commonSubMenu.elements[i].text, sizeof(commonSubMenu.elements[i].text), "%s", langGetString(buffer));
    }
    return 0;
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Order by Menu:
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
int showOrderByMenu(){
    commonSubMenu.selected = selectedOrderBy;
    while(!osl_quit && !exitFlagMediaLibrary){
        oslStartDrawing();
        drawCommonGraphics();
        drawButtonBar(MODE_MEDIA_LIBRARY);

        if (mediaLibraryStatus == STATUS_QUERYMENU)
            drawMLinfo();
        drawMenu(&commonSubMenu);
        oslEndDrawing();
        oslEndFrame();
        oslSyncFrame();

        oslReadKeys();
        if (getCancelButton())
            break;
		else if (getConfirmButton()){
			selectedOrderBy = commonSubMenu.selected;
            strcpy(MlStatus[currentStatus].orderBy, orderBy[selectedOrderBy]);
			buildQueryMenu(MlStatus[currentStatus].select, MlStatus[currentStatus].where, MlStatus[currentStatus].orderBy, MlStatus[currentStatus].exitFunction, 0);
			break;
		}
        processMenuKeys(&commonSubMenu);
    }

    return 0;
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Media Library:
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
int gui_mediaLibrary(){
    int ratingChangedUpDown = 0;
    currentStatus = -1;

    //Load images:
    snprintf(buffer, sizeof(buffer), "%s/medialibraryscanbkg.png", userSettings->skinImagesPath);
    scanBkg = oslLoadImageFilePNG(buffer, OSL_IN_RAM | OSL_SWIZZLED, OSL_PF_8888);
    if (!scanBkg)
        errorLoadImage(buffer);

    snprintf(buffer, sizeof(buffer), "%s/medialibraryinfobkg.png", userSettings->skinImagesPath);
    infoBkg = oslLoadImageFilePNG(buffer, OSL_IN_RAM | OSL_SWIZZLED, OSL_PF_8888);
    if (!infoBkg)
        errorLoadImage(buffer);

    //Build menu:
    buildMainMenu();
    buildOrderByMenu();

    exitFlagMediaLibrary = 0;
	int skip = 0;

	cpuRestore();
    while(!osl_quit && !exitFlagMediaLibrary){
		if (!skip){
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
	    	oslEndDrawing();
		}
        oslEndFrame();
    	skip = oslSyncFrame();

        oslReadKeys();
        if (confirmStatus == STATUS_CONFIRM_SCAN){
            if(getConfirmButton()){
                confirmStatus = STATUS_CONFIRM_NONE;
                scanMS();
            }else if(osl_pad.pressed.circle)
                confirmStatus = STATUS_CONFIRM_NONE;
        }else if (confirmStatus == STATUS_HELP){
            if (getConfirmButton() || getCancelButton())
                confirmStatus = STATUS_CONFIRM_NONE;
        }else{
            if (osl_pad.held.L && osl_pad.held.R){
                confirmStatus = STATUS_HELP;
            }else if(mediaLibraryStatus == STATUS_QUERYMENU && osl_pad.released.triangle && mlQueryType == QUERY_SINGLE_ENTRY){
                showOrderByMenu();
            }else if(mediaLibraryStatus == STATUS_MAINMENU && getConfirmButton() && commonMenu.selected == 7){
                confirmStatus = STATUS_CONFIRM_SCAN;
            }else if(osl_pad.released.start && mediaLibraryStatus == STATUS_QUERYMENU && commonMenu.numberOfElements){
                addSelectionToPlaylist(commonMenu.elements[commonMenu.selected].data, "artist, album, tracknumber, title", 0, tempM3Ufile);
            }else if(!ratingChangedUpDown && getConfirmButton() && commonMenu.numberOfElements && mediaLibraryStatus == STATUS_QUERYMENU && mlQueryType == QUERY_SINGLE_ENTRY){
                M3U_clear();
                M3U_save(MLtempM3Ufile);
                addSelectionToPlaylist(MlStatus[currentStatus].where, MlStatus[currentStatus].orderBy, 1, MLtempM3Ufile);
                snprintf(userSettings->selectedBrowserItem, sizeof(userSettings->selectedBrowserItem), "%s", MLtempM3Ufile);
                snprintf(userSettings->selectedBrowserItemShort, sizeof(userSettings->selectedBrowserItemShort), "%s", MLtempM3Ufile);
                userSettings->playlistStartIndex = commonMenu.selected;
                mediaLibraryRetValue = MODE_PLAYER;
                userSettings->previousMode = MODE_MEDIA_LIBRARY;
                exitFlagMediaLibrary = 1;
            }else if(osl_pad.released.square && mediaLibraryStatus == STATUS_QUERYMENU && commonMenu.numberOfElements && (mlQueryType == QUERY_COUNT || mlQueryType == QUERY_COUNT_RATING || mlQueryType == QUERY_COUNT_ARTIST)){
                M3U_clear();
                M3U_save(MLtempM3Ufile);
                addSelectionToPlaylist(commonMenu.elements[commonMenu.selected].data,  "artist, album, tracknumber, title", 1, MLtempM3Ufile);
                snprintf(userSettings->selectedBrowserItem, sizeof(userSettings->selectedBrowserItem), "%s", MLtempM3Ufile);
                snprintf(userSettings->selectedBrowserItemShort, sizeof(userSettings->selectedBrowserItemShort), "%s", MLtempM3Ufile);
                userSettings->playlistStartIndex = -1;
                mediaLibraryRetValue = MODE_PLAYER;
                userSettings->previousMode = MODE_MEDIA_LIBRARY;
                exitFlagMediaLibrary = 1;
            }else if (osl_pad.held.cross && osl_pad.held.up && commonMenu.numberOfElements && mediaLibraryStatus == STATUS_QUERYMENU && mlQueryType == QUERY_SINGLE_ENTRY){
                if (++MLresult[commonMenu.selected - mlBufferPosition].rating > ML_MAX_RATING)
                    MLresult[commonMenu.selected - mlBufferPosition].rating = ML_MAX_RATING;
                ratingChangedUpDown = 1;
                ML_updateEntry(MLresult[commonMenu.selected - mlBufferPosition], "");
                sceKernelDelayThread(userSettings->KEY_AUTOREPEAT_PLAYER*15000);
            }else if (osl_pad.held.cross && osl_pad.held.down  && commonMenu.numberOfElements && mediaLibraryStatus == STATUS_QUERYMENU && mlQueryType == QUERY_SINGLE_ENTRY){
                if (--MLresult[commonMenu.selected - mlBufferPosition].rating < 0)
                    MLresult[commonMenu.selected - mlBufferPosition].rating = 0;
                ratingChangedUpDown = 1;
                ML_updateEntry(MLresult[commonMenu.selected - mlBufferPosition], "");
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

            if (ratingChangedUpDown && getConfirmButton())
                ratingChangedUpDown = 0;
        }
    }

    clearMenu(&commonMenu);
    clearMenu(&commonSubMenu);

    oslDeleteImage(scanBkg);
    scanBkg = NULL;
    oslDeleteImage(infoBkg);
    infoBkg = NULL;
	if (coverArt){
		oslDeleteImage(coverArt);
        coverArt = NULL;
    }
	return mediaLibraryRetValue;
}
