// Written 10.Apr.2015

#include <stdio.h>
#include <errno.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>

#if ( defined(_MSC_VER) )
#   include "dirent.h" // local implementation by Kevlin Henney (kevlin@acm.org, kevlin@curbralan.com)
#else
#   include <dirent.h>
#endif

#include "toolbox.h"
#include "toolbox-basic-types.h"
#include "toolbox-filesystem.h"


#if defined(_MSC_VER)

const char * GetBaseName(const char * fullpath)
{
    char * strRet = strrchr(fullpath, '\\');
    if (strRet == NULL) return fullpath;

    return strRet+1;
}
#endif

int traverseDir(const char* directory, fileEntryCallback_func f_callback, void * opaque1, void * opaque2, int includeHidden)
{
    DIR *pRecordDir;
    struct dirent *pEntry;
    struct stat FStat;
    const char *pszBaseName;
    char szFullPathName[512];

    FILE_ENTRY curEntry;

    if (directory == NULL) return -1;
    if (f_callback == NULL) return -2;

    printf("Called for dir \"%s\".\n", directory);
    pRecordDir = opendir(directory);
    if (pRecordDir == NULL)
    {
        return -10;
    }

    while ((pEntry = readdir(pRecordDir)) != NULL)
    {
        pszBaseName = GetBaseName(pEntry->d_name);
        if (pszBaseName != NULL && (includeHidden || pszBaseName[0] != '.'))
        {
            memset(&curEntry, 0, sizeof(curEntry));
#if defined(_MSC_VER)
			snprintf(szFullPathName, sizeof(szFullPathName) - 1, "%s\\%s", directory, pszBaseName);
#else
			snprintf(szFullPathName, sizeof(szFullPathName) - 1, "%s/%s", directory, pszBaseName);
#endif
            if (pEntry->d_type == DT_REG)
            {
                // C_GetFileSize(szFullPathName, &curEntry.size);
            }
            else if (pEntry->d_type == DT_DIR)
            {
                curEntry.bIsDir = 1;
            }
            else if (pEntry->d_type == DT_UNKNOWN)
            {
#if ( defined(_MSC_VER) )
                if (stat(szFullPathName, &FStat) == 0)
#else
                if (lstat(szFullPathName, &FStat) == 0)
#endif
                {
                    if (S_ISDIR(FStat.st_mode))
                    {
                        curEntry.bIsDir = 1;
                    }
                }
                else
                {
                    //DBGPRINTEX(g_hDbg, DBG_LEVEL_WARNING, "lstat failed for \"%s\", errno=%d.\n", szFullPathName);
                }

            }

            curEntry.szName = pEntry->d_name;

            f_callback(szFullPathName, &curEntry, opaque1, opaque2, includeHidden);
        }

	}

    closedir (pRecordDir);
    return 1;
}
