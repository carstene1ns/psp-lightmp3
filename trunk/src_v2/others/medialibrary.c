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
#include "../system/opendir.h"
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
static char dirToScan[500][264];
static char dirToScanShort[500][264];

static int transaction = 0;

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Private functions:
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
int ML_INTERNAL_closeDB(){
    if (transaction)
        return 0;
    sqlite3_close(db);
    return 0;
}

int ML_INTERNAL_openDB(char *directory, char *fileName){
    if (transaction)
        return 0;

    if (sceIoChdir(directory) < 0)
        return ML_ERROR_CHDIR;
	int result = sqlite3_open(fileName, &db);
    if (result)
        return ML_ERROR_OPENDB;

    result = sqlite3_exec(db, "PRAGMA synchronous=OFF", NULL, 0, &zErr);
    if (result != SQLITE_OK){
        sqlite3_free(zErr);
        ML_INTERNAL_closeDB();
        return ML_ERROR_SQL;
    }

    return 0;
}


///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Clear buffer:
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void ML_clearBuffer(struct libraryEntry **resultBuffer){
    int i;
    for (i=0; i<ML_BUFFERSIZE; i++){
        if (resultBuffer[i]){
            free(resultBuffer[i]);
            resultBuffer[i] = NULL;
        }
    }

}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Fix string field for SQL:
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
int ML_fixStringField(char *fieldStr){
    char *repStr = NULL;
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
        snprintf(buffer, sizeof(buffer), "%s/%s", directory, fileName);
    else
        snprintf(buffer, sizeof(buffer), "%s%s", directory, fileName);
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

    snprintf(sql, sizeof(sql), "create table media (artist varchar(256), album varchar(256), \
                                      title varchar(256), genre varchar(256), year varchar(4), \
                                      path varchar(264), shortpath varchar(264), extension varchar(4), \
                                      seconds integer, tracknumber integer, rating integer, \
                                      samplerate integer, bitrate integer, played integer, \
                                      PRIMARY KEY (path));");
    result = sqlite3_exec(db, sql, NULL, 0, &zErr);
    if (result != SQLITE_OK){
        sqlite3_free(zErr);
        ML_INTERNAL_closeDB();
        return ML_ERROR_SQL;
    }

    snprintf(sql, sizeof(sql), "create table sysvars (varname varchar(256), varvalue varchar(256), \
                                        PRIMARY KEY (varname));");
    result = sqlite3_exec(db, sql, NULL, 0, &zErr);
    if (result != SQLITE_OK){
        sqlite3_free(zErr);
        ML_INTERNAL_closeDB();
        return ML_ERROR_SQL;
    }
	snprintf(sql, sizeof(sql), "Insert into sysvars(varname, varvalue) values('DB_VERSION', '%s');", ML_DB_VERSION);
    result = sqlite3_exec(db, sql, NULL, 0, &zErr);
    if (result != SQLITE_OK){
        sqlite3_free(zErr);
        ML_INTERNAL_closeDB();
        return ML_ERROR_SQL;
    }


	char indexes[20][100] = {
							 "artist, album",
							 "genre",
							 "rating",
							 "played desc, rating desc, title",
							 "title, artist",
 							 "artist, album, tracknumber",
                             "artist, album, title",
                             "artist, title",
                             "title",
                             "year, title",
                             "year, artist, album, tracknumber",
                             "year, artist, album, title",
                             "rating desc, artist, album, tracknumber",
                             "rating desc, artist, album, title"
							};

	int i = 0;
	for (i=0; i<20; i++){
		if (!strlen(indexes[i]))
			break;
		snprintf(sql, sizeof(sql), "create index media_idx_%2.2i on media (%s);", i + 1, indexes[i]);
		result = sqlite3_exec(db, sql, NULL, 0, &zErr);
		if (result != SQLITE_OK){
            sqlite3_free(zErr);
			ML_INTERNAL_closeDB();
			return ML_ERROR_SQL;
		}
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
// Scan all memory stick for media (returns number of media found or < 0 if an error occurred):
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
int ML_scanMS(char *rootDir,
              char extFilter[][5], int extNumber,
              int (*scanDir)(char *dirName),
              int (*scanFile)(char *fileName, int errorCode)){
    int mediaFound = 0;
    int errorCode;
    //char dirToScan[500][264];
    //char dirToScanShort[500][264];
    char fullName[264] = "";
    char fullNameShort[264] = "";
    char ext[5] = "";
    int dirScanned = 0;
    int dirToScanNumber = 1;
    struct fileInfo info;
	struct opendir_struct openedDir;
    char *result = NULL;
    int i = 0;
    int retValue = 0;

    if (ML_INTERNAL_openDB(dbDirectory, dbFileName))
        return ML_ERROR_OPENDB;

    retValue = sqlite3_exec(db, "BEGIN TRANSACTION", NULL, 0, &zErr);
    if (retValue != SQLITE_OK){
        sqlite3_free(zErr);
        ML_INTERNAL_closeDB();
        return ML_ERROR_SQL;
    }

    //FILE *log = fopen("ML_scan.txt", "w");

    strcpy(dirToScan[0], rootDir);
    strcpy(dirToScanShort[0], rootDir);
    while (dirScanned < dirToScanNumber){
        //fwrite("Scanning: ", sizeof(char), strlen("Scanning: "), log);
        //fwrite(dirToScan[dirScanned], sizeof(char), strlen(dirToScan[dirScanned]), log);

        //Directory callback:
        if (scanDir != NULL)
            if ((*scanDir)(dirToScan[dirScanned]) < 0)
                break;

    	result = opendir_open(&openedDir, dirToScan[dirScanned], dirToScanShort[dirScanned], extFilter, extNumber, 1);
    	if (result == 0){
            for (i = 0; i < openedDir.number_of_directory_entries; i++){
                if (!strcmp(openedDir.directory_entry[i].d_name, ".") || !strcmp(openedDir.directory_entry[i].d_name, ".."))
                    continue;

                //fwrite("  ", sizeof(char), strlen("  "), log);
                //fwrite(openedDir.directory_entry[i].longname, sizeof(char), strlen(openedDir.directory_entry[i].longname), log);

                if (dirToScan[dirScanned][strlen(dirToScan[dirScanned])-1] != '/'){
                    snprintf(fullName, sizeof(fullName), "%s/%s", dirToScan[dirScanned], openedDir.directory_entry[i].longname);
                    snprintf(fullNameShort, sizeof(fullNameShort), "%s/%s", dirToScanShort[dirScanned], openedDir.directory_entry[i].d_name);
                }else{
                    snprintf(fullName, sizeof(fullName), "%s%s", dirToScan[dirScanned], openedDir.directory_entry[i].longname);
                    snprintf(fullNameShort, sizeof(fullNameShort), "%s%s", dirToScanShort[dirScanned], openedDir.directory_entry[i].d_name);
                }

                if (FIO_S_ISREG(openedDir.directory_entry[i].d_stat.st_mode)){
                    //Media found:
                    if (setAudioFunctions(fullNameShort, 1))
						continue;
                    info = (*getTagInfoFunct)(fullNameShort);
                    errorCode = 0;
                    getExtension(fullNameShort, ext, 4);
                    ML_fixStringField(info.artist);
                    ML_fixStringField(info.album);
                    ML_fixStringField(info.title);
                    ML_fixStringField(info.genre);
                    //ML_fixStringField(fullName);
                    ML_fixStringField(fullNameShort);
                    snprintf(sql, sizeof(sql), "insert into media (artist, album, title, genre, year, path, shortpath, \
                                                     extension, seconds, samplerate, bitrate, tracknumber) \
                                  values('%s', '%s', '%s', '%s', '%s', upper('%s'), '%s', '%s', %li, %li, %i, %i);",
                                  info.artist, info.album, info.title, info.genre, info.year,
                                  fullNameShort, fullNameShort, ext,
                                  info.length, info.hz, info.kbit, atoi(info.trackNumber));
                    retValue = sqlite3_prepare_v2(db, sql, -1, &stmt, 0);
                    if (retValue != SQLITE_OK){
                        //ML_INTERNAL_closeDB();
                        //return ML_ERROR_SQL;
                        sqlite3_finalize(stmt);
                        continue;
                    }

                    if (sqlite3_step(stmt) == SQLITE_CONSTRAINT){
                        sqlite3_finalize(stmt);
                        continue;
                    }
                    sqlite3_finalize(stmt);
                    mediaFound++;
                    unsetAudioFunctions();

                    //File callback:
                    if (scanFile != NULL)
                        if ((*scanFile)(openedDir.directory_entry[i].longname, errorCode) < 0){
                            dirScanned = dirToScanNumber;
                            break;
                        }
                }else if (FIO_S_ISDIR(openedDir.directory_entry[i].d_stat.st_mode)){
                    strcpy(dirToScan[dirToScanNumber], fullName);
                    strcpy(dirToScanShort[dirToScanNumber], fullNameShort);
                    dirToScanNumber++;
                }
            }
        }
    	opendir_close(&openedDir);
        dirScanned++;
    }
    //fclose(log);

    retValue = sqlite3_exec(db, "COMMIT", NULL, 0, &zErr);
    if (retValue != SQLITE_OK){
        sqlite3_free(zErr);
        ML_INTERNAL_closeDB();
        return ML_ERROR_SQL;
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
    snprintf(sql, sizeof(sql), "Select count(*) \
                  From media \
                  Where %s;", whereCondition);
    recordsCount = 0;
    int retValue = sqlite3_prepare_v2(db, sql, -1, &stmt, 0);
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
    snprintf(sql, sizeof(sql), "%s", select);
    recordsCount = 0;
    int retValue = sqlite3_prepare_v2(db, sql, -1, &stmt, 0);
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
int ML_queryDB(char *whereCondition, char *orderByCondition, int offset, int limit, struct libraryEntry **resultBuffer){
    if (ML_INTERNAL_openDB(dbDirectory, dbFileName))
        return ML_ERROR_OPENDB;
    snprintf(sql, sizeof(sql), "Select coalesce(artist, ''), coalesce(album, ''), coalesce(title, ''), coalesce(genre, ''), coalesce(year, ''), \
                         path, coalesce(extension, ''), seconds, rating, \
                         samplerate, bitrate, played, coalesce(shortpath, '') \
                  From media \
                  Where %s \
                  Order by %s \
                  LIMIT %i OFFSET %i", whereCondition, orderByCondition, limit, offset);
    int retValue = sqlite3_prepare_v2(db, sql, -1, &stmt, 0);
    if (retValue != SQLITE_OK){
        ML_INTERNAL_closeDB();
        return ML_ERROR_SQL;
    }

    ML_clearBuffer(resultBuffer);
    int count = 0;
    while(sqlite3_step(stmt) == SQLITE_ROW) {
        if (!resultBuffer[count]){
            resultBuffer[count] = malloc(sizeof(struct libraryEntry));
            if (!resultBuffer[count])
                break;
            ML_clearEntry(resultBuffer[count]);
        }

        strcpy(resultBuffer[count]->artist, (char*)sqlite3_column_text(stmt, 0));
        strcpy(resultBuffer[count]->album, (char*)sqlite3_column_text(stmt, 1));
        strcpy(resultBuffer[count]->title, (char*)sqlite3_column_text(stmt, 2));
        strcpy(resultBuffer[count]->genre, (char*)sqlite3_column_text(stmt, 3));
        strcpy(resultBuffer[count]->year, (char*)sqlite3_column_text(stmt, 4));
        //strcpy(resultBuffer[count]->path, (char*)sqlite3_column_text(stmt, 5));
        strcpy(resultBuffer[count]->extension, (char*)sqlite3_column_text(stmt, 6));
        resultBuffer[count]->seconds = sqlite3_column_int(stmt, 7);
        resultBuffer[count]->rating = sqlite3_column_int(stmt, 8);
        if (resultBuffer[count]->rating > ML_MAX_RATING)
            resultBuffer[count]->rating = ML_MAX_RATING;
        else if (resultBuffer[count]->rating < 0)
            resultBuffer[count]->rating = 0;
        resultBuffer[count]->samplerate = sqlite3_column_int(stmt, 9);
        resultBuffer[count]->bitrate = sqlite3_column_int(stmt, 10);
        resultBuffer[count]->played = sqlite3_column_int(stmt, 11);
        strcpy(resultBuffer[count]->path, (char*)sqlite3_column_text(stmt, 12));
        strcpy(resultBuffer[count]->shortpath, (char*)sqlite3_column_text(stmt, 12));
        if (++count > ML_BUFFERSIZE)
            break;
    }
    sqlite3_finalize(stmt);
    ML_INTERNAL_closeDB();
    return count;
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Query the DB with a SQL statement:
// The SQL statement can select all fields from media table plus:
// strfield char
// datafield char
// intfield01 int
// intfield02 int
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
int ML_queryDBSelect(char *select, int offset, int limit, struct libraryEntry **resultBuffer){
    if (ML_INTERNAL_openDB(dbDirectory, dbFileName))
        return ML_ERROR_OPENDB;
	if (strstr(select, " LIMIT "))
	    snprintf(sql, sizeof(sql), "%s OFFSET %i", select, offset);
	else
	    snprintf(sql, sizeof(sql), "%s LIMIT %i OFFSET %i", select, limit, offset);

    int retValue = sqlite3_prepare_v2(db, sql, -1, &stmt, 0);
    if (retValue != SQLITE_OK){
        ML_INTERNAL_closeDB();
        return ML_ERROR_SQL;
    }

    ML_clearBuffer(resultBuffer);
    int count = 0;
    while(sqlite3_step(stmt) == SQLITE_ROW) {
        if (!resultBuffer[count]){
            resultBuffer[count] = malloc(sizeof(struct libraryEntry));
            if (!resultBuffer[count])
                break;
            ML_clearEntry(resultBuffer[count]);
        }

        int i = 0;
        char columnName[100] = "";
        for (i=0; i<sqlite3_column_count(stmt); i++){
            strcpy(columnName, sqlite3_column_name(stmt, i));
            if (!stricmp(columnName, "artist"))
                strcpy(resultBuffer[count]->artist, (char*)sqlite3_column_text(stmt, i));
            else if (!stricmp(columnName, "album"))
                strcpy(resultBuffer[count]->album, (char*)sqlite3_column_text(stmt, i));
            else if (!stricmp(columnName, "title"))
                strcpy(resultBuffer[count]->title, (char*)sqlite3_column_text(stmt, i));
            else if (!stricmp(columnName, "genre"))
                strcpy(resultBuffer[count]->genre, (char*)sqlite3_column_text(stmt, i));
            else if (!stricmp(columnName, "year"))
                strcpy(resultBuffer[count]->year, (char*)sqlite3_column_text(stmt, i));
            else if (!stricmp(columnName, "path"))
                strcpy(resultBuffer[count]->path, (char*)sqlite3_column_text(stmt, i));
            else if (!stricmp(columnName, "shortpath"))
                strcpy(resultBuffer[count]->shortpath, (char*)sqlite3_column_text(stmt, i));
            else if (!stricmp(columnName, "extension"))
                strcpy(resultBuffer[count]->extension, (char*)sqlite3_column_text(stmt, i));
            else if (!stricmp(columnName, "seconds"))
                resultBuffer[count]->seconds = sqlite3_column_int(stmt, i);
            else if (!stricmp(columnName, "rating")){
                resultBuffer[count]->rating = sqlite3_column_int(stmt, i);
                if (resultBuffer[count]->rating > ML_MAX_RATING)
                    resultBuffer[count]->rating = ML_MAX_RATING;
                else if (resultBuffer[count]->rating < 0)
                    resultBuffer[count]->rating = 0;
            }else if (!stricmp(columnName, "samplerate"))
                resultBuffer[count]->samplerate = sqlite3_column_int(stmt, i);
            else if (!stricmp(columnName, "bitrate"))
                resultBuffer[count]->bitrate = sqlite3_column_int(stmt, i);
            else if (!stricmp(columnName, "played"))
                resultBuffer[count]->played = sqlite3_column_int(stmt, i);
            else if (!stricmp(columnName, "strfield")){
                strncpy(resultBuffer[count]->strField, (char*)sqlite3_column_text(stmt, i), 263);
				resultBuffer[count]->strField[263] = '\0';
            }else if (!stricmp(columnName, "datafield")){
                strncpy(resultBuffer[count]->dataField, (char*)sqlite3_column_text(stmt, i), 511);
				resultBuffer[count]->dataField[511] = '\0';
            }else if (!stricmp(columnName, "intfield01"))
                resultBuffer[count]->intField01 = sqlite3_column_double(stmt, i);
            else if (!stricmp(columnName, "intfield02"))
                resultBuffer[count]->intField02 = sqlite3_column_double(stmt, i);
        }
        if (++count > ML_BUFFERSIZE)
            break;
    }
    sqlite3_finalize(stmt);
    ML_INTERNAL_closeDB();
    return count;
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Add a single entry in the library:
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
int ML_addEntry(struct libraryEntry *entry){
    if (ML_INTERNAL_openDB(dbDirectory, dbFileName))
        return ML_ERROR_OPENDB;

    ML_fixStringField(entry->path);
    ML_fixStringField(entry->artist);
    ML_fixStringField(entry->album);
    ML_fixStringField(entry->title);
    ML_fixStringField(entry->genre);
    if (entry->rating > ML_MAX_RATING)
        entry->rating = ML_MAX_RATING;
    else if (entry->rating < 0)
        entry->rating = 0;

    snprintf(sql, sizeof(sql), "insert into media (artist, album, title, genre, year, path, shortpath, \
                                     extension, seconds, samplerate, bitrate, tracknumber, \
                                     rating, played) \
                  values('%s', '%s', '%s', '%s', '%s', \
                         upper('%s'), '%s', '%s', \
                         %i, %i, %i, %i, \
                         %i, %i);",
                  entry->artist, entry->album, entry->title, entry->genre, entry->year,
                  entry->path, entry->path, entry->extension,
                  entry->seconds, entry->samplerate, entry->bitrate, entry->tracknumber,
                  entry->rating, entry->played);
    int retValue = sqlite3_prepare_v2(db, sql, -1, &stmt, 0);
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
// Pass newPath to change the path (PrimaryKey)
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
int ML_updateEntry(struct libraryEntry *entry, char *newPath){
    if (ML_INTERNAL_openDB(dbDirectory, dbFileName))
        return ML_ERROR_OPENDB;

    char localNewPath[264] = "";
    ML_fixStringField(entry->path);
    ML_fixStringField(entry->shortpath);
    ML_fixStringField(entry->artist);
    ML_fixStringField(entry->album);
    ML_fixStringField(entry->title);
    ML_fixStringField(entry->genre);
    if (entry->rating > ML_MAX_RATING)
        entry->rating = ML_MAX_RATING;
    else if (entry->rating < 0)
        entry->rating = 0;

    if (!strlen(newPath))
        strcpy(localNewPath, entry->path);
    else
    {
        strcpy(localNewPath, newPath);
        ML_fixStringField(localNewPath);

    }

    snprintf(sql, sizeof(sql),
				 "update media \
                  set artist = '%s', album = '%s', title = '%s', genre = '%s', \
                      year = '%s', extension = '%s', \
                      seconds = %i, samplerate = %i, bitrate = %i, tracknumber = %i, \
                      rating = %i, played = %i, shortpath = '%s', \
                      path = upper('%s') \
                  where path = upper('%s');",
                  entry->artist, entry->album, entry->title, entry->genre,
				  entry->year, entry->extension,
                  entry->seconds, entry->samplerate, entry->bitrate, entry->tracknumber,
				  entry->rating, entry->played, entry->shortpath,
                  localNewPath,
				  entry->path);
    int retValue = sqlite3_prepare_v2(db, sql, -1, &stmt, 0);
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
void ML_clearEntry(struct libraryEntry *entry){
	memset(entry, 0, sizeof(struct libraryEntry));
}


///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Check every entry and delete if file doesen't exists
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
int ML_checkFiles(int (*checkFile)(char *fileName)){
    if (ML_INTERNAL_openDB(dbDirectory, dbFileName))
        return ML_ERROR_OPENDB;

    int retValue = sqlite3_exec(db, "BEGIN TRANSACTION", NULL, 0, &zErr);
    if (retValue != SQLITE_OK){
        sqlite3_free(zErr);
        ML_INTERNAL_closeDB();
        return ML_ERROR_SQL;
    }

    snprintf(sql, sizeof(sql), "Select path, coalesce(shortpath, '') From media Order by path");
    retValue = sqlite3_prepare_v2(db, sql, -1, &stmt, 0);
    if (retValue != SQLITE_OK){
        ML_INTERNAL_closeDB();
        return ML_ERROR_SQL;
    }

    while(sqlite3_step(stmt) == SQLITE_ROW) {
        int test = 0;
        char fileName[264] = "";
        char path[264] = "";
        sqlite3_stmt *del;
        strcpy(path, (char*)sqlite3_column_text(stmt, 0));
        strcpy(fileName, (char*)sqlite3_column_text(stmt, 1));
        if (checkFile != NULL)
            if ((*checkFile)(fileName) < 0)
                break;

        test = sceIoOpen(fileName, PSP_O_RDONLY, 0777);
        if (test < 0){
            ML_fixStringField(fileName);
            snprintf(sql, sizeof(sql), "Delete From media Where path = upper('%s');", path);
            int retValue = sqlite3_prepare_v2(db, sql, -1, &del, 0);
            if (retValue != SQLITE_OK)
                continue;
            sqlite3_step(del);
            sqlite3_finalize(del);
        }else{
            sceIoClose(test);
        }
    }
    sqlite3_finalize(stmt);

    retValue = sqlite3_exec(db, "COMMIT", NULL, 0, &zErr);
    if (retValue != SQLITE_OK){
        sqlite3_free(zErr);
        ML_INTERNAL_closeDB();
        return ML_ERROR_SQL;
    }

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

    snprintf(sql, sizeof(sql), "VACUUM");
    int retValue = sqlite3_prepare_v2(db, sql, -1, &stmt, 0);
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
// Start a transaction:
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
int ML_startTransaction()
{
    if (ML_INTERNAL_openDB(dbDirectory, dbFileName))
        return ML_ERROR_OPENDB;

    int retValue = sqlite3_exec(db, "BEGIN TRANSACTION", NULL, 0, &zErr);
    if (retValue != SQLITE_OK){
        sqlite3_free(zErr);
        ML_INTERNAL_closeDB();
        return ML_ERROR_SQL;
    }
    transaction = 1;
    return 0;
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Commit transaction:
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
int ML_commitTransaction()
{
    int retValue = sqlite3_exec(db, "COMMIT", NULL, 0, &zErr);
    if (retValue != SQLITE_OK){
        sqlite3_free(zErr);
        transaction = 0;
        ML_INTERNAL_closeDB();
        return ML_ERROR_SQL;
    }

    transaction = 0;
    ML_INTERNAL_closeDB();
    return 0;
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Return 1 if we are in a transaction:
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
int ML_inTransaction()
{
    return transaction;
}
