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
#include "gui_mediaLibrary.h"
#include "common.h"
#include "languages.h"
#include "settings.h"
#include "menu.h"
#include "../others/strreplace.h"
#include "../others/medialibrary.h"
#include "../system/osk.h"
#include "../system/clock.h"
#include "../players/player.h"
#include "../players/m3u.h"

#define STATUS_MAINMENU 0
#define STATUS_QUERYMENU 1

#define STATUS_CONFIRM_NONE 0
#define STATUS_CONFIRM_SCAN 1

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
struct menuElement tMenuEl;
char buffer[264];
int mediaLibraryStatus = STATUS_MAINMENU;
int confirmStatus = STATUS_CONFIRM_NONE;

int mlQueryType = QUERY_SINGLE_ENTRY;
int mlPrevQueryType = 0;
int mlQueryCount = -1;
int mlBufferPosition = 0;
OSL_IMAGE *scanBkg;
OSL_IMAGE *infoBkg;

char currentSql[ML_SQLMAXLENGTH] = "";
char previousSql[ML_SQLMAXLENGTH] = "";
char tempSql[ML_SQLMAXLENGTH] = "";

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Add selection to playlist:
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
int addSelectionToPlaylist(char *where, int fastMode, char *m3uName){
    int offset = 0;
    struct libraryEntry localResult[ML_BUFFERSIZE];
    struct fileInfo info;
    char onlyName[264] = "";
    char message[10] = "";

    int i = 0;
    int count = ML_countRecords(where);
    if (count){
        setCpuClock(222);
        setBusClock(111);
        M3U_open(m3uName);
        ML_queryDB(where, "title", offset, ML_BUFFERSIZE, localResult);
        for (i=0; i<count; i++){
            if (i >= offset + ML_BUFFERSIZE){
                offset += ML_BUFFERSIZE;
                ML_queryDB(where, "title", offset, ML_BUFFERSIZE, localResult);
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
        	oslSyncFrame();
        }
        M3U_save(m3uName);
        setBusClock(userSettings->BUS);
        setCpuClock(userSettings->CLOCK_GUI);
    }
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
    oslSetTextColor(RGBA(userSettings->colorPopupTitle[0], userSettings->colorPopupTitle[1], userSettings->colorPopupTitle[2], userSettings->colorPopupTitle[3]));
    oslDrawString((480 - oslGetStringWidth(langGetString("CHECKING_FILE"))) / 2, startY + 3, langGetString("CHECKING_FILE"));
    oslSetTextColor(RGBA(userSettings->colorPopupMessage[0], userSettings->colorPopupMessage[1], userSettings->colorPopupMessage[2], userSettings->colorPopupMessage[3]));
    getFileName(fileName, buffer);
    if (strlen(buffer) > 70)
       buffer[70] = '\0';
    oslDrawString((480 - oslGetStringWidth(buffer)) / 2, startY + 30, buffer);

    oslReadKeys();
    oslEndDrawing();
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
    oslSetTextColor(RGBA(userSettings->colorPopupTitle[0], userSettings->colorPopupTitle[1], userSettings->colorPopupTitle[2], userSettings->colorPopupTitle[3]));
    oslDrawString((480 - oslGetStringWidth(langGetString("SCANNING"))) / 2, startY + 3, langGetString("SCANNING"));
    oslSetTextColor(RGBA(userSettings->colorPopupMessage[0], userSettings->colorPopupMessage[1], userSettings->colorPopupMessage[2], userSettings->colorPopupMessage[3]));
    getFileName(dirName, buffer);
    if (strlen(buffer) > 70)
       buffer[70] = '\0';
    oslDrawString((480 - oslGetStringWidth(buffer)) / 2, startY + 30, buffer);

    oslReadKeys();
    oslEndDrawing();
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
        if (!osl_keys->pressed.hold){
            if(osl_keys->released.cross){
                oslEndDrawing();
                oslSyncFrame();
                break;
            }
        }

        oslEndDrawing();
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
    oslSyncFrame();
}


///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Draw info on selected item:
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void drawMLinfo(){
    int startX = userSettings->playlistInfoX;
    int startY = userSettings->playlistInfoY;
    OSL_FONT *font = fontNormal;

    if (mediaLibraryStatus != STATUS_QUERYMENU)
        return;

    oslDrawImageXY(infoBkg, startX, startY);
    oslSetFont(font);
    oslSetTextColor(RGBA(userSettings->colorLabel[0], userSettings->colorLabel[1], userSettings->colorLabel[2], userSettings->colorLabel[3]));

    if (mlQueryType == QUERY_COUNT || mlQueryType == QUERY_COUNT_RATING){
        oslDrawString(startX + 2, startY + 5, langGetString("TOTAL_TRACKS"));
        oslDrawString(startX + 2, startY + 20, langGetString("TOTAL_TIME"));
        oslSetTextColor(RGBA(userSettings->colorText[0], userSettings->colorText[1], userSettings->colorText[2], userSettings->colorText[3]));
        sprintf(buffer, "%.f", MLresult[commonMenu.selected - mlBufferPosition].intField01);
        oslDrawString(startX + 80, startY + 5, buffer);

        formatHHMMSS(MLresult[commonMenu.selected - mlBufferPosition].intField02, buffer);
        oslDrawString(startX + 80, startY + 20, buffer);
    }else if (mlQueryType == QUERY_SINGLE_ENTRY){
        oslDrawString(startX + 2, startY + 5, langGetString("GENRE"));
        oslDrawString(startX + 2, startY + 20, langGetString("YEAR"));
        oslDrawString(startX + 200, startY + 20, langGetString("TIME"));
        oslDrawString(startX + 2, startY + 35, langGetString("RATING"));

        oslSetTextColor(RGBA(userSettings->colorText[0], userSettings->colorText[1], userSettings->colorText[2], userSettings->colorText[3]));
        oslDrawString(startX + 80, startY + 5, MLresult[commonMenu.selected - mlBufferPosition].genre);
        oslDrawString(startX + 80, startY + 20, MLresult[commonMenu.selected - mlBufferPosition].year);

        formatHHMMSS(MLresult[commonMenu.selected - mlBufferPosition].seconds, buffer);
        oslDrawString(startX + 250, startY + 20, buffer);

        drawRating(startX + 80, startY + 33, MLresult[commonMenu.selected - mlBufferPosition].rating);
    }
}


///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Exit from current selection:
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
int exitSelection(){
    strcpy(tempSql, previousSql);
    buildQueryMenu(tempSql, backToMainMenu);
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
    buildQueryMenu(tempSql, exitSelection);
    mlPrevQueryType = mlQueryType;
    mlQueryType = QUERY_SINGLE_ENTRY;
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
        strcpy(element->data, MLresult[index - mlBufferPosition].dataField);
        element->triggerFunction = NULL;
    }else if (mlQueryType == QUERY_COUNT_RATING){
        drawRating(commonMenu.xPos + 4, commonMenu.yPos + (fontNormal->charHeight * index + commonMenu.interline * index), atoi(MLresult[index - mlBufferPosition].strField));
        sprintf(buffer, "(%.f)", MLresult[index - mlBufferPosition].intField01);
        oslSetFont(fontNormal);
        if (index == commonMenu.selected)
            oslSetTextColor(RGBA(userSettings->colorMenuSelected[0], userSettings->colorMenuSelected[1], userSettings->colorMenuSelected[2], userSettings->colorMenuSelected[3]));
        else
            oslSetTextColor(RGBA(userSettings->colorMenu[0], userSettings->colorMenu[1], userSettings->colorMenu[2], userSettings->colorMenu[3]));
        oslDrawString(commonMenu.xPos + star->sizeX*ML_MAX_RATING + 8, commonMenu.yPos + (fontNormal->charHeight * index + commonMenu.interline * index), buffer);
        strcpy(element->data, MLresult[index - mlBufferPosition].dataField);
        element->triggerFunction = enterSelection;
    }
}

int buildQueryMenu(char *select, int (*cancelFunction)()){
    drawQueryRunning();
    mediaLibraryStatus = STATUS_QUERYMENU;

    mlQueryCount = ML_countRecordsSelect(select);

    setCpuClock(222);
    setBusClock(111);
    ML_queryDBSelect(select, 0, ML_BUFFERSIZE, MLresult);
    setBusClock(userSettings->BUS);
    setCpuClock(userSettings->CLOCK_GUI);
    strcpy(currentSql, select);

    clearMenu(&commonMenu);
    commonMenu.numberOfElements = mlQueryCount;
    commonMenu.first = 0;
    commonMenu.selected = 0;
    commonMenu.yPos = userSettings->playlistBrowserY;
    commonMenu.xPos = userSettings->playlistBrowserX;
    commonMenu.fastScrolling = 1;
    sprintf(buffer, "%s/medialibraryquerybkg.png", userSettings->skinImagesPath);
    commonMenu.background = oslLoadImageFilePNG(buffer, OSL_IN_RAM | OSL_SWIZZLED, OSL_PF_8888);
    if (!commonMenu.background)
        errorLoadImage(buffer);
    sprintf(buffer, "%s/menuhighlight.png", userSettings->skinImagesPath);
    commonMenu.highlight = oslLoadImageFilePNG(buffer, OSL_IN_RAM | OSL_SWIZZLED, OSL_PF_8888);
    if (!commonMenu.highlight)
        errorLoadImage(buffer);
    commonMenu.width = commonMenu.background->sizeX;
    commonMenu.height = commonMenu.background->sizeY;
    commonMenu.dataFeedFunction = queryDataFeed;
    commonMenu.align = ALIGN_LEFT;
    commonMenu.interline = 1;
    commonMenu.maxNumberVisible = commonMenu.background->sizeY / (fontNormal->charHeight + commonMenu.interline);
    commonMenu.cancelFunction = (cancelFunction);

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
// Search:
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
int search(){
	oslEndDrawing();
	oslSyncFrame();
	setCpuClock(222);
    char *searchString = requestString(langGetString("ASK_SEARCH_STRING"), "");
    oslInitGfx(OSL_PF_8888, 1); //Re-init OSLib to avoid gaphics corruption!

    oslStartDrawing();
    drawCommonGraphics();
    drawButtonBar(MODE_PLAYLIST_EDITOR);
	oslEndDrawing();
	oslSyncFrame();

    setCpuClock(userSettings->CLOCK_GUI);

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
    }
    return 0;
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Main Menu:
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
int buildMainMenu(){
    mediaLibraryStatus = STATUS_MAINMENU;
    commonMenu.yPos = userSettings->medialibraryY;
    commonMenu.xPos = userSettings->medialibraryX;
    commonMenu.fastScrolling = 0;
    commonMenu.align = ALIGN_CENTER;
    sprintf(buffer, "%s/medialibrarybkg.png", userSettings->skinImagesPath);
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

    commonMenu.first = 0;
    commonMenu.selected = 0;

    strcpy(tMenuEl.text, langGetString("BROWSE_ARTIST"));
    tMenuEl.triggerFunction = browseByArtist;
    commonMenu.elements[0] = tMenuEl;
    strcpy(tMenuEl.text, langGetString("BROWSE_ALBUM"));
    tMenuEl.triggerFunction = browseByAlbum;
    commonMenu.elements[1] = tMenuEl;
    strcpy(tMenuEl.text, langGetString("BROWSE_GENRE"));
    tMenuEl.triggerFunction = browseByGenre;
    commonMenu.elements[2] = tMenuEl;
    strcpy(tMenuEl.text, langGetString("BROWSE_RATING"));
    tMenuEl.triggerFunction = browseByRating;
    commonMenu.elements[3] = tMenuEl;
    strcpy(tMenuEl.text, langGetString("SEARCH"));
    tMenuEl.triggerFunction = search;
    commonMenu.elements[4] = tMenuEl;
    strcpy(tMenuEl.text, langGetString("SCAN_MS"));
    tMenuEl.triggerFunction = NULL;
    commonMenu.elements[5] = tMenuEl;

    commonMenu.numberOfElements = 6;
    return 0;
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Media Library:
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
int gui_mediaLibrary(){
    //int ratingChangedUpDown = 0;

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

        if (confirmStatus == STATUS_CONFIRM_SCAN)
            drawConfirm(langGetString("CONFIRM_SCAN_MS_TITLE"), langGetString("CONFIRM_SCAN_MS"));

        oslReadKeys();
        if (!osl_keys->pressed.hold){
            /*if (ratingChangedUpDown)
                ratingChangedUpDown = 0;
            else*/
            processMenuKeys(&commonMenu);

            if (confirmStatus == STATUS_CONFIRM_SCAN){
                if(osl_keys->released.cross){
                    confirmStatus = STATUS_CONFIRM_NONE;
                    scanMS();
                }else if(osl_keys->pressed.circle)
                    confirmStatus = STATUS_CONFIRM_NONE;
            }else{
                if(mediaLibraryStatus == STATUS_MAINMENU && osl_keys->released.cross && commonMenu.selected == 5){
                    confirmStatus = STATUS_CONFIRM_SCAN;
                }else if(osl_keys->released.start && mediaLibraryStatus == STATUS_QUERYMENU){
                    addSelectionToPlaylist(commonMenu.elements[commonMenu.selected].data, 0, tempM3Ufile);
                }else if(osl_keys->released.square && mediaLibraryStatus == STATUS_QUERYMENU && (mlQueryType == QUERY_COUNT || mlQueryType == QUERY_COUNT_RATING)){
                    M3U_clear();
                    M3U_save(MLtempM3Ufile);
                    addSelectionToPlaylist(commonMenu.elements[commonMenu.selected].data, 1, MLtempM3Ufile);
                    sprintf(userSettings->selectedBrowserItem, "%s", MLtempM3Ufile);
                    mediaLibraryRetValue = MODE_PLAYER;
                    userSettings->previousMode = MODE_MEDIA_LIBRARY;
                    exitFlagMediaLibrary = 1;
               /* }else if (osl_keys->held.cross && osl_keys->held.up && mediaLibraryStatus == STATUS_QUERYMENU && mlQueryType == QUERY_SINGLE_ENTRY){
                    if (++MLresult[commonMenu.selected - mlBufferPosition].rating > ML_MAX_RATING)
                        MLresult[commonMenu.selected - mlBufferPosition].rating = ML_MAX_RATING;
                    ratingChangedUpDown = 1;
                    ML_updateEntry(MLresult[commonMenu.selected - mlBufferPosition]);
                    sceKernelDelayThread(KEY_AUTOREPEAT_PLAYER*15000);
                }else if (osl_keys->held.cross && osl_keys->held.down  && mediaLibraryStatus == STATUS_QUERYMENU && mlQueryType == QUERY_SINGLE_ENTRY){
                    if (--MLresult[commonMenu.selected - mlBufferPosition].rating < 0)
                        MLresult[commonMenu.selected - mlBufferPosition].rating = 0;
                    ratingChangedUpDown = 1;
                    ML_updateEntry(MLresult[commonMenu.selected - mlBufferPosition]);
                    sceKernelDelayThread(KEY_AUTOREPEAT_PLAYER*15000);*/
                }else if(osl_keys->released.R){
                    mediaLibraryRetValue = nextAppMode(MODE_MEDIA_LIBRARY);
                    exitFlagMediaLibrary = 1;
                }else if(osl_keys->released.L){
                    mediaLibraryRetValue = previousAppMode(MODE_MEDIA_LIBRARY);
                    exitFlagMediaLibrary = 1;
                }
            }
        }
    	oslEndDrawing();
    	oslSyncFrame();
    }

    clearMenu(&commonMenu);
    oslDeleteImage(scanBkg);
    oslDeleteImage(infoBkg);
    return mediaLibraryRetValue;
}
