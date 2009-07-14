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
//    This code is based upon the sample code by hrimfaxi and cooleyes from ps2dev.org
//    http://forums.ps2dev.org/viewtopic.php?t=11986
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <malloc.h>
#include <psputility_avmodules.h>

#include "id3.h"
#include "player.h"
#include "wmaplayerME.h"
#include "cooleyesBridge.h"
#include "libasfparser/pspasfparser.h"
#include "system/mem64.h"

#define THREAD_PRIORITY 12
#define WMA_OUTPUT_BUFFER_SIZE	2048

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//Globals:
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
static int WMA_threadActive = 0;
static char WMA_fileName[264];
static int WMA_isPlaying = 0;
static int WMA_thid = -1;
static int WMA_audio_channel = 0;
static int WMA_eof = 0;
static struct fileInfo WMA_info;
static int WMA_playingSpeed = 0; // 0 = normal
static unsigned int WMA_volume_boost = 0;
static float WMA_playingTime = 0;
static int WMA_volume = 0;
int WMA_defaultCPUClock = 20;
static double WMA_filesize = 0;
static double WMA_filePos = 0;
static double WMA_newFilePos = 0;
static int WMA_tagRead = 0;
static double WMA_tagsize = 0;

//Globals for decoding:
static SceUID WMA_handle = -1;

unsigned long WMA_codec_buffer[65] __attribute__((aligned(64)));
static short WMA_mix_buffer[8192] __attribute__((aligned(64)));
static short WMA_cache_buffer[16384] __attribute__((aligned(64)));
unsigned long WMA_cache_samples = 0;

static short WMA_output_buffer[2][WMA_OUTPUT_BUFFER_SIZE * 2] __attribute__((aligned(64)));
int WMA_output_index = 0;

void* WMA_frame_buffer;

SceAsfParser* parser = 0;
ScePVoid need_mem_buffer = 0;


u8 WMA_getEDRAM = 0;

u16 WMA_channels  = 2;
u32 WMA_samplerate = 0xAC44;
u16 WMA_format_tag = 0x0161;
u32 WMA_avg_bytes_per_sec = 3998;
u16 WMA_block_align = 1485;
u16 WMA_bits_per_sample = 16;
u16 WMA_flag = 0x1F;


static long WMA_suspendPosition = -1;
static long WMA_suspendIsPlaying = 0;

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//Private functions:
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
int asf_read_cb(void *userdata, void *buf, SceSize size) {

	int ret = sceIoRead(WMA_handle, buf, size);
	WMA_filePos += ret;
	return ret;
}

SceOff asf_seek_cb(void *userdata, void *buf, SceOff offset, int whence) {
	SceOff ret = -1;
	ret = sceIoLseek(WMA_handle, offset, whence);
	WMA_filePos = ret;
	return ret;
}

//Decode thread:
int WMA_decodeThread(SceSize args, void *argp){
    int ret;

	sceAudiocodecReleaseEDRAM(WMA_codec_buffer); //Fix: ReleaseEDRAM at the end is not enough to play another file.
	WMA_threadActive = 1;

    WMA_handle = sceIoOpen(WMA_fileName, PSP_O_RDONLY, 0777);
    if (WMA_handle < 0)
        WMA_threadActive = 0;
	WMA_filesize = sceIoLseek(WMA_handle, 0, PSP_SEEK_END);
    sceIoLseek(WMA_handle, 0, PSP_SEEK_SET);

	parser = malloc_64(sizeof(SceAsfParser));
	if ( !parser ) {
		WMA_threadActive = 0;
	}
	memset(parser, 0, sizeof(SceAsfParser));

	parser->iUnk0 = 0x01;
	parser->iUnk1 = 0x00;
	parser->iUnk2 = 0x000CC003;
	parser->iUnk3 = 0x00;
	parser->iUnk4 = 0x00000000;
	parser->iUnk5 = 0x00000000;
	parser->iUnk6 = 0x00000000;
	parser->iUnk7 = 0x00000000;

	ret = sceAsfCheckNeedMem(parser);
	if ( ret < 0 ) {
		WMA_threadActive = 0;
	}

	if ( parser->iNeedMem > 0x8000 ) {
	    WMA_threadActive = 0;
	}

	//sceAsfInitParser
	need_mem_buffer = malloc_64(parser->iNeedMem);
	if ( !need_mem_buffer ) {
		WMA_threadActive = 0;
	}

	parser->pNeedMemBuffer = need_mem_buffer;
	parser->iUnk3356 = 0x00000000;

	ret = sceAsfInitParser(parser, 0, &asf_read_cb, &asf_seek_cb);
	if ( ret < 0 ) {
		WMA_threadActive = 0;
	}
	WMA_channels = *((u16*)need_mem_buffer);
	WMA_samplerate = *((u32*)(need_mem_buffer+4));
	WMA_format_tag = 0x0161;
	WMA_avg_bytes_per_sec = *((u32*)(need_mem_buffer+8));
	WMA_block_align = *((u16*)(need_mem_buffer+12));
	WMA_bits_per_sample = 16;
	WMA_flag = *((u16*)(need_mem_buffer+14));

	u64 duration = parser->lDuration/100000;
	int hour, minute, second, msecond;
	msecond = duration %100;
	duration /= 100;
	second =  duration % 60;
	duration /= 60;
	minute = duration % 60;
	hour = duration / 60;

	memset(WMA_codec_buffer, 0, sizeof(WMA_codec_buffer));

	//CheckNeedMem
	WMA_codec_buffer[5] = 1;
	ret = sceAudiocodecCheckNeedMem(WMA_codec_buffer, 0x1005);
	if ( ret < 0 ) {
		WMA_threadActive = 0;
	}

	//GetEDRAM
	ret=sceAudiocodecGetEDRAM(WMA_codec_buffer, 0x1005);
	if ( ret < 0 ) {
		WMA_threadActive = 0;
	}

	WMA_getEDRAM = 1;

	//Init
	u16* p16 = (u16*)(&WMA_codec_buffer[10]);
	p16[0] = WMA_format_tag;
	p16[1] = WMA_channels;
	WMA_codec_buffer[11] = WMA_samplerate;
	WMA_codec_buffer[12] = WMA_avg_bytes_per_sec;
	p16 = (u16*)(&WMA_codec_buffer[13]);
	p16[0] = WMA_block_align;
	p16[1] = WMA_bits_per_sample;
	p16[2] = WMA_flag;

	ret=sceAudiocodecInit(WMA_codec_buffer, 0x1005);
	if ( ret < 0 ) {
		WMA_threadActive = 0;
	}

	WMA_frame_buffer = malloc_64(WMA_block_align);
	if (!WMA_frame_buffer) {
		WMA_threadActive = 0;
	}

	int npt = 0 * 1000;

	ret = sceAsfSeekTime(parser, 1, &npt);
	if (ret < 0) {
		WMA_threadActive = 0;
	}

	memset(WMA_cache_buffer, 0, 32768);
	WMA_cache_samples = 0;

    WMA_eof = 0;
	WMA_info.framesDecoded = 0;

	while (WMA_threadActive){
		while( !WMA_eof && WMA_isPlaying )
		{
			if ( WMA_cache_samples >= WMA_OUTPUT_BUFFER_SIZE ) {
				memcpy(WMA_output_buffer[WMA_output_index], WMA_cache_buffer, 8192);
				WMA_cache_samples -= WMA_OUTPUT_BUFFER_SIZE;
				if ( WMA_cache_samples > 0 )
					memmove(WMA_cache_buffer, WMA_cache_buffer+4096, WMA_cache_samples*2*2);

				//Volume Boost:
				if (WMA_volume_boost){
                    int i;
                    for (i=0; i<WMA_cache_samples*2; i++){
    					WMA_output_buffer[WMA_output_index][i] = volume_boost(&WMA_output_buffer[WMA_output_index][i], &WMA_volume_boost);
                    }
                }
                audioOutput(WMA_volume, WMA_output_buffer[WMA_output_index]);
				WMA_output_index = (WMA_output_index + 1) % 2;
				continue;
			}

			memset(WMA_frame_buffer, 0, WMA_block_align);
			parser->sFrame.pData = WMA_frame_buffer;
			ret = sceAsfGetFrameData(parser, 1, &parser->sFrame);

			if (ret < 0) {
				WMA_eof = 1;
				break;
			}
			memset(WMA_mix_buffer, 0, 16384);

			WMA_codec_buffer[6] = (unsigned long)WMA_frame_buffer;
			WMA_codec_buffer[8] = (unsigned long)WMA_mix_buffer;
			WMA_codec_buffer[9] = 16384;;

			WMA_codec_buffer[15] = parser->sFrame.iUnk2;
			WMA_codec_buffer[16] = parser->sFrame.iUnk3;
			WMA_codec_buffer[17] = parser->sFrame.iUnk5;
			WMA_codec_buffer[18] = parser->sFrame.iUnk6;
			WMA_codec_buffer[19] = parser->sFrame.iUnk4;

			ret = sceAudiocodecDecode(WMA_codec_buffer, 0x1005);
			if (ret < 0) {
				WMA_eof = 1;
				break;
			}
            WMA_playingTime += (float)(WMA_codec_buffer[9]/4)/(float)WMA_samplerate;
		    WMA_info.framesDecoded++;\

			memcpy(WMA_cache_buffer+(WMA_cache_samples*2), WMA_mix_buffer, WMA_codec_buffer[9]);

			WMA_cache_samples += (WMA_codec_buffer[9]/4);

		}
		sceKernelDelayThread(10000);
	}

    if (getEDRAM)
        sceAudiocodecReleaseEDRAM(WMA_codec_buffer);

    if ( WMA_handle >= 0){
      sceIoClose(WMA_handle);
      WMA_handle = -1;
    }

	if ( parser )
		free_64(parser);
	if ( need_mem_buffer )
		free_64(need_mem_buffer);

	if (WMA_frame_buffer)
		free_64(WMA_frame_buffer);

	sceKernelExitThread(0);
    return 0;
}


void getWMATagInfo(char *filename, struct fileInfo *targetInfo){
    strcpy(WMA_fileName, filename);

	strcpy(targetInfo->title, filename);

    WMA_info = *targetInfo;
    WMA_tagRead = 1;
}

//Get info on file:
int WMAgetInfo(){
    if (!WMA_tagRead)
        getWMATagInfo(WMA_fileName, &WMA_info);

    pspDebugScreenPrintf("Artist: %s\n", WMA_info.artist);
    pspDebugScreenPrintf("Album : %s\n", WMA_info.album);
    pspDebugScreenPrintf("Title : %s\n", WMA_info.title);

	sceAudiocodecReleaseEDRAM(WMA_codec_buffer);

    WMA_handle = sceIoOpen(WMA_fileName, PSP_O_RDONLY, 0777);
    if (WMA_handle < 0)
    {
        pspDebugScreenPrintf("Error opening file\n");
        return -1;
    }
	WMA_filesize = sceIoLseek(WMA_handle, 0, PSP_SEEK_END);
    sceIoLseek(WMA_handle, 0, PSP_SEEK_SET);

    WMA_info.fileType = WMA_TYPE;
    WMA_info.defaultCPUClock = WMA_defaultCPUClock;
    WMA_info.needsME = 1;
	WMA_info.fileSize = WMA_filesize;
    WMA_info.framesDecoded = 0;
    strcpy(WMA_info.emphasis,"no");

	parser = malloc_64(sizeof(SceAsfParser));
	if ( !parser ) {
        pspDebugScreenPrintf("Error allocing parser\n");
		return -1;
	}
	memset(parser, 0, sizeof(SceAsfParser));

	parser->iUnk0 = 0x01;
	parser->iUnk1 = 0x00;
	parser->iUnk2 = 0x000CC003;
	parser->iUnk3 = 0x00;
	parser->iUnk4 = 0x00000000;
	parser->iUnk5 = 0x00000000;
	parser->iUnk6 = 0x00000000;
	parser->iUnk7 = 0x00000000;

	int ret = sceAsfCheckNeedMem(parser);
	if ( ret < 0 ) {
        pspDebugScreenPrintf("Error AsfCheckNeedMem\n");
        sceIoClose(WMA_handle);
        WMA_handle = -1;
		return -1;
	}

	if ( parser->iNeedMem > 0x8000 ) {
        pspDebugScreenPrintf("Error parser->iNeedMem\n");
        sceIoClose(WMA_handle);
	    return -1;
	}

	need_mem_buffer = malloc_64(parser->iNeedMem);
	if ( !need_mem_buffer ) {
        pspDebugScreenPrintf("Error need_mem_buffer\n");
        sceIoClose(WMA_handle);
        WMA_handle = -1;
		return -1;
	}

	parser->pNeedMemBuffer = need_mem_buffer;
	parser->iUnk3356 = 0x00000000;

	ret = sceAsfInitParser(parser, 0, &asf_read_cb, &asf_seek_cb);
	if ( ret < 0 ) {
        pspDebugScreenPrintf("Error sceAsfInitParser = 0x%08x\n", ret);
        sceIoClose(WMA_handle);
        WMA_handle = -1;
		return -1;
	}

	WMA_channels = *((u16*)need_mem_buffer);
	WMA_info.hz = *((u32*)(need_mem_buffer+4));
	WMA_format_tag = 0x0161;
	WMA_avg_bytes_per_sec = *((u32*)(need_mem_buffer+8));
    WMA_info.kbit = WMA_avg_bytes_per_sec * 8 / 1000;
    WMA_info.instantBitrate = WMA_avg_bytes_per_sec * 8;
	WMA_block_align = *((u16*)(need_mem_buffer+12));
	WMA_bits_per_sample = 16;
	WMA_flag = *((u16*)(need_mem_buffer+14));

    switch (WMA_channels) {
        case 2:
            strcpy(WMA_info.mode, "normal LR stereo");
            break;
        case 1:
            strcpy(WMA_info.mode, "mono");
            break;
        default:
            strcpy(WMA_info.emphasis,"unknown");
            break;
    }

	u64 duration = parser->lDuration/100000;
	int hour, minute, second, msecond;
	msecond = duration %100;
	duration /= 100;
	second =  duration % 60;
	duration /= 60;
	minute = duration % 60;
	hour = duration / 60;

    WMA_info.length = hour * 3600 + minute * 60 + second;
	snprintf(WMA_info.strLength, sizeof(WMA_info.strLength), "%2.2i:%2.2i:%2.2i", hour, minute, second);

	if ( parser )
		free_64(parser);
	if ( need_mem_buffer )
		free_64(need_mem_buffer);

    sceIoClose(WMA_handle);
    WMA_handle = -1;
    return 0;
}


///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//Public functions:
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void WMA_Init(int channel){
    WMA_audio_channel = channel;
	WMA_playingSpeed = 0;
    WMA_playingTime = 0;
	WMA_volume_boost = 0;
	WMA_volume = PSP_AUDIO_VOLUME_MAX;
    MIN_PLAYING_SPEED=-119;
    MAX_PLAYING_SPEED=119;

	initMEAudioModules();
    initFileInfo(&WMA_info);
    WMA_tagRead = 0;
}


int WMA_Load(char *fileName){
    WMA_filePos = 0;
    WMA_playingSpeed = 0;
    WMA_isPlaying = 0;
    strcpy(WMA_fileName, fileName);
    if (WMAgetInfo() != 0){
        strcpy(WMA_fileName, "");
        return ERROR_OPENING;
    }

    releaseAudio();
    if (setAudioFrequency(WMA_OUTPUT_BUFFER_SIZE, WMA_info.hz, 2) < 0){
        WMA_End();
        return ERROR_INVALID_SAMPLE_RATE;
    }

    WMA_thid = -1;
    WMA_eof = 0;
    WMA_thid = sceKernelCreateThread("WMA_decodeThread", WMA_decodeThread, THREAD_PRIORITY, DEFAULT_THREAD_STACK_SIZE, PSP_THREAD_ATTR_USER, NULL);
    if(WMA_thid < 0)
        return ERROR_CREATE_THREAD;

    sceKernelStartThread(WMA_thid, 0, NULL);
    return OPENING_OK;
}


int WMA_Play(){
    if(WMA_thid < 0)
        return -1;
    WMA_isPlaying = 1;
    return 0;
}


void WMA_Pause(){
    WMA_isPlaying = !WMA_isPlaying;
}

int WMA_Stop(){
    WMA_isPlaying = 0;
    WMA_threadActive = 0;
	sceKernelWaitThreadEnd(WMA_thid, NULL);
    sceKernelDeleteThread(WMA_thid);
    return 0;
}

int WMA_EndOfStream(){
    return WMA_eof;
}

void WMA_End(){
    WMA_Stop();
}


struct fileInfo *WMA_GetInfo(){
    return &WMA_info;
}


float WMA_GetPercentage(){
	//Calcolo posizione in %:
	float perc = 0.0f;

    if (WMA_filesize > 0){
        perc = ((float)WMA_filePos - (float)WMA_tagsize) / ((float)WMA_filesize - (float)WMA_tagsize) * 100.0;
        if (perc > 100)
            perc = 100;
    }else{
        perc = 0;
    }
    return(perc);
}


int WMA_getPlayingSpeed(){
	return WMA_playingSpeed;
}


int WMA_setPlayingSpeed(int playingSpeed){
	if (playingSpeed >= MIN_PLAYING_SPEED && playingSpeed <= MAX_PLAYING_SPEED){
		WMA_playingSpeed = playingSpeed;
		if (playingSpeed == 0)
			WMA_volume = PSP_AUDIO_VOLUME_MAX;
		else
			WMA_volume = FASTFORWARD_VOLUME;
		return 0;
	}else{
		return -1;
	}
}


void WMA_setVolumeBoost(int boost){
    WMA_volume_boost = boost;
}


int WMA_getVolumeBoost(){
    return(WMA_volume_boost);
}

void WMA_GetTimeString(char *dest){
    char timeString[9];
    int secs = (int)WMA_playingTime;
    int hh = secs / 3600;
    int mm = (secs - hh * 3600) / 60;
    int ss = secs - hh * 3600 - mm * 60;
    snprintf(timeString, sizeof(timeString), "%2.2i:%2.2i:%2.2i", hh, mm, ss);
    strcpy(dest, timeString);
}

struct fileInfo WMA_GetTagInfoOnly(char *filename){
    struct fileInfo tempInfo;
    initFileInfo(&tempInfo);
    getWMATagInfo(filename, &tempInfo);
	return tempInfo;
}

int WMA_isFilterSupported(){
	return 0;
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//Set mute:
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
int WMA_setMute(int onOff){
	if (onOff)
    	WMA_volume = MUTED_VOLUME;
	else
    	WMA_volume = PSP_AUDIO_VOLUME_MAX;
    return 0;
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//Fade out:
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void WMA_fadeOut(float seconds){
    int i = 0;
    long timeToWait = (long)((seconds * 1000.0) / (float)WMA_volume);
    for (i=WMA_volume; i>=0; i--){
        WMA_volume = i;
        sceKernelDelayThread(timeToWait);
    }
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//Manage suspend:
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
int WMA_suspend(){
    WMA_suspendPosition = WMA_filePos;
    WMA_suspendIsPlaying = WMA_isPlaying;

	WMA_isPlaying = 0;
    sceIoClose(WMA_handle);
    WMA_handle = -1;
    return 0;
}

int WMA_resume(){
	if (WMA_suspendPosition >= 0){
		WMA_handle = sceIoOpen(WMA_fileName, PSP_O_RDONLY, 0777);
		if (WMA_handle >= 0){
			WMA_filePos = WMA_suspendPosition;
			sceIoLseek32(WMA_handle, WMA_filePos, PSP_SEEK_SET);
			WMA_isPlaying = WMA_suspendIsPlaying;
		}
	}
	WMA_suspendPosition = -1;
    return 0;
}

void WMA_setVolumeBoostType(char *boostType){
    //Only old method supported
    MAX_VOLUME_BOOST = 4;
    //MAX_VOLUME_BOOST = 0;
    MIN_VOLUME_BOOST = 0;
}

double WMA_getFilePosition()
{
    return WMA_filePos;
}

void WMA_setFilePosition(double position)
{
    WMA_newFilePos = position;
}

//TODO:

int WMA_GetStatus(){
    return 0;
}


int WMA_setFilter(double tFilter[32], int copyFilter){
    return 0;
}

void WMA_enableFilter(){
}

void WMA_disableFilter(){
}

int WMA_isFilterEnabled(){
    return 0;
}

