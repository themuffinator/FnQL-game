/*
===========================================================================
FnQL-game -- clean-room Quake Live compatible game modules.

This file is part of FnQL-game and is free software; you can redistribute
it and/or modify it under the terms of the GNU General Public License as
published by the Free Software Foundation; either version 2 of the License,
or (at your option) any later version.
===========================================================================
*/

// FnQL extension channels, server side (bg_fnql_ext.h has the protocol).
// None of this is retail behavior.  Retail clients never answer the
// fnql_game announcement, so nothing here ever reaches them: every send
// goes to one negotiated client's own command stream, and the commands that
// arrive from such a client are claimed here before the retail command
// table sees them.

#include "g_local.h"
#include "bg_fnql_ext.h"
#include "../../version/fnql_game_version.h"

static gclient_t *G_FnQLExtClient( int clientNum ) {
	if ( clientNum < 0 || clientNum >= level.maxclients || !level.clients ) {
		return NULL;
	}
	return &level.clients[ clientNum ];
}

static qboolean G_FnQLExtIsBot( int clientNum ) {
	return ( g_entities[ clientNum ].r.svFlags & SVF_BOT ) ? qtrue : qfalse;
}

/*
================
G_FnQLExtensions

The extensions enabled on this client's channel, 0 for a retail client,
a bot, or a free slot.
================
*/
unsigned G_FnQLExtensions( int clientNum ) {
	const gclient_t *client = G_FnQLExtClient( clientNum );

	if ( !client || client->pers.connected == CON_DISCONNECTED || G_FnQLExtIsBot( clientNum ) ) {
		return 0;
	}
	return client->pers.fnqlExtensions;
}

/*
================
G_FnQLExtSend

Sends an extension command to one client, only if that client enabled the
extension.  Returns whether it was sent.
================
*/
qboolean G_FnQLExtSend( int clientNum, unsigned extension, const char *text ) {
	if ( !extension || !text || ( G_FnQLExtensions( clientNum ) & extension ) != extension ) {
		return qfalse;
	}
	trap_SendServerCommand( clientNum, text );
	return qtrue;
}

/*
================
G_FnQLExtBroadcast

The extension counterpart of a -1 send: every client that enabled the
extension gets the command on its own channel, and nobody else does.
Returns how many clients got it.
================
*/
int G_FnQLExtBroadcast( unsigned extension, const char *text ) {
	int i, sent = 0;

	for ( i = 0; i < level.maxclients; i++ ) {
		if ( level.clients[ i ].pers.connected == CON_CONNECTED &&
			G_FnQLExtSend( i, extension, text ) ) {
			sent++;
		}
	}
	return sent;
}

static void G_FnQLExtSendHello( int clientNum ) {
	G_FnQLExtSend( clientNum, FNQL_EXT_HELLO, va( FNQL_EXT_CMD_HELLO " %i %i %s",
		FNQL_EXT_PROTOCOL, G_FnQLExtensions( clientNum ), FNQL_GAME_VERSION_STRING ) );
}

/*
================
G_FnQLExtUserinfo

Called from ClientUserinfoChanged for a connecting or connected client.
The client's fnql_cg key decides its channel; a client that newly enables
it while already in the game gets the handshake at once (one that is still
connecting gets it from ClientBegin).
================
*/
void G_FnQLExtUserinfo( int clientNum, const char *userinfo ) {
	gclient_t *client = G_FnQLExtClient( clientNum );
	unsigned previous, offered;
	int protocol;

	if ( !client ) {
		return;
	}
	previous = client->pers.fnqlExtensions;
	protocol = G_FnQLExtIsBot( clientNum ) ? 0
		: BG_FnQLExtParseProtocol( Info_ValueForKey( userinfo, FNQL_EXT_USERINFO_KEY ) );
	offered = protocol >= 1 ? FNQL_EXT_SUPPORTED : 0;

	client->pers.fnqlExtensions = offered;
	if ( offered != previous ) {
		client->pers.fnqlExtConfirmed = qfalse;
		if ( offered && client->pers.connected == CON_CONNECTED ) {
			G_FnQLExtSendHello( clientNum );
		}
	}
}

/*
================
G_FnQLExtBegin

Called from ClientBegin: each level (and restart) starts a new cgame, so
the handshake is repeated for it.
================
*/
void G_FnQLExtBegin( int clientNum ) {
	gclient_t *client = G_FnQLExtClient( clientNum );

	if ( !client || !G_FnQLExtensions( clientNum ) ) {
		return;
	}
	client->pers.fnqlExtConfirmed = qfalse;
	G_FnQLExtSendHello( clientNum );
}

/*
================
G_FnQLExtCommand

Claims the extension client commands of a negotiated client.  For anyone
else it claims nothing, so the command takes the retail path (an unknown
command there, as on a retail server).
================
*/
qboolean G_FnQLExtCommand( gentity_t *ent, const char *cmd ) {
	char arg[ MAX_TOKEN_CHARS ];
	int clientNum;
	unsigned extensions;

	if ( !ent || !ent->client || Q_stricmp( cmd, FNQL_EXT_CMD_ACK ) ) {
		return qfalse;
	}
	clientNum = (int)( ent - g_entities );
	extensions = G_FnQLExtensions( clientNum );
	if ( !extensions ) {
		return qfalse;
	}
	trap_Argv( 1, arg, sizeof( arg ) );
	if ( BG_FnQLExtParseProtocol( arg ) != FNQL_EXT_PROTOCOL ) {
		return qtrue;	// a channel command, but not one this server speaks
	}
	trap_Argv( 2, arg, sizeof( arg ) );
	if ( BG_FnQLExtParseMask( arg ) == extensions && !ent->client->pers.fnqlExtConfirmed ) {
		ent->client->pers.fnqlExtConfirmed = qtrue;
		G_Printf( "FnQL extensions confirmed for client %i (mask %i)\n", clientNum, extensions );
	}
	return qtrue;
}
