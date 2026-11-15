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

// Chat tokens.  Retail expands them in team chat and tells (qagamex86
// 0x10041450, called from G_Say at 0x10041800 for modes 1 and 2):
//
//   #h  the speaker's health ("0" when below one)
//   #a  the speaker's armor
//   #w  the held weapon and its ammo, "Rocket Launcher [25]"; the gauntlet
//       is named without a count
//   ##  a literal '#'
//
// Any other character after '#' is kept and the '#' dropped.  A spectator's
// text is copied unchanged.

// The names are the first column of the retail weapon table (0x1008ff00,
// rows of 0x30 bytes); 0x1002cd10 answers "None" outside weapons 1..14.
static const char *const chatWeaponNames[WP_NUM_WEAPONS] = {
	"None", "Gauntlet", "Machine Gun", "Shotgun", "Grenade Launcher",
	"Rocket Launcher", "Lightning Gun", "Railgun", "Plasma Gun", "BFG",
	"Grappling Hook", "Nailgun", "Prox Launcher", "Chaingun", "Heavy Machinegun"
};

const char *G_ChatWeaponName( int weapon ) {
	if ( weapon < WP_GAUNTLET || weapon >= WP_NUM_WEAPONS ) {
		return chatWeaponNames[WP_NONE];
	}
	return chatWeaponNames[weapon];
}

/*
================
G_ExpandChatTokens

Writes the expanded text to output, always terminated.  Returns qfalse when
the text did not fit and was cut.

Observed in retail: the loop gives up, with a console note, once it has read
MAX_SAY_TEXT - 1 input characters, without terminating the text there.  An
expansion is appended with Q_strcat (0x1004155f calling 0x10070ab0) at a
write position that was never terminated, so it lands after whatever stale
stack bytes precede the next NUL: two retail runs of one message print
different text (the G3 retail-twice self-check diverges on any #h, #a or
#w), and a long enough run of stale bytes ends in Q_strcat's overflow error.

This version implements the evident intent instead: each expansion goes at
the write position.  It keeps the read limit and the note, always terminates
the text, and stops at the end of the output.
================
*/
qboolean G_ExpandChatTokens( const playerState_t *ps, qboolean spectator,
		const char *input, char *output, int outputSize ) {
	char		expansion[64];
	const char	*piece;
	int			read, written = 0;
	qboolean	pending = qfalse;
	qboolean	complete = qtrue;

	if ( !output || outputSize <= 0 ) {
		return qfalse;
	}
	output[0] = '\0';
	if ( !input ) {
		return qtrue;
	}
	if ( !ps || spectator ) {
		Q_strncpyz( output, input, outputSize );
		return strlen( input ) < (size_t)outputSize;
	}

	for ( read = 0; input[read] && complete; read++ ) {
		const char ch = input[read];

		if ( read >= MAX_SAY_TEXT - 1 ) {
			G_Printf( "Chat token output buffer overflow...\n" );
			complete = qfalse;
			break;
		}
		if ( !pending ) {
			if ( ch == '#' ) {
				pending = qtrue;
				continue;
			}
			expansion[0] = ch;
			expansion[1] = '\0';
			piece = expansion;
		} else {
			pending = qfalse;
			switch ( ch ) {
			case 'a':
				Com_sprintf( expansion, sizeof( expansion ), "%d", ps->stats[STAT_ARMOR] );
				break;
			case 'h':
				if ( ps->stats[STAT_HEALTH] >= 1 ) {
					Com_sprintf( expansion, sizeof( expansion ), "%d", ps->stats[STAT_HEALTH] );
				} else {
					Q_strncpyz( expansion, "0", sizeof( expansion ) );
				}
				break;
			case 'w':
				if ( ps->weapon != WP_GAUNTLET ) {
					// retail indexes the ammo array with the raw weapon number
					const int ammo = ( ps->weapon >= 0 && ps->weapon < MAX_WEAPONS ) ? ps->ammo[ps->weapon] : 0;
					Com_sprintf( expansion, sizeof( expansion ), "%s [%d]", G_ChatWeaponName( ps->weapon ), ammo );
				} else {
					Com_sprintf( expansion, sizeof( expansion ), "%s", G_ChatWeaponName( WP_GAUNTLET ) );
				}
				break;
			default:	// "##" and every unknown token: the character alone
				expansion[0] = ch;
				expansion[1] = '\0';
				break;
			}
			piece = expansion;
		}

		while ( *piece ) {
			if ( written >= outputSize - 1 ) {
				complete = qfalse;
				break;
			}
			output[written++] = *piece++;
		}
	}
	output[written] = '\0';
	return complete;
}
