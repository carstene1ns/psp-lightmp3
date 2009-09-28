//    LightMP3
//    Copyright (C) 2007,2008 Sakya
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
#ifndef __medialibrary_h
#define __medialibrary_h (1)

#define ML_DB_VERSION "01.00"

#define ML_ERROR_CHDIR  -1
#define ML_ERROR_OPENDB -2
#define ML_ERROR_SQL    -3

#define ML_SQLMAXLENGTH 1024*3
#define ML_BUFFERSIZE 200
#define ML_MAX_RATING 5

struct libraryEntry{
    char artist[260];
    char album[260];
    char title[260];
    char genre[260];
    char year[8];
    int tracknumber;
    int played;
    char path[268];
    char shortpath[268];
    char extension[8];
    int seconds;
    int rating;
    int samplerate;
    int bitrate;

    //Fields for free sql query:
    char strField[1024];
    char dataField[1024];
    double intField01;
    double intField02;
};

int ML_fixStringField(char *fieldStr);
int ML_existsDB(char *directory, char *fileName);
int ML_createEmptyDB(char *directory, char *fileName);
int ML_openDB(char *directory, char *fileName);
int ML_closeDB();
int ML_scanMS(char *rootDir, char extFilter[][5], int extNumber, int (*scanDir)(char *dirName), int (*scanFile)(char *fileName, int errorCode));
int ML_countRecords(char *whereCondition);
int ML_countRecordsSelect(char *select);
int ML_queryDB(char *whereCondition, char *orderByCondition, int offset, int limit, struct libraryEntry **result);
int ML_queryDBSelect(char *select, int offset, int limit, struct libraryEntry **resultBuffer);

void ML_clearBuffer(struct libraryEntry **resultBuffer);
void ML_clearEntry(struct libraryEntry *entry);
int ML_addEntry(struct libraryEntry *entry);
int ML_updateEntry(struct libraryEntry *entry, char *newPath);
int ML_checkFiles(int (*checkFile)(char *fileName));
int ML_getLastSQL(char *sqlOut);
int ML_vacuum();

int ML_startTransaction();
int ML_commitTransaction();
int ML_inTransaction();

#endif
