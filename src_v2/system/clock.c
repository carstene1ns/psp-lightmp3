//    LightMP3
//    Copyright (C) 2007, 2008 Sakya
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
#include <psppower.h>
#include <pspsdk.h>
#include <kubridge.h>

#include "clock.h"

int getModelKernel();

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Funzioni gestione BUS & CLOCK
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
int getCpuClock(){
    return scePowerGetCpuClockFrequency();
}

int getBusClock(){
    return scePowerGetBusClockFrequency();
}

void setBusClock(int bus){
    if (bus >= 54 && bus <= 111 && sceKernelDevkitVersion() < 0x03070110){
        scePowerSetBusClockFrequency(bus);
		//if (getBusClock() < bus)
		//	scePowerSetBusClockFrequency(++bus);
	}
}

void setCpuClock(int cpu){
    if (cpu >= getMinCPUClock() && cpu <= 266){
        if (sceKernelDevkitVersion() < 0x03070110){
            scePowerSetCpuClockFrequency(cpu);
            if (scePowerGetCpuClockFrequency() < cpu)
                scePowerSetCpuClockFrequency(++cpu);
        }else{
            scePowerSetClockFrequency(cpu, cpu, cpu/2);
            if (scePowerGetCpuClockFrequency() < cpu){
                cpu++;
                scePowerSetClockFrequency(cpu, cpu, cpu/2);
            }
        }
    }
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Velocit� minima di clock in base al modello (FAT/SLIM):
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
int getMinCPUClock(){
    if (getModel() == PSP_MODEL_SLIM_AND_LITE || sceKernelDevkitVersion() >= 0x03070110)
        return 19;
    else
        return 10;
}


///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Restituisce il modello (FAT = 0/SLIM = 1):
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
int getModel(){
    if (sceKernelDevkitVersion() >= 0x03060010)
        return kuKernelGetModel();
    else
        if (getModelKernel() == 1)
            return PSP_MODEL_SLIM_AND_LITE;
        else
            return 0;
}
