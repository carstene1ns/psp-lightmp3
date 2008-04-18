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
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "skinsettings.h"
#include "../system/opendir.h"

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//Globals:
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
int skinsCount = 0;


///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//Clear skins params:
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
int skinClear(){
    int i;
    for (i=0; i<skinParams.paramCount; i++){
        strcpy(skinParams.params[i].name, "");
        skinParams.params[i].value = 0;
    }
    skinParams.paramCount = 0;

    for (i=0; i<skinPositions.positionCount; i++){
        strcpy(skinPositions.positions[i].name, "");
        memset(&skinPositions.positions[i].value, 0, sizeof(skinPositions.positions[i].value));
    }
    skinPositions.positionCount = 0;

    for (i=0; i<skinColors.colorCount; i++){
        strcpy(skinParams.params[i].name, "");
        memset(&skinParams.params[i].value, 0, sizeof(skinParams.params[i].value));
    }
    skinColors.colorCount = 0;

    return 0;
}


///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//Sort skin's settings:
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
int sortParams(){
	int n = 0;
	int i = 0;

    //Params:
    i = 0;
    n = skinParams.paramCount;
	while (i < n){
		if (i == 0 || strcmp(skinParams.params[i-1].name, skinParams.params[i].name) <= 0) i++;
		else {struct intSkinParam tmp = skinParams.params[i]; skinParams.params[i] = skinParams.params[i-1]; skinParams.params[--i] = tmp;}
	}

    //Positions:
    i = 0;
    n = skinPositions.positionCount;
	while (i < n){
		if (i == 0 || strcmp(skinPositions.positions[i-1].name, skinPositions.positions[i].name) <= 0) i++;
		else {struct intSkinPosition tmp = skinPositions.positions[i]; skinPositions.positions[i] = skinPositions.positions[i-1]; skinPositions.positions[--i] = tmp;}
	}

    //Colors:
    i = 0;
    n = skinColors.colorCount;
	while (i < n){
		if (i == 0 || strcmp(skinColors.colors[i-1].name, skinColors.colors[i].name) <= 0) i++;
		else {struct intSkinColor tmp = skinColors.colors[i]; skinColors.colors[i] = skinColors.colors[i-1]; skinColors.colors[--i] = tmp;}
	}
    return 0;
}


///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//Read a RGB color from settings (rrr,ggg,bbb,aaaa)
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
int skinReadRGBA(int *RGBA, char *string){
    char *result = NULL;
    int count = 0;

    result = strtok(string, ",");
    while(result != NULL){
        RGBA[count] = atoi(result);
        if (++count > 3)
            break;
        result = strtok(NULL, ",");
    }
    return 0;
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//Read a X,Y POS from settings (X,Y)
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
int skinReadPOS(int *POS, char *string){
    char *result = NULL;
    int count = 0;

    result = strtok(string, ",");
    while(result != NULL){
        POS[count] = atoi(result);
        if (++count > 1)
            break;
        result = strtok(NULL, ",");
    }
    return 0;
}


///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//Load skin's settings:
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
int skinLoad(char *fileName){
	FILE *f;
	char lineText[256];
    char name[52];

    skinClear();
	f = fopen(fileName, "rt");
	if (f == NULL){
		//Error opening file:
		return -1;
	}

	while(fgets(lineText, 256, f) != NULL){
		int element = 0;

		if (strlen(lineText) > 0){
			if (lineText[0] != '#'){
                //Tolgo caratteri di termine riga:
                if ((int)lineText[strlen(lineText) - 1] == 10 || (int)lineText[strlen(lineText) - 1] == 13)
                    lineText[strlen(lineText) - 1] = '\0';
                if ((int)lineText[strlen(lineText) - 1] == 10 || (int)lineText[strlen(lineText) - 1] == 13)
                    lineText[strlen(lineText) - 1] = '\0';

				//Split line:
				element = 0;
				char *result = NULL;
				result = strtok(lineText, "=");
				while(result != NULL){
					if (strlen(result) > 0){
						if (element == 0)
							strcpy(name, result);
						else if (element == 1){
                            if (!strncmp(name, "RGBA_", 5)){
                                if (skinColors.colorCount < MAX_SKINCOLORS){
                                    strcpy(skinColors.colors[skinColors.colorCount].name, name);
                                    skinReadRGBA(skinColors.colors[skinColors.colorCount].value, result);
                                    skinColors.colorCount++;
                                }
                            }else if (!strncmp(name, "POS_", 4)){
                                if (skinPositions.positionCount < MAX_SKINPOSITIONS){
                                    strcpy(skinPositions.positions[skinPositions.positionCount].name, name);
                                    skinReadPOS(skinPositions.positions[skinPositions.positionCount].value, result);
                                    skinPositions.positionCount++;
                                }
                            }else{
                                if (skinParams.paramCount < MAX_SKINPARAMS){
                                    strcpy(skinParams.params[skinParams.paramCount].name, name);
                                    skinParams.params[skinParams.paramCount].value = atoi(result);
                                    skinParams.paramCount++;
                                }
                            }
                        }
						element++;
					}
					result = strtok(NULL, "=");
				}
			}
		}
	}
	fclose(f);
    sortParams();
	return 0;
}


///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//Get a skin's param:
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
int skinGetParam(char *paramName){
    int p,u,m;
    p = 0;
    u = skinParams.paramCount - 1;
    while(p <= u) {
       m = (p + u) / 2;
       if(!strcmp(skinParams.params[m].name, paramName))
           return skinParams.params[m].value;
       if(strcmp(skinParams.params[m].name, paramName) < 0)
           p = m + 1;
       else
           u = m - 1;
    }
    return -1;
}


///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//Get a skin's color:
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
int skinGetColor(char *colorName, int *color){
    int p,u,m;
    p = 0;
    memset(color, 0, sizeof(skinColors.colors[0].value));
    u = skinColors.colorCount - 1;
    while(p <= u) {
       m = (p + u) / 2;
       if(!strcmp(skinColors.colors[m].name, colorName)){
           memcpy(color, skinColors.colors[m].value, sizeof(skinColors.colors[m].value));
           return 0;
       }
       if(strcmp(skinColors.colors[m].name, colorName) < 0)
           p = m + 1;
       else
           u = m - 1;
    }
    return -1;
}


///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//Get a skin's position:
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
int skinGetPosition(char *positionName, int *position){
    int p,u,m;
    p = 0;
    memset(position, 0, sizeof(skinPositions.positions[0].value));
    u = skinPositions.positionCount - 1;
    while(p <= u) {
       m = (p + u) / 2;
       if(!strcmp(skinPositions.positions[m].name, positionName)){
           memcpy(position, skinPositions.positions[m].value, sizeof(skinPositions.positions[m].value));
           return 0;
       }
       if(strcmp(skinPositions.positions[m].name, positionName) < 0)
           p = m + 1;
       else
           u = m - 1;
    }
    return -1;
}


///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//Load available skins list:
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void skinLoadList(char *dirName){
    int i;
	struct opendir_struct dhDir;
    for (i=0; i<skinsCount; i++)
        strcpy(skinsList[i], "");
    skinsCount = 0;

    char *result = opendir_open(&dhDir, dirName, NULL, 0, 1);
	if (result == 0){
        sortDirectory(&dhDir);
        for (i = 0; i < dhDir.number_of_directory_entries; i++)
            strcpy(skinsList[i], dhDir.directory_entry[skinsCount++].d_name);
    }
}
