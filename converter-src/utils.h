// ~~~~~~~~~~~~~ Makito API ~~~~~~~~~~~~~
//    REST API using fastcgi 
// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
//
// utils.h, copied from cli/utils.h
// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~


#if !defined (UTILS_H_INCLUDED)
#define UTILS_H_INCLUDED

#include <time.h>
#include <inttypes.h>

typedef struct tagFILE_ENTRY
{
    int  nLength;
    int bIsDir;
    char *szName;
    unsigned long duration; // seconds
    char szDuration[40]; // formatted
    char szStatus[20];
    char szType[5]; // TS, MP4
    time_t ctime;
    time_t mtime;
    uint64_t size;
} FILE_ENTRY;


#endif  /* UTILS_H_INCLUDED */

