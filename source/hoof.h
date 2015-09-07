// copyright 2014 to 2015 jeremiah martell
// all rights reserved
/* LICENSE BSD 3 CLAUSE
	Redistribution and use in source and binary forms, with or without modification, are permitted provided that the following conditions are met:
	- Redistributions of source code must retain the above copyright notice, this list of conditions and the following disclaimer.
	- Redistributions in binary form must reproduce the above copyright notice, this list of conditions and the following disclaimer in the documentation and/or other materials provided with the distribution.
	- Neither the name of Jeremiah Martell nor the name of Geek Horse nor the name of Hoof nor the names of its contributors may be used to endorse or promote products derived from this software without specific prior written permission.
	THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
	*/
#ifndef hoof_h
	#define hoof_h
	// includes
		#include <inttypes.h>
	// you only need 2 atomic types, byte and large signed integer
		// byte
		#ifndef b
			#define b uint8_t
			#endif
		// large signed integer
		#ifndef n
			#define n int64_t
			#endif
	// hoof information
		#define hoof_name "hoof"
		#define hoof_copyright "copyright 2014-2015 jeremiah martell"
		#define hoof_version_string "0 9 01 wip"
		#define hoof_version 9010
		#define hoof_version_major       ( hoof_version / 10000 )
		#define hoof_version_minor       ( ( hoof_version / 1000 ) % 10 )
		#define hoof_version_subminor    ( ( hoof_version / 10 ) % 100 )
		#define hoof_version_final       ( hoof_version % 10 )
	// standard geek horse rc
		#define hoof_rc_success                       0
		#define hoof_rc_error_precond                 1
		#define hoof_rc_error_memory                  2
		#define hoof_rc_error_file                    3
		#define hoof_rc_error_network                 4
		#define hoof_rc_error_standard_library        5
		#define hoof_rc_error_failure_point           6
		#define hoof_rc_standard_errors_max           6
	// hoof rc
		#define hoof_rc_quit                        301
		#define hoof_rc_error_filename_bad          302
		#define hoof_rc_error_filename_long         303
		#define hoof_rc_error_file_bad              304
		#define hoof_rc_error_word_bad              305
		#define hoof_rc_error_word_long             306
		#define hoof_rc_error_value_long            307
		#define hoof_rc_hoof_errors_min             301
		#define hoof_rc_hoof_errors_max             307
	// defines
		// TODO: rename these: hoof_word_length_max hoof_value_length_max
		#define hoof_max_word_length  31
		// TODO: probably need to change the maximum word length to 32
		// TODO: need max words per value, probably 32 too
		#define hoof_max_value_length 30
		// TODO: or change both of these to 30
	// types
		struct hoof ;
		struct hoof_interface
			{
			b input_word [ hoof_max_word_length + 1 ] ;
			b output_value [ hoof_max_value_length + 1 ] [ hoof_max_word_length + 1 ] ;
			} ;
		typedef void ( * hoof_draw_function )( n column , n row , b * text ) ;
	// public functions
		n hoof_init( b * filename , struct hoof * * hoof_a ) ;
		void hoof_free( struct hoof * * hoof_f ) ;
		n hoof_set_draw_function( struct hoof * hoof , n max_columns , n max_rows , hoof_draw_function draw_function ) ;
		n hoof_do( struct hoof * hoof , struct hoof_interface * hoof_interface ) ;
		const b * hoof_rc_to_string( n rc ) ;
	#endif
