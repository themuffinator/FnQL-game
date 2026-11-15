/*
===========================================================================
FnQL-game -- clean-room Quake Live compatible game modules.

This file is part of FnQL-game and is free software; you can redistribute
it and/or modify it under the terms of the GNU General Public License as
published by the Free Software Foundation; either version 2 of the License,
or (at your option) any later version.
===========================================================================
*/

#include "g_json.h"

#include <math.h>
#include <stdio.h>

// While a container is open its members sit in the buffer back to back,
// each a complete piece of text: "KEY":value in an object, value in an
// array.  Closing it rewrites that stretch as {a,b,c} or [a,b,c]; an object
// is sorted by key first and loses every member a later one of the same key
// replaced.  A closed container is itself one member of its parent.

static void JSON_Append( jsonWriter_t *writer, const char *text, int length ) {
	if ( writer->failed ) {
		return;
	}
	if ( length < 0 || length >= writer->size - writer->length ) {
		writer->failed = qtrue;
		return;
	}
	memcpy( writer->buffer + writer->length, text, length );
	writer->length += length;
}

static void JSON_AppendString( jsonWriter_t *writer, const char *text ) {
	JSON_Append( writer, text, (int)strlen( text ) );
}

// Quoted text.  Everything outside printable ASCII goes out as \u00XX, so
// the document is plain ASCII whatever bytes a player name carries (the
// bytes are read as Latin-1, the game's own character set).
static void JSON_AppendQuoted( jsonWriter_t *writer, const char *text ) {
	char	escape[8];

	JSON_Append( writer, "\"", 1 );
	for ( ; *text && !writer->failed; text++ ) {
		const unsigned char ch = (unsigned char)*text;

		switch ( ch ) {
		case '"':	JSON_Append( writer, "\\\"", 2 ); break;
		case '\\':	JSON_Append( writer, "\\\\", 2 ); break;
		case '\b':	JSON_Append( writer, "\\b", 2 ); break;
		case '\f':	JSON_Append( writer, "\\f", 2 ); break;
		case '\n':	JSON_Append( writer, "\\n", 2 ); break;
		case '\r':	JSON_Append( writer, "\\r", 2 ); break;
		case '\t':	JSON_Append( writer, "\\t", 2 ); break;
		default:
			if ( ch < 0x20 || ch >= 0x7f ) {
				snprintf( escape, sizeof( escape ), "\\u%04x", ch );
				JSON_Append( writer, escape, 6 );
			} else {
				JSON_Append( writer, text, 1 );
			}
			break;
		}
	}
	JSON_Append( writer, "\"", 1 );
}

static qboolean JSON_KeyIsPlain( const char *key ) {
	if ( !key || !key[0] ) {
		return qfalse;
	}
	for ( ; *key; key++ ) {
		const char ch = *key;

		if ( !( ( ch >= 'A' && ch <= 'Z' ) || ( ch >= 'a' && ch <= 'z' ) || ( ch >= '0' && ch <= '9' ) || ch == '_' ) ) {
			return qfalse;
		}
	}
	return qtrue;
}

// Starts a member of the open container (or the root value).
static void JSON_BeginMember( jsonWriter_t *writer, const char *key ) {
	jsonFrame_t	*frame;

	if ( writer->failed ) {
		return;
	}
	if ( writer->depth == 0 ) {
		if ( writer->length != 0 ) {
			writer->failed = qtrue;		// one root value only
		}
		return;
	}
	frame = &writer->frames[writer->depth - 1];
	if ( frame->count >= JSON_MAX_MEMBERS ) {
		writer->failed = qtrue;
		return;
	}
	frame->offsets[frame->count++] = writer->length;
	if ( frame->object ) {
		if ( !JSON_KeyIsPlain( key ) ) {
			writer->failed = qtrue;
			return;
		}
		JSON_Append( writer, "\"", 1 );
		JSON_AppendString( writer, key );
		JSON_Append( writer, "\":", 2 );
	}
}

void JSON_Init( jsonWriter_t *writer, char *buffer, char *scratch, int size ) {
	memset( writer, 0, sizeof( *writer ) );
	writer->buffer = buffer;
	writer->scratch = scratch;
	writer->size = size;
	if ( !buffer || !scratch || size < 8 ) {
		writer->failed = qtrue;
		writer->size = 0;
	}
}

void JSON_Null( jsonWriter_t *writer, const char *key ) {
	JSON_BeginMember( writer, key );
	JSON_Append( writer, "null", 4 );
}

void JSON_Bool( jsonWriter_t *writer, const char *key, qboolean value ) {
	JSON_BeginMember( writer, key );
	JSON_AppendString( writer, value ? "true" : "false" );
}

void JSON_Int( jsonWriter_t *writer, const char *key, int value ) {
	char	text[16];

	JSON_BeginMember( writer, key );
	snprintf( text, sizeof( text ), "%d", value );
	JSON_AppendString( writer, text );
}

/*
================
JSON_Real

A real always reads as one: retail's documents went through JsonCpp, whose
writer prints seventeen significant digits' worth ("%#.16g") and then drops
the trailing zeros after the last digit that matters, keeping one, so 4
prints as 4.0 and 0.5 as 0.5000000000000000 -> 0.50.  Kept here: a consumer
that tells integers from reals by the text sees the same types.  Not-a-number
and the infinities have no JSON form and are written as 0.0.
================
*/
void JSON_Real( jsonWriter_t *writer, const char *key, double value ) {
	char	text[40];
	int		length, last;

	JSON_BeginMember( writer, key );
	if ( value != value || value > 1.7e308 || value < -1.7e308 ) {
		value = 0.0;
	}
	snprintf( text, sizeof( text ), "%#.16g", value );
	length = (int)strlen( text );
	// only a plain "digits.digits" tail is trimmed; an exponent is left alone
	if ( length > 0 && text[length - 1] == '0' && !strchr( text, 'e' ) && !strchr( text, 'E' ) ) {
		last = length - 1;
		while ( last > 0 && text[last] == '0' ) {
			last--;
		}
		// last is the final non-zero character: a digit, or the point itself
		text[last + 2] = '\0';
	}
	JSON_AppendString( writer, text );
}

void JSON_String( jsonWriter_t *writer, const char *key, const char *value ) {
	JSON_BeginMember( writer, key );
	JSON_AppendQuoted( writer, value ? value : "" );
}

static void JSON_Begin( jsonWriter_t *writer, const char *key, qboolean object ) {
	jsonFrame_t	*frame;

	JSON_BeginMember( writer, key );
	if ( writer->failed ) {
		return;
	}
	if ( writer->depth >= JSON_MAX_DEPTH ) {
		writer->failed = qtrue;
		return;
	}
	frame = &writer->frames[writer->depth++];
	frame->object = object;
	frame->start = writer->length;
	frame->count = 0;
}

void JSON_BeginObject( jsonWriter_t *writer, const char *key ) {
	JSON_Begin( writer, key, qtrue );
}

void JSON_BeginArray( jsonWriter_t *writer, const char *key ) {
	JSON_Begin( writer, key, qfalse );
}

// The key of an object member's text: the characters between its first two
// quotes (keys are plain, so there is no escape to look for).
static int JSON_CompareKeys( const char *a, const char *b ) {
	a++;
	b++;
	while ( *a != '"' && *b != '"' && *a == *b ) {
		a++;
		b++;
	}
	if ( *a == '"' || *b == '"' ) {
		return ( *a == '"' ? 0 : 1 ) - ( *b == '"' ? 0 : 1 );
	}
	return (unsigned char)*a - (unsigned char)*b;
}

void JSON_End( jsonWriter_t *writer ) {
	jsonFrame_t	*frame;
	int			order[JSON_MAX_MEMBERS];
	int			lengths[JSON_MAX_MEMBERS];
	int			i, j, count, out;

	if ( writer->failed ) {
		return;
	}
	if ( writer->depth <= 0 ) {
		writer->failed = qtrue;
		return;
	}
	frame = &writer->frames[--writer->depth];
	count = frame->count;
	for ( i = 0; i < count; i++ ) {
		lengths[i] = ( i + 1 < count ? frame->offsets[i + 1] : writer->length ) - frame->offsets[i];
		order[i] = i;
	}

	if ( frame->object ) {
		// stable insertion sort by key, so equal keys stay in writing order
		for ( i = 1; i < count; i++ ) {
			const int member = order[i];

			for ( j = i; j > 0 && JSON_CompareKeys( writer->buffer + frame->offsets[order[j - 1]],
					writer->buffer + frame->offsets[member] ) > 0; j-- ) {
				order[j] = order[j - 1];
			}
			order[j] = member;
		}
		// of equal keys only the last written survives
		for ( i = 0, j = 0; i < count; i++ ) {
			if ( i + 1 < count && JSON_CompareKeys( writer->buffer + frame->offsets[order[i]],
					writer->buffer + frame->offsets[order[i + 1]] ) == 0 ) {
				continue;
			}
			order[j++] = order[i];
		}
		count = j;
	}

	// brackets and commas add count + 1 bytes at most (2 when empty)
	out = 0;
	if ( writer->length - frame->start + count + 2 >= writer->size - frame->start ) {
		writer->failed = qtrue;
		return;
	}
	writer->scratch[out++] = frame->object ? '{' : '[';
	for ( i = 0; i < count; i++ ) {
		if ( i > 0 ) {
			writer->scratch[out++] = ',';
		}
		memcpy( writer->scratch + out, writer->buffer + frame->offsets[order[i]], lengths[order[i]] );
		out += lengths[order[i]];
	}
	writer->scratch[out++] = frame->object ? '}' : ']';
	memcpy( writer->buffer + frame->start, writer->scratch, out );
	writer->length = frame->start + out;
}

const char *JSON_Finish( jsonWriter_t *writer ) {
	if ( writer->failed || writer->depth != 0 || writer->length <= 0 || writer->length >= writer->size ) {
		return NULL;
	}
	writer->buffer[writer->length] = '\0';
	return writer->buffer;
}
