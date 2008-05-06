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
#include <psppower.h>
#include <pspsysmem.h>
#include <psprtc.h>
#include <oslib/oslib.h>
#include <stdio.h>
#include <unistd.h>
#include <string.h>

#include "common.h"
#include "menu.h"
#include "languages.h"
#include "../main.h"
#include "../system/freeMem.h"
#include "../system/opendir.h"
#include "../others/medialibrary.h"
#include "settings.h"
#include "skinsettings.h"

// Functions imported from support prx:
int imposeGetVolume();
int readButtons(SceCtrlData *pad_data, int count);

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Globals
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
static char buffer[264];
OSL_IMAGE *commonBkg;
OSL_IMAGE *commonTopToolbar;
OSL_IMAGE *commonBottomToolbar;
OSL_IMAGE *commonButtons[5];
OSL_IMAGE *commonButtonsSel[5];
OSL_IMAGE *commonBattery;
OSL_IMAGE *commonBatteryLow;
OSL_IMAGE *commonBatteryCharging;
OSL_IMAGE *popupBkg;
OSL_IMAGE *star;
OSL_IMAGE *blankStar;
OSL_IMAGE *helpBkg;
OSL_IMAGE *commonVolume;
OSL_IMAGE *commonHeadphone;
OSL_IMAGE *commonCPU;
OSL_IMAGE *commonMenuHighlight;

OSL_FONT *fontNormal;

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Load common graphics:
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
int loadCommonGraphics(){
    int i = 0;

    //Load images:
    sprintf(buffer, "%s/bkg_default.png", userSettings->skinImagesPath);
    commonBkg = oslLoadImageFilePNG(buffer, OSL_IN_RAM | OSL_SWIZZLED, OSL_PF_8888);
    if (!commonBkg)
        errorLoadImage(buffer);

    sprintf(buffer, "%s/toptoolbar.png", userSettings->skinImagesPath);
    commonTopToolbar = oslLoadImageFilePNG(buffer, OSL_IN_RAM | OSL_SWIZZLED, OSL_PF_8888);
    if (!commonTopToolbar)
        errorLoadImage(buffer);

    sprintf(buffer, "%s/bottomtoolbar.png", userSettings->skinImagesPath);
    commonBottomToolbar = oslLoadImageFilePNG(buffer, OSL_IN_RAM | OSL_SWIZZLED, OSL_PF_8888);
    if (!commonBottomToolbar)
        errorLoadImage(buffer);

    for (i=0; i<5; i++){
        sprintf(buffer, "%s/button%i.png", userSettings->skinImagesPath, i + 1);
        commonButtons[i] = oslLoadImageFilePNG(buffer, OSL_IN_RAM | OSL_SWIZZLED, OSL_PF_8888);
        if (!commonButtons[i])
            errorLoadImage(buffer);
        sprintf(buffer, "%s/buttonsel%i.png", userSettings->skinImagesPath, i + 1);
        commonButtonsSel[i] = oslLoadImageFilePNG(buffer, OSL_IN_RAM | OSL_SWIZZLED, OSL_PF_8888);
        if (!commonButtonsSel[i])
            errorLoadImage(buffer);
    }

    sprintf(buffer, "%s/battery.png", userSettings->skinImagesPath);
    commonBattery = oslLoadImageFilePNG(buffer, OSL_IN_RAM | OSL_SWIZZLED, OSL_PF_8888);
    if (!commonBattery)
        errorLoadImage(buffer);

    sprintf(buffer, "%s/batterylow.png", userSettings->skinImagesPath);
    commonBatteryLow = oslLoadImageFilePNG(buffer, OSL_IN_RAM | OSL_SWIZZLED, OSL_PF_8888);
    if (!commonBatteryLow)
        errorLoadImage(buffer);

    sprintf(buffer, "%s/batterycharging.png", userSettings->skinImagesPath);
    commonBatteryCharging = oslLoadImageFilePNG(buffer, OSL_IN_RAM | OSL_SWIZZLED, OSL_PF_8888);
    if (!commonBatteryCharging)
        errorLoadImage(buffer);

    sprintf(buffer, "%s/volume.png", userSettings->skinImagesPath);
    commonVolume = oslLoadImageFilePNG(buffer, OSL_IN_RAM | OSL_SWIZZLED, OSL_PF_8888);
    if (!commonVolume)
        errorLoadImage(buffer);

    sprintf(buffer, "%s/headphone.png", userSettings->skinImagesPath);
    commonHeadphone = oslLoadImageFilePNG(buffer, OSL_IN_RAM | OSL_SWIZZLED, OSL_PF_8888);
    if (!commonHeadphone)
        errorLoadImage(buffer);

    sprintf(buffer, "%s/cpu.png", userSettings->skinImagesPath);
    commonCPU = oslLoadImageFilePNG(buffer, OSL_IN_RAM | OSL_SWIZZLED, OSL_PF_8888);
    if (!commonCPU)
        errorLoadImage(buffer);

    sprintf(buffer, "%s/popupbkg.png", userSettings->skinImagesPath);
    popupBkg = oslLoadImageFilePNG(buffer, OSL_IN_RAM | OSL_SWIZZLED, OSL_PF_8888);
    if (!popupBkg)
        errorLoadImage(buffer);

    sprintf(buffer, "%s/star.png", userSettings->skinImagesPath);
    star = oslLoadImageFilePNG(buffer, OSL_IN_RAM | OSL_SWIZZLED, OSL_PF_8888);
    if (!star)
        errorLoadImage(buffer);

    sprintf(buffer, "%s/blankstar.png", userSettings->skinImagesPath);
    blankStar = oslLoadImageFilePNG(buffer, OSL_IN_RAM | OSL_SWIZZLED, OSL_PF_8888);
    if (!blankStar)
        errorLoadImage(buffer);

    sprintf(buffer, "%s/helpbkg.png", userSettings->skinImagesPath);
    helpBkg = oslLoadImageFilePNG(buffer, OSL_IN_RAM | OSL_SWIZZLED, OSL_PF_8888);
    if (!helpBkg)
        errorLoadImage(buffer);

    sprintf(buffer, "%s/menuhighlight.png", userSettings->skinImagesPath);
    commonMenuHighlight = oslLoadImageFilePNG(buffer, OSL_IN_RAM | OSL_SWIZZLED, OSL_PF_8888);
    if (!commonMenuHighlight)
        errorLoadImage(buffer);

    //sprintf(buffer, "%s/fontNormal.oft", userSettings->skinImagesPath);
    sprintf(buffer, "flash0:/font/ltn0.pgf");
    //sprintf(buffer, "%s/fontNormal.pgf", userSettings->skinImagesPath);
	fontNormal = oslLoadFontFile(buffer);
    if (!fontNormal)
        errorLoadImage(buffer);
    oslIntraFontSetStyle(fontNormal, 0.5f,0xFFFFFFFF,0xFF000000,INTRAFONT_ALIGN_LEFT);
    fontNormal->charHeight -= 2;
    return 0;
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// unLoad common graphics:
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
int unLoadCommonGraphics(){
    int i = 0;
    oslDeleteImage(commonBkg);
    oslDeleteImage(commonTopToolbar);
    oslDeleteImage(commonBottomToolbar);
    for (i=0; i<5; i++){
        oslDeleteImage(commonButtons[i]);
        oslDeleteImage(commonButtonsSel[i]);
    }
    oslDeleteImage(commonBattery);
    oslDeleteImage(commonBatteryLow);
    oslDeleteImage(commonBatteryCharging);
    oslDeleteImage(commonVolume);
    oslDeleteImage(commonHeadphone);
    oslDeleteImage(commonCPU);

    oslDeleteImage(popupBkg);
    oslDeleteImage(star);
    oslDeleteImage(blankStar);
    oslDeleteImage(helpBkg);
    oslDeleteImage(commonMenuHighlight);
    oslDeleteFont(fontNormal);
    return 0;
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Draw toolbars:
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
int drawToolbars(){
    int m, h, btrh, btrm, btr, rh, battPerc;
    char remote[3] = "";

    skinGetPosition("POS_TOOLBAR_1", tempPos);
    oslDrawImageXY(commonTopToolbar, tempPos[0], tempPos[1]);
    skinGetPosition("POS_TOOLBAR_2", tempPos);
    oslDrawImageXY(commonBottomToolbar, tempPos[0], tempPos[1]);

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

    if (oslIsRemoteExist())
        strcpy(remote, "RM");

	//Current time:
    pspTime currTime;
    sceRtcGetCurrentClockLocalTime(&currTime);

    oslSetFont(fontNormal);
    skinGetColor("RGBA_TOOLBAR_TEXT", tempColor);
    skinGetColor("RGBA_TOOLBAR_TEXT_SHADOW", tempColorShadow);
    oslIntraFontSetStyle(fontNormal, 0.5f, RGBA(tempColor[0], tempColor[1], tempColor[2], tempColor[3]), RGBA(tempColorShadow[0], tempColorShadow[1], tempColorShadow[2], tempColorShadow[3]), INTRAFONT_ALIGN_LEFT);
    //oslSetTextColor(RGBA(tempColor[0], tempColor[1], tempColor[2], tempColor[3]));

    sprintf(buffer, "LightMP3 v. %s by Sakya", VERSION);
    skinGetPosition("POS_APP_NAME_TEXT", tempPos);
    oslDrawString(tempPos[0], tempPos[1], buffer);

    //Battery:
    skinGetPosition("POS_BATTERY_ICON", tempPos);
    if (scePowerIsPowerOnline())
        oslDrawImageXY(commonBatteryCharging, tempPos[0], tempPos[1]);
    else if (battPerc > 10)
        oslDrawImageXY(commonBattery, tempPos[0], tempPos[1]);
    else
        oslDrawImageXY(commonBatteryLow, tempPos[0], tempPos[1]);
    sprintf(buffer, "%i%% - %2.2i:%2.2i", battPerc, btrh, btrm);
    skinGetPosition("POS_BATTERY_TEXT", tempPos);
    oslDrawString(tempPos[0], tempPos[1], buffer);

    //Volume:
	int percentage = (int)((double)imposeGetVolume() / 30.0 * 100.0);
    skinGetPosition("POS_VOLUME_ICON", tempPos);
    oslDrawImageXY(commonVolume, tempPos[0], tempPos[1]);
    sprintf(buffer, "%i%%", percentage);
    skinGetPosition("POS_VOLUME_TEXT", tempPos);
    oslDrawString(tempPos[0], tempPos[1], buffer);

    //Headphones:
    if (sceHprmIsHeadphoneExist()){
        skinGetPosition("POS_HEADPHONES_ICON", tempPos);
        oslDrawImageXY(commonHeadphone, tempPos[0], tempPos[1]);
    }

    //Clock:
    skinGetPosition("POS_CPU_ICON", tempPos);
    oslDrawImageXY(commonCPU, tempPos[0], tempPos[1]);
    sprintf(buffer, "%i / %i", scePowerGetCpuClockFrequency(), scePowerGetBusClockFrequency());
    skinGetPosition("POS_CPU_TEXT", tempPos);
    oslDrawString(tempPos[0], tempPos[1], buffer);

    skinGetPosition("POS_FREE_MEMORY_LABEL", tempPos);
    oslDrawString(tempPos[0], tempPos[1], langGetString("FREE_MEMORY"));
    sprintf(buffer, "%i kb", sceKernelTotalFreeMemSize() / 1024);
    skinGetPosition("POS_FREE_MEMORY_TEXT", tempPos);
    oslDrawString(tempPos[0], tempPos[1], buffer);

	sprintf(buffer, "%2.2d/%2.2d/%4.4d %2.2d:%2.2d",currTime.day, currTime.month, currTime.year, currTime.hour, currTime.minutes);
    skinGetPosition("POS_CLOCK_TEXT", tempPos);
    oslDrawString(tempPos[0], tempPos[1], buffer);

    return 0;
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Draw common graphics:
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
int drawCommonGraphics(){
    oslDrawImageXY(commonBkg, 0, 0);
    drawToolbars(userSettings);
    return 0;
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Draw button bar:
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
int drawButtonBar(int selectedButton){
    int i = 0;
    char langString[20] = "";
    char posName[52] = "";

    oslSetFont(fontNormal);
    for (i=0; i<5; i++){
        sprintf(langString, "BUTTON_%i", i + 1);
        sprintf(posName, "POS_BUTTON_%i", i + 1);
        skinGetPosition(posName, tempPos);
        int stringX = tempPos[0] + (commonButtons[i]->sizeX - oslGetStringWidth(langGetString(langString))) / 2.0;

        if (i == selectedButton){
            oslDrawImageXY(commonButtonsSel[i], tempPos[0], tempPos[1]);
            skinGetColor("RGBA_BUTTONBAR_SELECTED_TEXT", tempColor);
            skinGetColor("RGBA_BUTTONBAR_SELECTED_TEXT_SHADOW", tempColorShadow);
            oslIntraFontSetStyle(fontNormal, 0.5f, RGBA(tempColor[0], tempColor[1], tempColor[2], tempColor[3]), RGBA(tempColorShadow[0], tempColorShadow[1], tempColorShadow[2], tempColorShadow[3]), INTRAFONT_ALIGN_LEFT);
            //oslSetTextColor(RGBA(tempColor[0], tempColor[1], tempColor[2], tempColor[3]));
        }else{
            oslDrawImageXY(commonButtons[i], tempPos[0], tempPos[1]);
            skinGetColor("RGBA_BUTTONBAR_TEXT", tempColor);
            skinGetColor("RGBA_BUTTONBAR_TEXT_SHADOW", tempColorShadow);
            oslIntraFontSetStyle(fontNormal, 0.5f, RGBA(tempColor[0], tempColor[1], tempColor[2], tempColor[3]), RGBA(tempColorShadow[0], tempColorShadow[1], tempColorShadow[2], tempColorShadow[3]), INTRAFONT_ALIGN_LEFT);
            //oslSetTextColor(RGBA(tempColor[0], tempColor[1], tempColor[2], tempColor[3]));
        }
        oslDrawString(stringX, tempPos[1], langGetString(langString));
    }
    return 0;
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Critical error:
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void errorLoadImage(char *imageName){
    char errMsg[300];
    snprintf(errMsg, sizeof(errMsg), "Cannot load image/font: %s", imageName);
    oslMessageBox(errMsg, "Critical error",  oslMake3Buttons(OSL_KEY_CROSS, OSL_MB_QUIT , 0, 0, 0, 0));
    oslQuit();
}


///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Debug message with messageBox:
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void debugMessageBox(char *message){
    oslMessageBox(message, "Debug",  oslMake3Buttons(OSL_KEY_CROSS, OSL_MB_OK , 0, 0, 0, 0));
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Returns next and previous mode:
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
int nextAppMode(int currentMode){
    if (currentMode >= MODE_SETTINGS)
        return MODE_FILEBROWSER;
    else
        return currentMode + 1;
}

int previousAppMode(int currentMode){
    if (currentMode <= MODE_FILEBROWSER)
        return MODE_SETTINGS;
    else
        return currentMode - 1;
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Build menu (directory):
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
int buildMenuFromDirectory(struct menuElements *menu, struct opendir_struct *directory, char *selected){
    int i = 0;

    menu->first = 0;
    menu->selected = 0;
    for (i=0; i<directory->number_of_directory_entries; i++){
        if (FIO_S_ISDIR(directory->directory_entry[i].d_stat.st_mode))
            sprintf(menu->elements[i].text, "/%s", directory->directory_entry[i].d_name);
        else
            strcpy(menu->elements[i].text, directory->directory_entry[i].d_name);
        menu->elements[i].triggerFunction = NULL;
        if (!strcmp(selected, directory->directory_entry[i].d_name)){
            menu->first = i;
            menu->selected = i;
        }
    }
    menu->numberOfElements = directory->number_of_directory_entries;
    return 0;
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Draw a confirm box:
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
int drawConfirm(char *title, char *message){
    skinGetPosition("POS_POPUP", tempPos);
    int startX = tempPos[0];
    int startY = tempPos[1];

    oslSetFont(fontNormal);
    oslDrawImageXY(popupBkg, startX, startY);
    skinGetColor("RGBA_POPUP_TITLE_TEXT", tempColor);
    skinGetColor("RGBA_POPUP_TITLE_TEXT_SHADOW", tempColorShadow);
    oslIntraFontSetStyle(fontNormal, 0.5f, RGBA(tempColor[0], tempColor[1], tempColor[2], tempColor[3]), RGBA(tempColorShadow[0], tempColorShadow[1], tempColorShadow[2], tempColorShadow[3]), INTRAFONT_ALIGN_LEFT);
    //oslSetTextColor(RGBA(tempColor[0], tempColor[1], tempColor[2], tempColor[3]));
    oslDrawString((480 - oslGetStringWidth(title)) / 2, startY + 3, title);
    skinGetColor("RGBA_POPUP_MESSAGE_TEXT", tempColor);
    skinGetColor("RGBA_POPUP_MESSAGE_TEXT_SHADOW", tempColorShadow);
    oslIntraFontSetStyle(fontNormal, 0.5f, RGBA(tempColor[0], tempColor[1], tempColor[2], tempColor[3]), RGBA(tempColorShadow[0], tempColorShadow[1], tempColorShadow[2], tempColorShadow[3]), INTRAFONT_ALIGN_LEFT);
    //oslSetTextColor(RGBA(tempColor[0], tempColor[1], tempColor[2], tempColor[3]));
    oslDrawString((480 - oslGetStringWidth(message)) / 2, startY + 30, message);
    oslDrawString((480 - oslGetStringWidth(langGetString("CONFIRM_MESSAGE"))) / 2, startY + 60, langGetString("CONFIRM_MESSAGE"));
    return 0;
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Draw a wait message
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
int drawWait(char *title, char *message){
    skinGetPosition("POS_POPUP", tempPos);
    int startX = tempPos[0];
    int startY = tempPos[1];

    oslSetFont(fontNormal);
    oslDrawImageXY(popupBkg, startX, startY);
    skinGetColor("RGBA_POPUP_TITLE_TEXT", tempColor);
    skinGetColor("RGBA_POPUP_TITLE_TEXT_SHADOW", tempColorShadow);
    oslIntraFontSetStyle(fontNormal, 0.5f, RGBA(tempColor[0], tempColor[1], tempColor[2], tempColor[3]), RGBA(tempColorShadow[0], tempColorShadow[1], tempColorShadow[2], tempColorShadow[3]), INTRAFONT_ALIGN_LEFT);
    //oslSetTextColor(RGBA(tempColor[0], tempColor[1], tempColor[2], tempColor[3]));
    oslDrawString((480 - oslGetStringWidth(title)) / 2, startY + 3, title);
    skinGetColor("RGBA_POPUP_MESSAGE_TEXT", tempColor);
    skinGetColor("RGBA_POPUP_MESSAGE_TEXT_SHADOW", tempColorShadow);
    oslIntraFontSetStyle(fontNormal, 0.5f, RGBA(tempColor[0], tempColor[1], tempColor[2], tempColor[3]), RGBA(tempColorShadow[0], tempColorShadow[1], tempColorShadow[2], tempColorShadow[3]), INTRAFONT_ALIGN_LEFT);
    //oslSetTextColor(RGBA(tempColor[0], tempColor[1], tempColor[2], tempColor[3]));
    oslDrawString((480 - oslGetStringWidth(message)) / 2, startY + 30, message);
    return 0;
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Draw a message box
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
int drawMessageBox(char *title, char *message){
    skinGetPosition("POS_POPUP", tempPos);
    int startX = tempPos[0];
    int startY = tempPos[1];

    oslSetFont(fontNormal);
    oslDrawImageXY(popupBkg, startX, startY);
    skinGetColor("RGBA_POPUP_TITLE_TEXT", tempColor);
    skinGetColor("RGBA_POPUP_TITLE_TEXT_SHADOW", tempColorShadow);
    oslIntraFontSetStyle(fontNormal, 0.5f, RGBA(tempColor[0], tempColor[1], tempColor[2], tempColor[3]), RGBA(tempColorShadow[0], tempColorShadow[1], tempColorShadow[2], tempColorShadow[3]), INTRAFONT_ALIGN_LEFT);
    //oslSetTextColor(RGBA(tempColor[0], tempColor[1], tempColor[2], tempColor[3]));
    oslDrawString((480 - oslGetStringWidth(title)) / 2, startY + 3, title);
    skinGetColor("RGBA_POPUP_MESSAGE_TEXT", tempColor);
    skinGetColor("RGBA_POPUP_MESSAGE_TEXT_SHADOW", tempColorShadow);
    oslIntraFontSetStyle(fontNormal, 0.5f, RGBA(tempColor[0], tempColor[1], tempColor[2], tempColor[3]), RGBA(tempColorShadow[0], tempColorShadow[1], tempColorShadow[2], tempColorShadow[3]), INTRAFONT_ALIGN_LEFT);
    //oslSetTextColor(RGBA(tempColor[0], tempColor[1], tempColor[2], tempColor[3]));
    oslDrawString((480 - oslGetStringWidth(message)) / 2, startY + 30, message);
    oslDrawString((480 - oslGetStringWidth(langGetString("MESSAGEBOX_CONTINUE"))) / 2, startY + 60, langGetString("MESSAGEBOX_CONTINUE"));
    return 0;
}


///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Draw help screen:
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
int drawHelp(char *help){
    skinGetPosition("POS_HELP", tempPos);
    int startX = tempPos[0];
    int startY = tempPos[1];
    int i = 0;

    oslSetFont(fontNormal);
    oslDrawImageXY(helpBkg, startX, startY);
    skinGetColor("RGBA_HELP_TITLE_TEXT", tempColor);
    skinGetColor("RGBA_HELP_TITLE_TEXT_SHADOW", tempColorShadow);
    oslIntraFontSetStyle(fontNormal, 0.5f, RGBA(tempColor[0], tempColor[1], tempColor[2], tempColor[3]), RGBA(tempColorShadow[0], tempColorShadow[1], tempColorShadow[2], tempColorShadow[3]), INTRAFONT_ALIGN_LEFT);
    //oslSetTextColor(RGBA(tempColor[0], tempColor[1], tempColor[2], tempColor[3]));
    sprintf(buffer, "HELP_%s_TITLE", help);
    oslDrawString((480 - oslGetStringWidth(langGetString(buffer))) / 2, startY + 3, langGetString(buffer));
    skinGetColor("RGBA_HELP_TEXT", tempColor);
    skinGetColor("RGBA_HELP_TEXT_SHADOW", tempColorShadow);
    oslIntraFontSetStyle(fontNormal, 0.5f, RGBA(tempColor[0], tempColor[1], tempColor[2], tempColor[3]), RGBA(tempColorShadow[0], tempColorShadow[1], tempColorShadow[2], tempColorShadow[3]), INTRAFONT_ALIGN_LEFT);
    //oslSetTextColor(RGBA(tempColor[0], tempColor[1], tempColor[2], tempColor[3]));
    for (i=1; i<11; i++){
        sprintf(buffer, "HELP_%s_%2.2i", help, i);
        oslDrawString(startX + 2, startY + 3 + i * 15, langGetString(buffer));
    }
    return 0;
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Draw rating:
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
int drawRating(int startX, int startY, int rating){
    int i = 0;
    for (i=0; i<ML_MAX_RATING; i++){
        if (i<rating)
            oslDrawImageXY(star, startX + star->sizeX * i, startY);
        else
            oslDrawImageXY(blankStar, startX + star->sizeX * i, startY);
    }
    return 0;
}


///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Format seconds ina string HH:MM:SS
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
int formatHHMMSS(int seconds, char *timeString){
    int hh = seconds / 3600;
    int mm = (seconds - hh * 3600) / 60;
    int ss = seconds - hh * 3600 - mm * 60;
    sprintf(timeString, "%2.2i:%2.2i:%2.2i", hh, mm, ss);
    return 0;
}


///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Init fonts based on language encoding:
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
int initFonts(){
	//Init intraFont based on language settings:
	char encoding[10];
	strcpy(encoding, langGetString("ENCODING"));
	if (!strcmp(encoding, "UTF8"))
	    oslIntraFontInit(INTRAFONT_CACHE_LARGE | INTRAFONT_STRING_UTF8);
	else
	    oslIntraFontInit(INTRAFONT_CACHE_LARGE);
	return 0;
}
