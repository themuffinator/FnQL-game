/*
===========================================================================
FnQL-game -- clean-room Quake Live compatible game modules.

This file is part of FnQL-game and is free software; you can redistribute
it and/or modify it under the terms of the GNU General Public License as
published by the Free Software Foundation; either version 2 of the License,
or (at your option) any later version.
===========================================================================
*/

// FnQL extension channels (code/game/g_fnql_ext.c, bg_fnql_ext.h): only a
// client that answered fnql_game is ever sent an extension command, and
// retail clients, bots and free slots never are.

#include "../code/game/g_local.h"
#include "../code/game/bg_fnql_ext.h"

#define CHECK(x) do { if (!(x)) { fprintf(stderr, "%s:%d: %s\n", __FILE__, __LINE__, #x); exit(1); } } while (0)

level_locals_t level;
gentity_t g_entities[MAX_GENTITIES];
static gclient_t clients[4];

static int sends;
static int lastClient = -99;
static char lastText[1024];
static const char *argv[4];

void trap_SendServerCommand( int clientNum, const char *text ) {
	CHECK( clientNum >= 0 && clientNum < 4 );	// never -1: channels are per client
	lastClient = clientNum;
	Q_strncpyz( lastText, text, sizeof( lastText ) );
	sends++;
}

void trap_Argv( int n, char *buffer, int bufferLength ) {
	Q_strncpyz( buffer, ( n >= 0 && n < 4 && argv[n] ) ? argv[n] : "", bufferLength );
}

void QDECL G_Printf( const char *fmt, ... ) {
	(void)fmt;
}

void QDECL Com_Error( int level, const char *error, ... ) {
	(void)level;
	fprintf( stderr, "Com_Error: %s\n", error );
	exit( 1 );
}

void QDECL Com_Printf( const char *msg, ... ) {
	(void)msg;
}

static void Reset( void ) {
	memset( clients, 0, sizeof( clients ) );
	memset( g_entities, 0, sizeof( g_entities ) );
	level.clients = clients;
	level.maxclients = 4;
	sends = 0;
	lastClient = -99;
	lastText[0] = '\0';
}

static void TestParsers( void ) {
	CHECK( BG_FnQLExtParseProtocol( "1" ) == 1 );
	CHECK( BG_FnQLExtParseProtocol( "12" ) == 12 );
	CHECK( BG_FnQLExtParseProtocol( "" ) == 0 );
	CHECK( BG_FnQLExtParseProtocol( NULL ) == 0 );
	CHECK( BG_FnQLExtParseProtocol( "1x" ) == 0 );
	CHECK( BG_FnQLExtParseProtocol( "-1" ) == 0 );
	CHECK( BG_FnQLExtParseProtocol( "99999" ) == 0 );
	CHECK( BG_FnQLExtParseMask( "0x1" ) == 1u );
	CHECK( BG_FnQLExtParseMask( "0xff" ) == 255u );
	CHECK( BG_FnQLExtParseMask( "3" ) == 3u );
	CHECK( BG_FnQLExtParseMask( "0x" ) == 0u );
	CHECK( BG_FnQLExtParseMask( "0x1g" ) == 0u );
	CHECK( BG_FnQLExtParseMask( "0x123456789" ) == 0u );
}

static void TestRetailClientsNeverGetExtensions( void ) {
	Reset();
	clients[0].pers.connected = CON_CONNECTED;
	G_FnQLExtUserinfo( 0, "\\name\\retail\\model\\sarge" );
	CHECK( G_FnQLExtensions( 0 ) == 0 );
	G_FnQLExtBegin( 0 );
	CHECK( !G_FnQLExtSend( 0, FNQL_EXT_HELLO, FNQL_EXT_CMD_HELLO " 1 0x1 x" ) );
	CHECK( G_FnQLExtBroadcast( FNQL_EXT_HELLO, "x" ) == 0 );
	CHECK( sends == 0 );
	// a malformed answer is no answer
	G_FnQLExtUserinfo( 0, "\\name\\odd\\fnql_cg\\yes" );
	CHECK( G_FnQLExtensions( 0 ) == 0 && sends == 0 );
	// the ack of a client that did not negotiate takes the retail path
	argv[0] = FNQL_EXT_CMD_ACK; argv[1] = "1"; argv[2] = "0x1";
	CHECK( !G_FnQLExtCommand( &g_entities[0], FNQL_EXT_CMD_ACK ) );
}

static void TestBotsNeverNegotiate( void ) {
	Reset();
	g_entities[1].client = &clients[1];
	g_entities[1].r.svFlags = SVF_BOT;
	clients[1].pers.connected = CON_CONNECTED;
	G_FnQLExtUserinfo( 1, "\\name\\bot\\fnql_cg\\1" );
	CHECK( G_FnQLExtensions( 1 ) == 0 );
	G_FnQLExtBegin( 1 );
	CHECK( sends == 0 );
}

static void TestHandshake( void ) {
	Reset();
	g_entities[2].client = &clients[2];
	clients[2].pers.connected = CON_CONNECTING;
	// answer during connect: the hello waits for ClientBegin
	G_FnQLExtUserinfo( 2, "\\name\\fnql\\fnql_cg\\1" );
	CHECK( G_FnQLExtensions( 2 ) == FNQL_EXT_SUPPORTED );
	CHECK( sends == 0 );
	clients[2].pers.connected = CON_CONNECTED;
	G_FnQLExtBegin( 2 );
	CHECK( sends == 1 && lastClient == 2 );
	CHECK( !strncmp( lastText, FNQL_EXT_CMD_HELLO " 1 1 ", strlen( FNQL_EXT_CMD_HELLO " 1 1 " ) ) );
	CHECK( strlen( lastText ) > strlen( FNQL_EXT_CMD_HELLO " 1 1 " ) );	// the server version follows
	// an unchanged answer sends nothing more
	G_FnQLExtUserinfo( 2, "\\name\\renamed\\fnql_cg\\1" );
	CHECK( sends == 1 );
	// a newer protocol offer is answered at ours
	G_FnQLExtUserinfo( 2, "\\fnql_cg\\7" );
	CHECK( G_FnQLExtensions( 2 ) == FNQL_EXT_SUPPORTED && sends == 1 );
	// the ack confirms only the exact enabled set
	argv[0] = FNQL_EXT_CMD_ACK; argv[1] = "1"; argv[2] = "0x2";
	CHECK( G_FnQLExtCommand( &g_entities[2], FNQL_EXT_CMD_ACK ) );
	CHECK( !clients[2].pers.fnqlExtConfirmed );
	argv[2] = "0x1";
	CHECK( G_FnQLExtCommand( &g_entities[2], FNQL_EXT_CMD_ACK ) );
	CHECK( clients[2].pers.fnqlExtConfirmed );
	CHECK( !G_FnQLExtCommand( &g_entities[2], "say" ) );
	// other clients stay off the channel
	clients[3].pers.connected = CON_CONNECTED;
	CHECK( G_FnQLExtBroadcast( FNQL_EXT_HELLO, "fnql_test" ) == 1 && lastClient == 2 );
	// withdrawing the answer closes the channel
	G_FnQLExtUserinfo( 2, "\\name\\fnql" );
	CHECK( G_FnQLExtensions( 2 ) == 0 && !clients[2].pers.fnqlExtConfirmed );
	CHECK( !G_FnQLExtSend( 2, FNQL_EXT_HELLO, "fnql_test" ) );
	// answering while in the game sends the hello at once
	sends = 0;
	G_FnQLExtUserinfo( 2, "\\fnql_cg\\1" );
	CHECK( sends == 1 && lastClient == 2 );
	// a free slot has no channel
	clients[2].pers.connected = CON_DISCONNECTED;
	CHECK( G_FnQLExtensions( 2 ) == 0 );
	CHECK( G_FnQLExtensions( -1 ) == 0 && G_FnQLExtensions( 4 ) == 0 );
}

int main( void ) {
	TestParsers();
	TestRetailClientsNeverGetExtensions();
	TestBotsNeverNegotiate();
	TestHandshake();
	printf( "game_fnql_ext: ok\n" );
	return 0;
}
