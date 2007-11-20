	.set noreorder

#include "pspimport.s"

	IMPORT_START "scePower_Service",0x00010000
	IMPORT_FUNC	"scePower_Service",0xB8D7B3FB,scePowerSetBusClockFrequency
	IMPORT_FUNC	"scePower_Service",0xAF70529A,scePowerSetBusClockFrequency371
	
	IMPORT_FUNC	"scePower_Service",0xB8D7B3FB,scePowerSetBusClockFrequency
	IMPORT_FUNC	"scePower_Service",0xAF70529A,scePowerSetBusClockFrequency371
