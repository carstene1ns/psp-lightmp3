TARGET = lightmp3
OBJS = prx/supportlib.o \
       players/m3u.o players/pspaudiolib.o players/id3.o players/mp3player.o players/oggplayer.o players/aacplayer.o \
       players/mp3playerME.o players/aa3playerME.o players/flacplayer.o players/player.o players/equalizer.o \
       system/clock.o system/freeMem.o system/mem64.o system/opendir.o \
       system/exception.o system/usb.o system/osk.o \
       others/audioscrobbler.o others/medialibrary.o others/log.o others/strreplace.o \
       gui/settings.o gui/languages.o gui/menu.o gui/common.o \
       gui/gui_fileBrowser.o gui/gui_player.o gui/gui_playlistsBrowser.o \
       gui/gui_playlistEditor.o gui/gui_settings.o gui/gui_mediaLibrary.o \
       main.o

#To build for custom firmware:
BUILD_PRX = 1
PSP_FW_VERSION=371

#To debug -g
CFLAGS = -O3 -frename-registers -ffast-math -fomit-frame-pointer -G0 -Wall
#CFLAGS = -O3 -g -Wall
CXXFLAGS = $(CFLAGS) -fno-exceptions -fno-rtti
ASFLAGS = $(CFLAGS)
LIBDIR =

#LIBS for ogg-vorbis: -lvorbisfile -lvorbis -logg
#LIBS for libtremor : -lvorbisidec

LIBS= -lsqlite3 -losl -lpng -lz \
      -laac -lFLAC -lmad -lvorbisidec \
      -lpsphprm -lpspusb -lpspusbstor -lpspaudio -lpspaudiocodec -lpspkubridge \
	  -lpspsdk -lpspctrl -lpspumd -lpsprtc -lpsppower -lpspgu -lpspaudiolib -lpspaudio -lm

LDFLAGS =
EXTRA_TARGETS = EBOOT.PBP
PSP_EBOOT_TITLE = LightMP3
PSP_EBOOT_ICON = ICON0.PNG
PSPSDK=$(shell psp-config --pspsdk-path)
include $(PSPSDK)/lib/build.mak
