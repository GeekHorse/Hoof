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
#ifndef hoof_H
#define hoof_H

/******************************************************************************/
#define HOOF_NAME "Hoof"

#define HOOF_COPYRIGHT "Copyright (C) 2014-2015 Jeremiah Martell"

#define HOOF_VERSION_STRING "0.9.01"
#define HOOF_VERSION 9010
#define HOOF_VERSION_MAJOR       ( HOOF_VERSION / 10000 )
#define HOOF_VERSION_MINOR       ( ( HOOF_VERSION / 1000 ) % 10 )
#define HOOF_VERSION_SUBMINOR    ( ( HOOF_VERSION / 10 ) % 100 )
#define HOOF_VERSION_FINAL       ( HOOF_VERSION % 10 )

/******************************************************************************/
#define HOOF_LIBRARY_NUMBER 300

/******************************************************************************/
#define HOOF_RC int

#define HOOF_RC_SUCCESS                          0
#define HOOF_RC_ERROR_PRECOND                    1
#define HOOF_RC_ERROR_MEMORY                     2
#define HOOF_RC_ERROR_FILE                       3
#define HOOF_RC_ERROR_NETWORK                    4
#define HOOF_RC_ERROR_STANDARD_LIBRARY           5
#define HOOF_RC_ERROR_FAILURE_POINT              6

/* This must be kept in sync with the above defines */
#define HOOF_RC_STANDARD_ERRORS_MAX              6

/* Hoof specific rc values */
#define HOOF_RC_QUIT                             301
#define HOOF_RC_ERROR_FILENAME_BAD               302
#define HOOF_RC_ERROR_FILENAME_LONG              303
#define HOOF_RC_ERROR_FILE_BAD                   304
#define HOOF_RC_ERROR_WORD_BAD                   305
#define HOOF_RC_ERROR_WORD_LONG                  306

/* These must be kept in sync with the above defines */
#define HOOF_RC_HOOF_ERRORS_MIN                  301
#define HOOF_RC_HOOF_ERRORS_MAX                  306

/******************************************************************************/
#define HOOF_MAX_WORD_LENGTH  31

/******************************************************************************/
typedef struct HoofSTRUCT Hoof;

typedef struct
{
	char inputWord[ HOOF_MAX_WORD_LENGTH + 1 ];
	char outputWord[ HOOF_MAX_WORD_LENGTH + 1 ];
	int moreToOutput;
	int delayAfterSayingOutputWord;
} HoofInterface;

HOOF_RC hoofInit( char *filename, Hoof **hoof_A );
void hoofFree( Hoof **hoof_F );

HOOF_RC hoofDo( Hoof *hoof, HoofInterface *hoofInterface );

const char *hoofRCToString( int rc );

/******************************************************************************/
#endif

