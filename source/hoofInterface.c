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
#define HOOF_FILE_NUMBER 4

/******************************************************************************/
#include "hoof.h"
#include "hoofInternal.h"

/******************************************************************************/
/*!
	\brief Loads filename into a hoof context.
	\param[in] filename Filename to load.
	\param[out] hoof_A On success, new hoof context. Caller is responsible to
		free with hoofFree().
	\return HOOF_RC
*/
HOOF_RC hoofInit( char *filename, Hoof **hoof_A )
{
	/* DATA */
	HOOF_RC rc = HOOF_RC_SUCCESS;

	Hoof *newHoof = NULL;
	HoofValue *newPage = NULL;
	char *newFilename = NULL;


	/* CODE */
	PARANOID_ERR_IF( filename == NULL );
	PARANOID_ERR_IF( hoof_A == NULL );
	PARANOID_ERR_IF( (*hoof_A) != NULL );

	ERR_PASSTHROUGH( hoofWordVerify( filename ) );

	HOOF_CALLOC( newHoof, Hoof, 1 );

	ERR_PASSTHROUGH( hoofStrdup( filename, &newFilename ) );

	ERR_PASSTHROUGH( hoofPageInit( NULL, 1, &newPage ) );

	newHoof->filename = newFilename;
	newFilename = NULL;

	newHoof->root = newPage;
	newPage = NULL;

	hoofMakeCurrentValue( newHoof, newHoof->root->down );

	/* load file */
	ERR_PASSTHROUGH( hoofLoad( newHoof ) );

	/* give back */
	(*hoof_A) = newHoof;
	newHoof = NULL;


	/* CLEANUP */
	cleanup:

	if ( newHoof != NULL )
	{
		HOOF_FREE( newHoof->filename );
		hoofPageDelete( newHoof, &(newHoof->root) );
		HOOF_FREE( newHoof );
	}
	HOOF_FREE( newFilename );

	return rc;
}

/******************************************************************************/
/*!
	\brief Frees hoof context. Does not save any data back to file.
	\param[in] hoof_F Hoof context to free.
	\return void
*/
void hoofFree( Hoof **hoof_F )
{
	/* CODE */
	if ( hoof_F == NULL || (*hoof_F) == NULL )
	{
		goto cleanup;
	}

	HOOF_FREE( (*hoof_F)->filename );

	hoofPageDelete( (*hoof_F), &((*hoof_F)->root) );

	HOOF_FREE( (*hoof_F) );


	/* CLEANUP */
	cleanup:

	return;
}

/******************************************************************************/
/*!
	\brief Interface into the hoof context. How you pass user words in and get
		hoof response words out.
	\param[in] hoof Hoof context.
	\param[in,out] interface Interface structure.
	\return HOOF_RC
*/
HOOF_RC hoofDo( Hoof *hoof, HoofInterface *interface )
{
	/* DATA */
	HOOF_RC rc = HOOF_RC_SUCCESS;

	int i = 0;
	int huh = 0;


	/* CODE */
	PARANOID_ERR_IF( hoof == NULL );
	PARANOID_ERR_IF( interface == NULL );

	while ( i <= HOOF_MAX_VALUE_LENGTH )
	{
		interface->outputValue[ i ][ 0 ] = '\0';
		i += 1;
	}

	ERR_PASSTHROUGH( hoofWordVerify( interface->inputWord ) );

	/* handle pause and resume */
	if ( hoof->paused )
	{
		if ( HEAR( "resume" ) )
		{
			hoof->paused = 0;
			SAY( "resumed" );
			goto cleanup;
		}

		goto cleanup;
	}

	if (    hoof->literal == 0
	     && HEAR( "pause" )
	   )
	{
		hoof->paused = 1;
		SAY( "paused" );
		goto cleanup;
	}

	rc = hoof->state( hoof, interface, &huh );

	if ( interface->inputWord[ 0 ] != '\0' && huh == 1 )
	{
		SAY( "huh" );
		goto cleanup;
	}

	interface->inputWord[ 0 ] = '\0';


	/* CLEANUP */
	cleanup:

	return rc;
}

/******************************************************************************/
/*!
	\brief Returns text version of hoof RC.
	\param[in] rc Hoof RC.
	\return char* Text version of hoof RC.
*/
const char *hoofRCToString( int rc )
{
	static const char *rcStrings[] =
	{
		"Success",

		"Error Precondition",
		"Error Memory",
		"Error File",
		"Error Network",
		"Error Standard Library",
		"Error Failure Point",

		"Quit",
		"Error Filename Contains Bad Characters",
		"Error Filename Too Long",
		"Error File Contains Bad Content",
		"Error Word Contains Bad Characters",
		"Error Word Too Long",
		"Error Value Too Long"
	};

	static const char *rcUnknown = "Unknown Error";

	/* standard errors */
	if ( rc >= 0 && rc <= HOOF_RC_STANDARD_ERRORS_MAX )
	{
		return rcStrings[ rc ];
	}
	/* hoof errors */
	else if ( rc >= HOOF_RC_HOOF_ERRORS_MIN && rc <= HOOF_RC_HOOF_ERRORS_MAX )
	{
		return rcStrings[ HOOF_RC_STANDARD_ERRORS_MAX + rc - HOOF_RC_HOOF_ERRORS_MIN + 1 ];
	}

	return rcUnknown;
}

