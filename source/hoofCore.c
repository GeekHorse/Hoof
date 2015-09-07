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
#define HOOF_FILE_NUMBER 1

/******************************************************************************/
#include "hoof.h"
#include "hoofInternal.h"

/******************************************************************************/
#define HOOF_INT_MAX                 9223372036854775807
#define HOOF_INT_MAX_STRING         "9223372036854775807"
#define HOOF_INT_MAX_STRING_LENGTH 19
#define HOOF_INT_MIN_STRING        "-9223372036854775808"
#define HOOF_INT_MIN_STRING_LENGTH 20

/******************************************************************************/
/*!
	\brief Transfer a word to the output word in hoof interface. This is what
		SAY() uses.
	\param[in] whatToOutput The word to output.
	\param[out] outputWord The destination location.
	\return void
*/
HOOF_INTERNAL void hoofOutput( const char *whatToOutput, char *outputWord )
{
	int i = 0;
	outputWord[ 0 ] = '\0';
	while ( whatToOutput[ i ] != '\0' )
	{
		outputWord[ i ] = whatToOutput[ i ];
		outputWord[ i + 1 ] = '\0';
		i += 1;
	}

	return;
}

/******************************************************************************/
/*!
	\brief Verifies a word is good.
	\param[in] word Word to verify
	\return HOOF_RC

	Currently a word is good if it contains only lower case letters a through z
	and its length is <= HOOF_MAX_WORD_LENGTH.

	In the future, number words will be good too, like -59.201
*/
HOOF_INTERNAL HOOF_RC hoofWordVerify( char *word )
{
	/* DATA */
	HOOF_RC rc = HOOF_RC_SUCCESS;

	int i = 0;


	/* CODE */
	/* if normal word that contains letters */
	if ( word[ 0 ] >= 'a' && word[ 0 ] <= 'z' )
	{
		/* make sure word doesn't contain any whitespace, is only characters, and is not too long */
		for ( i = 0; word[ i ] != '\0'; i += 1 )
		{
			ERR_IF( ! ( word[ i ] >= 'a' && word[ i ] <= 'z' ), HOOF_RC_ERROR_WORD_BAD );
			ERR_IF( i >= HOOF_MAX_WORD_LENGTH, HOOF_RC_ERROR_WORD_LONG );
		}
	}
	/* if word is a number */
	else if ( word[ 0 ] == '-' || ( word[ 0 ] >= '0' && word[ 0 ] <= '9' ) )
	{
		i = 0;

		/* if it starts with zero then it must be zero */
		ERR_IF( word[ 0 ] == '0' && word[ 1 ] != '\0', HOOF_RC_ERROR_WORD_BAD );

		/* optional negative sign at beginning */
		if ( word[ i ] == '-' )
		{
			/* next */
			PARANOID_ERR_IF( i >= HOOF_MAX_WORD_LENGTH );
			i += 1;

			/* cannot be negative zero */
			ERR_IF( ! ( word[ i ] >= '1' && word[ i ] <= '9' ), HOOF_RC_ERROR_WORD_BAD );
		}

		/* verify it only contains numbers */
		while ( word[ i ] != '\0' )
		{
			ERR_IF( ! ( word[ i ] >= '0' && word[ i ] <= '9' ), HOOF_RC_ERROR_WORD_BAD )

			ERR_IF( i >= HOOF_MAX_WORD_LENGTH, HOOF_RC_ERROR_WORD_LONG );
			i += 1;
		}

		/* make sure it's not too big for a 64-bit integer */

		/* negative number */
		if ( word[ 0 ] == '-' )
		{
			/* i is now length of the word */
			ERR_IF( i > HOOF_INT_MIN_STRING_LENGTH, HOOF_RC_ERROR_WORD_BAD );

			if ( i < HOOF_INT_MIN_STRING_LENGTH )
			{
				goto cleanup;
			}

			i = 1; /* skip '-' at beginning of word */
			while ( word[ i ] != '\0' )
			{
				ERR_IF( word[ i ] > HOOF_INT_MIN_STRING[ i ], HOOF_RC_ERROR_WORD_BAD );

				if ( word[ i ] < HOOF_INT_MIN_STRING[ i ] )
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
			ERR_IF( i > HOOF_INT_MAX_STRING_LENGTH, HOOF_RC_ERROR_WORD_BAD );

			if ( i < HOOF_INT_MAX_STRING_LENGTH )
			{
				goto cleanup;
			}

			i = 0;
			while ( word[ i ] != '\0' )
			{
				ERR_IF( word[ i ] > HOOF_INT_MAX_STRING[ i ], HOOF_RC_ERROR_WORD_BAD );

				if ( word[ i ] < HOOF_INT_MAX_STRING[ i ] )
				{
					break;
				}

				i += 1;
			}
		}
	}
	/* not word or number, must be empty word, which happens when the client
	   is reading a value and calling hoofDo for each output word */
	/* TODO: when we go to being able to output the entire value at once, clients wont have to repeatedly call hoofDo, and we'll never get an empty word */
	else
	{
		ERR_IF( word[ 0 ] != '\0', HOOF_RC_ERROR_WORD_BAD );
	}


	/* CLEANUP */
	cleanup:

	return rc;
}

/******************************************************************************/
/*!
	\brief Helper function to duplicate a word.
	\param[in] wordIn Word to duplicate.
	\param[out] wordOut_A On success, new duplicated word. Caller is responsible
		to free with free() or hoofHookFree().
	\return HOOF_RC

	This is necessary because hoof can use other memory allocation functions
	than malloc. This function uses the memory hook functions if necessary.
*/
HOOF_INTERNAL HOOF_RC hoofStrdup( char *wordIn, char **wordOut_A )
{
	/* DATA */
	HOOF_RC rc = HOOF_RC_SUCCESS;

	int i = 0;


	/* CODE */
	PARANOID_ERR_IF( hoofWordVerify( wordIn ) );

	while ( wordIn[ i ] != '\0' )
	{
		i += 1;
	}
	/* one more for '\0' */
	i += 1;

	HOOF_CALLOC( (*wordOut_A), char, i );

	i = 0;
	while ( wordIn[ i ] != '\0' )
	{
		(*wordOut_A)[ i ] = wordIn[ i ];
		i += 1;
	}

	/* CLEANUP */
	cleanup:

	return rc;	
}

/******************************************************************************/
/*!
	\brief Changes current value and current word.
	\param[in] hoof Hoof context.
	\param[in] value Value to make current.
	\return void
*/
HOOF_INTERNAL void hoofMakeCurrentValue( Hoof *hoof, HoofValue *value )
{
	/* CODE */
	hoof->currentValue = value;
	hoof->currentWord = hoof->currentValue->wordHead->right;

	return;
}

/******************************************************************************/
/*!
	\brief Changes current value to root value.
	\param[in] hoof Hoof context.
	\return void
*/
HOOF_INTERNAL void hoofRoot( Hoof *hoof )
{
	hoofMakeCurrentValue( hoof, hoof->root->down );

	return;
}

/******************************************************************************/
/*!
	\brief Changes current value to most up value.
	\param[in] hoof Hoof context.
	\return void
*/
HOOF_INTERNAL void hoofMostUp( Hoof *hoof )
{
	while ( hoof->currentValue->up->wordHead != NULL )
	{
		hoof->currentValue = hoof->currentValue->up;
	}
	/* currentvalue is set, but this will also set currentword */
	hoofMakeCurrentValue( hoof, hoof->currentValue );

	return;
}

/******************************************************************************/
/*!
	\brief Changes current value to most down value.
	\param[in] hoof Hoof context.
	\return void
*/
HOOF_INTERNAL void hoofMostDown( Hoof *hoof )
{
	while ( hoof->currentValue->down->wordHead != NULL )
	{
		hoof->currentValue = hoof->currentValue->down;
	}
	/* currentvalue is set, but this will also set currentword */
	hoofMakeCurrentValue( hoof, hoof->currentValue );

	return;
}

/******************************************************************************/
/*!
	\brief Changes current value to most out value.
	\param[in] hoof Hoof context.
	\return void
*/
HOOF_INTERNAL void hoofMostOut( Hoof *hoof )
{
	while ( hoof->currentValue->out != NULL )
	{
		hoofMakeCurrentValue( hoof, hoof->currentValue->out );
	}

	return;
}

/******************************************************************************/
/*!
	\brief Changes current value to most in value.
	\param[in] hoof Hoof context.
	\return void
*/
HOOF_INTERNAL void hoofMostIn( Hoof *hoof )
{
	while ( hoof->currentValue->in != NULL )
	{
		hoofMakeCurrentValue( hoof, hoof->currentValue->in->down );
	}

	return;
}

/******************************************************************************/
/*!
	\brief Inserts a new word to the left of the current word.
	\param[in] hoof Hoof context
	\param[in] value Value of new word.
	\return HOOF_RC
*/
HOOF_INTERNAL HOOF_RC hoofWordInsert( Hoof *hoof, char *value )
{
	/* DATA */
	HOOF_RC rc = HOOF_RC_SUCCESS;

	HoofWord *newWord = NULL;
	char *newValue = NULL;


	/* CODE */
	PARANOID_ERR_IF( value[ 0 ] == '\0' );

	HOOF_CALLOC( newWord, HoofWord, 1 );

	ERR_PASSTHROUGH( hoofStrdup( value, &newValue ) );

	newWord->value = newValue;
	newValue = NULL;

	newWord->left = hoof->currentWord->left;
	newWord->right = hoof->currentWord;

	hoof->currentWord->left->right = newWord;
	hoof->currentWord->left = newWord;

	newWord = NULL;


	/* CLEANUP */
	cleanup:

	HOOF_FREE( newWord );
	HOOF_FREE( newValue );

	return rc;
}

/******************************************************************************/
/*!
	\brief Inserts a new value after the passed in value. Does not affect current
		value.
	\param[in] before Value to insert after.
	\return HOOF_RC
*/
HOOF_INTERNAL HOOF_RC hoofValueInsert( HoofValue *before )
{
	/* DATA */
	HOOF_RC rc = HOOF_RC_SUCCESS;

	HoofValue *newValue = NULL;
	HoofWord *newWordHead = NULL;
	HoofWord *newWordTail = NULL;


	/* CODE */
	HOOF_CALLOC( newValue, HoofValue, 1 );
	HOOF_CALLOC( newWordHead, HoofWord, 1 );
	HOOF_CALLOC( newWordTail, HoofWord, 1 );

	newWordHead->right = newWordTail;

	newWordTail->left = newWordHead;

	newValue->wordHead = newWordHead;

	newValue->up = before;
	newValue->down = before->down;
	
	before->down->up = newValue;
	before->down = newValue;

	newValue->out = before->out;

	newWordHead = NULL;
	newWordTail = NULL;
	newValue = NULL;


	/* CLEANUP */
	cleanup:

	HOOF_FREE( newValue );
	HOOF_FREE( newWordHead );
	HOOF_FREE( newWordTail );

	return rc;
}

/******************************************************************************/
/*!
	\brief Creates a new page.
	\param[in] parent Parent of new page. Can be NULL, but only hoofInit()
		should pass in NULL when creating the root page.
	\param[in] createEmptyValue Flag whether the new page should have an empty
		value. If 0, caller is responsible for providing at least 1 value in page.
	\param[out] page_A On success, New page. Can be NULL if caller does not need
		pointer to new page.
	\return HOOF_RC
*/
HOOF_INTERNAL HOOF_RC hoofPageInit( HoofValue *parent, int createEmptyValue, HoofValue **page_A )
{
	/* DATA */
	HOOF_RC rc = HOOF_RC_SUCCESS;

	HoofValue *newHead = NULL;
	HoofValue *newTail = NULL;


	/* CODE */
	HOOF_CALLOC( newHead, HoofValue, 1 );
	HOOF_CALLOC( newTail, HoofValue, 1 );

	newHead->up = NULL;
	newHead->down = newTail;

	newTail->up = newHead;
	newTail->down = NULL;

	newHead->out = parent;
	newTail->out = parent;

	if ( createEmptyValue )
	{
		ERR_PASSTHROUGH( hoofValueInsert( newHead ) );
	}

	if ( parent != NULL )
	{
		parent->in = newHead;
	}

	/* give back */
	if ( page_A != NULL )
	{
		(*page_A) = newHead;
	}
	newHead = NULL;
	newTail = NULL;


	/* CLEANUP */
	cleanup:

	HOOF_FREE( newHead );
	HOOF_FREE( newTail );

	return rc;
}

/******************************************************************************/
/*!
	\brief Removes all words from a value.
	\param[in] hoof Hoof context.
	\param[in] value Value to remove all words from.
	\return void
*/
HOOF_INTERNAL void hoofValueClear( Hoof *hoof, HoofValue *value )
{
	/* DATA */
	HoofWord *word = NULL;
	HoofWord *wordToDelete = NULL;

	/* CODE */
	word = value->wordHead;
	while ( word->right->value != NULL )
	{
		wordToDelete = word->right;

		word->right = word->right->right;
		word->right->left = word;

		HOOF_FREE( wordToDelete->value );
		HOOF_FREE( wordToDelete );
	}

	/* we may have deleted current word, so fix if necessary */
	if ( value == hoof->currentValue )
	{
		hoofMakeCurrentValue( hoof, hoof->currentValue );
	}

	return;
}

/******************************************************************************/
/*!
	\brief Deletes current word.
	\param[in] hoof Hoof context.
	\return void

	New current word will be the right word, else left word.
*/
HOOF_INTERNAL void hoofWordDelete( Hoof *hoof )
{
	HoofWord *wordToDelete = NULL;

	if ( hoof->currentWord->value != NULL )
	{
		hoof->currentWord->left->right = hoof->currentWord->right;
		hoof->currentWord->right->left = hoof->currentWord->left;

		wordToDelete = hoof->currentWord;
		if ( hoof->currentWord->right->value != NULL )
		{
			hoof->currentWord = hoof->currentWord->right;
		}
		else
		{
			hoof->currentWord = hoof->currentWord->left;
		}

		HOOF_FREE( wordToDelete->value );
		HOOF_FREE( wordToDelete );
	}

	return;
}

/******************************************************************************/
/*!
	\brief Deletes current value.
	\param[in] hoof Hoof context.
	\return void

	If current value is the only most out value, then this will just delete its
	children and clear the value.

	New current value will be down, else up, else out.
*/
HOOF_INTERNAL void hoofValueDelete( Hoof *hoof )
{
	/* DATA */
	HoofValue *value = NULL;
	HoofValue *valueToDelete = NULL;
	

	/* CODE */
	/* special case:
	   if value is only "most out" value, then just delete its
	   children and clear the value */
	if (    hoof->currentValue->out == NULL
	     && hoof->currentValue->up->wordHead == NULL
	     && hoof->currentValue->down->wordHead == NULL
	   )
	{
		if ( hoof->currentValue->in != NULL )
		{
			hoofPageDelete( hoof, &(hoof->currentValue->in) );
		}
		hoofValueClear( hoof, hoof->currentValue );

		return;
	}

	/* normal case */

	value = hoof->currentValue;
	if ( value->in != NULL )
	{
		hoofPageDelete( hoof, &(value->in) );
	}

	/* remember value to delete */
	valueToDelete = value;

	/* go down */
	value = value->down;

	/* remove value from list */
	value->up = valueToDelete->up;
	valueToDelete->up->down = value;

	/* free valueToDelete */
	hoofValueClear( hoof, valueToDelete );        /* words */
	HOOF_FREE( valueToDelete->wordHead->right ); /* word tail */
	HOOF_FREE( valueToDelete->wordHead );        /* word head */
	HOOF_FREE( valueToDelete );                  /* value */

	/* if value is tail */
	if ( value->wordHead == NULL )
	{
		/* go up */
		value = value->up;

		/* if value is head */
		if ( value->wordHead == NULL )
		{
			/* go out */
			value = value->out;

			/* delete empty in */
			HOOF_FREE( value->in->down ); /* value tail */
			HOOF_FREE( value->in );       /* value head */
			value->in = NULL;
		}
	}

	hoofMakeCurrentValue( hoof, value );

	return;
}

/******************************************************************************/
/*!
	\brief Deletes page and recursively all subpages.
	\param[in] hoof Hoof context.
	\param[in] page_F Page to free.
	\return void

	NOTE: this assumes pages will always contain at least one value.
*/
HOOF_INTERNAL void hoofPageDelete( Hoof *hoof, HoofValue **page_F )
{
	/* DATA */
	HoofValue *value = NULL;
	HoofValue *valueToDelete = NULL;


	/* CODE */
	if ( (*page_F) == NULL)
	{
		return;
	}

	value = (*page_F)->down;

	while ( 1 )
	{
		/* go most in */
		while ( value->in != NULL )
		{
			PARANOID_ERR_IF( value->in->out != value );
			value = value->in->down;
			PARANOID_ERR_IF( value->wordHead == NULL );
		}

		/* remember value we need to delete */
		valueToDelete = value;

		/* goto next value */
		value = value->down;

		/* remove value from list */
		value->up = valueToDelete->up;
		valueToDelete->up->down = value;

		/* free valueToDelete */
		hoofValueClear( hoof, valueToDelete );
		HOOF_FREE( valueToDelete->wordHead->right );
		HOOF_FREE( valueToDelete->wordHead );
		HOOF_FREE( valueToDelete );

		/* if value is tail */
		if ( value->wordHead == NULL )
		{
			/* if we're at tail of our original page */
			if ( value->up == (*page_F) )
			{
				break;
			}

			/* remember value we need to delete */
			valueToDelete = value;

			/* go out */
			value = value->out;

			/* set in to null */
			value->in = NULL;

			/* free head and tail */
			HOOF_FREE( valueToDelete->up );
			HOOF_FREE( valueToDelete );
		}
	}

	/* free head and tail */
	HOOF_FREE( value->up ); /* this is equivalent to (*page_F) */
	HOOF_FREE( value );

	(*page_F) = NULL;

	return;
}

/******************************************************************************/
/*!
	\brief Update currentValue to be the next matching value.
	\param[in] hoof Hoof context.
	\param[in] word Next word in dig query.
	\return void

	NOTE: This function may leave currentValue as tail value and may leave
	currentWord as undefined. Caller is responsible for updating currentValue
	and currentWord after using this function.
*/
HOOF_INTERNAL void hoofDig( Hoof *hoof, char *word )
{
	/* DATA */
	HoofValue *tempValue = NULL;
	HoofWord *tempWord1 = NULL;
	HoofWord *tempWord2 = NULL;
	int match = 0;


	/* CODE */
	/* if tail */
	if ( hoof->currentValue->wordHead == NULL )
	{
		goto cleanup;
	}

	/* if next word in currentValue matches */
	if ( word == NULL )
	{
		if ( hoof->currentWord->right->value == NULL )
		{
			/* update currentWord */
			hoof->currentWord = hoof->currentWord->right;
			goto cleanup;
		}
	}
	else
	{
		if (    hoof->currentWord->right->value != NULL
		     && strcmp( word, hoof->currentWord->right->value ) == 0
		   )
		{
			/* update currentWord */
			hoof->currentWord = hoof->currentWord->right;
			goto cleanup;
		}
	}

	/* we need to find the next value that matches */
	tempValue = hoof->currentValue;
	while ( 1 )
	{
		/* go down */
		tempValue = tempValue->down;

		/* if tail */
		if ( tempValue->wordHead == NULL )
		{
			hoof->currentValue = tempValue;
			goto cleanup;
		}

		/* do tempValue and currentValue match up until currentWord? */
		match = 1;

		tempWord1 = hoof->currentValue->wordHead;
		tempWord2 = tempValue->wordHead;

		while ( tempWord1 != hoof->currentWord )
		{
			tempWord1 = tempWord1->right;
			tempWord2 = tempWord2->right;

			if ( tempWord2->value == NULL )
			{
				match = 0;
				break;
			}

			if ( strcmp( tempWord1->value, tempWord2->value ) != 0 )
			{
				match = 0;
				break;
			}
		}

		if ( match )
		{
			if ( word == NULL )
			{
				if ( tempWord2->right->value == NULL )
				{
					hoof->currentValue = tempValue;
					hoof->currentWord = tempWord2->right;
					goto cleanup;
				}
			}
			else
			{
				/* if next word in this value matches */
				if (    tempWord2->right->value != NULL
				     && strcmp( tempWord2->right->value, word ) == 0
				   )
				{
					hoof->currentValue = tempValue;
					hoof->currentWord = tempWord2->right;
					goto cleanup;
				}
			}
		}
	}


	/* CLEANUP */
	cleanup:

	return;
}

