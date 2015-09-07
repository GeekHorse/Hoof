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
#undef HOOF_FILE_NUMBER
#define HOOF_FILE_NUMBER 3

/******************************************************************************/
#include "hoof.h"
#include "hoofInternal.h"

/******************************************************************************/
/*!
	\brief Helper function during save to save a word.
	\param[in] fp File pointer to write to.
	\param[in] word Word to write.
	\param[in] newvalue If true a newvalue will be written after word, else a
		space.
	\return HOOF_RC
*/
HOOF_INTERNAL HOOF_RC hoofSaveWord( FILE *fp, char *word, int newvalue )
{
	/* DATA */
	HOOF_RC rc = HOOF_RC_SUCCESS;

	unsigned int i = 0;

	char chSpace = ' ';
	char chNewvalue = '\n';


	/* CODE */
	while ( word[ i ] != '\0' )
	{
		i += 1;
	}

	ERR_IF( HOOF_HOOK_FWRITE( word, 1, i, fp ) != i, HOOF_RC_ERROR_FILE );

	if ( newvalue )
	{
		ERR_IF( HOOF_HOOK_FWRITE( &chNewvalue, 1, 1, fp ) != 1, HOOF_RC_ERROR_FILE );
	}
	else
	{
		ERR_IF( HOOF_HOOK_FWRITE( &chSpace, 1, 1, fp ) != 1, HOOF_RC_ERROR_FILE );
	}


	/* CLEANUP */
	cleanup:

	return rc;
}

/******************************************************************************/
/*!
	\brief Helper function during save to save a value.
	\param[in] fp File pointer to use.
	\param[in] value Value to save.
	\param[in] direction The insert direction of this value. Will be right, down,
		or in.
	\return HOOF_RC

	This function contains many PARANOID_ERR_IF checks to verify our hoof
	structure is still correct and consistent during tests.
*/
HOOF_INTERNAL HOOF_RC hoofSaveValue( FILE *fp, HoofValue *value, char *direction )
{
	/* DATA */
	HOOF_RC rc = HOOF_RC_SUCCESS;

	HoofWord *word = NULL;


	/* CODE */
	PARANOID_ERR_IF( value == NULL );
	PARANOID_ERR_IF( value->wordHead == NULL );
	PARANOID_ERR_IF( value->wordHead->value != NULL );
	PARANOID_ERR_IF( value->wordHead->right == NULL );
	PARANOID_ERR_IF( value->wordHead->right->left != value->wordHead );
	PARANOID_ERR_IF( value->wordHead->left != NULL );

	word = value->wordHead->right;

	ERR_PASSTHROUGH( hoofSaveWord( fp, "new", 0 ) );
	ERR_PASSTHROUGH( hoofSaveWord( fp, direction, 0 ) );

	PARANOID_ERR_IF( word == NULL );

	while ( word->value != NULL )
	{
		PARANOID_ERR_IF( word->left->right != word );
		PARANOID_ERR_IF( word->right->left != word );

		if (    strcmp( word->value, "done" ) == 0
		     || strcmp( word->value, "pause" ) == 0
		     || strcmp( word->value, "literal" ) == 0
		   )
		{
			ERR_PASSTHROUGH( hoofSaveWord( fp, "literal", 0 ) );
		}

		ERR_PASSTHROUGH( hoofSaveWord( fp, word->value, 0 ) );

		word = word->right;
	}

	PARANOID_ERR_IF( word->left->right != word );
	PARANOID_ERR_IF( word->right != NULL );

	ERR_PASSTHROUGH( hoofSaveWord( fp, "done", 1 ) );


	/* CLEANUP */
	cleanup:

	return rc;
}

/******************************************************************************/
/*!
	\brief Saves the hoof data back to the file it was loaded from.
	\param[in] hoof Hoof context.
	\return HOOF_RC

	This function contains many PARANOID_ERR_IF checks to verify our hoof
	structure is still correct and consistent during tests.
*/
HOOF_INTERNAL HOOF_RC hoofSave( Hoof *hoof )
{
	/* DATA */
	HOOF_RC rc = HOOF_RC_SUCCESS;

	int i = 0;

	/* +1 for NULL +1 for dot at beginning */
	char tempFilename[ HOOF_MAX_WORD_LENGTH + 2 ] = "";

	FILE *fp = NULL;

	HoofValue *value = NULL;

	int first = 1;


	/* CODE */
	/* create temp filename */
	tempFilename[ 0 ] = '.';
	while ( hoof->filename[ i ] != '\0' )
	{
		tempFilename[ i + 1 ] = hoof->filename[ i ];
		i += 1;
	}
	tempFilename[ i + 1 ] = '\0';

	/* open temp filename */
	fp = HOOF_HOOK_FOPEN( tempFilename, "w+" );
	ERR_IF( fp == NULL, HOOF_RC_ERROR_FILE );

	/* write file */
	value = hoof->root->down;

	PARANOID_ERR_IF( value == NULL );
	PARANOID_ERR_IF( value->wordHead == NULL );
	PARANOID_ERR_IF( value->up->wordHead != NULL );

	while ( value != NULL )
	{
		PARANOID_ERR_IF( value->up->down != value );
		PARANOID_ERR_IF( value->down->up != value );
		PARANOID_ERR_IF( value->out != value->up->out );
		PARANOID_ERR_IF( value->out != value->down->out );

		if ( first )
		{
			ERR_PASSTHROUGH( hoofSaveValue( fp, value, "right" ) );
			first = 0;
		}
		else
		{
			ERR_PASSTHROUGH( hoofSaveValue( fp, value, "down" ) );
		}

		/* go most in */
		while ( value->in != NULL )
		{
			PARANOID_ERR_IF( value->in->out != value );
			PARANOID_ERR_IF( value->in->wordHead != NULL );
			PARANOID_ERR_IF( value->in->down == NULL );

			value = value->in->down;

			PARANOID_ERR_IF( value->up->down != value );
			PARANOID_ERR_IF( value->down->up != value );
			PARANOID_ERR_IF( value->out != value->up->out );
			PARANOID_ERR_IF( value->out != value->down->out );

			ERR_PASSTHROUGH( hoofSaveValue( fp, value, "in" ) );
		}

		/* go down */
		value = value->down;

		PARANOID_ERR_IF( value->up->down != value );
		PARANOID_ERR_IF( value->down != NULL && value->down->up != value );
		PARANOID_ERR_IF( value->out != value->up->out );
		PARANOID_ERR_IF( value->down != NULL && value->out != value->down->out );

		/* if tail */
		if ( value->wordHead == NULL )
		{
			PARANOID_ERR_IF( value->down != NULL );

			/* while tail */
			while ( value->wordHead == NULL )
			{
				/* if we cant go out */
				if ( value->out == NULL )
				{
					/* we're done, we use value to break all loops */
					value = NULL;
					break;
				}

				/* go out */
				ERR_PASSTHROUGH( hoofSaveWord( fp, "out", 1 ) );

				value = value->out->down;
			}
		}
	}

	fclose( fp );
	fp = NULL;

	ERR_IF( HOOF_HOOK_RENAME( tempFilename, hoof->filename ) != 0, HOOF_RC_ERROR_FILE );

	tempFilename[ 0 ] = '\0';


	/* CLEANUP */
	cleanup:

	if ( tempFilename[ 0 ] != '\0' )
	{
		remove( tempFilename );
	}

	if ( fp != NULL )
	{
		fclose( fp );
		fp = NULL;
	}

	return rc;
}

/******************************************************************************/
/*!
	\brief Loads hoof data from file. Called in hoofInit().
	\param[in] hoof Hoof context.
	\return HOOF_RC
*/
HOOF_INTERNAL HOOF_RC hoofLoad( Hoof *hoof )
{
	/* DATA */
	HOOF_RC rc = HOOF_RC_SUCCESS;

	FILE *fp = NULL;

	HoofInterface interface;

	int done = 0;

	int i = 0;
	char ch = 0;


	/* CODE */
	hoof->loading = 1;
	hoof->state = hoofStateNavigate;;

	fp = HOOF_HOOK_FOPEN( hoof->filename, "r" );

	ERR_IF( fp == NULL, HOOF_RC_ERROR_FILE );

	while ( 1 )
	{
		/* get next word */
		i = 0;
		interface.inputWord[ 0 ] = '\0';

		while ( 1 )
		{
			if ( HOOF_HOOK_FREAD( &ch, 1, 1, fp ) != 1 )
			{
				ERR_IF( ! feof( fp ), HOOF_RC_ERROR_FILE );

				done = 1;
				break;
			}

			if ( ch == ' ' || ch == '\n' || ch == '\r' )
			{
				break;
			}

			ERR_IF( i >= HOOF_MAX_WORD_LENGTH, HOOF_RC_ERROR_WORD_LONG );

			interface.inputWord[ i ] = ch;
			interface.inputWord[ i + 1 ] = '\0';

			i += 1;
		}

		if ( done )
		{
			break;
		}

		/* feed into hoofDo */
		ERR_PASSTHROUGH( hoofDo( hoof, &interface ) );
		/* TODO: when we have a better loading file format, this can go away */
		ERR_IF( interface.outputValue[ 1 ][ 0 ] != '\0', HOOF_RC_ERROR_FILE_BAD );
	}

	hoofRoot( hoof );

	hoof->loading = 0;
	hoof->state = hoofStateHello;


	/* CLEANUP */
	cleanup:

	if ( fp != NULL )
	{
		fclose( fp );
		fp = NULL;
	}

	return rc;
}

