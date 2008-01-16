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
//    CREDITS:
//    This file contains functions taken from PSP Doom V.1.4 by Chilly Willy
#include <pspkernel.h>
#include <pspctrl.h>
#include <pspdebug.h>
#include <pspsdk.h>
#include <pspdisplay.h>
#include <pspgu.h>
#include <pspgum.h>
#include <psputility.h>
#include <psputility_osk.h>
#include <stdio.h>
#include <string.h>
#include "osk.h"

/*
 * OSK support
 *
 */

#define BUF_WIDTH (512)
#define SCR_WIDTH (480)
#define SCR_HEIGHT (272)
#define PIXEL_SIZE (4) /* change this if you change to another screenmode */
#define FRAME_SIZE (BUF_WIDTH * SCR_HEIGHT * PIXEL_SIZE)
#define ZBUF_SIZE (BUF_WIDTH SCR_HEIGHT * 2) /* zbuffer seems to be 16-bit? */
static unsigned int __attribute__((aligned(16))) list[262144];

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// setupGu and resetGU:
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
static void setupGu(void){
	sceGuInit();
	sceGuStart(GU_DIRECT,list);
	sceGuDrawBuffer(GU_PSM_8888,(void*)0,BUF_WIDTH);
	sceGuDispBuffer(SCR_WIDTH,SCR_HEIGHT,(void*)0x88000,BUF_WIDTH);
	sceGuDepthBuffer((void*)0x110000,BUF_WIDTH);
	sceGuOffset(2048 - (SCR_WIDTH/2),2048 - (SCR_HEIGHT/2));
	sceGuViewport(2048,2048,SCR_WIDTH,SCR_HEIGHT);
	sceGuDepthRange(0xc350,0x2710);
	sceGuScissor(0,0,SCR_WIDTH,SCR_HEIGHT);
	sceGuEnable(GU_SCISSOR_TEST);
	sceGuDepthFunc(GU_GEQUAL);
	sceGuEnable(GU_DEPTH_TEST);
	sceGuFrontFace(GU_CW);
	sceGuShadeModel(GU_SMOOTH);
	sceGuEnable(GU_CULL_FACE);
	sceGuEnable(GU_CLIP_PLANES);
	sceGuFinish();
	sceGuSync(0,0);
	sceDisplayWaitVblankStart();
	sceGuDisplay(GU_TRUE);
}

static void resetGu(void){
	sceGuInit();
	sceGuStart(GU_DIRECT, list);
	sceGuDrawBuffer(GU_PSM_8888, (void*)0, BUF_WIDTH);
	sceGuDispBuffer(SCR_WIDTH, SCR_HEIGHT, (void*)0x88000, BUF_WIDTH);
	sceGuDepthBuffer((void*)0x110000, BUF_WIDTH);
	sceGuOffset(2048 - (SCR_WIDTH/2), 2048 - (SCR_HEIGHT/2));
	sceGuViewport(2048, 2048, SCR_WIDTH, SCR_HEIGHT);
	sceGuDepthRange(65535, 0);
	sceGuScissor(0, 0, SCR_WIDTH, SCR_HEIGHT);
	sceGuEnable(GU_SCISSOR_TEST);
	sceGuDepthFunc(GU_GEQUAL);
	sceGuEnable(GU_DEPTH_TEST);
	sceGuFrontFace(GU_CW);
	sceGuShadeModel(GU_SMOOTH);
	sceGuEnable(GU_CULL_FACE);
	sceGuEnable(GU_CLIP_PLANES);
	sceGuEnable(GU_BLEND);
	sceGuBlendFunc(GU_ADD, GU_SRC_ALPHA, GU_ONE_MINUS_SRC_ALPHA, 0, 0);
	sceGuFinish();
	sceGuSync(0,0);
	sceDisplayWaitVblankStart();
	sceGuDisplay(GU_TRUE);
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Get text from OSK:
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
int get_text_osk(char *input, unsigned short *intext, unsigned short *desc){
	int done=0;
	unsigned short outtext[128] = { 0 }; // text after input

	setupGu();

	SceUtilityOskData data;
	memset(&data, 0, sizeof(data));
	data.language = 2;			// key glyphs: 0-1=hiragana, 2+=western/whatever the other field says
	data.lines = 1;				// just one line
	data.unk_24 = 1;			// set to 1
	data.desc = desc;
	data.intext = intext;
	data.outtextlength = 128;	// sizeof(outtext) / sizeof(unsigned short)
	data.outtextlimit = 50;		// just allow 50 chars
	data.outtext = (unsigned short*)outtext;

	SceUtilityOskParams osk;
	memset(&osk, 0, sizeof(osk));
	osk.base.size = sizeof(osk);
	// dialog language: 0=Japanese, 1=English, 2=French, 3=Spanish, 4=German,
	// 5=Italian, 6=Dutch, 7=Portuguese, 8=Russian, 9=Korean, 10-11=Chinese, 12+=default
	osk.base.language = 1;
	osk.base.buttonSwap = 1;		// X button: 1
	osk.base.graphicsThread = 17;	// gfx thread pri
	osk.base.unknown = 19;			// unknown thread pri (?)
	osk.base.fontThread = 18;
	osk.base.soundThread = 16;
	osk.unk_48 = 1;
	osk.data = &data;

	int rc = sceUtilityOskInitStart(&osk);
	if (rc) return 0;

	while(!done) {
		int i,j=0;

		sceGuStart(GU_DIRECT,list);

		// clear screen
		sceGuClearColor(0);
		sceGuClearDepth(0);
		sceGuClear(GU_COLOR_BUFFER_BIT|GU_DEPTH_BUFFER_BIT);

		sceGuFinish();
		sceGuSync(0,0);

		switch(sceUtilityOskGetStatus()){
			case PSP_UTILITY_DIALOG_INIT :
			break;
			case PSP_UTILITY_DIALOG_VISIBLE :
			sceUtilityOskUpdate(2); // 2 is taken from ps2dev.org recommendation
			break;
			case PSP_UTILITY_DIALOG_QUIT :
			sceUtilityOskShutdownStart();
			break;
			case PSP_UTILITY_DIALOG_FINISHED :
			done = 1;
			break;
			case PSP_UTILITY_DIALOG_NONE :
			default :
			break;
		}

		for(i = 0; data.outtext[i]; i++)
			if (data.outtext[i]!='\0' && data.outtext[i]!='\n' && data.outtext[i]!='\r'){
				input[j] = data.outtext[i];
				j++;
			}
		input[j] = 0;

		// wait TWO vblanks because one makes the input "twitchy"
		sceDisplayWaitVblankStart();
		sceDisplayWaitVblankStart();
		sceGuSwapBuffers();
	}

	resetGu();

	return 1;
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// requestString:
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
char *requestString (char *descStr, char *initialStr){
	int ok, i;
	static char str[64];
	unsigned short intext[128]  = { 0 }; // text already in the edit box on start
	unsigned short desc[128]  = { 0 };

	if (initialStr[0] != 0)
		for (i=0; i<=strlen(initialStr); i++)
			intext[i] = (unsigned short)initialStr[i];

	if (descStr[0] != 0)
		for (i=0; i<=strlen(descStr); i++)
			desc[i] = (unsigned short)descStr[i];
			
	ok = get_text_osk(str, intext, desc);

	pspDebugScreenInit();
	pspDebugScreenSetBackColor(0xFF000000);
	pspDebugScreenSetTextColor(0xFFFFFFFF);
	pspDebugScreenClear();

	if (ok)
		return str;

	return 0;
}
