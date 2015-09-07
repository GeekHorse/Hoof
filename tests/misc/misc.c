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
#define test_err_if( x ) \
	if ( (x) ) \
	{ \
		printf( "FAILED ON LINE: %d\n", __LINE__ ); \
		rc = -1; \
		goto cleanup; \
	}


/******************************************************************************/
void *hoof_hook_malloc( size_t size )
{
	return malloc( size );
}

/******************************************************************************/
void *hoof_hook_calloc( size_t nmemb, size_t size )
{
	return calloc( nmemb, size );
}

/******************************************************************************/
void hoof_hook_free( void *ptr )
{
	free( ptr );
}

/******************************************************************************/
FILE *hoof_hook_fopen( const char *path, const char *mode )
{
	return fopen( path, mode );
}

/******************************************************************************/
size_t hoof_hook_fread( void *ptr, size_t size, size_t nmemb, FILE *stream )
{
	return fread( ptr, size, nmemb, stream );
}

/******************************************************************************/
size_t hoof_hook_fwrite( const void *ptr, size_t size, size_t nmemb, FILE *stream )
{
	return fwrite( ptr, size, nmemb, stream );
}

/******************************************************************************/
int hoof_hook_rename( const char *oldpath, const char *newpath )
{
	return rename( oldpath, newpath );
}

/******************************************************************************/
int hoof_hook_remove( const char *pathname )
{
	return remove( pathname );
}

void hoof_hook_log( char * library , n line_number , n rc , n a , n b , n c )
{
	fprintf( stderr , "%s : %jd %jd : %jd %jd %jd\n" , library , line_number , rc , a , b , c ) ;
	fflush( stderr ) ;
}

/******************************************************************************/
int main( int argc, char **argv )
{
	/* DATA */
	int rc = 0;

	struct hoof *hoof = NULL;
	struct hoof_interface interface;

	const char *rc_string = NULL;

	int i = 0;


	/* CODE */
	/* test NULL to hoof_free */
	hoof_free( NULL );

	/* test bad rcs to hoof_rc_to_string */
	rc_string = hoof_rc_to_string( -1 );
	test_err_if( strcmp( rc_string, "Unknown Error" ) != 0 );
	
	rc_string = hoof_rc_to_string( hoof_rc_hoof_errors_min - 1 );
	test_err_if( strcmp( rc_string, "Unknown Error" ) != 0 );

	rc_string = hoof_rc_to_string( hoof_rc_hoof_errors_max + 1 );
	test_err_if( strcmp( rc_string, "Unknown Error" ) != 0 );

	/* init */
	rc = hoof_init( "miscdata", &hoof );
	test_err_if( rc != hoof_rc_success );

	/* just get past the "hello" */
	strcpy( interface.input_word, "" );
	rc = hoof_do( hoof, &interface );

	/* test bad input_word   normal word */
	for ( i = 0; i < ( hoof_max_word_length + 1 ); i += 1 )
	{
		interface.input_word[ i ] = 'x';
	}
	rc = hoof_do( hoof, &interface );
	test_err_if( rc != hoof_rc_error_word_long );

	/* test bad input_word   number */
	for ( i = 0; i < ( hoof_max_word_length + 1 ); i += 1 )
	{
		interface.input_word[ i ] = '5';
	}
	rc = hoof_do( hoof, &interface );
	test_err_if( rc != hoof_rc_error_word_long );

	/* signal success */
	rc = 0;


	/* CLEANUP */
	cleanup:

	hoof_free( &hoof );

	return rc;
}


