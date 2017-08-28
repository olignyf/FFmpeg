//±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±
//						 
// toolbox_flexstring.h
//					
// Copyright (c) 2015 HaiVision Network Video
//				
// Maintained by : Francois Oligny-Lemieux
//          Date : 12.Apr.2015
//
//  Description: Used fixed buffer if big enough, otherwise allocate.
//               The fixed buffer size should be chosen to get the best tradeoff from malloc calls and fixed memory usage.
//
//  Limitations:
//
//±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±±

#ifndef __C_TOOLBOX_FLEXSTRING__
#define __C_TOOLBOX_FLEXSTRING__

#ifdef __cplusplus
extern "C" {
#endif

#define TOOLBOX_FLEXSTRING_LENGTH 256

typedef struct flexString_S
{
   char fixed[TOOLBOX_FLEXSTRING_LENGTH];
   char * buffer; // used if fixed buffer is not big enough, always has more priority than fixed.
	unsigned int buffersize; // if non-zero, this is the valid string
} flexString_T;

int flexstrcpy(flexString_T * base, const char * str);
int flexstrcasecmp(flexString_T * base, const char * str);
int flexstrcmp(flexString_T * base, const char * str);
char* flexget(flexString_T * base);

#ifdef __cplusplus
}
#endif

#endif
