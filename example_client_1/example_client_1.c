// copyright 2014 to 2015 jeremiah martell
// all rights reserved
/* LICENSE BSD 3 CLAUSE
	Redistribution and use in source and binary forms, with or without modification, are permitted provided that the following conditions are met:
	- Redistributions of source code must retain the above copyright notice, this list of conditions and the following disclaimer.
	- Redistributions in binary form must reproduce the above copyright notice, this list of conditions and the following disclaimer in the documentation and/or other materials provided with the distribution.
	- Neither the name of Jeremiah Martell nor the name of Geek Horse nor the name of Hoof nor the names of its contributors may be used to endorse or promote products derived from this software without specific prior written permission.
	THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
	*/
// includes
	#include <sys/select.h> // select
	#include <stdio.h>      // printf fflush
	#include <unistd.h>     // read
	#include <termios.h>    // termios
	#include <stdlib.h>     // malloc calloc free atoi
	#include <errno.h>      // errno
	#include "hoof.h"
#ifdef hoof_debug
	// force failure counters
		static n fail_on_count = 0 ;
		static n current_count = 0 ;
	// hook functions
		void * hoof_hook_malloc( size_t size )
			{
			current_count += 1 ;
			if ( current_count == fail_on_count )
				{
				return NULL ;
				}
			return malloc( size ) ;
			}
		void * hoof_hook_calloc( size_t nmemb , size_t size )
			{
			current_count += 1 ;
			if ( current_count == fail_on_count )
				{
				return NULL ;
				}
			return calloc( nmemb , size ) ;
			}
		void hoof_hook_free( void * ptr )
			{
			free( ptr ) ;
			}
		// TODO: consider not using fopen/fclose/fwrite/fread but open/close/write/read
		FILE * hoof_hook_fopen( const char * path , const char * mode )
			{
			current_count += 1 ;
			if ( current_count == fail_on_count )
				{
				return NULL ;
				}
			return fopen( path , mode ) ;
			}
		size_t hoof_hook_fread( void * ptr , size_t size , size_t nmemb , FILE * stream )
			{
			current_count += 1 ;
			if ( current_count == fail_on_count )
				{
				return 0 ;
				}
			return fread( ptr , size , nmemb , stream ) ;
			}
		size_t hoof_hook_fwrite( const void * ptr , size_t size , size_t nmemb , FILE * stream )
			{
			current_count += 1 ;
			if ( current_count == fail_on_count )
				{
				return 0 ;
				}
			return fwrite( ptr , size , nmemb , stream ) ;
			}
		int hoof_hook_rename( const char * oldpath , const char * newpath )
			{
			current_count += 1 ;
			if ( current_count == fail_on_count )
				{
				return -1 ;
				}
			return rename( oldpath , newpath ) ;
			}
		int hoof_hook_remove( const char * pathname )
			{
			current_count += 1 ;
			if ( current_count == fail_on_count )
				{
				return -1 ;
				}
			return remove( pathname ) ;
			}
		void hoof_hook_log( char * library , n line_number , n rc , n a , n b , n c )
			{
			fprintf( stderr , "%s : %jd %jd : %jd %jd %jd\n" , library , line_number , rc , a , b , c ) ;
			fflush( stderr ) ;
			}
	#endif
// functions
	n key_available()
		{
		struct timeval timeout ;
		fd_set rdset ;

		FD_ZERO( & rdset ) ;
		FD_SET( STDIN_FILENO , & rdset ) ;
		timeout . tv_sec  = 0 ;
		timeout . tv_usec = 100000 ;

		return select( STDIN_FILENO + 1 , & rdset , NULL , NULL , & timeout ) ; 
		}
// main
	int main( int argc, char * * argv )
		{
		// data
		n rc = 0 ;
		struct hoof * hoof = NULL ;
		struct hoof_interface interface ;
		// TODO: can we change char to b ?
		char * filename = "" ;
		n interactive = 0 ;
		n done = 0 ;
		struct termios old_tio , new_tio ;
		n read_num = 0 ;
		char ch[ 3 ] = { 0 , 0 , 0 } ;
		n input_word_index = 0 ;
		n i = 0 ;

		// code
		// TODO need more comments
		interface . input_word[ 0 ] = '\0' ;

		if ( argc >= 2 )
			{
			filename = argv[ 1 ] ;
			}

		#ifdef hoof_debug
		if ( argc >= 3 )
			{
			fail_on_count = atoi( argv[ 2 ] ) ;
			}
		#endif

		if ( isatty( STDIN_FILENO ) )
			{
			interactive = 1 ;
			}
		else
			{
			interactive = 0 ;
			}

		if ( interactive )
			{
			// TODO comment what this does
			tcgetattr( STDIN_FILENO , & old_tio ) ;
			new_tio = old_tio ;
			new_tio . c_lflag &= ( ~ ICANON & ~ ECHO ) ;
			tcsetattr( STDIN_FILENO , TCSANOW , & new_tio ) ;

			#ifdef hoof_debug
			printf( "debug version\n" ) ;
			#endif
			}
		// setup hoof
		rc = hoof_init( filename , & hoof ) ;
		if ( rc != hoof_rc_success )
			{
			fprintf( stderr , "error hoof_init failed %s\n" , hoof_rc_to_string( rc ) ) ;
			fflush( stderr ) ;
			goto cleanup ;
			}
		// get first output from hoof
		rc = hoof_do( hoof , & interface ) ;
		if ( rc != hoof_rc_success )
			{
			fprintf( stderr , "\nerror first hoof_do failed %s\n" , hoof_rc_to_string( rc ) ) ;
			fflush( stderr ) ;
			goto cleanup ;
			}
		// print hoof output
		if ( interface . output_value[ 0 ][ 0 ] != '\0' )
			{
			printf( "%s " , interface.output_value[ 0 ] ) ;
			if ( interface . output_value[ 1 ][ 0 ] != '\0' )
				{
				printf( "  " ) ;
				}
			for ( i = 1 ; i <= hoof_max_value_length && interface . output_value[ i ][ 0 ] != '\0' ; i += 1 )
				{
				printf( "%s " , interface.output_value[ i ] ) ;
				}
			printf( "\n" );
			fflush( stdout );
			}
		// main loop
		while ( ! done )
			{
			// get input loop
			input_word_index = 0 ;
			interface . input_word[ 0 ] = '\0' ;
			while ( 1 )
				{
				// TODO see if we can use just if key_available here
				if ( ( interactive && key_available( ) ) || ( ! interactive ) )
					{
					// note : need to read 3 because some keys like left come through in 3 bytes
					if ( interactive )
						{
						read_num = read( STDIN_FILENO , ch , 3 ) ;
						}
					else
						{
						read_num = read( STDIN_FILENO , ch , 1 ) ;
						}
					// if we're not interactive and we dont read a byte then we are out of input
					if ( ( ! interactive) && read_num == 0 )
						{
						goto cleanup ;
						}
					if ( read_num == 1 )
						{
						// if backspace
						if ( ch[ 0 ] == 127 )
							{
							if ( interactive && input_word_index > 0 )
								{
								input_word_index -= 1 ;
								interface.input_word[ input_word_index ] = '\0' ;
								printf( "\b \b" ) ;
								fflush( stdout ) ;
								}
							}
						// else if whitespace
						else if ( ch[ 0 ] == ' ' || ch[ 0 ] == '\n' || ch[ 0 ] == '\r' )
							{
							if ( interactive )
								{
								printf( " " ) ;
								fflush( stdout ) ;
								}
							// we got a word
							break ;
							}
						// else character
						else if ( input_word_index < hoof_max_word_length )
							{
							interface . input_word[ input_word_index ] = ch[ 0 ] ;
							input_word_index += 1 ;
							interface.input_word[ input_word_index ] = '\0' ;

							if ( interactive )
								{
								printf( "%c" , ch[ 0 ] ) ;
								fflush( stdout ) ;
								}
							}
						}
					} // end if key available
				} // end while
			// give input to hoof
			rc = hoof_do( hoof , & interface ) ;
			if ( rc == hoof_rc_quit )
				{
				done = 1 ;
				}
			else if ( rc != hoof_rc_success )
				{
				fprintf( stderr , "\nerror hoof_do failed %s\n" , hoof_rc_to_string( rc ) ) ;
				fflush( stderr ) ;
				if ( ! interactive )
					{
					done = 1 ;
					}
				}
			// print hoof output
			if ( interface . output_value[ 0 ][ 0 ] != '\0' )
				{
				// insert newline between input and output
				if ( interactive )
					{
					printf( "\n" ) ;
					}
				printf( "%s " , interface.output_value[ 0 ] ) ;
				if ( interface . output_value[ 1 ][ 0 ] != '\0' )
					{
					printf( "  " ) ;
					}
				for ( i = 1 ; i <= hoof_max_value_length && interface . output_value[ i ][ 0 ] != '\0' ; i += 1 )
					{
					printf( "%s " , interface.output_value[ i ] ) ;
					}
				printf( "\n" );
				fflush( stdout );
				}
			} // end main loop
		// cleanup
		cleanup:
		hoof_free( & hoof ) ;
		if ( interactive )
			{
			tcsetattr( STDIN_FILENO , TCSANOW , & old_tio ) ;
			}
		// workaround for tests
		if ( rc == hoof_rc_quit )
			{
			rc = 99;
			}
		return rc ;
		}
