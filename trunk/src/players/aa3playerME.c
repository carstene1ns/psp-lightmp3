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
//
//    CREDITS:
//    This file contains functions to play aa3 files through the PSP's Media Engine.
//    This code is based upon this sample code from ps2dev.org
//    http://forums.ps2dev.org/viewtopic.php?t=8469
//    and the source code of Music prx by joek2100
#include <string.h>
#include <stdio.h>
#include <malloc.h>
//#include "log.h"

#include "player.h"
#include "aa3playerME.h"
#define AT3_THREAD_PRIORITY 12
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//Globals:
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
int AA3ME_threadActive = 0;
char AA3ME_fileName[262];
static int AA3ME_isPlaying = 0;
int AA3ME_thid = -1;
int AA3ME_audio_channel = 0;
int AA3ME_eof = 0;
struct fileInfo AA3ME_info;
float AA3ME_playingTime = 0;

unsigned char AA3ME_output_buffer[2048*4]__attribute__((aligned(64)));//at3+ sample_per_frame*4
unsigned long AA3ME_codec_buffer[65]__attribute__((aligned(64)));
unsigned char AA3ME_input_buffer[2889]__attribute__((aligned(64)));//mp3 has the largest max frame, at3+ 352 is 2176
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//Private functions:
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//Decode thread:
int AA3ME_decodeThread(SceSize args, void *argp){
    u8 ea3_header[0x60];
    int temp_size;
    int mod_64;
    int res;
    unsigned long decode_type;
    int tag_size;
	int offset = 0;
	
    AA3ME_threadActive = 1;
    openAudio(AA3ME_audio_channel, AT3_OUTPUT_BUFFER_SIZE/4);
    
	sceAudiocodecReleaseEDRAM(AA3ME_codec_buffer); //Fix: ReleaseEDRAM at the end is not enough to play another file.
    OutputBuffer_flip = 0;
    AT3_OutputPtr = AT3_OutputBuffer[0];
    
    tag_size = GetID3TagSize(AA3ME_fileName);
    fd = sceIoOpen(AA3ME_fileName, PSP_O_RDONLY, 0777);
    if (fd < 0)
        AA3ME_threadActive = 0;
    
    sceIoLseek32(fd, tag_size, PSP_SEEK_SET);//not all omg files have a fixed header
    
    if ( sceIoRead( fd, ea3_header, 0x60 ) != 0x60 )
        AA3ME_threadActive = 0;
    
    if ( ea3_header[0] != 0x45 || ea3_header[1] != 0x41 || ea3_header[2] != 0x33 )
        AA3ME_threadActive = 0;
    
    at3_at3plus_flagdata[0] = ea3_header[0x22];
    at3_at3plus_flagdata[1] = ea3_header[0x23];
       
    at3_type = (ea3_header[0x22] == 0x20) ? TYPE_ATRAC3 : ((ea3_header[0x22] == 0x28) ? TYPE_ATRAC3PLUS : 0x0);
    
    if ( at3_type != TYPE_ATRAC3 && at3_type != TYPE_ATRAC3PLUS )
        AA3ME_threadActive = 0;

    if ( at3_type == TYPE_ATRAC3 )
        data_align = ea3_header[0x23]*8;
    else
        data_align = (ea3_header[0x23]+1)*8;

    data_start = tag_size+0x60;
    data_size = sceIoLseek32(fd, 0, PSP_SEEK_END) - data_start;

    sceIoLseek32(fd, data_start, PSP_SEEK_SET);

    memset(AA3ME_codec_buffer, 0, sizeof(AA3ME_codec_buffer));
   
    if ( at3_type == TYPE_ATRAC3 )
    {
        channel_mode = 0x0;

        if ( data_align == 0xC0 ) // atract3 have 3 bitrate, 132k,105k,66k, 132k align=0x180, 105k align = 0x130, 66k align = 0xc0
            channel_mode = 0x1;

        sample_per_frame = 1024;

        AA3ME_codec_buffer[26] = 0x20;
        if ( sceAudiocodecCheckNeedMem(AA3ME_codec_buffer, 0x1001) < 0 )
            AA3ME_threadActive = 0;
        if ( sceAudiocodecGetEDRAM(AA3ME_codec_buffer, 0x1001) < 0 )
            AA3ME_threadActive = 0;

        getEDRAM = 1;

        AA3ME_codec_buffer[10] = 4;
        AA3ME_codec_buffer[44] = 2;

        if ( data_align == 0x130 )
            AA3ME_codec_buffer[10] = 6;

        if ( sceAudiocodecInit(AA3ME_codec_buffer, 0x1001) < 0 )
            AA3ME_threadActive = 0;
    }
    else if ( at3_type == TYPE_ATRAC3PLUS )
    {
        sample_per_frame = 2048;
        temp_size = data_align+8;
        mod_64 = temp_size & 0x3f;
        if (mod_64 != 0) temp_size += 64 - mod_64;

        AA3ME_codec_buffer[5] = 0x1;
        AA3ME_codec_buffer[10] = at3_at3plus_flagdata[1];
        AA3ME_codec_buffer[10] = (AA3ME_codec_buffer[10] << 8 ) | at3_at3plus_flagdata[0];
        AA3ME_codec_buffer[12] = 0x1;
        AA3ME_codec_buffer[14] = 0x1;

        if ( sceAudiocodecCheckNeedMem(AA3ME_codec_buffer, 0x1000) < 0 )
            AA3ME_threadActive = 0;
        if ( sceAudiocodecGetEDRAM(AA3ME_codec_buffer, 0x1000) < 0 )
            AA3ME_threadActive = 0;

        getEDRAM = 1;

        if ( sceAudiocodecInit(AA3ME_codec_buffer, 0x1000) < 0 )
            AA3ME_threadActive = 0;
    }
    else
        AA3ME_threadActive = 0;
    samplerate = 44100;
   
	while (AA3ME_threadActive){
		while( !AA3ME_eof && AA3ME_isPlaying )
		{
			data_start = sceIoLseek32(fd, 0, PSP_SEEK_CUR);

			if (data_start < 0){
				AA3ME_isPlaying = 0;
				AA3ME_threadActive = 0;
				continue;
			}

			if ( at3_type == TYPE_ATRAC3 )
			{
				memset( AA3ME_input_buffer, 0, 0x180);

				res = sceIoRead( fd, AA3ME_input_buffer, data_align );

				if (res < 0){//error reading suspend/usb problem
					AA3ME_isPlaying = 0;
					AA3ME_threadActive = 0;
					continue;
				}else if (res != data_align){
					AA3ME_eof = 1;
					continue;
				}

				if ( channel_mode )
					memcpy(AA3ME_input_buffer+data_align, AA3ME_input_buffer, data_align);

				decode_type = 0x1001;
			}
			else
			{
				memset( AA3ME_input_buffer, 0, data_align+8);

				AA3ME_input_buffer[0] = 0x0F;
				AA3ME_input_buffer[1] = 0xD0;
				AA3ME_input_buffer[2] = at3_at3plus_flagdata[0];
				AA3ME_input_buffer[3] = at3_at3plus_flagdata[1];

				res = sceIoRead( fd, AA3ME_input_buffer+8, data_align );

				if (res < 0){//error reading suspend/usb problem
					AA3ME_isPlaying = 0;
					AA3ME_threadActive = 0;
					continue;
				}else if (res != data_align)
				{
					AA3ME_eof = 1;
					continue;
				}
				decode_type = 0x1000;
			}

			offset = data_start;

			data_size -= data_align;
			if (data_size <= 0)
			{
				AA3ME_eof = 1;
				continue;
			}

			AA3ME_codec_buffer[6] = (unsigned long)AA3ME_input_buffer;
			AA3ME_codec_buffer[8] = (unsigned long)AA3ME_output_buffer;
	   
			res = sceAudiocodecDecode(AA3ME_codec_buffer, decode_type);
			if ( res < 0 )
			{
				AA3ME_eof = 1;
				continue;
			}

            AA3ME_playingTime += (float)sample_per_frame/(float)samplerate;
		    AA3ME_info.framesDecoded++;
		    
            //Output:
			memcpy( AT3_OutputPtr, AA3ME_output_buffer, sample_per_frame*4);
			AT3_OutputPtr += (sample_per_frame * 4);
			if( AT3_OutputPtr + (sample_per_frame * 4) > &AT3_OutputBuffer[OutputBuffer_flip][AT3_OUTPUT_BUFFER_SIZE])
			{
				sceAudioOutputBlocking(AA3ME_audio_channel, PSP_AUDIO_VOLUME_MAX, AT3_OutputBuffer[OutputBuffer_flip] );

				OutputBuffer_flip ^= 1;
				AT3_OutputPtr = AT3_OutputBuffer[OutputBuffer_flip];
			}
		}
		sceKernelDelayThread(10000);
	}
    sceAudioChRelease(AA3ME_audio_channel);
    return 0;
}

//Get info on file:
int AA3MEgetInfo(){
    AA3ME_info.fileType = AT3_TYPE;
    AA3ME_info.needsME = 1;
    strcpy(AA3ME_info.layer, "");

    u8 ea3_header[0x60];
    int tag_size;
    float totalPlayingTime = 0;
    
    OutputBuffer_flip = 0;
    AT3_OutputPtr = AT3_OutputBuffer[0];

    tag_size = GetID3TagSize(AA3ME_fileName);
    fd = sceIoOpen(AA3ME_fileName, PSP_O_RDONLY, 0777);
    if (fd < 0)
        return -1;
    long fileSize = sceIoLseek(fd, 0, PSP_SEEK_END);
	sceIoLseek(fd, 0, PSP_SEEK_SET);
    sceIoLseek32(fd, tag_size, PSP_SEEK_SET);

    if ( sceIoRead( fd, ea3_header, 0x60 ) != 0x60 )
        return -1;

    if ( ea3_header[0] != 0x45 || ea3_header[1] != 0x41 || ea3_header[2] != 0x33 )
        return -1;

    at3_at3plus_flagdata[0] = ea3_header[0x22];
    at3_at3plus_flagdata[1] = ea3_header[0x23];

    at3_type = (ea3_header[0x22] == 0x20) ? TYPE_ATRAC3 : ((ea3_header[0x22] == 0x28) ? TYPE_ATRAC3PLUS : 0x0);

    if ( at3_type != TYPE_ATRAC3 && at3_type != TYPE_ATRAC3PLUS )
        return -1;

    if ( at3_type == TYPE_ATRAC3 )
        data_align = ea3_header[0x23]*8;
    else
        data_align = (ea3_header[0x23]+1)*8;

    if ( at3_type == TYPE_ATRAC3 )
        sample_per_frame = 1024;
    else if ( at3_type == TYPE_ATRAC3PLUS )
        sample_per_frame = 2048;
    else
        return -1;
    samplerate = 44100;
    
    totalPlayingTime = (float)fileSize / (float)data_align * (float)sample_per_frame/(float)samplerate;
    
	AA3ME_info.hz = samplerate;
	AA3ME_info.length = totalPlayingTime;
	long secs = AA3ME_info.length;
	int h = secs / 3600;
	int m = (secs - h * 3600) / 60;
	int s = secs - h * 3600 - m * 60;
	snprintf(AA3ME_info.strLength, sizeof(AA3ME_info.strLength), "%2.2i:%2.2i:%2.2i", h, m, s);
	strcpy(AA3ME_info.mode, "normal LR stereo");
	strcpy(AA3ME_info.emphasis,"no");
	sceIoClose(fd);
    return 0;
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//Public functions:
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void AA3ME_Init(int channel){
    AA3ME_audio_channel = channel;
    AA3ME_playingTime = 0;
	initMEAudioModules();
}


int AA3ME_Load(char *fileName){
    strcpy(AA3ME_fileName, fileName);
    if (AA3MEgetInfo() != 0){
        return ERROR_OPENING;
    }
    AA3ME_thid = -1;
    AA3ME_eof = 0;
    AA3ME_thid = sceKernelCreateThread("AA3ME_decodeThread", AA3ME_decodeThread, AT3_THREAD_PRIORITY, 0x4000, PSP_THREAD_ATTR_USER, NULL);
    if(AA3ME_thid < 0)
        return ERROR_OPENING;

    sceKernelStartThread(AA3ME_thid, 0, NULL);
    return OPENING_OK;
}


int AA3ME_Play(){
    if(AA3ME_thid < 0)
        return -1;
    AA3ME_isPlaying = 1;
    return 0;
}


void AA3ME_Pause(){
    AA3ME_isPlaying = !AA3ME_isPlaying;
}

int AA3ME_Stop(){
    AA3ME_isPlaying = 0;
    AA3ME_threadActive = 0;
    return 0;
}

int AA3ME_EndOfStream(){
    return AA3ME_eof;
}

void AA3ME_End(){
    AA3ME_Stop();
}


struct fileInfo AA3ME_GetInfo(){
    return AA3ME_info;
}


int AA3ME_GetPercentage(){
    return (int)(AA3ME_playingTime/(double)AA3ME_info.length*100.0);
}


///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//Get tag info:
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void getAA3METagInfo(char *filename, struct fileInfo *targetInfo){
    int aa3fd;
    char aa3buffer[512];
    aa3fd = sceIoOpen(filename, 0x0001, 0777);

    if (aa3fd < 0)
        return;
    sceIoRead(aa3fd,aa3buffer,3);

    if (strstr(aa3buffer,"ea3") != NULL){

    }
    sceIoClose(aa3fd);
}

struct fileInfo AA3ME_GetTagInfoOnly(char *filename){
    struct fileInfo tempInfo;
    getAA3METagInfo(filename, &tempInfo);
	return tempInfo;
}

int AA3ME_isFilterSupported(){
	return 0;
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//Manage suspend:
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
int AA3ME_suspend(){
    return 0;
}

int AA3ME_resume(){
    return 0;
}

int AA3ME_setMute(int onOff){
    return setMute(AA3ME_audio_channel, onOff);
}

void AA3ME_GetTimeString(char *dest){
    char timeString[9];
    int secs = (int)AA3ME_playingTime;
    int hh = secs / 3600;
    int mm = (secs - hh * 3600) / 60;
    int ss = secs - hh * 3600 - mm * 60;
    snprintf(timeString, sizeof(timeString), "%2.2i:%2.2i:%2.2i", hh, mm, ss);
    strcpy(dest, timeString);
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//Fade out:
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void AA3ME_fadeOut(float seconds){
    fadeOut(AA3ME_audio_channel, seconds);
}

//TODO:
int AA3_GetPercentage(){
    return 0;
}

int AA3ME_GetStatus(){
    return 0;
}

void AA3ME_setVolumeBoostType(char *boostType){
    //Only old method supported
    MAX_VOLUME_BOOST = 4;
    MIN_VOLUME_BOOST = 0;
}

void AA3ME_setVolumeBoost(int boost){

}

int AA3ME_getVolumeBoost(){
    return 0;
}

int AA3ME_getPlayingSpeed(){
    return 0;
}

int AA3ME_setPlayingSpeed(int playingSpeed){
    return 0;
}

int AA3ME_setFilter(double tFilter[32], int copyFilter){
    return 0;
}

void AA3ME_enableFilter(){
}

void AA3ME_disableFilter(){
}

int AA3ME_isFilterEnabled(){
    return 0;
}
