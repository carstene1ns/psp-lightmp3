//    LightMP3
//    Copyright (C) 2007,2008,2009 Sakya
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
#include "wma_tag.h"
#include <stdio.h>
#include <string.h>

int wmaJpegStart(unsigned char *buffer, int startPos, int maxSearch)
{
    int i = 0;
    int pos = startPos;

    for (i=0; i<maxSearch; i++)
    {
        if(!memcmp(buffer, WMA_JPEG, 3))
            return pos;
        pos++;
        buffer++;
    }
    return -1;
}

int wmaPngStart(unsigned char *buffer, int startPos, int maxSearch)
{
    int i = 0;
    int pos = startPos;

    for (i=0; i<maxSearch; i++)
    {
        if(!memcmp(buffer, WMA_PNG, 16))
            return pos;
        pos++;
        buffer++;
    }
    return -1;
}

int wmaReadTagValue(unsigned char *buffer, int tagSize, wmaTagValue *tag)
{
    int i,j = 0;
    for (i=0; i<tagSize; i++)
    {
        if (buffer[i] != 0)
        {
            if (j < 256)
                tag->value[j++] = buffer[i];
        }
        tag->value[j] = '\0';
    }
    return 0;
}

int wmaReadTag(unsigned char *buffer, int position, wmaTagValue *tag)
{
    int tagSize = 0;

    strcpy(tag->name, "");
    strcpy(tag->value, "");

    buffer += position;

	if(!memcmp(buffer, WMA_SDKVERSION, 25))
	{
	    buffer += 25 + 5;
	    position += 25 + 5;
	    tagSize = buffer[0] + buffer[1];
	    buffer += 2;
	    position += 2;

	    strcpy(tag->name, "SDKVERSION");
	    tag->size = tagSize;
	    tag->start = position;
	    wmaReadTagValue(buffer, tagSize, tag);

	    return position + tagSize + 2;
	}

	if(!memcmp(buffer, WMA_SDKNEEDED, 23))
	{
	    buffer += 23 + 5;
	    position += 23 + 5;
	    tagSize = buffer[0] + buffer[1];
	    buffer += 2;
	    position += 2;

	    strcpy(tag->name, "SDKNEEDED");
	    tag->size = tagSize;
	    tag->start = position;
	    wmaReadTagValue(buffer, tagSize, tag);

	    return position + tagSize + 2;
	}

	if(!memcmp(buffer, WMA_TRACKNUMBER, 27))
	{
	    buffer += 27 + 5;
	    position += 27 + 5;
	    tagSize = buffer[0] + buffer[1];
	    buffer += 2;
	    position += 2;

	    strcpy(tag->name, "TRACK_NUMBER");
	    tag->size = tagSize;
	    tag->start = position;
	    if (tagSize == 4)
            sprintf(tag->value, "%i", le2int(buffer));
        else
            wmaReadTagValue(buffer, tagSize, tag);

	    return position + tagSize + 2;
	}

	if(!memcmp(buffer, WMA_TRACK, 15))
	{
	    buffer += 15 + 5;
	    position += 15 + 5;
	    tagSize = buffer[0] + buffer[1];
	    buffer += 2;
	    position += 2;

	    strcpy(tag->name, "TRACK");
	    tag->size = tagSize;
	    tag->start = position;
        if (tagSize == 4)
            sprintf(tag->value, "%i", le2int(buffer));
        else
            wmaReadTagValue(buffer, tagSize, tag);

	    return position + tagSize + 2;
	}

	if(!memcmp(buffer, WMA_ALBUMARTIST, 27))
	{
	    buffer += 27 + 5;
	    position += 27 + 5;
	    tagSize = buffer[0] + buffer[1];
	    buffer += 2;
	    position += 2;

	    strcpy(tag->name, "ALBUM_ARTIST");
	    tag->size = tagSize;
	    tag->start = position;
	    wmaReadTagValue(buffer, tagSize, tag);
	    return position + tagSize + 2;
	}

	if(!memcmp(buffer, WMA_RATING, 37))
	{
	    buffer += 37 + 5;
	    position += 37 + 5;
	    tagSize = buffer[0] + buffer[1];
	    buffer += 2;
	    position += 2;

	    strcpy(tag->name, "RATING");
	    tag->size = tagSize;
	    tag->start = position;
	    wmaReadTagValue(buffer, tagSize, tag);
	    return position + tagSize + 2;
	}

	if(!memcmp(buffer, WMA_ALBUMTITLE, 25))
	{
	    buffer += 25 + 5;
	    position += 25 + 5;
	    tagSize = buffer[0] + buffer[1];
	    buffer += 2;
	    position += 2;

	    strcpy(tag->name, "ALBUM_TITLE");
	    tag->size = tagSize;
	    tag->start = position;
	    wmaReadTagValue(buffer, tagSize, tag);
	    return position + tagSize + 2;
	}

	if(!memcmp(buffer, WMA_YEAR, 13))
	{
	    buffer += 13 + 5;
	    position += 13 + 5;
	    tagSize = buffer[0] + buffer[1];
	    buffer += 2;
	    position += 2;

	    strcpy(tag->name, "YEAR");
	    tag->size = tagSize;
	    tag->start = position;
	    wmaReadTagValue(buffer, tagSize, tag);
	    return position + tagSize + 2;
	}

	if(!memcmp(buffer, WMA_GENRE, 15))
	{
	    buffer += 15 + 5;
	    position += 15 + 5;
	    tagSize = buffer[0] + buffer[1];
	    buffer += 2;
	    position += 2;

	    strcpy(tag->name, "GENRE");
	    tag->size = tagSize;
	    tag->start = position;
	    wmaReadTagValue(buffer, tagSize, tag);
	    return position + tagSize + 2;
	}

	if(!memcmp(buffer, WMA_LYRICS, 17))
	{
	    buffer += 17 + 5;
	    position += 17 + 5;
	    tagSize = buffer[0] + buffer[1];
	    buffer += 2;
	    position += 2;

	    strcpy(tag->name, "LYRICS");
	    tag->size = tagSize;
	    tag->start = position;
	    //wmaReadTagValue(buffer, tagSize, tag);
	    return position + tagSize + 2;
	}

	if(!memcmp(buffer, WMA_URL, 29))
	{
	    buffer += 29 + 5;
	    position += 29 + 5;
	    tagSize = buffer[0] + buffer[1];
	    buffer += 2;
	    position += 2;

	    strcpy(tag->name, "ENCODEDBY");
	    tag->size = tagSize;
	    tag->start = position;
	    wmaReadTagValue(buffer, tagSize, tag);
	    return position + tagSize + 2;
	}

	if(!memcmp(buffer, WMA_ENCODEDBY, 23))
	{
	    buffer += 23 + 5;
	    position += 23 + 5;
	    tagSize = buffer[0] + buffer[1];
	    buffer += 2;
	    position += 2;

	    strcpy(tag->name, "URL");
	    tag->size = tagSize;
	    tag->start = position;
	    wmaReadTagValue(buffer, tagSize, tag);
	    return position + tagSize + 2;
	}

	if(!memcmp(buffer, WMA_COMPOSER, 21))
	{
	    buffer += 21 + 5;
	    position += 21 + 5;
	    tagSize = buffer[0] + buffer[1];
	    buffer += 2;
	    position += 2;

	    strcpy(tag->name, "COMPOSER");
	    tag->size = tagSize;
	    tag->start = position;
	    wmaReadTagValue(buffer, tagSize, tag);
	    return position + tagSize + 2;
	}

	if(!memcmp(buffer, WMA_PICTURE, 19))
	{
	    buffer += 19 + 7;
	    position += 19 + 7;

        /* The imageType values should be:
        0 Picture of a type not specifically listed in this table
        1 32 pixel by 32 pixel file icon. Use only with portable network graphics (PNG) format
        2 File icon not conforming to type 1 above
        3 Front album cover
        4 Back album cover
        5 Leaflet page
        6 Media. Typically this type of image is of the label side of a CD
        7 Picture of the lead artist, lead performer, or soloist
        8 Picture of one of the artists or performers
        9 Picture of the conductor
        10 Picture of the band or orchestra
        11 Picture of the composer
        12 Picture of the lyricist or writer
        13 Picture of the recording studio or location
        14 Picture taken during a recording session
        15 Picture taken during a performance
        16 Screen capture from a movie or video
        17 A bright colored fish
        18 Illustration
        19 Logo of the band or artist
        20 Logo of the publisher or studio
        */
        int imageType = buffer[0];

        buffer += 1;
        position += 1;
        tagSize = le2int(buffer);

	    strcpy(tag->name, "PICTURE");
	    tag->size = tagSize;
	    tag->start = wmaJpegStart(buffer, position, 300);
	    if (tag->start < 0){
            tag->start = wmaPngStart(buffer, position, 300);
            if (tag->start >= 0)
                strcpy(tag->value, "PNG");
        } else
            strcpy(tag->value, "JPEG");
        if (tag->start >= 0)
            position = tag->start;
	    return position + tagSize;
	}

	if(!memcmp(buffer, WMA_ISVBR, 9))
	{
	    buffer += 9 + 5;
	    position += 9 + 5;
	    tagSize = buffer[0] + buffer[1];
	    buffer += 2;
	    position += 2;

	    strcpy(tag->name, "ISVBR");
	    tag->size = tagSize;
	    tag->start = position;
        if (tagSize == 4)
            sprintf(tag->value, "%i", le2int(buffer));
        else
            wmaReadTagValue(buffer, tagSize, tag);

	    return position + tagSize + 2;
	}

	if(!memcmp(buffer, WMA_ENCODINGTIME, 29))
	{
	    buffer += 29 + 5;
	    position += 29 + 5;
	    tagSize = buffer[0] + buffer[1];
	    buffer += 2;
	    position += 2;

	    strcpy(tag->name, "ENCODING_TIME");
	    tag->size = tagSize;
	    tag->start = position;
	    //Don't know how to read the value
	    //wmaReadTagValue(buffer, tagSize, tag);

	    return position + tagSize + 2;
	}

	if(!memcmp(buffer, WMA_FILEID, 45))
	{
	    buffer += 45 + 5;
	    position += 45 + 5;
	    tagSize = buffer[0] + buffer[1];
	    buffer += 2;
	    position += 2;

	    strcpy(tag->name, "FILE_ID");
	    tag->size = tagSize;
	    tag->start = position;
	    wmaReadTagValue(buffer, tagSize, tag);

	    return position + tagSize + 2;
	}

	if(!memcmp(buffer, WMA_PUBLISHER, 23))
	{
	    buffer += 23 + 5;
	    position += 23 + 5;
	    tagSize = buffer[0] + buffer[1];
	    buffer += 2;
	    position += 2;

	    strcpy(tag->name, "PUBLISHER");
	    tag->size = tagSize;
	    tag->start = position;
	    wmaReadTagValue(buffer, tagSize, tag);

	    return position + tagSize + 2;
	}

	if(!memcmp(buffer, WMA_MEDIAID, 43))
	{
	    buffer += 43 + 5;
	    position += 43 + 5;
	    tagSize = buffer[0] + buffer[1];
	    buffer += 2;
	    position += 2;

	    strcpy(tag->name, "MEDIAID");
	    tag->size = tagSize;
	    tag->start = position;
	    wmaReadTagValue(buffer, tagSize, tag);

	    return position + tagSize + 2;
	}

	if(!memcmp(buffer, WMA_MCDI, 13))
	{
	    buffer += 13 + 5;
	    position += 13 + 5;
	    tagSize = buffer[0] + buffer[1];
	    buffer += 2;
	    position += 2;

	    strcpy(tag->name, "MCDI");
	    tag->size = tagSize;
	    tag->start = position;
	    wmaReadTagValue(buffer, tagSize, tag);

	    return position + tagSize + 2;
	}
    return 0;
}

int wmaSearchExtendedStart(unsigned char *buffer, int startPos, int maxSearch)
{
    int i = 0;
    int pos = startPos;

    for (i=0; i<maxSearch; i++)
    {
        if(!memcmp(buffer, EXTENDED_GUID, 16))
            return pos;
        pos++;
        buffer++;
    }
    return -1;
}

int wmaSearchContentStart(unsigned char *buffer, int startPos, int maxSearch)
{
    int i = 0;
    int pos = startPos;

    for (i=0; i<maxSearch; i++)
    {
        if(!memcmp(buffer, CONTENT_GUID, 16))
            return pos;
        pos++;
        buffer++;
    }
    return -1;
}

int wmaReadContentTag(unsigned char *buffer, int startPos, wmaContentTag *contentTag)
{
    buffer += startPos;

    contentTag->titleLen = buffer[0] + buffer[1];
    contentTag->authorLen = buffer[2] + buffer[3];
    contentTag->copyrightLen = buffer[4] + buffer[5];
    contentTag->descriptionLen = buffer[6] + buffer[7];
    //int rLen = buffer[8] + buffer[9];

    buffer += 10;
    startPos += 10;
    wmaTagValue tag;
    contentTag->title[0] = '\0';
    if (contentTag->titleLen > 0)
    {
        wmaReadTagValue(buffer, contentTag->titleLen, &tag);
        strcpy(contentTag->title, tag.value);
        startPos += contentTag->titleLen;
    }

    buffer += contentTag->titleLen;
    contentTag->author[0] = '\0';
    if (contentTag->authorLen > 0)
    {
        wmaReadTagValue(buffer, contentTag->authorLen, &tag);
        strcpy(contentTag->author, tag.value);
        startPos += contentTag->authorLen;
    }

    buffer += contentTag->authorLen;
    contentTag->copyright[0] = '\0';
    if (contentTag->copyrightLen > 0)
    {
        wmaReadTagValue(buffer, contentTag->copyrightLen, &tag);
        strcpy(contentTag->copyright, tag.value);
        startPos += contentTag->copyrightLen;
    }

    buffer += contentTag->copyrightLen;
    contentTag->description[0] = '\0';
    if (contentTag->descriptionLen > 0)
    {
        wmaReadTagValue(buffer, contentTag->descriptionLen, &tag);
        strcpy(contentTag->description, tag.value);
        startPos += contentTag->descriptionLen;
    }
    return startPos;
}
