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
#include <stdio.h> /* printf, fopen, fwrite, fread, rename, remove */
#include <string.h> /* strcmp, strcpy */
#include <stdlib.h> /* malloc, calloc, free */

#include "hoof.h"


/******************************************************************************/
#define TEST_ERR_IF( x ) \
	if ( (x) ) \
	{ \
		printf( "FAILED ON LINE: %d\n", __LINE__ ); \
		rc = -1; \
		goto cleanup; \
	}


/******************************************************************************/
void *hoofHookMalloc( size_t size )
{
	return malloc( size );
}

/******************************************************************************/
void *hoofHookCalloc( size_t nmemb, size_t size )
{
	return calloc( nmemb, size );
}

/******************************************************************************/
void hoofHookFree( void *ptr )
{
	free( ptr );
}

/******************************************************************************/
FILE *hoofHookFopen( const char *path, const char *mode )
{
	return fopen( path, mode );
}

/******************************************************************************/
size_t hoofHookFread( void *ptr, size_t size, size_t nmemb, FILE *stream )
{
	return fread( ptr, size, nmemb, stream );
}

/******************************************************************************/
size_t hoofHookFwrite( const void *ptr, size_t size, size_t nmemb, FILE *stream )
{
	return fwrite( ptr, size, nmemb, stream );
}

/******************************************************************************/
int hoofHookRename( const char *oldpath, const char *newpath )
{
	return rename( oldpath, newpath );
}

/******************************************************************************/
int hoofHookRemove( const char *pathname )
{
	return remove( pathname );
}

/******************************************************************************/
int main( int argc, char **argv )
{
	/* DATA */
	int rc = 0;

	Hoof *hoof = NULL;
	HoofInterface interface;

	const char *rcString = NULL;

	int i = 0;


	/* CODE */
	/* test NULL to hoofFree */
	hoofFree( NULL );

	/* test bad rcs to hoofRCToString */
	rcString = hoofRCToString( -1 );
	TEST_ERR_IF( strcmp( rcString, "Unknown Error" ) != 0 );
	
	rcString = hoofRCToString( HOOF_RC_HOOF_ERRORS_MIN - 1 );
	TEST_ERR_IF( strcmp( rcString, "Unknown Error" ) != 0 );

	rcString = hoofRCToString( HOOF_RC_HOOF_ERRORS_MAX + 1 );
	TEST_ERR_IF( strcmp( rcString, "Unknown Error" ) != 0 );

	/* test cancelling reads */
	rc = hoofInit( "misc", &hoof );
	TEST_ERR_IF( rc != HOOF_RC_SUCCESS );

	/* just get past the "hello" */
	strcpy( interface.inputWord, "" );
	rc = hoofDo( hoof, &interface );

	/* read word */
	strcpy( interface.inputWord, "word" );
	rc = hoofDo( hoof, &interface );
	TEST_ERR_IF( rc != HOOF_RC_SUCCESS );
	TEST_ERR_IF( interface.moreToOutput != 1 );

	strcpy( interface.inputWord, "cancel" );
	rc = hoofDo( hoof, &interface );
	TEST_ERR_IF( rc != HOOF_RC_SUCCESS );
	TEST_ERR_IF( interface.moreToOutput == 1 );

	/* read value */
	strcpy( interface.inputWord, "value" );
	rc = hoofDo( hoof, &interface );
	TEST_ERR_IF( rc != HOOF_RC_SUCCESS );
	TEST_ERR_IF( interface.moreToOutput != 1 );

	strcpy( interface.inputWord, "cancel" );
	rc = hoofDo( hoof, &interface );
	TEST_ERR_IF( rc != HOOF_RC_SUCCESS );
	TEST_ERR_IF( interface.moreToOutput == 1 );

	/* test bad inputWord   normal word */
	for ( i = 0; i < ( HOOF_MAX_WORD_LENGTH + 1 ); i += 1 )
	{
		interface.inputWord[ i ] = 'x';
	}
	rc = hoofDo( hoof, &interface );
	TEST_ERR_IF( rc != HOOF_RC_ERROR_WORD_LONG );

	/* test bad inputWord   number */
	for ( i = 0; i < ( HOOF_MAX_WORD_LENGTH + 1 ); i += 1 )
	{
		interface.inputWord[ i ] = '5';
	}
	rc = hoofDo( hoof, &interface );
	TEST_ERR_IF( rc != HOOF_RC_ERROR_WORD_LONG );

	/* signal success */
	rc = 0;


	/* CLEANUP */
	cleanup:

	hoofFree( &hoof );

	return rc;
}


