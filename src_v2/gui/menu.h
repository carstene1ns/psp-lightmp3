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
#ifndef __menu_h
#define __menu_h (1)

#include "settings.h"
#define MENU_MAX_ELEMENTS 5000

#define ALIGN_LEFT 0
#define ALIGN_RIGHT 1
#define ALIGN_CENTER 2

extern OSL_FONT *fontMenuNormal;

struct menuElement{
    OSL_IMAGE *icon;
    char text[264];
    int value;
    char data[1024];
    int (*triggerFunction)();
    float xPos; //For scrolling
};

struct menuElements{
    int selected;
    int first;
    int maxNumberVisible;
    int yPos;
    int xPos;
    int width;
    int height;
    OSL_IMAGE *background;
    OSL_IMAGE *highlight;
    int numberOfElements;
    int interline;
    int fastScrolling;
    int align;
    struct menuElement *elements[MENU_MAX_ELEMENTS];
    int (*cancelFunction)();
    void (*dataFeedFunction)(int index, struct menuElement *element);
};


int initMenu();
int clearMenu(struct menuElements *menu);
void clearMenuElement(struct menuElement *element);
int drawMenu(struct menuElements *menu);
int processMenuKeys(struct menuElements *menu);
int endMenu();

#endif
