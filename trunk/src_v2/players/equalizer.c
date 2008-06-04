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
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "equalizer.h"

struct equalizersList list;

//Equalizers value:
double EQ_None[32] = {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0};
int EQ_NUMBER = 1;

//Read user equalizers from file:
int initEqualizers(int initFromEQ){
	FILE *f;
	char lineText[256];
    struct equalizer tEQ;

	f = fopen("equalizers", "rt");
	if (f == NULL){
		//Error opening file:
		return(0);
	}

	while(fgets(lineText, 256, f) != NULL){
		int element = 0;
		if (strlen(lineText) > 0){
			if (lineText[0] != '#'){
				//Split line:
				element = 0;
				char *result = NULL;
				result = strtok(lineText, ";");
				while(result != NULL){
					if (strlen(result) > 0 && element < 34){
						if (element == 0){
							strncpy(tEQ.name, result, 31);
							tEQ.name[31] = '\0';
						}else if(element == 1){
							strncpy(tEQ.shortName, result, 3);
							tEQ.shortName[3] = '\0';
						}else{
							tEQ.filter[element - 2] = atof(result);
						}
						element++;
					}
					result = strtok(NULL, ";");
				}
				//If all data OK then add the EQ to the list:
				if (element == 34){
                    list.EQ[initFromEQ].index = EQ_NUMBER;
					strcpy(list.EQ[initFromEQ].name, tEQ.name);
                    strcpy(list.EQ[initFromEQ].shortName, tEQ.shortName);
                    memcpy(list.EQ[initFromEQ].filter, tEQ.filter, sizeof(list.EQ[initFromEQ].filter));
                    initFromEQ++;
					EQ_NUMBER++;
				}
			}
		}
	}
	fclose(f);
	return(1);
}


//Init:
void EQ_init(){
	struct equalizer tEQ;

	//None:
	strcpy(tEQ.name, "None");
    tEQ.index = 0;
	strcpy(tEQ.shortName, "NO");
	memcpy(tEQ.filter, EQ_None, sizeof(EQ_None));
	list.EQ[0] = tEQ;

	//Read equalizers from file:
	initEqualizers(1);
}


//Get EQ by name:
struct equalizer EQ_get(char *name){
	int i;
	for (i = 0; i < EQ_NUMBER; i++){
		if (strcmp(list.EQ[i].name, name) == 0){
			return list.EQ[i];
		}
	}
	struct equalizer voidEQ;
	return(voidEQ);
}


//Get EQ by short name:
struct equalizer EQ_getShort(char *shortName){
	int i;
	for (i = 0; i < EQ_NUMBER; i++){
		if (strcmp(list.EQ[i].shortName, shortName) == 0){
			return list.EQ[i];
		}
	}
	struct equalizer voidEQ;
	return(voidEQ);
}


//Get EQ by index:
struct equalizer EQ_getIndex(int index){
	if (index < EQ_NUMBER){
		return list.EQ[index];
	}
	struct equalizer voidEQ;
	return(voidEQ);
}

//Get EQ number:
int EQ_getEqualizersNumber(){
	return(EQ_NUMBER);
}

