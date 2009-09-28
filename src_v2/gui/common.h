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
#ifndef __common_h
#define __common_h (1)

#include "settings.h"
#include "../system/opendir.h"
#include "menu.h"

#define ANALOG_SENS 80

/*extern OSL_IMAGE *commonBkg;
extern OSL_IMAGE *commonTopToolbar;
extern OSL_IMAGE *commonBottomToolbar;
extern OSL_IMAGE *commonButtons[5];
extern OSL_IMAGE *commonButtonsSel[5];
extern OSL_IMAGE *commonBattery;
extern OSL_IMAGE *commonBatteryLow;
extern OSL_IMAGE *commonBatteryCharging;
extern OSL_IMAGE *commonVolume;
extern OSL_IMAGE *commonHeadphone;
extern OSL_IMAGE *commonCPU;
extern OSL_IMAGE *popupBkg;
extern OSL_IMAGE *star;
extern OSL_IMAGE *blankStar;
extern OSL_IMAGE *commonMenuHighlight;*/

extern OSL_IMAGE *popupBkg;
extern OSL_IMAGE *commonMenuHighlight;
extern OSL_IMAGE *folderIcon;
extern OSL_IMAGE *musicIcon;

extern OSL_FONT *fontNormal;

int loadCommonGraphics();
int unLoadCommonGraphics();
void loadFonts();
void unloadFonts();
int drawCommonGraphics();
int drawButtonBar(int selectedButton);
void errorLoadImage(char *imageName);
void debugMessageBox(char *message);
void debugFreeMemory();
unsigned int errorMessageBox(char *message, int yesNo);
int drawConfirm(char *title, char *message);
int drawWait(char *title, char *message);
int drawMessageBox(char *title, char *message);

int nextAppMode(int currentMode);
int previousAppMode(int currentMode);
int buildMenuFromDirectory(struct menuElements *menu, struct opendir_struct *directory, char *selected);
void fixMenuSize(struct menuElements *menu);
int drawHelp(char *help);
int drawRating(int startX, int startY, int rating);

int formatHHMMSS(int seconds, char *timeString, int stringLimit);
int initFonts();
void setFontStyle(OSL_FONT *f, float size, unsigned int color, unsigned int shadowColor, unsigned int options);
int getOSKlang();
int limitString(const char *string, const int width, char *target);
void initTimezone();

void setSwapButton();
int getConfirmButton();
int getCancelButton();

#endif
