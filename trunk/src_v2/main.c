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
#include <psprtc.h>
#include <stdio.h>
#include <kubridge.h>
#include <oslib/oslib.h>

#include "main.h"
#include "gui/common.h"
#include "gui/languages.h"
#include "gui/menu.h"
#include "gui/skinsettings.h"
#include "gui/gui_fileBrowser.h"
#include "gui/gui_playlistsBrowser.h"
#include "gui/gui_playlistEditor.h"
#include "gui/gui_player.h"
#include "gui/gui_settings.h"
#include "gui/gui_mediaLibrary.h"

#include "system/exception.h"
#include "system/clock.h"
#include "system/brightness.h"
#include "players/m3u.h"
#include "players/player.h"
#include "players/equalizer.h"
#include "others/log.h"
#include "others/audioscrobbler.h"
#include "others/medialibrary.h"

PSP_MODULE_INFO("LightMP3", 0, 1, 0);
PSP_MAIN_THREAD_ATTR(PSP_THREAD_ATTR_USER | PSP_THREAD_ATTR_VFPU);
PSP_HEAP_SIZE_KB(-4*1024);

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Globals:
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
static char ebootDirectory[264] = "";
static int resuming = 0;
static int suspended = 0;
struct settings *userSettings;

//Shared globals:
int fileExtCount = 10;
char fileExt[10][5] = {"MP3", "OGG", "AA3", "OMA", "OMG", "FLAC", "AAC", "M4A", "WMA", "M3U"};
char tempM3Ufile[264] = "";
char MLtempM3Ufile[264] = "";
struct libraryEntry MLresult[ML_BUFFERSIZE];
struct menuElements commonMenu;
struct menuElements commonSubMenu;
int tempPos[2];
int tempColor[4];
int tempColorShadow[4];
float defaultTextSize = 0.0f;

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Functions imported from prx:
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
int imposeGetVolume();
int imposeSetVolume();
int imposeGetMute();
int imposeSetMute(int value);
int getBrightness();
void setBrightness(int brightness);
int imposeSetBrightness(int value);
int imposeGetBrightness();
int displayEnable(void);
int displayDisable(void);
int getBrightness();
void setBrightness(int brightness);
int readButtons(SceCtrlData *pad_data, int count);
int imposeSetHomePopup(int value);

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Power Callback:
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
int powerCallback(int unknown, int powerInfo, void *common){
    static int reopenTrans = 0;
    if ((powerInfo & PSP_POWER_CB_SUSPENDING) || (powerInfo & PSP_POWER_CB_POWER_SWITCH)){
       if (!suspended){
           resuming = 1;
           //Suspend
           if (suspendFunct != NULL)
              (*suspendFunct)();
           if (ML_inTransaction())
           {
              ML_commitTransaction();
              reopenTrans = 1;
           }
           //Riattivo il display:
           if (!userSettings->displayStatus){
               displayEnable();
               imposeSetHomePopup(1);
               setBrightness(userSettings->curBrightness);
               userSettings->displayStatus = 1;
           }
		   sceKernelDcacheWritebackAll();
           suspended = 1;
       }
    }else if (powerInfo & PSP_POWER_CB_RESUMING){
       resuming = 1;
    }else if (powerInfo & PSP_POWER_CB_RESUME_COMPLETE){
       //Resume
       if (reopenTrans)
       {
          ML_startTransaction();
          reopenTrans = 0;
       }
       if (resumeFunct != NULL)
          (*resumeFunct)();
       resuming = 0;
       suspended = 0;
    }
    return 0;
}


///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Callbacks:
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/* Exit callback */
int exit_callback(int arg1, int arg2, void *common) {
   if (endFunct != NULL)
      (*endFunct)();
    osl_quit = 1;
    return 0;
}

/* Callback thread */
int CallbackThread(SceSize args, void *argp) {
    int power_cbid = sceKernelCreateCallback("powercb", (SceKernelCallbackFunction)powerCallback, NULL);
    scePowerRegisterCallback(0, power_cbid);
    sceKernelSleepThreadCB();
    return 0;
}

/* Sets up the callback thread and returns its thread id */
int SetupCallbacks(void) {
    int thid = 0;

    thid = sceKernelCreateThread("update_thread", CallbackThread, 0x11, 0x10000, PSP_THREAD_ATTR_USER, 0);
    if(thid >= 0)
        sceKernelStartThread(thid, 0, 0);
    return thid;
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Set initial clock:
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void setInitialClock(){
    //Velocità di clock:
    if (sceKernelDevkitVersion() < 0x03070110)
    	scePowerSetClockFrequency(222, 222, 111);
    else
        scePowerSetClockFrequency(222, 222, 95);

    //Imposto il clock:
    if (userSettings->CLOCK_AUTO)
        setCpuClock(userSettings->CLOCK_GUI);
    else{
        setCpuClock(userSettings->CPU);
        if (scePowerGetCpuClockFrequency() < userSettings->CPU)
            setCpuClock(++userSettings->CPU);
    }

    if (sceKernelDevkitVersion() < 0x03070110){
        scePowerSetBusClockFrequency(userSettings->BUS);
        if (scePowerGetBusClockFrequency() < userSettings->BUS)
            scePowerSetBusClockFrequency(++userSettings->BUS);
    }
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Splash:
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
int splashThread(SceSize args, void *argp){
    sceIoChdir(ebootDirectory);
    oslShowSplashScreen(1);
    sceKernelExitDeleteThread(0);

    /*OSL_IMAGE *splash;
    splash = oslLoadImageFilePNG("logo/neoflash.png", OSL_IN_RAM | OSL_SWIZZLED, OSL_PF_8888);
    if (splash){
        oslStartDrawing();
        oslDrawImageXY(splash, 0, 0);
        oslEndDrawing();
        oslEndFrame();
        oslSyncFrame();

        sceKernelDelayThread(3*1000000);
        oslDeleteImage(splash);
        splash = NULL;
    }
    sceKernelExitDeleteThread(0);*/
    return 0;
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Init OSLib:
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void initOSLib(){
    oslInit(0);
    oslInitGfx(OSL_PF_8888, 1);
    oslSetQuitOnLoadFailure(0);
    //Init input
    //oslSetReadKeysFunction(readButtons);
    oslSetHoldForAnalog(1);
    oslSetKeyAnalogToDPad(ANALOG_SENS);
    oslSetExitCallback(exit_callback);
    oslSetBkColor(RGBA(0, 0, 0, 0));
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// End OSLib:
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void endOSLib(){
    oslEndGfx();
    osl_quit = 1;
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Check initial brightness:
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void checkBrightness(){
    int done = 0;

    int initialB = getBrightness();
    int minB = 0;
    if (getModel() == PSP_MODEL_SLIM_AND_LITE || sceKernelDevkitVersion() >= 0x03070110)
        minB = 36;
    else
        minB = 24;

    if (initialB > minB){
        while(!osl_quit && !done){
            oslStartDrawing();
            drawCommonGraphics();
            drawConfirm(langGetString("CHECK_BRIGHTNESS_TITLE"), langGetString("CHECK_BRIGHTNESS"));
        	oslEndDrawing();
            oslEndFrame();
        	oslSyncFrame();

            oslReadKeys();
            if(getConfirmButton()){
				fadeDisplay(minB, DISPLAY_FADE_TIME);
            	userSettings->curBrightness = minB;
                imposeSetBrightness(0);
                done = 1;
            }else if(getCancelButton()){
                done = 1;
            }
        }
    }
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Ask to confirm to load a bookmark at startup
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
int confirmLoadBookmark()
{
    int retValue = 0;
    int skip = 0;

    while(!osl_quit)
    {
        if (!skip)
        {
            oslStartDrawing();
            drawCommonGraphics();
            drawConfirm(langGetString("CONFIRM_BOOKMARK_TITLE"), langGetString("CONFIRM_BOOKMARK_LOAD"));
        	oslEndDrawing();
        }
        oslEndFrame();
        skip = oslSyncFrame();

        oslReadKeys();
        if(getConfirmButton())
        {
            retValue = 1;
            break;
        }
        else if(getCancelButton())
        {
            retValue = 0;
            break;
        }
    }

    oslReadKeys();
    return retValue;
}


///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Main:
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
int main(){
    char buffer[264] = "";
    int currentMode = MODE_FILEBROWSER;

    initExceptionHandler();
	//initTimezone();
    //openLog("lightMP3.log", 0);

    //Init:
    SetupCallbacks();
    initOSLib();
    setSwapButton();

    getcwd(ebootDirectory, 256);
	//Full name for settings' file:
	if (ebootDirectory[strlen(ebootDirectory)-1] != '/')
		strcat(ebootDirectory, "/");
    snprintf(buffer, sizeof(buffer), "%s%s", ebootDirectory, "settings");

	//Carico le opzioni:
	if (!SETTINGS_load(buffer))
		userSettings = SETTINGS_get();
	else{
        userSettings = SETTINGS_default();
		strcpy(userSettings->fileName, buffer);
    }
    currentMode = userSettings->startTab;
    strcpy(userSettings->ebootPath, ebootDirectory);
    userSettings->displayStatus = 1;
    userSettings->BUS = getMinBUSClock(); //Override the saved value (the less == the better!)
    oslSetFrameskip(userSettings->frameskip);

    //Splash screen:
	int splash_thid = 0;
	if (userSettings->SHOW_SPLASH){
		splash_thid = sceKernelCreateThread("splash", splashThread, 15, DEFAULT_THREAD_STACK_SIZE, PSP_THREAD_ATTR_USER | THREAD_ATTR_VFPU, NULL);
		if(splash_thid >= 0)
			sceKernelStartThread(splash_thid, 0, NULL);
	}

    //Start support prx
	SceUID modid = pspSdkLoadStartModule("support.prx", PSP_MEMORY_PARTITION_KERNEL);
	if (modid < 0){
        snprintf(buffer, sizeof(buffer), "Error 0x%08X loading/starting support.prx!\n", modid);
        debugMessageBox(buffer);
        sceKernelDelayThread(3000000);
        sceKernelExitGame();
        return -1;
	}

    //Start miniconv.prx
	modid = pspSdkLoadStartModule("miniconv.prx", PSP_MEMORY_PARTITION_USER);
	if (modid < 0){
        char buffer[128];
        snprintf(buffer, sizeof(buffer), "Error 0x%08X loading/starting miniconv.prx!\n", modid);
        debugMessageBox(buffer);
        sceKernelDelayThread(3000000);
        sceKernelExitGame();
        return -1;
	}

    oslSetKeyAutorepeatInit(userSettings->KEY_AUTOREPEAT_GUI);
    oslSetKeyAutorepeatInterval(userSettings->KEY_AUTOREPEAT_GUI);
    //oslSetRemoteKeyAutorepeatInit(userSettings->KEY_AUTOREPEAT_PLAYER);
    //oslSetRemoteKeyAutorepeatInterval(userSettings->KEY_AUTOREPEAT_PLAYER);

    //Temp m3u filename:
    snprintf(MLtempM3Ufile, sizeof(MLtempM3Ufile), "%s%s", userSettings->ebootPath, "MLtemp.m3u");
    snprintf(tempM3Ufile, sizeof(tempM3Ufile), "%s%s", userSettings->ebootPath, "temp.m3u");
    M3U_clear();
    M3U_save(tempM3Ufile);

    //Clock per filetype:
    MP3_defaultCPUClock = userSettings->CLOCK_MP3;
    MP3ME_defaultCPUClock = userSettings->CLOCK_MP3ME;
    OGG_defaultCPUClock = userSettings->CLOCK_OGG;
    FLAC_defaultCPUClock = userSettings->CLOCK_FLAC;
    AA3ME_defaultCPUClock = userSettings->CLOCK_AA3;
    AAC_defaultCPUClock = userSettings->CLOCK_AAC;
    WMA_defaultCPUClock = userSettings->CLOCK_WMA;
	CLOCK_WHEN_PAUSE = getMinCPUClock();

    //Load language file:
    snprintf(buffer, sizeof(buffer), "%slanguages/", userSettings->ebootPath);
    langLoadList(buffer);
    snprintf(buffer, sizeof(buffer), "%slanguages/%s/lang.txt", userSettings->ebootPath, userSettings->lang);
    if (langLoad(buffer)){
        debugMessageBox("Error loading language file.");
    	sceKernelExitGame();
        return 0;
    }
	initFonts();

    snprintf(buffer, sizeof(buffer), "%sskins/", userSettings->ebootPath);
    skinLoadList(buffer);
    snprintf(buffer, sizeof(buffer), "%sskins/%s/skin.cfg", userSettings->ebootPath, userSettings->skinName);
    skinLoad(buffer);

   	//Skin's s images path:
	if ( skinGetString("STR_IMAGE_PATH", buffer) == 0 ) {
    	snprintf(userSettings->skinImagesPath, sizeof(userSettings->skinImagesPath), "%sskins/%s/images", userSettings->ebootPath, buffer);
	}
	else {
    	snprintf(userSettings->skinImagesPath, sizeof(userSettings->skinImagesPath), "%sskins/%s/images", userSettings->ebootPath, userSettings->skinName);
	}

	//Default text size:
	defaultTextSize = skinGetParam("FONT_NORMAL_SIZE") / 100.0;

    //Volume iniziale:
    int initialMute = imposeGetMute();
    if (initialMute)
        imposeSetMute(0);
    int initialVolume = imposeGetVolume();
    imposeSetVolume(userSettings->VOLUME);

	//Equalizer init:
	struct equalizer tEQ;
	EQ_init();

	tEQ = EQ_getShort(userSettings->EQ);
	if (strlen(tEQ.shortName) == 0)
        tEQ = EQ_getIndex(0);
    userSettings->currentEQ = tEQ.index;

	//AudioScrobbler:
	if (userSettings->SCROBBLER){
        snprintf(buffer, sizeof(buffer), "%s%s", ebootDirectory, ".scrobbler.log");
		SCROBBLER_init(buffer);
	}

	//Media library:
    if (!ML_existsDB(ebootDirectory, ML_DB))
        ML_createEmptyDB(ebootDirectory, ML_DB);
    ML_openDB(ebootDirectory, ML_DB);

    loadCommonGraphics();
    initMenu();
    setInitialClock();
    strcpy(userSettings->lastBrowserDir, "");

    //Random seed:
	u64 mytime = 0;
	sceRtcGetCurrentTick (&mytime);
    //oslSrand(mytime);
    srand(mytime);


    //Wait for splash:
	if (userSettings->SHOW_SPLASH)
		sceKernelWaitThreadEnd(splash_thid, NULL);
    oslSetReadKeysFunction(readButtons);
	oslSetDepthTest(1);

	//Controllo luminosità:
    int initialBrightness = getBrightness();
	int initialBrightnessValue = imposeGetBrightness();

	if (userSettings->BRIGHTNESS_CHECK == 1)
    	checkBrightness();
    else if (userSettings->BRIGHTNESS_CHECK == 2){
		fadeDisplay(24, DISPLAY_FADE_TIME);
    	userSettings->curBrightness = 24;
        imposeSetBrightness(0);
    }

    //Check for bookmark:
    snprintf(buffer, sizeof(buffer), "%s%s", userSettings->ebootPath, "bookmark.bkm");
    if (fileExists(buffer) > 0)
    {
        if (confirmLoadBookmark())
        {
            currentMode = MODE_PLAYER;
            strcpy(userSettings->selectedBrowserItemShort, buffer);
            strcpy(userSettings->selectedBrowserItem, buffer);
        }
    }

    while(!osl_quit){
		cpuBoost();
        switch (currentMode){
            case (MODE_FILEBROWSER):
                currentMode = gui_fileBrowser();
                break;
            case (MODE_PLAYLISTS):
                currentMode = gui_playlistsBrowser();
                break;
            case (MODE_PLAYLIST_EDITOR):
                currentMode = gui_playlistEditor();
                break;
            case (MODE_MEDIA_LIBRARY):
                currentMode = gui_mediaLibrary();
                break;
            case (MODE_SETTINGS):
                currentMode = gui_settings();
                break;
            case (MODE_PLAYER):
                currentMode = gui_player();
                break;
            default:
                osl_quit = 1;
                break;
        }
    }
	//Salvo le opzioni:
    userSettings->CPU = scePowerGetCpuClockFrequency();
    userSettings->BUS = scePowerGetBusClockFrequency();
    userSettings->VOLUME = imposeGetVolume();
    tEQ = EQ_getIndex(userSettings->currentEQ);
    strcpy(userSettings->EQ, tEQ.shortName);
	SETTINGS_save(userSettings);

	//Riporto la luminosità a quella iniziale:
    setBrightness(initialBrightness);
    imposeSetBrightness(initialBrightnessValue);

    //Riporto il volume al suo valore iniziale:
    imposeSetVolume(initialVolume);
    if (initialMute)
        imposeSetMute(initialMute);

    ML_closeDB();
    endOSLib();

    //Sleep mode:
    if (userSettings->shutDown)
        scePowerRequestStandby();
	sceKernelExitGame();
    return 0;
}
