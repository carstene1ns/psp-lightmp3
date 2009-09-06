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
#include <psprtc.h>

#include "../main.h"
#include "gui_fileBrowser.h"
#include "common.h"
#include "languages.h"
#include "settings.h"
#include "skinsettings.h"
#include "menu.h"
#include "../system/opendir.h"
#include "../system/clock.h"
#include "../players/player.h"
#include "../players/m3u.h"

#define music_directory_1 "ms0:/MUSIC"
#define music_directory_2 "ms0:/PSP/MUSIC"

#define STATUS_NORMAL 0
#define STATUS_HELP 1

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Globals:
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
static int fileBrowserRetValue = 0;
static int exitFlagFileBrowser = 0;


///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Draw USB message:
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
int drawUSBmessage(){
    OSL_FONT *font = fontNormal;
    //char buffer[50];

    oslDrawImageXY(popupBkg, (480 - popupBkg->sizeX) / 2, (272 - popupBkg->sizeY) / 2);
    oslSetFont(font);
    skinGetColor("RGBA_POPUP_TITLE_TEXT", tempColor);
    skinGetColor("RGBA_POPUP_TITLE_TEXT_SHADOW", tempColorShadow);
    setFontStyle(fontNormal, defaultTextSize, RGBA(tempColor[0], tempColor[1], tempColor[2], tempColor[3]), RGBA(tempColorShadow[0], tempColorShadow[1], tempColorShadow[2], tempColorShadow[3]), INTRAFONT_ALIGN_LEFT);
    oslDrawString((480 - oslGetStringWidth(langGetString("USB_ACTIVE"))) / 2, (272 - popupBkg->sizeY) / 2 + 3, langGetString("USB_ACTIVE"));
    skinGetColor("RGBA_POPUP_MESSAGE_TEXT", tempColor);
    skinGetColor("RGBA_POPUP_MESSAGE_TEXT_SHADOW", tempColorShadow);
    setFontStyle(fontNormal, defaultTextSize, RGBA(tempColor[0], tempColor[1], tempColor[2], tempColor[3]), RGBA(tempColorShadow[0], tempColorShadow[1], tempColorShadow[2], tempColorShadow[3]), INTRAFONT_ALIGN_LEFT);
    oslDrawString((480 - oslGetStringWidth(langGetString("USB_MESSAGE"))) / 2, (272 - popupBkg->sizeY) / 2 + 40, langGetString("USB_MESSAGE"));
    return 0;
}


///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Add one file to the current playlist:
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void addFileToPlaylist(char *fileName, int save){
	struct fileInfo *info = NULL;
	char onlyName[264] = "";
    char extension[5] = "";

	if (save == 1){
		M3U_clear();
		M3U_open(tempM3Ufile);
	}

    getExtension(fileName, extension, 4);
    if (!strcmp(extension, "M3U"))
    {
        //Add a playlist:
        M3U_open(fileName);
    }
    else
    {
        //Add a file:
        if (setAudioFunctions(fileName, userSettings->MP3_ME))
            return;

        (*initFunct)(0);
        if ((*loadFunct)(fileName) == OPENING_OK){
            info = (*getInfoFunct)();
            (*endFunct)();
            if (strlen(info->title)){
                M3U_addSong(fileName, info->length, info->title);
            }else{
                getFileName(fileName, onlyName);
                M3U_addSong(fileName, info->length, onlyName);
            }
        }
        unsetAudioFunctions();
    }

    if (save == 1)
        M3U_save(tempM3Ufile);
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Add a directory to the current playlist:
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void addDirectoryToPlaylist(char *dirName, char *dirNameShort){
	char fileToAdd[264] = "";
	char message[100] = "";
	int i;
	int perc = 0;
    int oldPerc = -1;
	struct opendir_struct dirToAdd;

	cpuBoost();
	char *result = opendir_open(&dirToAdd, dirName, dirNameShort, fileExt, fileExtCount, 0);
	if (result == 0){
		M3U_clear();
        M3U_open(tempM3Ufile);
		for (i = 0; i < dirToAdd.number_of_directory_entries; i++){
			perc = (int)((i+1.0) / dirToAdd.number_of_directory_entries * 100.0);

            if (perc != oldPerc){
                oldPerc = perc;
                snprintf(message, sizeof(message), "%3.3i%% %s", perc, langGetString("DONE"));

                oslStartDrawing();
                drawCommonGraphics();
                drawButtonBar(MODE_FILEBROWSER);
                drawMenu(&commonMenu);
                drawWait(langGetString("ADDING_PLAYLIST"), message);
                oslEndDrawing();
                oslEndFrame();
                oslSyncFrame();
            }
			strcpy(fileToAdd, dirNameShort);
			if (fileToAdd[strlen(fileToAdd)-1] != '/')
				strcat(fileToAdd, "/");
			strcat(fileToAdd, dirToAdd.directory_entry[i].d_name);
			addFileToPlaylist(fileToAdd, 0);
		}
		M3U_save(tempM3Ufile);
	}
	opendir_close(&dirToAdd);
	cpuRestore();
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// File Browser:
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
int gui_fileBrowser(){
    char buffer[264];
    char bufferShort[264];
	char curDir[264];
    char curDirShort[264];
	struct opendir_struct directory;
    char *result;
    int USBactive = 0;
    int status = STATUS_NORMAL;
	OSL_IMAGE *coverArt = NULL;
    OSL_IMAGE *tmpCoverArt = NULL;
	int coverArtFailed = 0;
	u64 lastMenuChange = 0;
	int lastSelected = -1;

    fileBrowserRetValue = -1;

    //Build menu:
    skinGetPosition("POS_FILE_BROWSER", tempPos);
    commonMenu.yPos = tempPos[1];
    commonMenu.xPos = tempPos[0];
    commonMenu.fastScrolling = 1;
    snprintf(buffer, sizeof(buffer), "%s/menubkg.png", userSettings->skinImagesPath);
    commonMenu.background = oslLoadImageFilePNG(buffer, OSL_IN_RAM | OSL_SWIZZLED, OSL_PF_8888);
    if (!commonMenu.background)
        errorLoadImage(buffer);
    commonMenu.highlight = commonMenuHighlight;
    commonMenu.width = commonMenu.background->sizeX;
    commonMenu.height = commonMenu.background->sizeY;
    commonMenu.interline = skinGetParam("MENU_INTERLINE");
    commonMenu.maxNumberVisible = commonMenu.background->sizeY / (fontMenuNormal->charHeight + commonMenu.interline);
    commonMenu.cancelFunction = NULL;
    if (!strlen(userSettings->lastBrowserDir)){
		//Provo la prima directory:
		strcpy(curDir, music_directory_1);
		strcpy(curDirShort, music_directory_1);
		result = opendir_open(&directory, curDir, curDirShort, fileExt, fileExtCount, 1);
		if (result){
			//Provo la seconda directory:
			strcpy(curDir, music_directory_2);
			strcpy(curDirShort, music_directory_2);
			result = opendir_open(&directory, curDir, curDirShort, fileExt, fileExtCount, 1);
			/*if (result){
                snprintf(buffer, sizeof(buffer), "\"%s\" and \"%s\" not found or empty", music_directory_1, music_directory_2);
                debugMessageBox(buffer);
                oslQuit();
				return -1;
			}*/
		}
    }else{
		strcpy(curDir, userSettings->lastBrowserDir);
		strcpy(curDirShort, userSettings->lastBrowserDirShort);
		result = opendir_open(&directory, curDir, curDirShort, fileExt, fileExtCount, 1);
    }
    getFileName(userSettings->selectedBrowserItemShort, buffer);
    buildMenuFromDirectory(&commonMenu, &directory, buffer);

    exitFlagFileBrowser = 0;
	int skip = 0;
	cpuRestore();
    while(!osl_quit && !exitFlagFileBrowser){

		if (!skip){
			oslStartDrawing();
			drawCommonGraphics();
			drawButtonBar(MODE_FILEBROWSER);
			drawMenu(&commonMenu);
			if (coverArt){
				skinGetPosition("POS_FILE_BROWSER_COVERART", tempPos);
				oslDrawImageXY(coverArt, tempPos[0], tempPos[1]);
			}
			if (status == STATUS_HELP)
				drawHelp("FILE_BROWSER");

			if (USBactive)
				drawUSBmessage();
	    	oslEndDrawing();
		}
        oslEndFrame();
    	skip = oslSyncFrame();

        oslReadKeys();
        if (status == STATUS_HELP){
            if (osl_pad.released.cross || osl_pad.released.circle)
                status = STATUS_NORMAL;
        }else{
            if (!USBactive){
                processMenuKeys(&commonMenu);
				//Coverart:
				if (commonMenu.selected != lastSelected){
					sceRtcGetCurrentTick(&lastMenuChange);
					lastSelected = commonMenu.selected;
					if (coverArt){
						oslDeleteImage(coverArt);
						coverArt = NULL;
					}
					coverArtFailed = 0;
				}else if (!coverArt && !coverArtFailed && commonMenu.numberOfElements){
					u64 currentTime = 0;
					sceRtcGetCurrentTick(&currentTime);
					if (currentTime - lastMenuChange > COVERTART_DELAY){
						char dirName[264];
					    int size = 0;

						cpuBoost();
						if (curDirShort[strlen(curDirShort)-1] != '/')
							snprintf(dirName, sizeof(dirName), "%s/%s", curDirShort, directory.directory_entry[commonMenu.selected].d_name);
						else
							snprintf(dirName, sizeof(dirName), "%s%s", curDirShort, directory.directory_entry[commonMenu.selected].d_name);
						if (FIO_S_ISREG(directory.directory_entry[commonMenu.selected].d_stat.st_mode))
							directoryUp(dirName);
						//Look for folder.jpg in the same directory:
						snprintf(buffer, sizeof(buffer), "%s/%s", dirName, "folder.jpg");
						size = fileExists(buffer);
						if (size > 0 && size <= MAX_IMAGE_DIMENSION)
				            tmpCoverArt = oslLoadImageFileJPG(buffer, OSL_IN_RAM | OSL_SWIZZLED, OSL_PF_8888);
						else{
							//Look for cover.jpg in same directory:
							snprintf(buffer, sizeof(buffer), "%s/%s", dirName, "cover.jpg");
							size = fileExists(buffer);
							if (size > 0 && size <= MAX_IMAGE_DIMENSION)
					            tmpCoverArt = oslLoadImageFileJPG(buffer, OSL_IN_RAM | OSL_SWIZZLED, OSL_PF_8888);
						}
						if (tmpCoverArt){
                            int coverArtWidth  = skinGetParam("FILE_BROWSER_COVERART_WIDTH");
                            int coverArtHeight = skinGetParam("FILE_BROWSER_COVERART_HEIGHT");

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
			}

            if (!USBactive && osl_pad.held.L && osl_pad.held.R){
                status = STATUS_HELP;
            }else if (!USBactive && osl_pad.released.cross && commonMenu.numberOfElements){
                if (FIO_S_ISDIR(directory.directory_entry[commonMenu.selected].d_stat.st_mode)){
                    //Enter directory:
                    if (curDir[strlen(curDir)-1] != '/'){
                        strcat(curDir, "/");
                        strcat(curDirShort, "/");
					}
                    strcat(curDir, directory.directory_entry[commonMenu.selected].longname);
                    strcat(curDirShort, directory.directory_entry[commonMenu.selected].d_name);
					opendir_close(&directory);
					//debugMessageBox(curDirShort);
					cpuBoost();
                    result = opendir_open(&directory, curDir, curDirShort, fileExt, fileExtCount, 1);
                    buildMenuFromDirectory(&commonMenu, &directory, "");
					cpuRestore();
                }else if (FIO_S_ISREG(directory.directory_entry[commonMenu.selected].d_stat.st_mode)){
                    //Play file:
                    if (curDir[strlen(curDir)-1] != '/'){
                        snprintf(userSettings->selectedBrowserItem, sizeof(userSettings->selectedBrowserItem), "%s/%s", curDir, directory.directory_entry[commonMenu.selected].longname);
                        snprintf(userSettings->selectedBrowserItemShort, sizeof(userSettings->selectedBrowserItemShort), "%s/%s", curDirShort, directory.directory_entry[commonMenu.selected].d_name);
					}else{
                        snprintf(userSettings->selectedBrowserItem, sizeof(userSettings->selectedBrowserItem), "%s%s", curDir, directory.directory_entry[commonMenu.selected].longname);
                        snprintf(userSettings->selectedBrowserItemShort, sizeof(userSettings->selectedBrowserItemShort), "%s%s", curDirShort, directory.directory_entry[commonMenu.selected].d_name);
					}
                    fileBrowserRetValue = MODE_PLAYER;
                    userSettings->previousMode = MODE_FILEBROWSER;
                    exitFlagFileBrowser = 1;
                }
            }else if (!USBactive && osl_pad.released.start && commonMenu.numberOfElements){
                if (curDir[strlen(curDir)-1] != '/'){
                    snprintf(buffer, sizeof(buffer), "%s/%s", curDir, directory.directory_entry[commonMenu.selected].longname);
                    snprintf(bufferShort, sizeof(bufferShort), "%s/%s", curDirShort, directory.directory_entry[commonMenu.selected].d_name);
				}else{
                    snprintf(buffer, sizeof(buffer), "%s%s", curDir, directory.directory_entry[commonMenu.selected].longname);
                    snprintf(bufferShort, sizeof(bufferShort), "%s%s", curDirShort, directory.directory_entry[commonMenu.selected].d_name);
				}
                if (FIO_S_ISDIR(directory.directory_entry[commonMenu.selected].d_stat.st_mode)){
                    addDirectoryToPlaylist(buffer, bufferShort);
                    continue;
                }else if (FIO_S_ISREG(directory.directory_entry[commonMenu.selected].d_stat.st_mode))
                    drawWait(langGetString("ADDING_PLAYLIST"), langGetString("WAIT"));
					cpuBoost();
                    addFileToPlaylist(buffer, 1);
					cpuRestore();
                    continue;
            }else if(!USBactive && osl_pad.released.square && commonMenu.numberOfElements){
                //Play directory:
                if (FIO_S_ISDIR(directory.directory_entry[commonMenu.selected].d_stat.st_mode)){
                    if (curDir[strlen(curDir)-1] != '/'){
                        snprintf(userSettings->selectedBrowserItem, sizeof(userSettings->selectedBrowserItem), "%s/%s", curDir, directory.directory_entry[commonMenu.selected].longname);
                        snprintf(userSettings->selectedBrowserItemShort, sizeof(userSettings->selectedBrowserItemShort), "%s/%s", curDirShort, directory.directory_entry[commonMenu.selected].d_name);
					}else{
                        snprintf(userSettings->selectedBrowserItem, sizeof(userSettings->selectedBrowserItem), "%s%s", curDir, directory.directory_entry[commonMenu.selected].longname);
                        snprintf(userSettings->selectedBrowserItemShort, sizeof(userSettings->selectedBrowserItemShort), "%s%s", curDirShort, directory.directory_entry[commonMenu.selected].d_name);
					}
                    fileBrowserRetValue = MODE_PLAYER;
                    userSettings->previousMode = MODE_FILEBROWSER;
                    exitFlagFileBrowser = 1;
                }
            }else if(!USBactive && osl_pad.released.circle){
                //Up one level:
                char tempDir[264] = "";
                getFileName(curDir, tempDir);
                if (directoryUp(curDir) == 0){
					directoryUp(curDirShort);
                    int i;
                    opendir_close(&directory);
					cpuBoost();
                    result = opendir_open(&directory, curDir, curDirShort, fileExt, fileExtCount, 1);
                    buildMenuFromDirectory(&commonMenu, &directory, "");
                    //Mi riposiziono:
                    for (i = 0; i < directory.number_of_directory_entries; i++){
                        if (strcmp(tempDir, directory.directory_entry[i].longname) == 0){
                            commonMenu.first = i;
                            commonMenu.selected = i;
                            break;
                        }
                    }
					cpuRestore();
                }
            }else if(osl_pad.released.select){
                //USB activate/deactivate:
                USBactive = !USBactive;
                if (USBactive){
					cpuBoost();
                    oslInitUsbStorage();
                    oslStartUsbStorage();
                }else{
                    oslStopUsbStorage();
                    oslDeinitUsbStorage();

					char tempDir[264] = "";
					strcpy(tempDir, directory.directory_entry[commonMenu.selected].d_name);
					opendir_close(&directory);
					result = opendir_open(&directory, curDir, curDirShort, fileExt, fileExtCount, 1);
					buildMenuFromDirectory(&commonMenu, &directory, tempDir);
					cpuRestore();
                }
            }else if(!USBactive && osl_pad.released.R){
                fileBrowserRetValue = nextAppMode(MODE_FILEBROWSER);
                exitFlagFileBrowser = 1;
            }else if(!USBactive && osl_pad.released.L){
                fileBrowserRetValue = previousAppMode(MODE_FILEBROWSER);
                exitFlagFileBrowser = 1;
            }
        }
    }
    opendir_close(&directory);
    //unLoad images:
	if (coverArt){
		oslDeleteImage(coverArt);
        coverArt = NULL;
    }
	clearMenu(&commonMenu);

    strcpy(userSettings->lastBrowserDir, curDir);
    strcpy(userSettings->lastBrowserDirShort, curDirShort);
	return fileBrowserRetValue;
}
