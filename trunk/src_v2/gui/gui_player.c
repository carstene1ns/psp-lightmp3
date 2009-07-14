//    LightMP3
//    Copyright (C) 2007,2008,2009 Sakya
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
#include <psprtc.h>
#include <stdio.h>
#include <stdlib.h>
#include <oslib/oslib.h>
#include <malloc.h>

#include "../main.h"
#include "gui_player.h"
#include "common.h"
#include "languages.h"
#include "settings.h"
#include "skinsettings.h"
#include "../system/opendir.h"
#include "../system/clock.h"
#include "../system/brightness.h"
#include "../players/player.h"
#include "../players/equalizer.h"
#include "../players/m3u.h"
#include "../players/id3.h"
#include "../others/audioscrobbler.h"
#include "../others/medialibrary.h"
#include "../others/bookmark.h"
#include "../others/strreplace.h"

#define PLAYER_STOP 2
#define PLAYER_NEXT 1
#define PLAYER_END 0
#define PLAYER_PREVIOUS -1

#define STATUS_NORMAL 0
#define STATUS_HELP 1
#define STATUS_CONFIRM_BOOKMARK 2

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Functions imported from prx:
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
int displayEnable(void);
int displayDisable(void);
int getBrightness();
void setBrightness(int brightness);
int imposeSetHomePopup(int value);

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Globals:
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
static int playerRetValue = 0;
static OSL_IMAGE *coverArt;
static OSL_IMAGE *noCoverArt;
static OSL_IMAGE *fileInfoBkg;
static OSL_IMAGE *fileSpecsBkg;
static OSL_IMAGE *progressBkg;
static OSL_IMAGE *progress;
static OSL_IMAGE *playerStatusBkg;
static char buffer[264];

static char sleepModeDesc[7][102];
static char playModeDesc[5][102];
static char playerStatusDesc[3][102];
static int playerStatus = 0; //-1=open 0=paused 1=playing
static struct equalizer tEQ;
static u64 sleepStartTime = 0; //Current ticks when sleep timer was started.

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Draws a file's info
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
int drawFileInfo(struct fileInfo *info, struct libraryEntry *libEntry, char *trackMessage){
    char timestring[20] = "";

    skinGetPosition("POS_FILE_INFO_BKG", tempPos);
    oslDrawImageXY(fileInfoBkg, tempPos[0], tempPos[1]);
    oslSetFont(fontNormal);
    skinGetColor("RGBA_LABEL_TEXT", tempColor);
    skinGetColor("RGBA_LABEL_TEXT_SHADOW", tempColorShadow);
    setFontStyle(fontNormal, defaultTextSize, RGBA(tempColor[0], tempColor[1], tempColor[2], tempColor[3]), RGBA(tempColorShadow[0], tempColorShadow[1], tempColorShadow[2], tempColorShadow[3]), INTRAFONT_ALIGN_LEFT);
    skinGetPosition("POS_TITLE_LABEL", tempPos);
    oslDrawString(tempPos[0], tempPos[1], langGetString("TITLE"));
    skinGetPosition("POS_ARTIST_LABEL", tempPos);
    oslDrawString(tempPos[0], tempPos[1], langGetString("ARTIST"));
    skinGetPosition("POS_ALBUM_LABEL", tempPos);
    oslDrawString(tempPos[0], tempPos[1], langGetString("ALBUM"));
    skinGetPosition("POS_GENRE_LABEL", tempPos);
    oslDrawString(tempPos[0], tempPos[1], langGetString("GENRE"));
    skinGetPosition("POS_RATING_LABEL", tempPos);
    oslDrawString(tempPos[0], tempPos[1], langGetString("RATING"));
    skinGetPosition("POS_YEAR_LABEL", tempPos);
    oslDrawString(tempPos[0], tempPos[1], langGetString("YEAR"));
    skinGetPosition("POS_PLAYED_LABEL", tempPos);
    oslDrawString(tempPos[0], tempPos[1], langGetString("PLAYED"));
    skinGetPosition("POS_TIME_LABEL", tempPos);
    oslDrawString(tempPos[0], tempPos[1], langGetString("TIME"));
    skinGetPosition("POS_TRACK_LABEL", tempPos);
    oslDrawString(tempPos[0], tempPos[1], langGetString("TRACK"));

    skinGetColor("RGBA_TEXT", tempColor);
    skinGetColor("RGBA_TEXT_SHADOW", tempColorShadow);
    setFontStyle(fontNormal, defaultTextSize, RGBA(tempColor[0], tempColor[1], tempColor[2], tempColor[3]), RGBA(tempColorShadow[0], tempColorShadow[1], tempColorShadow[2], tempColorShadow[3]), INTRAFONT_ALIGN_LEFT);
    skinGetPosition("POS_TITLE_VALUE", tempPos);
    oslDrawString(tempPos[0], tempPos[1], info->title);
    skinGetPosition("POS_ARTIST_VALUE", tempPos);
    oslDrawString(tempPos[0], tempPos[1], info->artist);
    skinGetPosition("POS_ALBUM_VALUE", tempPos);
    oslDrawString(tempPos[0], tempPos[1], info->album);
    skinGetPosition("POS_GENRE_VALUE", tempPos);
    oslDrawString(tempPos[0], tempPos[1], info->genre);
    skinGetPosition("POS_YEAR_VALUE", tempPos);
    oslDrawString(tempPos[0], tempPos[1], info->year);
    skinGetPosition("POS_PLAYED_VALUE", tempPos);
    snprintf(buffer, sizeof(buffer), "%i", libEntry->played);
    oslDrawString(tempPos[0], tempPos[1], buffer);
    skinGetPosition("POS_RATING_VALUE", tempPos);
    drawRating(tempPos[0], tempPos[1], libEntry->rating);

    (*getTimeStringFunct)(timestring);
    if (!strlen(info->strLength))
        strcpy(info->strLength, "00:00:00");
    snprintf(buffer, sizeof(buffer), "%s / %s", timestring, info->strLength);
    skinGetPosition("POS_TIME_VALUE", tempPos);
    oslDrawString(tempPos[0], tempPos[1], buffer);
    skinGetPosition("POS_TRACK_VALUE", tempPos);
    oslDrawString(tempPos[0], tempPos[1], trackMessage);
    return 0;
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Draws cover art:
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
int drawCoverArt(){
    skinGetPosition("POS_COVERART", tempPos);
    if (coverArt)
        oslDrawImageXY(coverArt, tempPos[0], tempPos[1]);
    else
        oslDrawImageXY(noCoverArt, tempPos[0], tempPos[1]);
    return 0;
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Draws a file's specs
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
int drawFileSpecs(struct fileInfo *info){
    OSL_FONT *font = fontNormal;

    skinGetPosition("POS_FILE_SPECS_BKG", tempPos);
    oslDrawImageXY(fileSpecsBkg, tempPos[0], tempPos[1]);
    oslSetFont(font);
    skinGetColor("RGBA_LABEL_TEXT", tempColor);
    skinGetColor("RGBA_LABEL_TEXT_SHADOW", tempColorShadow);
    setFontStyle(fontNormal, defaultTextSize, RGBA(tempColor[0], tempColor[1], tempColor[2], tempColor[3]), RGBA(tempColorShadow[0], tempColorShadow[1], tempColorShadow[2], tempColorShadow[3]), INTRAFONT_ALIGN_LEFT);
    skinGetPosition("POS_MODE_LABEL", tempPos);
    oslDrawString(tempPos[0], tempPos[1], langGetString("MODE"));
    skinGetPosition("POS_BITRATE_LABEL", tempPos);
    oslDrawString(tempPos[0], tempPos[1], langGetString("BITRATE"));
    skinGetPosition("POS_SAMPLERATE_LABEL", tempPos);
    oslDrawString(tempPos[0], tempPos[1], langGetString("SAMPLERATE"));
    skinGetPosition("POS_FILE_FORMAT_LABEL", tempPos);
    oslDrawString(tempPos[0], tempPos[1], langGetString("FILE_FORMAT"));
    skinGetPosition("POS_EMPHASIS_LABEL", tempPos);
    oslDrawString(tempPos[0], tempPos[1], langGetString("EMPHASIS"));

    skinGetColor("RGBA_TEXT", tempColor);
    skinGetColor("RGBA_TEXT_SHADOW", tempColorShadow);
    setFontStyle(fontNormal, defaultTextSize, RGBA(tempColor[0], tempColor[1], tempColor[2], tempColor[3]), RGBA(tempColorShadow[0], tempColorShadow[1], tempColorShadow[2], tempColorShadow[3]), INTRAFONT_ALIGN_LEFT);
    skinGetPosition("POS_MODE_VALUE", tempPos);
    oslDrawString(tempPos[0], tempPos[1], info->mode);
    skinGetPosition("POS_BITRATE_VALUE", tempPos);
    snprintf(buffer, sizeof(buffer), "%3.3i [%3.3i] kbit", info->kbit, (int)(info->instantBitrate / 1000));
    oslDrawString(tempPos[0], tempPos[1], buffer);
    skinGetPosition("POS_SAMPLERATE_VALUE", tempPos);
    snprintf(buffer, sizeof(buffer), "%li Hz", info->hz);
    oslDrawString(tempPos[0], tempPos[1], buffer);
    skinGetPosition("POS_FILE_FORMAT_VALUE", tempPos);
    if (info->fileType == MP3_TYPE)
        snprintf(buffer, sizeof(buffer), "%s %s %s", fileTypeDescription[info->fileType], langGetString("LAYER"), info->layer);
    else if (info->fileType >= 0)
        snprintf(buffer, sizeof(buffer), "%s", fileTypeDescription[info->fileType]);
    oslDrawString(tempPos[0], tempPos[1], buffer);
    skinGetPosition("POS_EMPHASIS_VALUE", tempPos);
    oslDrawString(tempPos[0], tempPos[1], info->emphasis);
    return 0;
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Draws players's status
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
int drawPlayerStatus(){
    skinGetPosition("POS_PLAYER_STATUS_BKG", tempPos);
    oslDrawImageXY(playerStatusBkg, tempPos[0], tempPos[1]);
    oslSetFont(fontNormal);
    skinGetColor("RGBA_LABEL_TEXT", tempColor);
    skinGetColor("RGBA_LABEL_TEXT_SHADOW", tempColorShadow);
    setFontStyle(fontNormal, defaultTextSize, RGBA(tempColor[0], tempColor[1], tempColor[2], tempColor[3]), RGBA(tempColorShadow[0], tempColorShadow[1], tempColorShadow[2], tempColorShadow[3]), INTRAFONT_ALIGN_LEFT);
    skinGetPosition("POS_SLEEP_MODE_LABEL", tempPos);
    oslDrawString(tempPos[0], tempPos[1], langGetString("SLEEP_MODE"));
    skinGetPosition("POS_PLAY_MODE_LABEL", tempPos);
    oslDrawString(tempPos[0], tempPos[1], langGetString("PLAY_MODE"));
    skinGetPosition("POS_STATUS_LABEL", tempPos);
    oslDrawString(tempPos[0], tempPos[1], langGetString("STATUS"));
    skinGetPosition("POS_VOLUME_BOOST_LABEL", tempPos);
    oslDrawString(tempPos[0], tempPos[1], langGetString("VOLUME_BOOST"));
    skinGetPosition("POS_EQUALIZER_LABEL", tempPos);
    oslDrawString(tempPos[0], tempPos[1], langGetString("EQUALIZER"));

    skinGetColor("RGBA_TEXT", tempColor);
    skinGetColor("RGBA_TEXT_SHADOW", tempColorShadow);
    setFontStyle(fontNormal, defaultTextSize, RGBA(tempColor[0], tempColor[1], tempColor[2], tempColor[3]), RGBA(tempColorShadow[0], tempColorShadow[1], tempColorShadow[2], tempColorShadow[3]), INTRAFONT_ALIGN_LEFT);
    skinGetPosition("POS_SLEEP_MODE_VALUE", tempPos);
    oslDrawString(tempPos[0], tempPos[1], sleepModeDesc[userSettings->sleepMode]);
    skinGetPosition("POS_PLAY_MODE_VALUE", tempPos);
    oslDrawString(tempPos[0], tempPos[1], playModeDesc[userSettings->playMode]);
    skinGetPosition("POS_STATUS_VALUE", tempPos);
    int currentSpeed = (*getPlayingSpeedFunct)();
    if (!currentSpeed)
        oslDrawString(tempPos[0], tempPos[1], playerStatusDesc[playerStatus+1]);
    else{
        if (currentSpeed > 0)
            snprintf(buffer, sizeof(buffer), "%s %ix", playerStatusDesc[playerStatus+1], currentSpeed + 1);
        else
            snprintf(buffer, sizeof(buffer), "%s %ix", playerStatusDesc[playerStatus+1], currentSpeed - 1);
        oslDrawString(tempPos[0], tempPos[1], buffer);
    }

    skinGetPosition("POS_VOLUME_BOOST_VALUE", tempPos);
    if (!MAX_VOLUME_BOOST)
        oslDrawString(tempPos[0], tempPos[1], langGetString("NOT_SUPPORTED"));
    else{
        snprintf(buffer, sizeof(buffer), "%i", userSettings->volumeBoost);
        oslDrawString(tempPos[0], tempPos[1], buffer);
    }

    skinGetPosition("POS_EQUALIZER_VALUE", tempPos);
    if (!(*isFilterSupportedFunct)())
        oslDrawString(tempPos[0], tempPos[1], langGetString("NOT_SUPPORTED"));
    else
        oslDrawString(tempPos[0], tempPos[1], tEQ.name);
    return 0;
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Draws progress bar:
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
int drawProgressBar(float perc){
    skinGetPosition("POS_PROGRESS", tempPos);
    oslDrawImageXY(progressBkg, tempPos[0], tempPos[1]);
    OSL_IMAGE *imageTile = oslCreateImageTile(progress, 0, 0, (int)((float)progress->sizeX / 100.00 * perc), progress->sizeY);
    oslDrawImageXY(imageTile, tempPos[0], tempPos[1] + 2);
    oslDeleteImage(imageTile);
    imageTile = NULL;
    //progress->stretchX = (int)((float)progress->sizeX / 100.00 * perc);
    //oslDrawImageXY(progress, tempPos[0], tempPos[1] + 2);
    return 0;
}


///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Draws the player:
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
int drawPlayer(int status, struct libraryEntry *libEntry, char *trackMessage){
	struct fileInfo *info = NULL;

	oslStartDrawing();
	drawCommonGraphics();
	info = (*getInfoFunct)();
	drawFileInfo(info, libEntry, trackMessage);
	drawFileSpecs(info);
	drawPlayerStatus();
	drawProgressBar((*getPercentageFunct)());
	drawCoverArt();
	if (status == STATUS_HELP)
		drawHelp("PLAYER");
    else if(status == STATUS_CONFIRM_BOOKMARK)
        drawConfirm(langGetString("CONFIRM_BOOKMARK_TITLE"), langGetString("CONFIRM_BOOKMARK"));
	oslEndDrawing();
	return 0;
}


///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Ask to confirm the creation of a bookmark and exit
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
int confirmBookmark(struct libraryEntry *libEntry, char *trackMessage, int index)
{
    struct bookmark bookm;
    char bookmarkName[264] = "";
    int skip = 0;

    snprintf(bookmarkName, sizeof(bookmarkName), "%s%s", userSettings->ebootPath, "bookmark.bkm");

    while(!osl_quit)
    {
        if (!skip)
            drawPlayer(STATUS_CONFIRM_BOOKMARK, libEntry, trackMessage);
        oslEndFrame();
        skip = oslSyncFrame();

        oslReadKeys();
        if(osl_pad.released.cross)
        {
            strcpy(bookm.fileName, libEntry->shortpath);
            bookm.playListIndex = index;
            if (getFilePositionFunct != NULL)
                bookm.position = (*getFilePositionFunct)();
            else
                bookm.position = 0;
            saveBookmark(bookmarkName, &bookm);
            snprintf(bookmarkName, sizeof(bookmarkName), "%s%s", userSettings->ebootPath, "bookmark.m3u");
            M3U_save(bookmarkName);
            return 1;
        }
        else if(osl_pad.released.circle)
        {
            break;
        }
    }

    oslReadKeys();
    return 0;
}


///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Play a single file:
//		 Returns PLAYER_NEXT     if user pressed NEXT
//				 PLAYER_END      if song ended
//				 PLAYER_PREVIOUS if user pressed PREVIOUS
//				 PLAYER_STOP     if user pressed STOP
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
int playFile(char *fileName, char *trackMessage, int index, double startFilePos){
    int retValue = PLAYER_END;
    struct fileInfo tagInfo;
	struct fileInfo *info = NULL;
    struct libraryEntry libEntry;
    int currentSpeed = 0;
    int clock = 0;
    float lastPercentage = 0.0f;
    int ratingChangedUpDown = 0;
    int ratingChangedCross = 0;
    int helpShown = 0;
    int flagExit = 0;
    int status = STATUS_NORMAL;
	int headphone = sceHprmIsHeadphoneExist();
	int skip = 0;
	OSL_IMAGE* tmpCoverArt = NULL;

	cpuBoost();

    if (userSettings->displayStatus){
        oslStartDrawing();
        drawCommonGraphics();
    	oslEndDrawing();
        oslEndFrame();
    	oslSyncFrame();
    }

    setVolume(0,0x8000);
    if (setAudioFunctions(fileName, userSettings->MP3_ME)){
        snprintf(buffer, sizeof(buffer), "Unknown audio format for file\n%s\n\nContinue with next file?", fileName);
        int response = errorMessageBox(buffer, 1);
        oslReadKeys();
		cpuRestore();
		if (response == OSL_MB_YES)
			return PLAYER_END;
		else
			return PLAYER_STOP;
	}

    //Tipo di volume boost:
    if (strcmp(userSettings->BOOST, "OLD") == 0)
        (*setVolumeBoostTypeFunct)("OLD");
    else
        (*setVolumeBoostTypeFunct)("NEW");

    if (userSettings->volumeBoost > MAX_VOLUME_BOOST)
        userSettings->volumeBoost = MAX_VOLUME_BOOST;
    if (userSettings->volumeBoost < MIN_VOLUME_BOOST)
        userSettings->volumeBoost = MIN_VOLUME_BOOST;

    //Equalizzatore:
    if (!(*isFilterSupportedFunct)())
    {
        userSettings->currentEQ = 0;
    }
    else
    {
        tEQ = EQ_getIndex(userSettings->currentEQ);
        (*setFilterFunct)(tEQ.filter, 1);
    }

    if (!userSettings->currentEQ)
      (*disableFilterFunct)();
    else
      (*enableFilterFunct)();

    (*initFunct)(0);
    (*setVolumeBoostFunct)(userSettings->volumeBoost);

    //Read TAG and Media Library:
    tagInfo = (*getTagInfoFunct)(fileName);
    getCovertArtImageName(fileName, &tagInfo);

    char whereCond[312] = "";
    char fixedName[264] = "";
    strcpy(fixedName, fileName);
    ML_fixStringField(fixedName);
    snprintf(whereCond, sizeof(whereCond), "path = upper('%s')", fixedName);
    int update = ML_queryDB(whereCond, "path", 0, 1, MLresult);
    if (update > 0)
        libEntry = MLresult[0];
    else
    {
        //Look for an entry with same artist, album, title but with dead link:
        char fixedArtist[260] = "";
        char fixedAlbum[260] = "";
        char fixedTitle[260] = "";

        strcpy(fixedArtist, tagInfo.artist);
        ML_fixStringField(fixedArtist);
        strcpy(fixedAlbum, tagInfo.album);
        ML_fixStringField(fixedAlbum);
        strcpy(fixedTitle, tagInfo.title);
        ML_fixStringField(fixedTitle);
        snprintf(whereCond, sizeof(whereCond), "artist = '%s' and album = '%s' and title = '%s'", fixedArtist, fixedAlbum, fixedTitle);
        update = ML_queryDB(whereCond, "path", 0, 1, MLresult);
        if (update > 0)
        {
            if (fileExists(MLresult[0].shortpath) < 0)
            {
                libEntry = MLresult[0];
                strcpy(libEntry.shortpath, fileName);
            }
            else
            {
                ML_clearEntry(&libEntry);
                update = 0;
            }
        }
        else
            ML_clearEntry(&libEntry);
    }

    //Save temp coverart file:
    coverArt = NULL;
    tmpCoverArt = NULL;
    if (tagInfo.encapsulatedPictureOffset && tagInfo.encapsulatedPictureLength <= MAX_IMAGE_DIMENSION){
		int in = sceIoOpen(fileName, PSP_O_RDONLY, 0777);
        if (tagInfo.encapsulatedPictureType == JPEG_IMAGE)
            snprintf(buffer, sizeof(buffer), "%scoverart.jpg", userSettings->ebootPath);
        else if (tagInfo.encapsulatedPictureType == PNG_IMAGE)
            snprintf(buffer, sizeof(buffer), "%scoverart.png", userSettings->ebootPath);
		int out = sceIoOpen(buffer, PSP_O_WRONLY | PSP_O_CREAT | PSP_O_TRUNC, 0777);

		int buffSize = 128*1024;
        u8 *cover = (unsigned char *) malloc(buffSize);
        if (cover != NULL)
         {
            sceIoLseek(in, tagInfo.encapsulatedPictureOffset, PSP_SEEK_SET);
            int remaining = tagInfo.encapsulatedPictureLength;
            while (remaining > 0){
                if (remaining < buffSize)
                    buffSize = remaining;
                int write = sceIoRead(in, cover, buffSize);
                sceIoWrite(out, cover, write);
                remaining -= write;
            }
            free(cover);
         }
		sceIoClose(out);
		sceIoClose(in);
		if (tagInfo.encapsulatedPictureType == JPEG_IMAGE)
		{
            tmpCoverArt = oslLoadImageFileJPG(buffer, OSL_IN_RAM | OSL_SWIZZLED, OSL_PF_8888);
		}
        else if (tagInfo.encapsulatedPictureType == PNG_IMAGE)
            tmpCoverArt = oslLoadImageFilePNG(buffer, OSL_IN_RAM | OSL_SWIZZLED, OSL_PF_8888);
        sceIoRemove(buffer);
    }else if (strlen(tagInfo.coverArtImageName))
        tmpCoverArt = oslLoadImageFileJPG(tagInfo.coverArtImageName, OSL_IN_RAM | OSL_SWIZZLED, OSL_PF_8888);

    if (tmpCoverArt){
    	int coverArtWidth  = skinGetParam("COVERART_WIDTH");
    	int coverArtHeight = skinGetParam("COVERART_HEIGHT");

		coverArt = oslScaleImageCreate(tmpCoverArt, OSL_IN_RAM | OSL_SWIZZLED, coverArtWidth, coverArtHeight, OSL_PF_8888);
		oslDeleteImage(tmpCoverArt);
		tmpCoverArt = NULL;

        coverArt->stretchX = coverArtWidth;
        coverArt->stretchY = coverArtHeight;
    }

    playerStatus = -1;
    if (userSettings->displayStatus){
        oslStartDrawing();
        drawCommonGraphics();
        drawFileInfo(&tagInfo, &libEntry, trackMessage);
        drawFileSpecs(&tagInfo);
        drawPlayerStatus();
        drawProgressBar(0.0);
        drawCoverArt();
    	oslEndDrawing();
        oslEndFrame();
    	oslSyncFrame();
    }

    //Apro il file:
    int retLoad = (*loadFunct)(fileName);
    if (retLoad != OPENING_OK){
		char tempDir[256] = "";
		getcwd(tempDir, 256);
		snprintf(buffer, sizeof(buffer), "Error %i opening file:\n%s\nCwd: %s\n\nContinue with next file?", retLoad, fileName, tempDir);
        int response = errorMessageBox(buffer, 1);
        (*endFunct)();
        unsetAudioFunctions();
		//Unload images:
		if (coverArt){
			oslDeleteImage(coverArt);
			coverArt = NULL;
		}
		oslReadKeys();

		if (response == OSL_MB_YES)
			return PLAYER_END;
		else
			return PLAYER_STOP;
    }
    info = (*getInfoFunct)();

    //Update media library info:
    getExtension(fileName, libEntry.extension, 4);
    strcpy(libEntry.album, info->album);
    strcpy(libEntry.artist, info->artist);
    strcpy(libEntry.title, info->title);
    strcpy(libEntry.genre, info->genre);
    strcpy(libEntry.year, info->year);
    if (update <= 0)
        strcpy(libEntry.path, fileName);
    libEntry.seconds = info->length;
    libEntry.bitrate = info->kbit;
    libEntry.samplerate = info->hz;
    libEntry.tracknumber = atoi(info->trackNumber);

	cpuRestore();

    //Imposto il clock:
    if (userSettings->CLOCK_AUTO){
		setBusClock(userSettings->BUS);
        if (userSettings->displayStatus)
            setCpuClock(info->defaultCPUClock);
        else
            setCpuClock(info->defaultCPUClock - userSettings->CLOCK_DELTA_ECONOMY_MODE);
        clock = info->defaultCPUClock;
    }else
        clock = getCpuClock();

    if (setFilePositionFunct != NULL && startFilePos > 0)
        (*setFilePositionFunct)(startFilePos);

    time_t startTime;
    sceKernelLibcTime(&startTime);
    (*playFunct)();
    playerStatus = 1;

    flagExit = 0;
    while(!osl_quit && !flagExit){
        if (userSettings->displayStatus && !skip)
			drawPlayer(status, &libEntry, trackMessage);

        if (!userSettings->displayStatus){
			scePowerTick(0);
			if (getBrightness())
				userSettings->displayStatus = 1;
		}else{
			oslEndFrame();
			skip = oslSyncFrame();
		}

		//Metto in pausa se sono state staccate le cuffie:
		if (headphone && !sceHprmIsHeadphoneExist()){
			if (playerStatus == 1){
				(*pauseFunct)();
				playerStatus = !playerStatus;
				if (userSettings->CLOCK_AUTO)
					setCpuClock(CLOCK_WHEN_PAUSE);
			}
		}
		headphone = sceHprmIsHeadphoneExist();

		//Spengo il display se HOLD:
        if (osl_pad.pressed.hold && userSettings->displayStatus){
			//Spengo il display:
			userSettings->curBrightness = getBrightness();
			if (!info->needsME){
				cpuBoost();
				fadeDisplay(0, DISPLAY_FADE_TIME);
				cpuRestore();
			}else
				fadeDisplay(0, DISPLAY_FADE_TIME);
			displayDisable();
			imposeSetHomePopup(0);
			userSettings->displayStatus = 0;
			//Downclock:
			if (userSettings->CLOCK_DELTA_ECONOMY_MODE)
				setCpuClock(clock - userSettings->CLOCK_DELTA_ECONOMY_MODE);
		}

		//Accendo il display al release di hold:
		if (osl_pad.released.hold && !userSettings->displayStatus){
			//Accendo il display:
			if (userSettings->CLOCK_DELTA_ECONOMY_MODE)
				setCpuClock(clock);
			drawPlayer(status, &libEntry, trackMessage);
			oslEndFrame();
			skip = oslSyncFrame();
			displayEnable();
			setBrightness(0);
			imposeSetHomePopup(1);
			if (!info->needsME){
				cpuBoost();
				fadeDisplay(userSettings->curBrightness, DISPLAY_FADE_TIME);
				cpuRestore();
			}else
				fadeDisplay(userSettings->curBrightness, DISPLAY_FADE_TIME);
			userSettings->displayStatus = 1;
		}

        lastPercentage = (*getPercentageFunct)();
        oslReadKeys();
        oslReadRemoteKeys();

        if (status == STATUS_HELP){
            if (osl_pad.released.cross || osl_pad.released.circle)
                status = STATUS_NORMAL;
        }else{
            if (osl_pad.held.L && osl_pad.released.circle){
                if (currentSpeed)
                    (*setPlayingSpeedFunct)(0);
                if (playerStatus)
                    (*pauseFunct)();

                if (confirmBookmark(&libEntry, trackMessage, index))
                {
                    flagExit = 1;
                    osl_quit = 1;
                    if (userSettings->shutDownAfterBookmark)
                        userSettings->shutDown = 1;
                }
                else
                {
                    if (playerStatus)
                        (*pauseFunct)();
                }
            }else if (osl_pad.held.L && osl_pad.held.R){
                if (userSettings->displayStatus){
                    helpShown = 1;
                    status = STATUS_HELP;
                }
            }else if (osl_pad.held.cross && osl_pad.held.up){
                if (++libEntry.rating > ML_MAX_RATING)
                    libEntry.rating = ML_MAX_RATING;
                ratingChangedUpDown = 1;
                ratingChangedCross= 1;
                sceKernelDelayThread(userSettings->KEY_AUTOREPEAT_PLAYER*15000);
            }else if (osl_pad.held.cross && osl_pad.held.down){
                if (--libEntry.rating < 0)
                    libEntry.rating = 0;
                ratingChangedUpDown = 1;
                ratingChangedCross= 1;
                sceKernelDelayThread(userSettings->KEY_AUTOREPEAT_PLAYER*15000);
            }else if (osl_pad.released.circle){
                (*endFunct)();
                playerStatus = 0;
                retValue = PLAYER_STOP;
                flagExit = 1;
				userSettings->sleepMode = SLEEP_NONE;
            }else if (osl_pad.released.square){
        		if (playerStatus == 1){
                    userSettings->muted = !userSettings->muted;
        			(*setMuteFunct)(userSettings->muted);
                }
            }else if (osl_pad.released.R || osl_remote.released.rmforward){
                if (helpShown)
                    helpShown = 0;
                else{
                    if (userSettings->FADE_OUT)
                        (*fadeOutFunct)(0.3);
                    (*endFunct)();
                    retValue = PLAYER_NEXT;
                    flagExit = 1;
                }
            }else if (osl_pad.released.L || osl_remote.released.rmback){
                if (helpShown)
                    helpShown = 0;
                else{
                    if (userSettings->FADE_OUT)
                        (*fadeOutFunct)(0.3);
                    (*endFunct)();
                    retValue = PLAYER_PREVIOUS;
                    flagExit = 1;
                }
            }else if (osl_pad.released.cross || osl_remote.released.rmplaypause){
                if (ratingChangedCross)
                    ratingChangedCross = 0;
                else{
                    currentSpeed = (*getPlayingSpeedFunct)();
                    if (currentSpeed){
                        (*setPlayingSpeedFunct)(0);
						if (userSettings->CLOCK_AUTO){
						    setBusClock(userSettings->BUS);
							setCpuClock(getCpuClock() - FFWDREW_CPU_BOOST);
						}
                    }else{
                        (*pauseFunct)();
                        playerStatus = !playerStatus;
						if (userSettings->CLOCK_AUTO){
							if (playerStatus == 1)
								setCpuClock(info->defaultCPUClock);
							else
								setCpuClock(CLOCK_WHEN_PAUSE);
						}
                    }
                }
            }else if (osl_pad.released.up){
                if (ratingChangedUpDown)
                    ratingChangedUpDown = 0;
                else
            		if (userSettings->volumeBoost < MAX_VOLUME_BOOST)
            			(*setVolumeBoostFunct)(++userSettings->volumeBoost);
            }else if (osl_pad.released.down){
                if (ratingChangedUpDown)
                    ratingChangedUpDown = 0;
                else
            		if (userSettings->volumeBoost > MIN_VOLUME_BOOST)
            			(*setVolumeBoostFunct)(--userSettings->volumeBoost);
            }else if (osl_pad.released.right && playerStatus == 1){
                currentSpeed = (*getPlayingSpeedFunct)();
				if (currentSpeed < MAX_PLAYING_SPEED){
					if (currentSpeed < 0)
						currentSpeed = 0;
					else if (currentSpeed < 9)
						currentSpeed = 9;
					else if (currentSpeed < 29)
						currentSpeed = 29;
					else
						currentSpeed = MAX_PLAYING_SPEED;

					if (userSettings->CLOCK_AUTO){
						if (currentSpeed == 9){
						    setBusClock(userSettings->BUS);
							setCpuClock(getCpuClock() + FFWDREW_CPU_BOOST);
						}else if (currentSpeed == 0){
						    setBusClock(userSettings->BUS);
							setCpuClock(getCpuClock() - FFWDREW_CPU_BOOST);
						}
					}
					(*setPlayingSpeedFunct)(currentSpeed);
				}
            }else if (osl_pad.released.left && playerStatus == 1){
                currentSpeed = (*getPlayingSpeedFunct)();
				if (currentSpeed > MIN_PLAYING_SPEED){
					if (currentSpeed > 0)
						currentSpeed = 0;
					else if (currentSpeed > -9)
						currentSpeed = -9;
					else if (currentSpeed > -29)
						currentSpeed = -29;
					else
						currentSpeed = MIN_PLAYING_SPEED;

					if (userSettings->CLOCK_AUTO){
						if (currentSpeed == -9){
						    setBusClock(userSettings->BUS);
							setCpuClock(getCpuClock() + FFWDREW_CPU_BOOST);
						}else if (currentSpeed == 0){
						    setBusClock(userSettings->BUS);
							setCpuClock(getCpuClock() - FFWDREW_CPU_BOOST);
						}
					}
					(*setPlayingSpeedFunct)(currentSpeed);
				}
            }else if (osl_pad.analogY < -ANALOG_SENS && !osl_pad.pressed.hold){
                if (clock < 222)
                    setCpuClock(++clock);
                sceKernelDelayThread(100000);
            }else if (osl_pad.analogY > ANALOG_SENS && !osl_pad.pressed.hold){
                if (clock > getMinCPUClock())
                    setCpuClock(--clock);
                sceKernelDelayThread(100000);
            }else if (osl_pad.released.triangle){
        		//Change sleep mode:
        		if (++userSettings->sleepMode > SLEEP_120)
        			userSettings->sleepMode = 0;
				if (userSettings->sleepMode >= SLEEP_30)
					sceRtcGetCurrentTick(&sleepStartTime);
    		}else if(osl_pad.released.note){
    			//Cambio equalizzatore:
                if ((*isFilterSupportedFunct)()){
                    if (++userSettings->currentEQ >= EQ_getEqualizersNumber()){
                        userSettings->currentEQ = 0;
                        (*disableFilterFunct)();
                    }else
                        (*enableFilterFunct)();
                    tEQ = EQ_getIndex(userSettings->currentEQ);
                    (*setFilterFunct)(tEQ.filter, 1);
                }
            }else if (osl_pad.released.select){
    				//Change playing mode:
    				if (++userSettings->playMode > MODE_SHUFFLE_REPEAT)
    					userSettings->playMode = MODE_NORMAL;
            }else if (osl_pad.released.start){
        		if (userSettings->displayStatus){
        			//Spengo il display:
                    userSettings->curBrightness = getBrightness();
					if (!info->needsME){
						cpuBoost();
						fadeDisplay(0, DISPLAY_FADE_TIME);
						cpuRestore();
					}else
						fadeDisplay(0, DISPLAY_FADE_TIME);
					displayDisable();
        			imposeSetHomePopup(0);
        			userSettings->displayStatus = 0;
        			//Downclock:
                    if (userSettings->CLOCK_DELTA_ECONOMY_MODE)
                        setCpuClock(clock - userSettings->CLOCK_DELTA_ECONOMY_MODE);
        		} else {
        			//Accendo il display:
                    if (userSettings->CLOCK_DELTA_ECONOMY_MODE)
            			setCpuClock(clock);
					drawPlayer(status, &libEntry, trackMessage);
					oslEndFrame();
					skip = oslSyncFrame();
					displayEnable();
        			setBrightness(0);
        			imposeSetHomePopup(1);
					if (!info->needsME){
						cpuBoost();
						fadeDisplay(userSettings->curBrightness, DISPLAY_FADE_TIME);
						cpuRestore();
					}else
						fadeDisplay(userSettings->curBrightness, DISPLAY_FADE_TIME);
        			userSettings->displayStatus = 1;
        		}
            }
        }

        //Controllo se la riproduzione è finita:
        if (!flagExit && (*endOfStreamFunct)() == 1) {
            (*endFunct)();
            retValue = PLAYER_END;
            flagExit = 1;
        }
    }

	cpuBoost();
	//Srobbler LOG:
    if (userSettings->SCROBBLER && strlen(info->title)){
        if (lastPercentage >= 50)
            SCROBBLER_addTrack(*info, info->length, "L", startTime);
        else
        	SCROBBLER_addTrack(*info, info->length, "S", startTime);
    }

    unsetAudioFunctions();

    //Unload images:
    if (coverArt){
        oslDeleteImage(coverArt);
        coverArt = NULL;
    }

    //Update media Library info:
    if (lastPercentage >= 50)
        libEntry.played++;
    if (update > 0)
        ML_updateEntry(libEntry, fileName);
    else
        ML_addEntry(libEntry);

    //Sleep mode:
    if (userSettings->sleepMode == SLEEP_TRACK){
		osl_quit = 1;
		userSettings->shutDown = 1;
    }
    else if ((userSettings->sleepMode >= SLEEP_30) && (userSettings->sleepMode <= SLEEP_120)) {
    	//Not sure whether the resolution is dependent on the current clock speed, so safer to get 'res' everytime:
    	u32 res = sceRtcGetTickResolution();
		u64 currentTime = 0;
		sceRtcGetCurrentTick(&currentTime);
		u32 diff = (u32)((currentTime - sleepStartTime)/((float)res));

    	if (((userSettings->sleepMode == SLEEP_30) && (diff >= 30*60)) ||\
			((userSettings->sleepMode == SLEEP_60) && (diff >= 60*60)) ||\
			((userSettings->sleepMode == SLEEP_90) && (diff >= 90*60)) ||\
			((userSettings->sleepMode == SLEEP_120) && (diff >= 120*60))) {
			userSettings->sleepMode = SLEEP_NONE;
			scePowerRequestSuspend(); //Suspend is probably better than Standby over here as they might want to turn it on again and continue listening.
			sceKernelDelayThread(3000*1000); //Makes it Suspend immediately right here before the next file handle is opened and invalidated due to Suspend.
    	}
    }
	cpuRestore();
    return retValue;
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Get a random track:
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
int randomTrack(int max, unsigned int played, unsigned int used[]){
	int random;
	int found = 0;
	if (played == max){
		return(-1);
	}

	found = 1;
	while (found == 1){
		//random = (int)oslRandf(0, max);
        random = rand()%max;
		//Controllo se l'ho già suonata:
		int i;
		found = 0;
		for (i=0; i<played; i++){
			if (used[i] == random){
				found = 1;
				break;
			}
		}
	}
	return(random);
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Play playlist:
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
int playPlaylist(struct M3U_playList *playList, int startIndex, double startFilePos){
	int songCount = M3U_getSongCount();
	unsigned int playedTracks[songCount];
	unsigned int playedTracksNumber = 0;
	unsigned int currentTrack = 0;
    struct M3U_songEntry *song;
    int i = 0;
    char message[20] = "";

	//Chdir for playlist with relative paths:
	/*if (strlen(playList->fileName))
	{
		char onlyDir[256] = "";
		strcpy(onlyDir, playList->fileName);
		directoryUp(onlyDir);
		sceIoChdir(onlyDir);
	}*/

    ML_startTransaction();

    if (startIndex >= 0)
        i = startIndex;
	else if (userSettings->playMode == MODE_SHUFFLE || userSettings->playMode == MODE_SHUFFLE_REPEAT)
		i = randomTrack(songCount, playedTracksNumber, playedTracks);
	else
		i = 0;

	currentTrack = 0;
	while(!osl_quit){
		song = M3U_getSong(i);
		snprintf(message, sizeof(message), "%i / %i", i + 1, songCount);
		int playerReturn = playFile(song->fileName, message, i, startFilePos);
        startFilePos = 0;
		//Played tracks:
		int found = 0;
		int ci = 0;
		found = 0;
		for (ci=0; ci<playedTracksNumber; ci++){
			if (playedTracks[ci] == i){
				found = 1;
				break;
			}
		}
		if (!found){
			playedTracks[playedTracksNumber] = i;
			playedTracksNumber++;
		}
		if (playerReturn == PLAYER_STOP){
			break;
		}else if (playerReturn == PLAYER_NEXT || playerReturn == PLAYER_END){
            //Controllo la ripetizione della traccia:
            if (playerReturn == PLAYER_END && userSettings->playMode == MODE_REPEAT)
                continue;
			//Controllo se sono in shuffle:
			if (userSettings->playMode == MODE_SHUFFLE || userSettings->playMode == MODE_SHUFFLE_REPEAT){
				if (currentTrack < playedTracksNumber - 1 && playerReturn == PLAYER_NEXT){
					i = playedTracks[++currentTrack];
				}else{
					i = randomTrack(songCount, playedTracksNumber, playedTracks);
					if (i == -1){
                        if (userSettings->playMode == MODE_SHUFFLE_REPEAT){
                            playedTracksNumber = 0;
                            memset(playedTracks, 0, songCount);
                            i = randomTrack(songCount, playedTracksNumber, playedTracks);
                        }else
						  break;
					}
					currentTrack++;
				}
			}else{
				//Controllo se è finita la playlist e non sono in repeat:
				if ((playerReturn == PLAYER_END) & (i == songCount - 1) & (userSettings->playMode != MODE_REPEAT_ALL)){
					break;
				}
				//Altrimenti passo al file successivo:
				i++;
				if (i > songCount - 1){
					i = 0;
				}
				currentTrack = i;
			}
		}else if (playerReturn == PLAYER_PREVIOUS){
			//Controllo se sono in shuffle:
			if (userSettings->playMode == MODE_SHUFFLE || userSettings->playMode == MODE_SHUFFLE_REPEAT){
				if (currentTrack){
					i = playedTracks[--currentTrack];
				}
			}else{
				i--;
				if (i < 0){
					i = songCount - 1;
				}
				currentTrack = i;
			}
		}
    }

    ML_commitTransaction();
	if (!userSettings->displayStatus){
        oslStartDrawing();
        oslClearScreen(RGBA(0, 0, 0, 255));
    	oslEndDrawing();
        oslEndFrame();
    	oslSyncFrame();
    	displayEnable();
		setBrightness(0);
    	imposeSetHomePopup(1);
        fadeDisplay(userSettings->curBrightness, DISPLAY_FADE_TIME);
        userSettings->displayStatus = 1;
    }

    //Sleep mode:
    if (userSettings->sleepMode == SLEEP_PLAYLIST){
      osl_quit = 1;
      userSettings->shutDown = 1;
    }
    return 0;
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Play a directory:
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
int playDirectory(char *dirName, char *dirNameShort, char *startFile){
    struct opendir_struct directory;
    int startIndex = -1;
    char onlyName[264] = "";

    M3U_clear();
	cpuBoost();
    char *result = opendir_open(&directory, dirName, dirNameShort, fileExt, fileExtCount - 1, 0);
	cpuRestore();
    if (!result){
        getFileName(startFile, onlyName);
        int i = 0;
        for (i=0; i<directory.number_of_directory_entries; i++){
            if (dirName[strlen(dirName)-1] != '/')
                snprintf(buffer, sizeof(buffer), "%s/%s", dirNameShort, directory.directory_entry[i].d_name);
            else
                snprintf(buffer, sizeof(buffer), "%s%s", dirNameShort, directory.directory_entry[i].d_name);
            if (!strcmp(directory.directory_entry[i].d_name, onlyName))
                startIndex = i;
            M3U_addSong(buffer, 0, buffer);
        }
        opendir_close(&directory);
		if (M3U_getSongCount())
	        playPlaylist(M3U_getPlaylist(), startIndex, 0);
    }
    M3U_clear();
    return 0;
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Player:
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
int gui_player(){
    SceIoStat stat;
    sceIoGetstat(userSettings->selectedBrowserItemShort, &stat);

    //Load images:
    snprintf(buffer, sizeof(buffer), "%s/nocoverart.png", userSettings->skinImagesPath);
    noCoverArt = oslLoadImageFilePNG(buffer, OSL_IN_RAM | OSL_SWIZZLED, OSL_PF_8888);
    if (!noCoverArt)
        errorLoadImage(buffer);

    snprintf(buffer, sizeof(buffer), "%s/fileinfobkg.png", userSettings->skinImagesPath);
    fileInfoBkg = oslLoadImageFilePNG(buffer, OSL_IN_RAM | OSL_SWIZZLED, OSL_PF_8888);
    if (!fileInfoBkg)
        errorLoadImage(buffer);

    snprintf(buffer, sizeof(buffer), "%s/filespecsbkg.png", userSettings->skinImagesPath);
    fileSpecsBkg = oslLoadImageFilePNG(buffer, OSL_IN_RAM | OSL_SWIZZLED, OSL_PF_8888);
    if (!fileSpecsBkg)
        errorLoadImage(buffer);

    snprintf(buffer, sizeof(buffer), "%s/playerstatusbkg.png", userSettings->skinImagesPath);
    playerStatusBkg = oslLoadImageFilePNG(buffer, OSL_IN_RAM | OSL_SWIZZLED, OSL_PF_8888);
    if (!playerStatusBkg)
        errorLoadImage(buffer);

    snprintf(buffer, sizeof(buffer), "%s/progressbkg.png", userSettings->skinImagesPath);
    progressBkg = oslLoadImageFilePNG(buffer, OSL_IN_RAM | OSL_SWIZZLED, OSL_PF_8888);
    if (!progressBkg)
        errorLoadImage(buffer);

    snprintf(buffer, sizeof(buffer), "%s/progress.png", userSettings->skinImagesPath);
    progress = oslLoadImageFilePNG(buffer, OSL_IN_RAM | OSL_SWIZZLED, OSL_PF_8888);
    if (!progress)
        errorLoadImage(buffer);

    playerRetValue = userSettings->previousMode;

    //Load lang strings:
    strncpy(sleepModeDesc[0], langGetString("SLEEP_MODE_0"), 100);
	sleepModeDesc[0][100] = '\0';
    strncpy(sleepModeDesc[1], langGetString("SLEEP_MODE_1"), 100);
	sleepModeDesc[1][100] = '\0';
	strncpy(sleepModeDesc[2], langGetString("SLEEP_MODE_2"), 100);
	sleepModeDesc[2][100] = '\0';
	strncpy(sleepModeDesc[3], langGetString("SLEEP_MODE_3"), 100);
	sleepModeDesc[3][100] = '\0';
    strncpy(sleepModeDesc[4], langGetString("SLEEP_MODE_4"), 100);
	sleepModeDesc[4][100] = '\0';
	strncpy(sleepModeDesc[5], langGetString("SLEEP_MODE_5"), 100);
	sleepModeDesc[5][100] = '\0';
	strncpy(sleepModeDesc[6], langGetString("SLEEP_MODE_6"), 100);
	sleepModeDesc[6][100] = '\0';

    strncpy(playModeDesc[0], langGetString("PLAY_MODE_0"), 100);
	playModeDesc[0][100] = '\0';
    strncpy(playModeDesc[1], langGetString("PLAY_MODE_1"), 100);
	playModeDesc[1][100] = '\0';
	strncpy(playModeDesc[2], langGetString("PLAY_MODE_2"), 100);
	playModeDesc[2][100] = '\0';
    strncpy(playModeDesc[3], langGetString("PLAY_MODE_3"), 100);
	playModeDesc[3][100] = '\0';
    strncpy(playModeDesc[4], langGetString("PLAY_MODE_4"), 100);
	playModeDesc[4][100] = '\0';

    strncpy(playerStatusDesc[0], langGetString("PLAYER_STATUS_0"), 100);
	playerStatusDesc[0][100] = '\0';
    strncpy(playerStatusDesc[1], langGetString("PLAYER_STATUS_1"), 100);
	playerStatusDesc[1][100] = '\0';
	strncpy(playerStatusDesc[2], langGetString("PLAYER_STATUS_2"), 100);
	playerStatusDesc[2][100] = '\0';

    oslSetKeyAutorepeatInit(userSettings->KEY_AUTOREPEAT_PLAYER);
    oslSetKeyAutorepeatInterval(userSettings->KEY_AUTOREPEAT_PLAYER);
    oslSetKeyAnalogToDPad(0);

    char dir[264] = "";
    char dirShort[264] = "";
	char ext[5] = "";
    if (FIO_S_ISREG(stat.st_mode)){
        getExtension(userSettings->selectedBrowserItemShort, ext, 4);
        if (!strcmp(ext, "M3U")){
            //Playlist:
            M3U_clear();
            M3U_open(userSettings->selectedBrowserItemShort);
            playPlaylist(M3U_getPlaylist(), userSettings->playlistStartIndex, 0);
        }else if (!strcmp(ext, "BKM")){
            //Bookmark:
            struct bookmark book;
            M3U_clear();
            char *repStr = NULL;
            repStr = replace(userSettings->selectedBrowserItemShort, ".bkm", ".m3u");
            if (repStr != NULL){
                M3U_open(repStr);
                sceIoRemove(repStr);
                free(repStr);
            }
            readBookmark(userSettings->selectedBrowserItemShort, &book);
            sceIoRemove(userSettings->selectedBrowserItemShort);
            playPlaylist(M3U_getPlaylist(), book.playListIndex, book.position);
        }else{
            //File:
            strcpy(dir, userSettings->selectedBrowserItem);
            strcpy(dirShort, userSettings->selectedBrowserItemShort);
			directoryUp(dir);
            directoryUp(dirShort);
			playDirectory(dir, dirShort, userSettings->selectedBrowserItemShort);
        }
    }else if (FIO_S_ISDIR(stat.st_mode)){
        //Directory:
        playDirectory(userSettings->selectedBrowserItem, userSettings->selectedBrowserItemShort, "");
    }

    oslDeleteImage(noCoverArt);
    noCoverArt = NULL;
    oslDeleteImage(fileInfoBkg);
    fileInfoBkg = NULL;
    oslDeleteImage(fileSpecsBkg);
    fileSpecsBkg = NULL;
    oslDeleteImage(playerStatusBkg);
    playerStatusBkg = NULL;
    oslDeleteImage(progressBkg);
    progressBkg = NULL;
    oslDeleteImage(progress);
    progress = NULL;

    if (userSettings->CLOCK_AUTO)
      setCpuClock(userSettings->CLOCK_GUI);

    oslSetKeyAutorepeatInit(userSettings->KEY_AUTOREPEAT_GUI);
    oslSetKeyAutorepeatInterval(userSettings->KEY_AUTOREPEAT_GUI);
    oslSetKeyAnalogToDPad(ANALOG_SENS);
    return playerRetValue;
}
