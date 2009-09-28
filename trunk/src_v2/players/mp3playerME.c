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
//    This file contains functions to play mp3 files through the PSP's Media Engine.
//    This code is based upon this sample code from ps2dev.org
//    http://forums.ps2dev.org/viewtopic.php?t=8469
//    and the source code of Music prx by joek2100
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <malloc.h>
#include <unistd.h>

#include "id3.h"
#include "mp3xing.h"
#include "player.h"
#include "mp3playerME.h"

#define THREAD_PRIORITY 12
#define OUTPUT_BUFFER_SIZE	(1152*4)

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//Globals:
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
static int MP3ME_threadActive = 0;
static char MP3ME_fileName[264];
static int MP3ME_isPlaying = 0;
static int MP3ME_thid = -1;
static int MP3ME_audio_channel = 0;
static int MP3ME_eof = 0;
static struct fileInfo MP3ME_info;
static int MP3ME_playingSpeed = 0; // 0 = normal
static unsigned int MP3ME_volume_boost = 0;
static float MP3ME_playingTime = 0;
static int MP3ME_volume = 0;
int MP3ME_defaultCPUClock = 20;
static double MP3ME_filePos = 0;
static double MP3ME_newFilePos = -1;
static int MP3ME_tagRead = 0;

//Globals for decoding:
static SceUID MP3ME_handle = -1;

static int samplerates[4][3] =
{
    {11025, 12000, 8000,},//mpeg 2.5
    {0, 0, 0,}, //reserved
    {22050, 24000, 16000,},//mpeg 2
    {44100, 48000, 32000}//mpeg 1
};
static int bitrates[] = {0, 32, 40, 48, 56, 64, 80, 96, 112, 128, 160, 192, 224, 256, 320 };
static int bitrates_v2[] = {0, 8, 16, 24, 32, 40, 48, 56, 64, 80, 96, 112, 128, 144, 160 };

static unsigned char MP3ME_input_buffer[2889]__attribute__((aligned(64)));//mp3 has the largest max frame, at3+ 352 is 2176
static unsigned long MP3ME_codec_buffer[65]__attribute__((aligned(64)));
static unsigned char MP3ME_output_buffer[2048*4]__attribute__((aligned(64)));//at3+ sample_per_frame*4
static short OutputBuffer[2][OUTPUT_BUFFER_SIZE];
static short *OutputPtrME = OutputBuffer[0];

static long MP3ME_suspendPosition = -1;
static long MP3ME_suspendIsPlaying = 0;
static double MP3ME_filesize = 0;
static double MP3ME_tagsize = 0;

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//Private functions:
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//Decode thread:
int decodeThread(SceSize args, void *argp){
    int res;
    unsigned char MP3ME_header_buf[4];
    int MP3ME_header;
    int version;
    int bitrate;
    int padding;
    int frame_size;
    int size;
    int total_size;
	int offset = 0;

	sceAudiocodecReleaseEDRAM(MP3ME_codec_buffer); //Fix: ReleaseEDRAM at the end is not enough to play another mp3.
	MP3ME_threadActive = 1;
    OutputBuffer_flip = 0;
    OutputPtrME = OutputBuffer[0];

    sceIoChdir(audioCurrentDir);
    MP3ME_handle = sceIoOpen(MP3ME_fileName, PSP_O_RDONLY, 0777);
    if (MP3ME_handle < 0)
        MP3ME_threadActive = 0;

	//now search for the first sync byte, tells us where the mp3 stream starts
	total_size = sceIoLseek32(MP3ME_handle, 0, PSP_SEEK_END);
	size = total_size;
	sceIoLseek32(MP3ME_handle, 0, PSP_SEEK_SET);
	data_start = ID3v2TagSize(MP3ME_fileName);
	sceIoLseek32(MP3ME_handle, data_start, PSP_SEEK_SET);
    data_start = SeekNextFrameMP3(MP3ME_handle);

	if (data_start < 0)
		MP3ME_threadActive = 0;

    size -= data_start;

    memset(MP3ME_codec_buffer, 0, sizeof(MP3ME_codec_buffer));
    memset(MP3ME_input_buffer, 0, sizeof(MP3ME_input_buffer));
    memset(MP3ME_output_buffer, 0, sizeof(MP3ME_output_buffer));

    if ( sceAudiocodecCheckNeedMem(MP3ME_codec_buffer, 0x1002) < 0 )
        MP3ME_threadActive = 0;

    if ( sceAudiocodecGetEDRAM(MP3ME_codec_buffer, 0x1002) < 0 )
        MP3ME_threadActive = 0;

    getEDRAM = 1;

    if ( sceAudiocodecInit(MP3ME_codec_buffer, 0x1002) < 0 )
        MP3ME_threadActive = 0;

    MP3ME_eof = 0;
	MP3ME_info.framesDecoded = 0;

	while (MP3ME_threadActive){
		while( !MP3ME_eof && MP3ME_isPlaying )
		{
            MP3ME_filePos = sceIoLseek32(MP3ME_handle, 0, PSP_SEEK_CUR);
			if ( sceIoRead( MP3ME_handle, MP3ME_header_buf, 4 ) != 4 ){
				MP3ME_isPlaying = 0;
				MP3ME_threadActive = 0;
				continue;
			}

			MP3ME_header = MP3ME_header_buf[0];
			MP3ME_header = (MP3ME_header<<8) | MP3ME_header_buf[1];
			MP3ME_header = (MP3ME_header<<8) | MP3ME_header_buf[2];
			MP3ME_header = (MP3ME_header<<8) | MP3ME_header_buf[3];

			bitrate = (MP3ME_header & 0xf000) >> 12;
			padding = (MP3ME_header & 0x200) >> 9;
			version = (MP3ME_header & 0x180000) >> 19;
			samplerate = samplerates[version][ (MP3ME_header & 0xC00) >> 10 ];

			if ((bitrate > 14) || (version == 1) || (samplerate == 0) || (bitrate == 0))//invalid frame, look for the next one
			{
				data_start = SeekNextFrameMP3(MP3ME_handle);
				if(data_start < 0)
				{
					MP3ME_eof = 1;
					continue;
				}
				size -= (data_start - offset);
				offset = data_start;
				continue;
			}

			if (version == 3) //mpeg-1
			{
				sample_per_frame = 1152;
				frame_size = 144000*bitrates[bitrate]/samplerate + padding;
				MP3ME_info.instantBitrate = bitrates[bitrate] * 1000;
			}else{
				sample_per_frame = 576;
				frame_size = 72000*bitrates_v2[bitrate]/samplerate + padding;
				MP3ME_info.instantBitrate = bitrates_v2[bitrate] * 1000;
			}

			sceIoLseek32(MP3ME_handle, data_start, PSP_SEEK_SET); //seek back

            if (MP3ME_newFilePos >= 0)
            {
                if (!MP3ME_newFilePos)
                    MP3ME_newFilePos = ID3v2TagSize(MP3ME_fileName);

                long old_start = data_start;
                if (sceIoLseek32(MP3ME_handle, MP3ME_newFilePos, PSP_SEEK_SET) != old_start){
                    data_start = SeekNextFrameMP3(MP3ME_handle);
                    if(data_start < 0){
                        MP3ME_eof = 1;
                    }
                    MP3ME_playingTime = (float)data_start / (float)frame_size /  (float)samplerate / 1000.0f;
                    offset = data_start;
                    size = total_size - data_start;
                }
                MP3ME_newFilePos = -1;
                continue;
            }

			size -= frame_size;
			if ( size <= 0)
			{
			   MP3ME_eof = 1;
			   continue;
			}

			//since we check for eof above, this can only happen when the file
			// handle has been invalidated by syspend/resume/usb
			if ( sceIoRead( MP3ME_handle, MP3ME_input_buffer, frame_size ) != frame_size ){
                //Resume from suspend:
                if ( MP3ME_handle >= 0 ){
                   sceIoClose(MP3ME_handle);
                   MP3ME_handle = -1;
                }
                MP3ME_handle = sceIoOpen(MP3ME_fileName, PSP_O_RDONLY, 0777);
                if (MP3ME_handle < 0){
                    MP3ME_isPlaying = 0;
                    MP3ME_threadActive = 0;
                    continue;
                }
                size = sceIoLseek32(MP3ME_handle, 0, PSP_SEEK_END);
                sceIoLseek32(MP3ME_handle, offset, PSP_SEEK_SET);
                data_start = offset;
				continue;
			}
			data_start += frame_size;
			offset = data_start;

			MP3ME_codec_buffer[6] = (unsigned long)MP3ME_input_buffer;
			MP3ME_codec_buffer[8] = (unsigned long)MP3ME_output_buffer;

			MP3ME_codec_buffer[7] = MP3ME_codec_buffer[10] = frame_size;
			MP3ME_codec_buffer[9] = sample_per_frame * 4;

			res = sceAudiocodecDecode(MP3ME_codec_buffer, 0x1002);

			if ( res < 0 )
			{
				//instead of quitting see if the next frame can be decoded
				//helps play files with an invalid frame
				//we must look for a valid frame, the offset above may be wrong
				data_start = SeekNextFrameMP3(MP3ME_handle);
				if(data_start < 0)
				{
					MP3ME_eof = 1;
					continue;
				}
				size -= (data_start - offset);
				offset = data_start;
				continue;
			}
            MP3ME_playingTime += (float)sample_per_frame/(float)samplerate;
		    MP3ME_info.framesDecoded++;

            //Output:
			memcpy( OutputPtrME, MP3ME_output_buffer, sample_per_frame*4);
			OutputPtrME += (sample_per_frame * 4);
			if( OutputPtrME + (sample_per_frame * 4) > &OutputBuffer[OutputBuffer_flip][OUTPUT_BUFFER_SIZE])
			{
				//Volume Boost:
				if (MP3ME_volume_boost){
                    int i;
                    for (i=0; i<OUTPUT_BUFFER_SIZE; i++){
    					OutputBuffer[OutputBuffer_flip][i] = volume_boost(&OutputBuffer[OutputBuffer_flip][i], &MP3ME_volume_boost);
                    }
                }

                audioOutput(MP3ME_volume, OutputBuffer[OutputBuffer_flip]);

				OutputBuffer_flip ^= 1;
				OutputPtrME = OutputBuffer[OutputBuffer_flip];
		        //Check for playing speed:
                if (MP3ME_playingSpeed){
                    long old_start = data_start;
                    if (sceIoLseek32(MP3ME_handle, frame_size * MP3ME_playingSpeed, PSP_SEEK_CUR) != old_start){
                        data_start = SeekNextFrameMP3(MP3ME_handle);
                        if(data_start < 0){
                            MP3ME_eof = 1;
                            continue;
                        }
                        float framesSkipped = (float)abs(old_start - data_start) / (float)frame_size;
                        if (MP3ME_playingSpeed > 0)
                            MP3ME_playingTime += framesSkipped * (float)sample_per_frame/(float)samplerate;
                        else
                            MP3ME_playingTime -= framesSkipped * (float)sample_per_frame/(float)samplerate;

                        offset = data_start;
                        size = total_size - data_start;
                    }else
                        MP3ME_setPlayingSpeed(0);
                }
			}
		}
		sceKernelDelayThread(10000);
	}
    if (getEDRAM)
        sceAudiocodecReleaseEDRAM(MP3ME_codec_buffer);

    if ( MP3ME_handle >= 0){
      sceIoClose(MP3ME_handle);
      MP3ME_handle = -1;
    }
	sceKernelExitThread(0);
    return 0;
}


void getMP3METagInfo(char *filename, struct fileInfo *targetInfo){
    //ID3:
    struct ID3Tag ID3;
    strcpy(MP3ME_fileName, filename);
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

    MP3ME_info = *targetInfo;
    MP3ME_tagRead = 1;
}

//Get info on file:
//Uso LibMad per calcolare la durata del pezzo perché
//altrimenti dovrei gestire il buffer anche nella seekNextFrame (senza è troppo lenta).
//E' una porcheria ma è più semplice. :)
int MP3MEgetInfo(){
	unsigned long FrameCount = 0;
    int fd = -1;
    int bufferSize = 1024*496;
    u8 *localBuffer;
    long singleDataRed = 0;
	struct mad_stream stream;
	struct mad_header header;
    int timeFromID3 = 0;
    float mediumBitrate = 0.0f;
    int has_xing = 0;
    struct xing xing;
	memset(&xing, 0, sizeof(xing));

    if (!MP3ME_tagRead)
        getMP3METagInfo(MP3ME_fileName, &MP3ME_info);

	mad_stream_init (&stream);
	mad_header_init (&header);

    fd = sceIoOpen(MP3ME_fileName, PSP_O_RDONLY, 0777);
    if (fd < 0)
        return -1;

	long size = sceIoLseek(fd, 0, PSP_SEEK_END);
    sceIoLseek(fd, 0, PSP_SEEK_SET);

    MP3ME_tagsize = ID3v2TagSize(MP3ME_fileName);
	double startPos = MP3ME_tagsize;
	sceIoLseek32(fd, startPos, PSP_SEEK_SET);

    //Check for xing frame:
	unsigned char *xing_buffer;
	xing_buffer = (unsigned char *)malloc(XING_BUFFER_SIZE);
	if (xing_buffer != NULL)
	{
        sceIoRead(fd, xing_buffer, XING_BUFFER_SIZE);
        if(parse_xing(xing_buffer, 0, &xing))
        {
            if (xing.flags & XING_FRAMES && xing.frames){
                has_xing = 1;
                bufferSize = 50 * 1024;
            }
        }
        free(xing_buffer);
        xing_buffer = NULL;
    }

    size -= startPos;

    if (size < bufferSize * 3)
        bufferSize = size;
    localBuffer = (unsigned char *) malloc(sizeof(unsigned char)  * bufferSize);
    unsigned char *buff = localBuffer;

    MP3ME_info.fileType = MP3_TYPE;
    MP3ME_info.defaultCPUClock = MP3ME_defaultCPUClock;
    MP3ME_info.needsME = 1;
	MP3ME_info.fileSize = size;
	MP3ME_filesize = size;
    MP3ME_info.framesDecoded = 0;

    double totalBitrate = 0;
    int i = 0;

	for (i=0; i<3; i++){
        memset(localBuffer, 0, bufferSize);
        singleDataRed = sceIoRead(fd, localBuffer, bufferSize);
    	mad_stream_buffer (&stream, localBuffer, singleDataRed);

        while (1){
    		if (mad_header_decode (&header, &stream) == -1){
                if (stream.buffer == NULL || stream.error == MAD_ERROR_BUFLEN)
                    break;
    			else if (MAD_RECOVERABLE(stream.error)){
    				continue;
    			}else{
    				break;
    			}
    		}
    		//Informazioni solo dal primo frame:
    	    if (FrameCount++ == 0){
    			switch (header.layer) {
    			case MAD_LAYER_I:
    				strcpy(MP3ME_info.layer,"I");
    				break;
    			case MAD_LAYER_II:
    				strcpy(MP3ME_info.layer,"II");
    				break;
    			case MAD_LAYER_III:
    				strcpy(MP3ME_info.layer,"III");
    				break;
    			default:
    				strcpy(MP3ME_info.layer,"unknown");
    				break;
    			}

    			MP3ME_info.kbit = header.bitrate / 1000;
    			MP3ME_info.instantBitrate = header.bitrate;
    			MP3ME_info.hz = header.samplerate;
    			switch (header.mode) {
    			case MAD_MODE_SINGLE_CHANNEL:
    				strcpy(MP3ME_info.mode, "single channel");
    				break;
    			case MAD_MODE_DUAL_CHANNEL:
    				strcpy(MP3ME_info.mode, "dual channel");
    				break;
    			case MAD_MODE_JOINT_STEREO:
    				strcpy(MP3ME_info.mode, "joint (MS/intensity) stereo");
    				break;
    			case MAD_MODE_STEREO:
    				strcpy(MP3ME_info.mode, "normal LR stereo");
    				break;
    			default:
    				strcpy(MP3ME_info.mode, "unknown");
    				break;
    			}

    			switch (header.emphasis) {
    			case MAD_EMPHASIS_NONE:
    				strcpy(MP3ME_info.emphasis,"no");
    				break;
    			case MAD_EMPHASIS_50_15_US:
    				strcpy(MP3ME_info.emphasis,"50/15 us");
    				break;
    			case MAD_EMPHASIS_CCITT_J_17:
    				strcpy(MP3ME_info.emphasis,"CCITT J.17");
    				break;
    			case MAD_EMPHASIS_RESERVED:
    				strcpy(MP3ME_info.emphasis,"reserved(!)");
    				break;
    			default:
    				strcpy(MP3ME_info.emphasis,"unknown");
    				break;
    			}

    			//Check if lenght found in tag info:
                if (MP3ME_info.length > 0){
                    timeFromID3 = 1;
                    break;
                }
                if (has_xing)
                    break;
            }

            totalBitrate += header.bitrate;
		}
        if (size == bufferSize)
            break;
        else if (i==0)
            sceIoLseek(fd, startPos + size/3, PSP_SEEK_SET);
        else if (i==1)
            sceIoLseek(fd, startPos + 2 * size/3, PSP_SEEK_SET);

        if (timeFromID3 || has_xing)
            break;
	}
	mad_header_finish (&header);
	mad_stream_finish (&stream);
    if (buff){
    	free(buff);
        buff = NULL;
    }
    sceIoClose(fd);

    int secs = 0;
    if (has_xing)
    {
        /* modify header.duration since we don't need it anymore */
        mad_timer_multiply(&header.duration, xing.frames);
        secs = mad_timer_count(header.duration, MAD_UNITS_SECONDS);
		MP3ME_info.length = secs;
	}
    else if (!MP3ME_info.length){
		mediumBitrate = totalBitrate / (float)FrameCount;
		secs = size * 8 / mediumBitrate;
        MP3ME_info.length = secs;
    }else{
        secs = MP3ME_info.length;
    }

	//Formatto in stringa la durata totale:
	int h = secs / 3600;
	int m = (secs - h * 3600) / 60;
	int s = secs - h * 3600 - m * 60;
	snprintf(MP3ME_info.strLength, sizeof(MP3ME_info.strLength), "%2.2i:%2.2i:%2.2i", h, m, s);

    return 0;
}


///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//Public functions:
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void MP3ME_Init(int channel){
    MP3ME_audio_channel = channel;
	MP3ME_playingSpeed = 0;
    MP3ME_playingTime = 0;
	MP3ME_volume_boost = 0;
	MP3ME_volume = PSP_AUDIO_VOLUME_MAX;
    MIN_PLAYING_SPEED=-119;
    MAX_PLAYING_SPEED=119;
	initMEAudioModules();
    initFileInfo(&MP3ME_info);
    MP3ME_tagRead = 0;
}


int MP3ME_Load(char *fileName){
    MP3ME_filePos = 0;
    MP3ME_playingSpeed = 0;
    MP3ME_isPlaying = 0;

    getcwd(audioCurrentDir, 256);

    //initFileInfo(&MP3ME_info);
    strcpy(MP3ME_fileName, fileName);
    if (MP3MEgetInfo() != 0){
        strcpy(MP3ME_fileName, "");
        return ERROR_OPENING;
    }

    releaseAudio();
    if (setAudioFrequency(OUTPUT_BUFFER_SIZE/4, MP3ME_info.hz, 2) < 0){
        MP3ME_End();
        return ERROR_INVALID_SAMPLE_RATE;
    }

    MP3ME_thid = -1;
    MP3ME_eof = 0;
    MP3ME_thid = sceKernelCreateThread("decodeThread", decodeThread, THREAD_PRIORITY, DEFAULT_THREAD_STACK_SIZE, PSP_THREAD_ATTR_USER, NULL);
    if(MP3ME_thid < 0)
        return ERROR_CREATE_THREAD;

    sceKernelStartThread(MP3ME_thid, 0, NULL);
    return OPENING_OK;
}


int MP3ME_Play(){
    if(MP3ME_thid < 0)
        return -1;
    MP3ME_isPlaying = 1;
    return 0;
}


void MP3ME_Pause(){
    MP3ME_isPlaying = !MP3ME_isPlaying;
}

int MP3ME_Stop(){
    MP3ME_isPlaying = 0;
    MP3ME_threadActive = 0;
	sceKernelWaitThreadEnd(MP3ME_thid, NULL);
    sceKernelDeleteThread(MP3ME_thid);
    return 0;
}

int MP3ME_EndOfStream(){
    return MP3ME_eof;
}

void MP3ME_End(){
    MP3ME_Stop();
}


struct fileInfo *MP3ME_GetInfo(){
    return &MP3ME_info;
}


float MP3ME_GetPercentage(){
	//Calcolo posizione in %:
	float perc = 0.0f;

    if (MP3ME_filesize > 0){
        perc = ((float)MP3ME_filePos - (float)MP3ME_tagsize) / ((float)MP3ME_filesize - (float)MP3ME_tagsize) * 100.0;
        if (perc > 100)
            perc = 100;
    }else{
        perc = 0;
    }
    return(perc);
}


int MP3ME_getPlayingSpeed(){
	return MP3ME_playingSpeed;
}


int MP3ME_setPlayingSpeed(int playingSpeed){
	if (playingSpeed >= MIN_PLAYING_SPEED && playingSpeed <= MAX_PLAYING_SPEED){
		MP3ME_playingSpeed = playingSpeed;
		if (playingSpeed == 0)
			MP3ME_volume = PSP_AUDIO_VOLUME_MAX;
		else
			MP3ME_volume = FASTFORWARD_VOLUME;
		return 0;
	}else{
		return -1;
	}
}


void MP3ME_setVolumeBoost(int boost){
    MP3ME_volume_boost = boost;
}


int MP3ME_getVolumeBoost(){
    return(MP3ME_volume_boost);
}

void MP3ME_GetTimeString(char *dest){
    char timeString[9];
    int secs = (int)MP3ME_playingTime;
    int hh = secs / 3600;
    int mm = (secs - hh * 3600) / 60;
    int ss = secs - hh * 3600 - mm * 60;
    snprintf(timeString, sizeof(timeString), "%2.2i:%2.2i:%2.2i", hh, mm, ss);
    strcpy(dest, timeString);
}

struct fileInfo MP3ME_GetTagInfoOnly(char *filename){
    struct fileInfo tempInfo;
    initFileInfo(&tempInfo);
    getMP3METagInfo(filename, &tempInfo);
	return tempInfo;
}

int MP3ME_isFilterSupported(){
	return 0;
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//Set mute:
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
int MP3ME_setMute(int onOff){
	if (onOff)
    	MP3ME_volume = MUTED_VOLUME;
	else
    	MP3ME_volume = PSP_AUDIO_VOLUME_MAX;
    return 0;
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//Fade out:
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void MP3ME_fadeOut(float seconds){
    int i = 0;
    long timeToWait = (long)((seconds * 1000.0) / (float)MP3ME_volume);
    for (i=MP3ME_volume; i>=0; i--){
        MP3ME_volume = i;
        sceKernelDelayThread(timeToWait);
    }
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//Manage suspend:
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
int MP3ME_suspend(){
    MP3ME_suspendPosition = MP3ME_filePos;
    MP3ME_suspendIsPlaying = MP3ME_isPlaying;

	MP3ME_isPlaying = 0;
    sceIoClose(MP3ME_handle);
    MP3ME_handle = -1;
    return 0;
}

int MP3ME_resume(){
	if (MP3ME_suspendPosition >= 0){
		MP3ME_handle = sceIoOpen(MP3ME_fileName, PSP_O_RDONLY, 0777);
		if (MP3ME_handle >= 0){
			MP3ME_filePos = MP3ME_suspendPosition;
			sceIoLseek32(MP3ME_handle, MP3ME_filePos, PSP_SEEK_SET);
			MP3ME_isPlaying = MP3ME_suspendIsPlaying;
		}
	}
	MP3ME_suspendPosition = -1;
    return 0;
}

void MP3ME_setVolumeBoostType(char *boostType){
    //Only old method supported
    MAX_VOLUME_BOOST = 4;
    //MAX_VOLUME_BOOST = 0;
    MIN_VOLUME_BOOST = 0;
}

double MP3ME_getFilePosition()
{
    return MP3ME_filePos;
}

void MP3ME_setFilePosition(double position)
{
    MP3ME_newFilePos = position;
}

//TODO:

int MP3ME_GetStatus(){
    return 0;
}


int MP3ME_setFilter(double tFilter[32], int copyFilter){
    return 0;
}

void MP3ME_enableFilter(){
}

void MP3ME_disableFilter(){
}

int MP3ME_isFilterEnabled(){
    return 0;
}

