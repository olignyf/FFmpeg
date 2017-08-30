// ./configure --enable-x86asm  --enable-yasm --disable-ffserver --disable-avdevice --disable-doc --disable-ffplay --disable-ffprobe --disable-shared --disable-bzlib --disable-libopenjpeg --disable-iconv --disable-zlib --enable-debug
// instream.exe --read file:///c:\\dev\\0.ts --vfvideo-mode 2
#include <stdlib.h>
#include <stdio.h>
#include <time.h>
#include <string.h>
#include <sys/stat.h>
#include "converter-src/toolbox.h"

#include <string.h>
#include <strings.h> // strcasecmp
#include <stdlib.h> 
#include <stdio.h>
#include "libavformat/avformat.h"
/* FIXME: those are internal headers, ffserver _really_ shouldn't use them */

#include "libavutil/avstring.h"
#include "libavutil/intreadwrite.h"
#include "libavutil/mathematics.h"
#include "libavutil/random_seed.h"
#include "libavutil/rational.h"
#include "libavutil/parseutils.h"
#include "libavutil/opt.h"
#include "libavutil/time.h"

#include "converter_opt.h"

#include <stdarg.h>
#if HAVE_UNISTD_H
#include <unistd.h>
#endif
#include <fcntl.h>
#if HAVE_POLL_H
#include <poll.h>
#endif
#include <errno.h>
#include <time.h>
#include <signal.h>

#include "cmdutils.h"
#include <inttypes.h>


#define CONVERTER_MAX_PATH 10000


char g_directory[CONVERTER_MAX_PATH] = "";

// will insert into childs of opaque1 treeItem_T
// opaque2 is a uint64_t size to add with our own size and any of our children
static int fileEntryCallback(const char *name, const FILE_ENTRY *entry, void * opaque1, void * opaque2, int includeHidden)
{
    int iret;
    treeItem_T * newElement = NULL;
    treeItem_T * level = (treeItem_T *)opaque1;
    uint64_t * pCurrentLevelSize = (uint64_t*)opaque2;
    uint64_t childsSize = 0;
    genericTree_T * tree;
    int nLength;

    if (opaque1 == NULL) return -3;
    if (entry == NULL) return -2;
    if (name == NULL) return -1;

    tree = (genericTree_T *)level->tree;

    //printf("name:%s\n", name);
    if (entry->bIsDir && strcasecmp(name, "/mnt/storage1/recordings/tmp") == 0)
    {
        return 0;
    }

    nLength = strlen(name);
    if (strstr(name, "/mnt/storage1/recordings/") != NULL
     && strcasecmp(&name[nLength - 4], ".mp4") != 0
     && strcasecmp(&name[nLength - 3], ".ts") != 0
     && !entry->bIsDir
     )
    {
        return 0;
    }

    iret = genericTree_Insert(tree, level, name, 0, NULL, &newElement);
    if ( iret > 0 && newElement )
    {
        FILE_ENTRY * newEntry = (FILE_ENTRY *)malloc(sizeof(FILE_ENTRY));
        if ( newEntry == NULL )
        {
            genericTree_Delete(tree, newElement);
            return TOOLBOX_ERROR_MALLOC;
        }
        memcpy(newEntry, entry, sizeof(FILE_ENTRY));
        newEntry->szName = (char*)malloc(strlen(entry->szName)+1);
        if ( newEntry->szName == NULL )
        {
            genericTree_Delete(tree, newElement);
            return TOOLBOX_ERROR_MALLOC;
        }

        // get file stats
        //if ( !entry->bIsDir )
        {
            struct stat fileStats;
            if (stat(name, &fileStats) == 0)
            {
                newEntry->ctime = fileStats.st_ctime;
                newEntry->mtime = fileStats.st_mtime;
                newEntry->size = fileStats.st_size;
            }
        }

        newElement->client = newEntry;
        strcpy(newEntry->szName, entry->szName);
        newElement->parent = level;

        if ( entry->bIsDir )
        {
             traverseDir(name, fileEntryCallback, newElement, &childsSize, includeHidden);
             ((FILE_ENTRY*)newElement->client)->size = childsSize;
        }

        if (pCurrentLevelSize)
        {
             *pCurrentLevelSize += ((FILE_ENTRY*)newElement->client)->size;
        }

        return 1;
    }

    return -10;
}

static void PrintRecursive(treeItem_T * item, int level, char * szLargeBuffer, unsigned int szLargeBufferSize)
{
    int count = 0;
    //uint64_t childSize = 0;

    while (item)
    {
        FILE_ENTRY * node = (FILE_ENTRY*)item->client;

        if (node)
        {
            if (count >= 1) fputs(",", stdout);
            fputs("{", stdout);

            fputs("\"isDir\":", stdout);
            if (node->bIsDir) fputs("true", stdout);
            else fputs("false", stdout);

            fputs(",\"name\":\"", stdout);
            if (item->name.buffer) fputs(item->name.buffer, stdout);
            else fputs(item->name.fixed, stdout);
            fputs("\"", stdout);

            if (node->bIsDir == FALSE && C_strendstr(szLargeBuffer, "manifest"))
            {
                szLargeBuffer[szLargeBufferSize-1] = '\0';
                if (item->name.buffer) strncpy(szLargeBuffer, item->name.buffer, szLargeBufferSize-1);
                else strncpy(szLargeBuffer, item->name.fixed, szLargeBufferSize-1);

                // open manifest

            }

            szLargeBuffer[0] = '\0';
            strftime(szLargeBuffer, 100, "%d %b %Y %H:%M:%S GMT", gmtime(&node->ctime));
            fputs(",\"ctime\":\"", stdout);
            fputs(szLargeBuffer, stdout);
            fputs("\"", stdout);

            szLargeBuffer[0] = '\0';
            strftime(szLargeBuffer, 100, "%d %b %Y %H:%M:%S GMT", gmtime(&node->mtime));
            fputs(",\"mtime\":\"", stdout);
            fputs(szLargeBuffer, stdout);
            fputs("\"", stdout);

            if (node->bIsDir && item->childs != NULL)
            {
                printf(",\"childs\": [\n");

                PrintRecursive(item->childs, level+1, szLargeBuffer, szLargeBufferSize);

                printf("]\n");
            }

            fputs(",\"size\":\"", stdout);            
            snprintf(szLargeBuffer, 100, "%"PRIu64, node->size);
            fputs(szLargeBuffer, stdout);
            fputs("\"", stdout);

            
            fputs("}\n", stdout);
            
            if (node->size > 1024)
            {
				char * name = flexget(&item->name);
              FILE *tsCheck = fopen(name, "rb");
              if (tsCheck)
              {
                int ret, status;
                unsigned char buffer[188*2];
                char output_filename[1024];
                memset(buffer,0,sizeof(buffer));
                size_t size_read = fread(buffer,1,sizeof(buffer),tsCheck);
                if (buffer[0] == 0x47 && buffer[188] == 0x47)
                {
                   printf("YES ts file\n");
                   
				   char command[1024] = "";
#if defined(_MSC_VER) || defined(WIN32)
				   snprintf(command, sizeof(command) - 1, "C:\\ffmpeg\\bin\\ffmpeg.exe -t 30  -i \"%s\" out.wav -loglevel 56 -y > /dev/null", name); 
#else
				   snprintf(command, sizeof(command) - 1, "ffmpeg -t 30  -i \"%s\" out.wav -loglevel 56 -y > /dev/null", name); 
#endif
				   // -t 30 to limit to 30 sec worth of input
				   // - y force yes overwrite
                   char output[0xFFFF] = "";
				   int stderrOnly = 1;
				   ret = C_System2(command, output, sizeof(output), &status, stderrOnly);
//				   
                   if (strstr(output, "PES packet size mismatch"))
                   {
                      printf("YES broken\n");
	  			      uint64_t nUserAvailable = 0, nRootAvailable = 0, nTotalCapacity = 0;
                      ret = getDiskSpace(g_directory, &nUserAvailable, &nRootAvailable, &nTotalCapacity);
                      printf("free space %"PRIu64"\n",nUserAvailable);
                      stderrOnly = 0;
				   strcpy(output_filename, name);
				   strcat(output_filename, "-fixed.ts");
                      snprintf(command, sizeof(command) - 1, "./tsmuxer -i \"%s\" -o \"%s\" --legacy-audio true", name, output_filename); 
                      ret = C_System2(command, output, sizeof(output), &status, stderrOnly);
                      if (ret & status == 0)
                      {
                         printf("Success converting\n");
                      }
                      else
                      {
                         printf("Error converting status = %d\n", status);
                         printf("\noutput:%s\n", output);
                      }  
                   }
				   else
				   {
					   printf("not broken\n");
				   }
                }
                else
                {
                   printf("NO ts file\n");
                }
              }
            }
            
            count++;
        }

        item = item->next;
    }
}


int main(int argc, char ** argv)
{
    char szLargeBuffer[CONVERTER_MAX_PATH] = "";
    genericTree_T genericTree;
    uint64_t totalSize = 0;
    int iret, ret;
    
    //show_banner(argc, argv, options);

    if (argc <= 1)
    {
        printf("Usage: \nconverter <directory> %d\n", argc);
        exit(0);
    }
    
    if (strlen(argv[1]) > CONVERTER_MAX_PATH)
    {
        printf("Error: to large directory string length\n");
        exit(0);
    }
    
    strcpy(g_directory, argv[1]); 
    

    szLargeBuffer[sizeof(szLargeBuffer)-1] = '\0';
    genericTree_Constructor(&genericTree);

    iret = traverseDir(g_directory, fileEntryCallback, &genericTree.top, &totalSize, FALSE);
    if (iret <= 0)
    {
        printf("Failed traverseDir path(%s) with error(%d)\n", g_directory, iret);
    }

    fputs("\"rootPath\":\"", stdout); fputs(g_directory, stdout); fputs("\"", stdout);
    printf(",\"content\": [");
    PrintRecursive(genericTree.top.childs, 0, szLargeBuffer, sizeof(szLargeBuffer));
    printf("]");

    genericTree_Destructor(&genericTree);
    
    return 0;
}

