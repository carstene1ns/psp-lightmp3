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
#include <mad.h>

extern int WMA_defaultCPUClock;

//private functions
void WMA_Init(int channel);
int WMA_Play();
void WMA_Pause();
int WMA_Stop();
void WMA_End();
void WMA_FreeTune();
int WMA_Load(char *filename);
void WMA_GetTimeString(char *dest);
int WMA_EndOfStream();
struct fileInfo *WMA_GetInfo();
struct fileInfo WMA_GetTagInfoOnly(char *filename);
int WMA_GetStatus();
float WMA_GetPercentage();
void WMA_setVolumeBoostType(char *boostType);
void WMA_setVolumeBoost(int boost);
int WMA_getVolumeBoost();
int WMA_getPlayingSpeed();
int WMA_setPlayingSpeed(int playingSpeed);
int WMA_setMute(int onOff);
void WMA_fadeOut(float seconds);

//Functions for filter (equalizer):
int WMA_setFilter(double tFilter[32], int copyFilter);
void WMA_enableFilter();
void WMA_disableFilter();
int WMA_isFilterEnabled();
int WMA_isFilterSupported();

double WMA_getFilePosition();
void WMA_setFilePosition(double position);

//Manage suspend:
int WMA_suspend();
int WMA_resume();

