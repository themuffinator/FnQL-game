/*
===========================================================================
FnQL-game -- clean-room Quake Live compatible game modules.

This file is part of FnQL-game and is free software; you can redistribute
it and/or modify it under the terms of the GNU General Public License as
published by the Free Software Foundation; either version 2 of the License,
or (at your option) any later version.
===========================================================================
*/

#include "g_local.h"

// Retail uniqueness (0x1003a350/0x10070b90) compares the first 39 bytes,
// ignores numeric color codes only, and folds ASCII case. Keep this separate
// from display-name cleaning and the broader shared Q_CleanStr policy.
static void PlayerNameKey( const char *name, char key[MAX_NETNAME] ) {
	size_t read = 0, written = 0;
	if ( name ) {
		while ( read < MAX_NETNAME - 1 && name[read] ) {
			unsigned char ch = (unsigned char)name[read++];
			if ( ch == '^' && read < MAX_NETNAME - 1 &&
				name[read] >= '0' && name[read] <= '7' ) {
				++read;
				continue;
			}
			if ( ch < ' ' ) continue;
			key[written++] = ch >= 'a' && ch <= 'z' ? ch - 'a' + 'A' : ch;
		}
	}
	key[written] = '\0';
}

qboolean G_PlayerNamesEqual( const char *a, const char *b ) {
	char left[MAX_NETNAME], right[MAX_NETNAME];
	PlayerNameKey(a, left);
	PlayerNameKey(b, right);
	return strcmp(left, right) == 0;
}

// Retail normalizes into an info-sized buffer (0x1003a0b0), then copies
// at most 39 bytes into the client's name (0x1003a771). In particular, the
// final copy may clip a color token; do not move that truncation earlier.
void G_CleanPlayerName( const char *input, char *output, int outputSize ) {
	char cleaned[MAX_INFO_STRING];
	size_t length = 0, visible = 0, spaces = 0, reserved = 0, copy;
	qboolean colored = qfalse;
	const unsigned char *cursor = (const unsigned char *)(input ? input : "");

	if ( !output || outputSize <= 0 ) {
		return;
	}
	while ( *cursor ) {
		unsigned char ch = *cursor++;
		// Retain the existing protection against controls in protocol text.
		// Bytes above ASCII stay intact, as in the retail name cleaner.
		if ( ch < ' ' || ch == 127 || ch == '"' || ch == '\'' || ch == '&' || ch == '<' || ch == '>'
			|| ( !length && ch == ' ' ) ) {
			continue;
		}
		if ( ch == Q_COLOR_ESCAPE ) {
			unsigned char code = *cursor;
			qboolean nextColored;
			size_t nextReserved;
			if ( !code ) {
				break;
			}
			cursor++;
			if ( code < ' ' || code == 127 || ColorIndex(code) == 0 ) {
				continue; // black is discarded without changing the active color
			}
			nextColored = ColorIndex(code) != 7;
			// Retail charges two bytes per nonwhite token, including repeated
			// colors, and refunds one charge when returning to white.
			nextReserved = nextColored ? reserved + 2 : reserved - ( colored ? 2 : 0 );
			if ( length + 2 + nextReserved >= sizeof(cleaned) ) {
				break;
			}
			cleaned[length++] = Q_COLOR_ESCAPE;
			cleaned[length++] = (char)code;
			reserved = nextReserved;
			colored = nextColored;
			continue;
		}
		spaces = ch == ' ' ? spaces + 1 : 0;
		if ( spaces > 3 ) {
			continue;
		}
		if ( length + 1 + reserved >= sizeof(cleaned) ) {
			break;
		}
		cleaned[length++] = (char)ch;
		visible++;
	}
	if ( !visible ) {
		const char fallback[] = "UnnamedPlayer";
		length = sizeof(fallback) - 1;
		memcpy( cleaned, fallback, length );
	} else if ( colored ) {
		// Retail 0x1003a1c6: prevent the name's last color leaking into text
		// that follows it. Explicit white (including aliases) needs no reset.
		cleaned[length++] = Q_COLOR_ESCAPE;
		cleaned[length++] = '7';
	}
	copy = length < (size_t)outputSize - 1 ? length : (size_t)outputSize - 1;
	memcpy( output, cleaned, copy );
	output[copy] = '\0';
}
