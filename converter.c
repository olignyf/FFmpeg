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
FILE * g_report = NULL;

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
    char * name;
    int ret;
    //uint64_t childSize = 0;

    while (item)
    {
        FILE_ENTRY * node = (FILE_ENTRY*)item->client;

        if (node)
        {
            if (count >= 1) fputs(",", g_report);
            fputs("{", g_report);

            fputs("\"isDir\":", g_report);
            if (node->bIsDir) fputs("true", g_report);
            else fputs("false", g_report);

            fputs(",\"name\":\"", g_report);
            if (item->name.buffer) fputs(item->name.buffer, g_report);
            else fputs(item->name.fixed, g_report);
            fputs("\"", g_report);

            if (node->bIsDir == FALSE && C_strendstr(szLargeBuffer, "manifest"))
            {
                szLargeBuffer[szLargeBufferSize-1] = '\0';
                if (item->name.buffer) strncpy(szLargeBuffer, item->name.buffer, szLargeBufferSize-1);
                else strncpy(szLargeBuffer, item->name.fixed, szLargeBufferSize-1);
                // TODO open manifest here if needed
            }

            szLargeBuffer[0] = '\0';
            strftime(szLargeBuffer, 100, "%d %b %Y %H:%M:%S GMT", gmtime(&node->ctime));
            fputs(",\"ctime\":\"", g_report);
            fputs(szLargeBuffer, g_report);
            fputs("\"", g_report);

            szLargeBuffer[0] = '\0';
            strftime(szLargeBuffer, 100, "%d %b %Y %H:%M:%S GMT", gmtime(&node->mtime));
            fputs(",\"mtime\":\"", g_report);
            fputs(szLargeBuffer, g_report);
            fputs("\"", g_report);

            if (node->bIsDir && item->childs != NULL)
            {
                fprintf(g_report, ",\"childs\": [\n");

                PrintRecursive(item->childs, level+1, szLargeBuffer, szLargeBufferSize);

                fprintf(g_report, "]");
            } // complete before continuing bellow to get node size
        
            name = flexget(&item->name);

            fputs(",\"size\":\"", g_report);
            snprintf(szLargeBuffer, 100, "%"PRIu64, node->size);
            fputs(szLargeBuffer, g_report);
            fputs("\"", g_report);
            
            if (node->bIsDir == FALSE)
            {
               if (C_strendstr(name, ".ori"))
               {
                 fputs(",\"status\":\"original, keeping untouched\"", g_report);
               }
               else
               {
                  // will probe 
                  if (node->size > 1024)
                  {
                    FILE *tsCheck = fopen(name, "rb");
                    if (tsCheck)
                    {
                      int status;
                      unsigned char buffer[188*2];
                      char output_filename[CONVERTER_MAX_PATH];
                      char tmp_filename[CONVERTER_MAX_PATH];
                      memset(buffer,0,sizeof(buffer));
                      size_t size_read = fread(buffer,1,sizeof(buffer), tsCheck);
                      if (buffer[0] == 0x47 && buffer[188] == 0x47)
                      {
                         printf("YES ts file\n");
                         
                         char command[1024+CONVERTER_MAX_PATH] = "";
#if defined(_MSC_VER) || defined(WIN32)
                         snprintf(command, sizeof(command) - 1, "C:\\ffmpeg\\bin\\ffmpeg.exe -t 30  -i \"%s\" test.wav -loglevel 56 -y > /dev/null", name); 
#else
                         snprintf(command, sizeof(command) - 1, "./ffmpeg -t 30  -i \"%s\" test.wav -loglevel 56 -y > /dev/null", name); 
#endif
                         // -t 30 to limit to 30 sec worth of input
                         // - y force yes overwrite
                         unsigned int output_size = 0xFFFF;
                         char * output = malloc(output_size*sizeof(char));
                         output[0] = output[output_size-1] = '\0';
                         int stderrOnly = 1;
                         printf("executing cmd %s\n", command);
                         ret = C_System2(command, output, output_size, &status, stderrOnly);
      //               
                         if (status)
                         {
                            printf("cmd status %d\n", status);
                         }
                         printf("cmd %s\n", output);
                         if (strstr(output, "PES packet size mismatch"))
                         {
                            printf("YES broken\n");
                            
                            uint64_t nUserAvailable = 0, nRootAvailable = 0, nTotalCapacity = 0;
                            ret = getDiskSpace(g_directory, &nUserAvailable, &nRootAvailable, &nTotalCapacity);
                            printf("free space %"PRIu64"\n",nUserAvailable);
                            
                            strcpy(output_filename, name);
                            strcat(output_filename, "-fixed.ts");
                            strcpy(tmp_filename, name);
                            strcat(tmp_filename, ".ori");
                            snprintf(command, sizeof(command) - 1, "./tsmuxer -i \"%s\" -o \"%s\" --legacy-audio true", name, output_filename); 
                            printf("command %s\n", command);
                            
                            stderrOnly = 0;
                            ret = C_System2(command, output, output_size, &status, stderrOnly);
                            if (ret & status == 0)
                            {
                               printf("strlen of output %zu, or 0x%zX\n", strlen(output), strlen(output));
                               int outputStringMaxedOut = output_size -1 == strlen(output);
                               if (strstr(output, "Completed without errors") || outputStringMaxedOut)
                               {
                                 printf("Success converting\n");
                                 printf("outputStringMaxedOut %d\n", outputStringMaxedOut);
                                 ret = C_MoveFileEx(name, tmp_filename, TOOLBOX_OVERWRITE_DESTINATION);
                                 if (ret >= 1)
                                 {
                                   ret = C_MoveFileEx(output_filename, name, 0);
                                   if (ret >= 1)
                                   {
                                      printf("Success replacing files\n");
                                      fprintf(g_report, ",\"status\":\"success converting\"", status);
                                   }
                                   else
                                   {  // error
                                      ret = C_MoveFileEx(tmp_filename, name, TOOLBOX_OVERWRITE_DESTINATION); // restore
                                      printf("Restoring after error\n");
                                      fprintf(g_report, ",\"status\":\"error failed to move files after converting\"");
                                   }
                                 }
                                 else
                                 {
                                    fprintf(g_report, ",\"status\":\"error failed to rename original file after converting\"");
                                 }
                               }
                               else
                               {
                                  printf("There might have been a problem during convertion, output: %s\n", output);
                                  fprintf(g_report, ",\"status\":\"error converting, see converter.log for details\"");
                               }
                            }
                            else
                            {
                               printf("Error converting status = %d\n", status);
                               printf("\noutput:%s\n", output);
                               fprintf(g_report, ",\"status\":\"error converting, status %d\"", status);
                            }
                         }
                         else
                         {
                            printf("not broken\n");
                            fputs(",\"status\":\"TS file not broken, keeping untouched\"", g_report);
                         }
                      }
                      else
                      {
                         //printf("NOT ts file\n");
                         fputs(",\"status\":\"not TS file\"", g_report);
                      }
                      fclose(tsCheck);
                    }
                  }
               }
            }
            fputs("}\n", g_report);
            
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
        printf("Usage: \nconverter <directory>\n");
        exit(1);
    }
    
    if (strlen(argv[1]) > CONVERTER_MAX_PATH)
    {
        printf("Error: to large directory string length\n");
        exit(2);
    }
    
    strcpy(g_directory, argv[1]); 

    szLargeBuffer[sizeof(szLargeBuffer)-1] = '\0';
    
    // to hold directory structure
    genericTree_Constructor(&genericTree);
    
    g_report = fopen("report.txt", "wb");

    iret = traverseDir(g_directory, fileEntryCallback, &genericTree.top, &totalSize, FALSE);
    if (iret <= 0)
    {
        printf("Failed traverseDir path(%s) with error(%d)\n", g_directory, iret);
    }

    fputs("{\"rootPath\":\"", g_report); fputs(g_directory, g_report); fputs("\"", g_report);
    fprintf(g_report, ",\"content\": [");
    PrintRecursive(genericTree.top.childs, 0, szLargeBuffer, sizeof(szLargeBuffer));
    fprintf(g_report, "]}");

    genericTree_Destructor(&genericTree);
    fclose(g_report);
    return 0;
}

