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
#include <pspsdk.h>
#include <pspiofilemgr.h>
#include <sqlite3.h>
#include <ctype.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>

#include "../others/strreplace.h"
#include "../players/player.h"
#include "medialibrary.h"

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Globals:
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
static char dbDirectory[264];
static char dbFileName[264];
static sqlite3 *db;
static char sql[ML_SQLMAXLENGTH] = "";
static sqlite3_stmt *stmt;
static char *zErr = 0;
static long recordsCount = 0;

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Private functions:
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
int ML_INTERNAL_openDB(char *directory, char *fileName){
    if (sceIoChdir(directory) < 0)
        return ML_ERROR_CHDIR;
	int result = sqlite3_open(fileName, &db);
    if (result)
        return ML_ERROR_OPENDB;
    return 0;
}

int ML_INTERNAL_closeDB(){
    sqlite3_close(db);
    return 0;
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Clear buffer:
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
int clearBuffer(struct libraryEntry *resultBuffer){
    int i;
    for (i=0; i<ML_BUFFERSIZE; i++)
        ML_clearEntry(&resultBuffer[i]);
    return 0;
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Fix string field for SQL:
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
int ML_fixStringField(char *fieldStr){
    char *repStr;
    repStr = replace(fieldStr, "'", "''");
    if (repStr != NULL){
        strcpy(fieldStr, repStr);
        free(repStr);
    }
    repStr = replace(fieldStr, "\\", "\\\\");
    if (repStr != NULL){
        strcpy(fieldStr, repStr);
        free(repStr);
    }
    return 0;
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Check for database existence:
// Returns 1 if the db exists, 0 if doesen't exists
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
int ML_existsDB(char *directory, char *fileName){
    char buffer[264] = "";
    if (directory[strlen(directory)-1] != '/')
        sprintf(buffer, "%s/%s", directory, fileName);
    else
        sprintf(buffer, "%s%s", directory, fileName);
    FILE *test = fopen(buffer, "r");
    if (test == NULL)
        return 0;
    fclose(test);
    return 1;
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Create empty database:
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
int ML_createEmptyDB(char *directory, char *fileName){
    if (sceIoChdir(directory) < 0)
        return ML_ERROR_CHDIR;
    sceIoRemove(fileName);

	int result = ML_INTERNAL_openDB(directory, fileName);
    if (result)
        return ML_ERROR_OPENDB;

    sprintf(sql, "create table media (artist varchar(256), album varchar(256), \
                                      title varchar(256), genre varchar(256), year varchar(4), \
                                      path varchar(264), realpath varchar(264), extension varchar(4), \
                                      seconds integer, tracknumber integer, rating integer, \
                                      samplerate integer, bitrate integer, played integer, \
                                      PRIMARY KEY (path));");
    result = sqlite3_exec(db, sql, NULL, 0, &zErr);
    if (result != SQLITE_OK){
        ML_INTERNAL_closeDB();
        return ML_ERROR_SQL;
    }

    sprintf(sql, "create index media_idx_01 on media (artist, album);");
    result = sqlite3_exec(db, sql, NULL, 0, &zErr);
    if (result != SQLITE_OK){
        ML_INTERNAL_closeDB();
        return ML_ERROR_SQL;
    }

    sprintf(sql, "create index media_idx_02 on media (genre);");
    result = sqlite3_exec(db, sql, NULL, 0, &zErr);
    if (result != SQLITE_OK){
        ML_INTERNAL_closeDB();
        return ML_ERROR_SQL;
    }

    sprintf(sql, "create index media_idx_03 on media (rating);");
    result = sqlite3_exec(db, sql, NULL, 0, &zErr);
    if (result != SQLITE_OK){
        ML_INTERNAL_closeDB();
        return ML_ERROR_SQL;
    }

    ML_INTERNAL_closeDB();
    return 0;
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Open a database:
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
int ML_openDB(char *directory, char *fileName){
    ML_closeDB();
    int retValue = ML_INTERNAL_openDB(directory, fileName);
    if (retValue)
        return retValue;
    strcpy(dbDirectory, directory);
    strcpy(dbFileName, fileName);
    ML_INTERNAL_closeDB();
    return 0;
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Close an opened database:
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
int ML_closeDB(){
    ML_INTERNAL_closeDB();
    strcpy(dbDirectory, "");
    strcpy(dbFileName, "");
    return 0;
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Scan all memory stick for media (returns number of media found):
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
int ML_scanMS(char extFilter[][5], int extNumber,
              int (*scanDir)(char *dirName),
              int (*scanFile)(char *fileName, int errorCode)){
    int mediaFound = 0;
    int invalidFound = 0;
    int openedDir, errorCode;
    static SceIoDirent oneDir;
    char dirToScan[500][264];
    char fullName[264];
    int dirScanned = 0;
    int dirToScanNumber = 1;
    struct fileInfo info;

    if (ML_INTERNAL_openDB(dbDirectory, dbFileName))
        return ML_ERROR_OPENDB;

    strcpy(dirToScan[0], "ms0:/");
    while (dirScanned < dirToScanNumber){
        //Directory callback:
        if (scanDir != NULL)
            if ((*scanDir)(dirToScan[dirScanned]) < 0)
                break;
        openedDir = sceIoDopen(dirToScan[dirScanned]);
        if (openedDir < 0)
            break;

        while (1){
            memset(&oneDir, 0, sizeof(SceIoDirent));
            if (sceIoDread(openedDir, &oneDir) <= 0)
                break;
            if (!strcmp(oneDir.d_name, ".") || !strcmp(oneDir.d_name, ".."))
                continue;

        	if (dirToScan[dirScanned][strlen(dirToScan[dirScanned])-1] != '/')
                sprintf(fullName, "%s/%s", dirToScan[dirScanned], oneDir.d_name);
            else
                sprintf(fullName, "%s%s", dirToScan[dirScanned], oneDir.d_name);

            if (FIO_S_ISREG(oneDir.d_stat.st_mode)){
                //Check extension filter:
				int extOK = 0;
				int i, j;
				char ext[5] = "";
				if (oneDir.d_name[strlen(oneDir.d_name) - 4] == '.')
					j = 3;
				else if (oneDir.d_name[strlen(oneDir.d_name) - 5] == '.')
					j = 4;
                else
                    j = 0;
				for (i = strlen(oneDir.d_name) - j; i < strlen(oneDir.d_name); i++)
					ext[i - strlen(oneDir.d_name) + j] = toupper(oneDir.d_name[i]);

				extOK = 0;
				for (i = 0; i < extNumber; i++){
					if (!strcmp(ext, extFilter[i])){
						extOK = 1;
						break;
					}
				}
				if (!extOK)
					continue;
                //Media found:
                setAudioFunctions(fullName, 1);
                //(*initFunct)(0);
                info = (*getTagInfoFunct)(fullName);
                //int loadRetValue = (*loadFunct)(fullName);
                int loadRetValue = OPENING_OK;
            	if (loadRetValue == OPENING_OK){
                    errorCode = 0;
                    //info = (*getInfoFunct)();
                    ML_fixStringField(info.artist);
                    ML_fixStringField(info.album);
                    ML_fixStringField(info.title);
                    ML_fixStringField(info.genre);
                    ML_fixStringField(oneDir.d_name);
                    ML_fixStringField(fullName);
                    sprintf(sql, "insert into media (artist, album, title, genre, year, path, realpath, \
                                                     extension, seconds, samplerate, bitrate, tracknumber) \
                                  values('%s', '%s', '%s', '%s', '%s', upper('%s'), '%s', '%s', %li, %li, %i, %i);",
                                  info.artist, info.album, info.title, info.genre, info.year,
                                  fullName, fullName, ext,
                                  info.length, info.hz, info.kbit, atoi(info.trackNumber));
                    int retValue = sqlite3_prepare(db, sql, -1, &stmt, 0);
                    if (retValue != SQLITE_OK){
                        ML_INTERNAL_closeDB();
                        return ML_ERROR_SQL;
                    }

                    if (sqlite3_step(stmt) == SQLITE_CONSTRAINT){
                        sqlite3_finalize(stmt);
                        continue;
                    }
                    sqlite3_finalize(stmt);
                    mediaFound++;
                }else{
                    errorCode = loadRetValue;
                    invalidFound++;
                }
                //(*endFunct)();
                unsetAudioFunctions();

                //File callback:
                if (scanFile != NULL)
                    if ((*scanFile)(oneDir.d_name, errorCode) < 0){
                        dirScanned = dirToScanNumber;
                        break;
                    }
            }else if (FIO_S_ISDIR(oneDir.d_stat.st_mode)){
                strcpy(dirToScan[dirToScanNumber++], fullName);
            }
        }
        sceIoDclose(openedDir);
        dirScanned++;
    }

    ML_INTERNAL_closeDB();
    return mediaFound;
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Count records for a query (only where condition):
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
int ML_countRecords(char *whereCondition){
    if (ML_INTERNAL_openDB(dbDirectory, dbFileName))
        return ML_ERROR_OPENDB;
    sprintf(sql, "Select count(*) \
                  From media \
                  Where %s;", whereCondition);
    recordsCount = 0;
    int retValue = sqlite3_prepare(db, sql, -1, &stmt, 0);
    if (retValue != SQLITE_OK){
        ML_INTERNAL_closeDB();
        return ML_ERROR_SQL;
    }

    if (sqlite3_step(stmt) == SQLITE_ROW)
        recordsCount = sqlite3_column_int(stmt, 0);
    sqlite3_finalize(stmt);
    ML_INTERNAL_closeDB();
    return recordsCount;
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Count records for a full select statement:
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
int ML_countRecordsSelect(char *select){
    if (ML_INTERNAL_openDB(dbDirectory, dbFileName))
        return ML_ERROR_OPENDB;
    sprintf(sql, "%s", select);
    recordsCount = 0;
    int retValue = sqlite3_prepare(db, sql, -1, &stmt, 0);
    if (retValue != SQLITE_OK){
        ML_INTERNAL_closeDB();
        return ML_ERROR_SQL;
    }

    while(sqlite3_step(stmt) == SQLITE_ROW)
        recordsCount++;
    sqlite3_finalize(stmt);
    ML_INTERNAL_closeDB();
    return recordsCount;
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Query the DB with a where condition:
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
int ML_queryDB(char *whereCondition, char *orderByCondition, int offset, int limit, struct libraryEntry *resultBuffer){
    if (ML_INTERNAL_openDB(dbDirectory, dbFileName))
        return ML_ERROR_OPENDB;
    sprintf(sql, "Select coalesce(artist, ''), coalesce(album, ''), coalesce(title, ''), coalesce(genre, ''), coalesce(year, ''), \
                         path, coalesce(extension, ''), seconds, rating, \
                         samplerate, bitrate, played, coalesce(realpath, '') \
                  From media \
                  Where %s \
                  Order by %s \
                  LIMIT %i OFFSET %i", whereCondition, orderByCondition, limit, offset);
    int retValue = sqlite3_prepare(db, sql, -1, &stmt, 0);
    if (retValue != SQLITE_OK){
        ML_INTERNAL_closeDB();
        return ML_ERROR_SQL;
    }

    clearBuffer(resultBuffer);
    int count = 0;
    while(sqlite3_step(stmt) == SQLITE_ROW) {
        strcpy(resultBuffer[count].artist, (char*)sqlite3_column_text(stmt, 0));
        strcpy(resultBuffer[count].album, (char*)sqlite3_column_text(stmt, 1));
        strcpy(resultBuffer[count].title, (char*)sqlite3_column_text(stmt, 2));
        strcpy(resultBuffer[count].genre, (char*)sqlite3_column_text(stmt, 3));
        strcpy(resultBuffer[count].year, (char*)sqlite3_column_text(stmt, 4));
        //strcpy(resultBuffer[count].path, (char*)sqlite3_column_text(stmt, 5));
        strcpy(resultBuffer[count].extension, (char*)sqlite3_column_text(stmt, 6));
        resultBuffer[count].seconds = sqlite3_column_int(stmt, 7);
        resultBuffer[count].rating = sqlite3_column_int(stmt, 8);
        if (resultBuffer[count].rating > ML_MAX_RATING)
            resultBuffer[count].rating = ML_MAX_RATING;
        else if (resultBuffer[count].rating < 0)
            resultBuffer[count].rating = 0;
        resultBuffer[count].samplerate = sqlite3_column_int(stmt, 9);
        resultBuffer[count].bitrate = sqlite3_column_int(stmt, 10);
        resultBuffer[count].played = sqlite3_column_int(stmt, 11);
        strcpy(resultBuffer[count].path, (char*)sqlite3_column_text(stmt, 12));
        if (++count > ML_BUFFERSIZE)
            break;
    }
    sqlite3_finalize(stmt);
    ML_INTERNAL_closeDB();
    return 0;
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Query the DB with a SQL statement:
// The SQL statement can select all fields from media table plus:
// strfield char
// datafield char
// intfield01 int
// intfield02 int
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
int ML_queryDBSelect(char *select, int offset, int limit, struct libraryEntry *resultBuffer){
    if (ML_INTERNAL_openDB(dbDirectory, dbFileName))
        return ML_ERROR_OPENDB;
    sprintf(sql, "%s LIMIT %i OFFSET %i", select, limit, offset);

    int retValue = sqlite3_prepare(db, sql, -1, &stmt, 0);
    if (retValue != SQLITE_OK){
        ML_INTERNAL_closeDB();
        return ML_ERROR_SQL;
    }

    clearBuffer(resultBuffer);
    int count = 0;
    while(sqlite3_step(stmt) == SQLITE_ROW) {
        int i = 0;
        char columnName[100] = "";
        for (i=0; i<sqlite3_column_count(stmt); i++){
            strcpy(columnName, sqlite3_column_name(stmt, i));
            if (!stricmp(columnName, "artist"))
                strcpy(resultBuffer[count].artist, (char*)sqlite3_column_text(stmt, i));
            else if (!stricmp(columnName, "album"))
                strcpy(resultBuffer[count].album, (char*)sqlite3_column_text(stmt, i));
            else if (!stricmp(columnName, "title"))
                strcpy(resultBuffer[count].title, (char*)sqlite3_column_text(stmt, i));
            else if (!stricmp(columnName, "genre"))
                strcpy(resultBuffer[count].genre, (char*)sqlite3_column_text(stmt, i));
            else if (!stricmp(columnName, "year"))
                strcpy(resultBuffer[count].year, (char*)sqlite3_column_text(stmt, i));
            else if (!stricmp(columnName, "realpath"))
                strcpy(resultBuffer[count].path, (char*)sqlite3_column_text(stmt, i));
            else if (!stricmp(columnName, "extension"))
                strcpy(resultBuffer[count].extension, (char*)sqlite3_column_text(stmt, i));
            else if (!stricmp(columnName, "seconds"))
                resultBuffer[count].seconds = sqlite3_column_int(stmt, i);
            else if (!stricmp(columnName, "rating")){
                resultBuffer[count].rating = sqlite3_column_int(stmt, i);
                if (resultBuffer[count].rating > ML_MAX_RATING)
                    resultBuffer[count].rating = ML_MAX_RATING;
                else if (resultBuffer[count].rating < 0)
                    resultBuffer[count].rating = 0;
            }else if (!stricmp(columnName, "samplerate"))
                resultBuffer[count].samplerate = sqlite3_column_int(stmt, i);
            else if (!stricmp(columnName, "bitrate"))
                resultBuffer[count].bitrate = sqlite3_column_int(stmt, i);
            else if (!stricmp(columnName, "played"))
                resultBuffer[count].played = sqlite3_column_int(stmt, i);
            else if (!stricmp(columnName, "strfield"))
                strcpy(resultBuffer[count].strField, (char*)sqlite3_column_text(stmt, i));
            else if (!stricmp(columnName, "datafield"))
                strcpy(resultBuffer[count].dataField, (char*)sqlite3_column_text(stmt, i));
            else if (!stricmp(columnName, "intfield01"))
                resultBuffer[count].intField01 = sqlite3_column_double(stmt, i);
            else if (!stricmp(columnName, "intfield02"))
                resultBuffer[count].intField02 = sqlite3_column_double(stmt, i);
        }
        if (++count > ML_BUFFERSIZE)
            break;
    }
    sqlite3_finalize(stmt);
    ML_INTERNAL_closeDB();
    return 0;
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Add a single entry in the library:
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
int ML_addEntry(struct libraryEntry entry){
    if (ML_INTERNAL_openDB(dbDirectory, dbFileName))
        return ML_ERROR_OPENDB;

    ML_fixStringField(entry.path);
    ML_fixStringField(entry.artist);
    ML_fixStringField(entry.album);
    ML_fixStringField(entry.title);
    ML_fixStringField(entry.genre);
    if (entry.rating > ML_MAX_RATING)
        entry.rating = ML_MAX_RATING;
    else if (entry.rating < 0)
        entry.rating = 0;

    sprintf(sql, "insert into media (artist, album, title, genre, year, path, realpath, \
                                     extension, seconds, samplerate, bitrate, tracknumber, \
                                     rating, played) \
                  values('%s', '%s', '%s', '%s', '%s', upper('%s'), '%s', '%s', %i, %i, %i, %i, %i, %i);",
                  entry.artist, entry.album, entry.title, entry.genre, entry.year,
                  entry.path, entry.path, entry.extension,
                  entry.seconds, entry.samplerate, entry.bitrate, entry.tracknumber,
                  entry.rating, entry.played);
    int retValue = sqlite3_prepare(db, sql, -1, &stmt, 0);
    if (retValue != SQLITE_OK){
        ML_INTERNAL_closeDB();
        return ML_ERROR_SQL;
    }
    sqlite3_step(stmt);
    sqlite3_finalize(stmt);
    ML_INTERNAL_closeDB();
    return 0;
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Update a single entry in the library:
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
int ML_updateEntry(struct libraryEntry entry){
    if (ML_INTERNAL_openDB(dbDirectory, dbFileName))
        return ML_ERROR_OPENDB;

    ML_fixStringField(entry.path);
    ML_fixStringField(entry.artist);
    ML_fixStringField(entry.album);
    ML_fixStringField(entry.title);
    ML_fixStringField(entry.genre);
    if (entry.rating > ML_MAX_RATING)
        entry.rating = ML_MAX_RATING;
    else if (entry.rating < 0)
        entry.rating = 0;

    sprintf(sql, "update media \
                  set artist = '%s', album = '%s', title = '%s', genre = '%s', \
                      year = '%s', extension = '%s', \
                      seconds = %i, samplerate = %i, bitrate = %i, tracknumber = %i, \
                      rating = %i, played = %i \
                  where path = upper('%s');",
                  entry.artist, entry.album, entry.title, entry.genre, entry.year,
                  entry.extension,
                  entry.seconds, entry.samplerate, entry.bitrate, entry.tracknumber, entry.rating,
                  entry.played, entry.path);
    int retValue = sqlite3_prepare(db, sql, -1, &stmt, 0);
    if (retValue != SQLITE_OK){
        ML_INTERNAL_closeDB();
        return ML_ERROR_SQL;
    }
    sqlite3_step(stmt);
    sqlite3_finalize(stmt);

    ML_INTERNAL_closeDB();
    return 0;
}


///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Clear an entry:
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
int ML_clearEntry(struct libraryEntry *entry){
    strcpy(entry->artist, "");
    strcpy(entry->album, "");
    strcpy(entry->title, "");
    strcpy(entry->genre, "");
    strcpy(entry->year, "");
    entry->tracknumber = 0;
    strcpy(entry->path, "");
    strcpy(entry->extension, "");
    entry->seconds = 0;
    entry->rating = 0;
    entry->samplerate = 0;
    entry->bitrate = 0;
    entry->played = 0;

    strcpy(entry->strField, "");
    strcpy(entry->dataField, "");
    entry->intField01 = 0;
    entry->intField02 = 0;
    return 0;
}


///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Check every entry and delete if file doesen't exists
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
int ML_checkFiles(int (*checkFile)(char *fileName)){
    if (ML_INTERNAL_openDB(dbDirectory, dbFileName))
        return ML_ERROR_OPENDB;

    sprintf(sql, "Select path, coalesce(realpath, '') From media Order by path");
    int retValue = sqlite3_prepare(db, sql, -1, &stmt, 0);
    if (retValue != SQLITE_OK){
        ML_INTERNAL_closeDB();
        return ML_ERROR_SQL;
    }

    while(sqlite3_step(stmt) == SQLITE_ROW) {
        FILE *test;
        char fileName[264] = "";
        char path[264] = "";
        sqlite3_stmt *del;
        strcpy(path, (char*)sqlite3_column_text(stmt, 0));
        strcpy(fileName, (char*)sqlite3_column_text(stmt, 1));
        if (checkFile != NULL)
            if ((*checkFile)(fileName) < 0)
                break;

        test = fopen(fileName, "r");
        if (test == NULL){
            ML_fixStringField(fileName);
            sprintf(sql, "Delete From media Where path = upper('%s');", path);
            int retValue = sqlite3_prepare(db, sql, -1, &del, 0);
            if (retValue != SQLITE_OK)
                continue;
            sqlite3_step(del);
            sqlite3_finalize(del);
        }else{
            fclose(test);
        }
    }
    sqlite3_finalize(stmt);

    ML_INTERNAL_closeDB();
    return 0;
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Returns the last SQL executed:
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
int ML_getLastSQL(char *sqlOut){
    strcpy(sqlOut, sql);
    return 0;
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Vacuum:
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
int ML_vacuum(){
    if (ML_INTERNAL_openDB(dbDirectory, dbFileName))
        return ML_ERROR_OPENDB;

    sprintf(sql, "VACUUM");
    int retValue = sqlite3_prepare(db, sql, -1, &stmt, 0);
    if (retValue != SQLITE_OK){
        ML_INTERNAL_closeDB();
        return ML_ERROR_SQL;
    }
    sqlite3_step(stmt);
    sqlite3_finalize(stmt);

    ML_INTERNAL_closeDB();
    return 0;
}

