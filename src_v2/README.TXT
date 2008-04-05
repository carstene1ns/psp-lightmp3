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
Simply copy the LightMP3 directory under ms0:/PSP/GAME3xx
The src directory contains source code: you don't need it to run the application.


FEATURES
--------
-Skinnable and multilanguage (english, italian, portuguese, french, polish, german)
-Support for kernel 3.x and Slim&Light
-Support for MP3, OGG Vorbis, ATRAC3+ (*.aa3, *.omg, *.oma), FLAC
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


Playlist Browser:
	CROSS   = Play selected playlist
	SQUARE  = Load selected playlist
	CIRCLE  = Remove selected playlist


Playlist Editor:
	CROSS   = Move selected track down
	SQUARE  = Move selected track up
	CIRCLE  = Remove selected track from playlist
	NOTE    = Play playlist
	START   = Save playlist
	SELECT  = Clear playlist


Media Library:
	CROSS   = Enter selection (genre/artist...)
	CIRCLE  = Exit selection
	SQUARE  = Play selection
	START   = Add selection to current playlist


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


DEPENDENCIES
------------
To compile LightMP3 you need:
-libMad
-libTremor
-libFLAC (built without OGG support)
-SQLite3

I built libMad, libTremor and libFLAC with
CFLAGS += -O3 -frename-registers -ffast-math -fomit-frame-pointer


SKINS:
------
LightMP3 is totally skinnable.
For further informations please refer to this forum:
http://www.sakya.it/forums/viewforum.php?f=5


BUGS:
-----
To report a bug or check for known bugs please refer to:
http://code.google.com/p/lightmp3/issues/list
You can also check this forum:
http://www.sakya.it/forums/viewforum.php?f=4


SOURCE:
-------
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
-fabiom, borgqueenx, Mizou93, G-King, coach777, Magic_PSP for tanslations.