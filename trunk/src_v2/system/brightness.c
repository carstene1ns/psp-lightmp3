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

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Functions imported from prx:
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
int getBrightness();
void setBrightness(int brightness);


///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Public Functions:
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void fadeDisplay(int targetValue, float seconds){
    int i = 0;
    int current = getBrightness();
    
    if (targetValue < current){
    	long timeToWait = (long)((seconds * 1000000.0) / (float)(current - targetValue));
		for (i=current; i>=targetValue; i--){
			setBrightness(i);
			sceKernelDelayThread(timeToWait);
		}
	}else if (targetValue > current){
    	long timeToWait = (long)((seconds * 1000000.0) / (float)(targetValue - current));
		for (i=current; i<=targetValue; i++){
			setBrightness(i);
			sceKernelDelayThread(timeToWait);
		}
	}	
}
