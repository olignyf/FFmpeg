//  ~~~~~~~~~~~~~~ GNUC Toolbox ~~~~~~~~~~~~~~
//    portable data manipulation functions
//    portable socket server functions
//  ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
//
// text_buffer_reader.h
// 
// Copyright (c) 2006-2010 Francois Oligny-Lemieux
//
// Maintained by : Francois Oligny-Lemieux
//       Created : 15.May.2006
//      Modified : 27.Sep.2010 (wide-string version)
//
//  Description: 
//      from a char * buffer, offer a getLine function
//      handles correctly windows and linux end-of-line
//
//  ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

#ifndef _TEXT_BUFFER_READER_H_
#define _TEXT_BUFFER_READER_H_

#include "toolbox-basic-types.h"
#include "toolbox-errors.h"

#ifdef __cplusplus
extern "C" {
#endif
	
#define TEXT_BUFFER_READER_MAX_LINE_SIZE 1024

typedef struct textBufferReader
{
	char * buffer;
	int buffer_size; // this is the malloc'ed size of m_buffer
	int internal_buffer;

	int offset; // points to the next unreaded character.
	int chop_empty_lines;
#if USE_MUTEX == 1
	Mutex mutex;
#endif	
} textBufferReader;

typedef struct textBufferReaderW
{
	wchar_t * buffer;
	int buffer_size; // this is the malloc'ed size of m_buffer
	int internal_buffer;

	int offset; // points to the next unreaded character.
	int chop_empty_lines;
#if USE_MUTEX == 1
	Mutex mutex;
#endif
	
} textBufferReaderW;

int TextBufferReader_Constructor(textBufferReader * reader, const char * const buffer, int buffer_size );
int TextBufferReader_ConstructorEx(textBufferReader * reader, char * const buffer, int buffer_size, int make_your_own_copy_of_buffer);
int TextBufferReader_Destructor(textBufferReader * reader);
int TextBufferReader_GetLine(textBufferReader * reader, char * out_value, int max_size);


int TextBufferReader_ConstructorW(textBufferReaderW * reader, const wchar_t * const buffer, int buffer_size);
int TextBufferReader_ConstructorExW(textBufferReaderW * reader, wchar_t * const buffer, int buffer_size, int make_your_own_copy_of_buffer);
int TextBufferReader_DestructorW(textBufferReaderW * reader);
int TextBufferReader_GetLineW(textBufferReaderW * reader, wchar_t * out_value, int max_length);


#ifdef __cplusplus
}
#endif

#endif //_TEXT_BUFFER_READER_H_
