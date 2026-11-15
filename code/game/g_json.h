/*
===========================================================================
FnQL-game -- clean-room Quake Live compatible game modules.

This file is part of FnQL-game and is free software; you can redistribute
it and/or modify it under the terms of the GNU General Public License as
published by the Free Software Foundation; either version 2 of the License,
or (at your option) any later version.
===========================================================================
*/

// A small bounded JSON writer for the match statistics documents.
//
// Retail builds those documents as JsonCpp values, whose objects are maps:
// members come out sorted by key, and assigning a key twice keeps the last
// value.  This writer gives the same two properties, so a builder can write
// members in the order the retail code touches them.

#ifndef G_JSON_H
#define G_JSON_H

#include "q_shared.h"

#define JSON_MAX_DEPTH		8
#define JSON_MAX_MEMBERS	80	// of one object or array

typedef struct {
	qboolean	object;
	int			start;						// where the container's text begins
	int			count;
	int			offsets[JSON_MAX_MEMBERS];	// each member's text, back to back
} jsonFrame_t;

typedef struct {
	char		*buffer;		// the document
	char		*scratch;		// same size, for closing containers
	int			size;
	int			length;
	int			depth;
	qboolean	failed;			// out of room, nesting, members, or a bad key
	jsonFrame_t	frames[JSON_MAX_DEPTH];
} jsonWriter_t;

// buffer and scratch are size bytes each and stay the caller's
void JSON_Init( jsonWriter_t *writer, char *buffer, char *scratch, int size );

// key names the member inside an object and is ignored (pass NULL) inside an
// array and for the document's root value.  Keys are [A-Za-z0-9_]+.
void JSON_Null( jsonWriter_t *writer, const char *key );
void JSON_Bool( jsonWriter_t *writer, const char *key, qboolean value );
void JSON_Int( jsonWriter_t *writer, const char *key, int value );
void JSON_Real( jsonWriter_t *writer, const char *key, double value );
void JSON_String( jsonWriter_t *writer, const char *key, const char *value );
void JSON_BeginObject( jsonWriter_t *writer, const char *key );
void JSON_BeginArray( jsonWriter_t *writer, const char *key );
void JSON_End( jsonWriter_t *writer );

// the finished, terminated document, or NULL when anything went wrong
const char *JSON_Finish( jsonWriter_t *writer );

#endif
