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
#include <unistd.h>

#include "player.h"
#include "id3.h"
#include "aa3playerME.h"
#include "../system/opendir.h"

#define AT3_THREAD_PRIORITY 12
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//Globals:
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
SceUID AA3ME_handle;
static int AA3ME_threadActive = 0;
//static int AA3ME_threadExited = 1;
static char AA3ME_fileName[264];
static int AA3ME_isPlaying = 0;
static int AA3ME_thid = -1;
static int AA3ME_audio_channel = 0;
static int AA3ME_eof = 0;
static struct fileInfo AA3ME_info;
static float AA3ME_playingTime = 0.0f;
static int AA3ME_volume = 0;
static int AA3ME_playingSpeed = 0; // 0 = normal
int AA3ME_defaultCPUClock = 20;
static unsigned int AA3ME_volume_boost = 0;
static long AA3ME_suspendPosition = -1;
static long AA3ME_suspendIsPlaying = 0;
static double AA3ME_filePos = 0;
static double AA3ME_newFilePos = -1;
static int AA3ME_tagRead = 0;

static unsigned char AA3ME_input_buffer[2889]__attribute__((aligned(64)));//mp3 has the largest max frame, at3+ 352 is 2176
static unsigned long AA3ME_codec_buffer[65]__attribute__((aligned(64)));
static short AA3ME_output_buffer[2048*2]__attribute__((aligned(64)));//at3+ sample_per_frame*4

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

	//AA3ME_threadExited = 0;
    AA3ME_threadActive = 1;

	sceAudiocodecReleaseEDRAM(AA3ME_codec_buffer); //Fix: ReleaseEDRAM at the end is not enough to play another file.
    OutputBuffer_flip = 0;
    AT3_OutputPtr = AT3_OutputBuffer[0];

    sceIoChdir(audioCurrentDir);
    tag_size = GetID3TagSize(AA3ME_fileName);
    AA3ME_handle = sceIoOpen(AA3ME_fileName, PSP_O_RDONLY, 0777);
    if (AA3ME_handle < 0)
        AA3ME_threadActive = 0;

    sceIoLseek32(AA3ME_handle, tag_size, PSP_SEEK_SET);//not all omg files have a fixed header

    if ( sceIoRead( AA3ME_handle, ea3_header, 0x60 ) != 0x60 )
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
    data_size = sceIoLseek32(AA3ME_handle, 0, PSP_SEEK_END) - data_start;

    sceIoLseek32(AA3ME_handle, data_start, PSP_SEEK_SET);

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
			data_start = sceIoLseek32(AA3ME_handle, 0, PSP_SEEK_CUR);
            AA3ME_filePos = data_start;

			if (data_start < 0){
				AA3ME_isPlaying = 0;
				AA3ME_threadActive = 0;
				continue;
			}

            if (AA3ME_newFilePos >= 0)
            {
                sceIoLseek32(AA3ME_handle, AA3ME_newFilePos, PSP_SEEK_SET);
                AA3ME_playingTime = (float)(AA3ME_newFilePos) / (float)data_align * (float)sample_per_frame/(float)samplerate;
                AA3ME_filePos = AA3ME_newFilePos;
                AA3ME_newFilePos = -1;
            }

			if ( at3_type == TYPE_ATRAC3 ) {
				memset( AA3ME_input_buffer, 0, 0x180);

				res = sceIoRead( AA3ME_handle, AA3ME_input_buffer, data_align );

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

				res = sceIoRead( AA3ME_handle, AA3ME_input_buffer+8, data_align );

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
				//Volume Boost:
				if (AA3ME_volume_boost){
                    int i;
                    for (i=0; i<AT3_OUTPUT_BUFFER_SIZE; i++){
    					AT3_OutputBuffer[OutputBuffer_flip][i] = volume_boost(&AT3_OutputBuffer[OutputBuffer_flip][i], &AA3ME_volume_boost);
                    }
                }
				audioOutput(AA3ME_volume, AT3_OutputBuffer[OutputBuffer_flip]);

				OutputBuffer_flip ^= 1;
				AT3_OutputPtr = AT3_OutputBuffer[OutputBuffer_flip];
		        //Check for playing speed:
                if (AA3ME_playingSpeed){
                    sceIoLseek32(AA3ME_handle, sceIoLseek32(AA3ME_handle, 0, PSP_SEEK_CUR) + data_align * AA3ME_playingSpeed, PSP_SEEK_SET);
                    AA3ME_playingTime += (float)(data_align * AA3ME_playingSpeed) / (float)data_align * (float)sample_per_frame/(float)samplerate;
                }
			}
		}
		sceKernelDelayThread(10000);
	}
	sceIoClose(AA3ME_handle);
    AA3ME_handle = -1;
    //AA3ME_threadExited = 1;
	sceKernelExitThread(0);
    return 0;
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//Get tag info:
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void getAA3METagInfo(char *filename, struct fileInfo *targetInfo){
    int fp = 0;

    int size;
    int tag_length;
    char tag[4];

    strcpy(AA3ME_fileName, filename);
    size = GetID3TagSize(filename);

	fp = sceIoOpen(filename, PSP_O_RDONLY, 0777);
    if (fp < 0) return;
	sceIoLseek(fp, 10, PSP_SEEK_SET);

    while (size != 0) {
		sceIoRead(fp, tag, 4);
        size -= 4;

        /* read 4 byte big endian tag length */
		sceIoRead(fp, &tag_length, sizeof(unsigned int));
        tag_length = (unsigned int) swapInt32BigToHost((int)tag_length);
        size -= 4;

		sceIoLseek(fp, 2, PSP_SEEK_CUR);
        size -= 2;

        /* Perform checks for end of tags and tag length overflow or zero */
        if(*tag == 0 || tag_length > size || tag_length == 0) break;

        if(!strncmp("TPE1",tag,4)) /* Artist */
        {
			sceIoLseek(fp, 1, PSP_SEEK_CUR);
            readTagData(fp, tag_length - 1, 260, targetInfo->artist);
        }
        else if(!strncmp("TIT2",tag,4)) /* Title */
        {
			sceIoLseek(fp, 1, PSP_SEEK_CUR);
			readTagData(fp, tag_length - 1, 260, targetInfo->title);
        }
        else if(!strncmp("TALB",tag,4)) /* Album */
        {
            sceIoLseek(fp, 1, PSP_SEEK_CUR);
            readTagData(fp, tag_length - 1, 260, targetInfo->album);
        }
        else if(!strncmp("TRCK",tag,4)) /* Track No. */
        {
            sceIoLseek(fp, 1, PSP_SEEK_CUR);
            readTagData(fp, tag_length - 1, 8, targetInfo->trackNumber);
        }
        else if(!strncmp("TYER",tag,4)) /* Year */
        {
            sceIoLseek(fp, 1, PSP_SEEK_CUR);
            readTagData(fp, tag_length - 1, 12, targetInfo->year);
        }
        else if(!strncmp("TCON",tag,4)) /* Genre */
        {
            sceIoLseek(fp, 1, PSP_SEEK_CUR);
            readTagData(fp, tag_length - 1, 260, targetInfo->genre);
        }
        else if(!strncmp("APIC",tag,4)) /* Picture */
        {
            /*sceIoLseek(fp, 1, PSP_SEEK_CUR);
            fseek(fp, 13, SEEK_CUR);
            targetInfo->encapsulatedPictureOffset = ftell(fp);
            targetInfo->encapsulatedPictureLength = tag_length-14;
            fseek(fp, tag_length-14, SEEK_CUR);*/
        }
        else
        {
			sceIoLseek(fp, tag_length, PSP_SEEK_CUR);
        }
        size -= tag_length;
	}
    if (!strlen(targetInfo->title))
        getFileName(AA3ME_fileName, targetInfo->title);
	sceIoClose(fp);

    AA3ME_info = *targetInfo;
    AA3ME_tagRead = 1;
}

struct fileInfo AA3ME_GetTagInfoOnly(char *filename){
    struct fileInfo tempInfo;
    initFileInfo(&tempInfo);
    getAA3METagInfo(filename, &tempInfo);
	return tempInfo;
}

//Get info on file:
int AA3MEgetInfo(){
    AA3ME_info.fileType = AT3_TYPE;
    AA3ME_info.defaultCPUClock = AA3ME_defaultCPUClock;
    AA3ME_info.needsME = 1;
    strcpy(AA3ME_info.layer, "");

    u8 ea3_header[0x60];
    int tag_size;
    float totalPlayingTime = 0.0f;

    tag_size = GetID3TagSize(AA3ME_fileName);
	SceUID fd;
    fd = sceIoOpen(AA3ME_fileName, PSP_O_RDONLY, 0777);
    if (fd < 0)
        return -1;
    double fileSize = sceIoLseek(fd, 0, PSP_SEEK_END);
	sceIoLseek(fd, 0, PSP_SEEK_SET);
    sceIoLseek32(fd, tag_size, PSP_SEEK_SET);

    if ( sceIoRead( fd, ea3_header, 0x60 ) != 0x60 ){
        sceIoClose(fd);
        fd = -1;
        return -1;
    }

    if ( ea3_header[0] != 0x45 || ea3_header[1] != 0x41 || ea3_header[2] != 0x33 ){
        sceIoClose(fd);
        fd = -1;
        return -1;
    }

    at3_at3plus_flagdata[0] = ea3_header[0x22];
    at3_at3plus_flagdata[1] = ea3_header[0x23];

    at3_type = (ea3_header[0x22] == 0x20) ? TYPE_ATRAC3 : ((ea3_header[0x22] == 0x28) ? TYPE_ATRAC3PLUS : 0x0);

    if ( at3_type != TYPE_ATRAC3 && at3_type != TYPE_ATRAC3PLUS ){
        sceIoClose(fd);
        fd = -1;
        return -1;
    }

    if ( at3_type == TYPE_ATRAC3 )
        data_align = ea3_header[0x23]*8;
    else
        data_align = (ea3_header[0x23]+1)*8;

    if ( data_align == 0xC0 )
        AA3ME_info.kbit = 66;
    else if ( data_align == 0x178 )
        AA3ME_info.kbit = 64;
    else if ( data_align == 0x230 )
        AA3ME_info.kbit = 96;
    else if ( data_align == 0x130 )
        AA3ME_info.kbit = 105;
    else if ( data_align == 0x180 )
        AA3ME_info.kbit = 132;
    else if ( data_align == 0x2E8 )
        AA3ME_info.kbit = 128;
	else
        AA3ME_info.kbit = data_align; //Unknown bitrate!
    AA3ME_info.instantBitrate = AA3ME_info.kbit * 1000;

    if ( at3_type == TYPE_ATRAC3 )
        sample_per_frame = 1024;
    else if ( at3_type == TYPE_ATRAC3PLUS )
        sample_per_frame = 2048;
    else{
        sceIoClose(fd);
        fd = -1;
        return -1;
    }
    samplerate = 44100;

    totalPlayingTime = (float)fileSize / (float)data_align * (float)sample_per_frame/(float)samplerate;

	AA3ME_info.hz = samplerate;
	AA3ME_info.length = totalPlayingTime;
	long secs = AA3ME_info.length;
	int h = secs / 3600;
	int m = (secs - h * 3600) / 60;
	int s = secs - h * 3600 - m * 60;
	snprintf(AA3ME_info.strLength, sizeof(AA3ME_info.strLength), "%2.2i:%2.2i:%2.2i", h, m, s);
	strcpy(AA3ME_info.mode, "joint (MS/intensity) stereo");
	strcpy(AA3ME_info.emphasis,"no");
	sceIoClose(fd);
    fd = -1;

    if (!AA3ME_tagRead)
        getAA3METagInfo(AA3ME_fileName, &AA3ME_info);
    return 0;
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//Public functions:
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void AA3ME_Init(int channel){
    AA3ME_audio_channel = channel;
    AA3ME_playingTime = 0;
    AA3ME_volume = PSP_AUDIO_VOLUME_MAX;
    MIN_PLAYING_SPEED=-119;
    MAX_PLAYING_SPEED=119;

    initFileInfo(&AA3ME_info);
    AA3ME_tagRead = 0;

	initMEAudioModules();
}


int AA3ME_Load(char *fileName){
    AA3ME_filePos = 0;
    AA3ME_playingSpeed = 0;
    AA3ME_isPlaying = 0;

    getcwd(audioCurrentDir, 256);
    strcpy(AA3ME_fileName, fileName);
    if (AA3MEgetInfo() != 0){
        return ERROR_OPENING;
    }

    releaseAudio();
    if (setAudioFrequency(AT3_OUTPUT_BUFFER_SIZE/2, AA3ME_info.hz, 2) < 0){
        AA3ME_End();
        return ERROR_INVALID_SAMPLE_RATE;
    }

    AA3ME_thid = -1;
    AA3ME_eof = 0;
    AA3ME_thid = sceKernelCreateThread("AA3ME_decodeThread", AA3ME_decodeThread, AT3_THREAD_PRIORITY, DEFAULT_THREAD_STACK_SIZE, PSP_THREAD_ATTR_USER, NULL);
    if(AA3ME_thid < 0)
        return ERROR_CREATE_THREAD;

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
    /*while (!AA3ME_threadExited)
        sceKernelDelayThread(100000);*/
	sceKernelWaitThreadEnd(AA3ME_thid, NULL);
    sceKernelDeleteThread(AA3ME_thid);
    return 0;
}

int AA3ME_EndOfStream(){
    return AA3ME_eof;
}

void AA3ME_End(){
    AA3ME_Stop();
}


struct fileInfo *AA3ME_GetInfo(){
    return &AA3ME_info;
}


float AA3ME_GetPercentage(){
    float perc = (float)(AA3ME_playingTime/(double)AA3ME_info.length*100.0);
    if (perc > 100)
        perc = 100;
	return perc;
}


int AA3ME_isFilterSupported(){
	return 0;
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//Manage suspend:
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
int AA3ME_suspend(){
    AA3ME_suspendPosition = AA3ME_filePos;
    AA3ME_suspendIsPlaying = AA3ME_isPlaying;

	AA3ME_isPlaying = 0;
    sceIoClose(AA3ME_handle);
    AA3ME_handle = -1;

    return 0;
}

int AA3ME_resume(){
	if (AA3ME_suspendPosition >= 0){
		AA3ME_handle = sceIoOpen(AA3ME_fileName, PSP_O_RDONLY, 0777);
		if (AA3ME_handle >= 0){
			AA3ME_filePos = AA3ME_suspendPosition;
			sceIoLseek32(AA3ME_handle, AA3ME_filePos, PSP_SEEK_SET);
			AA3ME_isPlaying = AA3ME_suspendIsPlaying;
		}
	}
	AA3ME_suspendPosition = -1;
    return 0;
}

int AA3ME_setMute(int onOff){
	if (onOff)
    	AA3ME_volume = MUTED_VOLUME;
	else
    	AA3ME_volume = PSP_AUDIO_VOLUME_MAX;
    return 0;
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
    int i = 0;
    long timeToWait = (long)((seconds * 1000.0) / (float)AA3ME_volume);
    for (i=AA3ME_volume; i>=0; i--){
        AA3ME_volume = i;
        sceKernelDelayThread(timeToWait);
    }
}

int AA3ME_getPlayingSpeed(){
	return AA3ME_playingSpeed;
}

int AA3ME_setPlayingSpeed(int playingSpeed){
	if (playingSpeed >= MIN_PLAYING_SPEED && playingSpeed <= MAX_PLAYING_SPEED){
		AA3ME_playingSpeed = playingSpeed;
		if (playingSpeed == 0)
			AA3ME_volume = PSP_AUDIO_VOLUME_MAX;
		else
			AA3ME_volume = FASTFORWARD_VOLUME;
		return 0;
	}else{
		return -1;
	}
}

void AA3ME_setVolumeBoostType(char *boostType){
    //Only old method supported
    MAX_VOLUME_BOOST = 4;
    MIN_VOLUME_BOOST = 0;
}

void AA3ME_setVolumeBoost(int boost){
    AA3ME_volume_boost = boost;
}

int AA3ME_getVolumeBoost(){
    return AA3ME_volume_boost;
}

double AA3ME_getFilePosition()
{
    return AA3ME_filePos;
}

void AA3ME_setFilePosition(double position)
{
    AA3ME_newFilePos = position;
}

//TODO:
int AA3ME_GetStatus(){
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
