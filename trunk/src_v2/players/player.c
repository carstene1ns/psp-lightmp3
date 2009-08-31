//    player.c
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
#include <pspkernel.h>
#include <pspsdk.h>
#include <string.h>
#include <stdio.h>
#include <psputility_avmodules.h>
#include <pspaudio.h>
#include "player.h"
#include "../system/opendir.h"
#include "cooleyesBridge.h"
#include "libasfparser/pspasfparser.h"

//shared global vars
char audioCurrentDir[264];
char fileTypeDescription[6][20] = {"MP3", "OGG Vorbis", "ATRAC3+", "FLAC", "AAC", "WMA"};
int MUTED_VOLUME = 800;
int MAX_VOLUME_BOOST=15;
int MIN_VOLUME_BOOST=-15;
int MIN_PLAYING_SPEED=-119;
int MAX_PLAYING_SPEED=119;
int currentVolume = 0;
int CLOCK_WHEN_PAUSE = 0;

//shared global vars for ME
static int HW_ModulesInit = 0;
u16 data_align;
u32 sample_per_frame;
u16 channel_mode;
u32 samplerate;
long data_start;
long data_size;
u8 getEDRAM;
u32 channels;
SceUID data_memid;
volatile int OutputBuffer_flip;
//shared between at3+aa3
u16 at3_type;
u8* at3_data_buffer;
u8 at3_at3plus_flagdata[2];
short  AT3_OutputBuffer[2][AT3_OUTPUT_BUFFER_SIZE]__attribute__((aligned(64))),
      *AT3_OutputPtr=AT3_OutputBuffer[0];

//Pointers for functions:
void (*initFunct)(int);
int (*loadFunct)(char *);
int (*playFunct)();
void (*pauseFunct)();
void (*endFunct)();
void (*setVolumeBoostTypeFunct)(char*);
void (*setVolumeBoostFunct)(int);
struct fileInfo *(*getInfoFunct)();
struct fileInfo (*getTagInfoFunct)();
void (*getTimeStringFunct)();
float (*getPercentageFunct)();
int (*getPlayingSpeedFunct)();
int (*setPlayingSpeedFunct)(int);
int (*endOfStreamFunct)();

int (*setMuteFunct)(int);
int (*setFilterFunct)(double[32], int copyFilter);
void (*enableFilterFunct)();
void (*disableFilterFunct)();
int (*isFilterEnabledFunct)();
int (*isFilterSupportedFunct)();
int (*suspendFunct)();
int (*resumeFunct)();
void (*fadeOutFunct)(float seconds);

double (*getFilePositionFunct)();
void (*setFilePositionFunct)(double positionInSecs);

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Functions for ME
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//Open audio for player:
int openAudio(int channel, int samplecount){
	int audio_channel = sceAudioChReserve(channel, samplecount, PSP_AUDIO_FORMAT_STEREO );
    if(audio_channel < 0)
        audio_channel = sceAudioChReserve(PSP_AUDIO_NEXT_CHANNEL, samplecount, PSP_AUDIO_FORMAT_STEREO );
	return audio_channel;
}

//Load a module:
SceUID LoadStartAudioModule(char *modname, int partition){
    SceKernelLMOption option;
    SceUID modid;

    memset(&option, 0, sizeof(option));
    option.size = sizeof(option);
    option.mpidtext = partition;
    option.mpiddata = partition;
    option.position = 0;
    option.access = 1;

    modid = sceKernelLoadModule(modname, 0, &option);
    if (modid < 0)
        return modid;

    return sceKernelStartModule(modid, 0, NULL, NULL, NULL);
}

//Load and start needed modules:
int initMEAudioModules(){
   if (!HW_ModulesInit){
       if (sceKernelDevkitVersion() == 0x01050001){
           LoadStartAudioModule("flash0:/kd/me_for_vsh.prx", PSP_MEMORY_PARTITION_KERNEL);
           LoadStartAudioModule("flash0:/kd/audiocodec.prx", PSP_MEMORY_PARTITION_KERNEL);
       }else{
           sceUtilityLoadAvModule(PSP_AV_MODULE_AVCODEC);
		   sceUtilityLoadAvModule(PSP_AV_MODULE_ATRAC3PLUS);

            int devkitVersion = sceKernelDevkitVersion();
            SceUID modid = -1;

            modid = pspSdkLoadStartModule("flash0:/kd/libasfparser.prx", PSP_MEMORY_PARTITION_USER);
            if (modid < 0)
                pspDebugScreenPrintf("Error loading libasfparser.prx\n");

            modid = pspSdkLoadStartModule("cooleyesBridge.prx", PSP_MEMORY_PARTITION_KERNEL);
            if (modid < 0)
                pspDebugScreenPrintf("Error loading cooleyesBridge.prx\n");

            cooleyesMeBootStart(devkitVersion, 3);
	   }

       HW_ModulesInit = 1;
   }
   return 0;
}

int GetID3TagSize(char *fname)
{
    SceUID fd;
    char header[10];
    int size = 0;
    fd = sceIoOpen(fname, PSP_O_RDONLY, 0777);
    if (fd < 0)
        return 0;

    sceIoRead(fd, header, sizeof(header));
    sceIoClose(fd);

    if (!strncmp((char*)header, "ea3", 3) || !strncmp((char*)header, "EA3", 3)
      ||!strncmp((char*)header, "ID3", 3))
    {
        //get the real size from the syncsafe int
        size = header[6];
        size = (size<<7) | header[7];
        size = (size<<7) | header[8];
        size = (size<<7) | header[9];

        size += 10;

        if (header[5] & 0x10) //has footer
            size += 10;
         return size;
    }
    return 0;
}

char GetOMGFileType(char *fname)
{
    SceUID fd;
    int size;
    char ea3_header[0x60];

    size = GetID3TagSize(fname);

    fd = sceIoOpen(fname, PSP_O_RDONLY, 0777);
    if (fd < 0)
        return UNK_TYPE;

    sceIoLseek32(fd, size, PSP_SEEK_SET);

    if (sceIoRead(fd, ea3_header, 0x60) != 0x60){
        sceIoClose(fd);
        return UNK_TYPE;
    }

    sceIoClose(fd);

    if (strncmp(ea3_header, "EA3", 3) != 0){
        return UNK_TYPE;
    }

    switch (ea3_header[3])
    {
        case 1:
        case 3:
            return AT3_TYPE;
            break;
        case 2:
            return MP3_TYPE;
            break;
        default:
            return UNK_TYPE;
            break;
    }
}


//Seek next valid frame
//NOTE: this function comes from Music prx 0.55 source
//      all credits goes to joek2100.
int SeekNextFrameMP3(SceUID fd)
{
    int offset = 0;
    unsigned char buf[1024];
    unsigned char *pBuffer;
    int i;
    int size = 0;

    offset = sceIoLseek32(fd, 0, PSP_SEEK_CUR);
    sceIoRead(fd, buf, sizeof(buf));
    if (!strncmp((char*)buf, "ID3", 3) || !strncmp((char*)buf, "ea3", 3)) //skip past id3v2 header, which can cause a false sync to be found
    {
        //get the real size from the syncsafe int
        size = buf[6];
        size = (size<<7) | buf[7];
        size = (size<<7) | buf[8];
        size = (size<<7) | buf[9];

        size += 10;

        if (buf[5] & 0x10) //has footer
            size += 10;
    }

    sceIoLseek32(fd, offset, PSP_SEEK_SET); //now seek for a sync
    while(1)
    {
        offset = sceIoLseek32(fd, 0, PSP_SEEK_CUR);
        size = sceIoRead(fd, buf, sizeof(buf));

        if (size <= 2)//at end of file
            return -1;

        if (!strncmp((char*)buf, "EA3", 3))//oma mp3 files have non-safe ints in the EA3 header
        {
            sceIoLseek32(fd, (buf[4]<<8)+buf[5], PSP_SEEK_CUR);
            continue;
        }

        pBuffer = buf;
        for( i = 0; i < size; i++)
        {
            //if this is a valid frame sync (0xe0 is for mpeg version 2.5,2+1)
            if ( (pBuffer[i] == 0xff) && ((pBuffer[i+1] & 0xE0) == 0xE0))
            {
                offset += i;
                sceIoLseek32(fd, offset, PSP_SEEK_SET);
                return offset;
            }
        }
       //go back two bytes to catch any syncs that on the boundary
        sceIoLseek32(fd, -2, PSP_SEEK_CUR);
    }
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//Set pointer to audio functions based on filename:
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
int setAudioFunctions(char *filename, int useME_MP3){
	char ext[6] = "";
	getExtension(filename, ext, 4);

	if (!stricmp(ext, "OGG")){
        //OGG Vorbis
		initFunct = OGG_Init;
		loadFunct = OGG_Load;
		playFunct = OGG_Play;
		pauseFunct = OGG_Pause;
		endFunct = OGG_End;
        setVolumeBoostTypeFunct = OGG_setVolumeBoostType;
        setVolumeBoostFunct = OGG_setVolumeBoost;
        getInfoFunct = OGG_GetInfo;
        getTagInfoFunct = OGG_GetTagInfoOnly;
        getTimeStringFunct = OGG_GetTimeString;
        getPercentageFunct = OGG_GetPercentage;
        getPlayingSpeedFunct = OGG_getPlayingSpeed;
        setPlayingSpeedFunct = OGG_setPlayingSpeed;
        endOfStreamFunct = OGG_EndOfStream;

        setMuteFunct = OGG_setMute;
        setFilterFunct = OGG_setFilter;
        enableFilterFunct = OGG_enableFilter;
        disableFilterFunct = OGG_disableFilter;
        isFilterEnabledFunct = OGG_isFilterEnabled;
        isFilterSupportedFunct = OGG_isFilterSupported;

        suspendFunct = OGG_suspend;
        resumeFunct = OGG_resume;
        fadeOutFunct = OGG_fadeOut;

        getFilePositionFunct = OGG_getFilePosition;
        setFilePositionFunct = OGG_setFilePosition;
		return 0;
    } else if (!stricmp(ext, "MP3")){
		if (useME_MP3)
		{
			//MP3 via Media Engine
			initFunct = MP3ME_Init;
			loadFunct = MP3ME_Load;
			playFunct = MP3ME_Play;
			pauseFunct = MP3ME_Pause;
			endFunct = MP3ME_End;
			setVolumeBoostTypeFunct = MP3ME_setVolumeBoostType;
			setVolumeBoostFunct = MP3ME_setVolumeBoost;
			getInfoFunct = MP3ME_GetInfo;
			getTagInfoFunct = MP3ME_GetTagInfoOnly;
			getTimeStringFunct = MP3ME_GetTimeString;
			getPercentageFunct = MP3ME_GetPercentage;
			getPlayingSpeedFunct = MP3ME_getPlayingSpeed;
			setPlayingSpeedFunct = MP3ME_setPlayingSpeed;
			endOfStreamFunct = MP3ME_EndOfStream;

			setMuteFunct = MP3ME_setMute;
			setFilterFunct = MP3ME_setFilter;
			enableFilterFunct = MP3ME_enableFilter;
			disableFilterFunct = MP3ME_disableFilter;
			isFilterEnabledFunct = MP3ME_isFilterEnabled;
			isFilterSupportedFunct = MP3ME_isFilterSupported;

			suspendFunct = MP3ME_suspend;
			resumeFunct = MP3ME_resume;
			fadeOutFunct = MP3ME_fadeOut;

            getFilePositionFunct = MP3ME_getFilePosition;
            setFilePositionFunct = MP3ME_setFilePosition;
		}
		else
		{
			//MP3 via LibMad
			initFunct = MP3_Init;
			loadFunct = MP3_Load;
			playFunct = MP3_Play;
			pauseFunct = MP3_Pause;
			endFunct = MP3_End;
			setVolumeBoostTypeFunct = MP3_setVolumeBoostType;
			setVolumeBoostFunct = MP3_setVolumeBoost;
			getInfoFunct = MP3_GetInfo;
			getTagInfoFunct = MP3_GetTagInfoOnly;
			getTimeStringFunct = MP3_GetTimeString;
			getPercentageFunct = MP3_GetPercentage;
			getPlayingSpeedFunct = MP3_getPlayingSpeed;
			setPlayingSpeedFunct = MP3_setPlayingSpeed;
			endOfStreamFunct = MP3_EndOfStream;

			setMuteFunct = MP3_setMute;
			setFilterFunct = MP3_setFilter;
			enableFilterFunct = MP3_enableFilter;
			disableFilterFunct = MP3_disableFilter;
			isFilterEnabledFunct = MP3_isFilterEnabled;
			isFilterSupportedFunct = MP3_isFilterSupported;

			suspendFunct = MP3_suspend;
			resumeFunct = MP3_resume;
			fadeOutFunct = MP3_fadeOut;

            getFilePositionFunct = MP3_getFilePosition;
            setFilePositionFunct = MP3_setFilePosition;
		}
		return 0;
    } else if (!stricmp(ext, "AA3") || !stricmp(ext, "OMA") || !stricmp(ext, "OMG")){
        //AA3
		initFunct = AA3ME_Init;
		loadFunct = AA3ME_Load;
		playFunct = AA3ME_Play;
		pauseFunct = AA3ME_Pause;
		endFunct = AA3ME_End;
        setVolumeBoostTypeFunct = AA3ME_setVolumeBoostType;
        setVolumeBoostFunct = AA3ME_setVolumeBoost;
        getInfoFunct = AA3ME_GetInfo;
        getTagInfoFunct = AA3ME_GetTagInfoOnly;
        getTimeStringFunct = AA3ME_GetTimeString;
        getPercentageFunct = AA3ME_GetPercentage;
        getPlayingSpeedFunct = AA3ME_getPlayingSpeed;
        setPlayingSpeedFunct = AA3ME_setPlayingSpeed;
        endOfStreamFunct = AA3ME_EndOfStream;

        setMuteFunct = AA3ME_setMute;
        setFilterFunct = AA3ME_setFilter;
        enableFilterFunct = AA3ME_enableFilter;
        disableFilterFunct = AA3ME_disableFilter;
        isFilterEnabledFunct = AA3ME_isFilterEnabled;
        isFilterSupportedFunct = AA3ME_isFilterSupported;

        suspendFunct = AA3ME_suspend;
        resumeFunct = AA3ME_resume;
        fadeOutFunct = AA3ME_fadeOut;

        getFilePositionFunct = AA3ME_getFilePosition;
        setFilePositionFunct = AA3ME_setFilePosition;
		return 0;
    } else if (!stricmp(ext, "FLAC") || !stricmp(ext, "FLA")){
        //FLAC
		initFunct = FLAC_Init;
		loadFunct = FLAC_Load;
		playFunct = FLAC_Play;
		pauseFunct = FLAC_Pause;
		endFunct = FLAC_End;
        setVolumeBoostTypeFunct = FLAC_setVolumeBoostType;
        setVolumeBoostFunct = FLAC_setVolumeBoost;
        getInfoFunct = FLAC_GetInfo;
        getTagInfoFunct = FLAC_GetTagInfoOnly;
        getTimeStringFunct = FLAC_GetTimeString;
        getPercentageFunct = FLAC_GetPercentage;
        getPlayingSpeedFunct = FLAC_getPlayingSpeed;
        setPlayingSpeedFunct = FLAC_setPlayingSpeed;
        endOfStreamFunct = FLAC_EndOfStream;

        setMuteFunct = FLAC_setMute;
        setFilterFunct = FLAC_setFilter;
        enableFilterFunct = FLAC_enableFilter;
        disableFilterFunct = FLAC_disableFilter;
        isFilterEnabledFunct = FLAC_isFilterEnabled;
        isFilterSupportedFunct = FLAC_isFilterSupported;

        suspendFunct = FLAC_suspend;
        resumeFunct = FLAC_resume;
        fadeOutFunct = FLAC_fadeOut;

        getFilePositionFunct = FLAC_getFilePosition;
        setFilePositionFunct = FLAC_setFilePosition;
		return 0;
    } else if (!stricmp(ext, "AAC") || !stricmp(ext, "M4A")){
        //AAC e AAC+ software
		initFunct = AAC_Init;
		loadFunct = AAC_Load;
		playFunct = AAC_Play;
		pauseFunct = AAC_Pause;
		endFunct = AAC_End;
        setVolumeBoostTypeFunct = AAC_setVolumeBoostType;
        setVolumeBoostFunct = AAC_setVolumeBoost;
        getInfoFunct = AAC_GetInfo;
        getTagInfoFunct = AAC_GetTagInfoOnly;
        getTimeStringFunct = AAC_GetTimeString;
        getPercentageFunct = AAC_GetPercentage;
        getPlayingSpeedFunct = AAC_getPlayingSpeed;
        setPlayingSpeedFunct = AAC_setPlayingSpeed;
        endOfStreamFunct = AAC_EndOfStream;

        setMuteFunct = AAC_setMute;
        setFilterFunct = AAC_setFilter;
        enableFilterFunct = AAC_enableFilter;
        disableFilterFunct = AAC_disableFilter;
        isFilterEnabledFunct = AAC_isFilterEnabled;
        isFilterSupportedFunct = AAC_isFilterSupported;

        suspendFunct = AAC_suspend;
        resumeFunct = AAC_resume;
        fadeOutFunct = AAC_fadeOut;

        getFilePositionFunct = AAC_getFilePosition;
        setFilePositionFunct = AAC_setFilePosition;
		return 0;
    } else if (!stricmp(ext, "WMA")){
        //WMA
		initFunct = WMA_Init;
		loadFunct = WMA_Load;
		playFunct = WMA_Play;
		pauseFunct = WMA_Pause;
		endFunct = WMA_End;
        setVolumeBoostTypeFunct = WMA_setVolumeBoostType;
        setVolumeBoostFunct = WMA_setVolumeBoost;
        getInfoFunct = WMA_GetInfo;
        getTagInfoFunct = WMA_GetTagInfoOnly;
        getTimeStringFunct = WMA_GetTimeString;
        getPercentageFunct = WMA_GetPercentage;
        getPlayingSpeedFunct = WMA_getPlayingSpeed;
        setPlayingSpeedFunct = WMA_setPlayingSpeed;
        endOfStreamFunct = WMA_EndOfStream;

        setMuteFunct = WMA_setMute;
        setFilterFunct = WMA_setFilter;
        enableFilterFunct = WMA_enableFilter;
        disableFilterFunct = WMA_disableFilter;
        isFilterEnabledFunct = WMA_isFilterEnabled;
        isFilterSupportedFunct = WMA_isFilterSupported;

        suspendFunct = WMA_suspend;
        resumeFunct = WMA_resume;
        fadeOutFunct = WMA_fadeOut;

        getFilePositionFunct = WMA_getFilePosition;
        setFilePositionFunct = WMA_setFilePosition;
		return 0;
    }
	return -1;
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//Unset pointer to audio functions:
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void unsetAudioFunctions(){
    initFunct = NULL;
    loadFunct = NULL;
    playFunct = NULL;
    pauseFunct = NULL;
    endFunct = NULL;
    setVolumeBoostTypeFunct = NULL;
    setVolumeBoostFunct = NULL;
    getInfoFunct = NULL;
    getTagInfoFunct = NULL;
    getTimeStringFunct = NULL;
    getPercentageFunct = NULL;
    getPlayingSpeedFunct = NULL;
    setPlayingSpeedFunct = NULL;
    endOfStreamFunct = NULL;

    setMuteFunct = NULL;
    setFilterFunct = NULL;
    enableFilterFunct = NULL;
    disableFilterFunct = NULL;
    isFilterEnabledFunct = NULL;
    isFilterSupportedFunct = NULL;

    suspendFunct = NULL;
    resumeFunct = NULL;

    getFilePositionFunct = NULL;
    setFilePositionFunct = NULL;
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//Volume boost for a single sample:
//OLD METHOD
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
short volume_boost(short *Sample, unsigned int *boost){
	int intSample = *Sample * (*boost + 1);
	if (intSample > 32767)
		return 32767;
	else if (intSample < -32768)
		return -32768;
	else
    	return intSample;
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//Set volume:
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
int setVolume(int channel, int volume){
    pspAudioSetVolume(channel, volume, volume);
    currentVolume = volume;
    return 0;
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//Set mute:
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
int setMute(int channel, int onOff){
	if (onOff)
        setVolume(channel, MUTED_VOLUME);
	else
        setVolume(channel, PSP_AUDIO_VOLUME_MAX);
	return 0;
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//Fade out:
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void fadeOut(int channel, float seconds){
    int i = 0;
    long timeToWait = (long)((seconds * 1000.0) / (float)currentVolume);
    for (i=currentVolume; i>=0; i--){
        pspAudioSetVolume(channel, i, i);
        sceKernelDelayThread(timeToWait);
    }
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//Set frequency for output:
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
int setAudioFrequency(unsigned short samples, unsigned short freq, char car){
	return sceAudioSRCChReserve(samples, freq, car);
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//Release audio:
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
int releaseAudio(void){
	while(sceAudioOutput2GetRestSample() > 0)
        sceKernelDelayThread(10);
	return sceAudioSRCChRelease();
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//Audio output:
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
int audioOutput(int volume, void *buffer){
	return sceAudioSRCOutputBlocking(volume, buffer);
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//Init pspaudiolib:
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
int initAudioLib(){
    pspAudioInit();
    return 0;
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//End pspaudiolib:
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
int endAudioLib(){
    pspAudioEnd();
    return 0;
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//Init a file info structure:
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void initFileInfo(struct fileInfo *info){
    info->fileType = -1;
    info->defaultCPUClock = 0;
    info->needsME = 0;
    info->fileSize = 0;
    strcpy(info->layer, "");
    info->kbit = 0;
    info->instantBitrate = 0;
    info->hz = 0;
    strcpy(info->mode, "");
    strcpy(info->emphasis, "");
    info->length = 0;
    strcpy(info->strLength, "");
    info->frames = 0;
    info->framesDecoded = 0;
    info->encapsulatedPictureType = 0;
    info->encapsulatedPictureOffset = 0;
    info->encapsulatedPictureLength = 0;

    strcpy(info->album, "");
    strcpy(info->title, "");
    strcpy(info->artist, "");
    strcpy(info->genre, "");
    strcpy(info->year, "");
    strcpy(info->trackNumber, "");
    strcpy(info->coverArtImageName, "");
}


///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//Get the cover art image name:
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void getCovertArtImageName(char *fileName, struct fileInfo *info){
    char dirName[264] = "";
    char buffer[264] = "";
    int size = 0;

    strcpy(info->coverArtImageName, "");
    strcpy(dirName, fileName);
    directoryUp(dirName);

    //Look for fileName.jpg in the same directory:
    snprintf(buffer, sizeof(buffer), "%s.jpg", fileName);
    size = fileExists(buffer);
    if (size > 0 && size <= MAX_IMAGE_DIMENSION){
        strcpy(info->coverArtImageName, buffer);
        return;
    }

    //Look for folder.jpg in the same directory:
    snprintf(buffer, sizeof(buffer), "%s/%s", dirName, "folder.jpg");
    size = fileExists(buffer);
    if (size > 0 && size <= MAX_IMAGE_DIMENSION){
        strcpy(info->coverArtImageName, buffer);
        return;
    }

    //Look for cover.jpg in same directory:
    snprintf(buffer, sizeof(buffer), "%s/%s", dirName, "cover.jpg");
    size = fileExists(buffer);
    if (size > 0 && size <= MAX_IMAGE_DIMENSION){
        strcpy(info->coverArtImageName, buffer);
        return;
    }

    //Look for albumName.jpg in the same directory:
    snprintf(buffer, sizeof(buffer), "%s/%s.jpg", dirName, info->album);
    size = fileExists(buffer);
    if (size > 0 && size <= MAX_IMAGE_DIMENSION){
        strcpy(info->coverArtImageName, buffer);
        return;
    }
}
