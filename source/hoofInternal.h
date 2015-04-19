/*
Copyright (c) 2014-2015 Jeremiah Martell
All rights reserved.

Redistribution and use in source and binary forms, with or without modification,
are permitted provided that the following conditions are met:

    - Redistributions of source code must retain the above copyright notice,
      this list of conditions and the following disclaimer.
    - Redistributions in binary form must reproduce the above copyright notice,
      this list of conditions and the following disclaimer in the documentation
      and/or other materials provided with the distribution.
    - Neither the name of Jeremiah Martell nor the name of GeekHorse nor the
      name of Hoof nor the names of its contributors may be used to endorse
      or promote products derived from this software without specific prior
      written permission.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR
ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
(INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON
ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
(INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
*/


/******************************************************************************/
#ifndef hoofInternal_H
#define hoofInternal_H

/******************************************************************************/
#include <stdlib.h> /* malloc, calloc, free */
#include <string.h> /* strcmp */
#include <stdio.h> /* FILE, fopen, fread, fwrite, fclose, rename, remove */

/******************************************************************************/
#define ERR_IF( cond, errorToReturn ) \
	if ( (cond) ) \
	{ \
		HOOF_HOOK_LOG( HOOF_LIBRARY_NUMBER, HOOF_FILE_NUMBER, __LINE__, errorToReturn, 0, 0, 0 ); \
		rc = errorToReturn; \
		goto cleanup; \
	}

#define ERR_PASSTHROUGH( func ) \
	rc = (func); \
	ERR_IF( rc != HOOF_RC_SUCCESS, rc );

/******************************************************************************/
#ifdef HOOF_BE_PARANOID
#define PARANOID_ERR_IF( cond ) \
	if ( (cond) ) \
	{ \
		fprintf( stderr, "HOOF PARANOID ERROR! %s %d", __FILE__, __LINE__ ); \
		fflush( stderr ); \
		exit( -99 ); \
	}
#else
#define PARANOID_ERR_IF( cond ) 
#endif

/******************************************************************************/
#define HOOF_MALLOC( POINTER, POINTER_TYPE, COUNT ) \
	POINTER = ( POINTER_TYPE * ) HOOF_HOOK_MALLOC( sizeof( POINTER_TYPE ) * (COUNT) ); \
	ERR_IF( POINTER == NULL, HOOF_RC_ERROR_MEMORY );

/******************************************************************************/
#define HOOF_CALLOC( POINTER, POINTER_TYPE, COUNT ) \
	POINTER = ( POINTER_TYPE * ) HOOF_HOOK_CALLOC( COUNT, sizeof( POINTER_TYPE ) ); \
	ERR_IF( POINTER == NULL, HOOF_RC_ERROR_MEMORY );

/******************************************************************************/
#define HOOF_FREE( POINTER ) \
	HOOF_HOOK_FREE( (POINTER) ); \
	(POINTER) = NULL;

/******************************************************************************/
#ifdef HOOF_USE_MEM_HOOKS

	extern void *hoofHookMalloc( size_t size );
	extern void *hoofHookCalloc( size_t nmemb, size_t size );
	extern void hoofHookFree( void *ptr );

	#define HOOF_HOOK_MALLOC hoofHookMalloc
	#define HOOF_HOOK_CALLOC hoofHookCalloc
	#define HOOF_HOOK_FREE hoofHookFree

#else

	#define HOOF_HOOK_MALLOC malloc
	#define HOOF_HOOK_CALLOC calloc
	#define HOOF_HOOK_FREE free

#endif

/******************************************************************************/
/*
	NOTE: We still use feof fclose and remove directly, these are just used to
	test file failures
*/
#ifdef HOOF_USE_FILE_HOOKS

	extern FILE *hoofHookFopen( const char *path, const char *mode );
	extern size_t hoofHookFread( void *ptr, size_t size, size_t nmemb, FILE *stream );
	extern size_t hoofHookFwrite( const void *ptr, size_t size, size_t nmemb, FILE *stream );
	extern int hoofHookRename( const char *oldpath, const char *newpath );

	#define HOOF_HOOK_FOPEN hoofHookFopen
	#define HOOF_HOOK_FREAD hoofHookFread
	#define HOOF_HOOK_FWRITE hoofHookFwrite
	#define HOOF_HOOK_RENAME hoofHookRename

#else

	#define HOOF_HOOK_FOPEN fopen
	#define HOOF_HOOK_FREAD fread
	#define HOOF_HOOK_FWRITE fwrite
	#define HOOF_HOOK_RENAME rename

#endif

/******************************************************************************/
#ifdef HOOF_ENABLE_LOGGING
	extern void hoofHookLog( s32 library, s32 file, s32 value, s32 rc, s32 a, s32 b, s32 c );
	#ifndef HOOF_HOOK_LOG
	#define HOOF_HOOK_LOG( p, f, l, r, a, b, c ) hoofHookLog( p, f, l, r, a, b, c )
	#endif

#else

	#ifndef HOOF_HOOK_LOG
	#define HOOF_HOOK_LOG( p, f, l, r, a, b, c )
	#endif

#endif

/******************************************************************************/
/* This is in case in the future we want to combine all the hoof .c files into
   one file, to help the compiler optimize, we can define HOOF_INTERNAL as
   static */
#define HOOF_INTERNAL

/******************************************************************************/
/* Macros to make the code easier to read */
#define HEAR( word ) \
	(strcmp( (word), interface->inputWord ) == 0)

#define SAY( word ) \
	hoofOutput( (word), interface->outputWord );

#define MORE \
	interface->moreToOutput = 1;

#define DELAY \
	interface->delayAfterSayingOutputWord = 1;

/******************************************************************************/
/*
	NOTE about the internal structure:
	pages have a value head and value tail, both with wordHead = NULL
	pages can not be empty, they must contain at least 1 real value
		the head value, a real value, and the tail value
	values have a word head and word tail, both with value = NULL
	values can be empty
		empty value is only word head and word tail
	currentValue will never be a value head or value tail
	currentWord may be a value head or a real word
	root is the value head of the root page
*/

/******************************************************************************/
typedef struct HoofWordSTRUCT
{
	struct HoofWordSTRUCT *left;
	struct HoofWordSTRUCT *right;

	char *value;
} HoofWord;

/******************************************************************************/
typedef struct HoofValueSTRUCT
{
	struct HoofValueSTRUCT *up;
	struct HoofValueSTRUCT *down;

	struct HoofValueSTRUCT *in;
	struct HoofValueSTRUCT *out;

	HoofWord *wordHead;
} HoofValue;

/******************************************************************************/
struct HoofSTRUCT
{
	char *filename;

	int loading;
	int paused;
	int literal;

	int (*state)( struct HoofSTRUCT *hoof, HoofInterface *interface, int *huh );

	HoofValue *root;

	HoofValue *currentValue;
	HoofWord *currentWord;

	HoofWord *readWord;
};

/******************************************************************************/
/* hoofCore.c */
HOOF_INTERNAL void hoofOutput( const char *whatToOutput, char *outputWord );
HOOF_INTERNAL HOOF_RC hoofWordVerify( char *word );
HOOF_INTERNAL HOOF_RC hoofStrdup( char *wordIn, char **wordOut_A );
HOOF_INTERNAL void hoofMakeCurrentValue( Hoof *hoof, HoofValue *value );

HOOF_INTERNAL void hoofRoot( Hoof *hoof );
HOOF_INTERNAL void hoofMostUp( Hoof *hoof );
HOOF_INTERNAL void hoofMostDown( Hoof *hoof );
HOOF_INTERNAL void hoofMostOut( Hoof *hoof );
HOOF_INTERNAL void hoofMostIn( Hoof *hoof );

HOOF_INTERNAL HOOF_RC hoofWordInsert( Hoof *hoof, char *value );
HOOF_INTERNAL HOOF_RC hoofValueInsert( HoofValue *before );

HOOF_INTERNAL HOOF_RC hoofPageInit( HoofValue *parent, int createEmptyValue, HoofValue **page_A );

HOOF_INTERNAL void hoofValueClear( Hoof *hoof, HoofValue *value );
HOOF_INTERNAL void hoofWordDelete( Hoof *hoof );
HOOF_INTERNAL void hoofValueDelete( Hoof *hoof );
HOOF_INTERNAL void hoofPageDelete( Hoof *hoof, HoofValue **page_F );

HOOF_INTERNAL void hoofDig( Hoof *hoof, char *word );

/******************************************************************************/
/* hoofStateMachine.c */
HOOF_INTERNAL HOOF_RC hoofStateHello( Hoof *hoof, HoofInterface *interface, int *huh );
HOOF_INTERNAL HOOF_RC hoofStateNavigate( Hoof *hoof, HoofInterface *interface, int *huh );
HOOF_INTERNAL HOOF_RC hoofStateMostChoice( Hoof *hoof, HoofInterface *interface, int *huh );
HOOF_INTERNAL HOOF_RC hoofStateReadWord( Hoof *hoof, HoofInterface *interface, int *huh );
HOOF_INTERNAL HOOF_RC hoofStateReadValue( Hoof *hoof, HoofInterface *interface, int *huh );
HOOF_INTERNAL HOOF_RC hoofStateNewChoice( Hoof *hoof, HoofInterface *interface, int *huh );
HOOF_INTERNAL HOOF_RC hoofStateNew( Hoof *hoof, HoofInterface *interface, int *huh );
HOOF_INTERNAL HOOF_RC hoofStateDeleteChoice( Hoof *hoof, HoofInterface *interface, int *huh );
HOOF_INTERNAL HOOF_RC hoofStateMoveChoice( Hoof *hoof, HoofInterface *interface, int *huh );
HOOF_INTERNAL HOOF_RC hoofStateDig( Hoof *hoof, HoofInterface *interface, int *huh );

/******************************************************************************/
/* hoofLoadSave.c */
HOOF_INTERNAL HOOF_RC hoofSaveWord( FILE *fp, char *word, int newvalue );
HOOF_INTERNAL HOOF_RC hoofSaveValue( FILE *fp, HoofValue *value, char *direction );
HOOF_INTERNAL HOOF_RC hoofSave( Hoof *hoof );
HOOF_INTERNAL HOOF_RC hoofLoad( Hoof *hoof );

/******************************************************************************/
#endif

