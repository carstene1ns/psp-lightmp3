//    LightMP3
//    Copyright (C) 2007,2008,2009 Sakya
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
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include "bookmark.h"

int saveBookmark(char *fileName, struct bookmark *bookmark)
{
    char line[272] = "";

    FILE  *out = fopen(fileName, "w");
    if (out == NULL)
        return -1;

    snprintf(line, sizeof(line), "FILE=%s\n", bookmark->fileName);
    fwrite(line, 1, strlen(line), out);
    snprintf(line, sizeof(line), "INDEX=%i\n", bookmark->playListIndex);
    fwrite(line, 1, strlen(line), out);
    snprintf(line, sizeof(line), "POS=%.0f\n", bookmark->position);
    fwrite(line, 1, strlen(line), out);
    fclose(out);
    return 0;
}


int readBookmark(char *fileName, struct bookmark *bookmark)
{
    char line[272];

	FILE *f = fopen(fileName, "rt");
	if (f == NULL)
		return -1;
	while(fgets(line, sizeof(line), f) != NULL)
    {
        int element = 0;
        char *result = NULL;
        char name[50] = "";
        result = strtok(line, "=");
        while(result != NULL){
            if (strlen(result) > 0){
                if ((int)line[strlen(line) - 1] == 10 || (int)line[strlen(line) - 1] == 13)
                    line[strlen(line) - 1] = '\0';
                if ((int)line[strlen(line) - 1] == 10 || (int)line[strlen(line) - 1] == 13)
                    line[strlen(line) - 1] = '\0';

                if (element == 0)
                    strcpy(name, result);
                else if (element == 1){
                    if (!stricmp(name, "FILE"))
                        strcpy(bookmark->fileName, result);
                    else if (!stricmp(name, "POS"))
                        bookmark->position = atof(result);
                    else if (!stricmp(name, "INDEX"))
                        bookmark->playListIndex = atoi(result);
                }
                element++;
            }
            result = strtok(NULL, "=");
        }
    }
    fclose(f);
    return 0;
}
