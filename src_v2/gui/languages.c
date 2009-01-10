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

#include "languages.h"
#include "../system/opendir.h"

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//Globals:
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
int languagesCount = 0;
static struct strLangStrings langStrings;

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//Reverse a string:
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
char *strrev(char *str){
      char *p1, *p2;

    if (! str || ! *str)
        return str;
    for (p1 = str, p2 = str + strlen(str) - 1; p2 > p1; ++p1, --p2){
        *p1 ^= *p2;
        *p2 ^= *p1;
        *p1 ^= *p2;
    }
    return str;
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//Clear language strings:
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
int langClear(){
    int i;
    for (i=0; i<langStrings.stringsCount; i++){
        strcpy(langStrings.strings[i].name, "");
        strcpy(langStrings.strings[i].value, "");
    }
    langStrings.stringsCount = 0;
    return 0;
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//Load language strings:
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
int langLoad(char *fileName){
	FILE *f;
	char lineText[256];
    int rightToLeft = 0;

    langClear();
	f = fopen(fileName, "rt");
	if (f == NULL){
		//Error opening file:
		return -1;
	}

	while(fgets(lineText, 256, f) != NULL){
		int element = 0;
		struct strLangString string;

		if (strlen(lineText) > 0){
			if (lineText[0] != '#'){
                //Tolgo caratteri di termine riga:
                if ((int)lineText[strlen(lineText) - 1] == 10 || (int)lineText[strlen(lineText) - 1] == 13)
                    lineText[strlen(lineText) - 1] = '\0';
                if ((int)lineText[strlen(lineText) - 1] == 10 || (int)lineText[strlen(lineText) - 1] == 13)
                    lineText[strlen(lineText) - 1] = '\0';

                //Check options:
                if (!strcmp(lineText, "@RIGHTTOLEFT") || !strcmp(lineText, "TFELOTTHGIR@"))
                    rightToLeft = 1;

				//Split line:
				element = 0;
				char *result = NULL;
				result = strtok(lineText, "=");
				while(result != NULL){
					if (strlen(result) > 0){
						if (element == 0)
							strcpy(string.name, result);
						else if (element == 1){
    						if (rightToLeft)
        						strcpy(string.value, strrev(result));
    						else
        						strcpy(string.value, result);
    						langStrings.strings[langStrings.stringsCount++] = string;
                        }
						element++;
					}
					result = strtok(NULL, "=");
				}
			}
		}
	}
	fclose(f);

    int i = 0;
    int n = langStrings.stringsCount;
	while (i < n){
		if (i == 0 || strcmp(langStrings.strings[i-1].name, langStrings.strings[i].name) <= 0) i++;
		else {struct strLangString tmp = langStrings.strings[i]; langStrings.strings[i] = langStrings.strings[i-1]; langStrings.strings[--i] = tmp;}
	}
	return 0;
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//Get a language string:
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
char *langGetString(char *stringName){
    int p,u,m;
    p = 0;
    u = langStrings.stringsCount - 1;
    while(p <= u) {
       m = (p + u) / 2;
       if(!strcmp(langStrings.strings[m].name, stringName))
           return langStrings.strings[m].value;
       if(strcmp(langStrings.strings[m].name, stringName) < 0)
           p = m + 1;
       else
           u = m - 1;
    }
    return "";
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//Load available languages list:
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void langLoadList(char *dirName){
    int i;
	struct opendir_struct dhDir;
    for (i=0; i<languagesCount; i++)
        strcpy(languagesList[i], "");
    languagesCount = 0;

    char *result = opendir_open(&dhDir, dirName, dirName, NULL, 0, 1);
	if (result == 0){
        for (i = 0; i < dhDir.number_of_directory_entries; i++)
        {
            if (dhDir.directory_entry[i].d_name[0] != '.')
                strcpy(languagesList[languagesCount++], dhDir.directory_entry[i].d_name);   
        }
    }
}

