/*
This source was taken (and modified) from:
PMP Mod
Copyright (C) 2006 jonny

Homepage: http://jonny.leffe.dnsalias.com
E-mail:   jonny@leffe.dnsalias.com

This program is free software; you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation; either version 2 of the License, or
(at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program; if not, write to the Free Software
Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
*/

/*
"opendir" c code (~3 lines of perl -> ~100 lines of c :s)
*/

#include <ctype.h>
#include <stdio.h>
#include <sys/stat.h>
#include "opendir.h"

static struct dirent *directory_entry;

void opendir_safe_constructor(struct opendir_struct *p){
	p->directory = NULL;
	p->directory_entry = 0;
	p->number_of_directory_entries = 0;
}


void opendir_close(struct opendir_struct *p){
	if (!(p->directory != NULL)) closedir(p->directory);
	if (p->directory_entry) free_64(p->directory_entry);

	opendir_safe_constructor(p);
}


char *opendir_open(struct opendir_struct *p, char *directory, char extFilter[][5], int extNumber, int includeDirs)
	{
	opendir_safe_constructor(p);

	p->directory = opendir(directory);
	if (p->directory == NULL){
		opendir_close(p);
		return("opendir_open: opendir failed");
	}

	unsigned int number_of_directory_entries = 0;

	while (1){
		memset(&directory_entry, 0, sizeof(struct dirent));
		directory_entry = readdir(p->directory);

		if (directory_entry == NULL)
			break;
		else
			number_of_directory_entries++;
    }

	closedir(p->directory);
	p->directory = NULL;
	p->directory = opendir(directory);
	if (p->directory == NULL){
		opendir_close(p);
		return("opendir_open: opendir failed");
	}

	p->directory_entry = malloc_64(sizeof(struct dirent) * number_of_directory_entries);
	if (p->directory_entry == 0){
		opendir_close(p);
		return("opendir_open: malloc_64 failed on directory_entry");
	}

	p->number_of_directory_entries = 0;
	int i = 0;

	for (; i < number_of_directory_entries; i++){
		memset(&directory_entry, 0, sizeof(struct dirent));
		directory_entry = readdir(p->directory);
		if (directory_entry == NULL)
			break;
		else{
			p->directory_entry[p->number_of_directory_entries] = *directory_entry;

			//Filtro le dir "." e "..":
			if (p->directory_entry[p->number_of_directory_entries].d_name[0] == '.')
				continue;

			//Controllo il filtro sulle estensioni (solo per i files):
			if (FIO_S_ISREG(p->directory_entry[p->number_of_directory_entries].d_stat.st_mode)){
				int extOK = 0;
				int i;
				char ext[5] = "";

				getExtension(p->directory_entry[p->number_of_directory_entries].d_name, ext, 4);
				extOK = 0;
				for (i = 0; i < extNumber; i++){
					if (strcmp(ext, extFilter[i]) == 0){
						extOK = 1;
						break;
					}
				}
				if (extOK == 0){
					continue;
				}
			}else if (FIO_S_ISDIR(p->directory_entry[p->number_of_directory_entries].d_stat.st_mode)){
				//Salto le directory se devo:
				if (includeDirs == 0)
					continue;
			}
			//Elemento ok:
			p->number_of_directory_entries++;
			}
		}


	closedir(p->directory);
	p->directory = NULL;

	if (p->number_of_directory_entries == 0){
		opendir_close(p);
		return("opendir_open: number_of_directory_entries == 0");
	}

	return(0);
}

//Ordinamento dei file di una directory:
void sortDirectory(struct opendir_struct *directory){
	int n = directory->number_of_directory_entries;
	int i = 0;
    char comp1[263];
    char comp2[263];

	while (i < n){
        sprintf(comp1, "%s-%s", FIO_S_ISDIR(directory->directory_entry[i-1].d_stat.st_mode)?"A":"Z", directory->directory_entry[i-1].d_name);
        sprintf(comp2, "%s-%s", FIO_S_ISDIR(directory->directory_entry[i].d_stat.st_mode)?"A":"Z", directory->directory_entry[i].d_name);

		if (i == 0 || stricmp(comp1, comp2) <= 0) i++;
		else {struct dirent tmp = directory->directory_entry[i]; directory->directory_entry[i] = directory->directory_entry[i-1]; directory->directory_entry[--i] = tmp;}
	}
}

//Extract extension from a filename:
void getExtension(char *fileName, char *extension, int extMaxLength){
    int i = 0;
    int j = 0;
    int count = 0;
    for (i = strlen(fileName) - 1; i >= 0; i--){
        if (fileName[i] == '.'){
            if (i == strlen(fileName) - 1)
                return;
            for (j = i+1; j < strlen(fileName); j++){
                extension[count++] = toupper(fileName[j]);
                if (count > extMaxLength)
                    return;
            }
            extension[count] = '\0';
            return;
        }
    }
}

//Get directory up one level:
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

//Check if a file esists (return fileSize if exists, -1 if doesen't exists):
int fileExists(char *fileName){
    struct stat stbuf;
    if(stat(fileName, &stbuf) == -1)
        return -1;
    return stbuf.st_size;
}
