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
#include <psphprm.h>

#include "../main.h"
#include "gui_player.h"
#include "common.h"
#include "languages.h"
#include "settings.h"
#include "../system/opendir.h"
#include "../system/clock.h"
#include "../players/player.h"
#include "../players/equalizer.h"
#include "../players/m3u.h"
#include "../players/id3.h"
#include "../others/audioscrobbler.h"
#include "../others/medialibrary.h"

#define STATUS_NORMAL 0
#define STATUS_HELP 1
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Functions imported from prx:
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
int displayEnable(void);
int displayDisable(void);
int getBrightness();
void setBrightness(int brightness);
void readButtons(SceCtrlData *pad_data, int count);
int imposeSetHomePopup(int value);
void MEDisable();
void MEEnable();

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Globals:
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
static int playerRetValue = 0;
OSL_IMAGE *coverArt;
OSL_IMAGE *noCoverArt;
OSL_IMAGE *fileInfoBkg;
OSL_IMAGE *fileSpecsBkg;
OSL_IMAGE *progressBkg;
OSL_IMAGE *progress;
OSL_IMAGE *playerStatusBkg;
char buffer[264];

char sleepModeDesc[3][50];
char playModeDesc[5][50];
char playerStatusDesc[3][50];
int playerStatus = 0; //-1=open 0=paused 1=playing
struct equalizer tEQ;

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Draws a file's info
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
int drawFileInfo(struct fileInfo *info, struct libraryEntry *libEntry, char *trackMessage){
    int startX = userSettings->fileInfoX;
    int startY = userSettings->fileInfoY;
    char timestring[20] = "";
    OSL_FONT *font = fontNormal;

    oslDrawImageXY(fileInfoBkg, startX, startY);
    oslSetFont(font);
    oslSetTextColor(RGBA(userSettings->colorLabel[0], userSettings->colorLabel[1], userSettings->colorLabel[2], userSettings->colorLabel[3]));
    oslDrawString(startX + 2, startY + 5, langGetString("TITLE"));
    oslDrawString(startX + 2, startY + 20, langGetString("ARTIST"));
    oslDrawString(startX + 2, startY + 35, langGetString("ALBUM"));
    oslDrawString(startX + 2, startY + 50, langGetString("GENRE"));
    oslDrawString(startX + 240, startY + 50, langGetString("RATING"));
    oslDrawString(startX + 2, startY + 65, langGetString("YEAR"));
    oslDrawString(startX + 240, startY + 65, langGetString("PLAYED"));
    oslDrawString(startX + 2, startY + 80, langGetString("TIME"));
    oslDrawString(startX + 240, startY + 80, langGetString("TRACK"));

    oslSetTextColor(RGBA(userSettings->colorText[0], userSettings->colorText[1], userSettings->colorText[2], userSettings->colorText[3]));
    oslDrawString(startX + 60, startY + 5, info->title);
    oslDrawString(startX + 60, startY + 20, info->artist);
    oslDrawString(startX + 60, startY + 35, info->album);
    oslDrawString(startX + 60, startY + 50, info->genre);
    oslDrawString(startX + 60, startY + 65, info->year);
    sprintf(buffer, "%i", libEntry->played);
    oslDrawString(startX + 330, startY + 65, buffer);

    drawRating(startX + 300, startY + 50, libEntry->rating);

    (*getTimeStringFunct)(timestring);
    if (!strlen(info->strLength))
        strcpy(info->strLength, "00:00:00");
    sprintf(buffer, "%s / %s", timestring, info->strLength);
    oslDrawString(startX + 60, startY + 80, buffer);
    oslDrawString(startX + 330, startY + 80, trackMessage);
    return 0;
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Draws cover art:
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
int drawCoverArt(){
    if (coverArt)
        oslDrawImageXY(coverArt, userSettings->coverArtX , userSettings->coverArtY);
    else
        oslDrawImageXY(noCoverArt, userSettings->coverArtX , userSettings->coverArtY);
    return 0;
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Draws a file's specs
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
int drawFileSpecs(struct fileInfo *info){
    int startX = userSettings->fileSpecsX;
    int startY = userSettings->fileSpecsY;
    OSL_FONT *font = fontNormal;

    oslDrawImageXY(fileSpecsBkg, startX, startY);
    oslSetFont(font);
    oslSetTextColor(RGBA(userSettings->colorLabel[0], userSettings->colorLabel[1], userSettings->colorLabel[2], userSettings->colorLabel[3]));
    oslDrawString(startX + 2, startY + 5, langGetString("MODE"));
    oslDrawString(startX + 2, startY + 20, langGetString("BITRATE"));
    oslDrawString(startX + 2, startY + 35, langGetString("SAMPLERATE"));
    oslDrawString(startX + 2, startY + 50, langGetString("FILE_FORMAT"));
    oslDrawString(startX + 2, startY + 65, langGetString("EMPHASIS"));

    oslSetTextColor(RGBA(userSettings->colorText[0], userSettings->colorText[1], userSettings->colorText[2], userSettings->colorText[3]));
    oslDrawString(startX + 70, startY + 5, info->mode);
    sprintf(buffer, "%3.3i [%3.3i] kbit", info->kbit, (int)(info->instantBitrate / 1000));
    oslDrawString(startX + 70, startY + 20, buffer);
    sprintf(buffer, "%li Hz", info->hz);
    oslDrawString(startX + 70, startY + 35, buffer);
    if (info->fileType == MP3_TYPE)
        sprintf(buffer, "%s %s %s", fileTypeDescription[info->fileType], langGetString("LAYER"), info->layer);
    else if (info->fileType >= 0)
        sprintf(buffer, "%s", fileTypeDescription[info->fileType]);
    oslDrawString(startX + 70, startY + 50, buffer);
    oslDrawString(startX + 70, startY + 65, info->emphasis);
    return 0;
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Draws players's status
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
int drawPlayerStatus(){
    int startX = userSettings->playerStatusX;
    int startY = userSettings->playerStatusY;
    OSL_FONT *font = fontNormal;

    oslDrawImageXY(playerStatusBkg, startX, startY);
    oslSetFont(font);
    oslSetTextColor(RGBA(userSettings->colorLabel[0], userSettings->colorLabel[1], userSettings->colorLabel[2], userSettings->colorLabel[3]));
    oslDrawString(startX + 2, startY + 5, langGetString("SLEEP_MODE"));
    oslDrawString(startX + 2, startY + 20, langGetString("PLAY_MODE"));
    oslDrawString(startX + 2, startY + 35, langGetString("STATUS"));
    oslDrawString(startX + 2, startY + 50, langGetString("VOLUME_BOOST"));
    oslDrawString(startX + 2, startY + 65, langGetString("EQUALIZER"));

    oslSetTextColor(RGBA(userSettings->colorText[0], userSettings->colorText[1], userSettings->colorText[2], userSettings->colorText[3]));
    oslDrawString(startX + 90, startY + 5, sleepModeDesc[userSettings->sleepMode]);
    oslDrawString(startX + 90, startY + 20, playModeDesc[userSettings->playMode]);
    int currentSpeed = (*getPlayingSpeedFunct)();
    if (!currentSpeed)
        oslDrawString(startX + 90, startY + 35, playerStatusDesc[playerStatus+1]);
    else{
        if (currentSpeed > 0)
            sprintf(buffer, "%s %ix", playerStatusDesc[playerStatus+1], currentSpeed + 1);
        else
            sprintf(buffer, "%s %ix", playerStatusDesc[playerStatus+1], currentSpeed - 1);
        oslDrawString(startX + 90, startY + 35, buffer);
    }

    if (!MAX_VOLUME_BOOST)
        oslDrawString(startX + 90, startY + 50, langGetString("NOT_SUPPORTED"));
    else{
        sprintf(buffer, "%i", userSettings->volumeBoost);
        oslDrawString(startX + 90, startY + 50, buffer);
    }

    if (!(*isFilterSupportedFunct)())
        oslDrawString(startX + 90, startY + 65, langGetString("NOT_SUPPORTED"));
    else
        oslDrawString(startX + 90, startY + 65, tEQ.name);
    return 0;
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Draws progress bar:
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
int drawProgressBar(){
    int perc = (*getPercentageFunct)();
    oslDrawImageXY(progressBkg, userSettings->progressX, userSettings->progressY);
    progress->stretchX = (int)((float)progress->sizeX / 100.00 * (float)perc);
    oslDrawImageXY(progress, userSettings->progressX, userSettings->progressY + 2);
    return 0;
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Play a single file:
//		 Returns 1 if user pressed NEXT
//				 0 if song ended
//				-1 if user pressed PREVIOUS
//				 2 if user pressed STOP
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
int playFile(char *fileName, char *trackMessage){
    int retValue = 0;
    struct fileInfo info;
    struct libraryEntry libEntry;
    int currentSpeed = 0;
    int clock = 0;
    int lastPercentage = 0;
    int ratingChangedUpDown = 0;
    int ratingChangedCross = 0;
    int helpShown = 0;
    SceCtrlData pad;
    u32 remoteButtons;
    int flagExit = 0;
    int status = STATUS_NORMAL;

    MEEnable();

    int oldClock = getCpuClock();
    int oldBus = getBusClock();
    setCpuClock(222);
    setBusClock(111);

    initFileInfo(&info);
    if (userSettings->displayStatus){
        oslStartDrawing();
        drawCommonGraphics();
    	oslEndDrawing();
    	oslSyncFrame();
    }

    setVolume(0,0x8000);
    setAudioFunctions(fileName, userSettings->MP3_ME);
    //Tipo di volume boost:
    if (strcmp(userSettings->BOOST, "OLD") == 0){
        (*setVolumeBoostTypeFunct)("OLD");
    }else{
        (*setVolumeBoostTypeFunct)("NEW");
    }
    if (userSettings->volumeBoost > MAX_VOLUME_BOOST)
        userSettings->volumeBoost = MAX_VOLUME_BOOST;
    if (userSettings->volumeBoost < MIN_VOLUME_BOOST)
        userSettings->volumeBoost = MIN_VOLUME_BOOST;

    //Equalizzatore:
    if (!(*isFilterSupportedFunct)())
      userSettings->currentEQ = 0;

    tEQ = EQ_getIndex(userSettings->currentEQ);
    (*setFilterFunct)(tEQ.filter, 1);
    if (!userSettings->currentEQ)
      (*disableFilterFunct)();
    else
      (*enableFilterFunct)();

    (*initFunct)(0);
    (*setVolumeBoostFunct)(userSettings->volumeBoost);

    //Read TAG and Media Library:
    info = (*getTagInfoFunct)(fileName);
    getCovertArtImageName(fileName, &info);

    char whereCond[200] = "";
    char fixedName[264] = "";
    strcpy(fixedName, fileName);
    ML_fixStringField(fixedName);
    sprintf(whereCond, "path = upper('%s')", fixedName);
    int update = ML_countRecords(whereCond);
    if (update > 0){
        ML_queryDB(whereCond, "path", 0, 1, MLresult);
        libEntry = MLresult[0];
    }else
        ML_clearEntry(&libEntry);

    //Save temp coverart file:
    coverArt = NULL;
    if (info.encapsulatedPictureOffset && info.encapsulatedPictureLength <= MAX_IMAGE_DIMENSION){
        FILE *in = fopen(fileName, "rb");
        if (info.encapsulatedPictureType == JPEG_IMAGE)
            sprintf(buffer, "%scoverart.jpg", userSettings->ebootPath);
        else if (info.encapsulatedPictureType == PNG_IMAGE)
            sprintf(buffer, "%scoverart.png", userSettings->ebootPath);
        FILE *out = fopen(buffer, "wb");
        int buffSize = 4*1024;
        unsigned char cover[4*1024] = "";
        fseek(in, info.encapsulatedPictureOffset, SEEK_SET);
        int remaining = info.encapsulatedPictureLength;
        while (remaining > 0){
            if (remaining < buffSize)
                buffSize = remaining;
            int write = fread(cover, sizeof(char), buffSize, in);
            fwrite(cover, sizeof(char), buffSize, out);
            remaining -= write;
        }
        fclose(out);
        fclose(in);
        if (info.encapsulatedPictureType == JPEG_IMAGE)
            coverArt = oslLoadImageFileJPG(buffer, OSL_IN_RAM | OSL_SWIZZLED, OSL_PF_8888);
        else if (info.encapsulatedPictureType == PNG_IMAGE)
            coverArt = oslLoadImageFilePNG(buffer, OSL_IN_RAM | OSL_SWIZZLED, OSL_PF_8888);
        sceIoRemove(buffer);
    }else if (strlen(info.coverArtImageName))
        coverArt = oslLoadImageFileJPG(info.coverArtImageName, OSL_IN_RAM | OSL_SWIZZLED, OSL_PF_8888);

    if (coverArt){
        coverArt->stretchX = 90;
        coverArt->stretchY = 90;
    }

    playerStatus = -1;
    if (userSettings->displayStatus){
        oslStartDrawing();
        drawCommonGraphics();
        drawFileInfo(&info, &libEntry, trackMessage);
        drawFileSpecs(&info);
        drawPlayerStatus();
        drawCoverArt();
    	oslEndDrawing();
    	oslSyncFrame();
    }

    //Apro il file:
    int retLoad = (*loadFunct)(fileName);
    if (retLoad != OPENING_OK){
        sprintf(buffer, "Error opening file: %i", retLoad);
        debugMessageBox(buffer);
        (*endFunct)();
        unsetAudioFunctions();
        return 0;
    }

    info = (*getInfoFunct)();
    setBusClock(oldBus);
    setCpuClock(oldClock);

    //Update media library info:
    getExtension(fileName, libEntry.extension, 4);
    strcpy(libEntry.album, info.album);
    strcpy(libEntry.artist, info.artist);
    strcpy(libEntry.title, info.title);
    strcpy(libEntry.genre, info.genre);
    strcpy(libEntry.year, info.year);
    strcpy(libEntry.path, fileName);
    libEntry.seconds = info.length;
    libEntry.bitrate = info.kbit;
    libEntry.samplerate = info.hz;
    libEntry.tracknumber = atoi(info.trackNumber);

    //Imposto il clock:
    if (userSettings->CLOCK_AUTO){
        if (userSettings->displayStatus)
            setCpuClock(info.defaultCPUClock);
        else
            setCpuClock(info.defaultCPUClock - userSettings->CLOCK_DELTA_ECONOMY_MODE);
        clock = info.defaultCPUClock;
    }else
        clock = getCpuClock();

    //Disable media engine if safe and unused
    if (!info.needsME && sceKernelDevkitVersion() < 0x03070110 && !getModel())
        MEDisable();

    (*playFunct)();
    playerStatus = 1;

    flagExit = 0;
    while(!osl_quit && !flagExit){
        if (userSettings->displayStatus){
            oslStartDrawing();
            drawCommonGraphics();
            info = (*getInfoFunct)();
            drawFileInfo(&info, &libEntry, trackMessage);
            drawFileSpecs(&info);
            drawPlayerStatus();
            drawProgressBar();
            drawCoverArt();
            if (status == STATUS_HELP)
                drawHelp("PLAYER");
        }else{
			scePowerTick(0);
        }

        lastPercentage = (*getPercentageFunct)();
        oslReadKeys();
        readButtons(&pad, 1);
        sceHprmPeekCurrentKey(&remoteButtons);

        if (status == STATUS_HELP){
            if (osl_keys->released.cross || osl_keys->released.circle)
                status = STATUS_NORMAL;
        }else{
            if (osl_keys->held.L && osl_keys->held.R){
                if (userSettings->displayStatus){
                    helpShown = 1;
                    status = STATUS_HELP;
                }
            }else if (osl_keys->held.cross && osl_keys->held.up){
                if (++libEntry.rating > ML_MAX_RATING)
                    libEntry.rating = ML_MAX_RATING;
                ratingChangedUpDown = 1;
                ratingChangedCross= 1;
                sceKernelDelayThread(KEY_AUTOREPEAT_PLAYER*15000);
            }else if (osl_keys->held.cross && osl_keys->held.down){
                if (--libEntry.rating < 0)
                    libEntry.rating = 0;
                ratingChangedUpDown = 1;
                ratingChangedCross= 1;
                sceKernelDelayThread(KEY_AUTOREPEAT_PLAYER*15000);
            }else if (osl_keys->released.circle){
                (*endFunct)();
                playerStatus = 0;
                retValue = 2;
                flagExit = 1;
            }else if (osl_keys->released.square){
        		if (playerStatus == 1){
                    userSettings->muted = !userSettings->muted;
        			(*setMuteFunct)(userSettings->muted);
                }
            }else if (osl_keys->released.R || (remoteButtons & PSP_HPRM_FORWARD)){
                if (helpShown)
                    helpShown = 0;
                else{
                    if (userSettings->FADE_OUT)
                        (*fadeOutFunct)(0.3);
                    (*endFunct)();
                    retValue = 1;
                    flagExit = 1;
                }
            }else if (osl_keys->released.L || (remoteButtons & PSP_HPRM_BACK)){
                if (helpShown)
                    helpShown = 0;
                else{
                    if (userSettings->FADE_OUT)
                        (*fadeOutFunct)(0.3);
                    (*endFunct)();
                    retValue = -1;
                    flagExit = 1;
                }
            }else if (osl_keys->released.cross || (remoteButtons & PSP_HPRM_PLAYPAUSE)){
                if (ratingChangedCross)
                    ratingChangedCross = 0;
                else{
                    currentSpeed = (*getPlayingSpeedFunct)();
                    if (currentSpeed)
                        (*setPlayingSpeedFunct)(0);
                    else{
                        (*pauseFunct)();
                        playerStatus = !playerStatus;
                    }
                }
            }else if (osl_keys->released.up){
                if (ratingChangedUpDown)
                    ratingChangedUpDown = 0;
                else
            		if (userSettings->volumeBoost < MAX_VOLUME_BOOST)
            			(*setVolumeBoostFunct)(++userSettings->volumeBoost);
            }else if (osl_keys->released.down){
                if (ratingChangedUpDown)
                    ratingChangedUpDown = 0;
                else
            		if (userSettings->volumeBoost > MIN_VOLUME_BOOST)
            			(*setVolumeBoostFunct)(--userSettings->volumeBoost);
            }else if (osl_keys->released.right && playerStatus == 1){
                currentSpeed = (*getPlayingSpeedFunct)();
                if (currentSpeed < MAX_PLAYING_SPEED)
                	(*setPlayingSpeedFunct)(++currentSpeed);
            }else if (osl_keys->released.left && playerStatus == 1){
                currentSpeed = (*getPlayingSpeedFunct)();
                if (currentSpeed > MIN_PLAYING_SPEED)
                    (*setPlayingSpeedFunct)(--currentSpeed);
            }else if (osl_keys->analogY < -ANALOG_SENS && !osl_keys->pressed.hold){
                if (clock < 222)
                    setCpuClock(++clock);
                sceKernelDelayThread(100000);
            }else if (osl_keys->analogY > ANALOG_SENS && !osl_keys->pressed.hold){
                if (clock > getMinCPUClock())
                    setCpuClock(--clock);
                sceKernelDelayThread(100000);
            }else if (osl_keys->released.triangle){
        		//Change sleep mode:
        		if (++userSettings->sleepMode > SLEEP_PLAYLIST)
        			userSettings->sleepMode = 0;
    		}else if(pad.Buttons & PSP_CTRL_NOTE){
    			//Cambio equalizzatore:
                if ((*isFilterSupportedFunct)()){
                    if (++userSettings->currentEQ >= EQ_getEqualizersNumber()){
                        userSettings->currentEQ = 0;
                        (*disableFilterFunct)();
                    }else
                        (*enableFilterFunct)();
                    tEQ = EQ_getIndex(userSettings->currentEQ);
                    (*setFilterFunct)(tEQ.filter, 1);
                    sceKernelDelayThread(400000);
                }
            }else if (osl_keys->released.select){
    				//Change playing mode:
    				if (++userSettings->playMode > MODE_SHUFFLE_REPEAT)
    					userSettings->playMode = MODE_NORMAL;
            }else if (osl_keys->released.start){
        		if (userSettings->displayStatus){
                    oslEndDrawing();
        			//Spengo il display:
                    userSettings->curBrightness = getBrightness();
                    setBrightness(0);
        			displayDisable();
                    oslSetFrameskip(5);
        			imposeSetHomePopup(0);
        			userSettings->displayStatus = 0;
        			//Downclock:
                    if (userSettings->CLOCK_DELTA_ECONOMY_MODE)
                        setCpuClock(clock - userSettings->CLOCK_DELTA_ECONOMY_MODE);
        		} else {
        			//Accendo il display:
                    oslStartDrawing();
                    oslClearScreen(RGBA(0, 0, 0, 255));
        			displayEnable();
                    oslSetFrameskip(0);
        			imposeSetHomePopup(1);
                    setBrightness(userSettings->curBrightness);
        			userSettings->displayStatus = 1;
                    if (userSettings->CLOCK_DELTA_ECONOMY_MODE)
            			setCpuClock(clock);
        		}
            }
        }

        if (userSettings->displayStatus)
        	oslEndDrawing();
    	oslSyncFrame();

        //Controllo se la riproduzione è finita:
        if ((*endOfStreamFunct)() == 1) {
            (*endFunct)();
            retValue = 0;
            flagExit = 1;
        }
    }

    //Srobbler LOG:
    if (userSettings->SCROBBLER && strlen(info.title)){
        time_t mytime;
        time(&mytime);
        if (lastPercentage >= 50){
            SCROBBLER_addTrack(info, info.length, "L", mytime);
        }else{
        	SCROBBLER_addTrack(info, info.length, "S", mytime);
        }
    }

    unsetAudioFunctions();

    //Unload images:
    if (coverArt)
        oslDeleteImage(coverArt);

    //Update media Library info:
    if (lastPercentage >= 50)
        libEntry.played++;
    if (update > 0)
        ML_updateEntry(libEntry);
    else
        ML_addEntry(libEntry);

    //Sleep mode:
    if (userSettings->sleepMode == SLEEP_TRACK){
      osl_quit = 1;
      userSettings->shutDown = 1;
    }
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
int playPlaylist(struct M3U_playList *playList, int startIndex){
	int songCount = M3U_getSongCount();
	unsigned int playedTracks[songCount];
	unsigned int playedTracksNumber = 0;
	unsigned int currentTrack = 0;
    struct M3U_songEntry *song;
    int i = 0;
    char message[20] = "";

    if (startIndex >= 0)
        i = startIndex;
	else if (userSettings->playMode == MODE_SHUFFLE || userSettings->playMode == MODE_SHUFFLE_REPEAT)
		i = randomTrack(songCount, playedTracksNumber, playedTracks);
	else
		i = 0;

	currentTrack = 0;
	while(!osl_quit){
		song = M3U_getSong(i);
		sprintf(message, "%i / %i", i + 1, songCount);
		int playerReturn = playFile(song->fileName, message);
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
		if (playerReturn == 2){
			break;
		}else if ((playerReturn == 1) | (playerReturn == 0)){
            //Controllo la ripetizione della traccia:
            if (playerReturn == 0 && userSettings->playMode == MODE_REPEAT)
                continue;
			//Controllo se sono in shuffle:
			if (userSettings->playMode == MODE_SHUFFLE || userSettings->playMode == MODE_SHUFFLE_REPEAT){
				if (currentTrack < playedTracksNumber - 1 && playerReturn == 1){
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
				if ((playerReturn == 0) & (i == songCount - 1) & (userSettings->playMode != MODE_REPEAT_ALL)){
					break;
				}
				//Altrimenti passo al file successivo:
				i++;
				if (i > songCount - 1){
					i = 0;
				}
				currentTrack = i;
			}
		}else if (playerReturn == -1){
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
	if (!userSettings->displayStatus){
        oslStartDrawing();
        oslClearScreen(RGBA(0, 0, 0, 255));
    	oslEndDrawing();
    	displayEnable();
    	oslSetFrameskip(0);
    	imposeSetHomePopup(1);
        setBrightness(userSettings->curBrightness);
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
int playDirectory(char *dirName, char *startFile){
    struct opendir_struct directory;
    int startIndex = -1;
    char onlyName[264] = "";

    M3U_clear();
    char *result = opendir_open(&directory, dirName, fileExt, fileExtCount, 0);
    if (!result){
        sortDirectory(&directory);
        getFileName(startFile, onlyName);
        int i = 0;
        for (i=0; i<directory.number_of_directory_entries; i++){
            if (dirName[strlen(dirName)-1] != '/')
                sprintf(buffer, "%s/%s", dirName, directory.directory_entry[i].d_name);
            else
                sprintf(buffer, "%s%s", dirName, directory.directory_entry[i].d_name);
            if (!strcmp(directory.directory_entry[i].d_name, onlyName))
                startIndex = i;
            M3U_addSong(buffer, 0, buffer);
        }
        opendir_close(&directory);
        playPlaylist(M3U_getPlaylist(), startIndex);
    }
    M3U_clear();
    return 0;
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Player:
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
int gui_player(){
    SceIoStat stat;
    sceIoGetstat(userSettings->selectedBrowserItem, &stat);

    //Load images:
    sprintf(buffer, "%s/nocoverart.png", userSettings->skinImagesPath);
    noCoverArt = oslLoadImageFilePNG(buffer, OSL_IN_RAM | OSL_SWIZZLED, OSL_PF_8888);
    if (!noCoverArt)
        errorLoadImage(buffer);

    sprintf(buffer, "%s/fileinfobkg.png", userSettings->skinImagesPath);
    fileInfoBkg = oslLoadImageFilePNG(buffer, OSL_IN_RAM | OSL_SWIZZLED, OSL_PF_8888);
    if (!fileInfoBkg)
        errorLoadImage(buffer);

    sprintf(buffer, "%s/filespecsbkg.png", userSettings->skinImagesPath);
    fileSpecsBkg = oslLoadImageFilePNG(buffer, OSL_IN_RAM | OSL_SWIZZLED, OSL_PF_8888);
    if (!fileSpecsBkg)
        errorLoadImage(buffer);

    sprintf(buffer, "%s/playerstatusbkg.png", userSettings->skinImagesPath);
    playerStatusBkg = oslLoadImageFilePNG(buffer, OSL_IN_RAM | OSL_SWIZZLED, OSL_PF_8888);
    if (!playerStatusBkg)
        errorLoadImage(buffer);

    sprintf(buffer, "%s/progressbkg.png", userSettings->skinImagesPath);
    progressBkg = oslLoadImageFilePNG(buffer, OSL_IN_RAM | OSL_SWIZZLED, OSL_PF_8888);
    if (!progressBkg)
        errorLoadImage(buffer);

    sprintf(buffer, "%s/progress.png", userSettings->skinImagesPath);
    progress = oslLoadImageFilePNG(buffer, OSL_IN_RAM | OSL_SWIZZLED, OSL_PF_8888);
    if (!progress)
        errorLoadImage(buffer);

    playerRetValue = userSettings->previousMode;

    //Load lang strings:
    strcpy(sleepModeDesc[0], langGetString("SLEEP_MODE_0"));
    strcpy(sleepModeDesc[1], langGetString("SLEEP_MODE_1"));
    strcpy(sleepModeDesc[2], langGetString("SLEEP_MODE_2"));

    strcpy(playModeDesc[0], langGetString("PLAY_MODE_0"));
    strcpy(playModeDesc[1], langGetString("PLAY_MODE_1"));
    strcpy(playModeDesc[2], langGetString("PLAY_MODE_2"));
    strcpy(playModeDesc[3], langGetString("PLAY_MODE_3"));
    strcpy(playModeDesc[4], langGetString("PLAY_MODE_4"));

    strcpy(playerStatusDesc[0], langGetString("PLAYER_STATUS_0"));
    strcpy(playerStatusDesc[1], langGetString("PLAYER_STATUS_1"));
    strcpy(playerStatusDesc[2], langGetString("PLAYER_STATUS_2"));

    oslSetKeyAutorepeatInit(KEY_AUTOREPEAT_PLAYER);
    oslSetKeyAutorepeatInterval(KEY_AUTOREPEAT_PLAYER);

    char dir[264] = "";
    char ext[5] = "";
    if (FIO_S_ISREG(stat.st_mode)){
        getExtension(userSettings->selectedBrowserItem, ext, 4);
        if (!strcmp(ext, "M3U")){
            M3U_open(userSettings->selectedBrowserItem);
            playPlaylist(M3U_getPlaylist(), -1);
        }else{
            strcpy(dir, userSettings->selectedBrowserItem);
            directoryUp(dir);
            playDirectory(dir, userSettings->selectedBrowserItem);
        }
    }else if (FIO_S_ISDIR(stat.st_mode)){
        playDirectory(userSettings->selectedBrowserItem, "");
    }

    oslDeleteImage(noCoverArt);
    oslDeleteImage(fileInfoBkg);
    oslDeleteImage(fileSpecsBkg);
    oslDeleteImage(playerStatusBkg);
    oslDeleteImage(progressBkg);
    oslDeleteImage(progress);

    if (userSettings->CLOCK_AUTO)
      setCpuClock(userSettings->CLOCK_GUI);

    oslSetKeyAutorepeatInit(KEY_AUTOREPEAT_GUI);
    oslSetKeyAutorepeatInterval(KEY_AUTOREPEAT_GUI);
    return playerRetValue;
}
