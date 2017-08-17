//  ~~~~~~~~~~~~~~ C Toolbox ~~~~~~~~~~~~~~
//    portable data manipulation functions
//    portable socket server functions
//  ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
//
// c_toolbox
//
//        Author : Francois Oligny-Lemieux
//       Created : 15.May.2006
//
//
//  ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~


#include <stdio.h>
#include <string.h>
#include <time.h>
#include <stdlib.h>
#include <ctype.h>
#include <errno.h>
//#include <utime.h>

#include "toolbox.h"
#include "toolbox-basic-types.h"
//#include "memdbg.h"

#if C_TOOLBOX_TIMING == 1
#   if ( defined(_MSC_VER) )
#       if ! defined(_WINDOWS_)
#           include <windows.h>
#       endif
#       include <mmsystem.h>
#   else
#       include "sys/time.h"
#       include <dirent.h>
#   endif
#endif

#include <sys/types.h>
#include <sys/stat.h>

#if defined(_MSC_VER) && !defined(__TOOLBOX_NETWORK_H__)
#   include <windows.h>
#endif

#if C_TOOLBOX_LOG_COMMAND == 1
FILE * g_toolbox_toolbox_log_file = NULL;
#endif

/* global variable configs */
/* your app can modify those */
int g_verbose = 0;

int char_extract_path(const char * source, char * destination, int destination_size)
{
    const char * strRet;
    unsigned int length = 0;

    if ( source == NULL )
    {
        return -1;
    }
    if ( destination == NULL )
    {
        return -2;
    }

    strRet = strrchr(source, '\\');
    if ( strRet == NULL )
    {   strRet = strrchr(source, '/');
    }
    if ( strRet )
    {
        length = strRet - source;
    }

    if ( destination_size <= (int)length )
    {
        return TOOLBOX_ERROR_BUFFER_TOO_SMALL;
    }

    strncpy(destination, source, length);
    destination[length] = '\0';

    return 1;
}


int char_extract_filename(const char * source, char * destination, int destination_size)
{
    const char * strRet;
    unsigned int length = 0;

    if ( source == NULL )
    {
        return -1;
    }
    if ( destination == NULL )
    {
        return -2;
    }

    destination[0] = '\0';

    strRet = strrchr(source, '\\');
    if ( strRet == NULL )
    {   strRet = strrchr(source, '/');
    }
    if ( strRet )
    {
        length = strlen(strRet+1);
        if ( length > 0 )
        {
            if ( destination_size <= (int)length )
            {
                return TOOLBOX_ERROR_BUFFER_TOO_SMALL;
            }
            strncpy(destination, strRet+1, length);
            destination[length] = '\0';

            return 1;
        }
    }

    return 0;
}


// utf8 string MUST be terminated.
char * C_utf8EndOfString(const char * utf8, int utf8_buffer_size)
{
    const char * zero = "\0";
    if ( utf8 == NULL )
    {
        return NULL;
    }

    if ( utf8_buffer_size == -1 )
    {
        utf8_buffer_size = 0x7FFFFFFF;
    }

    return C_memfind((unsigned char *)utf8, utf8_buffer_size, (unsigned char *)zero, 1);
}

// written 25.Jan.2008
// will realloc the string if too small, or malloc it if NULL
// separator is optional and can be NULL
// other parameters are mandatory
// max_length is either -1 or to the maximum length that can be copied from append.
int C_Append(char ** string, unsigned int * buffersize, const char * append, int max_length, const char * separator)
{
    //unsigned int strLen;
    unsigned int newSize = 0;
    int mallocop = 0;
    char * new_string = NULL;

    if ( max_length == 0 )
    {
        return 0;
    }

    if ( *string )
    {   newSize = strlen(*string);
    }
    if ( max_length < 0 )
    {   newSize += strlen(append); // append without restriction
    }
    else
    {   newSize += max_length; // append up to max_length
    }
    if ( separator )
    {   newSize += strlen(separator);
    }

    newSize += 2;
    if ( *string == NULL )
    {
        new_string = (char*)malloc(newSize);
        mallocop = 1;
        *buffersize = 0;
    }
    else if ( newSize > *buffersize )
    {
        new_string = (char*)realloc(*string, newSize);
        mallocop = 1;
    }

    if ( mallocop )
    {
        if ( new_string == NULL )
        {
            return TOOLBOX_ERROR_MALLOC;
        }
        *string = new_string;
        memset(new_string+*buffersize, 0, newSize - *buffersize); // frank fixme optimize by erasing only end char and first chars
        *buffersize = newSize;
    }

    if ( separator )
    {   strcat(*string, separator);
    }
    if ( max_length < 0 )
    {
        strcat(*string, append);
    }
    else
    {
        strncat(*string, append, max_length);
    }

    return 1;
}


// written 01.May.2008
int buffer_to_file(const char * buffer, const char * filename)
{
    FILE * file;
    int iret;

    if ( buffer == NULL )
    {
        return -1;
    }
    if ( filename == NULL )
    {
        return -2;
    }

    file = fopen(filename, "wb");

    if ( file == NULL )
    {
        return TOOLBOX_ERROR_CANNOT_OPEN_FILE;
    }

    iret = fwrite((char*)buffer, 1, strlen(buffer), file);

    fclose(file);

    if ( iret <= 0 )
    {
        return -10;
    }

    return 1;
}

int C_FileExists(const char * filename)
{
    FILE * test;
    int ret = 0;

    if ( filename == NULL || filename[0] == '\0' )
    {
        return -1;
    }

    test = fopen(filename, "r");
    if (test == NULL)
    {
        ret = 0;
    }
    else
    {
        ret = 1;
        fclose(test);
    }
    return ret;
}

const char * C_GetFileExtension(const char * filename)
{
    int count = 5;
    const char * strLook;

    int strLength = strlen(filename);
    if (strLength == 0) return NULL;

    strLook = &filename[strLength-1];
    while (count > 0 && strLook > &filename[0])
    {
        if (*strLook == '.') return strLook+1;
        strLook--;
        count--;
    }

    return NULL;
}



int C_DirectoryExists(const char * directory)
{
    //FILE * test;
    //int ret = 0;
#if ( defined(_MSC_VER) )
    DWORD attributes = GetFileAttributes(directory);
    if ( attributes != INVALID_FILE_ATTRIBUTES
      && attributes & FILE_ATTRIBUTE_DIRECTORY )
    {
        return 1;
    }
#   if _DEBUG
    attributes = GetLastError();
    attributes = attributes;
#   endif
    return 0;
#else
    DIR * dirPtr;
    dirPtr = opendir(directory);
    if ( dirPtr == NULL )
    {
        return 0;
    }
    closedir(dirPtr);
    return 1;
#endif
}


int C_CreateDirectory(const char * directory)
{
    //FILE * test;
    //int ret = 0;
#if ( defined(_MSC_VER) )
    BOOL bret = CreateDirectory(directory, NULL);
    if ( bret == FALSE )
    {
        // failure
        return -10;
    }
    return 1;
#else
    int iret;
    iret = mkdir(directory, 0755);
    if ( iret < 0 )
    {
        // failure
        return -10;
    }
    return 1;
#endif
}


int C_DeleteFile(const char * filename)
{
#if ( defined(_MSC_VER) )
#else
	char command[256];
#endif

	if ( filename == NULL )
	{
		return -1;
	}

#if ( defined(_MSC_VER) )
	DeleteFile(filename);
#else
	snprintf(command,255,"rm %s", filename);
	system(command);
#endif

	return 1;
}


// by default will overwrite file
int C_CopyFile(const char * source, const char * destination)
{
#if ( defined(_MSC_VER) )
    BOOL bret;
#else
    char command[256];
#endif

    if ( source == NULL )
    {
        return -1;
    }
    if ( destination == NULL )
    {
        return -1;
    }

#if ( defined(_MSC_VER) )
    bret = CopyFile(source,destination,FALSE/*bFailIfExists*/);
    if ( bret == FALSE )
    {
        // error
#if _DEBUG
        printf("WIN32 CopyFile failed with error(%d)\r\n", (int)GetLastError());
#endif
        return -10;
    }
#else
    snprintf(command,255,"cp -f \"%s\" \"%s\"", source,destination);
    system(command);
#endif

    return 1;
}

// written 08.Jan.2008
int C_CopyFileEx(const char * source, const char * destination, int flags)
{
#if ( defined(_MSC_VER) )
    BOOL bret;
    DWORD win32_flags = 0x00000008/*COPY_FILE_ALLOW_DECRYPTED_DESTINATION*/;
#else
    char command[256];
#endif

    if ( source == NULL )
    {
        return -1;
    }
    if ( destination == NULL )
    {
        return -1;
    }

#if ( defined(_MSC_VER) )

#   if(_WIN32_WINNT >= 0x0400)
    if ( (flags&TOOLBOX_OVERWRITE_DESTINATION)==0 )
    {   win32_flags |= 0x00000001/*COPY_FILE_FAIL_IF_EXISTS*/;
    }
    bret = CopyFileEx(source, destination, NULL/*progress*/, NULL/*progress data*/, FALSE, win32_flags);
    if ( bret == FALSE )
    {
        // error
#   if _DEBUG
        printf("WIN32 CopyFileEx failed with error(%d)\r\n", (int)GetLastError());
#   endif
        return -10;
    }

#   else

    bret = FALSE;
    if ( (flags&TOOLBOX_OVERWRITE_DESTINATION)==0 )
    {   bret = TRUE;
    }
    bret = CopyFile(source,destination,bret/*bFailIfExists*/);
    if ( bret == FALSE )
    {
        // error
#if _DEBUG
        printf("WIN32 CopyFile failed with error(%d)\r\n", (int)GetLastError());
#endif
        return -10;
    }
#   endif

#else
    if ( flags & TOOLBOX_OVERWRITE_DESTINATION )
    {
        snprintf(command,255,"cp -f \"%s\" \"%s\"", source,destination);
    }
    else
    {
        snprintf(command,255,"cp \"%s\" \"%s\"", source,destination);
    }
    system(command);
#endif

    return 1;
}


// by default will overwrite file
int C_MoveFile(const char * source, const char * destination)
{
#if ( defined(_MSC_VER) )
    BOOL bret;
#else
    char command[256];
#endif

    if ( source == NULL )
    {
        return -1;
    }
    if ( destination == NULL )
    {
        return -1;
    }

#if ( defined(_MSC_VER) )
    bret = MoveFileEx(source,destination, MOVEFILE_REPLACE_EXISTING|MOVEFILE_WRITE_THROUGH|MOVEFILE_COPY_ALLOWED);
    if ( bret == FALSE )
    {
        // error
#if _DEBUG
        int win32_error = GetLastError();
        printf("WIN32 MoveFileEx failed with error(%d)\r\n", win32_error);
#endif
        return -10;
    }
#else
    snprintf(command,255,"mv -f \"%s\" \"%s\"", source,destination);
    system(command);
#endif

    return 1;
}

// flags are agregates of gnucFlags_E
// written 08.Jan.2008
int C_MoveFileEx(const char * source, const char * destination, int flags)
{
#if ( defined(_MSC_VER) )
    BOOL bret;
    DWORD win32_flags = MOVEFILE_WRITE_THROUGH;
#else
    char command[256];
#endif

    if ( source == NULL )
    {
        return -1;
    }
    if ( destination == NULL )
    {
        return -2;
    }

#if ( defined(_MSC_VER) )
    if ( flags & TOOLBOX_OVERWRITE_DESTINATION )
    {   win32_flags |= MOVEFILE_REPLACE_EXISTING;
    }
    win32_flags |= MOVEFILE_COPY_ALLOWED;
    bret = MoveFileEx(source, destination, win32_flags);
    if ( bret == FALSE )
    {
        // error
#if _DEBUG
        int win32_error = GetLastError();
        printf("WIN32 MoveFile failed with error(%d)\r\n", win32_error);
#endif
        return -10;
    }
#else
    if ( flags & TOOLBOX_OVERWRITE_DESTINATION )
    {
        snprintf(command,255,"mv -f \"%s\" \"%s\"", source,destination);
    }
    else
    {
        snprintf(command,255,"mv \"%s\" \"%s\"", source,destination);
    }
    system(command);
#endif

    return 1;
}



// written 19.Feb.2011
int C_endSlashDirectory(char * directory_inout)
{
    int backslashes = 0;
    char * strRet;
    char * strRet2;

    strRet = strstr(directory_inout, "\\");
    if ( strRet )
    {
        backslashes = 1;
    }

    strRet = C_strendstr(directory_inout, "\\");
    strRet2 = C_strendstr(directory_inout, "/");
    if ( strRet == NULL && strRet2 == NULL )
    {
        if ( backslashes )
        {   strcat(directory_inout, "\\");
        }
        else
        {   strcat(directory_inout, "/");
        }
    }

    return 1;
}




int C_GetTempFilename(const char * path, char * out_filename)
{
#if defined(_MSC_VER)
    char temp[256]="";
#endif


    if ( out_filename == NULL )
    {
        return -1;
    }

#if defined(_MSC_VER)
    // windows
    if ( tmpnam(temp)==NULL )
    {
        return -12;
    }
    if ( temp[0] == '\\' )
    {
        int length = strlen(temp);
        memmove(temp, temp+1, length); // copy the last byte '\0'
        temp[length-1] = '\0'; // redundant
    }
    out_filename[0] = '\0';
    if ( path )
    {
        strcpy(out_filename, path);
        if ( path[strlen(path)-1] != '/'
            && path[strlen(path)-1] != '\\'
            )
        {
            // append slash
            strcat(out_filename, "/");
        }
    }
    strcat(out_filename, temp);
#else
    // linux
    int outfd;
    if ( path )
    {
        strcpy(out_filename, path);
        if ( path[strlen(path)-1] != '/'
            && path[strlen(path)-1] != '\\'
            )
        {
            // append slash
            strcat(out_filename, "/");
        }
    }
    strcat(out_filename, "XXXXXX");
    outfd = mkstemp(out_filename);
    if (outfd == -1)
    {
        printf("<!-- mkstemp on filename(%s) failed -->\n", out_filename);
        return -10;
    }
    close(outfd);
    /* Fix the permissions */
    if (chmod(out_filename, 0600) != 0)
    {
        unlink(out_filename);
        return -11;
    }
#endif
    return 1;
}



int C_itoa( unsigned int i, char * toLoad, unsigned int toLoad_size )
{
    char temp[22];
    char *p = &temp[21];

    *p-- = '\0';

    do {
        *p-- = '0' + i % 10;
        i /= 10;
    } while (i > 0);

    strncpy(toLoad, p+1, toLoad_size-1);
    toLoad[toLoad_size-1] = '\n';

    return 1;
}

// this function has been obtained somewhere on the web.
int C_axtoi(const char * hex)
{
    int n = 0; // position in string
    int m = 0; // position in digit[] to shift
    int count; // loop index
    int intValue = 0; // integer value of hex string
    int digit[32]; // hold values to convert
    if ( strstr(hex, "0x")==hex )
    {   hex+=2;
    }
    while (n < 32)
    {
        if (hex[n] == '\0')
        {   break; }
        if (hex[n] >= 0x30 && hex[n] <= 0x39 )  //if 0 to 9
        {   digit[n] = hex[n] & 0x0f; } //convert to int
        else if (hex[n] >= 'a' && hex[n] <= 'f') //if a to f
        {   digit[n] = (hex[n] & 0x0f) + 9; } //convert to int
        else if (hex[n] >= 'A' && hex[n] <= 'F') //if A to F
        {   digit[n] = (hex[n] & 0x0f) + 9; } //convert to int
        else
        {   break; }
        n++;
    }
    count = n;
    m = n - 1;
    n = 0;
    while (n < count)
    {
        // digit[n] is value of hex digit at position n
        // (m << 2) is the number of positions to shift
        // OR the bits into return value
        intValue = intValue | (digit[n] << (m << 2));
        m--; // adjust the position to set
        n++; // next digit to process
    }
    return (intValue);
}

// will replace in place
char* C_strreplace(char * source, const char token, const char replacement)
{
    char * strRet;

    if (source == NULL) return NULL;
    if (token == replacement) return source;

    strRet = strchr(source, token);
    while (strRet)
    {   *strRet = replacement;
        strRet = strchr(source, token);
    }
    return source;
}

// written 04.Feb.2009
// will write at most dst_max_length and always NULL-terminate the string.
int C_strncpy(char * destination, const char * source, int dst_max_length)
{
    if ( destination == NULL )
    {   return -1;
    }
    if ( source == NULL )
    {   return -2;
    }
    if ( dst_max_length <= 0 )
    {   return -3;
    }

    while ( *source != '\0' )
    {
        *destination = *source++;
        dst_max_length--;
        if ( dst_max_length == 0 )
        {
            *destination = '\0';
            break;
        }
        destination++;
    }

    return 1;
}

// returns NULL on error or not found
// returns pointer from inside string when found, at beginning of needle in string.
char * C_strcasestr(const char * string, const char * needle)
{
    int i;
    int j;
    int stringLen;
    int needleLen;
    int searchLen;

    if ( string == NULL )
    {
        return NULL;
    }
    if ( needle == NULL )
    {
        return NULL;
    }

    stringLen = strlen(string);
    needleLen = strlen(needle);
    searchLen = stringLen - needleLen + 1;

    for (i=0; i<searchLen; i++)
    {
        for (j=0; ; j++)
        {
            if ( j == needleLen )
            {
                return (char*) string+i;
            }
            if ( tolower(string[i+j]) != tolower(needle[j]) )
            {
                // we are not interested in this i
                break;
            }
        }
    }

    return NULL; // not found
}

char * C_strncasestr(const char * string, int string_length, const char * needle)
{
    int i;
    int j;
    int stringLen;
    int needleLen;
    int searchLen;

    if ( string == NULL )
    {
        return NULL;
    }
    if ( needle == NULL )
    {
        return NULL;
    }

    stringLen = string_length;
    needleLen = strlen(needle);
    searchLen = stringLen - needleLen + 1;

    for (i=0; i<searchLen; i++)
    {
        for (j=0; ; j++)
        {
            if ( j == needleLen )
            {
                return (char*) string+i;
            }
            if ( string[i+j] == '\0'
                || ( tolower(string[i+j]) != tolower(needle[j]) )
                )
            {
                // string is over.
                // we are not interested in this i
                break;
            }
        }
    }

    return NULL; // not found
}

// Author: Francois Oligny-Lemieux
// Created: 22.Mar.2007
// if string ends with needle, return pointer to first char of needle in string
// else return NULL
char * C_strendstr(const char * string, const char * needle)
{
    unsigned int stringLength;
    unsigned int needleLength;
    char * strRet;

    if ( string == NULL )
    {
        return NULL;
    }
    if ( needle == NULL )
    {
        return NULL;
    }

    stringLength = strlen(string);
    needleLength = strlen(needle);

    if ( needleLength > stringLength )
    {
        return NULL;
    }

    // string[stringLength] represents '\0'
    // string[stringLength-needleLength] represents theorical start of needle in string
    strRet = strstr(&string[stringLength-needleLength], needle);
    if ( strRet )
    {
        return strRet;
    }

    return NULL;
}


// Author: Francois Oligny-Lemieux
// Created: 22.Mar.2007
// if string ends with needle (case insensitive), return pointer to first char of needle in string
// else return NULL
char * C_striendstr(const char * string, const char * needle)
{
    unsigned int stringLength;
    unsigned int needleLength;
    char * strRet;

    if ( string == NULL )
    {
        return NULL;
    }
    if ( needle == NULL )
    {
        return NULL;
    }

    stringLength = strlen(string);
    needleLength = strlen(needle);

    if ( needleLength > stringLength )
    {
        return NULL;
    }

    // string[stringLength] represents '\0'
    // string[stringLength-needleLength] represents theorical start of needle in string
    strRet = C_strcasestr(&string[stringLength-needleLength], needle);
    if ( strRet )
    {
        return strRet;
    }

    return NULL;
}

// Author: Francois Oligny-Lemieux
// Created: 18.May.2007
// Binary-safe strlen
int C_strlen(const char * string, int string_buffer_size)
{
    uint8_t success = 0;
    int i = 0;
    if ( string == NULL )
    {
        return -1;
    }

    while ( i<string_buffer_size )
    {
        if ( string[i] == '\0' )
        {
            success = 1;
            break;
        }
        i++;
    }

    if ( success == 0 )
    {
        return -1;
    }
    return i;
}


// written 04.Feb.2009, returns end of string
const char * C_eos(const char * string)
{
    if ( string == NULL )
    {
        return NULL;
    }

    while ( *string != '\0' )
    {
        string++;
    }

    return string;
}

// written 20.Aug.2007
void * C_memfind(const unsigned char * buffer, int buffer_length, const unsigned char * needle, int needle_length)
{
    unsigned int i;
    unsigned int j;
    //unsigned int found;
    void * out_value = NULL;

    if ( buffer == NULL )
    {
        return NULL;
    }
    if ( buffer_length <= 0 )
    {
        return NULL;
    }
    if ( needle == NULL )
    {
        return NULL;
    }
    if ( needle_length <= 0 )
    {
        return NULL;
    }

    for (i=0; i<(unsigned int)buffer_length; i++)
    {
        for (j=0; j<(unsigned int)needle_length; j++)
        {
            if ( buffer[i+j] != needle[j] )
            {
                break;
            }
        }
        if ( j == (unsigned int)needle_length )
        {
            out_value = (void*)&buffer[i];
            return out_value;
        }
    }

    return NULL;
}

#if C_TOOLBOX_TEXT_BUFFER_READER == 1
// will change all \r\n to \n
// caller responsible to free() buffer after
int file_to_buffer_malloc(const char * filename, char ** buffer, unsigned int * buffer_size)
{
    int iret = 0;
    textFileReader reader;
    char * bufferPtr;
    char * bufferTemp;
    unsigned int bufferSize;
    unsigned int bufferSizeTemp;
    unsigned int bufferOffset = 0;
    unsigned int read_length = 0;

    if ( filename == NULL )
    {
        return -1;
    }
    if ( buffer == NULL )
    {
        return -2;
    }
    if ( buffer_size == NULL )
    {
        return -3;
    }

    // its better to free than to realloc since realloc will memcpy the stuff (processing for nothing)
    // also in case this new file content requires smaller a buffer size than it is right now, it will free some memory.
    if ( *buffer )
    {
        free(*buffer);
    }
    *buffer = NULL;
    *buffer_size = 0;

    bufferPtr = malloc(sizeof(char)*2048);
    if ( bufferPtr == NULL )
    {
        return TOOLBOX_ERROR_MALLOC;
    }
    bufferSize = 2048;

    iret = TextFileReader_Constructor(&reader, filename);
    if ( iret <= 0 )
    {
        if ( TOOLBOX_IS_TOOLBOX_SPECIFIC_ERROR(iret) )
        {   return iret;
        }
        else
        {   return -20;
        }
    }

    iret = TextFileReader_GetLine(&reader, &bufferPtr, &bufferSize, &read_length, 0/*realloc_if_necessary*/);

    while ( iret > 0 || iret==TOOLBOX_WARNING_CONTINUE_READING )
    {
        bufferOffset += read_length;
        if ( iret != TOOLBOX_WARNING_CONTINUE_READING )
        {
            bufferPtr[bufferOffset] = '\n';
            bufferPtr[bufferOffset+1] = '\0'; // safe because we always malloc +1 of maximum GetLine result
            bufferOffset++;
        }

        if ( bufferSize-bufferOffset < TOOLBOX_TEXT_FILE_READER_BUFFER_SIZE+1 )
        {   // to make sure we always give text file reader enough buffer size so it doesn't stops because of us.
            bufferPtr = realloc(bufferPtr, bufferSize+TOOLBOX_TEXT_FILE_READER_BUFFER_SIZE+1);
            if ( bufferPtr == NULL )
            {
                return TOOLBOX_ERROR_MALLOC;
            }
            bufferSize += TOOLBOX_TEXT_FILE_READER_BUFFER_SIZE+1;
        }

        bufferTemp = bufferPtr+bufferOffset;
        bufferSizeTemp = bufferSize-bufferOffset;
        iret = TextFileReader_GetLine(&reader, &bufferTemp, &bufferSizeTemp, &read_length, 0/*realloc_if_necessary*/);
    }

    iret = TextFileReader_Destructor(&reader);

    //fputs(bufferPtr, stdout);

    *buffer = bufferPtr;
    *buffer_size = bufferSize;

    return 1;
}
#endif


// Modified 27.Jun.2007 for multi-thread safety
#define RESULT_SIZE 1024*512
int C_System2(const char * command, char * loadme, unsigned int loadme_size, int * out_status, int stderrOnly)
{
#if !defined(_MSC_VER)
	char temp[256];
#endif
	FILE * file;
	FILE * fileErr = NULL;
	char tmpFilename[256] = "";
	char tmpFilenameErr[256] = "";
	char full_command[1024];
	char additions[128] = "";
	int status;
	int iret;
	int fret = 1;
	unsigned int result_written = 0;
	unsigned int read_size = 24;
	int size_read = 0;

#if C_TOOLBOX_LOG_COMMAND == 1
	static unsigned int log_fileSize = 0;
	uint64_t fileSize = 0;
#endif

	if (command == NULL)
	{
		return -1;
	}

	if (loadme == NULL)
	{
		return -2;
	}

	if (loadme_size == 0)
	{
		return -3;
	}

	loadme[loadme_size - 1] = '\0';
	additions[sizeof(additions) - 1] = '\0';

#if ( defined(_MSC_VER) )
	iret = C_GetTempFilename(tmpPath, tmpFilename);
	if (iret <= 0)
	{
		//printf("<!-- C_GetTempFilename returned iret(%d) -->\n",iret);
		return -10;
	}
#else
	snprintf(temp, 255, "%s/tempio/", tmpPath);
	iret = C_DirectoryExists(temp);
	if ( iret <= 0 )
	{
		snprintf(temp, 255, "mkdir %s/tempio/", tmpPath);
		system(temp);
		snprintf(temp, 255, "%s/tempio/", tmpPath);
	}

	iret = C_GetTempFilename(temp,&tmpFilename[0]);
	if ( iret <= 0 )
	{
		//printf("<!-- C_GetTempFilename returned iret(%d) -->\n",iret);
		return -11;
	}
	iret = C_GetTempFilename(temp,&tmpFilenameErr[0]);
	if ( iret <= 0 )
	{
		//printf("<!-- C_GetTempFilename returned iret(%d) -->\n",iret);
		return -12;
	}
#endif

	if (stderrOnly)
	{
		snprintf(additions, sizeof(additions) - 1, " 2> %s", tmpFilename);
    }
	else
	{
#if ( !defined(_MSC_VER) )
		snprintf(additions, sizeof(additions) - 1, " > %s 2> %s", tmpFilename, tmpFilenameErr);
#else
		snprintf(additions, sizeof(additions) - 1, " > %s 2>&1", tmpFilename); // Windows
#endif
	}

   if (strlen(command)+strlen(additions) >= sizeof(full_command))
   {
     snprintf(loadme, loadme_size-1, "Internal buffer too short for command");
     *out_status = -14;
      return -14; // command to big
   }

    strcpy(full_command, command);
    strcat(full_command, additions);

#if C_TOOLBOX_LOG_COMMAND == 1
    if ( g_toolbox_toolbox_log_file == NULL )
    {
        iret = C_GetFileSize(C_TOOLBOX_LOG_FILENAME, &fileSize);
        if ( iret > 0 && fileSize > 1024*1024 )
        {
            g_toolbox_toolbox_log_file = fopen(C_TOOLBOX_LOG_FILENAME, "w");
        }
        else
        {
            g_toolbox_toolbox_log_file = fopen(C_TOOLBOX_LOG_FILENAME, "aw");
        }
    }

    if ( g_toolbox_toolbox_log_file )
    {
        if ( log_fileSize > 1024*1024 )
        {
            fclose(g_toolbox_toolbox_log_file);
            g_toolbox_toolbox_log_file = fopen(C_TOOLBOX_LOG_FILENAME, "w");
        }
        fwrite(full_command, 1, strlen(full_command), g_toolbox_toolbox_log_file);
        fwrite("\n", 1, 1, g_toolbox_toolbox_log_file);
        fflush(g_toolbox_toolbox_log_file);
        log_fileSize += strlen(full_command)+1;
    }
#endif

    status = system(full_command);
    if (out_status) *out_status = status;

#if defined(_MSC_VER)
    file = fopen(tmpFilename, "r");
    if ( file == NULL )
    {
        return -12;
    }
#else
    fileErr = fopen(tmpFilenameErr, "r");
    file = fopen(tmpFilename, "r");
    if ( file == NULL && fileErr == NULL )
    {
        return -12;
    }
#endif

    if (file)
    {
        if ( loadme_size-1-result_written < read_size )
        {   read_size = loadme_size-1-result_written;
        }
        size_read = fread(loadme+result_written, 1, read_size, file);
        //printf("after initial fgets, charRet(0x%X) and feof=%d\n", (unsigned int)charRet, feof(outf));
        while ( size_read > 0 )
        {
            result_written += size_read;
            *(loadme+result_written) = '\0';

            if ( loadme_size-1-result_written < read_size )
            {   read_size = loadme_size-1-result_written;
            }

            if ( read_size == 0 )
            {
                fret=-13; // buffer is too small
                break;
            }

            size_read = fread(loadme+result_written, 1, read_size, file);
        }
        fclose(file);

        if (fileErr)
        {
            if ( loadme_size-1-result_written < read_size )
            {   read_size = loadme_size-1-result_written;
            }
            size_read = fread(loadme+result_written, 1, read_size, fileErr);
            //printf("after initial fgets, charRet(0x%X) and feof=%d\n", (unsigned int)charRet, feof(outf));
            while ( size_read > 0 )
            {
                result_written += size_read;
                *(loadme+result_written) = '\0';

                if ( loadme_size-1-result_written < read_size )
                {   read_size = loadme_size-1-result_written;
                }

                if ( read_size == 0 )
                {
                    fret=-13; // buffer is too small
                    break;
                }

                size_read = fread(loadme+result_written, 1, read_size, fileErr);
            }
            fclose(fileErr);
        }
    }

    // delete temporary file
#if ( defined(_MSC_VER) )
    C_DeleteFile(tmpFilename);
#else
    unlink(tmpFilename);
    unlink(tmpFilenameErr);
#endif

#if C_TOOLBOX_LOG_COMMAND == 1
    if ( g_toolbox_toolbox_log_file )
    {
        snprintf(temp, 255, " ... command passed through ... ");
        fwrite(temp, 1, strlen(temp), g_toolbox_toolbox_log_file);
        fwrite("\n", 1, 1, g_toolbox_toolbox_log_file);
        fflush(g_toolbox_toolbox_log_file);
    }
#endif

    return fret;
}

// we supply the char buffer from static memory (not thread safe)
int C_System(const char * command, char ** insider, int * status)
{
    int fret = 1;
    static char result[RESULT_SIZE];

    result[0] = '\0';

    result[RESULT_SIZE-1] =- '\0';

    if ( insider )
    {
        *insider = &result[0];
    }
    else
    {
        return -2;
    }

    if ( command == NULL )
    {
        return -1;
    }

    fret = C_System2(command, result, RESULT_SIZE, status, 0);

    return fret;
}


int C_GetFileSize(const char * filename, uint64_t * loadme)
{
    int iret;
#if defined(_MSC_VER)
    struct _stati64 statObject;
#else
    struct stat statObject;
#endif

    if (loadme==NULL)
    {
        return -1;
    }

#if defined(_MSC_VER)
    iret = _stati64(filename, &statObject);
#else
    iret = stat(filename, &statObject);
#endif

    if ( iret < 0 )
    {
        return -10;
    }

    *loadme = statObject.st_size;
    //printf("fileSize("I64u")\n",statObject.st_size);
    return 1;
}

int C_GetFileCreationTime(const char * filename, uint64_t * loadme)
{
    int iret;
#if defined(_MSC_VER)
    struct _stati64 statObject;
#else
    struct stat statObject;
#endif

    if (loadme==NULL)
    {
        return -1;
    }

#if defined(_MSC_VER)
    iret = _stati64(filename, &statObject);
#else
    iret = stat(filename, &statObject);
#endif

    if ( iret < 0 )
    {
        return -10;
    }

    *loadme = statObject.st_ctime;
    //printf("fileSize("I64u")\n",statObject.st_size);
    return 1;
}



#if defined(__TOOLBOX_NETWORK_H__)
int C_GetHostname(char * hostname, int hostname_bufsize)
{
    int iret;
    char * insider = NULL;

    if ( hostname == NULL )
    {
        return -1;
    }

    if ( hostname_bufsize <= 0 )
    {
        return -2;
    }

    hostname[0] = '\0';

#if defined(_MSC_VER)
    iret = gethostname(hostname, hostname_bufsize-1);
    if ( iret == SOCKET_ERROR )
    {
        iret = WSAGetLastError();
        if ( iret == WSANOTINITIALISED )
        {
            Network_Initialize();
            iret = gethostname(hostname, hostname_bufsize-1);
        }
    }
    if ( iret == 0 )
    {
        return 1;
    }
    return -10; // failed ?? :(
#else
    iret = C_System("hostname", &insider);
    if ( iret > 0 && insider )
    {
        char * strRet;
        strncpy(hostname, insider, hostname_bufsize-1);
        hostname[hostname_bufsize-1] = '\0';
        strRet = strchr(hostname, '\r');
        if ( strRet )
        {
            *strRet = '\0';
        }
        strRet = strchr(hostname, '\n');
        if ( strRet )
        {
            *strRet = '\0';
        }
    }
#endif
    return 1;
}
#endif


int C_Sleep(int milliseconds)
{
#if ( defined(_MSC_VER) )
    Sleep((DWORD)milliseconds);
#else
    usleep(1000*milliseconds);
#endif
    return 1;
}

static const char * const g_rand_source_alphanum = "1234567890ABCDEFGHIJKLMNOPQRSTUVWXYZ";
static const char * const g_rand_source_alpha = "ABCDEFGHIJKLMNOPQRSTUVWXYZ";
static const char * const g_rand_source_num = "0123456789";
static const char * const g_rand_source_num_nozero = "123456789123456789";
static int g_srand_done = 0;
// will write length char in toload + '\0', thus toload must be at least length+1 big
int C_Random_alpha(int length, char * toload, int buffer_size)
{
    int random_value = 0;
    unsigned int srandom_value = 0;
    char temp[256]="";
    int count=0;
    //int iret;
    int i;
    int number;
    char letter[3]="";

    if ( toload == NULL )
    {
        return -1;
    }

    if ( length <= 0 )
    {
        return -2;
    }

    if ( length >= buffer_size || length >= 127 )
    {
        return -3;
    }

    temp[254]='\0';
    temp[255]='\0';

    if ( g_srand_done == 0 )
    {
        srandom_value = (unsigned int) time(NULL);
        srand(srandom_value);
        g_srand_done = 1;
    }
    random_value = rand();

    //printf("<!-- rand(%d) -->\n",random_value);

    snprintf(temp, 255, "%d",random_value);

    while ( count < 200 )
    {
        random_value = rand();
        snprintf(temp+strlen(temp),255-strlen(temp),"%d",random_value);
        if ( temp[254] != '\0' )
        {
            // done
            break;
        }
        count++;
    }

    for (i=0; i<length; i++)
    {
        //number = atoi(&temp[i]);
        letter[0] = temp[i];
        letter[1] = temp[i+1];
        letter[2] = '\0';
        number = atoi(letter);
        if ( strlen(g_rand_source_alpha) > (unsigned int) number )
        {
            toload[i] = g_rand_source_alpha[number];
        }
        else
        {
            letter[1] = '\0';
            number = atoi(letter);
            toload[i] = g_rand_source_alpha[number];
        }
    }
    if ( length < buffer_size )
    {
        toload[length]='\0';
    }
    else
    {
        toload[buffer_size-1]='\0';
    }

    return 1;
}

int C_Random_numeric(int length, char * toload, int buffer_size)
{
    int random_value = 0;
    unsigned int srandom_value = 0;
    char temp[256]="";
    int count=0;
    int i;
    int number;
    char letter[2]="";

    if ( toload == NULL )
    {
        return -1;
    }

    if ( length <= 0 )
    {
        return -2;
    }

    if ( length > 255 )
    {
        return -3;
    }

    memset(&temp[0], 0, 256); // important for snprintf below since I don't add '\0' everytime

    if ( g_srand_done == 0 )
    {
        srandom_value = (unsigned int) time(NULL);
        srand(srandom_value);
        g_srand_done = 1;
    }
    random_value = rand();

    snprintf(temp,255,"%d",random_value);

    while ( count < 20 )
    {
        if ( strlen(temp) >= (unsigned int) length )
        {
            break;
        }
        random_value = rand();
        snprintf(temp+strlen(temp),255-strlen(temp),"%d",random_value);
        count++;
    }

    if ( strlen(temp) < (unsigned int) length )
    {
        length = strlen(temp);
    }

    for (i=0; i<length; i++)
    {
        letter[0] = temp[i];
        letter[1] = '\0';
        number = atoi(letter);
        if ( number>0 && (unsigned int) number < strlen(g_rand_source_num)  )
        {
            toload[i] = g_rand_source_num[number];
        }
        else
        {
            toload[i] = '0';
        }
    }
    if ( length < buffer_size )
    {
        toload[length]='\0';
    }
    else
    {
        toload[buffer_size-1]='\0';
    }

    return 1;
}

int C_Tolower(char * source)
{
    unsigned int i=0;
    if ( source == NULL )
    {
        return -1;
    }

    for (i=0; i<strlen(source); i++)
    {
        if ( source[i]=='\0' )
        {
            break;
        }

        source[i]=tolower(source[i]);
    }

    return 1;
}

int C_memFind(unsigned char * input, unsigned int inputLength, unsigned char * needle, unsigned int needleLength, unsigned char ** out_position_in_input)
{
    unsigned int inputLengthRemaining = inputLength;
    unsigned int inputOffset = 0;
    int iret;

    if ( input == NULL )
    {
        return -1;
    }
    if ( inputLength == 0 )
    {
        return -2;
    }
    if ( needle == NULL )
    {
        return -3;
    }
    if ( needleLength == 0 )
    {
        return -4;
    }
    if ( inputLength < needleLength )
    {
        return 0;
    }

    while (inputLengthRemaining >= needleLength)
    {
        iret = memcmp(input+inputOffset, needle, needleLength);
        if ( iret == 0 )
        {
            // we have a match
            if ( out_position_in_input )
            {
                *out_position_in_input = input + inputOffset;
            }
            return 1;
        }
        inputOffset += 1;
        inputLengthRemaining -= 1;
    }

    return 0;
}


//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
//
// C_duplicateString
//
// Written by : Francois Oligny-Lemieux
//    Created : 17.Apr.2007
//   Modified :
//
//  Description:
//    > malloc a new string and copy the input content into it.
//    > WARNING: you need to free yourself the string.
//
//   input (IN) original string
//   output (OUT) will be assigned the new buffer pointer
//   output_size (OUT) will be filled with the new buffer size
//
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

int C_duplicateString(const char * input, char ** output, unsigned int * output_size)
{
    unsigned int strLen;
    unsigned int ouputSize;
    if ( input == NULL )
    {
        return -1;
    }
    if ( output == NULL )
    {
        return -2;
    }

    *output = NULL;
    if ( output_size )
    {
        *output_size = 0;
    }

    strLen = strlen(input);

    // 25mb in hex is 0x1900000 bytes
    if ( strLen > 0x1900000 )
    {
        return -10; // way to big
    }

    ouputSize = strLen+1;

    *output = malloc(sizeof(char) * ouputSize );
    if ( *output == NULL )
    {
        return TOOLBOX_ERROR_MALLOC;
    }

    if ( output_size )
    {
        *output_size = ouputSize;
    }

    memcpy(*output, input, strLen);
    (*output)[strLen] = '\0';

    return 1;
}

#if C_TOOLBOX_TIMING == 1
unsigned int C_Timestamp()
{
    unsigned int retval=0;
#if ( defined(_MSC_VER) )
    retval = (unsigned int) timeGetTime();
#else
    struct timeval tv_date;
    mtime_t delay; /* delay in msec, signed to detect errors */

    /* see mdate() about gettimeofday() possible errors */
    gettimeofday( &tv_date, NULL );

    /* calculate delay and check if current date is before wished date */
    delay = (mtime_t) tv_date.tv_sec * 1000000 + (mtime_t) tv_date.tv_usec;
    delay = delay/1000;
    retval = (unsigned int) delay;

#endif
    return retval;
}

#endif // end C_TOOLBOX_TIMING


// written 30.Jul.2007
int C_getCompileDate(char * string, int string_size)
{
    char date[32] = __DATE__;
    char * month = NULL;
    char * day = NULL;
    char * year = NULL;
    char * strRet;

    if ( string == NULL )
    {
        return -1;
    }
    if ( string_size < 31 )
    {
        return TOOLBOX_ERROR_YOU_PASSED_A_BUFFER_TOO_SMALL;
    }

    string[0] = '\0';
    string[31] = '\0';
    strRet = strchr(date, ' ');
    month = &date[0];

    if ( strRet )
    {
        *strRet = '\0';
        if ( *(strRet+1) == ' ' )
        {   *(strRet+1) = '0';
        }
        day = strRet+1;
        strRet = strchr(strRet+1, ' ');
        if ( strRet )
        {
            *strRet = '\0';
            year = strRet+1;
        }
    }
    snprintf(string, 31, "%s.%s.%s", day, month, year);
    //sprintf(temp, "Build Date: %s", frank_date);

    return 1;
}




#if 1
BOOL C_FileTouch(const char *szFileName)
{
    BOOL bRetCode = FALSE;

    printf("(utime based) Called for \"%s\".\n", szFileName);

    if (utime(szFileName, NULL) == 0)
    {
        bRetCode = TRUE;
    }
    else
    {
        printf("utime failed for file \"%s\", errno=%d.\n", szFileName, errno);
    }

    return bRetCode;
}

#else


BOOL C_FileTouch(const char *szFileName)
{
    BOOL bRetCode = FALSE;
    char szBuffer[246];

    printf("(system based) Called for \"%s\".\n", szFileName);
    snprintf(szBuffer, sizeof(szBuffer)-1, "touch %s\n", szFileName);
    if (system(szBuffer) == 0)
    {
        bRetCode = TRUE;
    }
    else
    {
        printf("system failed when touching file \"%s\", errno=%d.\n", szFileName, errno);
    }

    return bRetCode;
}

#endif
