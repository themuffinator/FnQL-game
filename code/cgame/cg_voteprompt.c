/*
===========================================================================
FnQL-game -- clean-room Quake Live compatible game modules.

This file is part of FnQL-game and is free software; you can redistribute
it and/or modify it under the terms of the GNU General Public License as
published by the Free Software Foundation; either version 2 of the License,
or (at your option) any later version.
===========================================================================
*/

/* The vote and team-kill complaint prompt. Observed in retail cgame:
 *
 * - The vote drawer (0x1000dbb0) paints two lines, at (4, 300) in yellow
 *   (0x100761e4) and (8, 312) in white (0x10076214), both at scale 0.22 with
 *   a shadow.
 * - In the team modes other than Red Rover a complaint comes first while
 *   its time runs: "File complaint against %s for team-killing?" and
 *   "Press '%s' for Yes, or '%s' for No (%is)", never in a demo and only
 *   with cg_complaintWarning. A negative complaint number is a notice,
 *   painted alone in white at (3, 300); -3 reads "Comlaints" in retail.
 * - Otherwise the vote in progress: "VOTE(%is):%s Yes(%s):%i No(%s):%i" and
 *   "or press ESC then click Vote". The keys are the ones bound to
 *   "vote yes" and "vote no".
 * - The "complaint" server command (0x1004bc43) starts the prompt for
 *   fifteen seconds, or a notice for ten; "map_restart" clears it
 *   (0x1004a369). */

#include "cg_local.h"
#include "../ui/ui_shared.h"

/* Fills the two lines and returns how many are shown: 0 for nothing, 1 for
 * a complaint notice (in second), 2 for a complaint prompt or a vote. */
int CG_VotePromptText( char *first, int firstSize, char *second, int secondSize ) {
	char	yesKey[32], noKey[32];
	int		sec;

	first[0] = second[0] = '\0';
	trap_Key_KeynumToStringBuf( trap_Key_GetKey( "vote yes" ), yesKey, sizeof( yesKey ) );
	trap_Key_KeynumToStringBuf( trap_Key_GetKey( "vote no" ), noKey, sizeof( noKey ) );

	if ( cgs.gametype >= GT_TEAM && cgs.gametype != GT_RED_ROVER && cgs.complaintEndTime > cg.time ) {
		if ( cg_complaintWarning.integer && cgs.complaintClient >= 0 ) {
			if ( !cg.demoPlayback && cgs.complaintClient < MAX_CLIENTS ) {
				sec = ( cgs.complaintEndTime - cg.time ) / 1000;
				Com_sprintf( first, firstSize, "File complaint against %s for team-killing?",
					cgs.clientinfo[cgs.complaintClient].name );
				Com_sprintf( second, secondSize, "Press '%s' for Yes, or '%s' for No (%is)", yesKey, noKey, sec );
				return 2;
			}
		} else if ( !cg.demoPlayback && cg_complaintWarning.integer > 0 && cgs.complaintClient < 0 ) {
			const char *notice = NULL;

			switch ( cgs.complaintClient ) {
			case -1: notice = "Your complaint has been filed."; break;
			case -2: notice = "Your complaint has been dismissed."; break;
			case -3: notice = "Comlaints cannot be filed against server admins."; break;	/* retail's spelling */
			case -4: notice = "You received friendly fire from a server admin."; break;
			default: break;
			}
			if ( notice ) {
				Q_strncpyz( second, notice, secondSize );
				return 1;
			}
		}
	}

	if ( !cgs.voteTime ) {
		return 0;
	}
	sec = ( VOTE_TIME - ( cg.time - cgs.voteTime ) ) / 1000;
	if ( sec < 0 ) {
		sec = 0;
	}
	Com_sprintf( first, firstSize, "VOTE(%is):%s Yes(%s):%i No(%s):%i", sec, cgs.voteString,
		yesKey, cgs.voteYes, noKey, cgs.voteNo );
	Q_strncpyz( second, "or press ESC then click Vote", secondSize );
	return 2;
}

/* The "complaint" server command: a client number to complain about, or a
 * negative notice. */
void CG_SetComplaint( int clientNum ) {
	cgs.complaintClient = clientNum;
	cgs.complaintEndTime = cg.time + ( clientNum < 0 ? 10000 : 15000 );
}

void CG_ClearComplaint( void ) {
	cgs.complaintClient = -1;
	cgs.complaintEndTime = 0;
}

void CG_DrawVote( void ) {
	static vec4_t	yellow = { 1.0f, 1.0f, 0.0f, 1.0f };
	static vec4_t	white = { 1.0f, 1.0f, 1.0f, 1.0f };
	char	first[MAX_STRING_CHARS], second[256];
	int		lines;
	qboolean	vote;

	vote = cgs.voteTime != 0;
	lines = CG_VotePromptText( first, sizeof( first ), second, sizeof( second ) );
	if ( !lines ) {
		return;
	}
	if ( lines == 1 ) {
		CG_Text_Paint( 3, 300, 0.22f, white, second, 0, 0, ITEM_TEXTSTYLE_SHADOWED );
		return;
	}
	/* a talk beep whenever the vote shown changes (not for a complaint) */
	if ( vote && !strncmp( first, "VOTE(", 5 ) && cgs.voteModified ) {
		cgs.voteModified = qfalse;
		trap_S_StartLocalSound( cgs.media.talkSound, CHAN_LOCAL_SOUND );
	}
	CG_Text_Paint( 4, 300, 0.22f, yellow, first, 0, 0, ITEM_TEXTSTYLE_SHADOWED );
	CG_Text_Paint( 8, 312, 0.22f, white, second, 0, 0, ITEM_TEXTSTYLE_SHADOWED );
}
