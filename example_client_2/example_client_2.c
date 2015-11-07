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
	#include <sys/ioctl.h>  // ioctl
	#include <stdio.h>      // printf fflush
	#include <unistd.h>     // read
	#include <termios.h>    // termios , struct winsize
	#include <signal.h>     // signal
	#include <stdlib.h>     // malloc calloc free atoi
	#include <errno.h>      // errno
	#include "hoof.h"
// defines
	#define history_size 512
// state
	volatile b window_size_changed = 0 ;
	n window_width = 80 ;
	n window_height = 80 ;
// functions
	void signal_window_size_changed( int sig )
		{
		window_size_changed = 1 ;
		}
	n key_available( void )
		{
		struct timeval timeout ;
		fd_set rdset ;
		int rc ;

		FD_ZERO( & rdset ) ;
		FD_SET( STDIN_FILENO , & rdset ) ;
		timeout . tv_sec  = 0 ;
		timeout . tv_usec = 100000 ;

		errno = 0 ;
		do
			{
			rc = select( STDIN_FILENO + 1 , & rdset , NULL , NULL , & timeout ) ; 
			} while ( rc == -1 && errno == EINTR ) ;
		if ( rc > 0 )
			{
			return 1 ;
			}
		return 0 ;
		}
	void get_window_size( void )
		{
		struct winsize winsize ;
		ioctl( 0 , TIOCGWINSZ , & winsize ) ;
		window_width = winsize . ws_col ;
		window_height = winsize . ws_row ;
		}
	void draw_callback( n draw_mode , n column , n row , b * text )
		{
		if ( row <= 0 )
			{
			return ;
			}
		if ( row > window_width )
			{
			return ;
			}
		// if drawing off screen to left, increment text until we're back on screen
		while ( column <= 0 )
			{
			if ( ( * text ) == '\0' )
				{
				return ;
				}
			column += 1 ;
			text += 1 ;
			}
		if ( draw_mode == hoof_draw_current )
			{
			printf( "\x1b[32m" ) ;
			}
		printf( "\x1b[%jd;%jdf%s" , row , column , text );
		if ( draw_mode == hoof_draw_cursor )
			{
			printf( "\x1b[42m " ) ;
			}
		if ( draw_mode == hoof_draw_current || draw_mode == hoof_draw_cursor )
			{
			printf( "\x1b[0m" );
			}
		}
	void draw_everything( struct hoof * hoof, struct hoof_interface * interface , b * history )
		{
		// clear screen
		printf("\x1b[2J");
		// draw hoof state
		hoof_draw( hoof, window_width , window_height - 1 , draw_callback , interface ) ;
		// draw history
		draw_callback( hoof_draw_normal , window_width - history_size, window_height , history ) ;
		// flush
		fflush( stdout ) ;
		}
	void history_add_character( b * history , b character )
		{
		n i = 0 ;
		while ( i + 1 < history_size )
			{
			history[ i ] = history[ i + 1 ] ;
			i += 1 ;
			}
		history[ i ] = character ;
		}
	void history_add_word( b * history , b * word )
		{
		while ( ( * word ) != '\0' )
			{
			history_add_character( history , * word ) ;
			word += 1 ;
			}
		}
	void history_backspace( b * history )
		{
		n i = 0 ;
		i = history_size - 1 ;
		while ( i - 1 >= 0 )
			{
			history[ i ] = history[ i - 1 ] ;
			i -= 1 ;
			}
		history[ 0 ] = ' ' ;
		}
// main
	int main( int argc, b * * argv )
		{
		// data
		n rc = 0 ;
		struct hoof * hoof = NULL ;
		struct hoof_interface interface ;
		b * filename = "" ;
		n done = 0 ;
		struct termios old_tio , new_tio ;
		n read_num = 0 ;
		b ch[ 3 ] = { 0 , 0 , 0 } ;
		// TODO think about this
		b punctuation = 0 ;
		n i = 0 ;
		b history[ history_size + 1 ] = { 0 } ;
		n input_word_index = 0 ;

		// code
		// initialize history
		for ( i = 0 ; i < history_size ; i += 1 )
			{
			history[ i ] = ' ' ;
			}
			history[ history_size ] = '\0' ;

		interface . input_word[ 0 ] = '\0' ;

		if ( argc >= 2 )
			{
			filename = argv[ 1 ] ;
			}
		// setup terminal in noncanonical and non-echoing mode
		tcgetattr( STDIN_FILENO , & old_tio ) ;
		new_tio = old_tio ;
		new_tio . c_lflag &= ( ~ ICANON & ~ ECHO ) ;
		tcsetattr( STDIN_FILENO , TCSANOW , & new_tio ) ;
		// setup hoof
		rc = hoof_init( filename , & hoof ) ;
		if ( rc != hoof_rc_success )
			{
			fprintf( stderr , "error hoof_init failed %s\n" , hoof_rc_to_string( rc ) ) ;
			fflush( stderr ) ;
			goto cleanup ;
			}
		// setup window size signal handler
		if ( signal( SIGWINCH , signal_window_size_changed ) == SIG_ERR )
			{
			fprintf( stderr, "error signal failed %s\n", strerror( errno ) ) ;
			fflush( stderr ) ;
			goto cleanup ;
			}
		// get window size
		get_window_size( );
		// draw
		draw_everything( hoof , & interface , history );
		// main loop
		while ( ! done )
			{
			while ( 1 )
				{
				// did window size change?
				if ( window_size_changed )
					{
					get_window_size( );
					draw_everything( hoof , & interface , history );
					window_size_changed = 0 ;
					}
				// handle automatic next key punctuation thing
				if ( punctuation != 0 )
					{
					if ( punctuation == '\n' || punctuation == '\r' )
						{
						history_add_character( history , ' ' ) ;
						}
					else
						{
						history_add_character( history , punctuation ) ;
						}
					interface . input_word[ 0 ] = punctuation;
					interface . input_word[ 1 ] = '\0';
					punctuation = 0;
					break;
					}
				// wait a little bit for a key press
				if ( key_available( ) )
					{
					// note : need to read 3 because some keys like up down left right come in 3 bytes
					read_num = read( STDIN_FILENO , ch , 3 ) ;
					if ( read_num == 1 )
						{
						// if backspace
						if ( ch[ 0 ] == 127 )
							{
							if ( input_word_index > 0 )
								{
								input_word_index -= 1 ;
								interface . input_word[ input_word_index ] = '\0' ;
								history_backspace( history ) ;
								}
							}
						// else if whitespace
						else if ( ch[ 0 ] == ' ' )
							{
							history_add_character( history , ' ' ) ;
							break ;
							}
						// TODO: puncutation, commands
						// else if punctuation (TODO call hoof function?)
						else if ( ch[ 0 ] == ',' || ch[ 0 ] == '.' || ch[ 0 ] == '?' || ch[ 0 ] == '!' || ch[ 0 ] == 'D' || ch[ 0 ] == 'B' || ch[ 0 ] == '\r' || ch[ 0 ] == '\n' )
							{
							punctuation = ch[ 0 ];
							break ;
							}
						// else character
						else if ( input_word_index < hoof_max_word_length )
							{
							interface . input_word[ input_word_index ] = ch[ 0 ] ;
							input_word_index += 1 ;
							interface . input_word[ input_word_index ] = '\0' ;
							history_add_character( history , ch[ 0 ] ) ;
							}
						}
					// only allowed to use arrow keys when youre not in the middle of typing an input word
					else if ( read_num == 3 && interface . input_word[ 0 ] == '\0' )
						{
						// TODO: change this to special characters
						// up
						if ( ch[ 0 ] == 27 && ch[ 1 ] == '[' && ch[ 2 ] == 'A' )
							{
							history_add_character( history , 'u' ) ;
							history_add_character( history , 'p' ) ;
							history_add_character( history , ' ' ) ;
							interface . input_word[ 0 ] = 'u' ;
							interface . input_word[ 1 ] = 'p' ;
							interface . input_word[ 2 ] = '\0' ;
							break ;
							}
						// down
						else if ( ch[ 0 ] == 27 && ch[ 1 ] == '[' && ch[ 2 ] == 'B' )
							{
							history_add_character( history , 'd' ) ;
							history_add_character( history , 'o' ) ;
							history_add_character( history , 'w' ) ;
							history_add_character( history , 'n' ) ;
							history_add_character( history , ' ' ) ;
							interface . input_word[ 0 ] = 'd' ;
							interface . input_word[ 1 ] = 'o' ;
							interface . input_word[ 2 ] = 'w' ;
							interface . input_word[ 3 ] = 'n' ;
							interface . input_word[ 4 ] = '\0' ;
							break ;
							}
						// right
						else if ( ch[ 0 ] == 27 && ch[ 1 ] == '[' && ch[ 2 ] == 'C' )
							{
							history_add_character( history , 'i' ) ;
							history_add_character( history , 'n' ) ;
							history_add_character( history , ' ' ) ;
							interface . input_word[ 0 ] = 'i' ;
							interface . input_word[ 1 ] = 'n' ;
							interface . input_word[ 2 ] = '\0' ;
							break ;
							}
						// left
						else if ( ch[ 0 ] == 27 && ch[ 1 ] == '[' && ch[ 2 ] == 'D' )
							{
							history_add_character( history , 'o' ) ;
							history_add_character( history , 'u' ) ;
							history_add_character( history , 't' ) ;
							history_add_character( history , ' ' ) ;
							interface . input_word[ 0 ] = 'o' ;
							interface . input_word[ 1 ] = 'u' ;
							interface . input_word[ 2 ] = 't' ;
							interface . input_word[ 3 ] = '\0' ;
							break ;
							}
						}
					draw_everything( hoof , & interface , history );
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
				}
			// add hoof output to history
			// add space to separate input from output
			if ( interface . output_value[ 0 ][ 0 ] != '\0' )
				{
				// add space to separate input from output
				history_add_character( history , ' ' ) ;
				history_add_character( history , ' ' ) ;
				for ( i = 0 ; i <= hoof_max_value_length && interface . output_value[ i ][ 0 ] != '\0' ; i += 1 )
					{
					history_add_word( history , interface . output_value[ i ] ) ;
					history_add_character( history , ' ' ) ;
					}
				// add space to separate input from output
				history_add_character( history , ' ' ) ;
				history_add_character( history , ' ' ) ;
				}
			// reset input
			input_word_index = 0 ;
			interface . input_word[ 0 ] = '\0' ;
			// draw
			draw_everything( hoof , & interface , history );
			} // end main loop
		// make sure we print a newline so the prompt is good
		printf( "\n" ) ;
		fflush( stdout ) ;
		// cleanup
		cleanup:
		hoof_free( & hoof ) ;
		tcsetattr( STDIN_FILENO , TCSANOW , & old_tio ) ;
		return rc ;
		}
