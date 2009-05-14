TARGET = lightmp3
OBJS = prx/supportlib.o \
       players/m3u.o players/pspaudiolib.o players/id3.o players/mp3xing.o players/mp3player.o players/oggplayer.o players/aacplayer.o \
       players/mp3playerME.o players/aa3playerME.o players/flacplayer.o players/player.o players/wmaplayerME.o players/equalizer.o \
       system/clock.o system/freeMem.o system/mem64.o miniconv/MiniConv.o system/fat.o system/opendir.o \
       system/exception.o system/brightness.o \
       others/bookmark.o others/audioscrobbler.o others/medialibrary.o others/log.o others/strreplace.o \
       gui/settings.o gui/skinsettings.o gui/languages.o gui/menu.o gui/common.o \
       gui/gui_fileBrowser.o gui/gui_player.o gui/gui_playlistsBrowser.o \
       gui/gui_playlistEditor.o gui/gui_settings.o gui/gui_mediaLibrary.o \
       cooleyesBridge.o main.o

#To build for custom firmware:
BUILD_PRX = 1
PSP_FW_VERSION=500
PSP_LARGE_MEMORY=1

#To debug -g
CFLAGS = -O3 -frename-registers -ffast-math -fomit-frame-pointer -g -G0 -Wall
#CFLAGS = -O2 -g -G0 -Wall
CXXFLAGS = $(CFLAGS) -fno-exceptions -fno-rtti
ASFLAGS = $(CFLAGS)
LIBDIR =

#LIBS for ogg-vorbis: -lvorbisfile -lvorbis -logg
#LIBS for libtremor : -lvorbisidec

LIBS= -lsqlite3 -losl -lpng -lz \
      -lAac -lFLAC -lmad -lvorbisidec \
      -lpsphprm -lpspusb -lpspusbstor -lpspaudio -lpspaudiocodec -lpspkubridge \
	  -lpspsdk -lpspctrl -lpspumd -lpsprtc -lpsppower -lpspgu -lpspgum -lpspaudiolib -lpspasfparser -lpspaudio -lpsphttp -lpspssl -lpspwlan -lm -ljpeg

LDFLAGS =
EXTRA_TARGETS = EBOOT.PBP
PSP_EBOOT_TITLE = LightMP3
PSP_EBOOT_ICON = ICON0.PNG
PSP_EBOOT_PIC1 = PIC1.PNG
PSPSDK=$(shell psp-config --pspsdk-path)
include $(PSPSDK)/lib/build.mak
