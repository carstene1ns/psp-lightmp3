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
#include "opendir.h"

static SceIoDirent directory_entry;


void opendir_safe_constructor(struct opendir_struct *p)
	{
	p->directory = -1;

	p->directory_entry = 0;
	}


void opendir_close(struct opendir_struct *p)
	{
	if (!(p->directory < 0)) sceIoDclose(p->directory);

	if (p->directory_entry) free_64(p->directory_entry);


	opendir_safe_constructor(p);
	}


char *opendir_open(struct opendir_struct *p, char *directory, char extFilter[][5], int extNumber, int includeDirs)
	{
	opendir_safe_constructor(p);

	/*if (chdir(directory) < 0)
		{
		opendir_close(p);
		return("opendir_open: chdir failed");
		}*/




	p->directory = sceIoDopen(directory);
	if (p->directory < 0)
		{
		opendir_close(p);
		return("opendir_open: sceIoDopen failed");
		}




	unsigned int number_of_directory_entries = 0;


	while (1)
		{
		memset(&directory_entry, 0, sizeof(SceIoDirent));
		int result = sceIoDread(p->directory, &directory_entry);

		if (result == 0)
			{
			break;
			}
		else if (result > 0)
			{
			number_of_directory_entries++;
			}
		else
			{
			opendir_close(p);
			return("opendir_open: sceIoDread failed");
			}
		}




	sceIoDclose(p->directory);
	p->directory = -1;




	p->directory = sceIoDopen(directory);
	if (p->directory < 0)
		{
		opendir_close(p);
		return("opendir_open: sceIoDopen failed");
		}


	p->directory_entry = malloc_64(sizeof(SceIoDirent) * number_of_directory_entries);
	if (p->directory_entry == 0)
		{
		opendir_close(p);
		return("opendir_open: malloc_64 failed on directory_entry");
		}




	p->number_of_directory_entries = 0;


	int i = 0;

	for (; i < number_of_directory_entries; i++)
		{
		memset(&directory_entry, 0, sizeof(SceIoDirent));
		int result = sceIoDread(p->directory, &directory_entry);

		if (result == 0)
			{
			break;
			}
		else if (result > 0)
			{
			p->directory_entry[p->number_of_directory_entries] = directory_entry;

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
		else
			{
			opendir_close(p);
			return("opendir_open: sceIoDread failed");
			}
		}


	sceIoDclose(p->directory);
	p->directory = -1;




	if (p->number_of_directory_entries == 0)
		{
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

		if (i == 0 || strcmp(comp1, comp2) <= 0) i++;
		else {SceIoDirent tmp = directory->directory_entry[i]; directory->directory_entry[i] = directory->directory_entry[i-1]; directory->directory_entry[--i] = tmp;}
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
