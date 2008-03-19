TARGET = mp3
OBJS = prx/supportlib.o audioscrobbler.o settings.o system/log.o \
       players/equalizer.o players/m3u.o \
       players/pspaudiolib.o players/id3.o players/mp3player.o players/oggplayer.o players/aacplayer.o \
       players/mp3playerME.o players/aa3playerME.o players/flacplayer.o players/player.o \
       system/usb.o system/clock.o system/osk.o system/mem64.o system/opendir.o system/exception.o main.o

#To build for custom firmware:
BUILD_PRX = 1
#PSP_LARGE_MEMORY = 1
PSP_FW_VERSION=371

CFLAGS = -O3 -fomit-frame-pointer -ffast-math -frename-registers -G0 -Wall
CXXFLAGS = $(CFLAGS) -fno-exceptions -fno-rtti
ASFLAGS = $(CFLAGS)
LIBDIR =

LIBS = -laac -lFLAC -lmad -lvorbisidec -lm \
       -lpspgu -lpspgum -lpsppower \
       -lpsphprm -lpspusb -lpspusbstor -lpspaudio -lpspaudiocodec -lpspkubridge
LDFLAGS =
EXTRA_TARGETS = EBOOT.PBP
PSP_EBOOT_TITLE = LightMP3
PSP_EBOOT_ICON = ICON0.PNG
PSPSDK=$(shell psp-config --pspsdk-path)
include $(PSPSDK)/lib/build.mak
