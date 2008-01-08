//////////////////////////////////////////
//       LightMP3, 2007 by Sakya       //
/////////////////////////////////////////
Homepage   : http://sakya.altervista.org/new/index.php?mod=none_lightmp3
Google Code: http://code.google.com/p/lightmp3/
Contacts   : sakya_tg@yahoo.it
LightMP3 is ditributed under GNU General Public License, read LICENSE.TXT for details.


WHAT'S THAT?
------------
LightMP3 is a basic mp3/OGG Vorbis/FLAC/Atrac3+ player designed to drain little energy from your battery.
Can decode 128kbit (tested also with 160, 193, 320) MP3 at 70mhz CPU and 54mhz BUS.
Can decode 128kbit (tested also with 160, 193, 320) OGG Vorbis at 60mhz CPU and 54mhz BUS.
With battery at 100% and display set to minimum brightness it should last 9 hours.
With battery at 100% and display turned off (press the brightness button for a couple of seconds) it should last more than 11 hours.


INSTALL
-------
Simply copy the LightMP3 directory under ms0:/PSP/GAME3xx


FEATURES
--------
-Support for kernel 3.x and S&L
-Browse all your memstick
-Support m3u playlists.
-Built-in playlist editor
-Plays mp3 with 8khz, 11khz, 12khz, 16khz, 22khz, 24khz, 32khz, 44khz, 48khz sample rate
-Plays mp3 up to ~22Mb (sorry, no streaming/buffering...sorry: it's my first C program)
-Plays OGG vorbis with 8khz, 11khz, 12khz, 16khz, 22khz, 24khz, 32khz, 44khz, 48khz sample rate
-Plays OGG vorbis file without filesize limit
-Plays FLAC
-Plays ATRAC3+
-Detailed ID3v1 and file information
-Works with remote controller
-Four playing mode: Normal, Repeat, Repeat All and Shuffle
-Boost output volume
-Equalizer presets (only for MP3 libMAD): Rock, Classical, Pop, Dance, Reaggae, Soft
-Audioscrobbler log (you can upload you log to your last.fm's account with this page http://paulstead.com/scrob/)
-Sleep mode (the psp will shutdown automatically at the end of a track or directory/playlist)


LIBMAD EQUALIZERS
-----------------
NOTE: Equalizer works only with MP3 file through libMAD.
If you want to add your own preset or change the standard ones you have to edit the file equzlizers
This file contains one row for each preset.
Every row contains 34 columns separated by ;
The first column is the equalizer's long name
The second column is the equalizer's short name (the letters printed at the bottom EQ:xx)
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


DEPENDENCIES
------------
To compile LightMP3 you need:
-libMad (add -O3 to CFLAGS in tha makefile)
-libTremor
-libFLAC


BUGS:
-----
To report a bug or check for known bugs please refer to:
http://code.google.com/p/lightmp3/issues/list


SOURCE:
-------
Sources are included in every release.
If you want to check the development version (untested, maybe broken) you can use svn:
svn checkout http://lightmp3.googlecode.com/svn/trunk/ lightmp3-read-only


MANY THANKS TO
--------------
-jonny for the original menu code
-sturatt for the original ID3 tag code
-John_K for mp3player.c and mp3player.h
-Smerity for his audio tutorial
-AlphaWeapon for the icon. :)
-crazyc for his usefull patch
-John_K & adresd (PSPMediaCenter authors)
-Xart for the ID3v2 tag code.
-joek2100 for the Media Engine functions (taken from Music prx 0.55 and adapted)
-JLF65 for the FLAC playback function and testing
