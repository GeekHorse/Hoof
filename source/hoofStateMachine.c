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
#define HOOF_FILE_NUMBER 2

/******************************************************************************/
#include "hoof.h"
#include "hoofInternal.h"

/******************************************************************************/
/*!
	\brief State hello. First state.
	\param[in] hoof Hoof Context.
	\param[in] interface Hoof Interface.
	\param[out] huh If this state doesn't understand the input word, it will
		set huh to 1.
	\return HOOF_RC

	This state performs no actions nor allows the user to do anything. It's
	simply to say hello and possibly give information.
*/
HOOF_INTERNAL HOOF_RC hoofStateHello( Hoof *hoof, HoofInterface *interface, int *huh )
{
	/* DATA */
	HOOF_RC rc = HOOF_RC_SUCCESS;


	/* CODE */
	(void)huh;

	SAY( "hello" );
	hoof->state = hoofStateNavigate;

	return rc;
}

/******************************************************************************/
/*!
	\brief State navigate. Root state.
	\param[in] hoof Hoof Context.
	\param[in] interface Hoof Interface.
	\param[out] huh If this state doesn't understand the input word, it will
		set huh to 1.
	\return HOOF_RC
*/
HOOF_INTERNAL HOOF_RC hoofStateNavigate( Hoof *hoof, HoofInterface *interface, int *huh )
{
	/* DATA */
	HOOF_RC rc = HOOF_RC_SUCCESS;

	HoofWord *readWord = NULL;


	/* CODE */
	if ( HEAR( "quit" ) )
	{
		ERR_IF( hoof->loading, HOOF_RC_ERROR_FILE_BAD );

		ERR_PASSTHROUGH( hoofSave( hoof ) );
		SAY( "goodbye" );
		rc = HOOF_RC_QUIT;
	}
	else if ( HEAR( "cancel" ) )
	{
		SAY( "navigate" );
	}
	else if ( HEAR( "left" ) )
	{
		PARANOID_ERR_IF( hoof->currentWord->left == NULL );
		if ( hoof->currentWord->left->value == NULL )
		{
			SAY( "edge" );
			goto cleanup;
		}

		hoof->currentWord = hoof->currentWord->left;
		SAY( "ok" );
	}	
	else if ( HEAR( "right" ) )
	{
		if (    hoof->currentWord->right == NULL
		     || hoof->currentWord->right->value == NULL
		   )
		{
			SAY( "edge" );
			goto cleanup;
		}

		hoof->currentWord = hoof->currentWord->right;
		SAY( "ok" );
	}	
	else if ( HEAR( "up" ) )
	{
		if ( hoof->currentValue->up->wordHead == NULL )
		{
			SAY( "edge" );
			goto cleanup;
		}

		hoofMakeCurrentValue( hoof, hoof->currentValue->up );
		SAY( "ok" );
	}	
	else if ( HEAR( "down" ) )
	{
		if ( hoof->currentValue->down->wordHead == NULL )
		{
			SAY( "edge" );
			goto cleanup;
		}

		hoofMakeCurrentValue( hoof, hoof->currentValue->down );
		SAY( "ok" );
	}	
	else if ( HEAR( "in" ) )
	{
		if ( hoof->currentValue->in == NULL )
		{
			SAY( "edge" );
			goto cleanup;
		}

		hoofMakeCurrentValue( hoof, hoof->currentValue->in->down );
		SAY( "ok" );
	}	
	else if ( HEAR( "out" ) )
	{
		if ( hoof->currentValue->out == NULL )
		{
			SAY( "edge" );
			goto cleanup;
		}

		hoofMakeCurrentValue( hoof, hoof->currentValue->out );
		SAY( "ok" );
	}
	else if ( HEAR( "root" ) )
	{
		hoofMakeCurrentValue( hoof, hoof->root->down );

		SAY( "ok" );
	}
	else if ( HEAR( "save" ) )
	{
		ERR_IF( hoof->loading, HOOF_RC_ERROR_FILE_BAD );

		ERR_PASSTHROUGH( hoofSave( hoof ) );

		SAY( "ok" );
	}
	else if ( HEAR( "clear" ) )
	{
		hoofValueClear( hoof, hoof->currentValue );

		SAY( "ok" );
	}
	else if ( HEAR( "word" ) )
	{
		if ( hoof->currentWord->value == NULL )
		{
			SAY( "empty" );
		}
		else
		{
			SAY( "ok" );
			SAY( hoof->currentWord->value );
		}
		hoof->state = hoofStateNavigate;
	}
	else if ( HEAR( "value" ) )
	{
		readWord = hoof->currentValue->wordHead->right;

		if ( readWord->value == NULL )
		{
			SAY( "empty" );
		}
		else
		{
			SAY( "ok" );
			while ( readWord->value != NULL )
			{
				SAY( readWord->value );
				readWord = readWord->right;
			}
		}
		hoof->state = hoofStateNavigate;
	}
	else if ( HEAR( "most" ) )
	{
		hoof->state = hoofStateMostChoice;
	}
	else if ( HEAR( "new" ) )
	{
		hoof->state = hoofStateNewChoice;
	}
	else if ( HEAR( "delete" ) )
	{
		hoof->state = hoofStateDeleteChoice;
	}
	else if ( HEAR( "move" ) )
	{
		hoof->state = hoofStateMoveChoice;
	}
	else if ( HEAR( "dig" ) )
	{
		hoof->currentWord = hoof->currentValue->wordHead;

		hoof->state = hoofStateDig;
	}
	else
	{
		(*huh) = 1;
	}


	/* CLEANUP */
	cleanup:

	return rc;
}

/******************************************************************************/
/*!
	\brief State most choice. Waiting for user to say which direction they want
		to go as far as possible.
	\param[in] hoof Hoof Context.
	\param[in] interface Hoof Interface.
	\param[out] huh If this state doesn't understand the input word, it will
		set huh to 1.
	\return HOOF_RC
*/
HOOF_INTERNAL HOOF_RC hoofStateMostChoice( Hoof *hoof, HoofInterface *interface, int *huh )
{
	/* DATA */
	HOOF_RC rc = HOOF_RC_SUCCESS;


	/* CODE */
	if ( HEAR( "cancel" ) )
	{
		hoof->state = hoofStateNavigate;
		SAY( "cancel" );
	}
	else if ( HEAR( "left" ) )
	{
		while ( hoof->currentWord->left->value != NULL )
		{
			hoof->currentWord = hoof->currentWord->left;
		}

		hoof->state = hoofStateNavigate;
		SAY( "ok" );
	}
	else if ( HEAR( "right" ) )
	{
		while (    hoof->currentWord->right != NULL
		        && hoof->currentWord->right->value != NULL
		      )
		{
			hoof->currentWord = hoof->currentWord->right;
		}

		hoof->state = hoofStateNavigate;
		SAY( "ok" );
	}
	else if ( HEAR( "up" ) )
	{
		hoofMostUp( hoof );

		hoof->state = hoofStateNavigate;
		SAY( "ok" );
	}
	else if ( HEAR( "down" ) )
	{
		hoofMostDown( hoof );

		hoof->state = hoofStateNavigate;
		SAY( "ok" );
	}
	else if ( HEAR( "in" ) )
	{
		hoofMostIn( hoof );

		hoof->state = hoofStateNavigate;
		SAY( "ok" );
	}
	else if ( HEAR( "out" ) )
	{
		hoofMostOut( hoof );

		hoof->state = hoofStateNavigate;
		SAY( "out" );
	}
	else
	{
		(*huh) = 1;
	}


	/* CLEANUP */
	/* cleanup: */

	return rc;
}

/******************************************************************************/
/*!
	\brief State new choice. Waiting for user to select which direction to
		begin inserting. Modifies current word and possibly current value.
	\param[in] hoof Hoof Context.
	\param[in] interface Hoof Interface.
	\param[out] huh If this state doesn't understand the input word, it will
		set huh to 1.
	\return HOOF_RC

	Directions left and right will begin inserting in the current value.

	Directions up and down will create a new value above or below the current
	value, make the new value the current value, then begin inserting at the
	beginning of the new current value.

	Direction in will create a new value at the top of in, make the new value the
	current value, then begin inserting at the beginning of the new current value.

	Direction out will create a new value downward of out, make the new value the
	current value, then begin inserting at the beginning of the new current value.
*/
HOOF_INTERNAL HOOF_RC hoofStateNewChoice( Hoof *hoof, HoofInterface *interface, int *huh )
{
	/* DATA */
	HOOF_RC rc = HOOF_RC_SUCCESS;


	/* CODE */
	if ( HEAR( "cancel" ) )
	{
		hoof->state = hoofStateNavigate;
		SAY( "cancel" );
	}
	else if ( HEAR( "left" ) )
	{
		/* we insert before currentWord, so we don't need to update currentWord here */
		hoof->state = hoofStateNew;
		SAY( "new" );
	}	
	else if ( HEAR( "right" ) )
	{
		if ( hoof->currentWord->right != NULL )
		{
			hoof->currentWord = hoof->currentWord->right;
		}
		hoof->state = hoofStateNew;
		SAY( "new" );
	}	
	else if ( HEAR( "up" ) )
	{
		ERR_PASSTHROUGH( hoofValueInsert( hoof->currentValue->up ) );

		hoofMakeCurrentValue( hoof, hoof->currentValue->up );

		hoof->state = hoofStateNew;
		SAY( "new" );
	}	
	else if ( HEAR( "down" ) )
	{
		ERR_PASSTHROUGH( hoofValueInsert( hoof->currentValue ) );

		hoofMakeCurrentValue( hoof, hoof->currentValue->down );

		hoof->state = hoofStateNew;
		SAY( "new" );
	}	
	else if ( HEAR( "in" ) )
	{
		if ( hoof->currentValue->in == NULL )
		{
			ERR_PASSTHROUGH( hoofPageInit( hoof->currentValue, 1, NULL ) );
		}
		else
		{
			ERR_PASSTHROUGH( hoofValueInsert( hoof->currentValue->in ) );
		}

		hoofMakeCurrentValue( hoof, hoof->currentValue->in->down );

		hoof->state = hoofStateNew;
		SAY( "new" );
	}	
	else if ( HEAR( "out" ) )
	{
		if ( hoof->currentValue->out == NULL )
		{
			SAY( "edge" );
			goto cleanup;
		}

		ERR_PASSTHROUGH( hoofValueInsert( hoof->currentValue->out ) );
		hoofMakeCurrentValue( hoof, hoof->currentValue->out->down );

		hoof->state = hoofStateNew;
		SAY( "new" );
	}	
	else
	{
		(*huh) = 1;
	}


	/* CLEANUP */
	cleanup:

	return rc;
}

/******************************************************************************/
/*!
	\brief State new. Insert words after current word, updating current word
		to the newly inserted word.
	\param[in] hoof Hoof Context.
	\param[in] interface Hoof Interface.
	\param[out] huh If this state doesn't understand the input word, it will
		set huh to 1.
	\return HOOF_RC
*/
HOOF_INTERNAL HOOF_RC hoofStateNew( Hoof *hoof, HoofInterface *interface, int *huh )
{
	/* DATA */
	HOOF_RC rc = HOOF_RC_SUCCESS;


	/* CODE */
	(void)huh;

	/* FUTURE: handle number */
	if ( HEAR( "" ) )
	{
		goto cleanup;
	}
	else if ( hoof->literal )
	{
		ERR_PASSTHROUGH( hoofWordInsert( hoof, interface->inputWord ) );

		hoof->literal = 0;
	}
	else if ( HEAR( "literal" ) )
	{
		hoof->literal = 1;
	}
	else if ( HEAR( "done" ) )
	{
		if ( hoof->currentWord->left->value != NULL )
		{
			hoof->currentWord = hoof->currentWord->left;
		}

		hoof->state = hoofStateNavigate;
		SAY( "ok" );
	}
	else
	{
		ERR_PASSTHROUGH( hoofWordInsert( hoof, interface->inputWord ) );
	}


	/* CLEANUP */
	cleanup:

	return rc;
}

/******************************************************************************/
/*!
	\brief State delete choice. Waiting for user to choose to delete current
		word or current value.
	\param[in] hoof Hoof Context.
	\param[in] interface Hoof Interface.
	\param[out] huh If this state doesn't understand the input word, it will
		set huh to 1.
	\return HOOF_RC
*/
HOOF_INTERNAL HOOF_RC hoofStateDeleteChoice( Hoof *hoof, HoofInterface *interface, int *huh )
{
	/* DATA */
	HOOF_RC rc = HOOF_RC_SUCCESS;


	/* CODE */
	if ( HEAR( "cancel" ) )
	{
		hoof->state = hoofStateNavigate;
		SAY( "cancel" );
	}
	else if ( HEAR( "word" ) )
	{
		hoofWordDelete( hoof );

		hoof->state = hoofStateNavigate;
		SAY( "ok" );
	}
	else if ( HEAR( "value" ) )
	{
		hoofValueDelete( hoof );

		hoof->state = hoofStateNavigate;
		SAY( "ok" );
	}
	else
	{
		(*huh) = 1;
	}


	/* CLEANUP */
	/* cleanup: */

	return rc;
}

/******************************************************************************/
/*!
	\brief State move choice. Waiting for user to choose which direction to move
		the current value or current word.
	\param[in] hoof Hoof Context.
	\param[in] interface Hoof Interface.
	\param[out] huh If this state doesn't understand the input word, it will
		set huh to 1.
	\return HOOF_RC

	Directions left and right will move the current word left or right.

	Directions up and down will move the current value up or down.

	Direction in will move the current value to be the inward value of value up.

	Direction out will move the current value to be the next downward value of
	value out.
*/
HOOF_INTERNAL HOOF_RC hoofStateMoveChoice( Hoof *hoof, HoofInterface *interface, int *huh )
{
	/* DATA */
	HOOF_RC rc = HOOF_RC_SUCCESS;


	/* CODE */
	if ( HEAR( "cancel" ) )
	{
		hoof->state = hoofStateNavigate;
		SAY( "cancel" );
	}
	else if ( HEAR( "left" ) )
	{
		if (    hoof->currentWord->value == NULL
		     || hoof->currentWord->left->value == NULL
		   )
		{
			hoof->state = hoofStateNavigate;
			SAY( "edge" );
			goto cleanup;
		}

		/* remove word from list */
		hoof->currentWord->left->right = hoof->currentWord->right;
		hoof->currentWord->right->left = hoof->currentWord->left;

		/* update word's links */
		hoof->currentWord->right = hoof->currentWord->left;
		hoof->currentWord->left  = hoof->currentWord->left->left;

		/* update left and right links */
		hoof->currentWord->left->right = hoof->currentWord;
		hoof->currentWord->right->left = hoof->currentWord;

		hoof->state = hoofStateNavigate;
		SAY( "ok" );
	}
	else if ( HEAR( "right" ) )
	{
		if (    hoof->currentWord->value == NULL
		     || hoof->currentWord->right->value == NULL
		   )
		{
			hoof->state = hoofStateNavigate;
			SAY( "edge" );
			goto cleanup;
		}

		/* remove word from list */
		hoof->currentWord->left->right = hoof->currentWord->right;
		hoof->currentWord->right->left = hoof->currentWord->left;

		/* update word's links */
		hoof->currentWord->left  = hoof->currentWord->right;
		hoof->currentWord->right = hoof->currentWord->right->right;

		/* update left and right links */
		hoof->currentWord->left->right = hoof->currentWord;
		hoof->currentWord->right->left = hoof->currentWord;

		hoof->state = hoofStateNavigate;
		SAY( "ok" );
	}
	else if ( HEAR( "up" ) )
	{
		if ( hoof->currentValue->up->wordHead == NULL )
		{
			hoof->state = hoofStateNavigate;
			SAY( "edge" );
			goto cleanup;
		}

		/* remove value from list */
		hoof->currentValue->up->down = hoof->currentValue->down;
		hoof->currentValue->down->up = hoof->currentValue->up;

		/* update value's links */
		hoof->currentValue->down = hoof->currentValue->up;
		hoof->currentValue->up   = hoof->currentValue->up->up;

		/* update up and down links */
		hoof->currentValue->up->down = hoof->currentValue;
		hoof->currentValue->down->up = hoof->currentValue;

		hoof->state = hoofStateNavigate;
		SAY( "ok" );
	}
	else if ( HEAR( "down" ) )
	{
		if ( hoof->currentValue->down->wordHead == NULL )
		{
			hoof->state = hoofStateNavigate;
			SAY( "edge" );
			goto cleanup;
		}

		/* remove value from list */
		hoof->currentValue->up->down = hoof->currentValue->down;
		hoof->currentValue->down->up = hoof->currentValue->up;

		/* update value's links */
		hoof->currentValue->up   = hoof->currentValue->down;
		hoof->currentValue->down = hoof->currentValue->down->down;

		/* update up and down links */
		hoof->currentValue->up->down = hoof->currentValue;
		hoof->currentValue->down->up = hoof->currentValue;

		hoof->state = hoofStateNavigate;
		SAY( "ok" );
	}
	else if ( HEAR( "in" ) )
	{
		if ( hoof->currentValue->up->wordHead == NULL )
		{
			hoof->state = hoofStateNavigate;
			SAY( "edge" );
			goto cleanup;
		}

		if ( hoof->currentValue->up->in == NULL )
		{
			ERR_PASSTHROUGH( hoofPageInit( hoof->currentValue->up, 0, NULL ) );
		}

		/* remove value from list */
		hoof->currentValue->up->down = hoof->currentValue->down;
		hoof->currentValue->down->up = hoof->currentValue->up;

		/* update value's links */
		hoof->currentValue->down = hoof->currentValue->up->in->down;
		hoof->currentValue->up   = hoof->currentValue->up->in;

		/* update up and down links */
		hoof->currentValue->up->down = hoof->currentValue;
		hoof->currentValue->down->up = hoof->currentValue;

		/* update out link */
		hoof->currentValue->out = hoof->currentValue->up->out;

		hoof->state = hoofStateNavigate;
		SAY( "ok" );
	}
	else if ( HEAR( "out" ) )
	{
		if ( hoof->currentValue->out == NULL )
		{
			hoof->state = hoofStateNavigate;
			SAY( "edge" );
			goto cleanup;
		}

		/* remove value from list */
		hoof->currentValue->up->down = hoof->currentValue->down;
		hoof->currentValue->down->up = hoof->currentValue->up;

		/* update value's links */
		hoof->currentValue->up   = hoof->currentValue->out;
		hoof->currentValue->down = hoof->currentValue->out->down;

		/* update up and down links */
		hoof->currentValue->up->down = hoof->currentValue;
		hoof->currentValue->down->up = hoof->currentValue;

		/* update out */
		hoof->currentValue->out  = hoof->currentValue->up->out;

		/* see if up's in (old out's in) is empty */
		if ( hoof->currentValue->up->in->down->wordHead == NULL )
		{
			HOOF_FREE( hoof->currentValue->up->in->down );
			HOOF_FREE( hoof->currentValue->up->in );
			hoof->currentValue->up->in = NULL;
		}

		hoof->state = hoofStateNavigate;
		SAY( "ok" );
	}
	else
	{
		(*huh) = 1;
	}


	/* CLEANUP */
	cleanup:

	return rc;
}

/******************************************************************************/
/*!
	\brief State Gid. Allows users to quickly navigate the heirarchy by 'find'
	       and 'in'.
	\param[in] hoof Hoof Context.
	\param[in] interface Hoof Interface.
	\param[out] huh If this state doesn't understand the input word, it will
		set huh to 1.
	\return HOOF_RC

	Can temporarily set currentValue to tail value during this state, but will
	leave this state with currentValue set to most down value.
*/
HOOF_INTERNAL HOOF_RC hoofStateDig( Hoof *hoof, HoofInterface *interface, int *huh )
{
	/* DATA */
	HOOF_RC rc = HOOF_RC_SUCCESS;


	/* CODE */
	(void)huh;

	/* FUTURE: handle number */
	if ( HEAR( "" ) )
	{
		goto cleanup;
	}
	else if ( hoof->literal )
	{
		hoofDig( hoof, interface->inputWord );
		hoof->literal = 0;
	}
	else if ( HEAR( "literal" ) )
	{
		hoof->literal = 1;
	}
	else if ( HEAR( "in" ) )
	{
		/* we want to find a value that's an exact match */
		hoofDig( hoof, NULL );

		/* if tail */
		if ( hoof->currentValue->wordHead == NULL )
		{
			/* we're done with our dig */
			goto cleanup;
		}

		/* if there's not an in */
		if ( hoof->currentValue->in == NULL )
		{
			/* go to tail to mark that we couldnt go in, which
			   will also end our dig */
			while ( hoof->currentValue->down != NULL )
			{
				hoof->currentValue = hoof->currentValue->down;
			}

			goto cleanup;
		}

		/* go in */
		hoofMakeCurrentValue( hoof, hoof->currentValue->in->down );
		/* need to start with currentWord as wordHead */
		hoof->currentWord = hoof->currentValue->wordHead;
	}
	else if ( HEAR( "done" ) )
	{
		/* we want to find a value that's an exact match */
		hoofDig( hoof, NULL );

		/* if tail */
		if ( hoof->currentValue->wordHead == NULL )
		{
			hoofMakeCurrentValue( hoof, hoof->currentValue->up );
			hoof->state = hoofStateNavigate;
			SAY( "edge" );
		}
		else
		{
			hoofMakeCurrentValue( hoof, hoof->currentValue );
			hoof->state = hoofStateNavigate;
			SAY( "ok" );
		}
	}
	else if ( HEAR( "cancel" ) )
	{
		/* we want to find a value that just "starts with", so stick with whatever value we're currently at */

		/* if tail */
		if ( hoof->currentValue->wordHead == NULL )
		{
			hoofMakeCurrentValue( hoof, hoof->currentValue->up );
			hoof->state = hoofStateNavigate;
			SAY( "edge" );
		}
		else
		{
			hoofMakeCurrentValue( hoof, hoof->currentValue );
			hoof->state = hoofStateNavigate;
			SAY( "ok" );
		}
	}
	else
	{
		hoofDig( hoof, interface->inputWord );
	}


	/* CLEANUP */
	cleanup:

	return rc;
}

