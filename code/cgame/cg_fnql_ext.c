/*
===========================================================================
FnQL-game -- clean-room Quake Live compatible game modules.

This file is part of FnQL-game and is free software; you can redistribute
it and/or modify it under the terms of the GNU General Public License as
published by the Free Software Foundation; either version 2 of the License,
or (at your option) any later version.
===========================================================================
*/

// FnQL extension negotiation, client side (../game/bg_fnql_ext.h).  Not
// retail: retail cgame has none of it.  The fnql_cg userinfo answer is set
// only while this cgame runs on a server that announced fnql_game, so a
// retail server never receives it.

#include "cg_local.h"
#include "../game/bg_fnql_ext.h"

static unsigned cgFnQLExtensions;	// enabled by this level's handshake
static char cgFnQLServerVersion[ 32 ];

/*
================
CG_FnQLExtInit

After the serverinfo is parsed: answer an FnQL-game server, and withdraw
the answer from any other.
================
*/
void CG_FnQLExtInit( void ) {
	const char *announced = Info_ValueForKey( CG_ConfigString( CS_SERVERINFO ), FNQL_EXT_SERVERINFO_KEY );

	cgFnQLExtensions = 0;
	cgFnQLServerVersion[0] = '\0';
	// a temp userinfo cvar: never archived, sent only while set
	trap_Cvar_Register( NULL, FNQL_EXT_USERINFO_KEY, "", CVAR_USERINFO | CVAR_TEMP );
	trap_Cvar_Set( FNQL_EXT_USERINFO_KEY, announced[0] ? va( "%i", FNQL_EXT_PROTOCOL ) : "" );
}

/*
================
CG_FnQLExtShutdown

The next server may be a retail one: it must not receive the answer, not
even in the connect packet.
================
*/
void CG_FnQLExtShutdown( void ) {
	cgFnQLExtensions = 0;
	trap_Cvar_Set( FNQL_EXT_USERINFO_KEY, "" );
}

/*
================
CG_FnQLExtensions
================
*/
unsigned CG_FnQLExtensions( void ) {
	return cgFnQLExtensions;
}

/*
================
CG_FnQLExtCommand

Claims the extension server commands.  Only a server that read our
answer sends them, so on any other server this claims nothing.
================
*/
qboolean CG_FnQLExtCommand( const char *cmd ) {
	unsigned offered;

	if ( strcmp( cmd, FNQL_EXT_CMD_HELLO ) ) {
		return qfalse;
	}
	if ( BG_FnQLExtParseProtocol( CG_Argv( 1 ) ) != FNQL_EXT_PROTOCOL ) {
		CG_Printf( "FnQL-game server speaks extension protocol %s; ignoring it.\n", CG_Argv( 1 ) );
		return qtrue;
	}
	offered = BG_FnQLExtParseMask( CG_Argv( 2 ) );
	cgFnQLExtensions = offered & FNQL_EXT_SUPPORTED;
	Q_strncpyz( cgFnQLServerVersion, CG_Argv( 3 ), sizeof( cgFnQLServerVersion ) );
	CG_Printf( "FnQL-game server %s: extensions %i\n", cgFnQLServerVersion, (int)cgFnQLExtensions );
	trap_SendClientCommand( va( FNQL_EXT_CMD_ACK " %i %i", FNQL_EXT_PROTOCOL, (int)cgFnQLExtensions ) );
	return qtrue;
}
