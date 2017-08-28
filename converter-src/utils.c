/**********************************************************************
               C O P Y R I G H T    ( C )    2008, 2009   B Y
                         HaiVision Systems Inc.

    Information  contained  herein is proprietary to HaiVision
    Systems Inc.  Any use without the prior written consent
    of HaiVision Systems Inc. is expressly prohibited.
**********************************************************************
  utils.c, copied from cli/utils.c
**********************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include <errno.h>
#include <string.h>
#include <time.h>
#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/msg.h>
#include <unistd.h>
#include <dirent.h>

#include "mxtools.h"
//#include "globals.h"
#include "safestrings.h"
#include "utils.h"
#include "memdbg.h"

#define SECS_PER_MINUTE (60)
#define SECS_PER_HOUR   (SECS_PER_MINUTE * 60)
#define SECS_PER_DAY    (SECS_PER_HOUR   * 24)
#define SECS_PER_MONTH  (SECS_PER_DAY   * 30) /* No need to take into diffrent month duration since this only to compare two file names */
#define SECS_PER_YEAR   (SECS_PER_MONTH  * 12) /* No need to take into account bisextile years for simple sorting */


extern HDBG g_hDbg;

static const int PIN_ACTIVE_ON_TOP = 0;

int CompareFileEntries(const void *First, const void *Second)
{
    FILE_ENTRY *pFirstFile  = (FILE_ENTRY*)First;
    FILE_ENTRY *pSecondFile = (FILE_ENTRY*)Second;
    int iRetCode;

    iRetCode = strcasecmp(pFirstFile->szName, pSecondFile->szName);
    // printf("Comparing \"%s\" to \"%s\" rc=%d\n", (const char*)*(char**)First, (const char*)*(char**)Second, iRetCode);
    return iRetCode;
}



int CompareSnapShotFileEntriesR(const void *First, const void *Second)
{
    FILE_ENTRY *pFirstFile  = (FILE_ENTRY*)First;
    FILE_ENTRY *pSecondFile = (FILE_ENTRY*)Second;

    return  (pSecondFile->mtime - pFirstFile->mtime);
}

int CompareSnapShotFileEntries(const void *First, const void *Second)
{
    FILE_ENTRY *pFirstFile  = (FILE_ENTRY*)First;
    FILE_ENTRY *pSecondFile = (FILE_ENTRY*)Second;

    return  (pFirstFile->mtime - pSecondFile->mtime);
}



static BOOL _GetRecordingTimeFromName(const char *szFileName, ULONG *pnSeconds)
{
    char    szName[256];
    char    *pToken;
    char    *szValue;
    ULONG   nYear;
    ULONG   nMonth;
    ULONG   nDate;
    ULONG   nHours;
    ULONG   nMinutes;
    ULONG   nSeconds;
    ULONG   nTotalSecs;

    if (strlen(szFileName) >= sizeof(szName))
    {
        DBGPRINTEX(g_hDbg, DBG_LEVEL_NOTICE, "File name \"%s\" is too long.\n", szFileName);
        return FALSE;
    }

    strcpy(szName, szFileName);
    szValue = strtok_r(szName, "-", &pToken);
    if (szValue == NULL)
    {
        DBGPRINTEX(g_hDbg, DBG_LEVEL_NOTICE, "strtok_r failed.\n");
        return FALSE;
    }

#if 0 /* Prefix for recordings is user configurable but we won't allow dashes */
    if (strcmp(szValue, "snap") != 0)
    {
        DBGPRINTEX(g_hDbg, DBG_LEVEL_NOTICE, "value \"%s\" is not snap.\n", szValue);
        return FALSE;
    }
#endif

    szValue = strtok_r(NULL, "-", &pToken);
    if (szValue == NULL)
    {
        DBGPRINTEX(g_hDbg, DBG_LEVEL_NOTICE, "strtok_r failed.\n");
        return FALSE;
    }

    if (StringToDecULONG(szValue, &nYear) == FALSE)
    {
        DBGPRINTEX(g_hDbg, DBG_LEVEL_NOTICE, "value \"%s\" is not a valid year for filename %s.\n", szValue, szFileName);
        return FALSE;
    }

    if (nYear < 2000)
    {
        DBGPRINTEX(g_hDbg, DBG_LEVEL_NOTICE, "Year %lu is less than 2000.\n", nYear);
        return FALSE;
    }

    szValue = strtok_r(NULL, "-", &pToken);
    if (szValue == NULL)
    {
        DBGPRINTEX(g_hDbg, DBG_LEVEL_NOTICE, "strtok_r failed.\n");
        return FALSE;
    }

    if (StringToDecULONG(szValue, &nMonth) == FALSE)
    {
        DBGPRINTEX(g_hDbg, DBG_LEVEL_NOTICE, "value \"%s\" is not a valid month.\n", szValue);
        return FALSE;
    }


    szValue = strtok_r(NULL, "-", &pToken);
    if (szValue == NULL)
    {
        DBGPRINTEX(g_hDbg, DBG_LEVEL_NOTICE, "strtok_r failed.\n");
        return FALSE;
    }

    if (StringToDecULONG(szValue, &nDate) == FALSE)
    {
        DBGPRINTEX(g_hDbg, DBG_LEVEL_NOTICE, "value \"%s\" is not a valid date.\n", szValue);
        return FALSE;
    }


    szValue = strtok_r(NULL, "h", &pToken);
    if (szValue == NULL)
    {
        DBGPRINTEX(g_hDbg, DBG_LEVEL_NOTICE, "strtok_r failed.\n");
        return FALSE;
    }

    if (StringToDecULONG(szValue, &nHours) == FALSE)
    {
        DBGPRINTEX(g_hDbg, DBG_LEVEL_NOTICE, "value \"%s\" is not a valid hour.\n", szValue);
        return FALSE;
    }


    szValue = strtok_r(NULL, "m", &pToken);
    if (szValue == NULL)
    {
        DBGPRINTEX(g_hDbg, DBG_LEVEL_NOTICE, "strtok_r failed.\n");
        return FALSE;
    }

    if (StringToDecULONG(szValue, &nMinutes) == FALSE)
    {
        DBGPRINTEX(g_hDbg, DBG_LEVEL_NOTICE, "value \"%s\" is not a valid minute.\n", szValue);
        return FALSE;
    }


    szValue = strtok_r(NULL, "s", &pToken);
    if (szValue == NULL)
    {
        DBGPRINTEX(g_hDbg, DBG_LEVEL_NOTICE, "strtok_r failed.\n");
        return FALSE;
    }

    if (StringToDecULONG(szValue, &nSeconds) == FALSE)
    {
        DBGPRINTEX(g_hDbg, DBG_LEVEL_NOTICE, "value \"%s\" is not a valid second.\n", szValue);
        return FALSE;
    }

    nTotalSecs = (nYear - 2000) * SECS_PER_YEAR;
    nTotalSecs += (nMonth  * SECS_PER_MONTH);
    nTotalSecs += (nDate   * SECS_PER_DAY);
    nTotalSecs += (nHours  * SECS_PER_HOUR);
    nTotalSecs += (nMinutes * SECS_PER_MINUTE);
    nTotalSecs += nSeconds;

    //DBGPRINTEX(g_hDbg, DBG_LEVEL_ERROR, "Returning %d seconds for file name \"%s\".\n", nTotalSecs, szFileName);
    *pnSeconds = nTotalSecs;
    return TRUE;
}


// input: 5h34m22s
// or   5m10s
// or   40s
// or   40
// output: amount of seconds
BOOL GetDurationFromString(IN const char *szDuration, OUT ULONG *pnSeconds)
{
    char    szName[256];
    char    *pToken;
    char    *szValue;
    ULONG   nHours = 0;
    ULONG   nMinutes = 0;
    ULONG   nSeconds = 0;
    ULONG   nTotalSecs = 0;

    if (strlen(szDuration) >= sizeof(szName))
    {
        DBGPRINTEX(g_hDbg, DBG_LEVEL_NOTICE, "File name \"%s\" is too long.\n", szDuration);
        return FALSE;
    }

    strcpy(szName, szDuration);
    szValue = strchr(szName, 's');
    if (szValue == NULL && szDuration[0] != '\0')
    {
        StringToDecULONG(szDuration, &nSeconds);
        *pnSeconds = nSeconds;
        //DBGPRINTEX(g_hDbg, DBG_LEVEL_NOTICE, "input(%s) output(%lu).\n", szDuration, *pnSeconds);
        return TRUE;
    }

    szValue = strtok_r(NULL, "h", &pToken);
    if (szValue != NULL)
    {
        if (StringToDecULONG(szValue, &nHours) == FALSE)
        {
            DBGPRINTEX(g_hDbg, DBG_LEVEL_NOTICE, "value \"%s\" is not a valid hour.\n", szValue);
            return FALSE;
        }
    }

    szValue = strtok_r(NULL, "m", &pToken);
    if (szValue != NULL)
    {
        if (StringToDecULONG(szValue, &nMinutes) == FALSE)
        {
            DBGPRINTEX(g_hDbg, DBG_LEVEL_NOTICE, "value \"%s\" is not a valid minute.\n", szValue);
            return FALSE;
        }
    }

    szValue = strtok_r(NULL, "s", &pToken);
    if (szValue == NULL)
    {
        DBGPRINTEX(g_hDbg, DBG_LEVEL_NOTICE, "strtok_r failed.\n");
        return FALSE;
    }

    if (StringToDecULONG(szValue, &nSeconds) == FALSE)
    {
        DBGPRINTEX(g_hDbg, DBG_LEVEL_NOTICE, "value \"%s\" is not a valid second.\n", szValue);
        return FALSE;
    }

    nTotalSecs = (nHours  * SECS_PER_HOUR);
    nTotalSecs += (nMinutes * SECS_PER_MINUTE);
    nTotalSecs += nSeconds;

    //DBGPRINTEX(g_hDbg, DBG_LEVEL_DEBUG, "Returning %d seconds from szDuration \"%s\".\n", nTotalSecs, szDuration);
    *pnSeconds = nTotalSecs;
    return TRUE;
}


int CompareRecordingFileEntries(const void *First, const void *Second)
{
    FILE_ENTRY *pFirstFile  = (FILE_ENTRY*)First;
    FILE_ENTRY *pSecondFile = (FILE_ENTRY*)Second;
    int iRetCode;
    ULONG nFirstTimeValue  = 0;
    ULONG nSecondTimeValue = 0;

//  DBGPRINTEX(g_hDbg, DBG_LEVEL_INFO, "Comparing \"%s\" with mtime=%lu and status=\"%s\" with \"%s\" with mtime=%lu and status=\"%s\".\n",
//             pFirstFile->szName, pFirstFile->mtime, pFirstFile->szStatus,
//             pSecondFile->szName, pSecondFile->mtime, pSecondFile->szStatus);

    if ((pFirstFile->mtime != 0) &&
        (pSecondFile->mtime != 0))
    {
        if (pFirstFile->mtime > pSecondFile->mtime)
        {
            return 1;
        }
        else if (pFirstFile->mtime < pSecondFile->mtime)
        {
            return -1;
        }
        else
        {
            return 0;
        }
    }

    //DBGPRINTEX(g_hDbg, DBG_LEVEL_DEBUG, "Called!\n");
    if (_GetRecordingTimeFromName(pFirstFile->szName, &nFirstTimeValue) &&
        _GetRecordingTimeFromName(pSecondFile->szName, &nSecondTimeValue))
    {
        return (nFirstTimeValue - nSecondTimeValue);
    }

    iRetCode = strcasecmp(pFirstFile->szName, pSecondFile->szName);
    // printf("Comparing \"%s\" to \"%s\" rc=%d\n", (const char*)*(char**)First, (const char*)*(char**)Second, iRetCode);
    return iRetCode;
}


int CompareRecordingFileEntriesR(const void *First, const void *Second)
{
    FILE_ENTRY *pFirstFile  = (FILE_ENTRY*)First;
    FILE_ENTRY *pSecondFile = (FILE_ENTRY*)Second;
    char * firstPtr, * secondPtr;
    int iRetCode;
    ULONG nFirstTimeValue  = 0;
    ULONG nSecondTimeValue = 0;

//  DBGPRINTEX(g_hDbg, DBG_LEVEL_INFO, "Comparing \"%s\" with mtime=%lu and status=\"%s\" with \"%s\" with mtime=%lu and status=\"%s\".\n",
//             pFirstFile->szName, pFirstFile->mtime, pFirstFile->szStatus,
//             pSecondFile->szName, pSecondFile->mtime, pSecondFile->szStatus);

    if ((pFirstFile->mtime != 0) &&
        (pSecondFile->mtime != 0))
    {

        if (pFirstFile->mtime < pSecondFile->mtime)
        {
            return 1;
        }
        else if (pFirstFile->mtime > pSecondFile->mtime)
        {
            return -1;
        }
        else
        {
            return 0;
        }
    }

    firstPtr = strrchr(pFirstFile->szName, '-');
    if (firstPtr - 12 > &pFirstFile->szName[0])
    {   firstPtr -= 12;
    }
    else
    {   firstPtr = NULL;
    }

    secondPtr = strrchr(pSecondFile->szName, '-');
    if (secondPtr - 12 > &pSecondFile->szName[0])
    {   secondPtr -= 12;
    }
    else
    {   secondPtr = NULL;
    }

    if (firstPtr == NULL && secondPtr != NULL)
    {   return 1;
    }
    else if (firstPtr != NULL && secondPtr == NULL)
    {   return -1;
    }
    else if (firstPtr == NULL && secondPtr == NULL)
    {
        iRetCode = strcasecmp(pFirstFile->szName, pSecondFile->szName);
        // printf("Comparing \"%s\" to \"%s\" rc=%d\n", pFirstFile->szName, pSecondFile->szName, iRetCode);
        return iRetCode;
    }

    //DBGPRINTEX(g_hDbg, DBG_LEVEL_DEBUG, "Called!\n");
    if (_GetRecordingTimeFromName(firstPtr, &nFirstTimeValue) &&
        _GetRecordingTimeFromName(secondPtr, &nSecondTimeValue))
    {//
        //printf("Comparing \"%s\" to \"%s\" %lu with %lu\n", pFirstFile->szName, pSecondFile->szName, nFirstTimeValue, nSecondTimeValue);
        return (nSecondTimeValue - nFirstTimeValue);
    }

    //printf("falling back to file name between %s and %s\n", firstPtr, secondPtr);
    //printf("nFirstTimeValue(%lu) nSecondTimeValue(%lu)\n", nFirstTimeValue, nSecondTimeValue);
    iRetCode = strcasecmp(pFirstFile->szName, pSecondFile->szName);
    // printf("Comparing \"%s\" to \"%s\" rc=%d\n", pFirstFile->szName, pSecondFile->szName, iRetCode);
    return iRetCode;
}


int CompareRecordingFileBySize(const void *First, const void *Second)
{
    FILE_ENTRY *pFirstFile  = (FILE_ENTRY*)First;
    FILE_ENTRY *pSecondFile = (FILE_ENTRY*)Second;
    int iRetCode;

    DBGPRINTEX(g_hDbg, DBG_LEVEL_DEBUG, "Comparing %s recording \"%s\" of size %Lu with %s \"%s\" of size %Lu.\n",
            pFirstFile->bIsDir ? "Segmented" : "Regular",
            pFirstFile->szName, pFirstFile->size,
            pSecondFile->bIsDir ? "Segmented" : "Regular",
            pSecondFile->szName, pSecondFile->size);


    if (PIN_ACTIVE_ON_TOP)
    {
        if (strcmp(pFirstFile->szStatus, "Active") == 0 || strcmp(pFirstFile->szStatus, "Recording") == 0) return -1;
        if (strcmp(pSecondFile->szStatus, "Active") == 0 || strcmp(pSecondFile->szStatus, "Recording") == 0) return 1;
    }

    //DBGPRINTEX(g_hDbg, DBG_LEVEL_DEBUG, "Called!\n");
    if (pFirstFile->size != pSecondFile->size)
    {

        if (pFirstFile->size > pSecondFile->size) iRetCode = 1;
        else iRetCode = -1;

        //printf("Comparing \"%s\" to \"%s\" %lu with %lu, iRetCode(%d)\n", pFirstFile->szName, pSecondFile->szName, (unsigned long)pFirstFile->size/100, (unsigned long)pSecondFile->size/100, iRetCode);

        return iRetCode;
    }

    iRetCode = strcasecmp(pFirstFile->szName, pSecondFile->szName);

    return iRetCode;
}


int CompareRecordingFileBySizeR(const void *First, const void *Second)
{
    FILE_ENTRY *pFirstFile  = (FILE_ENTRY*)First;
    FILE_ENTRY *pSecondFile = (FILE_ENTRY*)Second;
    int iRetCode;

    DBGPRINTEX(g_hDbg, DBG_LEVEL_DEBUG, "Comparing %s recording \"%s\" of size %Lu with %s \"%s\" of size %Lu.\n",
               pFirstFile->bIsDir ? "Segmented" : "Regular",
               pFirstFile->szName, pFirstFile->size,
               pSecondFile->bIsDir ? "Segmented" : "Regular",
               pSecondFile->szName, pSecondFile->size);

    if (PIN_ACTIVE_ON_TOP)
    {
        if (strcmp(pFirstFile->szStatus, "Active") == 0 || strcmp(pFirstFile->szStatus, "Recording") == 0) return -1;
        if (strcmp(pSecondFile->szStatus, "Active") == 0 || strcmp(pSecondFile->szStatus, "Recording") == 0) return 1;
    }

    //DBGPRINTEX(g_hDbg, DBG_LEVEL_DEBUG, "Called!\n");
    if (pFirstFile->size != pSecondFile->size)
    {
        if (pFirstFile->size < pSecondFile->size) return 1;
        else return -1;
    }

    iRetCode = strcasecmp(pFirstFile->szName, pSecondFile->szName);
    // printf("Comparing \"%s\" to \"%s\" rc=%d\n", (const char*)*(char**)First, (const char*)*(char**)Second, iRetCode);
    return iRetCode;
}



int CompareRecordingFileByDuration(const void *First, const void *Second)
{
    FILE_ENTRY *pFirstFile  = (FILE_ENTRY*)First;
    FILE_ENTRY *pSecondFile = (FILE_ENTRY*)Second;
    int iRetCode;

    if (PIN_ACTIVE_ON_TOP)
    {
        if (strcmp(pFirstFile->szStatus, "Active") == 0 || strcmp(pFirstFile->szStatus, "Recording") == 0) return -1;
        if (strcmp(pSecondFile->szStatus, "Active") == 0 || strcmp(pSecondFile->szStatus, "Recording") == 0) return 1;
    }

    //DBGPRINTEX(g_hDbg, DBG_LEVEL_DEBUG, "Called!\n");
    if (pFirstFile->duration != pSecondFile->duration)
    {
        return pFirstFile->duration - pSecondFile->duration;
    }

    iRetCode = strcasecmp(pFirstFile->szName, pSecondFile->szName);
    // printf("Comparing \"%s\" to \"%s\" rc=%d\n", (const char*)*(char**)First, (const char*)*(char**)Second, iRetCode);
    return iRetCode;
}


int CompareRecordingFileByDurationR(const void *First, const void *Second)
{
    FILE_ENTRY *pFirstFile  = (FILE_ENTRY*)First;
    FILE_ENTRY *pSecondFile = (FILE_ENTRY*)Second;
    int iRetCode;

    if (PIN_ACTIVE_ON_TOP)
    {
        if (strcmp(pFirstFile->szStatus, "Active") == 0 || strcmp(pFirstFile->szStatus, "Recording") == 0) return -1;
        if (strcmp(pSecondFile->szStatus, "Active") == 0 || strcmp(pSecondFile->szStatus, "Recording") == 0) return 1;
    }

    //DBGPRINTEX(g_hDbg, DBG_LEVEL_DEBUG, "Called!\n");
    if (pFirstFile->duration != pSecondFile->duration)
    {
        return pSecondFile->duration - pFirstFile->duration;
    }

    iRetCode = strcasecmp(pFirstFile->szName, pSecondFile->szName);
    // printf("Comparing \"%s\" to \"%s\" rc=%d\n", (const char*)*(char**)First, (const char*)*(char**)Second, iRetCode);
    return iRetCode;
}


int CompareRecordingFileByName(const void *First, const void *Second)
{
    FILE_ENTRY *pFirstFile  = (FILE_ENTRY*)First;
    FILE_ENTRY *pSecondFile = (FILE_ENTRY*)Second;
    int iRetCode;

    if (PIN_ACTIVE_ON_TOP)
    {
        if (strcmp(pFirstFile->szStatus, "Active") == 0 || strcmp(pFirstFile->szStatus, "Recording") == 0) return -1;
        if (strcmp(pSecondFile->szStatus, "Active") == 0 || strcmp(pSecondFile->szStatus, "Recording") == 0) return 1;
    }

    //DBGPRINTEX(g_hDbg, DBG_LEVEL_DEBUG, "Called!\n");

    iRetCode = strcasecmp(pFirstFile->szName, pSecondFile->szName);
    // printf("Comparing \"%s\" to \"%s\" rc=%d\n", (const char*)*(char**)First, (const char*)*(char**)Second, iRetCode);
    return iRetCode;
}

int CompareRecordingFileByNameR(const void *First, const void *Second)
{
    FILE_ENTRY *pFirstFile  = (FILE_ENTRY*)First;
    FILE_ENTRY *pSecondFile = (FILE_ENTRY*)Second;
    int iRetCode;

    if (PIN_ACTIVE_ON_TOP)
    {
        if (strcmp(pFirstFile->szStatus, "Active") == 0 || strcmp(pFirstFile->szStatus, "Recording") == 0) return -1;
        if (strcmp(pSecondFile->szStatus, "Active") == 0 || strcmp(pSecondFile->szStatus, "Recording") == 0) return 1;
    }

    //DBGPRINTEX(g_hDbg, DBG_LEVEL_DEBUG, "Called!\n");

    iRetCode = strcasecmp(pSecondFile->szName, pFirstFile->szName);
    // printf("Comparing \"%s\" to \"%s\" rc=%d\n", (const char*)*(char**)First, (const char*)*(char**)Second, iRetCode);
    return iRetCode;
}

int CompareRecordingFileByType(const void *First, const void *Second)
{
    FILE_ENTRY *pFirstFile  = (FILE_ENTRY*)First;
    FILE_ENTRY *pSecondFile = (FILE_ENTRY*)Second;
    int iRetCode;

    if (PIN_ACTIVE_ON_TOP)
    {
        if (strcmp(pFirstFile->szStatus, "Active") == 0 || strcmp(pFirstFile->szStatus, "Recording") == 0) return -1;
        if (strcmp(pSecondFile->szStatus, "Active") == 0 || strcmp(pSecondFile->szStatus, "Recording") == 0) return 1;
    }

    iRetCode = 0;

    char * pFirstFileExt = pFirstFile->szType;
    char * pSecondFileExt = pSecondFile->szType;

    //printf("comparing type %s (%s) vs %s (%s) ", pFirstFileExt, pFirstFile->szName, pSecondFileExt, pSecondFile->szName);

    if (pFirstFileExt && pSecondFileExt)
    {
        iRetCode = strcasecmp(pSecondFileExt, pFirstFileExt);
        if (iRetCode != 0)
        {
            //printf("returning %d\n", iRetCode);
            return iRetCode;
        }
    }

    if (pFirstFileExt && pSecondFileExt == NULL)
    {
        //printf("returning 1 because one NULL\n");
        return 1;
    }
    else if (pFirstFileExt == NULL && pSecondFileExt)
    {
        //printf("returning -1 because one NULL\n");
        return -1;
    }

    iRetCode = strcasecmp(pSecondFile->szName, pFirstFile->szName);
    //printf("returning %d from name compare\n", iRetCode);
    return iRetCode;
}

int CompareRecordingFileByTypeR(const void *First, const void *Second)
{
    FILE_ENTRY *pFirstFile  = (FILE_ENTRY*)First;
    FILE_ENTRY *pSecondFile = (FILE_ENTRY*)Second;
    int iRetCode;

    if (PIN_ACTIVE_ON_TOP)
    {
        if (strcmp(pFirstFile->szStatus, "Active") == 0 || strcmp(pFirstFile->szStatus, "Recording") == 0) return -1;
        if (strcmp(pSecondFile->szStatus, "Active") == 0 || strcmp(pSecondFile->szStatus, "Recording") == 0) return 1;
    }

    iRetCode = 0;

    char * pFirstFileExt = pFirstFile->szType;
    char * pSecondFileExt = pSecondFile->szType;

    if (pFirstFileExt && pSecondFileExt)
    {
        iRetCode = strcasecmp(pFirstFileExt, pSecondFileExt);
        if (iRetCode != 0) return iRetCode;
    }

    if (pFirstFileExt && pSecondFileExt == NULL)
    {
        return -1;
    }
    else if (pFirstFileExt == NULL && pSecondFileExt)
    {
        return 1;
    }

    iRetCode = strcasecmp(pFirstFile->szName, pSecondFile->szName);
    return iRetCode;
}

