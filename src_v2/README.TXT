///////////////////////////////////////////////
//       LightMP3, 2007, 2008 by Sakya       //
///////////////////////////////////////////////
Homepage   : http://sakya.altervista.org/new/index.php?mod=none_lightmp3
Forums     : http://www.sakya.it/forums/viewforum.php?f=4
Google Code: http://code.google.com/p/lightmp3/
Contacts   : sakya_tg@yahoo.it
To donate  : https://www.paypal.com/xclick/business=sakya_tg@yahoo.it&item_name=lightmp3&item_number=0001&no_note=1&tax=0&currency_code=EUR
LightMP3 is ditributed under GNU General Public License, read LICENSE.TXT for details.


WHAT IS IT?
-----------
LightMP3 is a MP3/OGG Vorbis/FLAC/Atrac3+ player designed to drain little energy from your battery.
Can decode MP3 and ATRAC3+ via Media Engine with CPU set at 20Mhz, OGG Vorbis with CPU set at 50Mhz, MP3 via libMAD
with CPU set at 70Mhz, FLAC with CPU set at 100Mhz.
With battery at 100% and display set to minimum brightness it should last 9 hours.
With battery at 100% and display turned off (press START while playing a track) it should last more than 11
hours.


INSTALL
-------
Simply copy the LightMP3 directory under ms0:/PSP/GAME3xx, ms0:/PSP/GAME4XX or ms0:/PSP/GAME5XX depending on your custom firmware.
The src directory contains source code: you don't need it to run the application.


A NOTE ON CPU CLOCK
-------------------
Please note that default cpu clocks are higher than it needs.
This is because if you have some plugins running you can encounter problems due to cpu clock too low.
If you don't have any cpu-hungry plugin running try to set the clock with this values:
-MP3 with libMad        = 65
-MP3 with Media Engine  = 19
-OGG Vorbis             = 45
-Atrac3+                = 19
-WMA                    = 19

FEATURES
--------
-Bookmark function for audiobook
-Skinnable and multilanguage (english, italian, portuguese, french, polish, dutch)
-Support for kernel 3.x and Slim&Light
-Support for MP3, OGG Vorbis, ATRAC3+ (*.aa3, *.omg, *.oma), FLAC, WMA
-File browser
-Support for m3u playlist
-Playlist Editor
-Retrieve track's information from ID3v1/ID3v2 or Vorbis Comments
-Coverart retrieved from ID3v2 (only MP3) [image formats: jpeg, png]
-Works with remote controller
-Playing mode: Normal, Repeat Track, Repeat All, Shuffle, Shuffle/Repeat
-Equalizers (only for MP3 played via libMAD)
-Audioscrobbler log (you can upload your log to your last.fm's account with this page http://paulstead.com/scrob/)
-Sleep mode (the psp will shutdown automatically at the end of a track or directory/playlist)
-Media Library: scan your ms for media and then browse them by Artist, Album, Genre or make your search.


CONTROLS
--------

General:
	L	= Previous function
	R	= Next function
	L + R   = Show help


File Browser:
	CROSS   = Enter directory/play file
	SQUARE  = Play directory
	CIRCLE  = Go up one level
	START   = Add directory/file to current playlist
    SELECT  = Toggle USB mode


Playlist Browser:
	CROSS   = Play selected playlist
	SQUARE  = Load selected playlist
	CIRCLE  = Remove selected playlist
    START   = Add selected playlist to current one


Playlist Editor:
	CROSS   = Move selected track down
	SQUARE  = Move selected track up
	CIRCLE  = Remove selected track from playlist
    TRIANGLE= To check files (removes dead links)
	NOTE    = Play playlist
	START   = Save playlist
	SELECT  = Clear playlist


Media Library:
	CROSS   = Enter selection (genre/artist...)
	CIRCLE  = Exit selection
	SQUARE  = Play selection
	START   = Add selection to current playlist
    TRIANGLE= Sort tracks


Player:
    CROSS               = Pause/Resume
    CIRLCE              = Stop
    R                   = Next track
    L                   = Previous track
    TRIANGLE            = Change sleep mode
    START               = Toggle economy mode
    SELECT              = Change play mode
    DPAD UP             = Raise volume boost
    DPAD DOWN           = Lower volume boost
    ANALOG UP           = Raise CPU clock
    ANALOG DOWN         = Lower CPU clock
    NOTE                = Change equalizer (if supported)
    CROSS + DPAD UP     = Raise track rating
    CROSS + DPAD DOWN   = Lower track rating
    L + CIRCLE          = Create a bookmark and exit the application (or shut down, see the settings file)


VOLUME BOOST
------------
Please note that by using volume boost (DPAD UP while listening to a track) to raise too much the volume you can damage the psp speakers.
Use it at your own risk.


LIBMAD EQUALIZERS
-----------------
Equalizer works only with MP3s played through libMAD!
If you want to add your own preset or change the standard ones you have to edit the file equalizers
This file contains one row for each preset.
Every row contains 34 columns separated by ;
The first column is the equalizer's long name
The second column is the equalizer's short name
Then 32 columns with the equalizer's values (in dB).

Example:
User EQ n.1;U1;0.0;0.0;0.0;0.0;0.0;0.0;0.0;0.0;0.0;0.0;0.0;0.0;0.0;0.0;0.0;0.0;0.0;0.0;0.0;0.0;0.0;0.0;0.0;0.0;0.0;0.0;0.0;0.0;0.0;0.0;0.0;0.0;


AUDIOSCROBBLER LOG
------------------
LightMP3 can save a .scrobbler.log file that you can then upload to your last.fm account (visit http://www.last.fm).
This feature is disabled by default.
To enable scrobbler go to the options screen.

You can upload the log through this page (remember to delete the .scrobbler.log file after uploading it):
http://paulstead.com/scrob/
Time in the .scrobbler.log is UTC, so submit your log choosing UTC.


COVERART
--------
LightMP3 will look for cover:
1.In ID3v2 info (only for MP3 files)
2.For a file named like the file + .jpg
  Ex: fileName.mp3 --> fileName.mp3.jpg
3.For a file named as the album title (from ID3 info) with jpg extension
  Ex: Album Title.jpg
4.For a file folder.jpg in the same directory
5.For a file named cover.jpg in the same directory


MEDIA LIBRARY
-------------
You can choose the media library root directory (the directory that will be scanned when you choose "Scan MS for media").
Open the settings file and change this setting:

ML_ROOT=ms0:/
Ex:
ML_ROOT=ms0:/MUSIC/


BOOKMARKS
---------
You can create a bookmark by pressing L + CIRCLE while listening to a track.
The app will quit, the next time you'll start it the player will ask you if you want to load the saved bookmark.
Once loaded, the bookmark is deleted from the ms.

NOTE: If you want the app to shut down after creating a bookmark edit the settings file and change:
SHUTDOWN_AFTER_BOOKMARK=0
to
SHUTDOWN_AFTER_BOOKMARK=1


DEPENDENCIES
------------
To compile LightMP3 you need:
-OldSchool Library MOD (OSLib MOD)
-libMad
-libTremor
-libFLAC (built without OGG support)
-LibAac
-SQLite3

I built libMad, libTremor and libFLAC with
CFLAGS += -O3 -frename-registers -ffast-math -fomit-frame-pointer

I often update my sdk so you'll need an updated sdk to compile LightMP3.


SKINS
-----
LightMP3 is totally skinnable.
You can find more skins for LightMP3 here:
http://sakya.altervista.org/new/index.php?mod=none_lightmp3skins

For further informations please refer to this forum:
http://www.sakya.it/forums/viewforum.php?f=5


KOREAN LANGUAGE
---------------
To use korean language you must have flash0:/font/kr0.pgf (not all PSP has this font)
To use the Korean language:
1.Set in the setting tab the default_KOR skin
2.Set in the setting tab the Korean language

JAPANESE LANGUAGE
---------------
To use japanese language you must have flash0:/font/jpn0.pgf (not all PSP has this font)
To use the Japanese language:
1.Set in the setting tab the default_JPN skin
2.Set in the setting tab the Japanese language

BUGS
----
To report a bug or check for known bugs please refer to:
http://code.google.com/p/lightmp3/issues/list
You can also check this forum:
http://www.sakya.it/forums/viewforum.php?f=4


SOURCE
------
Sources are included in every release.
If you want to check the development version (untested, maybe broken) you can use svn:
svn checkout http://lightmp3.googlecode.com/svn/trunk/ lightmp3-read-only


MANY THANKS TO
--------------
-sturatt for the original ID3v1 tag code
-John_K for the original mp3player.c and mp3player.h
-Smerity for his audio tutorial
-crazyc for his useful patch
-John_K & adresd (PSPMediaCenter authors)
-Xart for the original ID3v2 tag code.
-joek2100 for the Media Engine functions (taken from Music prx 0.55 and adapted)
-JLF65 for the FLAC playback function and testing
-Ghoti for the libMad streaming code
-fabiom, borgqueenx, Mizou93, coach777, Magic PSP for tanslations.
-nextos for help with unicode and libiconv
-cooleyes for the fat driver
-dario1crisafulli for the skins, ICON0 and PIC1 images.
-Kevin Kim for his work for better Korean language support and other fixes/improvements
-cooleyes and hrimfaxi for the WMA decode sample
