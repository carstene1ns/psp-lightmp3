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
#include "gui_settings.h"
#include "common.h"
#include "languages.h"
#include "settings.h"
#include "skinsettings.h"
#include "menu.h"
#include "system/clock.h"
#include "players/player.h"
#include "../others/audioscrobbler.h"

#define STATUS_CONFIRM_NONE 0
#define STATUS_CONFIRM_SAVE 1
#define STATUS_HELP 2

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Globals:
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
static int settingsRetValue = 0;
static int exitFlagSettings = 0;
char buffer[264] = "";

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Get a settings value from menu index:
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
int getSettingVal(int index, char *value, int valueLimit){
    char yesNo[2][20];
    char bCheck[3][50];

    strcpy(yesNo[0], langGetString("NO"));
    strcpy(yesNo[1], langGetString("YES"));

    strcpy(bCheck[0], langGetString("BRIGHTNESS_NO"));
    strcpy(bCheck[1], langGetString("BRIGHTNESS_YES_WARNING"));
    strcpy(bCheck[2], langGetString("BRIGHTNESS_YES"));

    switch(index){
        case 0:
            snprintf(value, valueLimit, "%s", userSettings->skinName);
            break;
        case 1:
            snprintf(value, valueLimit, "%s", userSettings->lang);
            break;
        case 2:
            snprintf(value, valueLimit, "%s", yesNo[userSettings->SCROBBLER]);
            break;
        case 3:
            snprintf(value, valueLimit, "%s", yesNo[userSettings->FADE_OUT]);
            break;
        case 4:
            snprintf(value, valueLimit, "%i", userSettings->MUTED_VOLUME);
            break;
        case 5:
            snprintf(value, valueLimit, "%s", bCheck[userSettings->BRIGHTNESS_CHECK]);
            break;
        case 6:
            snprintf(value, valueLimit, "%s", yesNo[userSettings->MP3_ME]);
            break;
        case 7:
            snprintf(buffer, sizeof(buffer), "BOOST_%s", userSettings->BOOST);
            snprintf(value, valueLimit, "%s", langGetString(buffer));
            break;
        case 8:
            snprintf(value, valueLimit, "%s", yesNo[userSettings->CLOCK_AUTO]);
            break;
        case 9:
            snprintf(value, valueLimit, "%i", userSettings->CLOCK_GUI);
            break;
        case 10:
            snprintf(value, valueLimit, "%i", userSettings->CLOCK_MP3);
            break;
        case 11:
            snprintf(value, valueLimit, "%i", userSettings->CLOCK_MP3ME);
            break;
        case 12:
            snprintf(value, valueLimit, "%i", userSettings->CLOCK_OGG);
            break;
        case 13:
            snprintf(value, valueLimit, "%i", userSettings->CLOCK_FLAC);
            break;
        case 14:
            snprintf(value, valueLimit, "%i", userSettings->CLOCK_AA3);
            break;
        case 15:
            snprintf(value, valueLimit, "%i", userSettings->CLOCK_WMA);
            break;
        case 16:
            snprintf(value, valueLimit, "%i", userSettings->KEY_AUTOREPEAT_GUI);
            break;
        case 17:
            snprintf(value, valueLimit, "%s", yesNo[userSettings->SHOW_SPLASH]);
            break;
		case 18:
            snprintf(value, valueLimit, "%s", yesNo[userSettings->HOLD_DISPLAYOFF]);
            break;
	}

    return 0;
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Init settings menu:
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
int initSettingsMenu(){
    //Build menu:
    commonMenu.first = 0;
    commonMenu.selected = 0;
    skinGetPosition("POS_SETTINGS", tempPos);
    commonMenu.yPos = tempPos[1];
    commonMenu.xPos = tempPos[0];
    commonMenu.fastScrolling = 0;
    commonMenu.align = ALIGN_LEFT;
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

    //Values menu:
    commonSubMenu.yPos = tempPos[1];
    commonSubMenu.xPos = tempPos[0] + 350;
    commonSubMenu.align = ALIGN_RIGHT;
    commonSubMenu.fastScrolling = 0;
    commonSubMenu.background = NULL;
    commonSubMenu.highlight = NULL;
    commonSubMenu.width = commonMenu.width - 350;
    commonSubMenu.height = commonMenu.height;
    commonSubMenu.interline = commonMenu.interline;
    commonSubMenu.maxNumberVisible = commonMenu.maxNumberVisible;
    commonSubMenu.cancelFunction = NULL;
    return 0;
}


///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Build settings menu:
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
int buildSettingsMenu(struct menuElements *menu, struct menuElements *values){
    int i = 0;
    char name[100] = "";
    char settingVal[50] = "";
    struct menuElement tMenuEl;

    menu->numberOfElements = 19;

    values->first = menu->first;
    values->selected = menu->selected;
    values->numberOfElements = menu->numberOfElements;
    for (i=0; i<menu->numberOfElements; i++){
        snprintf(name, sizeof(name), "SETTINGS_%2.2i", i + 1);
        strcpy(tMenuEl.text, langGetString(name));
        tMenuEl.icon = NULL;
        tMenuEl.triggerFunction = NULL;
        menu->elements[i] = tMenuEl;
        getSettingVal(i, settingVal, sizeof(settingVal));
        strcpy(tMenuEl.text, settingVal);
        tMenuEl.triggerFunction = NULL;
        values->elements[i] = tMenuEl;
    }
    return 0;
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Change a setting value from menu index:
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
int changeSettingVal(int index, int delta){
    int current = 0;
    int i = 0;

    switch(index){
        case 0:
            for (i=0; i<skinsCount; i++){
                if (!strcmp(skinsList[i], userSettings->skinName)){
                    current = i;
                    break;
                }
            }
            if (current + delta < skinsCount && current + delta >= 0){
				cpuBoost();
				oslStartDrawing();
				drawCommonGraphics();
				drawButtonBar(MODE_SETTINGS);
				drawMenu(&commonMenu);
				drawMenu(&commonSubMenu);
                drawWait(langGetString("LOADING_SKIN_TITLE"), langGetString("LOADING_SKIN"));
				oslEndDrawing();
				oslEndFrame();
				oslSyncFrame();

				current += delta;
                strcpy(userSettings->skinName, skinsList[current]);
                snprintf(buffer, sizeof(buffer), "%sskins/%s/skin.cfg", userSettings->ebootPath, userSettings->skinName);
                skinLoad(buffer);

                //debugMessageBox("skin's settings loaded");
   				//Skin's s images path:
				if ( skinGetString("STR_IMAGE_PATH", buffer) == 0 ) {
    				snprintf(userSettings->skinImagesPath, sizeof(userSettings->skinImagesPath), "%sskins/%s/images", userSettings->ebootPath, buffer);
				}
				else {
    				snprintf(userSettings->skinImagesPath, sizeof(userSettings->skinImagesPath), "%sskins/%s/images", userSettings->ebootPath, userSettings->skinName);
				}

				//Default text size:
				defaultTextSize = skinGetParam("FONT_NORMAL_SIZE") / 100.0;

				clearMenu(&commonMenu);
			    clearMenu(&commonSubMenu);
                //debugMessageBox("cleared menues");
                unLoadCommonGraphics();
                //debugMessageBox("unloaded common");
                loadCommonGraphics();
                //debugMessageBox("loaded common");
				endMenu();
				initMenu();
                initSettingsMenu();
				oslSetFont(fontNormal);
				cpuRestore();
            }
            break;
        case 1:
            for (i=0; i<languagesCount; i++){
                if (!strcmp(languagesList[i], userSettings->lang)){
                    current = i;
                    break;
                }
            }

            if (current + delta < languagesCount && current + delta >= 0){
                cpuBoost();

                oslStartDrawing();
                drawCommonGraphics();
                drawButtonBar(MODE_SETTINGS);
                drawMenu(&commonMenu);
                drawMenu(&commonSubMenu);
                drawWait(langGetString("LOADING_LANGUAGE_TITLE"), langGetString("LOADING_LANGUAGE"));
                oslEndDrawing();
                oslEndFrame();
                oslSyncFrame();

                current += delta;
                strcpy(userSettings->lang, languagesList[current]);

                snprintf(buffer, sizeof(buffer), "%slanguages/%s/lang.txt", userSettings->ebootPath, userSettings->lang);
                if (langLoad(buffer)){
                    char message[512] = "";
                    snprintf(message, sizeof(message), "Error loading language file:\n%s", buffer);
                    debugMessageBox(message);
                    oslQuit ();
                    return 0;
                }
				clearMenu(&commonMenu);
			    clearMenu(&commonSubMenu);
				endMenu();

                unloadFonts();
                oslIntraFontShutdown();
                initFonts();
                loadFonts();

				initMenu();
                initSettingsMenu();
				oslSetFont(fontNormal);
                buildSettingsMenu(&commonMenu, &commonSubMenu);
                commonMenu.selected = 1;

                cpuRestore();
            }
            break;
        case 2:
            userSettings->SCROBBLER = !userSettings->SCROBBLER;
            //AudioScrobbler:
            if (userSettings->SCROBBLER){
                snprintf(buffer, sizeof(buffer), "%s%s", userSettings->ebootPath, ".scrobbler.log");
                SCROBBLER_init(buffer);
            }
            break;
        case 3:
            userSettings->FADE_OUT = !userSettings->FADE_OUT;
            break;
        case 4:
            if (userSettings->MUTED_VOLUME + delta < 8000 && userSettings->MUTED_VOLUME + delta >= 0)
                userSettings->MUTED_VOLUME += delta;
            break;
        case 5:
            if (++userSettings->BRIGHTNESS_CHECK > 2)
                userSettings->BRIGHTNESS_CHECK = 0;
            break;
        case 6:
            userSettings->MP3_ME = !userSettings->MP3_ME;
            break;
        case 7:
            if (!strcmp(userSettings->BOOST, "NEW"))
                strcpy(userSettings->BOOST, "OLD");
            else
                strcpy(userSettings->BOOST, "NEW");
            break;
        case 8:
            userSettings->CLOCK_AUTO = !userSettings->CLOCK_AUTO;
            break;
        case 9:
            if (userSettings->CLOCK_GUI + delta <= 222 && userSettings->CLOCK_GUI + delta >= getMinCPUClock())
                userSettings->CLOCK_GUI += delta;
                if (userSettings->CLOCK_AUTO)
                    setCpuClock(userSettings->CLOCK_GUI);
            break;
        case 10:
            if (userSettings->CLOCK_MP3 + delta <= 222 && userSettings->CLOCK_MP3 + delta >= getMinCPUClock())
                userSettings->CLOCK_MP3 += delta;
                MP3_defaultCPUClock = userSettings->CLOCK_MP3;
            break;
        case 11:
            if (userSettings->CLOCK_MP3ME + delta <= 222 && userSettings->CLOCK_MP3ME + delta >= getMinCPUClock())
                userSettings->CLOCK_MP3ME += delta;
                MP3ME_defaultCPUClock = userSettings->CLOCK_MP3ME;
            break;
        case 12:
            if (userSettings->CLOCK_OGG + delta <= 222 && userSettings->CLOCK_OGG + delta >= getMinCPUClock())
                userSettings->CLOCK_OGG += delta;
                OGG_defaultCPUClock = userSettings->CLOCK_OGG;
            break;
        case 13:
            if (userSettings->CLOCK_FLAC + delta <= 222 && userSettings->CLOCK_FLAC + delta >= getMinCPUClock())
                userSettings->CLOCK_FLAC += delta;
                FLAC_defaultCPUClock = userSettings->CLOCK_FLAC;
            break;
        case 14:
            if (userSettings->CLOCK_AA3 + delta <= 222 && userSettings->CLOCK_AA3 + delta >= getMinCPUClock())
                userSettings->CLOCK_AA3 += delta;
                AA3ME_defaultCPUClock = userSettings->CLOCK_AA3;
            break;
        case 15:
            if (userSettings->CLOCK_WMA + delta <= 222 && userSettings->CLOCK_WMA + delta >= getMinCPUClock())
                userSettings->CLOCK_WMA += delta;
                WMA_defaultCPUClock = userSettings->CLOCK_WMA;
            break;
        case 16:
            if (userSettings->KEY_AUTOREPEAT_GUI + delta <= 30 && userSettings->KEY_AUTOREPEAT_GUI + delta >= 1){
                userSettings->KEY_AUTOREPEAT_GUI += delta;
                oslSetKeyAutorepeatInterval(userSettings->KEY_AUTOREPEAT_GUI);
            }
            break;
        case 17:
            userSettings->SHOW_SPLASH = !userSettings->SHOW_SPLASH;
            break;
		case 18:
            userSettings->HOLD_DISPLAYOFF = !userSettings->HOLD_DISPLAYOFF;
            break;
    }
    return 0;
}


///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Settings:
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
int gui_settings(){
    int confirmStatus = STATUS_CONFIRM_NONE;
    initSettingsMenu();
    buildSettingsMenu(&commonMenu, &commonSubMenu);

    exitFlagSettings = 0;
	int skip = 0;
	cpuRestore();
    while(!osl_quit && !exitFlagSettings){
		if (!skip){
			oslStartDrawing();
			drawCommonGraphics();
			drawButtonBar(MODE_SETTINGS);
			drawMenu(&commonMenu);
			drawMenu(&commonSubMenu);

			switch (confirmStatus){
				case STATUS_CONFIRM_SAVE:
					drawConfirm(langGetString("CONFIRM_SAVE_SETTINGS_TITLE"), langGetString("CONFIRM_SAVE_SETTINGS"));
					break;
				case STATUS_HELP:
					drawHelp("SETTINGS");
					break;
			}
	    	oslEndDrawing();
		}
        oslEndFrame();
    	skip = oslSyncFrame();

        oslReadKeys();
        if (confirmStatus == STATUS_CONFIRM_SAVE){
            if(getConfirmButton()){
                SETTINGS_save(userSettings);
                confirmStatus = STATUS_CONFIRM_NONE;
            }else if(osl_pad.pressed.circle){
                confirmStatus = STATUS_CONFIRM_NONE;
            }
        }else if (confirmStatus == STATUS_HELP){
            if (getConfirmButton() || getCancelButton())
                confirmStatus = STATUS_CONFIRM_NONE;
        }else{
            processMenuKeys(&commonMenu);
            commonSubMenu.selected = commonMenu.selected;
            commonSubMenu.first = commonMenu.first;

            if (osl_pad.held.L && osl_pad.held.R){
                confirmStatus = STATUS_HELP;
            }else if (osl_pad.pressed.right){
                changeSettingVal(commonMenu.selected, +1);
                buildSettingsMenu(&commonMenu, &commonSubMenu);
            }else if (osl_pad.pressed.left){
                changeSettingVal(commonMenu.selected, -1);
                buildSettingsMenu(&commonMenu, &commonSubMenu);
            }else if(osl_pad.released.start){
                confirmStatus = STATUS_CONFIRM_SAVE;
            }else if(osl_pad.released.R){
                settingsRetValue = nextAppMode(MODE_SETTINGS);
                exitFlagSettings = 1;
            }else if(osl_pad.released.L){
                settingsRetValue = previousAppMode(MODE_SETTINGS);
                exitFlagSettings = 1;
            }
        }
    }
    //save settings:
    SETTINGS_save(userSettings);

    //unLoad images:
    clearMenu(&commonMenu);
    clearMenu(&commonSubMenu);

    return settingsRetValue;
}
