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


// Followin UTF8 functions from 
// http://git.ozlabs.org/?p=ccan;a=blob_plain;f=ccan/json/json.c;hb=HEAD
/*
 * Validate a single UTF-8 character starting at @s.
 * The string must be null-terminated.
 *
 * If it's valid, return its length (1 thru 4).
 * If it's invalid or clipped, return 0.
 *
 * This function implements the syntax given in RFC3629, which is
 * the same as that given in The Unicode Standard, Version 6.0.
 *
 * It has the following properties:
 *
 *  * All codepoints U+0000..U+10FFFF may be encoded,
 *    except for U+D800..U+DFFF, which are reserved
 *    for UTF-16 surrogate pair encoding.
 *  * UTF-8 byte sequences longer than 4 bytes are not permitted,
 *    as they exceed the range of Unicode.
 *  * The sixty-six Unicode "non-characters" are permitted
 *    (namely, U+FDD0..U+FDEF, U+xxFFFE, and U+xxFFFF).
 */
static int utf8_validate_cz(const char *s)
{
	unsigned char c = *s++;
	
	if (c <= 0x7F) {        /* 00..7F */
		return 1;
	} else if (c <= 0xC1) { /* 80..C1 */
		/* Disallow overlong 2-byte sequence. */
		return 0;
	} else if (c <= 0xDF) { /* C2..DF */
		/* Make sure subsequent byte is in the range 0x80..0xBF. */
		if (((unsigned char)*s++ & 0xC0) != 0x80)
			return 0;
		
		return 2;
	} else if (c <= 0xEF) { /* E0..EF */
		/* Disallow overlong 3-byte sequence. */
		if (c == 0xE0 && (unsigned char)*s < 0xA0)
			return 0;
		
		/* Disallow U+D800..U+DFFF. */
		if (c == 0xED && (unsigned char)*s > 0x9F)
			return 0;
		
		/* Make sure subsequent bytes are in the range 0x80..0xBF. */
		if (((unsigned char)*s++ & 0xC0) != 0x80)
			return 0;
		if (((unsigned char)*s++ & 0xC0) != 0x80)
			return 0;
		
		return 3;
	} else if (c <= 0xF4) { /* F0..F4 */
		/* Disallow overlong 4-byte sequence. */
		if (c == 0xF0 && (unsigned char)*s < 0x90)
			return 0;
		
		/* Disallow codepoints beyond U+10FFFF. */
		if (c == 0xF4 && (unsigned char)*s > 0x8F)
			return 0;
		
		/* Make sure subsequent bytes are in the range 0x80..0xBF. */
		if (((unsigned char)*s++ & 0xC0) != 0x80)
			return 0;
		if (((unsigned char)*s++ & 0xC0) != 0x80)
			return 0;
		if (((unsigned char)*s++ & 0xC0) != 0x80)
			return 0;
		
		return 4;
	} else {                /* F5..FF */
		return 0;
	}
}

/* Validate a null-terminated UTF-8 string. */
static int utf8_validate(const char *s)
{
	int len;
	
	for (; *s != 0; s += len) {
		len = utf8_validate_cz(s);
		if (len == 0)
			return 0;
	}
	
	return 1;
}

/*
 * Read a single UTF-8 character starting at @s,
 * returning the length, in bytes, of the character read.
 *
 * This function assumes input is valid UTF-8,
 * and that there are enough characters in front of @s.
 WARNING byte endinenesss
 */
static int utf8_read_char(const char *s, unsigned char *out)
{
	const unsigned char *c = (const unsigned char*) s;
	
	if (utf8_validate_cz(s) == 0)
	{
	   printf("ERROR validate utf8\n");
	}

	if (c[0] <= 0x7F) {
		/* 00..7F */
		*out = c[0];
		return 1;
	} else if (c[0] <= 0xDF) {
		/* C2..DF (unless input is invalid) */
		*out = ((unsigned char)c[0] & 0x1F) << 6 |
		       ((unsigned char)c[1] & 0x3F);
		return 2;
	} else if (c[0] <= 0xEF) {
		/* E0..EF */
		*out = ((unsigned char)c[0] &  0xF) << 12 |
		       ((unsigned char)c[1] & 0x3F) << 6  |
		       ((unsigned char)c[2] & 0x3F);
		return 3;
	} else {
		/* F0..F4 (unless input is invalid) */
		*out = ((unsigned char)c[0] &  0x7) << 18 |
		       ((unsigned char)c[1] & 0x3F) << 12 |
		       ((unsigned char)c[2] & 0x3F) << 6  |
		       ((unsigned char)c[3] & 0x3F);
		return 4;
	}
}

/*
 * Write a single UTF-8 character to @s,
 * returning the length, in bytes, of the character written.
 *
 * @unicode must be U+0000..U+10FFFF, but not U+D800..U+DFFF.
 *
 * This function will write up to 4 bytes to @out.
 */
static int utf8_write_char(unsigned char unicode, char *out)
{
	unsigned char *o = (unsigned char*) out;
	
	if (unicode <= 0x10FFFF && !(unicode >= 0xD800 && unicode <= 0xDFFF))
	{
	  // ok
	}
	else
	{
	  printf("ERROR invalid range\n");
	}

	if (unicode <= 0x7F) {
		/* U+0000..U+007F */
		*o++ = unicode;
		return 1;
	} else if (unicode <= 0x7FF) {
		/* U+0080..U+07FF */
		*o++ = 0xC0 | unicode >> 6;
		*o++ = 0x80 | (unicode & 0x3F);
		return 2;
	} else if (unicode <= 0xFFFF) {
		/* U+0800..U+FFFF */
		*o++ = 0xE0 | unicode >> 12;
		*o++ = 0x80 | (unicode >> 6 & 0x3F);
		*o++ = 0x80 | (unicode & 0x3F);
		return 3;
	} else {
		/* U+10000..U+10FFFF */
		*o++ = 0xF0 | unicode >> 18;
		*o++ = 0x80 | (unicode >> 12 & 0x3F);
		*o++ = 0x80 | (unicode >> 6 & 0x3F);
		*o++ = 0x80 | (unicode & 0x3F);
		return 4;
	}
}


/*
 * Compute the Unicode codepoint of a UTF-16 surrogate pair.
 *
 * @uc should be 0xD800..0xDBFF, and @lc should be 0xDC00..0xDFFF.
 * If they aren't, this function returns false.
 */
static int from_surrogate_pair(uint16_t uc, uint16_t lc, unsigned char *unicode)
{
	if (uc >= 0xD800 && uc <= 0xDBFF && lc >= 0xDC00 && lc <= 0xDFFF) {
		*unicode = 0x10000 + ((((unsigned char)uc & 0x3FF) << 10) | (lc & 0x3FF));
		return 1;
	} else {
		return 0;
	}
}

/*
 * Construct a UTF-16 surrogate pair given a Unicode codepoint.
 *
 * @unicode must be U+10000..U+10FFFF.
 */
static void to_surrogate_pair(unsigned char unicode, uint16_t *uc, uint16_t *lc)
{
	unsigned char n;
	
	if (unicode >= 0x10000 && unicode <= 0x10FFFF)
	{ // ok
	}
	else
	{
	  printf("ERROR in to_surrogate_pair\n");
	}
	
	n = unicode - 0x10000;
	*uc = ((n >> 10) & 0x3FF) | 0xD800;
	*lc = (n & 0x3FF) | 0xDC00;
}
/*
 * Encodes a 16-bit number into hexadecimal,
 * writing exactly 4 hex chars.
 */
static int write_hex16(char *out, uint16_t val)
{
	const char *hex = "0123456789ABCDEF";
	
	*out++ = hex[(val >> 12) & 0xF];
	*out++ = hex[(val >> 8)  & 0xF];
	*out++ = hex[(val >> 4)  & 0xF];
	*out++ = hex[ val        & 0xF];
	
	return 4;
}

void WriteHex16ToFile(uint16_t unicode, FILE * file);
void WriteHex16ToFile(uint16_t unicode, FILE * file)
{
   char sixchar[7];
   sixchar[0] = '\\';
   sixchar[1] = 'u';
   printf("unicode %d\n", (int) unicode);
   write_hex16(&sixchar[2], unicode);
   sixchar[6] = '\0';
   fputs(sixchar, file);
}

const int escape_unicode = 1;
void PrintJsonValueToFile(const char * value, FILE * file);
void PrintJsonValueToFile(const char * value, FILE * file)
{
  int len;
  unsigned int l = strlen(value);
  for (unsigned i = 0; i<l; i++)
  {
 // fputc(value[i], file);
 // continue;
    if (value[i] == '"')
    {  fputs("\\\"", file);
    }
    else if (value[i] == '\\')
    {  fputs("\\\\", file);
    }
    else if (value[i] == '\b')
    {  fputs("\\b", file);
    }
    else if (value[i] == '\f')
    {  fputs("\\f", file);
    }
    else if (value[i] == '\n')
    {  fputs("\\n", file);
    }
    else if (value[i] == '\r')
    {  fputs("\\r", file);
    }
    else if (value[i] == '\t')
    {  fputs("\\t", file);
    }
    else
    {
      /* Encode using \u.... */

      len = utf8_validate_cz(&value[i]);
        
      if (len == 0) 
      {
        /*
         * Handle invalid UTF-8 character gracefully in production
         * by writing a replacement character (U+FFFD)
         * and skipping a single byte.
         *
         * This should never happen when assertions are enabled
         * due to the assertion at the beginning of this function.
         */
        printf("ERROR\n");
        if (escape_unicode) 
        {
          fputs("\\uFFFD", file);
        }
        else
        {
          //  *b++ = 0xEF;
          //  *b++ = 0xBF;
          //  *b++ = 0xBD;
          fputs("\EF\BF\BD", file);
        }
      }
      else if (value[i] < 0x1F || (value[i] >= 0x80 && escape_unicode)) 
      {
        /* Encode using \u.... */
        uint32_t unicode;
        
        len = utf8_read_char(&value[i], &unicode);
      
        if (unicode <= 0xFFFF) 
        {
          printf("unicode is %d\n", unicode);
          WriteHex16ToFile(unicode, file);
        }
        else 
        {
          /* Produce a surrogate pair. */
          uint16_t uc, lc;
          if (unicode > 0x10FFFF)
          {
            printf("ERROR unicode > 0x10FFFF\n");
          }
          printf("unicode is long %d\n", unicode);
          to_surrogate_pair(unicode, &uc, &lc);
          WriteHex16ToFile(uc, file);
          WriteHex16ToFile(lc, file);
        }
        
        if (len > 1)
        {
          i += len-1;
        }
        
      }
      else
      {
        fputc(value[i], file); // print as-is
        len--;
        while (len > 0)
        {
          i++;
          fputc(value[i], file); // print as-is
          len--;
        }
      }
    }
  }
}

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

static int PrintRecursive(treeItem_T * item, int level, char * szLargeBuffer, unsigned int szLargeBufferSize)
{
    int count = 0;
    char * name;
    int ret;
    int printed_something = 0; // will set to true if useful and printed something
    int system_status = 0;
    int converter_status = 0;
    const char * status = "";
    //uint64_t childSize = 0;

    // loop current node level
    while (item)
    {
        FILE_ENTRY * node = (FILE_ENTRY*)item->client;

        if (node)
        {
            name = flexget(&item->name);

            if (node->bIsDir == FALSE)
            {
               if (C_strendstr(name, ".ori"))
               {
                 status = "original, keeping untouched";
                 printed_something = 1;
               }
               else
               {
                  // will probe 
                  if (node->size > 1024)
                  {
                    FILE *tsCheck = fopen(name, "rb");
                    if (tsCheck)
                    {
                      unsigned char buffer[188*2];
                      char output_filename[CONVERTER_MAX_PATH];
                      char tmp_filename[CONVERTER_MAX_PATH];
                      memset(buffer,0,sizeof(buffer));
                      size_t size_read = fread(buffer,1,sizeof(buffer), tsCheck);
                      if (buffer[0] == 0x47 && buffer[188] == 0x47)
                      {
                         printf("YES ts file\n");
                         printed_something = 1;
                         
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
                         ret = C_System2(command, output, output_size, &system_status, stderrOnly);
      //               
                         if (system_status)
                         {
                            printf("cmd status %d\n", system_status);
                         }
                         printf("cmd %s\n", output);
                         if (strstr(output, "PES packet size mismatch"))
                         {
                            printf("YES broken\n");
                            
                            uint64_t nUserAvailable = 0, nRootAvailable = 0, nTotalCapacity = 0;
                            ret = getDiskSpace(g_directory, &nUserAvailable, &nRootAvailable, &nTotalCapacity);
                            printf("free space %"PRIu64"\n",nUserAvailable);
                            if (nUserAvailable < 2*1.3*node->size)
                            {
                               // not enough disk space
                               printf("Not enough disk space to continue\n");
                               exit(10);
                            }
                            
                            strcpy(output_filename, name);
                            strcat(output_filename, "-fixed.ts");
                            strcpy(tmp_filename, name);
                            strcat(tmp_filename, ".ori");
                            snprintf(command, sizeof(command) - 1, "./tsmuxer -i \"%s\" -o \"%s\" --legacy-audio true", name, output_filename); 
                            printf("command %s\n", command);
                            
                            stderrOnly = 0;
                            ret = C_System2(command, output, output_size, &converter_status, stderrOnly);
                            if (ret & converter_status == 0)
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
                                      status = "success converting";
                                   }
                                   else
                                   {  // error
                                      ret = C_MoveFileEx(tmp_filename, name, TOOLBOX_OVERWRITE_DESTINATION); // restore
                                      printf("Restoring after error\n");
                                      status = "error failed to move files after converting";
                                      
                                   }
                                 }
                                 else
                                 {
                                    status = "error failed to rename original file after converting";
                                 }
                               }
                               else
                               {
                                  printf("There might have been a problem during convertion, output: %s\n", output);
                                  status = "error converting, see converter.log for details";
                               }
                            }
                            else
                            {
                               printf("Error converting status = %d\n", converter_status);
                               printf("\noutput:%s\n", output);
                            }
                         }
                         else
                         {
                            printf("not broken\n");
                            status = "TS file not broken, keeping untouched";
                         }
                      }
                      else
                      {
                         //printf("NOT ts file\n");
                         status = "not TS file";
                      }
                      fclose(tsCheck);
                    }
                  }
               }
            }
            
            if (node->bIsDir || printed_something) 
            {
              if (count >= 1) fputs(",", g_report);
              fputs("{", g_report);

              fputs("\"name\":\"", g_report); //FIXME(sanitize to JSON)
              if (item->name.buffer) PrintJsonValueToFile(item->name.buffer, g_report);
              else PrintJsonValueToFile(item->name.fixed, g_report);
              fputs("\"", g_report);

              fputs(",\"size\":\"", g_report);
              snprintf(szLargeBuffer, 100, "%"PRIu64, node->size);
              fputs(szLargeBuffer, g_report);
              fputs("\"", g_report);

              fputs(",\"isDir\":", g_report);
              if (node->bIsDir) fputs("true", g_report);
              else fputs("false", g_report);

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
              
              if (converter_status != 0)
              {
                fprintf(g_report, ",\"status\":\"error converting, status %d\"", converter_status);
              }
              else
              {
                fprintf(g_report, ",\"status\":\"%s\"", status);
              }

              if (node->bIsDir && item->childs != NULL)
              {
                fprintf(g_report, ",\"childs\": [\n");

                int subdir_printed = PrintRecursive(item->childs, level+1, szLargeBuffer, szLargeBufferSize);
                if (printed_something == 0 && subdir_printed)
                {
                   printed_something = 1;
                }

                fprintf(g_report, "]");
              }
        
              fputs("}\n", g_report);
            
              count++;
            }
        }

        item = item->next;
    }
    
    return printed_something;
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

