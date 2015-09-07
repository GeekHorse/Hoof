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
	#include "hoof.h"
	#include <stdlib.h> // malloc calloc free
	#include <stdio.h> // FILE fopen fread fwrite fclose rename remove
// defines
	#define null NULL
	// error handling
		#define err_if( cond, error_to_return ) \
			if ( cond ) \
				{ \
				hoof_hook_log( hoof_name " " hoof_version_string , __LINE__ , error_to_return , 0 , 0 , 0 ) ; \
				rc = error_to_return ; \
				goto cleanup ; \
				}
		#define err_passthrough( func ) \
			rc = ( func ) ; \
			err_if( rc != hoof_rc_success , rc ) ;
		#ifdef hoof_be_paranoid
			#define paranoid_err_if( cond ) \
				if ( cond ) \
					{ \
					fprintf( stderr , "hoof paranoid error! %s %d" , __FILE__ , __LINE__ ) ; \
					fflush( stderr ) ; \
					exit( -55 ) ; \
					}
		#else
			#define paranoid_err_if( cond ) 
		#endif
	// memory macros
		#define hoof_memory_malloc( pointer , pointer_type , count ) \
			pointer = ( pointer_type * ) hoof_hook_malloc( sizeof( pointer_type ) * ( count ) ) ; \
			err_if( pointer == null , hoof_rc_error_memory ) ;
		#define hoof_memory_calloc( pointer , pointer_type , count ) \
			pointer = ( pointer_type * ) hoof_hook_calloc( count , sizeof( pointer_type ) ) ; \
			err_if( pointer == null , hoof_rc_error_memory ) ;
		#define hoof_memory_free( pointer ) \
			hoof_hook_free( pointer ) ; \
			pointer = null ;
		#ifdef hoof_use_mem_hooks
			extern void *hoof_hook_malloc( size_t size ) ;
			extern void *hoof_hook_calloc( size_t nmemb , size_t size ) ;
			extern void hoof_hook_free( void * ptr ) ;
		#else
			#define hoof_hook_malloc malloc
			#define hoof_hook_calloc calloc
			#define hoof_hook_free free
		#endif
	// file hooks
		// note: these are just used to test file failures. we still use feof fclose and remove directly
		#ifdef hoof_use_file_hooks
			extern FILE *hoof_hook_fopen( const b * path , const b * mode ) ;
			extern size_t hoof_hook_rread( void * ptr , size_t size , size_t nmemb , FILE * stream ) ;
			extern size_t hoof_hook_rwrite( const void * ptr , size_t size , size_t nmemb , FILE * stream ) ;
			extern int hoof_hook_rename( const b * oldpath , const b * newpath ) ;
		#else
			#define hoof_hook_fopen fopen
			#define hoof_hook_fread fread
			#define hoof_hook_fwrite fwrite
			#define hoof_hook_rename rename
		#endif
	// logging
		// TODO: still need to think about a b c and if we want to change this function type
		#ifdef hoof_enable_logging
			extern void hoof_hook_log( b * library , n line_number , n rc , n a , n b , n c ) ;
		#else
			#define hoof_hook_log( library, line_number, rc, a, b, c )
		#endif
	// macros to make the code easier to read
		#define hear( word ) ( hoof_words_are_same( ( b * ) word , interface->input_word ) )
		#define say( word ) hoof_output( ( b * ) word , interface ) ;
	// misc
		#define hoof_int_max_string         "9223372036854775807"
		#define hoof_int_max_string_length 19
		#define hoof_int_min_string        "-9223372036854775808"
		#define hoof_int_min_string_length 20
// structures
	/*	note about the internal structure:
		pages have a value head and value tail, both with word_head = null
		pages can not be empty, they must contain at least 1 real value
			the head value, a real value, and the tail value
			TODO: this will change, pages should be able to be empty, and the current value can be the head or tail value
		values have a word head and word tail, both with value = null
		values can be empty
			empty value is only word head and word tail
		current_value will never be a value head or value tail
			TODO: this will change
		current_word may be a value head or a real word
		root is the value head of the root page
		*/
	struct hoof_word
	{
		struct hoof_word * left ;
		struct hoof_word * right ;
		b * value ; // bad name, need to be changed
	} ;
	struct hoof_value
	{
		struct hoof_value * up ;
		struct hoof_value * down ;
		struct hoof_value * in ;
		struct hoof_value * out ;
		struct hoof_word * word_head ;
	} ;
	struct hoof
	{
		b * filename ;
		// TODO: when we switch to a new file format, we can get rid of loading
		n loading ;
		n paused ;
		n literal ;
		n max_columns ;
		n max_rows ;
		void ( * draw_function )( n column , n row , b * text ) ;
		n ( * state )( struct hoof * hoof , struct hoof_interface * interface , n * huh ) ;
		struct hoof_value * root ;
		struct hoof_value * current_value ;
		struct hoof_word * current_word ;
	} ;
// static function prototypes
	static n hoof_words_are_same( b * word_1 , b * word_2 ) ;
	// states
		static n hoof_state_hello( struct hoof * hoof , struct hoof_interface * interface , n * huh ) ;
		static n hoof_state_navigate( struct hoof * hoof , struct hoof_interface * interface , n * huh ) ;
		static n hoof_state_most_choice( struct hoof * hoof , struct hoof_interface * interface , n * huh ) ;
		static n hoof_state_new_choice( struct hoof * hoof , struct hoof_interface *interface , n * huh ) ;
		static n hoof_state_new( struct hoof * hoof , struct hoof_interface * interface , n * huh ) ;
		static n hoof_state_delete_choice( struct hoof * hoof , struct hoof_interface * interface , n * huh ) ;
		static n hoof_state_move_choice( struct hoof * hoof , struct hoof_interface * interface , n * huh ) ;
		static n hoof_state_dig( struct hoof * hoof , struct hoof_interface * interface , n * huh ) ;
	// loading and saving
		static n hoof_load( struct hoof * hoof ) ;
		static n hoof_save_word( FILE * fp , b * word , n newline ) ;
		static n hoof_save_value( FILE * fp , struct hoof_value * value , b * direction ) ;
		static n hoof_save( struct hoof  *hoof ) ;
	// drawing
		static n hoof_word_length( b * word ) ;
		static void hoof_draw_value( hoof_draw_function draw_function , struct hoof_value * value , n max_columns , n row , n * row_size ) ;
		static void hoof_draw( struct hoof * hoof ) ;
	static void hoof_output( const b * what_to_output, struct hoof_interface *interface ) ;
	static n hoof_word_verify( b *word ) ;
	static n hoof_strdup( b *word_in, b **word_out_A ) ;
	static void hoof_make_current_value( struct hoof *hoof, struct hoof_value *value ) ;
	static void hoof_root( struct hoof *hoof ) ;
	static void hoof_most_up( struct hoof *hoof ) ;
	static void hoof_most_down( struct hoof *hoof ) ;
	static void hoof_most_out( struct hoof *hoof ) ;
	static void hoof_most_in( struct hoof *hoof ) ;
	static n hoof_word_insert( struct hoof *hoof, b *value ) ;
	static n hoof_value_insert( struct hoof_value *before ) ;
	static n hoof_page_init( struct hoof_value *parent, n create_empty_value, struct hoof_value **page_A ) ;
	static void hoof_value_clear( struct hoof *hoof, struct hoof_value *value ) ;
	static void hoof_word_delete( struct hoof *hoof ) ;
	static void hoof_value_delete( struct hoof *hoof ) ;
	static void hoof_page_delete( struct hoof *hoof, struct hoof_value **page_F ) ;
	static void hoof_dig( struct hoof *hoof, b *word ) ;
// functions
	static n hoof_words_are_same( b * word_1 , b * word_2 )
		{
		while ( ( * word_1 ) != '\0' || ( * word_2 ) != '\0' )
			{
			if ( ( * word_1 ) != ( * word_2 ) )
				{
				return 0 ;
				}
			word_1 += 1 ;
			word_2 += 1 ;
			}
		return 1 ;
		}
	// states
		static n hoof_state_hello( struct hoof *hoof, struct hoof_interface *interface, n *huh )
			{
			/* DATA */
			n rc = hoof_rc_success;


			/* CODE */
			(void)huh;
			say( (b *) "hello" );
			hoof->state = hoof_state_navigate;

			return rc;
			}
		static n hoof_state_navigate( struct hoof *hoof, struct hoof_interface *interface, n *huh )
			{
			/* DATA */
			n rc = hoof_rc_success;

			struct hoof_word *read_word = null;


			/* CODE */
			if ( hear( "quit" ) )
			{
				err_if( hoof->loading, hoof_rc_error_file_bad );

				err_passthrough( hoof_save( hoof ) );
				say( "goodbye" );
				rc = hoof_rc_quit;
			}
			else if ( hear( "cancel" ) )
			{
				say( "navigate" );
			}
			else if ( hear( "left" ) )
			{
				paranoid_err_if( hoof->current_word->left == null );
				if ( hoof->current_word->left->value == null )
				{
					say( "edge" );
					goto cleanup;
				}

				hoof->current_word = hoof->current_word->left;
				say( "ok" );
			}	
			else if ( hear( "right" ) )
			{
				if (    hoof->current_word->right == null
					 || hoof->current_word->right->value == null
				   )
				{
					say( "edge" );
					goto cleanup;
				}

				hoof->current_word = hoof->current_word->right;
				say( "ok" );
			}	
			else if ( hear( "up" ) )
			{
				if ( hoof->current_value->up->word_head == null )
				{
					say( "edge" );
					goto cleanup;
				}

				hoof_make_current_value( hoof, hoof->current_value->up );
				say( "ok" );
			}	
			else if ( hear( "down" ) )
			{
				if ( hoof->current_value->down->word_head == null )
				{
					say( "edge" );
					goto cleanup;
				}

				hoof_make_current_value( hoof, hoof->current_value->down );
				say( "ok" );
			}	
			else if ( hear( "in" ) )
			{
				if ( hoof->current_value->in == null )
				{
					say( "edge" );
					goto cleanup;
				}

				hoof_make_current_value( hoof, hoof->current_value->in->down );
				say( "ok" );
			}	
			else if ( hear( "out" ) )
			{
				if ( hoof->current_value->out == null )
				{
					say( "edge" );
					goto cleanup;
				}

				hoof_make_current_value( hoof, hoof->current_value->out );
				say( "ok" );
			}
			else if ( hear( "root" ) )
			{
				hoof_make_current_value( hoof, hoof->root->down );

				say( "ok" );
			}
			else if ( hear( "save" ) )
			{
				err_if( hoof->loading, hoof_rc_error_file_bad );

				err_passthrough( hoof_save( hoof ) );

				say( "ok" );
			}
			else if ( hear( "clear" ) )
			{
				hoof_value_clear( hoof, hoof->current_value );

				say( "ok" );
			}
			else if ( hear( "word" ) )
			{
				if ( hoof->current_word->value == null )
				{
					say( "empty" );
				}
				else
				{
					say( "ok" );
					say( hoof->current_word->value );
				}
				hoof->state = hoof_state_navigate;
			}
			else if ( hear( "value" ) )
			{
				read_word = hoof->current_value->word_head->right;

				if ( read_word->value == null )
				{
					say( "empty" );
				}
				else
				{
					say( "ok" );
					while ( read_word->value != null )
					{
						say( read_word->value );
						read_word = read_word->right;
					}
				}
				hoof->state = hoof_state_navigate;
			}
			else if ( hear( "most" ) )
			{
				hoof->state = hoof_state_most_choice;
			}
			else if ( hear( "new" ) )
			{
				hoof->state = hoof_state_new_choice;
			}
			else if ( hear( "delete" ) )
			{
				hoof->state = hoof_state_delete_choice;
			}
			else if ( hear( "move" ) )
			{
				hoof->state = hoof_state_move_choice;
			}
			else if ( hear( "dig" ) )
			{
				hoof->current_word = hoof->current_value->word_head;

				hoof->state = hoof_state_dig;
			}
			else
			{
				(*huh) = 1;
			}


			/* CLEANUP */
			cleanup:

			return rc;
			}
		static n hoof_state_most_choice( struct hoof *hoof, struct hoof_interface *interface, n *huh )
			{
			/* DATA */
			n rc = hoof_rc_success;


			/* CODE */
			if ( hear( "cancel" ) )
			{
				hoof->state = hoof_state_navigate;
				say( "cancel" );
			}
			else if ( hear( "left" ) )
			{
				while ( hoof->current_word->left->value != null )
				{
					hoof->current_word = hoof->current_word->left;
				}

				hoof->state = hoof_state_navigate;
				say( "ok" );
			}
			else if ( hear( "right" ) )
			{
				while (    hoof->current_word->right != null
						&& hoof->current_word->right->value != null
					  )
				{
					hoof->current_word = hoof->current_word->right;
				}

				hoof->state = hoof_state_navigate;
				say( "ok" );
			}
			else if ( hear( "up" ) )
			{
				hoof_most_up( hoof );

				hoof->state = hoof_state_navigate;
				say( "ok" );
			}
			else if ( hear( "down" ) )
			{
				hoof_most_down( hoof );

				hoof->state = hoof_state_navigate;
				say( "ok" );
			}
			else if ( hear( "in" ) )
			{
				hoof_most_in( hoof );

				hoof->state = hoof_state_navigate;
				say( "ok" );
			}
			else if ( hear( "out" ) )
			{
				hoof_most_out( hoof );

				hoof->state = hoof_state_navigate;
				say( "out" );
			}
			else
			{
				(*huh) = 1;
			}


			/* CLEANUP */
			/* cleanup: */

			return rc;
			}
		static n hoof_state_new_choice( struct hoof *hoof, struct hoof_interface *interface, n *huh )
			{
			/* DATA */
			n rc = hoof_rc_success;


			/* CODE */
			if ( hear( "cancel" ) )
			{
				hoof->state = hoof_state_navigate;
				say( "cancel" );
			}
			else if ( hear( "left" ) )
			{
				/* we insert before current_word, so we don't need to update current_word here */
				hoof->state = hoof_state_new;
				say( "new" );
			}	
			else if ( hear( "right" ) )
			{
				if ( hoof->current_word->right != null )
				{
					hoof->current_word = hoof->current_word->right;
				}
				hoof->state = hoof_state_new;
				say( "new" );
			}	
			else if ( hear( "up" ) )
			{
				err_passthrough( hoof_value_insert( hoof->current_value->up ) );

				hoof_make_current_value( hoof, hoof->current_value->up );

				hoof->state = hoof_state_new;
				say( "new" );
			}	
			else if ( hear( "down" ) )
			{
				err_passthrough( hoof_value_insert( hoof->current_value ) );

				hoof_make_current_value( hoof, hoof->current_value->down );

				hoof->state = hoof_state_new;
				say( "new" );
			}	
			else if ( hear( "in" ) )
			{
				if ( hoof->current_value->in == null )
				{
					err_passthrough( hoof_page_init( hoof->current_value, 1, null ) );
				}
				else
				{
					err_passthrough( hoof_value_insert( hoof->current_value->in ) );
				}

				hoof_make_current_value( hoof, hoof->current_value->in->down );

				hoof->state = hoof_state_new;
				say( "new" );
			}	
			else if ( hear( "out" ) )
			{
				if ( hoof->current_value->out == null )
				{
					say( "edge" );
					goto cleanup;
				}

				err_passthrough( hoof_value_insert( hoof->current_value->out ) );
				hoof_make_current_value( hoof, hoof->current_value->out->down );

				hoof->state = hoof_state_new;
				say( "new" );
			}	
			else
			{
				(*huh) = 1;
			}


			/* CLEANUP */
			cleanup:

			return rc;
			}
		static n hoof_state_new( struct hoof *hoof, struct hoof_interface *interface, n *huh )
			{
			/* DATA */
			n rc = hoof_rc_success;


			/* CODE */
			(void)huh;

			/* FUTURE: handle number */
			if ( hear( "" ) )
			{
				goto cleanup;
			}
			else if ( hoof->literal )
			{
				err_passthrough( hoof_word_insert( hoof, interface->input_word ) );

				hoof->literal = 0;
			}
			else if ( hear( "literal" ) )
			{
				hoof->literal = 1;
			}
			else if ( hear( "done" ) )
			{
				if ( hoof->current_word->left->value != null )
				{
					hoof->current_word = hoof->current_word->left;
				}

				hoof->state = hoof_state_navigate;
				say( "ok" );
			}
			else
			{
				err_passthrough( hoof_word_insert( hoof, interface->input_word ) );
			}


			/* CLEANUP */
			cleanup:

			return rc;
			}
		static n hoof_state_delete_choice( struct hoof *hoof, struct hoof_interface *interface, n *huh )
			{
			/* DATA */
			n rc = hoof_rc_success;


			/* CODE */
			if ( hear( "cancel" ) )
			{
				hoof->state = hoof_state_navigate;
				say( "cancel" );
			}
			else if ( hear( "word" ) )
			{
				hoof_word_delete( hoof );

				hoof->state = hoof_state_navigate;
				say( "ok" );
			}
			else if ( hear( "value" ) )
			{
				hoof_value_delete( hoof );

				hoof->state = hoof_state_navigate;
				say( "ok" );
			}
			else
			{
				(*huh) = 1;
			}


			/* CLEANUP */
			/* cleanup: */

			return rc;
			}
		static n hoof_state_move_choice( struct hoof *hoof, struct hoof_interface *interface, n *huh )
			{
			/* DATA */
			n rc = hoof_rc_success;


			/* CODE */
			if ( hear( "cancel" ) )
			{
				hoof->state = hoof_state_navigate;
				say( "cancel" );
			}
			else if ( hear( "left" ) )
			{
				if (    hoof->current_word->value == null
					 || hoof->current_word->left->value == null
				   )
				{
					hoof->state = hoof_state_navigate;
					say( "edge" );
					goto cleanup;
				}

				/* remove word from list */
				hoof->current_word->left->right = hoof->current_word->right;
				hoof->current_word->right->left = hoof->current_word->left;

				/* update word's links */
				hoof->current_word->right = hoof->current_word->left;
				hoof->current_word->left  = hoof->current_word->left->left;

				/* update left and right links */
				hoof->current_word->left->right = hoof->current_word;
				hoof->current_word->right->left = hoof->current_word;

				hoof->state = hoof_state_navigate;
				say( "ok" );
			}
			else if ( hear( "right" ) )
			{
				if (    hoof->current_word->value == null
					 || hoof->current_word->right->value == null
				   )
				{
					hoof->state = hoof_state_navigate;
					say( "edge" );
					goto cleanup;
				}

				/* remove word from list */
				hoof->current_word->left->right = hoof->current_word->right;
				hoof->current_word->right->left = hoof->current_word->left;

				/* update word's links */
				hoof->current_word->left  = hoof->current_word->right;
				hoof->current_word->right = hoof->current_word->right->right;

				/* update left and right links */
				hoof->current_word->left->right = hoof->current_word;
				hoof->current_word->right->left = hoof->current_word;

				hoof->state = hoof_state_navigate;
				say( "ok" );
			}
			else if ( hear( "up" ) )
			{
				if ( hoof->current_value->up->word_head == null )
				{
					hoof->state = hoof_state_navigate;
					say( "edge" );
					goto cleanup;
				}

				/* remove value from list */
				hoof->current_value->up->down = hoof->current_value->down;
				hoof->current_value->down->up = hoof->current_value->up;

				/* update value's links */
				hoof->current_value->down = hoof->current_value->up;
				hoof->current_value->up   = hoof->current_value->up->up;

				/* update up and down links */
				hoof->current_value->up->down = hoof->current_value;
				hoof->current_value->down->up = hoof->current_value;

				hoof->state = hoof_state_navigate;
				say( "ok" );
			}
			else if ( hear( "down" ) )
			{
				if ( hoof->current_value->down->word_head == null )
				{
					hoof->state = hoof_state_navigate;
					say( "edge" );
					goto cleanup;
				}

				/* remove value from list */
				hoof->current_value->up->down = hoof->current_value->down;
				hoof->current_value->down->up = hoof->current_value->up;

				/* update value's links */
				hoof->current_value->up   = hoof->current_value->down;
				hoof->current_value->down = hoof->current_value->down->down;

				/* update up and down links */
				hoof->current_value->up->down = hoof->current_value;
				hoof->current_value->down->up = hoof->current_value;

				hoof->state = hoof_state_navigate;
				say( "ok" );
			}
			else if ( hear( "in" ) )
			{
				if ( hoof->current_value->up->word_head == null )
				{
					hoof->state = hoof_state_navigate;
					say( "edge" );
					goto cleanup;
				}

				if ( hoof->current_value->up->in == null )
				{
					err_passthrough( hoof_page_init( hoof->current_value->up, 0, null ) );
				}

				/* remove value from list */
				hoof->current_value->up->down = hoof->current_value->down;
				hoof->current_value->down->up = hoof->current_value->up;

				/* update value's links */
				hoof->current_value->down = hoof->current_value->up->in->down;
				hoof->current_value->up   = hoof->current_value->up->in;

				/* update up and down links */
				hoof->current_value->up->down = hoof->current_value;
				hoof->current_value->down->up = hoof->current_value;

				/* update out link */
				hoof->current_value->out = hoof->current_value->up->out;

				hoof->state = hoof_state_navigate;
				say( "ok" );
			}
			else if ( hear( "out" ) )
			{
				if ( hoof->current_value->out == null )
				{
					hoof->state = hoof_state_navigate;
					say( "edge" );
					goto cleanup;
				}

				/* remove value from list */
				hoof->current_value->up->down = hoof->current_value->down;
				hoof->current_value->down->up = hoof->current_value->up;

				/* update value's links */
				hoof->current_value->up   = hoof->current_value->out;
				hoof->current_value->down = hoof->current_value->out->down;

				/* update up and down links */
				hoof->current_value->up->down = hoof->current_value;
				hoof->current_value->down->up = hoof->current_value;

				/* update out */
				hoof->current_value->out  = hoof->current_value->up->out;

				/* see if up's in (old out's in) is empty */
				if ( hoof->current_value->up->in->down->word_head == null )
				{
					hoof_memory_free( hoof->current_value->up->in->down );
					hoof_memory_free( hoof->current_value->up->in );
					hoof->current_value->up->in = null;
				}

				hoof->state = hoof_state_navigate;
				say( "ok" );
			}
			else
			{
				(*huh) = 1;
			}


			/* CLEANUP */
			cleanup:

			return rc;
			}
		static n hoof_state_dig( struct hoof *hoof, struct hoof_interface *interface, n *huh )
			{
			/*!	\brief State Dig. Allows users to quickly navigate the heirarchy by 'find' and 'in'.
				\param[in] hoof struct hoof Context.
				\param[in] interface struct hoof Interface.
				\param[out] huh If this state doesn't understand the input word, it will set huh to 1.
				\return HOOF_RC

				Can temporarily set current_value to tail value during this state, but will leave this state with current_value set to most down value.
				*/
			/* DATA */
			n rc = hoof_rc_success;


			/* CODE */
			(void)huh;

			/* FUTURE: handle number */
			if ( hear( "" ) )
			{
				goto cleanup;
			}
			else if ( hoof->literal )
			{
				hoof_dig( hoof, interface->input_word );
				hoof->literal = 0;
			}
			else if ( hear( "literal" ) )
			{
				hoof->literal = 1;
			}
			else if ( hear( "in" ) )
			{
				/* we want to find a value that's an exact match */
				hoof_dig( hoof, null );

				/* if tail */
				if ( hoof->current_value->word_head == null )
				{
					/* we're done with our dig */
					goto cleanup;
				}

				/* if there's not an in */
				if ( hoof->current_value->in == null )
				{
					/* go to tail to mark that we couldnt go in, which
					   will also end our dig */
					while ( hoof->current_value->down != null )
					{
						hoof->current_value = hoof->current_value->down;
					}

					goto cleanup;
				}

				/* go in */
				hoof_make_current_value( hoof, hoof->current_value->in->down );
				/* need to start with current_word as word_head */
				hoof->current_word = hoof->current_value->word_head;
			}
			else if ( hear( "done" ) )
			{
				/* we want to find a value that's an exact match */
				hoof_dig( hoof, null );

				/* if tail */
				if ( hoof->current_value->word_head == null )
				{
					hoof_make_current_value( hoof, hoof->current_value->up );
					hoof->state = hoof_state_navigate;
					say( "edge" );
				}
				else
				{
					hoof_make_current_value( hoof, hoof->current_value );
					hoof->state = hoof_state_navigate;
					say( "ok" );
				}
			}
			else if ( hear( "cancel" ) )
			{
				/* we want to find a value that just "starts with", so stick with whatever value we're currently at */

				/* if tail */
				if ( hoof->current_value->word_head == null )
				{
					hoof_make_current_value( hoof, hoof->current_value->up );
					hoof->state = hoof_state_navigate;
					say( "edge" );
				}
				else
				{
					hoof_make_current_value( hoof, hoof->current_value );
					hoof->state = hoof_state_navigate;
					say( "ok" );
				}
			}
			else
			{
				hoof_dig( hoof, interface->input_word );
			}


			/* CLEANUP */
			cleanup:

			return rc;
			}
	// loading and saving
		static n hoof_load( struct hoof *hoof )
			{
			/* DATA */
			n rc = hoof_rc_success;

			FILE *fp = null;

			struct hoof_interface interface;

			n done = 0;

			n i = 0;
			b ch = 0;


			/* CODE */
			hoof->loading = 1;
			hoof->state = hoof_state_navigate;

			fp = hoof_hook_fopen( ( char * ) hoof->filename, "r" );
			err_if( fp == null, hoof_rc_error_file );

			while ( 1 )
			{
				/* get next word */
				i = 0;
				interface.input_word[ 0 ] = '\0';

				while ( 1 )
				{
					if ( hoof_hook_fread( &ch, 1, 1, fp ) != 1 )
					{
						err_if( ! feof( fp ), hoof_rc_error_file );

						done = 1;
						break;
					}

					if ( ch == ' ' || ch == '\n' || ch == '\r' )
					{
						break;
					}

					err_if( i >= hoof_max_word_length, hoof_rc_error_word_long );

					interface.input_word[ i ] = ch;
					interface.input_word[ i + 1 ] = '\0';

					i += 1;
				}

				if ( done )
				{
					break;
				}

				/* feed into hoof_do */
				err_passthrough( hoof_do( hoof, &interface ) );
				/* TODO: when we have a better loading file format, this can go away */
				err_if( interface.output_value[ 1 ][ 0 ] != '\0', hoof_rc_error_file_bad );
			}

			hoof_root( hoof );

			hoof->loading = 0;
			hoof->state = hoof_state_hello;


			/* CLEANUP */
			cleanup:

			if ( fp != null )
			{
				fclose( fp );
				fp = null;
			}

			return rc;
			}
		static n hoof_save_word( FILE *fp, b *word, n newline )
			{
			/* DATA */
			n rc = hoof_rc_success;

			size_t i = 0;

			b ch_space = ' ';
			b ch_newline = '\n';


			/* CODE */
			while ( word[ i ] != '\0' )
			{
				i += 1;
			}

			err_if( hoof_hook_fwrite( word, 1, i, fp ) != i, hoof_rc_error_file );

			if ( newline )
			{
				err_if( hoof_hook_fwrite( &ch_newline, 1, 1, fp ) != 1, hoof_rc_error_file );
			}
			else
			{
				err_if( hoof_hook_fwrite( &ch_space, 1, 1, fp ) != 1, hoof_rc_error_file );
			}


			/* CLEANUP */
			cleanup:

			return rc;
			}
		static n hoof_save_value( FILE *fp, struct hoof_value *value, b *direction )
			{
			/* DATA */
			n rc = hoof_rc_success;

			struct hoof_word *word = null;


			/* CODE */
			paranoid_err_if( value == null );
			paranoid_err_if( value->word_head == null );
			paranoid_err_if( value->word_head->value != null );
			paranoid_err_if( value->word_head->right == null );
			paranoid_err_if( value->word_head->right->left != value->word_head );
			paranoid_err_if( value->word_head->left != null );

			word = value->word_head->right;

			err_passthrough( hoof_save_word( fp, ( b * ) "new", 0 ) );
			err_passthrough( hoof_save_word( fp, direction, 0 ) );

			paranoid_err_if( word == null );

			while ( word->value != null )
			{
				paranoid_err_if( word->left->right != word );
				paranoid_err_if( word->right->left != word );

				if (    hoof_words_are_same( word->value, ( b * ) "done" )
					 || hoof_words_are_same( word->value, ( b * ) "pause" )
					 || hoof_words_are_same( word->value, ( b * ) "literal" )
				   )
				{
					err_passthrough( hoof_save_word( fp, ( b * ) "literal", 0 ) );
				}

				err_passthrough( hoof_save_word( fp, word->value, 0 ) );

				word = word->right;
			}

			paranoid_err_if( word->left->right != word );
			paranoid_err_if( word->right != null );

			err_passthrough( hoof_save_word( fp, ( b * ) "done", 1 ) );


			/* CLEANUP */
			cleanup:

			return rc;
			}
		static n hoof_save( struct hoof *hoof )
			{
			/* DATA */
			n rc = hoof_rc_success;

			n i = 0;

			/* +1 for null +1 for dot at beginning */
			b temp_filename[ hoof_max_word_length + 2 ] = "";

			FILE *fp = null;

			struct hoof_value *value = null;

			n first = 1;


			/* CODE */
			/* create temp filename */
			temp_filename[ 0 ] = '.';
			while ( hoof->filename[ i ] != '\0' )
			{
				temp_filename[ i + 1 ] = hoof->filename[ i ];
				i += 1;
			}
			temp_filename[ i + 1 ] = '\0';

			/* open temp filename */
			fp = hoof_hook_fopen( ( char * ) temp_filename, "w+" );
			err_if( fp == null, hoof_rc_error_file );

			/* write file */
			value = hoof->root->down;

			paranoid_err_if( value == null );
			paranoid_err_if( value->word_head == null );
			paranoid_err_if( value->up->word_head != null );

			while ( value != null )
			{
				paranoid_err_if( value->up->down != value );
				paranoid_err_if( value->down->up != value );
				paranoid_err_if( value->out != value->up->out );
				paranoid_err_if( value->out != value->down->out );

				if ( first )
				{
					err_passthrough( hoof_save_value( fp, value, ( b * ) "right" ) );
					first = 0;
				}
				else
				{
					err_passthrough( hoof_save_value( fp, value, ( b * ) "down" ) );
				}

				/* go most in */
				while ( value->in != null )
				{
					paranoid_err_if( value->in->out != value );
					paranoid_err_if( value->in->word_head != null );
					paranoid_err_if( value->in->down == null );

					value = value->in->down;

					paranoid_err_if( value->up->down != value );
					paranoid_err_if( value->down->up != value );
					paranoid_err_if( value->out != value->up->out );
					paranoid_err_if( value->out != value->down->out );

					err_passthrough( hoof_save_value( fp, value, ( b * ) "in" ) );
				}

				/* go down */
				value = value->down;

				paranoid_err_if( value->up->down != value );
				paranoid_err_if( value->down != null && value->down->up != value );
				paranoid_err_if( value->out != value->up->out );
				paranoid_err_if( value->down != null && value->out != value->down->out );

				/* if tail */
				if ( value->word_head == null )
				{
					paranoid_err_if( value->down != null );

					/* while tail */
					while ( value->word_head == null )
					{
						/* if we cant go out */
						if ( value->out == null )
						{
							/* we're done, we use value to break all loops */
							value = null;
							break;
						}

						/* go out */
						err_passthrough( hoof_save_word( fp, ( b * ) "out", 1 ) );

						value = value->out->down;
					}
				}
			}

			fclose( fp );
			fp = null;

			err_if( hoof_hook_rename( ( char * ) temp_filename , ( char * ) hoof->filename ) != 0, hoof_rc_error_file );

			temp_filename[ 0 ] = '\0';


			/* CLEANUP */
			cleanup:

			if ( temp_filename[ 0 ] != '\0' )
			{
				remove( ( char * ) temp_filename );
			}

			if ( fp != null )
			{
				fclose( fp );
				fp = null;
			}

			return rc;
			}
	// drawing
		static n hoof_word_length( b * word )
			{
			n length = 0 ;
			while ( word[ length ] != '\0' )
				{
				length += 1 ;
				}
			return length ;
			}
		static void hoof_draw_value( hoof_draw_function draw_function , struct hoof_value * value , n max_columns , n row , n * row_size )
			{
			// data
			n column = 0 ;
			struct hoof_word * word = NULL ;
			n word_length = 0 ;
			// code
			( * row_size ) = 1 ;
			word = value -> word_head -> right ;
			// draw bullet
			if ( draw_function != NULL )
				{
				draw_function( 0 , row , ( b * ) "* " ) ;
				}
			// foreach word
			while ( word -> value != NULL )
				{
				word_length = hoof_word_length( word -> value ) ;
				// do we have enough room on the current line ?
				if ( column != 2 && ( column + word_length ) >= max_columns )
					{
					column = 2 ;
					row += 1 ;
					( * row_size ) += 1 ;
					}
				// draw word
				if ( draw_function != NULL )
					{
					draw_function( column , row , word -> value ) ;
					}
				// update column
				column += word_length ;
				column += 1 ;
				// next word
				word = word -> right ;
				}
			}
		static void hoof_draw( struct hoof * hoof )
			{
			// data
			hoof_draw_function draw_function = NULL ;
			n row = 0 ;
			n row_size = 0 ;
			struct hoof_value * value = NULL ;
			// code
			// draw current value
			value = hoof -> current_value ;
			row = ( hoof -> max_rows ) / 2 ;
			hoof_draw_value( draw_function , value , hoof -> max_columns , row , & row_size ) ;
			// draw down values until we run out of space or run out of values
			while ( 1 )
				{
				row += row_size ;
				value = value -> down ;
				if ( row >= ( hoof -> max_rows ) || value -> word_head -> value == NULL )
					{
					break ;
					}
				hoof_draw_value( draw_function , value , hoof -> max_columns , row , & row_size ) ;
				}
			// draw up values until we run out of space or run out of values
			value = hoof -> current_value ;
			row = ( hoof -> max_rows ) / 2 ;
			while ( 1 )
				{
				value = value -> up ;
				if ( value -> word_head -> value == NULL )
					{
					break ;
					}
				hoof_draw_value( NULL , value , hoof -> max_columns , row , & row_size ) ;
				row -= row_size ;
				hoof_draw_value( draw_function , value , hoof -> max_columns , row , & row_size ) ;
				if ( row < 0 )
					{
					break ;
					}
				}
			}
	static void hoof_output( const b * what_to_output, struct hoof_interface *interface )
		{
		n i = 0;
		b *output_word = null;
		while ( i <= hoof_max_value_length && interface->output_value[ i ][ 0 ] != '\0' )
		{
			i += 1;
		}
		paranoid_err_if( i == hoof_max_value_length && interface->output_value[ hoof_max_value_length ][ 0 ] != '\0' );

		output_word = interface->output_value[ i ];
		output_word[ 0 ] = '\0';
		i = 0;
		while ( what_to_output[ i ] != '\0' )
		{
			output_word[ i ] = what_to_output[ i ];
			output_word[ i + 1 ] = '\0';
			i += 1;
		}

		return;
		}
	static n hoof_word_verify( b *word )
		{
		/* DATA */
		n rc = hoof_rc_success;

		n i = 0;


		/* CODE */
		/* if normal word that contains letters */
		if ( word[ 0 ] >= 'a' && word[ 0 ] <= 'z' )
		{
			/* make sure word doesn't contain any whitespace, is only characters, and is not too long */
			for ( i = 0; word[ i ] != '\0'; i += 1 )
			{
				err_if( ! ( word[ i ] >= 'a' && word[ i ] <= 'z' ), hoof_rc_error_word_bad );
				err_if( i >= hoof_max_word_length, hoof_rc_error_word_long );
			}
		}
		/* if word is a number */
		else if ( word[ 0 ] == '-' || ( word[ 0 ] >= '0' && word[ 0 ] <= '9' ) )
		{
			i = 0;

			/* if it starts with zero then it must be zero */
			err_if( word[ 0 ] == '0' && word[ 1 ] != '\0', hoof_rc_error_word_bad );

			/* optional negative sign at beginning */
			if ( word[ i ] == '-' )
			{
				/* next */
				paranoid_err_if( i >= hoof_max_word_length );
				i += 1;

				/* cannot be negative zero */
				err_if( ! ( word[ i ] >= '1' && word[ i ] <= '9' ), hoof_rc_error_word_bad );
			}

			/* verify it only contains numbers */
			while ( word[ i ] != '\0' )
			{
				err_if( ! ( word[ i ] >= '0' && word[ i ] <= '9' ), hoof_rc_error_word_bad )

				err_if( i >= hoof_max_word_length, hoof_rc_error_word_long );
				i += 1;
			}

			/* make sure it's not too big for a 64-bit integer */

			/* negative number */
			if ( word[ 0 ] == '-' )
			{
				/* i is now length of the word */
				err_if( i > hoof_int_min_string_length, hoof_rc_error_word_bad );

				if ( i < hoof_int_min_string_length )
				{
					goto cleanup;
				}

				i = 1; /* skip '-' at beginning of word */
				while ( word[ i ] != '\0' )
				{
					err_if( word[ i ] > hoof_int_min_string[ i ], hoof_rc_error_word_bad );

					if ( word[ i ] < hoof_int_min_string[ i ] )
					{
						break;
					}

					i += 1;
				}
			}
			/* positive number */
			else
			{
				/* i is now length of the word */
				err_if( i > hoof_int_max_string_length, hoof_rc_error_word_bad );

				if ( i < hoof_int_max_string_length )
				{
					goto cleanup;
				}

				i = 0;
				while ( word[ i ] != '\0' )
				{
					err_if( word[ i ] > hoof_int_max_string[ i ], hoof_rc_error_word_bad );

					if ( word[ i ] < hoof_int_max_string[ i ] )
					{
						break;
					}

					i += 1;
				}
			}
		}
		/* not word or number, must be empty word, which happens when the client
		   is reading a value and calling hoof_do for each output word */
		/* TODO: when we go to being able to output the entire value at once, clients wont have to repeatedly call hoof_do, and we'll never get an empty word */
		else
		{
			err_if( word[ 0 ] != '\0', hoof_rc_error_word_bad );
		}


		/* CLEANUP */
		cleanup:

		return rc;
		}
	static n hoof_strdup( b *word_in, b **word_out_A )
		{
		/* DATA */
		n rc = hoof_rc_success;

		n i = 0;


		/* CODE */
		paranoid_err_if( hoof_word_verify( word_in ) );

		while ( word_in[ i ] != '\0' )
		{
			i += 1;
		}
		/* one more for '\0' */
		i += 1;

		hoof_memory_calloc( (*word_out_A), b, i );

		i = 0;
		while ( word_in[ i ] != '\0' )
		{
			(*word_out_A)[ i ] = word_in[ i ];
			i += 1;
		}

		/* CLEANUP */
		cleanup:

		return rc;	
		}
	static void hoof_make_current_value( struct hoof *hoof, struct hoof_value *value )
		{
		/* CODE */
		hoof->current_value = value;
		hoof->current_word = hoof->current_value->word_head->right;

		return;
		}
	static void hoof_root( struct hoof *hoof )
		{
		hoof_make_current_value( hoof, hoof->root->down );

		return;
		}
	static void hoof_most_up( struct hoof *hoof )
		{
		while ( hoof->current_value->up->word_head != null )
		{
			hoof->current_value = hoof->current_value->up;
		}
		/* currentvalue is set, but this will also set currentword */
		hoof_make_current_value( hoof, hoof->current_value );

		return;
		}
	static void hoof_most_down( struct hoof *hoof )
		{
		while ( hoof->current_value->down->word_head != null )
		{
			hoof->current_value = hoof->current_value->down;
		}
		/* currentvalue is set, but this will also set currentword */
		hoof_make_current_value( hoof, hoof->current_value );

		return;
		}
	static void hoof_most_out( struct hoof *hoof )
		{
		while ( hoof->current_value->out != null )
		{
			hoof_make_current_value( hoof, hoof->current_value->out );
		}

		return;
		}
	static void hoof_most_in( struct hoof *hoof )
		{
		while ( hoof->current_value->in != null )
		{
			hoof_make_current_value( hoof, hoof->current_value->in->down );
		}

		return;
		}
	static n hoof_word_insert( struct hoof *hoof, b *value )
		{
		/* DATA */
		n rc = hoof_rc_success;

		n i = 0;

		struct hoof_word *word = null;
		struct hoof_word *new_word = null;
		b *new_value = null;


		/* CODE */
		paranoid_err_if( value[ 0 ] == '\0' );

		/* make sure value isn't too long */
		i = 0;
		word = hoof->current_value->word_head->right;
		while ( word->value != null )
		{
			i += 1;
			word = word->right;
		}

		err_if( i == hoof_max_value_length, hoof_rc_error_value_long );

		/* insert */
		hoof_memory_calloc( new_word, struct hoof_word, 1 );

		err_passthrough( hoof_strdup( value, &new_value ) );

		new_word->value = new_value;
		new_value = null;

		new_word->left = hoof->current_word->left;
		new_word->right = hoof->current_word;

		hoof->current_word->left->right = new_word;
		hoof->current_word->left = new_word;

		new_word = null;


		/* CLEANUP */
		cleanup:

		hoof_memory_free( new_word );
		hoof_memory_free( new_value );

		return rc;
		}
	static n hoof_value_insert( struct hoof_value *before )
		{
		/* DATA */
		n rc = hoof_rc_success;

		struct hoof_value *new_value = null;
		struct hoof_word *new_word_head = null;
		struct hoof_word *new_word_tail = null;


		/* CODE */
		hoof_memory_calloc( new_value, struct hoof_value, 1 );
		hoof_memory_calloc( new_word_head, struct hoof_word, 1 );
		hoof_memory_calloc( new_word_tail, struct hoof_word, 1 );

		new_word_head->right = new_word_tail;

		new_word_tail->left = new_word_head;

		new_value->word_head = new_word_head;

		new_value->up = before;
		new_value->down = before->down;
		
		before->down->up = new_value;
		before->down = new_value;

		new_value->out = before->out;

		new_word_head = null;
		new_word_tail = null;
		new_value = null;


		/* CLEANUP */
		cleanup:

		hoof_memory_free( new_value );
		hoof_memory_free( new_word_head );
		hoof_memory_free( new_word_tail );

		return rc;
		}
	static n hoof_page_init( struct hoof_value *parent, n create_empty_value, struct hoof_value **page_A )
		{
		/*!	\brief Creates a new page.
			\param[in] parent Parent of new page. Can be null, but only hoof_init()
				should pass in null when creating the root page.
			\param[in] create_empty_value Flag whether the new page should have an empty
				value. If 0, caller is responsible for providing at least 1 value in page.
			\param[out] page_A On success, New page. Can be null if caller does not need
				pointer to new page.
			\return n
			*/
		/* DATA */
		n rc = hoof_rc_success;

		struct hoof_value *new_head = null;
		struct hoof_value *new_tail = null;


		/* CODE */
		hoof_memory_calloc( new_head, struct hoof_value, 1 );
		hoof_memory_calloc( new_tail, struct hoof_value, 1 );

		new_head->up = null;
		new_head->down = new_tail;

		new_tail->up = new_head;
		new_tail->down = null;

		new_head->out = parent;
		new_tail->out = parent;

		if ( create_empty_value )
		{
			err_passthrough( hoof_value_insert( new_head ) );
		}

		if ( parent != null )
		{
			parent->in = new_head;
		}

		/* give back */
		if ( page_A != null )
		{
			(*page_A) = new_head;
		}
		new_head = null;
		new_tail = null;


		/* CLEANUP */
		cleanup:

		hoof_memory_free( new_head );
		hoof_memory_free( new_tail );

		return rc;
		}
	static void hoof_value_clear( struct hoof *hoof, struct hoof_value *value )
		{
		/* DATA */
		struct hoof_word *word = null;
		struct hoof_word *word_to_delete = null;

		/* CODE */
		word = value->word_head;
		while ( word->right->value != null )
		{
			word_to_delete = word->right;

			word->right = word->right->right;
			word->right->left = word;

			hoof_memory_free( word_to_delete->value );
			hoof_memory_free( word_to_delete );
		}

		/* we may have deleted current word, so fix if necessary */
		if ( value == hoof->current_value )
		{
			hoof_make_current_value( hoof, hoof->current_value );
		}

		return;
		}
	static void hoof_word_delete( struct hoof *hoof )
		{
		struct hoof_word *word_to_delete = null;

		if ( hoof->current_word->value != null )
		{
			hoof->current_word->left->right = hoof->current_word->right;
			hoof->current_word->right->left = hoof->current_word->left;

			word_to_delete = hoof->current_word;
			if ( hoof->current_word->right->value != null )
			{
				hoof->current_word = hoof->current_word->right;
			}
			else
			{
				hoof->current_word = hoof->current_word->left;
			}

			hoof_memory_free( word_to_delete->value );
			hoof_memory_free( word_to_delete );
		}

		return;
		}
	static void hoof_value_delete( struct hoof *hoof )
		{
		/* DATA */
		struct hoof_value *value = null;
		struct hoof_value *value_to_delete = null;
		

		/* CODE */
		/* special case:
		   if value is only "most out" value, then just delete its
		   children and clear the value */
		if (    hoof->current_value->out == null
			 && hoof->current_value->up->word_head == null
			 && hoof->current_value->down->word_head == null
		   )
		{
			if ( hoof->current_value->in != null )
			{
				hoof_page_delete( hoof, &(hoof->current_value->in) );
			}
			hoof_value_clear( hoof, hoof->current_value );

			return;
		}

		/* normal case */

		value = hoof->current_value;
		if ( value->in != null )
		{
			hoof_page_delete( hoof, &(value->in) );
		}

		/* remember value to delete */
		value_to_delete = value;

		/* go down */
		value = value->down;

		/* remove value from list */
		value->up = value_to_delete->up;
		value_to_delete->up->down = value;

		/* free value_to_delete */
		hoof_value_clear( hoof, value_to_delete );        /* words */
		hoof_memory_free( value_to_delete->word_head->right ); /* word tail */
		hoof_memory_free( value_to_delete->word_head );        /* word head */
		hoof_memory_free( value_to_delete );                  /* value */

		/* if value is tail */
		if ( value->word_head == null )
		{
			/* go up */
			value = value->up;

			/* if value is head */
			if ( value->word_head == null )
			{
				/* go out */
				value = value->out;

				/* delete empty in */
				hoof_memory_free( value->in->down ); /* value tail */
				hoof_memory_free( value->in );       /* value head */
				value->in = null;
			}
		}

		hoof_make_current_value( hoof, value );

		return;
		}
	static void hoof_page_delete( struct hoof *hoof, struct hoof_value **page_F )
		{
		/*!	\brief Deletes page and recursively all subpages.
			\param[in] hoof struct hoof context.
			\param[in] page_F Page to free.
			\return void

			NOTE: this assumes pages will always contain at least one value.
			*/
		/* DATA */
		struct hoof_value *value = null;
		struct hoof_value *value_to_delete = null;


		/* CODE */
		if ( (*page_F) == null)
		{
			return;
		}

		value = (*page_F)->down;

		while ( 1 )
		{
			/* go most in */
			while ( value->in != null )
			{
				paranoid_err_if( value->in->out != value );
				value = value->in->down;
				paranoid_err_if( value->word_head == null );
			}

			/* remember value we need to delete */
			value_to_delete = value;

			/* goto next value */
			value = value->down;

			/* remove value from list */
			value->up = value_to_delete->up;
			value_to_delete->up->down = value;

			/* free value_to_delete */
			hoof_value_clear( hoof, value_to_delete );
			hoof_memory_free( value_to_delete->word_head->right );
			hoof_memory_free( value_to_delete->word_head );
			hoof_memory_free( value_to_delete );

			/* if value is tail */
			if ( value->word_head == null )
			{
				/* if we're at tail of our original page */
				if ( value->up == (*page_F) )
				{
					break;
				}

				/* remember value we need to delete */
				value_to_delete = value;

				/* go out */
				value = value->out;

				/* set in to null */
				value->in = null;

				/* free head and tail */
				hoof_memory_free( value_to_delete->up );
				hoof_memory_free( value_to_delete );
			}
		}

		/* free head and tail */
		hoof_memory_free( value->up ); /* this is equivalent to (*page_F) */
		hoof_memory_free( value );

		(*page_F) = null;

		return;
		}
	static void hoof_dig( struct hoof *hoof, b *word )
		{
		/*!	\brief Update current_value to be the next matching value.
			\param[in] hoof struct hoof context.
			\param[in] word Next word in dig query.
			\return void

			NOTE: This function may leave current_value as tail value and may leave
			current_word as undefined. Caller is responsible for updating current_value
			and current_word after using this function.
			*/
		/* DATA */
		struct hoof_value *temp_value = null;
		struct hoof_word *temp_word1 = null;
		struct hoof_word *temp_word2 = null;
		n match = 0;


		/* CODE */
		/* if tail */
		if ( hoof->current_value->word_head == null )
		{
			goto cleanup;
		}

		/* if next word in current_value matches */
		if ( word == null )
		{
			if ( hoof->current_word->right->value == null )
			{
				/* update current_word */
				hoof->current_word = hoof->current_word->right;
				goto cleanup;
			}
		}
		else
		{
			if (    hoof->current_word->right->value != null
				 && hoof_words_are_same( word, hoof->current_word->right->value )
			   )
			{
				/* update current_word */
				hoof->current_word = hoof->current_word->right;
				goto cleanup;
			}
		}

		/* we need to find the next value that matches */
		temp_value = hoof->current_value;
		while ( 1 )
		{
			/* go down */
			temp_value = temp_value->down;

			/* if tail */
			if ( temp_value->word_head == null )
			{
				hoof->current_value = temp_value;
				goto cleanup;
			}

			/* do temp_value and current_value match up until current_word? */
			match = 1;

			temp_word1 = hoof->current_value->word_head;
			temp_word2 = temp_value->word_head;

			while ( temp_word1 != hoof->current_word )
			{
				temp_word1 = temp_word1->right;
				temp_word2 = temp_word2->right;

				if ( temp_word2->value == null )
				{
					match = 0;
					break;
				}

				if ( ! hoof_words_are_same( temp_word1->value, temp_word2->value ) )
				{
					match = 0;
					break;
				}
			}

			if ( match )
			{
				if ( word == null )
				{
					if ( temp_word2->right->value == null )
					{
						hoof->current_value = temp_value;
						hoof->current_word = temp_word2->right;
						goto cleanup;
					}
				}
				else
				{
					/* if next word in this value matches */
					if (    temp_word2->right->value != null
						 && hoof_words_are_same( temp_word2->right->value, word )
					   )
					{
						hoof->current_value = temp_value;
						hoof->current_word = temp_word2->right;
						goto cleanup;
					}
				}
			}
		}


		/* CLEANUP */
		cleanup:

		return;
		}
	n hoof_init( b *filename, struct hoof **hoof_A )
		{
		/* DATA */
		n rc = hoof_rc_success;

		struct hoof *new_hoof = null;
		struct hoof_value *new_page = null;
		b *new_filename = null;


		/* CODE */
		paranoid_err_if( filename == null );
		paranoid_err_if( hoof_A == null );
		paranoid_err_if( (*hoof_A) != null );

		err_passthrough( hoof_word_verify( filename ) );

		hoof_memory_calloc( new_hoof, struct hoof, 1 );

		err_passthrough( hoof_strdup( filename, &new_filename ) );

		err_passthrough( hoof_page_init( null, 1, &new_page ) );

		new_hoof->filename = new_filename;
		new_filename = null;

		new_hoof->root = new_page;
		new_page = null;

		hoof_make_current_value( new_hoof, new_hoof->root->down );

		/* load file */
		err_passthrough( hoof_load( new_hoof ) );

		/* give back */
		(*hoof_A) = new_hoof;
		new_hoof = null;


		/* CLEANUP */
		cleanup:

		/* TODO: call hoof_free? */
		if ( new_hoof != null )
		{
			hoof_memory_free( new_hoof->filename );
			hoof_page_delete( new_hoof, &(new_hoof->root) );
			hoof_memory_free( new_hoof );
		}
		hoof_memory_free( new_filename );

		return rc;
		}
	void hoof_free( struct hoof **hoof_F )
		{
		/* CODE */
		if ( hoof_F == null || (*hoof_F) == null )
		{
			goto cleanup;
		}

		hoof_memory_free( (*hoof_F)->filename );

		hoof_page_delete( (*hoof_F), &((*hoof_F)->root) );

		hoof_memory_free( (*hoof_F) );


		/* CLEANUP */
		cleanup:

		return;
		}
	n hoof_set_draw_function( struct hoof * hoof , n max_columns , n max_rows , hoof_draw_function draw_function )
		{
		hoof -> max_columns = max_columns ;
		hoof -> max_rows = max_rows ;
		hoof -> draw_function = draw_function ;
		return hoof_rc_success ;
		}
	n hoof_do( struct hoof *hoof, struct hoof_interface *interface )
		{
		/* DATA */
		n rc = hoof_rc_success;

		n i = 0;
		n huh = 0;


		/* CODE */
		paranoid_err_if( hoof == null );
		paranoid_err_if( interface == null );

		while ( i <= hoof_max_value_length )
		{
			interface->output_value[ i ][ 0 ] = '\0';
			i += 1;
		}

		err_passthrough( hoof_word_verify( interface->input_word ) );

		/* handle pause and resume */
		if ( hoof->paused )
		{
			if ( hear( "resume" ) )
			{
				hoof->paused = 0;
				say( "resumed" );
				goto cleanup;
			}

			goto cleanup;
		}

		if (    hoof->literal == 0
			 && hear( "pause" )
		   )
		{
			hoof->paused = 1;
			say( "paused" );
			goto cleanup;
		}

		rc = hoof->state( hoof, interface, &huh );

		if ( interface->input_word[ 0 ] != '\0' && huh == 1 )
		{
			say( "huh" );
			goto cleanup;
		}

		interface->input_word[ 0 ] = '\0';

		// TODO
		(void)hoof_draw;

		/* CLEANUP */
		cleanup:

		return rc;
		}
	const b *hoof_rc_to_string( n rc )
		{
		/* TODO: this function doesnt return success/fail or void, think about this */
		static const b *rc_strings[] =
		{
			( b * ) "Success",

			( b * ) "Error Precondition",
			( b * ) "Error Memory",
			( b * ) "Error File",
			( b * ) "Error Network",
			( b * ) "Error Standard Library",
			( b * ) "Error Failure Point",

			( b * ) "Quit",
			( b * ) "Error Filename Contains Bad Characters",
			( b * ) "Error Filename Too Long",
			( b * ) "Error File Contains Bad Content",
			( b * ) "Error Word Contains Bad Characters",
			( b * ) "Error Word Too Long",
			( b * ) "Error Value Too Long"
		};

		static const b *rc_unknown = ( b * ) "Unknown Error";

		/* standard errors */
		if ( rc >= 0 && rc <= hoof_rc_standard_errors_max )
		{
			return rc_strings[ rc ];
		}
		/* hoof errors */
		else if ( rc >= hoof_rc_hoof_errors_min && rc <= hoof_rc_hoof_errors_max )
		{
			return rc_strings[ hoof_rc_standard_errors_max + rc - hoof_rc_hoof_errors_min + 1 ];
		}

		return rc_unknown;
		}
