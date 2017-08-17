//  ~~~~~~~~~~~~~~ GNUC Toolbox ~~~~~~~~~~~~~~
//    portable data manipulation functions
//    portable socket server functions
//  ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
//
// toolbox-flexstring.c
//
// Copyright (c) 2015 HaiVision Network Video
//
// Maintained by : Francois Oligny-Lemieux
//       Created : 12.Apr.2015
//
//  Description:
//
//
//  ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

#include "toolbox.h"

#include <limits.h>
#include <stdlib.h>
#include <string.h>


int flexstrcpy(flexString_T * base, const char * str)
{
   if (strlen(str) >= sizeof(base->fixed))
   {
      // allocate
      if (base->buffersize > 0) base->buffer[0] = '\0';
      return C_Append(&base->buffer, &base->buffersize, str, 0/*max_length*/, NULL/*separator*/);
   }

   strcpy(base->fixed, str);

    return 1;
}


int flexstrcasecmp(flexString_T * base, const char * str)
{
   if (base->buffersize > 0)
   {
      return strcasecmp(base->buffer, str);
   }
   return strcasecmp(base->fixed, str);
}

int flexstrcmp(flexString_T * base, const char * str)
{
   if (base->buffersize > 0)
   {
      return strcmp(base->buffer, str);
   }
   return strcmp(base->fixed, str);
}

char* flexget(flexString_T * base)
{
	if (base->buffer) return base->buffer;
	return base->fixed;
}