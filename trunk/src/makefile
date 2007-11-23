TARGET = mp3
OBJS = prx/supportlib.o usb.o audioscrobbler.o settings.o equalizer.o log.o m3u.o \
       players/pspaudiolib.o players/id3.o players/mp3player.o players/oggplayer.o \
       players/mp3playerME.o players/aa3playerME.o players/player.o \
       mem64.o opendir.o main.o

#To build for custom firmware:
BUILD_PRX = 1
#PSP_LARGE_MEMORY = 1

CFLAGS = -O3 -fomit-frame-pointer -ffast-math -frename-registers -G0 -Wall
CXXFLAGS = $(CFLAGS) -fno-exceptions -fno-rtti
ASFLAGS = $(CFLAGS)
LIBDIR =

LIBS = -lm -lpspgu -lmad -lvorbisidec  -lpspaudio -lpsppower \
       -lpsphprm -lpspusb -lpspusbstor -lpspkubridge -lpspaudio -lpspaudiocodec
LDFLAGS =
EXTRA_TARGETS = EBOOT.PBP
PSP_EBOOT_TITLE = LightMP3
PSP_EBOOT_ICON = ICON0.PNG
PSPSDK=$(shell psp-config --pspsdk-path)
include $(PSPSDK)/lib/build.mak
