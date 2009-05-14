/*
 * PSP Software Development Kit - http://www.pspdev.org
 * -----------------------------------------------------------------------
 * Licensed under the BSD license, see LICENSE in PSPSDK root for details.
 *
 * pspasfparser.h - Prototypes for the sceAsfParser library.
 *
 * Copyright (c) 2009 Hrimfaxi <outmatch@gmail.com>
 * Copyright (c) 2009 cooleyes <eyes.cooleyes@gmail.com>
 *
 */
#ifndef __SCELIBASFPARSER_H__
#define __SCELIBASFPARSER_H__

#include <psptypes.h>

#ifdef __cplusplus
extern "C" {
#endif

/** Asf Frame structure */
typedef struct SceAsfFrame {
	ScePVoid  pData;		// frame data buffer, alloc by yourself
	SceUInt32 iFrameMs; 		// current frame ms
	SceUInt32 iUnk2;		// 0~7
	SceUInt32 iUnk3;		// maybe frame data size
	SceUInt32 iUnk4;		// first frame is 1, other is 0
	SceUInt32 iUnk5;		// 128 256 512 1024 2048
	SceUInt32 iUnk6;		// 128 256 512 1024 2048
	SceUInt32 iUnk7;		// unknow 
} SceAsfFrame;

/** Asf Parser structure */
typedef struct SceAsfParser {
	SceUInt32 iUnk0;
	SceUInt32 iUnk1;
	SceUInt32 iUnk2;
	SceUInt32 iUnk3;
	SceUInt32 iUnk4;
	SceUInt32 iUnk5;
	SceUInt32 iUnk6;
	SceUInt32 iUnk7;
	SceUInt32 iNeedMem; //8
	ScePVoid  pNeedMemBuffer; //9
	SceUInt32 iUnk10_19[10]; //10-19
	SceUInt64 lDuration; //20,21
	SceUInt32 iUnk22_3337[3316]; // 22 - 3337
	SceAsfFrame sFrame; //3338 - 3345
	SceUInt32 iUnk3346_3355[10]; //3346 - 3355
	SceUInt32 iUnk3356;
	SceUInt32 iUnk3357_4095[739];
} SceAsfParser;

/** Asf read callback */
typedef int (*PSPAsfReadCallback)(void *userdata, void *buf, SceSize size);

/** Asf seek callback */
typedef SceOff (*PSPAsfSeekCallback)(void *userdata, void *buf, SceOff offset, int whence);

/**
 * sceAsfCheckNeedMem
 *
 * @param parser - pointer to a SceAsfParser struct
 *
 * @return < 0 if error else if (parser->iNeedMem > 0x8000) also error.
 */
int sceAsfCheckNeedMem(SceAsfParser* parser);

/**
 * sceAsfInitParser
 *
 * @param parser - pointer to a SceAsfParser struct
 * @param unknown - unknown pointer, can be NULL
 * @param read_cb - read callback
 * @param seek_cb - seek callback
 *
 * @return < 0 if error else success.
 */
int sceAsfInitParser(SceAsfParser* parser, ScePVoid unknown, PSPAsfReadCallback read_cb, PSPAsfSeekCallback seek_cb);

/**
 * sceAsfGetFrameData
 *
 * @param parser - pointer to a SceAsfParser struct
 * @param unknown - unknown, set to 1
 * @param frame - pointer to a SceAsfFrame struct
 *
 * @return < 0 if error else success.
 */
int sceAsfGetFrameData(SceAsfParser* parser, int unknown, SceAsfFrame* frame);

/**
 * sceAsfSeekTime
 *
 * @param parser - pointer to a SceAsfParser struct
 * @param unknown - unknown, set to 1
 * @param ms - will contain value of seek ms
 *
 * @return < 0 if error else success.
 */
int sceAsfSeekTime(SceAsfParser* parser, int unknown, int *ms);

#ifdef __cplusplus
}
#endif

#endif
