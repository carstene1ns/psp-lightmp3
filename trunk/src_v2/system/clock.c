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
void setKernelBusClock(int bus);
void setKernelCpuClock(int cpu);


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
    if (bus < getMinBUSClock())
        bus = getMinBUSClock();

    if (bus >= getMinBUSClock() && bus <= 111 && sceKernelDevkitVersion() < 0x03070110){
        scePowerSetBusClockFrequency(bus);
		if (getBusClock() < bus)
			scePowerSetBusClockFrequency(++bus);
	}
}

void setCpuClock(int cpu){
    if (cpu < getMinCPUClock())
        cpu = getMinCPUClock();

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
// Velocità minima di clock in base al modello (FAT/SLIM):
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
int getMinCPUClock(){
    if (getModel() == PSP_MODEL_SLIM_AND_LITE || sceKernelDevkitVersion() >= 0x03070110)
        return 19;
    else
        return 10;
}


///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Velocità minima di bus in base al modello (FAT/SLIM):
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
int getMinBUSClock(){
    if (getModel() == PSP_MODEL_SLIM_AND_LITE || sceKernelDevkitVersion() >= 0x03070110)
        return 95;
    else
        return 54;
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


///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Boost per particolari operazioni:
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
#define BOOST_CPU 222
#define BOOST_BUS 111

static int oldBus = 0;
static int oldClock = 0;

int cpuBoost(){
    oldClock = getCpuClock();
    oldBus = getBusClock();
	if (BOOST_CPU <= 222){
		setCpuClock(BOOST_CPU);
		setBusClock(BOOST_BUS);
	}else{
		setKernelCpuClock(BOOST_CPU);
		setKernelBusClock(BOOST_BUS);
	}
	return 0;
}

int cpuRestore(){
    setBusClock(oldBus);
	setCpuClock(oldClock);
	oldClock = 0;
    oldBus = 0;
	return 0;
}
