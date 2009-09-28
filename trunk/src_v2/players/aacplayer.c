//    Copyright (C) 2009 Sakya
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
//    This file contains functions to play aac files through the PSP's Media Engine.
//    This code is based upon this sample code from ps2dev.org
//    http://forums.ps2dev.org/viewtopic.php?t=8469
//    and the source code of Music prx by joek2100
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <malloc.h>
#include <math.h>
#include <ctype.h>

#include "id3.h"
#include "player.h"
#include "aacplayer.h"

#define THREAD_PRIORITY 12
#define OUTPUT_BUFFER_SIZE	(1152*4)

#define aac_sample_per_frame 1024
#define ADIF_MAX_SIZE 30 /* Should be enough */
#define ADTS_MAX_SIZE 10 /* Should be enough */
static int sample_rates[] = {96000,88200,64000,48000,44100,32000,24000,22050,16000,12000,11025,8000};

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//Globals:
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
static int AAC_threadActive = 0;
static char AAC_fileName[264];
static int AAC_isPlaying = 0;
static int AAC_thid = -1;
static int AAC_audio_channel = 0;
static struct fileInfo AAC_info;
static int AAC_playingSpeed = 0; // 0 = normal
static unsigned int AAC_volume_boost = 0;
static float AAC_playingTime = 0;
static int AAC_volume = 0;
int AAC_defaultCPUClock = 20;
static double AAC_filePos = 0;
static double AAC_newFilePos = -1;
static int AAC_tagRead = 0;
static double AAC_tagsize = 0;
static double AAC_filesize = 0;

static long AAC_suspendPosition = -1;
static long AAC_suspendIsPlaying = 0;

//Globals for decoding:
static unsigned long aac_codec_buffer[65] __attribute__((aligned(64)));
static short aac_mix_buffer[aac_sample_per_frame * 2] __attribute__((aligned(64)));
static short aac_output_buffer[4][aac_sample_per_frame * 2] __attribute__((aligned(64)));
static int aac_output_index = 0;

static int AAC_eof = 0;
static SceUID AAC_handle;

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//Private functions:
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//Decode thread:
int AACdecodeThread(SceSize args, void *argp){
   u8* aac_data_buffer = NULL;
   u32 aac_data_start;
   u8 aac_getEDRAM;
   u32 aac_channels;
   u32 aac_samplerate;

   sceAudiocodecReleaseEDRAM(aac_codec_buffer);
   sceIoChdir(audioCurrentDir);

   AAC_threadActive = 1;
   AAC_handle = sceIoOpen(AAC_fileName, PSP_O_RDONLY, 0777);
   if (  ! AAC_handle ){
      AAC_threadActive = 0;
      goto end;
   }

   aac_output_index = 0;
   aac_channels = 2;
   aac_samplerate = AAC_info.hz;

   aac_data_start = 0;
   if (AAC_tagsize > 0)
       aac_data_start = AAC_tagsize + 10;

   sceIoLseek32(AAC_handle, aac_data_start, PSP_SEEK_SET);
   aac_data_start = SeekNextFrameMP3(AAC_handle);

   memset(aac_codec_buffer, 0, sizeof(aac_codec_buffer));

   if ( sceAudiocodecCheckNeedMem(aac_codec_buffer, 0x1003) < 0 ){
      AAC_threadActive = 0;
      goto end;
   }

   if ( sceAudiocodecGetEDRAM(aac_codec_buffer, 0x1003) < 0 ){
         AAC_threadActive = 0;
         goto end;
   }
   aac_getEDRAM = 1;

   aac_codec_buffer[10] = aac_samplerate;
   if ( sceAudiocodecInit(aac_codec_buffer, 0x1003) < 0 ){
      AAC_threadActive = 0;
      goto end;
   }

    int samplesdecoded;
    AAC_eof = 0;
    unsigned char aac_header_buf[7];
    int skip = 0;

	while (AAC_threadActive){
		while( !AAC_eof && AAC_isPlaying ){
              memset(aac_mix_buffer, 0, aac_sample_per_frame*2*2);
              if ( sceIoRead( AAC_handle, aac_header_buf, 7 ) != 7 ) {
                 AAC_eof = 1;
                 continue;
              }
              int aac_header = aac_header_buf[3];
              aac_header = (aac_header<<8) | aac_header_buf[4];
              aac_header = (aac_header<<8) | aac_header_buf[5];
              aac_header = (aac_header<<8) | aac_header_buf[6];

              int frame_size = aac_header & 67100672;
              frame_size = frame_size >> 13;
              frame_size = frame_size - 7;

              if ( aac_data_buffer ){
                 free(aac_data_buffer);
                 aac_data_buffer = NULL;
              }
              aac_data_buffer = (u8*)memalign(64, frame_size);

              if (AAC_newFilePos >= 0)
              {
                  if (!AAC_newFilePos)
                      AAC_newFilePos = AAC_tagsize + 10;

                  u32 old_start = aac_data_start;
                  if (sceIoLseek32(AAC_handle, AAC_newFilePos, PSP_SEEK_SET) != old_start){
                      aac_data_start = SeekNextFrameMP3(AAC_handle);
                      if(aac_data_start < 0){
                          AAC_eof = 1;
                      }
                      AAC_playingTime = (float)aac_data_start / (float)frame_size /  (float)aac_samplerate / 1000.0f;
                  }
                  AAC_newFilePos = -1;
                  continue;
              }

		      //Check for playing speed:
              if (AAC_playingSpeed){
                  if (skip){
                      u32 old_start = aac_data_start;
                      if (sceIoLseek32(AAC_handle, frame_size / 4 * AAC_playingSpeed, PSP_SEEK_CUR) != old_start){
                          aac_data_start = SeekNextFrameMP3(AAC_handle);
                          if(aac_data_start < 0){
                              AAC_eof = 1;
                              continue;
                          }
                          AAC_filePos = aac_data_start;
                          float framesSkipped = (float)aac_data_start / (float)frame_size;
                          AAC_playingTime = framesSkipped * (float)aac_sample_per_frame/(float)aac_samplerate;
                      }else
                          AAC_setPlayingSpeed(0);
                      skip = !skip;
                      continue;
                  }
                  skip = !skip;
              }

              if ( sceIoRead( AAC_handle, aac_data_buffer, frame_size ) != frame_size ) {
                 AAC_eof = 1;
                 continue;
              }

              aac_data_start += (frame_size+7);
              AAC_filePos = aac_data_start;

              aac_codec_buffer[6] = (unsigned long)aac_data_buffer;
              aac_codec_buffer[8] = (unsigned long)aac_mix_buffer;

              aac_codec_buffer[7] = frame_size;
              aac_codec_buffer[9] = aac_sample_per_frame * 4;


              int res = sceAudiocodecDecode(aac_codec_buffer, 0x1003);
              if ( res < 0 ) {
                 AAC_eof = 1;
                 continue;
              }
              memcpy(aac_output_buffer[aac_output_index], aac_mix_buffer, aac_sample_per_frame*4);

              //Volume Boost:
              if (AAC_volume_boost){
                  int i;
                  for (i=0; i<aac_sample_per_frame * 2; i++){
                    aac_output_buffer[aac_output_index][i] = volume_boost(&aac_output_buffer[aac_output_index][i], &AAC_volume_boost);
                  }
              }

              audioOutput(AAC_volume, aac_output_buffer[aac_output_index]);
              aac_output_index = (aac_output_index+1)%4;

              samplesdecoded = aac_sample_per_frame;

              AAC_playingTime += (float)aac_sample_per_frame/(float)aac_samplerate;
              AAC_info.framesDecoded++;
            }
            sceKernelDelayThread(10000);
    }
end:
   if ( AAC_handle )
      sceIoClose(AAC_handle);

   if ( aac_data_buffer)
   {
      free(aac_data_buffer);
      aac_data_buffer = NULL;
   }

   if ( aac_getEDRAM )
      sceAudiocodecReleaseEDRAM(aac_codec_buffer);

    sceKernelExitThread(0);
    return 0;
}


void getAACTagInfo(char *filename, struct fileInfo *targetInfo){
    //ID3:
    struct ID3Tag ID3;
    strcpy(AAC_fileName, filename);
    ParseID3(filename, &ID3);
    strcpy(targetInfo->title, ID3.ID3Title);
    strcpy(targetInfo->artist, ID3.ID3Artist);
    strcpy(targetInfo->album, ID3.ID3Album);
    strcpy(targetInfo->year, ID3.ID3Year);
    strcpy(targetInfo->genre, ID3.ID3GenreText);
    strcpy(targetInfo->trackNumber, ID3.ID3TrackText);
    targetInfo->length = ID3.ID3Length;
    targetInfo->encapsulatedPictureType = ID3.ID3EncapsulatedPictureType;
    targetInfo->encapsulatedPictureOffset = ID3.ID3EncapsulatedPictureOffset;
    targetInfo->encapsulatedPictureLength = ID3.ID3EncapsulatedPictureLength;

    AAC_info = *targetInfo;
    AAC_tagRead = 1;
}

int StringComp(char const *str1, char const *str2, unsigned long len)
{
    signed int c1 = 0, c2 = 0;

    while (len--)
    {
        c1 = tolower(*str1++);
        c2 = tolower(*str2++);

        if (c1 == 0 || c1 != c2)
            break;
    }

    return c1 - c2;
}

static int read_ADIF_header(SceUID file, unsigned long filesize, struct fileInfo *info)
{
    int bitstream;
    unsigned char buffer[ADIF_MAX_SIZE];
    int skip_size = 0;
    int sf_idx;

    /* Get ADIF header data */
    if (sceIoRead(file, buffer, ADIF_MAX_SIZE) != ADIF_MAX_SIZE)
        return -1;

    /* copyright string */
    if(buffer[0] & 0x80)
        skip_size += 9; /* skip 9 bytes */

    bitstream = buffer[0 + skip_size] & 0x10;
    /*info->bitrate = ((unsigned int)(buffer[0 + skip_size] & 0x0F)<<19)|
        ((unsigned int)buffer[1 + skip_size]<<11)|
        ((unsigned int)buffer[2 + skip_size]<<3)|
        ((unsigned int)buffer[3 + skip_size] & 0xE0);*/
    int bitrate = ((unsigned int)(buffer[0 + skip_size] & 0x0F)<<19)|
                  ((unsigned int)buffer[1 + skip_size]<<11)|
                  ((unsigned int)buffer[2 + skip_size]<<3)|
                  ((unsigned int)buffer[3 + skip_size] & 0xE0);
    info->kbit = bitrate / 1000;
    if (bitstream == 0)
    {
        //info->object_type =  ((buffer[6 + skip_size]&0x01)<<1)|((buffer[7 + skip_size]&0x80)>>7);
        sf_idx = (buffer[7 + skip_size]&0x78)>>3;
    } else {
        //info->object_type = (buffer[4 + skip_size] & 0x18)>>3;
        sf_idx = ((buffer[4 + skip_size] & 0x07)<<1)|((buffer[5 + skip_size] & 0x80)>>7);
    }
    info->hz = sample_rates[sf_idx];

    int length = (int)((float)filesize*8000.0/((float)bitrate));
    info->length = length / 1000;

    return 0;
}


static int read_ADTS_header(SceUID file, unsigned long filesize, struct fileInfo *info)
{
    /* Get ADTS header data */
    unsigned char buffer[ADTS_MAX_SIZE];
    int frames, t_framelength = 0, frame_length, sr_idx = 0, ID;
    int pos;
    float frames_per_sec = 0;
    unsigned long bytes;
    int framesToRead = 2000;

    for (frames = 0; /* */; frames++)
    {
        if (framesToRead && frames > framesToRead)
            break;

        bytes = sceIoRead(file, buffer, ADTS_MAX_SIZE);
        if (bytes != ADTS_MAX_SIZE)
            break;

        /* check syncword */
        if (!((buffer[0] == 0xFF)&&((buffer[1] & 0xF6) == 0xF0)))
            break;

        if (!frames)
        {
            /* fixed ADTS header is the same for every frame, so we read it only once */
            /* Syncword found, proceed to read in the fixed ADTS header */
            ID = buffer[1] & 0x08;
            //info->object_type = (buffer[2]&0xC0)>>6;
            sr_idx = (buffer[2]&0x3C)>>2;
            int channels = ((buffer[2]&0x01)<<2)|((buffer[3]&0xC0)>>6);
            switch (channels) {
            case 1:
                strcpy(AAC_info.mode, "single channel");
                break;
            case 2:
                strcpy(AAC_info.mode, "normal LR stereo");
                break;
            }
            frames_per_sec = sample_rates[sr_idx] / 1024.f;
        }

        /* ...and the variable ADTS header */
        int version = 0;
        if (ID == 0)
        {
            //info->version = 4;
            version = 4;
        } else { /* MPEG-2 */
            //info->version = 2;
            version = 2;
        }
        frame_length = ((((unsigned int)buffer[3] & 0x3)) << 11)
            | (((unsigned int)buffer[4]) << 3) | (buffer[5] >> 5);

        t_framelength += frame_length;

        pos = sceIoLseek32(file, 0, PSP_SEEK_CUR) - ADTS_MAX_SIZE;

        sceIoLseek32(file, frame_length - ADTS_MAX_SIZE, PSP_SEEK_CUR);
    }

    if (frames > 0)
    {
        float sec_per_frame, bytes_per_frame;
        info->hz = sample_rates[sr_idx];
        sec_per_frame = (float)sample_rates[sr_idx]/1024.0;
        bytes_per_frame = (float)t_framelength / (float)frames;
        info->kbit = 8 * (int)floor(bytes_per_frame * sec_per_frame) / 1000;
        info->instantBitrate = info->kbit * 1000;
        int length = (int)floor((float)frames/frames_per_sec)*1000;
        if (framesToRead)
        {
            length = (float)length * ((float)filesize / (float)pos);
        }
        info->length = length / 1000;
    }

    return 0;
}

//Get info on file:
int AACMEgetInfo(){
    if (!AAC_tagRead)
        getAACTagInfo(AAC_fileName, &AAC_info);

    AAC_info.kbit = 128;
    AAC_info.hz = 44100;
    AAC_info.length = 0;
    AAC_info.fileType = AAC_TYPE;
    AAC_info.defaultCPUClock = AAC_defaultCPUClock;
    AAC_info.needsME = 1;
    AAC_info.framesDecoded = 0;
    strcpy(AAC_info.mode, "normal LR stereo");
    strcpy(AAC_info.emphasis,"no");

    AAC_tagsize = ID3v2TagSize(AAC_fileName);
    unsigned long tagsize = 0;
    if (AAC_tagsize > 0)
        tagsize = AAC_tagsize + 10;

    SceUID file = -1;
    unsigned long file_len;
    unsigned char adxx_id[5];
    unsigned long tmp;

    file = sceIoOpen(AAC_fileName, PSP_O_RDONLY, 0777);

    if(file < 0)
        return - 1;

    file_len = sceIoLseek32(file, 0, PSP_SEEK_END);
    AAC_filesize = file_len;
    sceIoLseek32(file, tagsize, PSP_SEEK_SET);

    if (file_len)
        file_len -= tagsize;

    tmp = sceIoRead(file, adxx_id, 2);
    adxx_id[5-1] = 0;

    /* Determine the header type of the file, check the first two bytes */
    if (StringComp(adxx_id, "AD", 2) == 0)
    {
        /* We think its an ADIF header, but check the rest just to make sure */
        tmp = sceIoRead(file, adxx_id + 2, 2);

        if (StringComp(adxx_id, "ADIF", 4) == 0)
        {
            read_ADIF_header(file, file_len, &AAC_info);
        }
    } else {
        /* No ADIF, check for ADTS header */
        if ((adxx_id[0] == 0xFF)&&((adxx_id[1] & 0xF6) == 0xF0))
        {
            /* ADTS  header located */
            sceIoLseek32(file, tagsize, PSP_SEEK_SET);
            read_ADTS_header(file, file_len, &AAC_info);
        } else {
            //pspDebugScreenPrintf("Unknown headers\n");
        }
    }

    sceIoClose(file);

    long secs = AAC_info.length;
	//Formatto in stringa la durata totale:
	int h = secs / 3600;
	int m = (secs - h * 3600) / 60;
	int s = secs - h * 3600 - m * 60;
	snprintf(AAC_info.strLength, sizeof(AAC_info.strLength), "%2.2i:%2.2i:%2.2i", h, m, s);

    return 0;
}


///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//Public functions:
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void AAC_Init(int channel){
    AAC_audio_channel = channel;
	AAC_playingSpeed = 0;
    AAC_playingTime = 0;
	AAC_volume_boost = 0;
	AAC_volume = PSP_AUDIO_VOLUME_MAX;
    MIN_PLAYING_SPEED=-119;
    MAX_PLAYING_SPEED=119;
	initMEAudioModules();
    initFileInfo(&AAC_info);
    AAC_tagRead = 0;
}


int AAC_Load(char *fileName){
    AAC_filePos = 0;
    AAC_playingSpeed = 0;
    AAC_isPlaying = 0;

    getcwd(audioCurrentDir, 256);

    //initFileInfo(&AAC_info);
    strcpy(AAC_fileName, fileName);
    if (AACMEgetInfo() != 0){
        strcpy(AAC_fileName, "");
        return ERROR_OPENING;
    }

    releaseAudio();
    if (setAudioFrequency(aac_sample_per_frame, AAC_info.hz, 2) < 0){
        AAC_End();
        return ERROR_INVALID_SAMPLE_RATE;
    }

    AAC_thid = -1;
    AAC_eof = 0;
    AAC_thid = sceKernelCreateThread("AACdecodeThread", AACdecodeThread, THREAD_PRIORITY, DEFAULT_THREAD_STACK_SIZE, PSP_THREAD_ATTR_USER, NULL);
    if(AAC_thid < 0)
        return ERROR_CREATE_THREAD;

    sceKernelStartThread(AAC_thid, 0, NULL);
    return OPENING_OK;
}


int AAC_Play(){
    if(AAC_thid < 0)
        return -1;
    AAC_isPlaying = 1;
    return 0;
}


void AAC_Pause(){
    AAC_isPlaying = !AAC_isPlaying;
}

int AAC_Stop(){
    AAC_isPlaying = 0;
    AAC_threadActive = 0;
	sceKernelWaitThreadEnd(AAC_thid, NULL);
    sceKernelDeleteThread(AAC_thid);
    return 0;
}

int AAC_EndOfStream(){
    return AAC_eof;
}

void AAC_End(){
    AAC_Stop();
}


struct fileInfo *AAC_GetInfo(){
    return &AAC_info;
}


float AAC_GetPercentage(){
	//Calcolo posizione in %:
	float perc = 0.0f;

    if (AAC_filesize > 0){
        perc = ((float)AAC_filePos - (float)AAC_tagsize) / ((float)AAC_filesize - (float)AAC_tagsize) * 100.0;
        if (perc > 100)
            perc = 100;
    }else{
        perc = 0;
    }
    return(perc);
}


int AAC_getPlayingSpeed(){
	return AAC_playingSpeed;
}


int AAC_setPlayingSpeed(int playingSpeed){
	if (playingSpeed >= MIN_PLAYING_SPEED && playingSpeed <= MAX_PLAYING_SPEED){
		AAC_playingSpeed = playingSpeed;
		if (playingSpeed == 0)
			AAC_volume = PSP_AUDIO_VOLUME_MAX;
		else
			AAC_volume = FASTFORWARD_VOLUME;
		return 0;
	}else{
		return -1;
	}
}


void AAC_setVolumeBoost(int boost){
    AAC_volume_boost = boost;
}


int AAC_getVolumeBoost(){
    return(AAC_volume_boost);
}

void AAC_GetTimeString(char *dest){
    char timeString[9];
    int secs = (int)AAC_playingTime;
    int hh = secs / 3600;
    int mm = (secs - hh * 3600) / 60;
    int ss = secs - hh * 3600 - mm * 60;
    snprintf(timeString, sizeof(timeString), "%2.2i:%2.2i:%2.2i", hh, mm, ss);
    strcpy(dest, timeString);
}

struct fileInfo AAC_GetTagInfoOnly(char *filename){
    struct fileInfo tempInfo;
    initFileInfo(&tempInfo);
    getAACTagInfo(filename, &tempInfo);
	return tempInfo;
}

int AAC_isFilterSupported(){
	return 0;
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//Set mute:
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
int AAC_setMute(int onOff){
	if (onOff)
    	AAC_volume = MUTED_VOLUME;
	else
    	AAC_volume = PSP_AUDIO_VOLUME_MAX;
    return 0;
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//Fade out:
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void AAC_fadeOut(float seconds){
    int i = 0;
    long timeToWait = (long)((seconds * 1000.0) / (float)AAC_volume);
    for (i=AAC_volume; i>=0; i--){
        AAC_volume = i;
        sceKernelDelayThread(timeToWait);
    }
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//Manage suspend:
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
int AAC_suspend(){
    AAC_suspendPosition = AAC_filePos;
    AAC_suspendIsPlaying = AAC_isPlaying;

	AAC_isPlaying = 0;
    sceIoClose(AAC_handle);
    AAC_handle = -1;
    return 0;
}

int AAC_resume(){
	if (AAC_suspendPosition >= 0){
		AAC_handle = sceIoOpen(AAC_fileName, PSP_O_RDONLY, 0777);
		if (AAC_handle >= 0){
			AAC_filePos = AAC_suspendPosition;
			sceIoLseek32(AAC_handle, AAC_filePos, PSP_SEEK_SET);
			AAC_isPlaying = AAC_suspendIsPlaying;
		}
	}
	AAC_suspendPosition = -1;
    return 0;
}

void AAC_setVolumeBoostType(char *boostType){
    //Only old method supported
    MAX_VOLUME_BOOST = 4;
    //MAX_VOLUME_BOOST = 0;
    MIN_VOLUME_BOOST = 0;
}

double AAC_getFilePosition()
{
    return AAC_filePos;
}

void AAC_setFilePosition(double position)
{
    AAC_newFilePos = position;
}

//TODO:

int AAC_GetStatus(){
    return 0;
}


int AAC_setFilter(double tFilter[32], int copyFilter){
    return 0;
}

void AAC_enableFilter(){
}

void AAC_disableFilter(){
}

int AAC_isFilterEnabled(){
    return 0;
}

