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

	if (save == 1){
		M3U_clear();
		M3U_open(tempM3Ufile);
	}

    setAudioFunctions(fileName, userSettings->MP3_ME);
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

		if (save == 1)
			M3U_save(tempM3Ufile);
	}
    unsetAudioFunctions();
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
	int coverArtFailed = 0;
	time_t lastMenuChange = 0;
	int lastSelected = -1;

    fileBrowserRetValue = -1;

    //Build menu:
    skinGetPosition("POS_FILE_BROWSER", tempPos);
    commonMenu.yPos = tempPos[1];
    commonMenu.xPos = tempPos[0];
    commonMenu.fastScrolling = 1;
    sprintf(buffer, "%s/menubkg.png", userSettings->skinImagesPath);
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
                sprintf(buffer, "\"%s\" and \"%s\" not found or empty", music_directory_1, music_directory_2);
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
    sortDirectory(&directory);
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
					sceKernelLibcTime(&lastMenuChange);
					lastSelected = commonMenu.selected;
					if (coverArt){
						oslDeleteImage(coverArt);
						coverArt = NULL;
					}
					coverArtFailed = 0;
				}else if (!coverArt && !coverArtFailed && commonMenu.numberOfElements){
					time_t currentTime = 0;
					sceKernelLibcTime(&currentTime);
					if (currentTime - lastMenuChange > COVERTART_DELAY){
						char dirName[264];
					    int size = 0;

						cpuBoost();
						if (curDirShort[strlen(curDirShort)-1] != '/')
							sprintf(dirName, "%s/%s", curDirShort, directory.directory_entry[commonMenu.selected].d_name);
						else
							sprintf(dirName, "%s%s", curDirShort, directory.directory_entry[commonMenu.selected].d_name);
						if (FIO_S_ISREG(directory.directory_entry[commonMenu.selected].d_stat.st_mode))
							directoryUp(dirName);
						//Look for folder.jpg in the same directory:
						sprintf(buffer, "%s/%s", dirName, "folder.jpg");
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
							coverArt->stretchX = skinGetParam("FILE_BROWSER_COVERART_WIDTH");
							coverArt->stretchY = skinGetParam("FILE_BROWSER_COVERART_HEIGHT");
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
                    sortDirectory(&directory);
                    buildMenuFromDirectory(&commonMenu, &directory, "");
					cpuRestore();
                    //sceKernelDelayThread(200000);
                }else if (FIO_S_ISREG(directory.directory_entry[commonMenu.selected].d_stat.st_mode)){
                    //Play file:
                    if (curDir[strlen(curDir)-1] != '/'){
                        sprintf(userSettings->selectedBrowserItem, "%s/%s", curDir, directory.directory_entry[commonMenu.selected].longname);
                        sprintf(userSettings->selectedBrowserItemShort, "%s/%s", curDirShort, directory.directory_entry[commonMenu.selected].d_name);
					}else{
                        sprintf(userSettings->selectedBrowserItem, "%s%s", curDir, directory.directory_entry[commonMenu.selected].longname);
                        sprintf(userSettings->selectedBrowserItemShort, "%s%s", curDirShort, directory.directory_entry[commonMenu.selected].d_name);
					}
                    fileBrowserRetValue = MODE_PLAYER;
                    userSettings->previousMode = MODE_FILEBROWSER;
                    exitFlagFileBrowser = 1;
                }
            }else if (!USBactive && osl_pad.released.start && commonMenu.numberOfElements){
                if (curDir[strlen(curDir)-1] != '/'){
                    sprintf(buffer, "%s/%s", curDir, directory.directory_entry[commonMenu.selected].longname);
                    sprintf(bufferShort, "%s/%s", curDirShort, directory.directory_entry[commonMenu.selected].d_name);
				}else{
                    sprintf(buffer, "%s%s", curDir, directory.directory_entry[commonMenu.selected].longname);
                    sprintf(bufferShort, "%s%s", curDirShort, directory.directory_entry[commonMenu.selected].d_name);
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
                        sprintf(userSettings->selectedBrowserItem, "%s/%s", curDir, directory.directory_entry[commonMenu.selected].longname);
                        sprintf(userSettings->selectedBrowserItemShort, "%s/%s", curDirShort, directory.directory_entry[commonMenu.selected].d_name);
					}else{
                        sprintf(userSettings->selectedBrowserItem, "%s%s", curDir, directory.directory_entry[commonMenu.selected].longname);
                        sprintf(userSettings->selectedBrowserItemShort, "%s%s", curDirShort, directory.directory_entry[commonMenu.selected].d_name);
					}
                    fileBrowserRetValue = MODE_PLAYER;
                    userSettings->previousMode = MODE_FILEBROWSER;
                    exitFlagFileBrowser = 1;
                }
            }else if(!USBactive && osl_pad.released.circle){
                //Up one level:
                char tempDir[264] = "";
                strcpy(tempDir, curDir);
                if (directoryUp(curDir) == 0){
					directoryUp(curDirShort);
                    int i;
                    opendir_close(&directory);
					//debugMessageBox(curDir);
					cpuBoost();
                    result = opendir_open(&directory, curDir, curDirShort, fileExt, fileExtCount, 1);
                    sortDirectory(&directory);
                    buildMenuFromDirectory(&commonMenu, &directory, "");
                    //Mi riposiziono:
                    for (i = 0; i < directory.number_of_directory_entries; i++){
                        if (strstr(tempDir, directory.directory_entry[i].longname) != NULL){
                            commonMenu.first = i;
                            commonMenu.selected = i;
                            break;
                        }
                    }
					cpuRestore();
                }
                //sceKernelDelayThread(200000);
            }else if(osl_pad.released.select){
                //USB activate/deactivate:
                USBactive = !USBactive;
                if (USBactive){
					cpuBoost();

                    int retVal = oslInitUsbStorage();
                    if (retVal){
                        sprintf(buffer, "Error InitUsbStorage: %i", retVal);
                        //debugMessageBox(buffer);
                    }else{
                        oslStartUsbStorage();
                    }
                }else{
                    oslStopUsbStorage();
                    oslDeinitUsbStorage();
					cpuRestore();
                }
                //sceKernelDelayThread(200000);
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
