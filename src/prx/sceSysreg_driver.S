	.set noreorder

#include "pspimport.s"

	IMPORT_START "sceSysreg_driver",0x00010000
    	IMPORT_FUNC	"sceSysreg_driver",0x158AD4FC,sceSysregMeBusClockDisable
    	IMPORT_FUNC	"sceSysreg_driver",0x07881A0B,sceSysregMeBusClockDisable371
	
