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
#include <pspkernel.h>
#include <string.h>
#include <pspiofilemgr.h>

char logFile[262];
int enabled = 0;
int writing = 0;
//Open log: crea il file vuoto
int openLog(char *fileName){
	SceUID logFileO; 

	strcpy(logFile, fileName);
	logFileO = sceIoOpen(fileName, PSP_O_WRONLY | PSP_O_CREAT | PSP_O_TRUNC, 0777);
	if (!logFileO)
	{
		return(-1);
	}
	sceIoClose(logFileO);
	enabled = 1;
	return(0);
}

//Write log: scrive nel log (apre e chiude il file)
int writeLog(char *text){
	if ((strlen(logFile) > 0) & (enabled)){
		//Aspetto che il file sia libero:
		while (writing){
			sceKernelDelayThread(10000);
		}
		writing = 1;
		SceUID logFileO; 
		int byteW;

		logFileO = sceIoOpen(logFile, PSP_O_WRONLY|PSP_O_APPEND, 0777);
		byteW = sceIoWrite(logFileO, text, strlen(text));
		sceIoClose(logFileO);
		writing = 0;
		return(byteW);
	}
	return(-1);
}

//Enable log: abilita il logging
int enableLog(){
	enabled = 1;
	return(0);
}

//Disable log: disabilita il logging
int disableLog(){
	enabled = 0;
	return(0);
}
