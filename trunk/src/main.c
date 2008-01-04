//    LightMP3
//    Copyright (C) 2007 Sakya
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
#include <pspctrl.h>
#include <psphprm.h>
#include <pspdebug.h>
#include <pspaudio.h>
#include <psppower.h>
#include <pspsdk.h>

#include <time.h>
#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include <unistd.h>

#include "players/pspaudiolib.h"
#include "players/player.h"

//#include "osk.h"
#include "exception.h"
#include "usb.h"
#include "settings.h"
#include "opendir.h"
#include "m3u.h"
#include "equalizer.h"
#include "audioscrobbler.h"

//#include "log.h"

PSP_MODULE_INFO("LightMP3", 0, 1, 0);
PSP_MAIN_THREAD_ATTR(THREAD_ATTR_USER);
PSP_HEAP_SIZE_KB(22000);

// Functions imported from support prx:
int displayEnable(void);
int displayDisable(void);
int getBrightness();
void setBrightness(int brightness);
int imposeSetBrightness(int value);
int imposeGetBrightness();
int imposeGetVolume();
int imposeSetVolume();
void MEDisable();
void readButtons(SceCtrlData *pad_data, int count);
int imposeGetMute();
int imposeSetMute(int value);
int imposeSetHomePopup(int value);
int getModuleListID(SceUID modid[100], int *modcount);
int stopUnloadModule(SceUID modID);

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Constants:
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//Colori:
#define RGB(r, g, b) ((b << 16) | (g << 8) | r)
#define YELLOW RGB(255, 255, 0)
#define ORANGE RGB(255, 165, 0)
#define BLACK RGB(0, 0, 0)
#define WHITE RGB(255, 255, 255)
#define RED RGB(255, 0, 0)
#define DEEPSKYBLUE RGB(0, 178, 238)

//Altre costanti
#define music_directory_1 "ms0:/MUSIC"
#define music_directory_2 "ms0:/PSP/MUSIC"

//Costanti per playing mode:
#define MODE_NORMAL 0
#define MODE_REPEAT_ALL 1
#define MODE_REPEAT 2
#define MODE_SHUFFLE 3

//Sensibilità analogico:
#define ANALOG_SENS 80

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Globals:
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
int current_mode;
char version[10] = "1.7.2";
int oldBrightness = 0;
char ebootDirectory[262] = "";
char currentPlaylist[262] = "";
int playListModified;
char tempM3Ufile[262] = "";
char fileBrowserDir[262] = "";
char playlistDir[262] = "";
char scrobblerLog[262] = "";
int playingMode = 0;
char playingModeDesc[4][13] = {"Normal ", "Repeat all", "Repeat track", "Shuffle"};
int volumeBoost = 0;

int curBrightness = 0;
int displayStatus = 1;
int currentEQ = 0;

static int runningFlag = 1;
struct settings userSettings;
static int resuming = 0;
static int suspended = 0;

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Power Callback:
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
int powerCallback(int unknown, int powerInfo, void *common){
    if ((powerInfo & PSP_POWER_CB_SUSPENDING) || (powerInfo & PSP_POWER_CB_STANDBY) || (powerInfo & PSP_POWER_CB_POWER_SWITCH)){
       if (!suspended){
           resuming = 1;                   
           //Suspend
           if (suspendFunct != NULL){
              (*suspendFunct)();
           }
           //Riattivo il display:
           if (!displayStatus){
               displayEnable();
               setBrightness(curBrightness);
               displayStatus = 1;
           }
           //pspAudioEnd();
           suspended = 1;
       }
    }else if ((powerInfo & PSP_POWER_CB_RESUMING)){
       resuming = 1;
    }else if ((powerInfo & PSP_POWER_CB_RESUME_COMPLETE)){
       //Resume
       //pspAudioInit();
       if (resumeFunct != NULL)
          (*resumeFunct)();
       resuming = 0;       
       suspended = 0;       
    }
    return 0;
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Funzioni generiche
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//Check running modules:
int checkModules(){
    SceUID modid[100];
    int modcount;
    getModuleListID(modid, &modcount);
    SceKernelModuleInfo minfo;
    int i;
    for(i = 0; i < modcount; i++){
        memset(&minfo, 0, sizeof(SceKernelModuleInfo));
        minfo.size = sizeof(SceKernelModuleInfo);
        sceKernelQueryModuleInfo(modid[i], &minfo);
        if (!strcmp(minfo.name, "Music_prx"))
            stopUnloadModule(modid[i]);
    }
    return 0;
}

//Memoria libera totale:
typedef struct
{
      void *buffer;
      void *next;
} _LINK;

int freemem(){
   int size = 4096, total = 0;
    _LINK *first = NULL, *current = NULL, *lnk;

    while(runningFlag){
       lnk = (_LINK*)malloc(sizeof(_LINK));
       if (!lnk)
          break;

       total += sizeof(_LINK);

       lnk->buffer = malloc(size);
       if (!lnk->buffer){
          free(lnk);
          break;
       }

       total += size;
       lnk->next = NULL;

       if (current){
          current->next = (void*)lnk;
          current = lnk;
       } else {
          current = first = lnk;
       }
    }

    lnk = first;
    while (lnk){
       free(lnk->buffer);
       current = lnk->next;
       free(lnk);
       lnk = current;
    }
    return total;
}

//Modalità precedente/successiva:
void nextMode(){
	if (++current_mode > 3){
		current_mode = 0;
	}
}
void previousMode(){
	if (--current_mode < 0){
		current_mode = 3;
	}
}

//Prendo solo il nome del file/directory:
void getFileName(char *fileName, char *onlyName){
	int slash = -1;
	int retCount;

	strcpy(onlyName, fileName);
	//Cerco l'ultimo slash:
	int i = 0;
	for (i = strlen(fileName) - 1; i >= 0; i--){
		if ((fileName[i] == '/') & (i != strlen(fileName))){
			slash = i;
			break;
		}
	}
	if (slash){
		retCount = 0;
		for (i = slash + 1; i < strlen(fileName); i++){
			onlyName[retCount] = fileName[i];
			retCount++;
		}
		onlyName[retCount] = '\0';
	}else{
		strcpy(onlyName, fileName);
	}
}

//Calcolo livello superiore:
int directoryUp(char *dirName)
{
	if (dirName != "ms0:/"){
		//Cerco l'ultimo slash:
		int i = 0;
		for (i = strlen(dirName) - 1; i >= 0; i--){
			if ((dirName[i] == '/') & (i != strlen(dirName))){
				if (i > 4){
					dirName[i] = '\0';
				}else{
					dirName[i+1] = '\0';
				}
				break;
			}
		}
		return(0);
	}else{
		return(-1);
	}
}

//Spezzo autore e titolo nel nome di una directory:
void rtrim(char *toTrim)
{
	int i = 0;
	for (i = strlen(toTrim) - 1; i >= 0; i--){
		if (isspace(toTrim[i])){
			toTrim[i] = '\0';
		}else{
			break;
		}
	}
}

int splitAuthorTitle(char *dirName, char *author, char *title)
{
	if (strstr(dirName, "-") != NULL){
		int i = 0;
		int posSep = -1;
		int countAuthor = 0;
		int countTitle = 0;

		for (i = 0; i < strlen(dirName); i++){
			if (dirName[i] == '-'){
				posSep = i;
			}
			else if (posSep == -1){
				if ((countAuthor) | (dirName[i] != ' ')){
					author[countAuthor] = dirName[i];
					countAuthor++;
				}
			}else{
				if ((countTitle) | (dirName[i] != ' ')){
					title[countTitle] = dirName[i];
					countTitle++;
				}
			}
			author[countAuthor] = '\0';
			title[countTitle] = '\0';
		}
		rtrim(author);
		rtrim(title);
		return(1);
	}
	return(0);
}

//Costruisco la barra della percentuale:
void buildProgressBar(int *barLen, char *pBar, int *percentage){
	int i;
    double singlePerc = 0;

    singlePerc = 100.0 / (*barLen - 2);
    *pBar = '[';
	for (i = 1; i < *barLen - 1; i++){
        pBar++;
        if ((*percentage >= (i-1) * singlePerc) && *percentage > 0){
            *pBar = '*';
        }else{
            *pBar = ' ';
        }
    }
    *(++pBar) = ']';
    *(++pBar) = '\0';
}

//Chiedo conferma di una azione:
int confirm(char *message, u32 backColor){         
	int i;
	pspDebugScreenSetBackColor(backColor);

	for (i = 0; i < 5; i++){
		pspDebugScreenSetXY(15, 10+i);
		pspDebugScreenPrintf("%-38.38s", "");
		if (i == 0){
			pspDebugScreenSetTextColor(RED);
			pspDebugScreenSetXY(15 + (38 - strlen("Confirm")) / 2, 10+i);
			pspDebugScreenPrintf("%s", "Confirm");
		}else if (i == 2){
			pspDebugScreenSetTextColor(WHITE);
			pspDebugScreenSetXY(15 + (38 - strlen(message)) / 2, 10+i);
			pspDebugScreenPrintf("%s", message);
		}else if (i == 4){
			pspDebugScreenSetTextColor(YELLOW);
			pspDebugScreenSetXY(15, 10+i);
			pspDebugScreenPrintf(" Press X to confirm, CIRCLE to cancel ");
		}
	}
	sceKernelDelayThread(200000);
	SceCtrlData pad;
	while(runningFlag){
		readButtons(&pad, 1);
		if (pad.Buttons & PSP_CTRL_CROSS){
			sceKernelDelayThread(200000);
			return(1);
		} else if (pad.Buttons & PSP_CTRL_CIRCLE) {
			sceKernelDelayThread(200000);
			return(0);
		}
	}
	return(0);
}


//Draw a waiting message:
void waitMessage(char *message){
	int i;
	int max = 0;
	if (strlen(message) == 0){
		max = 3;
	}else{
		max = 4;
	}
	pspDebugScreenSetBackColor(BLACK);
	for (i=0; i < max; i++){
		pspDebugScreenSetXY(19, 10+i);
		pspDebugScreenPrintf("%-30.30s", "");
		if (i == 1){
			pspDebugScreenSetTextColor(YELLOW);
			pspDebugScreenSetXY(19 + (30 - strlen("Please wait...")) / 2, 10+i);
			pspDebugScreenPrintf("%s", "Please wait...");
		}else if (i == 2 && strlen(message) > 0){
			pspDebugScreenSetTextColor(WHITE);
			pspDebugScreenSetXY(19 + (30 - strlen(message)) / 2, 10+i);
			pspDebugScreenPrintf("%s", message);
		}
	}
}

//Insert a space in a string:
int insertSpace(char *string, int pos){
	int i;
	int count = 0;
	char tempStr[strlen(string)];

	for (i=0; i<strlen(string); i++){
		if (i == pos){
			tempStr[count] = ' ';
			count++;
		}
		tempStr[count] = string[i];
		count++;
	}
	tempStr[count] = '\0';
	strcpy(string, tempStr);
	return(0);
}

//Remove a char from a string:
int removeChar(char *string, int pos){
	int i;
	int count = 0;
	char tempStr[strlen(string)];

	for (i=0; i<strlen(string); i++){
		if (i == pos){
			continue;
		}
		tempStr[count] = string[i];
		count++;
	}
	tempStr[count] = '\0';
	strcpy(string, tempStr);
	return(0);
}

//Ask for a string (for playlist name)
int askString(char *string){
	int i;
	int cursorPos = 0;
	int changed = 0;
	char character[2] = "";
	char oldName[262] = "";
	int asciiCode = (int)' ';

	//Salvo il nome:
	strcpy(oldName, string);

    /*if (userSettings.USE_OSK){
        string = requestString(oldName);
        return 0;
    }*/
        
	pspDebugScreenSetBackColor(BLACK);
	for (i=0; i < 5 ; i++){
		pspDebugScreenSetXY(11, 10+i);
		pspDebugScreenPrintf("%-46.46s", "");
	}
	pspDebugScreenSetTextColor(RED);
	pspDebugScreenSetXY(11 + (46 - strlen("Insert playlist name")) / 2, 10);
	pspDebugScreenPrintf("%s", "Insert playlist name");
	pspDebugScreenSetTextColor(YELLOW);
	pspDebugScreenSetXY(11, 13);
	pspDebugScreenPrintf("Press X to insert space, SQUARE to delete char");
	pspDebugScreenSetXY(11, 14);
	pspDebugScreenPrintf("   Press START to confirm, SELECT to cancel");
	sceKernelDelayThread(200000);
	SceCtrlData pad;
	//Primo carattere:
	if (strlen(string) == 0){
		strcpy(string, " ");
		asciiCode = (int)' ';
	}else{
		asciiCode = (int)string[0];
	}
	changed = 1;

	while(runningFlag){
		if (changed == 1){
			changed = 0;
			//String:
			pspDebugScreenSetXY(11, 12);
			pspDebugScreenSetTextColor(WHITE);
			pspDebugScreenSetBackColor(BLACK);
			pspDebugScreenPrintf("%-46.46s", string);
			//Cursor:
			pspDebugScreenSetBackColor(WHITE);
			pspDebugScreenSetTextColor(BLACK);
			pspDebugScreenSetXY(11 + cursorPos, 12);
			character[0] = (char)asciiCode;
			pspDebugScreenPrintf(character);
		}
		readButtons(&pad, 1);
		if (pad.Buttons & PSP_CTRL_START){
			//Conferma nome:
			rtrim(string);
			sceKernelDelayThread(200000);
			if (strlen(string) == 0){
				return(-1);
			}
			break;
		}else if (pad.Buttons & PSP_CTRL_SELECT){
			//Restore del nome:
			strcpy(string, oldName);
			sceKernelDelayThread(200000);
			return(-1);
		}else if (pad.Buttons & PSP_CTRL_CROSS){
			//Inserisco uno spazio:
			if (strlen(string) <= 45){
				insertSpace(string, cursorPos);
				asciiCode = (int)' ';
				changed = 1;
				sceKernelDelayThread(200000);
			}
		}else if (pad.Buttons & PSP_CTRL_SQUARE){
			//Cancello un carattere:
			removeChar(string, cursorPos);
			if (cursorPos > strlen(string) - 1){
				if (strlen(string) > 0){
					cursorPos = strlen(string) - 1;
					asciiCode = (int)string[cursorPos];
				}else{
					cursorPos = 0;
					strcpy(string, " ");
					asciiCode = (int)' ';
				}
			}else{
				asciiCode = (int)string[cursorPos];
			}
			changed = 1;
			sceKernelDelayThread(200000);
		}else if ((pad.Buttons & PSP_CTRL_RIGHT) | (pad.Lx > 128 + ANALOG_SENS && !(pad.Buttons & PSP_CTRL_HOLD))){
			if (cursorPos < 45){
				cursorPos++;
				if (cursorPos < strlen(string) - 1){
					asciiCode = (int)string[cursorPos];
				}else{
					asciiCode = (int)' ';
					strcat(string, " ");
				}
				changed = 1;
				sceKernelDelayThread(200000);
			}
		}else if ((pad.Buttons & PSP_CTRL_LEFT) | (pad.Lx < 128 - ANALOG_SENS && !(pad.Buttons & PSP_CTRL_HOLD))){
			if (cursorPos > 0){
				cursorPos--;
				if (cursorPos < strlen(string) - 1){
					asciiCode = (int)string[cursorPos];
				}else{
					asciiCode = (int)' ';
				}
				changed = 1;
			}
			sceKernelDelayThread(200000);
		} else if((pad.Buttons & PSP_CTRL_UP) | (pad.Ly < 128 - ANALOG_SENS && !(pad.Buttons & PSP_CTRL_HOLD))) {
			if (asciiCode < (int)' ' + 94){
				asciiCode++;
				string[cursorPos] = (char)asciiCode;
			}else{
				asciiCode = (int)' ';
			}
			changed = 1;
			sceKernelDelayThread(120000);
		} else if((pad.Buttons & PSP_CTRL_DOWN) | (pad.Ly > 128 + ANALOG_SENS && !(pad.Buttons & PSP_CTRL_HOLD))) {
			if (asciiCode > (int)' '){
				asciiCode--;
				string[cursorPos] = (char)asciiCode;
			}else{
				asciiCode = (int)' ' + 94;
			}
			changed = 1;
			sceKernelDelayThread(120000);
		}
	}
	return(0);
}

//Random track for shuffle:
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

//Aggiungo un file alla playlist temporanea:
void addFileToPlaylist(char *fileName, int save){
	struct fileInfo info;
	char onlyName[262] = "";

    setAudioFunctions(fileName, userSettings.MP3_ME);
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

		if (save == 1){
			M3U_save(tempM3Ufile);
		}
	}
}


//Aggiungo una directory alla playlist temporanea:
void addDirectoryToPlaylist(char *dirName){
	char ext[6][5] = {"MP3", "OGG", "AA3", "OMA", "OMG", "FLAC"};
	char fileToAdd[262] = "";
	char message[10] = "";
	int i;
	float perc;
	struct opendir_struct dirToAdd;

	char *result = opendir_open(&dirToAdd, dirName, ext, 6, 0);
	if (result == 0){
		for (i = 0; i < dirToAdd.number_of_directory_entries; i++){
			strcpy(fileToAdd, dirName);
			if (fileToAdd[strlen(fileToAdd)-1] != '/'){
				strcat(fileToAdd, "/");
			}
			strcat(fileToAdd, dirToAdd.directory_entry[i].d_name);
			addFileToPlaylist(fileToAdd, 0);
			perc = ((float)(i + 1) / (float)dirToAdd.number_of_directory_entries) * 100.0;
			snprintf(message, sizeof(message), "%3.3i%% done", (int)perc);
			waitMessage(message);
		}
		M3U_save(tempM3Ufile);
	}
	opendir_close(&dirToAdd);
}


//Check brightness and warning:
void checkBrightness(){
	int i;
    curBrightness = getBrightness();

	if (curBrightness > 24){
		pspDebugScreenSetTextColor(WHITE);
		pspDebugScreenSetBackColor(0xaa4400);
		pspDebugScreenInit();

		pspDebugScreenSetBackColor(BLACK);
		for (i = 0; i < 5; i++){
			pspDebugScreenSetXY(15, 13+i);
			pspDebugScreenPrintf("%-38.38s", "");
			if (i == 0){
				pspDebugScreenSetTextColor(RED);
				pspDebugScreenSetXY(15 + (38 - strlen("Brightness warning")) / 2, 13+i);
				pspDebugScreenPrintf("%s", "Brightness warning");
			}else if (i == 2){
				pspDebugScreenSetTextColor(WHITE);
				pspDebugScreenSetXY(15 + (38 - strlen("Lower display brightness?")) / 2, 13+i);
				pspDebugScreenPrintf("%s", "Lower display brightness?");
			}else if (i == 4){
				pspDebugScreenSetTextColor(YELLOW);
				pspDebugScreenSetXY(15, 13+i);
				pspDebugScreenPrintf(" Press X to confirm, CIRCLE to cancel ");
			}
		}
		sceKernelDelayThread(200000);
		SceCtrlData pad;
		while(runningFlag){
			readButtons(&pad, 1);
			if (pad.Buttons & PSP_CTRL_CROSS){
                setBrightness(24);
				curBrightness = 24;
                imposeSetBrightness(0);
				break;
			} else if (pad.Buttons & PSP_CTRL_CIRCLE) {
				break;
			}
		}
	}
}

//Activate USB:
int showUSBactivate(){
	int i;

	//init USB:
    int retUSB = USBinit();
	if (retUSB){
        pspDebugScreenSetXY(0, 4);
        pspDebugScreenSetTextColor(RED);
        pspDebugScreenPrintf("Error %i initializing USB.\n", retUSB);
        sceKernelDelayThread(3000000);
	}

    USBActivate();
	pspDebugScreenSetBackColor(BLACK);
	for (i = 0; i < 4; i++){
		pspDebugScreenSetXY(15, 10+i);
		pspDebugScreenPrintf("%-38.38s", "");
		if (i == 0){
			pspDebugScreenSetTextColor(RED);
			pspDebugScreenSetXY(15 + (38 - strlen("USB Activated")) / 2, 10+i);
			pspDebugScreenPrintf("%s", "USB Activated");
		}else if (i == 3){
			pspDebugScreenSetTextColor(YELLOW);
			pspDebugScreenSetXY(15 + (38 - strlen("Press SELECT to deactivate USB")) / 2, 10+i);
			pspDebugScreenPrintf("%s", "Press SELECT to deactivate USB");
		}
	}
	sceKernelDelayThread(300000);
	SceCtrlData pad;
	while(runningFlag){
		readButtons(&pad, 1);
		if (pad.Buttons & PSP_CTRL_SELECT){
            USBDeactivate();
			sceKernelDelayThread(200000);
			break;
		}
	}
	USBEnd();
	return(0);
}


//Draw a volume bar:
void drawVolumeBar(x, y){
	//Volume bar:
	char pBar[33] = "";
	int barLength = 32;
	int percentage = (int)((double)imposeGetVolume() / 30.0 * 100.0);
	buildProgressBar(&barLength, pBar, &percentage);
	pspDebugScreenSetTextColor(WHITE);
	pspDebugScreenSetXY(x, y);
	pspDebugScreenPrintf("%s", pBar);
}


//Confirm exit:
int exitScreen(){
    int ret = confirm("Exit LightMP3?", 0x882200);
    if (ret)
       runningFlag = 0;
    return ret;
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Funzioni gestione BUS & CLOCK
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void setBusClock(int bus){
    if (bus >= 54 && bus <= 111 && sceKernelDevkitVersion() < 0x03070110)
        scePowerSetBusClockFrequency(bus);
}

void setCpuClock(int cpu){
    if (cpu >= 10 && cpu <= 266){
        if (sceKernelDevkitVersion() < 0x03070110){
            scePowerSetCpuClockFrequency(cpu);
            if (scePowerGetCpuClockFrequency() < cpu)
                scePowerSetCpuClockFrequency(++cpu);
        }else{
            scePowerSetClockFrequency(cpu, cpu, cpu/2);
            if (scePowerGetCpuClockFrequency() < cpu){
                cpu++;
                scePowerSetClockFrequency(cpu, cpu, cpu/2);
            }
        }
    }
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Inizializzazioni video:
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//Stringa azione corrente del player:
void screen_playerAction(int *action){
	int currentSpeed = 0;
	pspDebugScreenSetXY(0, 22);
	pspDebugScreenSetTextColor(WHITE);
    pspDebugScreenSetBackColor(BLACK);

	switch (*action){
	case -1:
		pspDebugScreenPrintf("%-20.20s", "Opening...");
		break;
	case 0:
		pspDebugScreenPrintf("%-20.20s", "Paused");
		break;
	case 1:
		currentSpeed = (*getPlayingSpeedFunct)();
		if (currentSpeed == 0){
			pspDebugScreenPrintf("%-20.20s", "Playing...");
		}else if (currentSpeed > 0){
			pspDebugScreenPrintf("Playing %ix...", currentSpeed + 1);
		}else if (currentSpeed < 0){
			pspDebugScreenPrintf("Playing %ix...", currentSpeed);
		}
		break;
	}

}

// Stringa impostazioni player:
void screen_playerSettings(){
	char mode[3] = "";
	struct equalizer tEQ;

	tEQ = EQ_getIndex(currentEQ);
	if (playingMode == MODE_NORMAL){
		mode[0] = 'N';
	}else if (playingMode == MODE_REPEAT_ALL){
        strcpy(mode, "RA");
	}else if (playingMode == MODE_REPEAT){
		mode[0] = 'R';
    }else if (playingMode == MODE_SHUFFLE){
		mode[0] = 'S';
	}
	pspDebugScreenSetBackColor(0x882200);
	pspDebugScreenSetTextColor(RED);
	pspDebugScreenSetXY(24, 33);
	pspDebugScreenPrintf("Mode:%-2.2s Boost:%i EQ:%-3.3s", mode, volumeBoost, tEQ.shortName);
}

//System info:
void screen_sysinfo(){
    int m;
    int h;
    int btrh;
    int btrm;
    int btr;
    int rh;
	int mem;
	int battPerc;
    char headPhones[3] = "";
    char remote[3] = "";

	btr = scePowerGetBatteryLifeTime();
    h = 60;
    m = 60;
    rh = ((btr/60)/60);
    btrm = (btr%m);
    btrh = (btr/h + rh);

	battPerc = scePowerGetBatteryLifePercent();
	if (battPerc < 0){
		battPerc = 0;
	}
	if (btrh < 0 || btrm < 0){
		btrh = 0;
		btrm = 0;
	}

    //Check for headphones and remote controller:
    if (sceHprmIsHeadphoneExist())
        strcpy(headPhones, "HP");

    if (sceHprmIsRemoteExist())
        strcpy(remote, "RM");

	pspDebugScreenSetTextColor(WHITE);
	pspDebugScreenSetBackColor(0x882200);
	pspDebugScreenSetXY(25, 0);
	if (!scePowerIsPowerOnline())
		pspDebugScreenPrintf("[CPU:%i BUS:%i BATT:%i%% - %2.2i:%2.2i] ", scePowerGetCpuClockFrequency(), scePowerGetBusClockFrequency(), battPerc, btrh, btrm);
	else
		pspDebugScreenPrintf("[CPU:%i BUS:%i BATT:%i%% -PLUGGED] ", scePowerGetCpuClockFrequency(), scePowerGetBusClockFrequency(), battPerc);

	pspDebugScreenSetTextColor(RED);
	pspDebugScreenSetXY(60, 0);
	pspDebugScreenPrintf("%2.2s %2.2s", headPhones, remote);

	//Free memory:
	pspDebugScreenSetTextColor(WHITE);
	mem = freemem() / 1024;
	pspDebugScreenSetXY(0, 33);
	pspDebugScreenPrintf("Free mem: %i kb   ", mem);
	//Current time:
	struct tm * ptm;
	time_t mytime;
	time(&mytime);
	ptm = localtime(&mytime);
	pspDebugScreenSetXY(51, 33);
	pspDebugScreenPrintf("%2.2d/%2.2d/%4.4d %2.2d:%2.2d",ptm->tm_mday, ptm->tm_mon + 1, ptm->tm_year + 1900, ptm->tm_hour,ptm->tm_min);
	screen_playerSettings();
}

//Inizializzazione video:
void screen_init(){
	char testo[68] = "";
	pspDebugScreenSetTextColor(WHITE);
	pspDebugScreenSetBackColor(BLACK);
	pspDebugScreenInit();
	pspDebugScreenSetBackColor(0x882200);
	//Righe sopra e sotto:
	pspDebugScreenSetXY(0, 0);
	snprintf(testo, sizeof(testo), "LightMP3 v%s by Sakya", version);
	pspDebugScreenPrintf("%-67.67s",testo);
	pspDebugScreenSetXY(0, 33);
	pspDebugScreenPrintf("%-67.67s", "");
	screen_sysinfo();
	//Riga descrizione:
	pspDebugScreenSetTextColor(WHITE);
	pspDebugScreenSetBackColor(BLACK);
	pspDebugScreenSetXY(0, 1);
	pspDebugScreenPrintf("a light MP3/OGG/FLAC/ATRAC3 Player to preserve your battery. ;)");
	pspDebugScreenSetXY(0, 33);
}

//Toolbar:
void screen_toolbar(){
    //<<L___File Browser____Playlists____Playlist Editor____Options___R>>
    char tb[68] = "<<L                                                             R>>";
    char tbElement[4][30] = {"File browser", "Playlists", "Playlist editor", "Options"};
    int tbElementPos[4] = {7, 23, 36, 55};
    int i;

    pspDebugScreenSetBackColor(0x882200);
	pspDebugScreenSetTextColor(WHITE);
	pspDebugScreenSetXY(0, 2);
	pspDebugScreenPrintf(tb);

    for (i=0; i<4; i++){
        if (i==current_mode)
	       pspDebugScreenSetBackColor(RED);
        else
            pspDebugScreenSetBackColor(0x882200);
        pspDebugScreenSetXY(tbElementPos[i], 2);
        pspDebugScreenPrintf(tbElement[i]);
    }
}

//Schermo per menu
void screen_menu_init(){
	pspDebugScreenSetBackColor(BLACK);
	pspDebugScreenSetTextColor(WHITE);
	pspDebugScreenSetXY(0, 28);
	pspDebugScreenPrintf("Press X to enter directory/play file");
	pspDebugScreenPrintf("\n");
	pspDebugScreenPrintf("Press SQUARE to play directory");
	pspDebugScreenPrintf("\n");
	pspDebugScreenPrintf("Press O to go up one level");
	pspDebugScreenPrintf("\n");
	pspDebugScreenPrintf("Press START to add file/directory to playlist");
	pspDebugScreenPrintf("\n");
	pspDebugScreenPrintf("Press SELECT to activate USB");

}

//Schermo per playlist
void screen_playlist_init(){
	pspDebugScreenSetBackColor(BLACK);
	pspDebugScreenSetTextColor(WHITE);
	pspDebugScreenSetXY(0, 28);
	pspDebugScreenPrintf("Press X to play playlist");
	pspDebugScreenPrintf("\n");
	pspDebugScreenPrintf("Press SQUARE to load playlist");
	pspDebugScreenPrintf("\n");
	pspDebugScreenPrintf("Press CIRCLE to remove playlist");
}

//Schermo per playlist editor
void screen_playlistEditor_init(){
	pspDebugScreenSetTextColor(YELLOW);
	pspDebugScreenSetBackColor(BLACK);
	pspDebugScreenSetXY(0, 22);
	pspDebugScreenPrintf("Artist   : %-30.30s", "");
	pspDebugScreenPrintf("\n");
	pspDebugScreenPrintf("Title    : %-30.30s", "");
	pspDebugScreenPrintf("\n");
	pspDebugScreenPrintf("Album    : %-30.30s", "");
	pspDebugScreenSetBackColor(BLACK);
	pspDebugScreenSetTextColor(WHITE);
	pspDebugScreenSetXY(0, 27);
	pspDebugScreenPrintf("Press X to move track down");
	pspDebugScreenPrintf("\n");
	pspDebugScreenPrintf("Press SQUARE to move track up");
	pspDebugScreenPrintf("\n");
	pspDebugScreenPrintf("Press CIRCLE to remove track from playlist");
	pspDebugScreenPrintf("\n");
	pspDebugScreenPrintf("Press START to save playlist");
	pspDebugScreenPrintf("\n");
	pspDebugScreenPrintf("Press SELECT to clear playlist");
	pspDebugScreenPrintf("\n");
	pspDebugScreenPrintf("Press NOTE to play the playlist");
}

//Schermo per player
void screen_player_tagInfo(struct fileInfo tag){
    pspDebugScreenSetTextColor(YELLOW);
    pspDebugScreenSetXY(0, 11);
    pspDebugScreenPrintf("Album      : %-54.54s", tag.album);
	pspDebugScreenPrintf("\n");
	pspDebugScreenPrintf("Title      : %-54.54s", tag.title);
	pspDebugScreenPrintf("\n");
    pspDebugScreenPrintf("Artist     : %-54.54s", tag.artist);
	pspDebugScreenPrintf("\n");
    pspDebugScreenPrintf("Year       : %s", tag.year);
	pspDebugScreenPrintf("\n");
	pspDebugScreenPrintf("Genre      : %-54.54s", tag.genre);
}

void screen_player_init(char *filename, struct fileInfo tag, char *numbers){
	char file[262] = "";
	pspDebugScreenSetTextColor(WHITE);
	pspDebugScreenSetBackColor(BLACK);
    pspDebugScreenSetXY(0, 7);
    pspDebugScreenPrintf("File Number: %s", numbers);
    pspDebugScreenSetXY(0, 8);
	getFileName(filename, file);
	pspDebugScreenPrintf("File Name  : %-54.54s", file);
	pspDebugScreenPrintf("\n");
	pspDebugScreenPrintf("Time       : 00:00:00 / 00:00:00");
	pspDebugScreenPrintf("\n");
	pspDebugScreenPrintf("             [                    ]");
	screen_player_tagInfo(tag);

	pspDebugScreenSetTextColor(DEEPSKYBLUE);
	pspDebugScreenSetXY(0, 16);
    pspDebugScreenPrintf("Layer      :");
	pspDebugScreenPrintf("\n");
    pspDebugScreenPrintf("Bitrate    :");
	pspDebugScreenPrintf("\n");
    pspDebugScreenPrintf("Sample rate:");
	pspDebugScreenPrintf("\n");
    pspDebugScreenPrintf("Mode       :");
	pspDebugScreenPrintf("\n");
    pspDebugScreenPrintf("Emphasis   :");

	pspDebugScreenSetTextColor(WHITE);
	pspDebugScreenSetXY(0, 25);
    pspDebugScreenPrintf("Volume     : ");
	drawVolumeBar(13,25);

	pspDebugScreenSetXY(0, 27);
	pspDebugScreenPrintf("Press X to pause, press O to stop");
	pspDebugScreenPrintf("\n");
	pspDebugScreenPrintf("Press SQUARE to toggle mute, press SELECT to switch mode");
    pspDebugScreenPrintf("\n");
    pspDebugScreenPrintf("Press L/R to change track");
	pspDebugScreenPrintf("\n");
	pspDebugScreenPrintf("Press UP/DOWN to change volume boost value");
	pspDebugScreenPrintf("\n");
	pspDebugScreenPrintf("Press START to turn on/off the display");
	pspDebugScreenPrintf("\n");
	pspDebugScreenPrintf("Use analog to change clock (UP/DOWN for cpu, LEFT/RIGHT for bus)");
}

//Informazioni sul file:
void screen_fileinfo(struct fileInfo info){
    pspDebugScreenSetXY(0, 16);
	pspDebugScreenSetTextColor(DEEPSKYBLUE);
    pspDebugScreenPrintf("Layer      : %s", info.layer);
	pspDebugScreenPrintf("\n");
    pspDebugScreenPrintf("Bitrate    : %i [%3.3i] kbit", info.kbit, info.kbit);
	pspDebugScreenPrintf("\n");
    pspDebugScreenPrintf("Sample rate: %li Hz", info.hz);
	pspDebugScreenPrintf("\n");
    pspDebugScreenPrintf("Mode       : %s", info.mode);
	pspDebugScreenPrintf("\n");
    pspDebugScreenPrintf("Emphasis   : %s", info.emphasis);
}

//Init for options screen:
void screen_options_init(){
	pspDebugScreenSetBackColor(BLACK);
	pspDebugScreenSetTextColor(WHITE);
	pspDebugScreenSetXY(0, 26);
	pspDebugScreenPrintf("LightMP3 is ditributed under GNU General Public License,\n");
    pspDebugScreenPrintf("read LICENSE.TXT for details.");
	pspDebugScreenSetXY(0, 30);
	pspDebugScreenPrintf("Press LEFT or RIGHT to change current option");
	pspDebugScreenPrintf("\n");
	pspDebugScreenPrintf("Press START to save options");
    pspDebugScreenPrintf("\n");
    pspDebugScreenPrintf("Press SELECT to restore default values");
	pspDebugScreenPrintf("\n");
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//Play a single file:
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
int playFile(char *filename, char *numbers, char *message) {
	  /* Play a single file.
		 Returns 1 if user pressed NEXT
				 0 if song ended
				-1 if user pressed PREVIOUS
				 2 if user pressed STOP
	  */
	  u32 remoteButtons;
	  int retVal = 0;
	  SceCtrlData pad;
	  int clock;
	  int bus;
	  int action = 0; //-1=open 0=paused 1=playing
	  char timestring[20] = "";
	  int updateInfo = 0;
	  struct fileInfo info;
	  char pBar[23] = "";
	  int muted = 0;
	  struct equalizer tEQ;
	  int lastPercentage = 0;
	  int lastPercentageDrawn = -1;
      int retLoad = 0;
      int lastVolume = -1;
   	  int currentSpeed = 0;
      struct fileInfo tempInfo;
      int barLength = 22;

	  screen_init();
	  info.kbit = 0;
	  info.hz = 0;

      setAudioFunctions(filename, userSettings.MP3_ME);
      //Tipo di volume boost:
      if (strcmp(userSettings.BOOST, "OLD") == 0){
          (*setVolumeBoostTypeFunct)("OLD");
      }else{
          (*setVolumeBoostTypeFunct)("NEW");
      }
      if (volumeBoost > MAX_VOLUME_BOOST)
          volumeBoost = MAX_VOLUME_BOOST;
      if (volumeBoost < MIN_VOLUME_BOOST)
          volumeBoost = MIN_VOLUME_BOOST;

      //Equalizzatore:
      if (!(*isFilterSupportedFunct)()){
          currentEQ = 0;
      }

      tEQ = EQ_getIndex(currentEQ);
      (*setFilterFunct)(tEQ.filter, 1);
      if (!currentEQ){
          (*disableFilterFunct)();
      }else{
          (*enableFilterFunct)();
      }
	  clock = scePowerGetCpuClockFrequency();
	  bus = scePowerGetBusClockFrequency();
	  //Leggo le informazioni del TAG:
      tempInfo = (*getTagInfoFunct)(filename);
	  screen_player_init(filename, tempInfo, numbers);
	  //Testo del modo di riproduzione:
	  pspDebugScreenSetXY(0, 4);
	  pspDebugScreenSetBackColor(BLACK);
	  pspDebugScreenSetTextColor(YELLOW);
	  pspDebugScreenPrintf("Play mode  : %-20.20s", playingModeDesc[playingMode]);

	  (*initFunct)(0);
	  (*setVolumeBoostFunct)(volumeBoost);
	  setVolume(0,0x8000);
	  pspDebugScreenSetXY(0, 21);
	  pspDebugScreenSetTextColor(YELLOW);
   	  tEQ = EQ_getIndex(currentEQ);
	  pspDebugScreenPrintf("Equalizer  : %-20.20s", tEQ.name);
	  action = -1;
	  screen_playerAction(&action);
	  if (strlen(message) > 0){
		  pspDebugScreenSetXY(0, 5);
		  pspDebugScreenPrintf(message);
	  }

	  //Apro il file:
      retLoad = (*loadFunct)(filename);
	  if (retLoad != OPENING_OK){
		  pspDebugScreenSetXY(0, 10);
		  pspDebugScreenSetTextColor(RED);

          if (retLoad == ERROR_OPENING){
		      pspDebugScreenPrintf("Error opening file!");
          }else if (retLoad == ERROR_INVALID_SAMPLE_RATE){
              pspDebugScreenPrintf("Invalid sample rate!");
          }else if (retLoad == ERROR_MEMORY){
              pspDebugScreenPrintf("Memory error!");
          }else if (retLoad == ERROR_CREATE_THREAD){
              pspDebugScreenPrintf("Error creating thread!");
          }
		  unsetAudioFunctions();
		  sceKernelDelayThread(1000000);
		  return(0);
	  }
	  info = (*getInfoFunct)();
	  screen_fileinfo(info);
	  
	  //Imposto il clock:
      if (userSettings.CLOCK_AUTO){
          clock = info.defaultCPUClock;
          setCpuClock(info.defaultCPUClock);
      }
          
	  (*playFunct)();
	  action = 1;
	  screen_playerAction(&action);

	  while(runningFlag){
            if (resuming)
               continue;
		    //Aggiorno info a video:
			if (!updateInfo){                            
                if (displayStatus){
                    screen_sysinfo();
                    //Tempo trascorso:
                    (*getTimeStringFunct)(timestring);
                    pspDebugScreenSetTextColor(WHITE);
                    pspDebugScreenSetBackColor(BLACK);
                    pspDebugScreenSetXY(13, 9);
                    pspDebugScreenPrintf("%s / %s", timestring, info.strLength);
                    //Percentuale:
                    lastPercentage = (*getPercentageFunct)();
                    if (lastPercentage == 1 || lastPercentage >= lastPercentageDrawn + 5){
                        buildProgressBar(&barLength, pBar, &lastPercentage);
                        pspDebugScreenSetXY(13, 10);
                        pspDebugScreenPrintf(pBar);
                        lastPercentageDrawn = lastPercentage;
                    }
                    //Instant kbit:
                    info = (*getInfoFunct)();
                    pspDebugScreenSetTextColor(DEEPSKYBLUE);
                    pspDebugScreenSetXY(13, 17);
                    pspDebugScreenPrintf("%i [%3.3li] kbit", info.kbit, info.instantBitrate / 1000);

                    //Volume:
                    if (imposeGetVolume() != lastVolume){
                        drawVolumeBar(13,25);
                        lastVolume = imposeGetVolume();
                    }
                }else{
                    //Impedisco che il video vada in standby se l'ho spento io:
					scePowerTick(0);
				}
			}
			if (++updateInfo == 15){
				updateInfo = 0;
			}
			readButtons(&pad, 1);
			sceHprmPeekCurrentKey(&remoteButtons);

            //Check home popup:
			if(pad.Buttons & PSP_CTRL_HOME){
                if (!displayStatus){
					displayEnable();
                    setBrightness(curBrightness);
					displayStatus = 1;
                }                                    
                exitScreen();
                pspDebugScreenSetBackColor(BLACK);                
                int i;                
            	for (i = 0; i < 5; i++){
            		pspDebugScreenSetXY(15, 10+i);
            		pspDebugScreenPrintf("%-38.38s", "");
                }
                screen_player_tagInfo(tempInfo);
                lastPercentageDrawn = -5;
                updateInfo = 1;   
                sceKernelDelayThread(200000);    
            }

            if(pad.Buttons & PSP_CTRL_CIRCLE) {
					  (*endFunct)();
					  action = 0;
					  retVal = 2;
					  break;
			} else if((pad.Buttons & PSP_CTRL_CROSS) | (remoteButtons & PSP_HPRM_PLAYPAUSE)) {
					  //Azzero la velocità se != 0:
					  currentSpeed = (*getPlayingSpeedFunct)();
					  if (currentSpeed){
					      (*setPlayingSpeedFunct)(0);
					  }else{
						  (*pauseFunct)();
						  if (action != 0){
							action = 0;
						  }else{
							action = 1;
						  }
					  }
					  screen_playerAction(&action);
					  sceKernelDelayThread(400000);
			} else if(pad.Buttons & PSP_CTRL_UP){
				//Boost volume up:
				if (volumeBoost < MAX_VOLUME_BOOST){
					(*setVolumeBoostFunct)(++volumeBoost);
					screen_playerSettings();
				}
				sceKernelDelayThread(200000);
			} else if(pad.Buttons & PSP_CTRL_DOWN){
				//Boost volume down:
				if (volumeBoost > MIN_VOLUME_BOOST){
					(*setVolumeBoostFunct)(--volumeBoost);
					screen_playerSettings();
				}
				sceKernelDelayThread(200000);
			} else if(pad.Ly > 128 + ANALOG_SENS && !(pad.Buttons & PSP_CTRL_HOLD)) {
				if (clock >= 10){
					clock--;
					//scePowerSetCpuClockFrequency(clock);
                    setCpuClock(clock);
					screen_sysinfo();
					sceKernelDelayThread(100000);
				}
			} else if(pad.Ly < 128 - ANALOG_SENS && !(pad.Buttons & PSP_CTRL_HOLD)) {
				if (clock <= 222){
					clock++;
					//scePowerSetCpuClockFrequency(clock);
                    setCpuClock(clock);
					screen_sysinfo();
					sceKernelDelayThread(100000);
				}
			} else if(pad.Lx < 128 - ANALOG_SENS && !(pad.Buttons & PSP_CTRL_HOLD)) {
				if (bus > 54 && sceKernelDevkitVersion() < 0x03070110){
                    bus--;
                    scePowerSetBusClockFrequency(bus);
                    screen_sysinfo();
                    sceKernelDelayThread(100000);
				}
			} else if(pad.Lx > 128 + ANALOG_SENS && !(pad.Buttons & PSP_CTRL_HOLD)) {
				if (clock < 111 && sceKernelDevkitVersion() < 0x03070110){
					bus++;
					scePowerSetBusClockFrequency(bus);
					screen_sysinfo();
					sceKernelDelayThread(100000);
				}
			} else if(pad.Buttons & PSP_CTRL_LTRIGGER || remoteButtons & PSP_HPRM_BACK) {
                if (userSettings.FADE_OUT)
                    (*fadeOutFunct)(0.3);
                (*endFunct)();
                retVal = -1;
                break;
			} else if(pad.Buttons & PSP_CTRL_RTRIGGER || remoteButtons & PSP_HPRM_FORWARD) {
                if (userSettings.FADE_OUT)
                    (*fadeOutFunct)(0.3);
                (*endFunct)();
                retVal = 1;
                break;
			} else if(pad.Buttons & PSP_CTRL_START) {
				if (displayStatus == 1){
					//Spengo il display:
                    curBrightness = getBrightness();
                    setBrightness(0);
					displayDisable();
					displayStatus = 0;
				} else {
					//Accendo il display:
					displayEnable();
                    setBrightness(curBrightness);
					displayStatus = 1;
                    updateInfo = 0;
				}
				sceKernelDelayThread(400000);
			} else if(pad.Buttons & PSP_CTRL_SCREEN) {
				displayStatus = 1;
			} else if(pad.Buttons & PSP_CTRL_SQUARE) {
				//Mute solo se sono in play:
				if (action == 1){
					pspDebugScreenSetXY(0, 23);
					if (muted == 0){
						muted = 1;
						pspDebugScreenSetTextColor(RED);
						pspDebugScreenPrintf("Mute ON");
					}else{
						muted = 0;
						pspDebugScreenSetBackColor(BLACK);
						pspDebugScreenPrintf("       ");
					}
					(*setMuteFunct)(muted);
					sceKernelDelayThread(400000);
				}
			} else if(pad.Buttons & PSP_CTRL_SELECT) {
				//Change playing mode:
				if (++playingMode > MODE_SHUFFLE){
					playingMode = MODE_NORMAL;
				}
				pspDebugScreenSetXY(0, 4);
				pspDebugScreenSetBackColor(BLACK);
				pspDebugScreenSetTextColor(YELLOW);
				pspDebugScreenPrintf("Play mode  : %-20.20s", playingModeDesc[playingMode]);
				screen_playerSettings();
				sceKernelDelayThread(400000);
			} else if(pad.Buttons & PSP_CTRL_NOTE){
				//Cambio equalizzatore:
                if ((*isFilterSupportedFunct)()){
                    if (++currentEQ >= EQ_getEqualizersNumber()){
                        currentEQ = 0;
                        (*disableFilterFunct)();
                    }else{
                        (*enableFilterFunct)();
                    }
                    tEQ = EQ_getIndex(currentEQ);
                    (*setFilterFunct)(tEQ.filter, 1);
                    pspDebugScreenSetXY(0, 21);
                    pspDebugScreenSetBackColor(BLACK);
                    pspDebugScreenSetTextColor(YELLOW);
                    pspDebugScreenPrintf("Equalizer  : %-20.20s", tEQ.name);
                    screen_playerSettings();
                }
				sceKernelDelayThread(400000);
			} else if(pad.Buttons & PSP_CTRL_VOLUP || remoteButtons & PSP_HPRM_VOL_UP){
				drawVolumeBar(13,25);
				sceKernelDelayThread(100000);
			} else if(pad.Buttons & PSP_CTRL_VOLDOWN || remoteButtons & PSP_HPRM_VOL_DOWN){
				drawVolumeBar(13,25);
				sceKernelDelayThread(100000);
			} else if(pad.Buttons & PSP_CTRL_RIGHT){
				//Aumento la velocità di riproduzione:
				currentSpeed = (*getPlayingSpeedFunct)();
				if (currentSpeed < MAX_PLAYING_SPEED){
					(*setPlayingSpeedFunct)(++currentSpeed);
					screen_playerAction(&action);
				}
				sceKernelDelayThread(400000);
			} else if(pad.Buttons & PSP_CTRL_LEFT){
				//Diminuisco la velocità di riproduzione:
				currentSpeed = (*getPlayingSpeedFunct)();
				if (currentSpeed > MIN_PLAYING_SPEED){
					(*setPlayingSpeedFunct)(--currentSpeed);
					screen_playerAction(&action);
				}
				sceKernelDelayThread(400000);
			}

			//Controllo la riproduzione è finita:
			if ((*endOfStreamFunct)() == 1) {
				//Controllo il repeat:
				if (playingMode == MODE_REPEAT){
					(*playFunct)();
				}else{
					(*endFunct)();
					retVal = 0;
					break;
				}
			}
	  }

	  //Srobbler LOG:
	  if (userSettings.SCROBBLER && strlen(info.title)){
		time_t mytime;
		time(&mytime);
		if (lastPercentage >= 50){
			SCROBBLER_addTrack(info, info.length, "L", mytime);
		}else{
			SCROBBLER_addTrack(info, info.length, "S", mytime);
		}
	  }

	  //Accendo il display se devo:
	  if (!displayStatus && retVal == 2){
	    displayEnable();
        setBrightness(curBrightness);
        displayStatus = 1;
      }
	  unsetAudioFunctions();

	  //Imposto il clock:
      if (userSettings.CLOCK_AUTO)
          setCpuClock(userSettings.CLOCK_GUI);
	  return(retVal);
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//Play directory:
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void playDirectory(char *dirName, char *initialFileName){
	struct opendir_struct dirToPlay;
	char ext[6][5] = {"MP3", "OGG", "AA3", "OMG", "OMA", "FLAC"};
	char testo[10] = "";
	char fileToPlay[262] = "";
	char message[68] = "";

	char *result = opendir_open(&dirToPlay, dirName, ext, 6, 0);
	unsigned int playedTracks[dirToPlay.number_of_directory_entries];
	unsigned int playedTracksNumber = 0;
	unsigned int currentTrack = 0;
	if (result == 0){
		int i = 0;
		int playerReturn = 0;
		char onlyName[262] = "";
		sortDirectory(&dirToPlay);
		getFileName(dirName, onlyName);
		snprintf(message, sizeof(message), "Directory  : %-60.60s", onlyName);

		if (playingMode == MODE_SHUFFLE){
			i = randomTrack(dirToPlay.number_of_directory_entries, playedTracksNumber, playedTracks);
		}else{
			i = 0;
		}
		//Controllo il file iniziale:
		if (strlen(initialFileName)){
			int j;
			for (j=0; j < dirToPlay.number_of_directory_entries; j++){
				if (strcmp(dirToPlay.directory_entry[j].d_name, initialFileName) == 0){
					i = j;
					break;
				}
			}
		}
		currentTrack = 0;

		while(runningFlag){
			snprintf(testo, sizeof(testo), "%i/%i", i + 1, dirToPlay.number_of_directory_entries);
			strcpy(fileToPlay, dirName);
			if (dirName[strlen(dirName)-1] != '/'){
				strcat(fileToPlay, "/");
			}
			strcat(fileToPlay, dirToPlay.directory_entry[i].d_name);
			playerReturn = playFile(fileToPlay, testo, message);
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
				//Controllo se sono in shuffle:
				if (playingMode == MODE_SHUFFLE){
					if (currentTrack < playedTracksNumber - 1 && playerReturn == 1){
						i = playedTracks[++currentTrack];
					}else{
						i = randomTrack(dirToPlay.number_of_directory_entries, playedTracksNumber, playedTracks);
						if (i == -1){
							break;
						}
						currentTrack++;
					}
				}else{
					//Controllo se è finita la directory e non sono in repeat:
					if ((playerReturn == 0) & (i == dirToPlay.number_of_directory_entries - 1) & (playingMode != MODE_REPEAT_ALL)){
						break;
					}
					//Altrimenti passo al file successivo:
					i++;
					if (i > dirToPlay.number_of_directory_entries - 1){
						i = 0;
					}
					currentTrack = i;
				}
			}else if (playerReturn == -1){
				//Controllo se sono in shuffle:
				if (playingMode == MODE_SHUFFLE){
					/*i = randomTrack(dirToPlay.number_of_directory_entries, playedTracksNumber, playedTracks);
					if (i == -1){
						break;
					}*/
					if (currentTrack){
						i = playedTracks[--currentTrack];
					}
				}else{
					i--;
					if (i < 0){
						i = dirToPlay.number_of_directory_entries - 1;
					}
					currentTrack = i;
				}
			}
		}
	}
	//Accendo il display se devo:
	if (!displayStatus){
	  displayEnable();
      setBrightness(curBrightness);
      displayStatus = 1;
    }
	opendir_close(&dirToPlay);
	return;
}


///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//Play M3U:
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void playM3U(char *m3uName){
	char testo[10] = "";
	char fileToPlay[262] = "";
	int playerReturn = 0;
	int i = 0;
	struct M3U_songEntry song;
	char message[60] = "";

	M3U_open(m3uName);
	int songCount = M3U_getSongCount();
	unsigned int playedTracks[songCount];
	unsigned int playedTracksNumber = 0;
	unsigned int currentTrack = 0;

	char onlyName[262] = "";
	getFileName(m3uName, onlyName);
	snprintf(message, sizeof(message), "Playlist   : %-60.60s", onlyName);

	if (playingMode == MODE_SHUFFLE){
		i = randomTrack(songCount, playedTracksNumber, playedTracks);
	}else{
		i = 0;
	}
	currentTrack = 0;

	while(runningFlag){
		snprintf(testo, sizeof(testo), "%i/%i", i + 1, songCount);
		song = M3U_getSong(i);
		strcpy(fileToPlay, song.fileName);
		playerReturn = playFile(fileToPlay, testo, message);
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
			//Controllo se sono in shuffle:
			if (playingMode == MODE_SHUFFLE){
				if (currentTrack < playedTracksNumber - 1 && playerReturn == 1){
					i = playedTracks[++currentTrack];
				}else{
					i = randomTrack(songCount, playedTracksNumber, playedTracks);
					if (i == -1){
						break;
					}
					currentTrack++;
				}
			}else{
				//Controllo se è finita la playlist e non sono in repeat:
				if ((playerReturn == 0) & (i == songCount - 1) & (playingMode != MODE_REPEAT_ALL)){
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
			if (playingMode == MODE_SHUFFLE){
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
	//Accendo il display se devo:
	if (displayStatus == 0){
	  displayEnable();
      setBrightness(curBrightness);
	}
	return;
}


///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//Main Menu:
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void fileBrowser_menu(){
	struct opendir_struct directory;
	char curDir[262] = "";
	char dirToPlay[262] = "";
	char fileToPlay[262] = "";
	char ext[7][5] = {"MP3", "M3U", "OGG", "AA3", "OMG", "OMA", "FLAC"};
	char *result;

	if (strlen(fileBrowserDir) == 0){
		//Provo la prima directory:
		strcpy(curDir, music_directory_1);
		result = opendir_open(&directory, curDir, ext, 7, 1);
		if (result){
			//Provo la seconda directory:
			strcpy(curDir, music_directory_2);
			result = opendir_open(&directory, curDir, ext, 7, 1);
			if (result){
				screen_init();
				pspDebugScreenSetXY(0, 4);
				pspDebugScreenSetTextColor(RED);
				pspDebugScreenPrintf("\"%s\" and \"%s\" not found or empty", music_directory_1, music_directory_2);
				sceKernelDelayThread(3000000);
				pspDebugScreenSetXY(0, 5);
				pspDebugScreenPrintf("Quitting...");
				sceKernelDelayThread(2000000);
				current_mode = -1;
				return;
			}
		}
	}else{
		strcpy(curDir, fileBrowserDir);
		result = opendir_open(&directory, curDir, ext, 7, 1);
	}
	sortDirectory(&directory);

	int selected_entry         = 0;
	int top_entry              = 0;
	int maximum_number_of_rows = 20;
	int starting_row           = 4;
	int updateInfo = 10;
	char author[50] = "";
	char title[100] = "";

	while(runningFlag)
		{
        SceCtrlData controller;
		screen_init();
		screen_toolbar();
		screen_menu_init();
		pspDebugScreenSetTextColor(WHITE);
		pspDebugScreenSetXY(0, 3);
		pspDebugScreenPrintf(curDir);

		while(runningFlag)
			{
			if (updateInfo >= 10)
			{
				screen_sysinfo();
				updateInfo = 0;
			}
			updateInfo++;

			readButtons(&controller, 1);
            //Check home popup:
			if(controller.Buttons & PSP_CTRL_HOME)
                exitScreen();
			
			if ((controller.Buttons & PSP_CTRL_DOWN) | (controller.Ly > 128 + ANALOG_SENS && !(controller.Buttons & PSP_CTRL_HOLD))){
				if (selected_entry + 1 < directory.number_of_directory_entries){
					selected_entry++;

					if (selected_entry == top_entry + maximum_number_of_rows){
						top_entry++;
						}
					}
			} else if ((controller.Buttons & PSP_CTRL_UP) | (controller.Ly < 128 - ANALOG_SENS && !(controller.Buttons & PSP_CTRL_HOLD))){
				if (selected_entry){
					selected_entry--;

					if (selected_entry == top_entry - 1){
						top_entry--;
					}
				}
			} else if ((controller.Buttons & PSP_CTRL_RIGHT) | (controller.Lx > 128 + ANALOG_SENS && !(controller.Buttons & PSP_CTRL_HOLD))){
				//Pagina giù:
				if (top_entry + maximum_number_of_rows < directory.number_of_directory_entries){
					top_entry = top_entry + maximum_number_of_rows;
					selected_entry += maximum_number_of_rows;
					if (selected_entry > directory.number_of_directory_entries - 1){
						selected_entry = directory.number_of_directory_entries - 1;
					}
				} else {
					selected_entry = directory.number_of_directory_entries - 1;
				}
				sceKernelDelayThread(100000);
			} else if ((controller.Buttons & PSP_CTRL_LEFT) | (controller.Lx < 128 - ANALOG_SENS && !(controller.Buttons & PSP_CTRL_HOLD))){
				//Pagina su:
				if (top_entry - maximum_number_of_rows >= 0){
					top_entry = top_entry - maximum_number_of_rows;
					selected_entry -= maximum_number_of_rows;
				} else {
					top_entry = 0;
					selected_entry = 0;
				}
				sceKernelDelayThread(100000);
			}
			pspDebugScreenSetXY(1, starting_row);
			pspDebugScreenSetTextColor(WHITE);
			pspDebugScreenSetBackColor(0xaa4400);

			//Segno precedente:
			if (top_entry > 0){
				pspDebugScreenPrintf("%-66.66s", "...");
			} else{
				pspDebugScreenPrintf("%-66.66s", "");
			}
			int i = 0;
			//Elementi della directory:
			for (; i < maximum_number_of_rows; i++){
				int current_entry = top_entry + i;

				pspDebugScreenSetXY(1, starting_row + i + 1);

				if (current_entry < directory.number_of_directory_entries){
					if (current_entry == selected_entry){
						pspDebugScreenSetBackColor(0x882200);
					} else {
						pspDebugScreenSetBackColor(0xcc6600);
					}
					//Controllo se è una directory o un file:
					if (FIO_S_ISREG(directory.directory_entry[current_entry].d_stat.st_mode)){
						if (strstr(directory.directory_entry[current_entry].d_name, ".m3u") != NULL || strstr(directory.directory_entry[current_entry].d_name, ".M3U") != NULL){
							pspDebugScreenSetTextColor(ORANGE);
						}else{
							pspDebugScreenSetTextColor(YELLOW);
						}
						pspDebugScreenPrintf("%-66.66s", directory.directory_entry[current_entry].d_name);
					} else if (FIO_S_ISDIR(directory.directory_entry[current_entry].d_stat.st_mode)){
						//Spezzo autore e titolo:
						if (splitAuthorTitle(directory.directory_entry[current_entry].d_name, author, title)){
							char format[10] = "";
							pspDebugScreenSetTextColor(YELLOW);
							pspDebugScreenPrintf("%s", author);
							pspDebugScreenPrintf(" ");
							pspDebugScreenSetTextColor(WHITE);
							snprintf(format, sizeof(format), "%%-%i.%is", 66 - strlen(author) - 1, 66 - strlen(author) - 1);
							pspDebugScreenPrintf(format, title);
						} else {
							pspDebugScreenSetTextColor(0xcccccc);
							pspDebugScreenPrintf("%-66.66s", directory.directory_entry[current_entry].d_name);
						}
					}
				} else {
					pspDebugScreenSetTextColor(WHITE);
					pspDebugScreenSetBackColor(0xaa4400);
					pspDebugScreenPrintf("%-66.66s", "");
				}
			}
			pspDebugScreenSetXY(1, starting_row + maximum_number_of_rows + 1);
			pspDebugScreenSetTextColor(WHITE);
			pspDebugScreenSetBackColor(0xaa4400);
			//Segno successivo:
			if (top_entry + maximum_number_of_rows < directory.number_of_directory_entries){
				pspDebugScreenPrintf("%-66.66s", "...");
			}else{
				pspDebugScreenPrintf("%-66.66s", "");
			}

			//Totale files:
			pspDebugScreenSetXY(55, starting_row + maximum_number_of_rows + 1);
			pspDebugScreenPrintf("Objects: %3.1i", directory.number_of_directory_entries);

			if (controller.Buttons & PSP_CTRL_CROSS || controller.Buttons & PSP_CTRL_CIRCLE ||
				controller.Buttons & PSP_CTRL_SQUARE || controller.Buttons & PSP_CTRL_RTRIGGER || controller.Buttons & PSP_CTRL_LTRIGGER ||
				controller.Buttons & PSP_CTRL_START || controller.Buttons & PSP_CTRL_SELECT){
				break;
			}
			sceKernelDelayThread(100000);
		}

        if (controller.Buttons & PSP_CTRL_CROSS) {
			//Controllo se è un file o una directory:
			if (FIO_S_ISREG(directory.directory_entry[selected_entry].d_stat.st_mode)){
				if (strstr(directory.directory_entry[selected_entry].d_name, ".m3u") != NULL || strstr(directory.directory_entry[selected_entry].d_name, ".M3U") != NULL){
					//File m3u:
					strcpy(fileToPlay, curDir);
					if (curDir[strlen(curDir)-1] != '/'){
						strcat(fileToPlay, "/");
					}
					strcat(fileToPlay, directory.directory_entry[selected_entry].d_name);
					playM3U(fileToPlay);
					sceKernelDelayThread(200000);
				}else{
					//File audio:
					playDirectory(curDir, directory.directory_entry[selected_entry].d_name);
					sceKernelDelayThread(200000);
                }
			} else if (FIO_S_ISDIR(directory.directory_entry[selected_entry].d_stat.st_mode)){
				if (curDir[strlen(curDir)-1] != '/'){
					strcat(curDir, "/");
				}
				strcat(curDir, directory.directory_entry[selected_entry].d_name);
				opendir_close(&directory);
				selected_entry = 0;
				top_entry = 0;
				result = opendir_open(&directory, curDir, ext, 7, 1);
				sortDirectory(&directory);
				sceKernelDelayThread(200000);
			}
		} else if (controller.Buttons & PSP_CTRL_CIRCLE){
                char tempDir[262] = "";
                strcpy(tempDir, curDir);
				if (directoryUp(curDir) == 0){
                    int i;
					opendir_close(&directory);
					selected_entry = 0;
					top_entry = 0;
					result = opendir_open(&directory, curDir, ext, 7, 1);
					sortDirectory(&directory);
                    //Mi riposiziono:
                    for (i = 0; i < directory.number_of_directory_entries; i++){
                        if (strstr(tempDir, directory.directory_entry[i].d_name) != NULL){
                           top_entry = i;
                           selected_entry = i;
                           break;
                        }
                    }
				}
				sceKernelDelayThread(200000);
		} else if (controller.Buttons & PSP_CTRL_SQUARE){
				//Controllo se è una directory:
				if (FIO_S_ISDIR(directory.directory_entry[selected_entry].d_stat.st_mode)){
					strcpy(dirToPlay, curDir);
					if (dirToPlay[strlen(dirToPlay)-1] != '/'){
						strcat(dirToPlay, "/");
					}
					strcat(dirToPlay, directory.directory_entry[selected_entry].d_name);
					playDirectory(dirToPlay, "");
					sceKernelDelayThread(200000);
				}
		} else if (controller.Buttons & PSP_CTRL_START){
			if (FIO_S_ISREG(directory.directory_entry[selected_entry].d_stat.st_mode)){
				//Aggiungo il file selezionato alla playlist:
				if (strstr(directory.directory_entry[selected_entry].d_name, ".mp3") != NULL || strstr(directory.directory_entry[selected_entry].d_name, ".MP3") != NULL ||
                    strstr(directory.directory_entry[selected_entry].d_name, ".ogg") != NULL || strstr(directory.directory_entry[selected_entry].d_name, ".OGG") != NULL){
					waitMessage("");
					//File mp3:
					strcpy(fileToPlay, curDir);
					if (curDir[strlen(curDir)-1] != '/'){
						strcat(fileToPlay, "/");
					}
					strcat(fileToPlay, directory.directory_entry[selected_entry].d_name);
					addFileToPlaylist(fileToPlay, 1);
				}
			}else if (FIO_S_ISDIR(directory.directory_entry[selected_entry].d_stat.st_mode)){
				//Aggiungo la directory alla playlist:
				waitMessage("000% done");
				strcpy(fileToPlay, curDir);
				if (curDir[strlen(curDir)-1] != '/'){
					strcat(fileToPlay, "/");
				}
				strcat(fileToPlay, directory.directory_entry[selected_entry].d_name);
				addDirectoryToPlaylist(fileToPlay);
			}
		} else if (controller.Buttons & PSP_CTRL_RTRIGGER){
			strcpy(fileBrowserDir, curDir);
			nextMode();
			break;
		} else if (controller.Buttons & PSP_CTRL_LTRIGGER){
			strcpy(fileBrowserDir, curDir);
			previousMode();
			break;
		} else if (controller.Buttons & PSP_CTRL_SELECT){
			showUSBactivate();
			//Refresh directory:
			opendir_close(&directory);
			selected_entry = 0;
			top_entry = 0;
			result = opendir_open(&directory, curDir, ext, 7, 1);
			sortDirectory(&directory);
			sceKernelDelayThread(200000);
		}
	}
	opendir_close(&directory);
}


///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//Playlist menu:
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void playlist_menu(){
	struct opendir_struct directory;
	char curDir[262] = "";
	char lastOp[60] = "";
	char fileToPlay[262] = "";
	int selected_entry         = 0;
	int top_entry              = 0;
	int maximum_number_of_rows = 15;
	int starting_row           = 4;
	int updateInfo = 10;
	int sel_updateInfo = 0;
	int sel_songCount = 0;
	int sel_total = 0;

	screen_init();
    screen_toolbar();
	screen_playlist_init();
	strcpy(curDir, ebootDirectory);
	if (curDir[strlen(curDir)-1] != '/'){
		strcat(curDir, "/");
	}
	strcat(curDir, "playLists");
	char ext[1][5] = {"M3U"};
	char *result = opendir_open(&directory, curDir, ext, 1, 0);
	sortDirectory(&directory);
	if (result){
		//current_mode = 0;
		//return;
	}else{
		//Carico i dati della prima playlist:
		strcpy(fileToPlay, curDir);
		if (fileToPlay[strlen(fileToPlay)-1] != '/'){
			strcat(fileToPlay, "/");
		}
		strcat(fileToPlay, directory.directory_entry[0].d_name);
		if (M3U_open(fileToPlay) == 0){
			sel_songCount = M3U_getSongCount();
			sel_total = M3U_getTotalLength();
			sel_updateInfo = 1;
		}
	}

	SceCtrlData controller;
	while(runningFlag){
		if (updateInfo >= 10){
			screen_sysinfo();
			updateInfo = 0;
		}
		updateInfo++;

		if (strlen(lastOp) > 0){
			pspDebugScreenSetTextColor(RED);
			pspDebugScreenSetBackColor(BLACK);
			pspDebugScreenSetXY(0, 3);
			pspDebugScreenPrintf(lastOp);
		}
		readButtons(&controller, 1);
        //Check home popup:
		if(controller.Buttons & PSP_CTRL_HOME)
		    exitScreen();
		
		if ((controller.Buttons & PSP_CTRL_DOWN) | (controller.Ly > 128 + ANALOG_SENS && !(controller.Buttons & PSP_CTRL_HOLD))){
			if (selected_entry + 1 < directory.number_of_directory_entries){
				selected_entry++;
				//Carico i dati della playlist selezionata:
				strcpy(fileToPlay, curDir);
				if (fileToPlay[strlen(fileToPlay)-1] != '/'){
					strcat(fileToPlay, "/");
				}
				strcat(fileToPlay, directory.directory_entry[selected_entry].d_name);
				if (M3U_open(fileToPlay) == 0){
					sel_songCount = M3U_getSongCount();
					sel_total = M3U_getTotalLength();
					sel_updateInfo = 1;
				}

				if (selected_entry == top_entry + maximum_number_of_rows){
					top_entry++;
					}
				}
		} else if ((controller.Buttons & PSP_CTRL_UP) | (controller.Ly < 128 - ANALOG_SENS && !(controller.Buttons & PSP_CTRL_HOLD))){
			if (selected_entry){
				selected_entry--;
				//Carico i dati della playlist selezionata:
				strcpy(fileToPlay, curDir);
				if (fileToPlay[strlen(fileToPlay)-1] != '/'){
					strcat(fileToPlay, "/");
				}
				strcat(fileToPlay, directory.directory_entry[selected_entry].d_name);
				if (M3U_open(fileToPlay) == 0){
					sel_songCount = M3U_getSongCount();
					sel_total = M3U_getTotalLength();
					sel_updateInfo = 1;
				}

				if (selected_entry == top_entry - 1){
					top_entry--;
				}
			}
		} else if ((controller.Buttons & PSP_CTRL_RIGHT) | (controller.Lx > 128 + ANALOG_SENS && !(controller.Buttons & PSP_CTRL_HOLD))){
			//Pagina giù:
			if (directory.number_of_directory_entries > 0){
				if (top_entry + maximum_number_of_rows < directory.number_of_directory_entries){
					top_entry = top_entry + maximum_number_of_rows;
					selected_entry += maximum_number_of_rows;
					if (selected_entry > directory.number_of_directory_entries - 1){
						selected_entry = directory.number_of_directory_entries - 1;
					}
				} else {
					selected_entry = directory.number_of_directory_entries - 1;
				}
				//Carico i dati della playlist selezionata:
				strcpy(fileToPlay, curDir);
				if (fileToPlay[strlen(fileToPlay)-1] != '/'){
					strcat(fileToPlay, "/");
				}
				strcat(fileToPlay, directory.directory_entry[selected_entry].d_name);
				if (M3U_open(fileToPlay) == 0){
					sel_songCount = M3U_getSongCount();
					sel_total = M3U_getTotalLength();
				}
				sceKernelDelayThread(100000);
			}
		} else if ((controller.Buttons & PSP_CTRL_LEFT) | (controller.Lx < 128 - ANALOG_SENS && !(controller.Buttons & PSP_CTRL_HOLD))){
			//Pagina su:
			if (directory.number_of_directory_entries > 0){
				if (top_entry - maximum_number_of_rows >= 0){
					top_entry = top_entry - maximum_number_of_rows;
					selected_entry -= maximum_number_of_rows;
				} else {
					top_entry = 0;
					selected_entry = 0;
				}
				//Carico i dati della playlist selezionata:
				strcpy(fileToPlay, curDir);
				if (fileToPlay[strlen(fileToPlay)-1] != '/'){
					strcat(fileToPlay, "/");
				}
				strcat(fileToPlay, directory.directory_entry[selected_entry].d_name);
				if (M3U_open(fileToPlay) == 0){
					sel_songCount = M3U_getSongCount();
					sel_total = M3U_getTotalLength();
					sel_updateInfo = 1;
				}
				sceKernelDelayThread(100000);
			}
		} else if (controller.Buttons & PSP_CTRL_CIRCLE && directory.number_of_directory_entries > 0) {
			if (confirm("Remove selected playlist?", BLACK) == 1){
				//Remove selected playlist:
				strcpy(fileToPlay, curDir);
				if (fileToPlay[strlen(fileToPlay)-1] != '/'){
					strcat(fileToPlay, "/");
				}
				strcat(fileToPlay, directory.directory_entry[selected_entry].d_name);
				sceIoRemove(fileToPlay);
				//Ricarico il contenuto della directory
				opendir_close(&directory);
				selected_entry = 0;
				top_entry = 0;
				result = opendir_open(&directory, curDir, ext, 1, 0);
				sortDirectory(&directory);
				if (directory.number_of_directory_entries > 0){
					//Carico i dati della playlist selezionata:
					strcpy(fileToPlay, curDir);
					if (fileToPlay[strlen(fileToPlay)-1] != '/'){
						strcat(fileToPlay, "/");
					}
					strcat(fileToPlay, directory.directory_entry[selected_entry].d_name);
					if (M3U_open(fileToPlay) == 0){
						sel_songCount = M3U_getSongCount();
						sel_total = M3U_getTotalLength();
						sel_updateInfo = 1;
					}
					sel_updateInfo = 1;
				}else{
					pspDebugScreenSetXY(0, 23);
					pspDebugScreenPrintf("%20.20s", "");
					pspDebugScreenSetXY(0, 24);
					pspDebugScreenPrintf("%20.20s", "");
				}
			}
		} else if (controller.Buttons & PSP_CTRL_SQUARE && directory.number_of_directory_entries > 0) {
			//Carico la playlist:
			if (FIO_S_ISREG(directory.directory_entry[selected_entry].d_stat.st_mode)){
				if (strstr(directory.directory_entry[selected_entry].d_name, ".m3u") != NULL || strstr(directory.directory_entry[selected_entry].d_name, ".M3U") != NULL){
					if (confirm("Load selected playlist?", BLACK) == 1){
						strcpy(lastOp, "Error loading playlist");
						strcpy(fileToPlay, curDir);
						if (fileToPlay[strlen(fileToPlay)-1] != '/'){
							strcat(fileToPlay, "/");
						}
						strcat(fileToPlay, directory.directory_entry[selected_entry].d_name);
						if (M3U_open(fileToPlay) == 0){
							getFileName(fileToPlay, currentPlaylist);
							M3U_save(tempM3Ufile);
							playListModified = 0;
							strcpy(lastOp, "Playlist loaded");
						}
						sceKernelDelayThread(100000);
					}
				}
			}
		} else if (controller.Buttons & PSP_CTRL_CROSS && directory.number_of_directory_entries > 0) {
			//Controllo se è un file:
			if (FIO_S_ISREG(directory.directory_entry[selected_entry].d_stat.st_mode)){
				if (strstr(directory.directory_entry[selected_entry].d_name, ".m3u") != NULL || strstr(directory.directory_entry[selected_entry].d_name, ".M3U") != NULL){
					strcpy(fileToPlay, curDir);
					if (fileToPlay[strlen(fileToPlay)-1] != '/'){
						strcat(fileToPlay, "/");
					}
					strcat(fileToPlay, directory.directory_entry[selected_entry].d_name);
					playM3U(fileToPlay);
					screen_init();
					screen_toolbar();
					screen_playlist_init();
					//Carico i dati della playlist selezionata:
					strcpy(fileToPlay, curDir);
					if (fileToPlay[strlen(fileToPlay)-1] != '/'){
						strcat(fileToPlay, "/");
					}
					strcat(fileToPlay, directory.directory_entry[selected_entry].d_name);
					if (M3U_open(fileToPlay) == 0){
						sel_songCount = M3U_getSongCount();
						sel_total = M3U_getTotalLength();
						sel_updateInfo = 1;
					}
					sceKernelDelayThread(200000);
				}
			}
		}

		pspDebugScreenSetXY(1, starting_row);
		pspDebugScreenSetTextColor(WHITE);
		pspDebugScreenSetBackColor(0xaa4400);

		//Segno precedente:
		if (top_entry > 0){
			pspDebugScreenPrintf("%-66.66s", "...");
		} else{
			pspDebugScreenPrintf("%-66.66s", "");
		}
		int i = 0;
		for (; i < maximum_number_of_rows; i++){
			int current_entry = top_entry + i;

			pspDebugScreenSetXY(1, starting_row + i + 1);

			if (current_entry < directory.number_of_directory_entries){
				if (current_entry == selected_entry){
					pspDebugScreenSetBackColor(0x882200);
				} else {
					pspDebugScreenSetBackColor(0xcc6600);
				}
				//Controllo se è una directory o un file:
				if (FIO_S_ISREG(directory.directory_entry[current_entry].d_stat.st_mode)){
					if (strstr(directory.directory_entry[current_entry].d_name, ".m3u") != NULL || strstr(directory.directory_entry[current_entry].d_name, ".M3U") != NULL){
						pspDebugScreenSetTextColor(ORANGE);
					}else{
						pspDebugScreenSetTextColor(YELLOW);
					}
					pspDebugScreenPrintf("%-66.66s", directory.directory_entry[current_entry].d_name);
				} else {
					pspDebugScreenSetTextColor(0xcccccc);
					pspDebugScreenPrintf("%-66.66s", directory.directory_entry[current_entry].d_name);
				}
			} else {
				pspDebugScreenSetTextColor(WHITE);
				pspDebugScreenSetBackColor(0xaa4400);
				pspDebugScreenPrintf("%-66.66s", "");
			}
		}
		pspDebugScreenSetXY(1, starting_row + maximum_number_of_rows + 1);
		pspDebugScreenSetTextColor(WHITE);
		pspDebugScreenSetBackColor(0xaa4400);
		//Segno successivo:
		if (top_entry + maximum_number_of_rows < directory.number_of_directory_entries){
			pspDebugScreenPrintf("%-66.66s", "...");
		}else{
			pspDebugScreenPrintf("%-66.66s", "");
		}

		//Totale files:
		pspDebugScreenSetXY(55, starting_row + maximum_number_of_rows + 1);
		pspDebugScreenPrintf("Objects: %3.1i", directory.number_of_directory_entries);

		//Playlist info:
		if (sel_updateInfo == 1){
			pspDebugScreenSetTextColor(YELLOW);
			pspDebugScreenSetBackColor(BLACK);
			pspDebugScreenSetXY(0, 23);
			pspDebugScreenPrintf("Tracks    : %i", sel_songCount);
			pspDebugScreenPrintf("\n");
			int h = 0;
			int m = 0;
			int s = 0;
			h = sel_total / 3600;
			m = (sel_total - h * 3600) / 60;
			s = sel_total - h * 3600 - m * 60;
			pspDebugScreenPrintf("Total time: %2.2i:%2.2i:%2.2i", h, m, s);
			sel_updateInfo = 0;
		}

		//Tasti di uscita:
		if (controller.Buttons & PSP_CTRL_RTRIGGER || controller.Buttons & PSP_CTRL_LTRIGGER){
			break;
		}
		sceKernelDelayThread(100000);
	}
	//Controllo input:
    if (controller.Buttons & PSP_CTRL_RTRIGGER){
		nextMode();
	} else if (controller.Buttons & PSP_CTRL_LTRIGGER){
		previousMode();
	}
	M3U_clear();
	opendir_close(&directory);
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//Playlist editor:
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void playlist_editor(){
	char lastOp[60] = "";
	char M3Ufilename[262] = "";
	char newPlaylist[262] = "";
	int selected_entry         = 0;
	int top_entry              = 0;
	int maximum_number_of_rows = 14;
	int starting_row           = 4;
	int updateInfo = 10;
	struct fileInfo tagInfo;
	struct M3U_songEntry song;
	int sel_updateInfo = 0;

	screen_init();
	screen_toolbar();
	//Apro il file temporaneo m3u:
	strcpy(M3Ufilename, tempM3Ufile);

	if (M3U_open(M3Ufilename) == 0){
		M3U_forceModified(playListModified);
		//Info primo file:
		if (M3U_getSongCount() > 0){
			song = M3U_getSong(0);
            setAudioFunctions(song.fileName, userSettings.MP3_ME);
			tagInfo = (*getTagInfoFunct)(song.fileName);
			sel_updateInfo = 1;
		}

		SceCtrlData controller;
		screen_playlistEditor_init();
		pspDebugScreenSetTextColor(WHITE);
		pspDebugScreenSetXY(0, 20);
		if (strlen(currentPlaylist) > 0){
			pspDebugScreenPrintf("Filename: %-55.55s", currentPlaylist);
		}else{
			pspDebugScreenPrintf("Filename: %-55.55s", "NEW PLAYLIST");
		}

		while(runningFlag){
			if (updateInfo >= 10){
				screen_sysinfo();
				updateInfo = 0;
			}
			updateInfo++;

			if (strlen(lastOp) > 0){
				pspDebugScreenSetTextColor(RED);
				pspDebugScreenSetBackColor(BLACK);
				pspDebugScreenSetXY(0, 3);
				pspDebugScreenPrintf(lastOp);
			}
			readButtons(&controller, 1);
            //Check home popup:
			if(controller.Buttons & PSP_CTRL_HOME){
              exitScreen();
			}                    
			
			if ((controller.Buttons & PSP_CTRL_DOWN) | (controller.Ly > 128 + ANALOG_SENS && !(controller.Buttons & PSP_CTRL_HOLD))){
				if (selected_entry + 1 < M3U_getSongCount()){
					selected_entry++;
					song = M3U_getSong(selected_entry);
                    setAudioFunctions(song.fileName, userSettings.MP3_ME);
                    tagInfo = (*getTagInfoFunct)(song.fileName);
					sel_updateInfo = 1;

					if (selected_entry == top_entry + maximum_number_of_rows){
						top_entry++;
					}
				}
			} else if ((controller.Buttons & PSP_CTRL_UP) | (controller.Ly < 128 - ANALOG_SENS && !(controller.Buttons & PSP_CTRL_HOLD))){
				if (selected_entry){
					selected_entry--;
					song = M3U_getSong(selected_entry);
                    setAudioFunctions(song.fileName, userSettings.MP3_ME);
                    tagInfo = (*getTagInfoFunct)(song.fileName);
					sel_updateInfo = 1;

					if (selected_entry == top_entry - 1){
						top_entry--;
					}
				}
			} else if ((controller.Buttons & PSP_CTRL_RIGHT) | (controller.Lx > 128 + ANALOG_SENS && !(controller.Buttons & PSP_CTRL_HOLD))){
				//Pagina giù:
				if (M3U_getSongCount() > 0){
					if (top_entry + maximum_number_of_rows < M3U_getSongCount()){
						top_entry = top_entry + maximum_number_of_rows;
						selected_entry += maximum_number_of_rows;
						if (selected_entry > M3U_getSongCount() - 1){
							selected_entry = M3U_getSongCount() - 1;
						}
					} else {
						selected_entry = M3U_getSongCount() - 1;
					}
					song = M3U_getSong(selected_entry);
                    setAudioFunctions(song.fileName, userSettings.MP3_ME);
                    tagInfo = (*getTagInfoFunct)(song.fileName);
					sel_updateInfo = 1;
					sceKernelDelayThread(100000);
				}
			} else if ((controller.Buttons & PSP_CTRL_LEFT) | (controller.Lx < 128 - ANALOG_SENS && !(controller.Buttons & PSP_CTRL_HOLD))){
				//Pagina su:
				if (M3U_getSongCount() > 0){
					if (top_entry - maximum_number_of_rows >= 0){
						top_entry = top_entry - maximum_number_of_rows;
						selected_entry -= maximum_number_of_rows;
					} else {
						top_entry = 0;
						selected_entry = 0;
					}
					song = M3U_getSong(selected_entry);
                    setAudioFunctions(song.fileName, userSettings.MP3_ME);
                    tagInfo = (*getTagInfoFunct)(song.fileName);
					sel_updateInfo = 1;
					sceKernelDelayThread(100000);
				}
			} else if (controller.Buttons & PSP_CTRL_CROSS && M3U_getSongCount() > 0){
				//Sposto giù:
				if (M3U_moveSongDown(selected_entry) == 0){
					selected_entry++;
					if (selected_entry == top_entry + maximum_number_of_rows){
						top_entry++;
					}
					sceKernelDelayThread(100000);
				}
			} else if (controller.Buttons & PSP_CTRL_SQUARE && M3U_getSongCount() > 0){
				//Sposto su:
				if (M3U_moveSongUp(selected_entry) == 0){
					selected_entry--;
					if (selected_entry == top_entry - 1){
						top_entry--;
					}
					sceKernelDelayThread(100000);
				}
			} else if (controller.Buttons & PSP_CTRL_CIRCLE && M3U_getSongCount() > 0){
				if (confirm("Remove selected track from playlist?", BLACK) == 1){
					M3U_removeSong(selected_entry);
					sceKernelDelayThread(100000);
					if (M3U_getSongCount() > 0){
						if (selected_entry >= M3U_getSongCount()){
							selected_entry--;
						}
						song = M3U_getSong(selected_entry);
                        setAudioFunctions(song.fileName, userSettings.MP3_ME);
                        tagInfo = (*getTagInfoFunct)(song.fileName);
						sel_updateInfo = 1;
						if (selected_entry == top_entry - 1){
							top_entry--;
						}
					}else{
						selected_entry = 0;
						top_entry = 0;
						screen_playlistEditor_init();
					}
				}
			} else if (controller.Buttons & PSP_CTRL_START && M3U_getSongCount() > 0){
				if (confirm("Save the playlist?", BLACK) == 1){
					if (askString(currentPlaylist) == 0){
						//Controllo l'estensione:
						if (strstr(currentPlaylist, ".m3u") == NULL && strstr(currentPlaylist, ".M3U") == NULL){
							strcat(currentPlaylist, ".m3u");
						}
						//Salvo la playlist:
						strcpy(newPlaylist, playlistDir);
						strcat(newPlaylist, "/");
						strcat(newPlaylist, currentPlaylist);
						M3U_save(newPlaylist);
						strcpy(lastOp, "Palylist saved");
						pspDebugScreenSetXY(0, 20);
						pspDebugScreenSetBackColor(BLACK);
						pspDebugScreenSetTextColor(WHITE);
						pspDebugScreenPrintf("Filename: %-55.55s", currentPlaylist);
						pspDebugScreenSetXY(0, 21);
						pspDebugScreenPrintf("%-20.20s", "");
					}
				}
			} else if (controller.Buttons & PSP_CTRL_SELECT && M3U_getSongCount() > 0){
				if (confirm("Clear the playlist?", BLACK) == 1){
					M3U_clear();
					strcpy(currentPlaylist, "");
					screen_playlistEditor_init();
					pspDebugScreenSetXY(0, 20);
					pspDebugScreenSetBackColor(BLACK);
					pspDebugScreenSetTextColor(WHITE);
					pspDebugScreenPrintf("Filename: %-55.55s", "NEW PLAYLIST");
					selected_entry = 0;
					top_entry = 0;
					sceKernelDelayThread(200000);
				}
			} else if (controller.Buttons & PSP_CTRL_NOTE && M3U_getSongCount() > 0){
				if (confirm("Play the playlist?", BLACK) == 1){
					M3U_save(M3Ufilename);
					playM3U(M3Ufilename);
					screen_init();
					screen_toolbar();
					screen_playlistEditor_init();
					sel_updateInfo = 1;
					sceKernelDelayThread(200000);
				}
			}
			pspDebugScreenSetXY(1, starting_row);
			pspDebugScreenSetTextColor(WHITE);
			pspDebugScreenSetBackColor(0xaa4400);

			//Segno precedente:
			if (top_entry > 0){
				pspDebugScreenPrintf("%-66.66s", "...");
			}else{
				pspDebugScreenPrintf("%-66.66s", "");
			}
			int i = 0;
			for (; i < maximum_number_of_rows; i++){
				int current_entry = top_entry + i;

				pspDebugScreenSetXY(1, starting_row + i + 1);

				if (current_entry < M3U_getSongCount()){
					if (current_entry == selected_entry){
						pspDebugScreenSetBackColor(0x882200);
					} else {
						pspDebugScreenSetBackColor(0xcc6600);
					}
					pspDebugScreenSetTextColor(YELLOW);
					pspDebugScreenPrintf("%-66.66s", M3U_getSong(current_entry).title);
				} else {
					pspDebugScreenSetTextColor(WHITE);
					pspDebugScreenSetBackColor(0xaa4400);
					pspDebugScreenPrintf("%-66.66s", "");
				}
			}
			pspDebugScreenSetXY(1, starting_row + maximum_number_of_rows + 1);
			pspDebugScreenSetTextColor(WHITE);
			pspDebugScreenSetBackColor(0xaa4400);
			//Segno successivo:
			if (top_entry + maximum_number_of_rows < M3U_getSongCount()){
				pspDebugScreenPrintf("%-66.66s", "...");
			}else{
				pspDebugScreenPrintf("%-66.66s", "");
			}

			//Totale tracce:
			pspDebugScreenSetXY(56, starting_row + maximum_number_of_rows + 1);
			pspDebugScreenPrintf("Tracks: %3.1i", M3U_getSongCount());

			//Informazioni sulla playlist:
			if (M3U_isModified() == 1 && strlen(currentPlaylist) > 0){
				pspDebugScreenSetTextColor(RED);
				pspDebugScreenSetBackColor(BLACK);
				pspDebugScreenSetXY(0, 21);
				pspDebugScreenPrintf("Unsaved changes");
			}

			//Informazioni sulla traccia:
			if (sel_updateInfo == 1){
				pspDebugScreenSetTextColor(YELLOW);
				pspDebugScreenSetBackColor(BLACK);
				pspDebugScreenSetXY(0, 22);
				pspDebugScreenPrintf("Artist   : %-30.30s", tagInfo.artist);
				pspDebugScreenPrintf("\n");
				pspDebugScreenPrintf("Title    : %-30.30s", tagInfo.title);
				pspDebugScreenPrintf("\n");
				pspDebugScreenPrintf("Album    : %-30.30s", tagInfo.album);
				sel_updateInfo = 0;
			}

			//Tasti di uscita:
			if (controller.Buttons & PSP_CTRL_RTRIGGER || controller.Buttons & PSP_CTRL_LTRIGGER){
				break;
			}
			sceKernelDelayThread(100000);
		}
		//Salvo lo stato di modifica della playlist:
		playListModified = M3U_isModified();

		//Controllo input:
        if (controller.Buttons & PSP_CTRL_RTRIGGER){
			M3U_save(M3Ufilename);
			nextMode();
		} else if (controller.Buttons & PSP_CTRL_LTRIGGER){
			M3U_save(M3Ufilename);
			previousMode();
		}
	}
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//Options menu:
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void options_menu(){
    int optionsNumber = 13;
    char optionsText[13][51] = {"Audio Scrobbler Log",
                                "Fadeout when skipping track",
                                "Muted volume",
                                "Brightness check",
                                "Play MP3 using Media Engine",
                                "Boost type for MP3 using libMAD",
                                "Automatic CPU Clock",
                                "CPU Clock: GUI",
                                "CPU Clock: MP3 libMAD",
                                "CPU Clock: MP3 Media Engine",
                                "CPU Clock: OGG Vorbis",
                                "CPU Clock: Flac",
                                "CPU Clock: ATRAC3"
                               };
	int selected_entry         = 0;
	int top_entry              = 0;
	int maximum_number_of_rows = 20;
	int starting_row           = 4;
	char optionValue[17];
    SceCtrlData controller;
    
    screen_init();
    screen_options_init();
    screen_toolbar();

	while(runningFlag){
    	pspDebugScreenSetXY(1, starting_row);
    	pspDebugScreenSetTextColor(WHITE);
    	pspDebugScreenSetBackColor(0xaa4400);

    	//Segno precedente:
    	if (top_entry > 0){
    		pspDebugScreenPrintf("%-66.66s", "...");
    	}else{
    		pspDebugScreenPrintf("%-66.66s", "");
    	}
    	int i = 0;
    	for (; i < maximum_number_of_rows; i++){
    		int current_entry = top_entry + i;

    		pspDebugScreenSetXY(1, starting_row + i + 1);

    		if (current_entry < optionsNumber){
    			if (current_entry == selected_entry){
    				pspDebugScreenSetBackColor(0x882200);
    			} else {
    				pspDebugScreenSetBackColor(0xcc6600);
    			}
    			pspDebugScreenSetTextColor(YELLOW);
    			
    			//Check for value:
                switch (current_entry){
                    case 0:
                        sprintf(optionValue, "%i", userSettings.SCROBBLER);
                        break;
                    case 1:
                        sprintf(optionValue, "%i", userSettings.FADE_OUT);
                        break;
                    case 2:
                        sprintf(optionValue, "%i", userSettings.MUTED_VOLUME);
                        break;
                    case 3:
                        sprintf(optionValue, "%i", userSettings.BRIGHTNESS_CHECK);
                        break;
                    case 4:
                        sprintf(optionValue, "%i", userSettings.MP3_ME);
                        break;
                    case 5:
                        sprintf(optionValue, "%s", userSettings.BOOST);
                        break;
                    case 6:
                        sprintf(optionValue, "%i", userSettings.CLOCK_AUTO);
                        break;
                    case 7:
                        sprintf(optionValue, "%i", userSettings.CLOCK_GUI);
                        break;
                    case 8:
                        sprintf(optionValue, "%i", userSettings.CLOCK_MP3);
                        break;
                    case 9:
                        sprintf(optionValue, "%i", userSettings.CLOCK_MP3ME);
                        break;
                    case 10:
                        sprintf(optionValue, "%i", userSettings.CLOCK_OGG);
                        break;
                    case 11:
                        sprintf(optionValue, "%i", userSettings.CLOCK_FLAC);
                        break;
                    case 12:
                        sprintf(optionValue, "%i", userSettings.CLOCK_AA3);
                        break;
                    default:
                        strcpy(optionValue, "");
                }
                
    			pspDebugScreenPrintf("%-50.50s%-16.16s", optionsText[current_entry], optionValue);
    		} else {
    			pspDebugScreenSetTextColor(WHITE);
    			pspDebugScreenSetBackColor(0xaa4400);
    			pspDebugScreenPrintf("%-66.66s", "");
    		}
    	}
    	pspDebugScreenSetXY(1, starting_row + maximum_number_of_rows + 1);
    	pspDebugScreenSetTextColor(WHITE);
    	pspDebugScreenSetBackColor(0xaa4400);
    	//Segno successivo:
    	if (top_entry + maximum_number_of_rows < optionsNumber){
    		pspDebugScreenPrintf("%-66.66s", "...");
    	}else{
    		pspDebugScreenPrintf("%-66.66s", "");
    	}

        readButtons(&controller, 1);
        //Check home popup:
		if(controller.Buttons & PSP_CTRL_HOME)
          exitScreen();

        //Tasti di navigazione:
		if ((controller.Buttons & PSP_CTRL_DOWN) | (controller.Ly > 128 + ANALOG_SENS && !(controller.Buttons & PSP_CTRL_HOLD))){
			if (selected_entry + 1 < optionsNumber){
				selected_entry++;
				if (selected_entry == top_entry + maximum_number_of_rows)
					top_entry++;
			}
		} else if ((controller.Buttons & PSP_CTRL_UP) | (controller.Ly < 128 - ANALOG_SENS && !(controller.Buttons & PSP_CTRL_HOLD))){
			if (selected_entry){
				selected_entry--;
				if (selected_entry == top_entry - 1)
					top_entry--;
			}

        //Modifica di una opzione:
		} else if (controller.Buttons & PSP_CTRL_RIGHT || controller.Buttons & PSP_CTRL_LEFT || (controller.Lx < 128 - ANALOG_SENS && !(controller.Buttons & PSP_CTRL_HOLD)) || (controller.Lx > 128 + ANALOG_SENS && !(controller.Buttons & PSP_CTRL_HOLD))){
                int delta = 0;
                if (controller.Buttons & PSP_CTRL_RIGHT || controller.Lx > 128 + ANALOG_SENS)
                    delta = 1;
                else
                    delta = -1;
                    
                switch (selected_entry){
                    case 0:
                        userSettings.SCROBBLER = !userSettings.SCROBBLER;
                        break;
                    case 1:
                        userSettings.FADE_OUT = !userSettings.FADE_OUT;
                        break;
                    case 2:
                        userSettings.MUTED_VOLUME += delta * 10;
                        if (userSettings.MUTED_VOLUME > 8000)
                            userSettings.MUTED_VOLUME = 8000;
                        else if (userSettings.MUTED_VOLUME < 0)
                            userSettings.MUTED_VOLUME = 0;
                        break;
                    case 3:
                        userSettings.BRIGHTNESS_CHECK += delta;
                        if (userSettings.BRIGHTNESS_CHECK > 2)
                            userSettings.BRIGHTNESS_CHECK = 2;
                        else if (userSettings.BRIGHTNESS_CHECK < 0)
                            userSettings.BRIGHTNESS_CHECK = 0;
                        break;
                    case 4:
                        userSettings.MP3_ME = !userSettings.MP3_ME;
                        break;
                    case 5:
                        if (!strcmp(userSettings.BOOST, "NEW"))
                            strcpy(userSettings.BOOST, "OLD");
                        else
                            strcpy(userSettings.BOOST, "NEW");
                        break;
                    case 6:
                        userSettings.CLOCK_AUTO = !userSettings.CLOCK_AUTO;
                        break;
                    case 7:
                        userSettings.CLOCK_GUI += delta;
                        if (userSettings.CLOCK_GUI > 222)
                            userSettings.CLOCK_GUI = 222;
                        else if (userSettings.CLOCK_GUI < 10)
                            userSettings.CLOCK_GUI = 10;
                        break;
                    case 8:
                        userSettings.CLOCK_MP3 += delta;
                        if (userSettings.CLOCK_MP3 > 222)
                            userSettings.CLOCK_MP3 = 222;
                        else if (userSettings.CLOCK_MP3 < 10)
                            userSettings.CLOCK_MP3 = 10;
                        break;
                    case 9:
                        userSettings.CLOCK_MP3ME += delta;
                        if (userSettings.CLOCK_MP3ME > 222)
                            userSettings.CLOCK_MP3ME = 222;
                        else if (userSettings.CLOCK_MP3ME < 10)
                            userSettings.CLOCK_MP3ME = 10;
                        break;
                    case 10:
                        userSettings.CLOCK_OGG += delta;
                        if (userSettings.CLOCK_OGG > 222)
                            userSettings.CLOCK_OGG = 222;
                        else if (userSettings.CLOCK_OGG < 10)
                            userSettings.CLOCK_OGG = 10;
                        break;
                    case 11:
                        userSettings.CLOCK_FLAC += delta;
                        if (userSettings.CLOCK_FLAC > 222)
                            userSettings.CLOCK_FLAC = 222;
                        else if (userSettings.CLOCK_FLAC < 10)
                            userSettings.CLOCK_FLAC = 10;
                        break;
                    case 12:
                        userSettings.CLOCK_AA3 += delta;
                        if (userSettings.CLOCK_AA3 > 222)
                            userSettings.CLOCK_AA3 = 222;
                        else if (userSettings.CLOCK_AA3 < 10)
                            userSettings.CLOCK_AA3 = 10;
                        break;
                }
                sceKernelDelayThread(200000);
		} else if (controller.Buttons & PSP_CTRL_LEFT){


        //Conferma e default:
		} else if (controller.Buttons & PSP_CTRL_START){
            if (confirm("Save options now?", BLACK) == 1)
                SETTINGS_save(userSettings);
            sceKernelDelayThread(200000);
		} else if (controller.Buttons & PSP_CTRL_SELECT){
            if (confirm("Restore default options?", BLACK) == 1)
                userSettings = SETTINGS_default();
            sceKernelDelayThread(200000);
		}
		
		//Tasti di uscita:
		if (controller.Buttons & PSP_CTRL_RTRIGGER || controller.Buttons & PSP_CTRL_LTRIGGER)
			break;
		sceKernelDelayThread(100000);
    }
	//Controllo input:
    if (controller.Buttons & PSP_CTRL_RTRIGGER)
		nextMode();
	else if (controller.Buttons & PSP_CTRL_LTRIGGER)
		previousMode();
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//Main
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
int main() {
	pspDebugScreenInit();
	//SetupCallbacks();
    initExceptionHandler();
    
    //Start support prx
	SceUID modid = pspSdkLoadStartModule("support.prx", PSP_MEMORY_PARTITION_KERNEL);
	if (modid < 0){
        pspDebugScreenSetXY(0, 4);
        pspDebugScreenSetTextColor(RED);
        pspDebugScreenPrintf("Error 0x%08X loading/starting support.prx.\n", modid);
        sceKernelDelayThread(3000000);
        sceKernelExitGame();
        return -1;
	}

	//openLog("ms0:/lightMP3.log", 0);
    //Directory corrente:
	getcwd(ebootDirectory, 262);

	//Nome per settings:
	strcpy(playlistDir, ebootDirectory);
	if (playlistDir[strlen(playlistDir)-1] != '/'){
		strcat(playlistDir, "/");
	}
	strcat(playlistDir, "settings");
	//Carico le opzioni:
	if (SETTINGS_load(playlistDir)){
		userSettings = SETTINGS_get();
	}else{
        userSettings = SETTINGS_default();
		strcpy(userSettings.fileName, playlistDir);
    }

    //Check for active modules:
    checkModules();
    
    MUTED_VOLUME = userSettings.MUTED_VOLUME;
    
    //Clock per filetype:
    MP3_defaultCPUClock = userSettings.CLOCK_MP3;
    MP3ME_defaultCPUClock = userSettings.CLOCK_MP3ME;
    OGG_defaultCPUClock = userSettings.CLOCK_OGG;
    FLAC_defaultCPUClock = userSettings.CLOCK_FLAC;
    AA3ME_defaultCPUClock = userSettings.CLOCK_AA3;
    
    //Disable the ME (su slim freeza al cambio di clock di CPU se lo eseguo):
    //if (sceKernelDevkitVersion() < 0x03070110 && !userSettings.MP3_ME)
    //    MEDisable();
        
    volumeBoost = userSettings.BOOST_VALUE;
    
    //Volume iniziale:
    int initialMute = imposeGetMute();
    if (initialMute)
        imposeSetMute(0);
    int initialVolume = imposeGetVolume();
    imposeSetVolume(userSettings.VOLUME);             
    
    //Velocità di clock:
    if (sceKernelDevkitVersion() < 0x03070110)
    	scePowerSetClockFrequency(222, 222, 111);
    else
        scePowerSetClockFrequency(222, 222, 95);

	//scePowerSetCpuClockFrequency(userSettings.CPU);
    setCpuClock(userSettings.CPU);
    if (scePowerGetCpuClockFrequency() < userSettings.CPU){
        //scePowerSetCpuClockFrequency(++userSettings.CPU);
        setCpuClock(++userSettings.CPU);
    }

    if (sceKernelDevkitVersion() < 0x03070110){
        scePowerSetBusClockFrequency(userSettings.BUS);
        if (scePowerGetBusClockFrequency() < userSettings.BUS){
            scePowerSetBusClockFrequency(++userSettings.BUS);
        }
    }

	//AudioScrobbler:
	if (userSettings.SCROBBLER){
		strcpy(scrobblerLog, ebootDirectory);
		if (scrobblerLog[strlen(scrobblerLog)-1] != '/'){
			strcat(scrobblerLog, "/");
		}
		strcat(scrobblerLog, ".scrobbler.log");
		SCROBBLER_init(scrobblerLog);
	}

	tzset();

	//Creo directory per playlist:
	strcpy(playlistDir, ebootDirectory);
	if (playlistDir[strlen(playlistDir)-1] != '/'){
		strcat(playlistDir, "/");
	}
	strcat(playlistDir, "playLists");
	sceIoMkdir(playlistDir, 0777);

	//Nome per playlist temporanea:
	strcpy(tempM3Ufile, ebootDirectory);
	if (tempM3Ufile[strlen(tempM3Ufile)-1] != '/'){
		strcat(tempM3Ufile, "/");
	}
	strcat(tempM3Ufile, "temp.m3u");
	FILE *f;
	f = fopen(tempM3Ufile, "w");
    if (f != NULL){
        fwrite("#EXTM3U\n", 1, strlen("#EXTM3U\n"), f);
        fclose(f);
    }
    
    //Disable home button:
    imposeSetHomePopup(0);
    
	//Controllo luminosità:
    int initialBrightness = getBrightness();
	int initialBrightnessValue = imposeGetBrightness();
	
	if (userSettings.BRIGHTNESS_CHECK == 1)
    	checkBrightness();
    else if (userSettings.BRIGHTNESS_CHECK == 2){
        setBrightness(24);
    	curBrightness = 24;
        imposeSetBrightness(0);
    }
    
	//Equalizer init:
	struct equalizer tEQ;
	EQ_init();

	tEQ = EQ_getShort(userSettings.EQ);
	if (strlen(tEQ.shortName) == 0){
        tEQ = EQ_getIndex(0);
    }
    currentEQ = tEQ.index;

	srand(time(NULL));
	current_mode = 0;
	while (current_mode >= 0 && runningFlag){
		if (current_mode == 0){
			fileBrowser_menu();
		}else if (current_mode == 1){
			playlist_menu();
		}else if (current_mode == 2){
			playlist_editor();
		}else if (current_mode == 3){
			options_menu();
		}
	}

	//Salvo le opzioni:
    userSettings.CPU = scePowerGetCpuClockFrequency();
    userSettings.BUS = scePowerGetBusClockFrequency();
    userSettings.VOLUME = imposeGetVolume();
    tEQ = EQ_getIndex(currentEQ);
    strcpy(userSettings.EQ, tEQ.shortName);
	SETTINGS_save(userSettings);

	//Riporto la luminosità a quella iniziale:
    setBrightness(initialBrightness);
    imposeSetBrightness(initialBrightnessValue);

    //Riporto il volume al suo valore iniziale:
    imposeSetVolume(initialVolume);             
    if (initialMute)
        imposeSetMute(initialMute);
    
	sceKernelExitGame();
	return(0);
}
