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
#include <sys/select.h> /* select */
#include <stdio.h>      /* printf fflush */
#include <unistd.h>     /* read */
#include <termios.h>    /* termios */
#include <stdlib.h>     /* malloc calloc free atoi */

#include "hoof.h"

/******************************************************************************/
#ifdef HOOF_DEBUG

/******************************************************************************/
static int failOnCount = 0;
static int currentCount = 0;

/******************************************************************************/
void *hoofHookMalloc( size_t size )
{
	currentCount += 1;
	if ( currentCount == failOnCount )
	{
		return NULL;
	}

	return malloc( size );
}

/******************************************************************************/
void *hoofHookCalloc( size_t nmemb, size_t size )
{
	currentCount += 1;
	if ( currentCount == failOnCount )
	{
		return NULL;
	}

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
	currentCount += 1;
	if ( currentCount == failOnCount )
	{
		return NULL;
	}

	return fopen( path, mode );
}

/******************************************************************************/
size_t hoofHookFread( void *ptr, size_t size, size_t nmemb, FILE *stream )
{
	currentCount += 1;
	if ( currentCount == failOnCount )
	{
		return 0;
	}

	return fread( ptr, size, nmemb, stream );
}

/******************************************************************************/
size_t hoofHookFwrite( const void *ptr, size_t size, size_t nmemb, FILE *stream )
{
	currentCount += 1;
	if ( currentCount == failOnCount )
	{
		return 0;
	}

	return fwrite( ptr, size, nmemb, stream );
}

/******************************************************************************/
int hoofHookRename( const char *oldpath, const char *newpath )
{
	currentCount += 1;
	if ( currentCount == failOnCount )
	{
		return -1;
	}

	return rename( oldpath, newpath );
}

/******************************************************************************/
int hoofHookRemove( const char *pathname )
{
	currentCount += 1;
	if ( currentCount == failOnCount )
	{
		return -1;
	}

	return remove( pathname );
}

/******************************************************************************/
#endif

/******************************************************************************/
int keyAvailable()
{
	struct timeval timeout;
	fd_set rdset;

	FD_ZERO(&rdset);
	FD_SET(STDIN_FILENO, &rdset);
	timeout.tv_sec  = 0;
	timeout.tv_usec = 100000;

	return select(STDIN_FILENO + 1, &rdset, NULL, NULL, &timeout); 
}

/******************************************************************************/
int main( int argc, char **argv )
{
	/* DATA */
	HOOF_RC rc = 0;

	Hoof *hoof = NULL;
	HoofInterface interface;

	char *filename = "";

	int interactive = 0;

	int done = 0;

	struct termios old_tio, new_tio;
	int readNum = 0;
	char ch[3] = {0, 0, 0};

	int inputWordIndex = 0;

	int hadInput = 0;
	int hadOutput = 0;
	int i = 0;


	/* CODE */
	interface.inputWord[ 0 ] = '\0';

	if ( argc >= 2 )
	{
		filename = argv[ 1 ];
	}

	#ifdef HOOF_DEBUG
	if ( argc >= 3 )
	{
		failOnCount = atoi( argv[ 2 ] );
	}
	#endif

	if ( isatty(0) )
	{
		interactive = 1;
	}
	else
	{
		interactive = 0;
	}

	if ( interactive )
	{
		tcgetattr( STDIN_FILENO, &old_tio );
		new_tio=old_tio;
		new_tio.c_lflag &= (~ICANON & ~ECHO);
		tcsetattr( STDIN_FILENO, TCSANOW, &new_tio );

		#ifdef HOOF_DEBUG
		printf( "DEBUG VERSION\n" );
		#endif
	}

	/* setup hoof */
	rc = hoofInit( filename, &hoof );
	if ( rc != HOOF_RC_SUCCESS )
	{
		fprintf( stderr, "ERROR hoofInit failed %s\n", hoofRCToString( rc ) );
		fflush( stderr );
		goto cleanup;
	}

	while ( 1 )
	{
		/* give input and print any response */
		rc = hoofDo( hoof, &interface );
		if ( rc == HOOF_RC_QUIT )
		{
			done = 1;
		}
		else if ( rc != HOOF_RC_SUCCESS )
		{
			fprintf( stderr, "\nERROR hoofDo failed %s\n", hoofRCToString( rc ) );
			fflush( stderr );
			if ( ! interactive )
			{
				done = 1;
			}
		}

		if ( interface.outputValue[ 0 ][ 0 ] != '\0' )
		{
			/* insert newline between input and output */
			if ( hadInput == 1 && interactive )
			{
				printf( "\n" );
			}
			hadInput = 0;
			hadOutput = 1;
			printf( "%s ", interface.outputValue[ 0 ] );
			if ( interface.outputValue[ 1 ][ 0 ] != '\0' )
			{
				printf( "  " );
			}
			for ( i = 1; i <= HOOF_MAX_VALUE_LENGTH && interface.outputValue[ i ][ 0 ] != '\0'; i += 1 )
			{
				printf( "%s ", interface.outputValue[ i ] );
			}
			fflush( stdout );
		}

		if ( hadOutput )
		{
			printf( "\n" );
			fflush( stdout );
			hadOutput = 0;
		}

		if ( done )
		{
			break;
		}

		/* get input */
		inputWordIndex = 0;
		interface.inputWord[ 0 ] = '\0';
		while ( 1 )
		{
			if ( ( interactive && keyAvailable( ) ) || ( ! interactive ) )
			{
				/* NOTE: need to read 3 because some keys like 'left' come through in 3 bytes */
				if ( interactive )
				{
					readNum = read(STDIN_FILENO, ch, 3);
				}
				else
				{
					readNum = read(STDIN_FILENO, ch, 1);
				}

				if ( ( ! interactive) && readNum == 0 )
				{
					done = 1;
					break;
				}

				if ( readNum == 1 )
				{
					/* backspace */
					if ( ch[ 0 ] == 127 )
					{
						if ( interactive && inputWordIndex > 0 )
						{
							inputWordIndex -= 1;
							interface.inputWord[ inputWordIndex ] = '\0';
							printf( "\b \b" );
							fflush( stdout );
						}
					}
					/* whitespace */
					else if ( ch[ 0 ] == ' ' || ch[ 0 ] == '\n' || ch[ 0 ] == '\r' )
					{
						if ( interactive )
						{
							printf( " " );
							fflush( stdout );
						}

						/* we got a word */
						break;
					}
					/* character */
					else
					{
						if ( inputWordIndex < HOOF_MAX_WORD_LENGTH )
						{
							interface.inputWord[ inputWordIndex ] = ch[ 0 ];
							inputWordIndex += 1;
							interface.inputWord[ inputWordIndex ] = '\0';

							if ( interactive )
							{
								printf( "%c", ch[ 0 ] );
								fflush( stdout );
							}
						}
					}
				}
			}
		}

		hadInput = 1;
	}


	/* CLEANUP */
	cleanup:

	hoofFree( &hoof );

	if ( interactive )
	{
		tcsetattr( STDIN_FILENO, TCSANOW, &old_tio );
	}

	/* workaround for tests */
	if ( rc == HOOF_RC_QUIT )
	{
		rc = 99;
	}

	return rc;
}

