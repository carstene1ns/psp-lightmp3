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
#include "gui_fileBrowser.h"
#include "common.h"
#include "languages.h"
#include "settings.h"
#include "skinsettings.h"
#include "menu.h"
#include "../system/opendir.h"
#include "../system/usb.h"
#include "../system/clock.h"
#include "../players/player.h"
#include "../players/m3u.h"

#define music_directory_1 "ms0:/MUSIC"
#define music_directory_2 "ms0:/PSP/MUSIC"

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
    oslSetTextColor(RGBA(tempColor[0], tempColor[1], tempColor[2], tempColor[3]));
    oslDrawString((480 - oslGetStringWidth(langGetString("USB_ACTIVE"))) / 2, (272 - popupBkg->sizeY) / 2 + 3, langGetString("USB_ACTIVE"));
    skinGetColor("RGBA_POPUP_MESSAGE_TEXT", tempColor);
    oslSetTextColor(RGBA(tempColor[0], tempColor[1], tempColor[2], tempColor[3]));
    oslDrawString((480 - oslGetStringWidth(langGetString("USB_MESSAGE"))) / 2, (272 - popupBkg->sizeY) / 2 + 40, langGetString("USB_MESSAGE"));
    return 0;
}


///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Add one file to the current playlist:
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void addFileToPlaylist(char *fileName, int save){
	struct fileInfo info;
	char onlyName[264] = "";

	if (save == 1)
		M3U_open(tempM3Ufile);

    setAudioFunctions(fileName, userSettings->MP3_ME);
	(*initFunct)(0);
	if ((*loadFunct)(fileName) == OPENING_OK){
		info = (*getInfoFunct)();
		(*endFunct)();
		if (strlen(info.title)){
			M3U_addSong(fileName, info.length, info.title);
		}else{
			getFileName(fileName, onlyName);
			M3U_addSong(fileName, info.length, onlyName);
		}

		if (save == 1)
			M3U_save(tempM3Ufile);
	}
    unsetAudioFunctions();
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Add a directory to the current playlist:
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void addDirectoryToPlaylist(char *dirName){
	char fileToAdd[264] = "";
	char message[10] = "";
	int i;
	float perc;
	struct opendir_struct dirToAdd;

	char *result = opendir_open(&dirToAdd, dirName, fileExt, fileExtCount, 0);
	if (result == 0){
        M3U_open(tempM3Ufile);
        setCpuClock(222);
        setBusClock(111);
		for (i = 0; i < dirToAdd.number_of_directory_entries; i++){
			perc = ((float)(i + 1) / (float)dirToAdd.number_of_directory_entries) * 100.0;
			snprintf(message, sizeof(message), "%3.3i%% %s", (int)perc, langGetString("DONE"));

            oslStartDrawing();
            drawCommonGraphics();
            drawButtonBar(MODE_FILEBROWSER);
            drawMenu(&commonMenu);
			drawWait(langGetString("ADDING_PLAYLIST"), message);
        	oslEndDrawing();
            oslEndFrame();
        	oslSyncFrame();

			strcpy(fileToAdd, dirName);
			if (fileToAdd[strlen(fileToAdd)-1] != '/')
				strcat(fileToAdd, "/");
			strcat(fileToAdd, dirToAdd.directory_entry[i].d_name);
			addFileToPlaylist(fileToAdd, 0);
		}
		M3U_save(tempM3Ufile);
        setBusClock(userSettings->BUS);
        setCpuClock(userSettings->CLOCK_GUI);
	}
	opendir_close(&dirToAdd);
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// File Browser:
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
int gui_fileBrowser(){
    char buffer[264];
    char curDir[264];
    struct opendir_struct directory;
    char *result;
    int USBactive = 0;

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
    sprintf(buffer, "%s/menuhighlight.png", userSettings->skinImagesPath);
    commonMenu.highlight = oslLoadImageFilePNG(buffer, OSL_IN_RAM | OSL_SWIZZLED, OSL_PF_8888);
    if (!commonMenu.highlight)
        errorLoadImage(buffer);
    commonMenu.width = commonMenu.background->sizeX;
    commonMenu.height = commonMenu.background->sizeY;
    commonMenu.interline = 1;
    commonMenu.maxNumberVisible = commonMenu.background->sizeY / (fontNormal->charHeight + commonMenu.interline);
    commonMenu.cancelFunction = NULL;
    if (!strlen(userSettings->lastBrowserDir)){
		//Provo la prima directory:
		strcpy(curDir, music_directory_1);
		result = opendir_open(&directory, curDir, fileExt, fileExtCount, 1);
		if (result){
			//Provo la seconda directory:
			strcpy(curDir, music_directory_2);
			result = opendir_open(&directory, curDir, fileExt, fileExtCount, 1);
			if (result){
                sprintf(buffer, "\"%s\" and \"%s\" not found or empty", music_directory_1, music_directory_2);
                debugMessageBox(buffer);
                oslQuit();
				return -1;
			}
		}
    }else{
		strcpy(curDir, userSettings->lastBrowserDir);
		result = opendir_open(&directory, curDir, fileExt, fileExtCount, 1);
    }
    sortDirectory(&directory);
    getFileName(userSettings->selectedBrowserItem, buffer);
    buildMenuFromDirectory(&commonMenu, &directory, buffer);

    exitFlagFileBrowser = 0;
    while(!osl_quit && !exitFlagFileBrowser){
        oslStartDrawing();

        drawCommonGraphics();
        drawButtonBar(MODE_FILEBROWSER);
        drawMenu(&commonMenu);

        if (USBactive)
            drawUSBmessage();

        oslReadKeys();
        if (!osl_pad.pressed.hold){
            if (!USBactive)
                processMenuKeys(&commonMenu);

            if (!USBactive && osl_pad.released.cross){
    			if (FIO_S_ISDIR(directory.directory_entry[commonMenu.selected].d_stat.st_mode)){
                    //Enter directory:
    				if (curDir[strlen(curDir)-1] != '/')
    					strcat(curDir, "/");
    				strcat(curDir, directory.directory_entry[commonMenu.selected].d_name);
    				opendir_close(&directory);
    				result = opendir_open(&directory, curDir, fileExt, fileExtCount, 1);
    				sortDirectory(&directory);
    				buildMenuFromDirectory(&commonMenu, &directory, "");
    				sceKernelDelayThread(200000);
    			}else if (FIO_S_ISREG(directory.directory_entry[commonMenu.selected].d_stat.st_mode)){
                    //Play file:
                    if (curDir[strlen(curDir)-1] != '/')
                        sprintf(userSettings->selectedBrowserItem, "%s/%s", curDir, directory.directory_entry[commonMenu.selected].d_name);
                    else
                        sprintf(userSettings->selectedBrowserItem, "%s%s", curDir, directory.directory_entry[commonMenu.selected].d_name);
                    fileBrowserRetValue = MODE_PLAYER;
                    userSettings->previousMode = MODE_FILEBROWSER;
                    exitFlagFileBrowser = 1;
                }
            }else if (!USBactive && osl_pad.released.start){
    			if (curDir[strlen(curDir)-1] != '/')
                    sprintf(buffer, "%s/%s", curDir, directory.directory_entry[commonMenu.selected].d_name);
                else
                    sprintf(buffer, "%s%s", curDir, directory.directory_entry[commonMenu.selected].d_name);
    			if (FIO_S_ISDIR(directory.directory_entry[commonMenu.selected].d_stat.st_mode)){
                    addDirectoryToPlaylist(buffer);
                    continue;
    			}else if (FIO_S_ISREG(directory.directory_entry[commonMenu.selected].d_stat.st_mode))
                    drawWait(langGetString("ADDING_PLAYLIST"), langGetString("WAIT"));
                	oslEndDrawing();
                    oslEndFrame();
                	oslSyncFrame();
                    addFileToPlaylist(buffer, 1);
                    continue;
            }else if(!USBactive && osl_pad.released.square){
                //Play directory:
                if (FIO_S_ISDIR(directory.directory_entry[commonMenu.selected].d_stat.st_mode)){
                    if (curDir[strlen(curDir)-1] != '/')
                        sprintf(userSettings->selectedBrowserItem, "%s/%s", curDir, directory.directory_entry[commonMenu.selected].d_name);
                    else
                        sprintf(userSettings->selectedBrowserItem, "%s%s", curDir, directory.directory_entry[commonMenu.selected].d_name);
                    fileBrowserRetValue = MODE_PLAYER;
                    userSettings->previousMode = MODE_FILEBROWSER;
                    exitFlagFileBrowser = 1;
                }
            }else if(!USBactive && osl_pad.released.circle){
                //Up one level:
                char tempDir[264] = "";
                strcpy(tempDir, curDir);
        		if (directoryUp(curDir) == 0){
                    int i;
        			opendir_close(&directory);
        			result = opendir_open(&directory, curDir, fileExt, fileExtCount, 1);
        			sortDirectory(&directory);
                    buildMenuFromDirectory(&commonMenu, &directory, "");
                    //Mi riposiziono:
                    for (i = 0; i < directory.number_of_directory_entries; i++){
                        if (strstr(tempDir, directory.directory_entry[i].d_name) != NULL){
                            commonMenu.first = i;
                            commonMenu.selected = i;
                            break;
                        }
                    }
        		}
        		sceKernelDelayThread(200000);
            }else if(osl_pad.released.select){
                //USB activate/deactivate:
                USBactive = !USBactive;
                if (USBactive){
                    int retVal = USBInit();
                    if (retVal){
                        sprintf(buffer, "Error USBInit: %i", retVal);
                        debugMessageBox(buffer);
                    }else{
                        retVal = USBActivate();
                        if (retVal){
                            sprintf(buffer, "Error USBActivate: %i", retVal);
                            debugMessageBox(buffer);
                        }
                    }
                }else{
                    USBDeactivate();
                    USBEnd();
                }
                sceKernelDelayThread(200000);
            }else if(!USBactive && osl_pad.released.R){
                fileBrowserRetValue = nextAppMode(MODE_FILEBROWSER);
                exitFlagFileBrowser = 1;
            }else if(!USBactive && osl_pad.released.L){
                fileBrowserRetValue = previousAppMode(MODE_FILEBROWSER);
                exitFlagFileBrowser = 1;
            }
        }
    	oslEndDrawing();
        oslEndFrame();
    	oslSyncFrame();
    }
    opendir_close(&directory);
    //unLoad images:
    clearMenu(&commonMenu);

    strcpy(userSettings->lastBrowserDir, curDir);
    return fileBrowserRetValue;
}
