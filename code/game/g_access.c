/*
===========================================================================
FnQL-game -- clean-room Quake Live compatible game modules.
Copyright (C) 2026 FnQL-game contributors.

This program is free software; you can redistribute it and/or modify it
under the terms of the GNU General Public License as published by the Free
Software Foundation; either version 2, or (at your option) any later version.
===========================================================================
*/
#include "g_local.h"
#include <stdlib.h>

/* Retail qagamex86 10032460..10032b5b: engine SteamIDs, id|ban/mod/admin
 * records, numeric ordering, 20 rows per page, temporary entries not saved.
 * A sorted, bounded array replaces the retail implementation's tree. */
typedef struct {
	unsigned long long id;
	int privilege;
	qboolean temporary;
} accessEntry_t;

#define ACCESS_LIMIT 16384
static accessEntry_t *accessEntries;
static int accessCount, accessCapacity;
static qboolean accessDirty;
static char accessPath[MAX_QPATH];

qboolean G_ParseAccount( const char *text, unsigned long long *id ) {
	unsigned long long value = 0;
	if ( !text || !*text ) return qfalse;
	for ( ; *text; ++text ) {
		unsigned int digit = (unsigned char)*text - '0';
		if ( digit > 9 || value > (~0ULL - digit) / 10 ) return qfalse;
		value = value * 10 + digit;
	}
	*id = value;
	return qtrue;
}

static int AccessPosition( unsigned long long id ) {
	int low = 0, high = accessCount;
	while ( low < high ) {
		int mid = low + (high - low) / 2;
		if ( accessEntries[mid].id < id ) low = mid + 1;
		else high = mid;
	}
	return low;
}

qboolean G_SetAccess( unsigned long long id, int privilege, qboolean temporary ) {
	int at = AccessPosition( id );
	if ( privilege < -1 || privilege > 2 ) return qfalse;
	if ( at == accessCount || accessEntries[at].id != id ) {
		accessEntry_t *grown;
		if ( accessCount == ACCESS_LIMIT ) return qfalse;
		if ( accessCount == accessCapacity ) {
			int capacity = accessCapacity ? accessCapacity * 2 : 32;
			if ( capacity > ACCESS_LIMIT ) capacity = ACCESS_LIMIT;
			grown = realloc(accessEntries, capacity * sizeof(*grown));
			if ( !grown ) return qfalse;
			accessEntries = grown;
			accessCapacity = capacity;
		}
		grown = accessEntries;
		memmove( grown + at + 1, grown + at, (accessCount - at) * sizeof(*grown) );
		++accessCount;
	}
	accessEntries[at].id = id;
	accessEntries[at].privilege = privilege;
	accessEntries[at].temporary = temporary;
	accessDirty = qtrue;
	return qtrue;
}

void G_RemoveAccess( unsigned long long id ) {
	int at = AccessPosition( id );
	if ( at == accessCount || accessEntries[at].id != id ) return;
	--accessCount;
	memmove( accessEntries + at, accessEntries + at + 1,
		(accessCount - at) * sizeof(*accessEntries) );
	accessDirty = qtrue;
}

int G_ClientPrivilege( int clientNum ) {
	char info[MAX_INFO_STRING];
	unsigned long long id;
	int at;
	if ( clientNum < 0 || clientNum >= level.maxclients ) return 0;
	trap_GetUserinfo( clientNum, info, sizeof(info) );
	if ( !strcmp( Info_ValueForKey( info, "ip" ), "localhost" ) ) return 3;
	id = trap_GetSteamId64( clientNum );
	at = AccessPosition( id );
	return at < accessCount && accessEntries[at].id == id ? accessEntries[at].privilege : 0;
}

void G_LoadAccess( void ) {
	fileHandle_t file = FS_INVALID_HANDLE;
	char path[MAX_QPATH], *buffer, *line;
	int length;
	trap_Cvar_VariableStringBuffer( "g_accessFile", path, sizeof(path) );
	G_Printf( "initializing access list...\n" );
	length = trap_FS_FOpenFile( path, &file, FS_READ );
	if ( file == FS_INVALID_HANDLE ) {
		G_Printf( "^1file not found: %s\n", path );
		/* A failed reload retains the active policy and its save destination. */
		if ( !accessPath[0] ) Q_strncpyz( accessPath, path, sizeof(accessPath) );
		return;
	}
	if ( length < 0 || length > 1024 * 1024 ) {
		trap_FS_FCloseFile( file );
		G_Printf( "Access file is too large: %s\n", path );
		return;
	}
	buffer = malloc( length + 1 );
	if ( !buffer ) { trap_FS_FCloseFile( file ); return; }
	trap_FS_Read( buffer, length, file );
	trap_FS_FCloseFile( file );
	buffer[length] = 0;
	/* Reserve once, before replacing the live policy; allocation failure leaves
	 * the current list intact. No partially parsed privileged record is used. */
	{
		accessEntry_t *reserved = realloc( accessEntries, ACCESS_LIMIT * sizeof(*reserved) );
		if ( !reserved ) { free( buffer ); return; }
		accessEntries = reserved;
		accessCapacity = ACCESS_LIMIT;
	}
	accessCount = 0;
	Q_strncpyz( accessPath, path, sizeof(accessPath) );
	line = buffer;
	while ( line && *line ) {
		char *next = strchr( line, '\n' ), *separator, *end;
		unsigned long long id;
		int tier = 0, at;
		if ( next ) *next++ = 0;
		end = line + strlen(line);
		while ( end > line && (end[-1] == '\r' || end[-1] == ' ' || end[-1] == '\t') ) *--end = 0;
		if ( *line && *line != '#' ) {
			separator = strchr( line, '|' );
			if ( separator ) {
				*separator++ = 0;
				if ( !strcmp(separator, "ban") ) tier = -1;
				else if ( !strcmp(separator, "mod") ) tier = 1;
				else if ( !strcmp(separator, "admin") ) tier = 2;
			}
			if ( !tier || !G_ParseAccount(line, &id) || accessCount == ACCESS_LIMIT ) {
				G_Printf( "^1invalid admin access format, skipping: %s\n", line );
			} else {
				at = AccessPosition(id);
				if ( at == accessCount || accessEntries[at].id != id ) {
					memmove(accessEntries + at + 1, accessEntries + at, (accessCount - at) * sizeof(*accessEntries));
					++accessCount;
				}
				accessEntries[at].id = id;
				accessEntries[at].privilege = tier;
				accessEntries[at].temporary = qfalse;
			}
		}
		line = next;
	}
	free(buffer);
	accessDirty = qfalse;
	G_Printf( "loaded %i steam ids into the access list\n", accessCount );
}

void G_SaveAccess( void ) {
	fileHandle_t file = FS_INVALID_HANDLE;
	int i;
	if ( !accessDirty || !accessPath[0] ) return;
	trap_FS_FOpenFile( accessPath, &file, FS_WRITE );
	if ( file == FS_INVALID_HANDLE ) {
		G_Printf( "Unable to save access list: %s\n", accessPath );
		return;
	}
	for ( i = 0; i < accessCount; ++i ) {
		const accessEntry_t *entry = &accessEntries[i];
		char line[64];
		if ( entry->temporary || !entry->privilege ) continue;
		Com_sprintf( line, sizeof(line), "%s|%s\r\n", G_SteamIdString(entry->id),
			entry->privilege < 0 ? "ban" : entry->privilege == 1 ? "mod" : "admin" );
		trap_FS_Write( line, (int)strlen(line), file );
	}
	trap_FS_FCloseFile(file);
	accessDirty = qfalse;
}

void G_ShutdownAccess( void ) {
	G_SaveAccess();
	free(accessEntries);
	accessEntries = NULL;
	accessCount = accessCapacity = 0;
	accessDirty = qfalse;
	accessPath[0] = 0;
}

void G_ListAccess( gentity_t *ent, int page ) {
	static const char *labels[] = { "BAN   ", "      ", "MOD   ", "ADMIN " };
	int i, lastPage = accessCount / 20;
	if ( page < 0 ) page = 0;
	if ( page > lastPage ) page = lastPage;
	G_CommandReply( ent, va("print \"Access List: Page %i of %i\n\"", page + 1, lastPage + 1) );
	G_CommandReply( ent, "print \"=============================\n\"" );
	for ( i = page * 20; i < accessCount && i < (page + 1) * 20; ++i ) {
		const accessEntry_t *entry = &accessEntries[i];
		G_CommandReply( ent, va("print \"%s %s %s\n\"", G_SteamIdString(entry->id),
			labels[entry->privilege + 1], entry->temporary ? "TEMP" : "PERM") );
	}
}

void G_ReloadAccess( void ) {
	int i;
	G_LoadAccess();
	for ( i = 0; i < level.maxclients; ++i ) {
		if ( level.clients[i].pers.connected == CON_DISCONNECTED ) continue;
		level.clients[i].pers.privileges = G_ClientPrivilege(i);
		ClientUserinfoChanged(i);
	}
}
