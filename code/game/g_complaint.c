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
#include <limits.h>

// Retail 0x1004756e: team kills prompt outside warmup, excluding bots and
// telefrags. Nonfatal damage never prompts: 0x10049363..6f sets CL to 0/1,
// tests it against 8, then always skips the damage-threshold path. Keep
// g_complaintDamageThreshold registered without activating that dead path.
void G_ComplaintKill( gentity_t *victim, gentity_t *attacker, int mod ) {
	clientPersistant_t *from, *to;
	if ( !victim || !attacker || !victim->client || !attacker->client || victim == attacker
		|| !OnSameTeam(victim, attacker) || !g_complaintLimit.integer || level.warmupTime
		|| mod == MOD_TELEFRAG || (attacker->r.svFlags & SVF_BOT) ) return;
	if ( !G_IsTeamGametype(g_gametype.integer) ) return;
	from = &attacker->client->pers;
	to = &victim->client->pers;
	if ( from->privileges == 2 ) {
		trap_SendServerCommand(victim->s.number, "complaint -4");
		return;
	}
	trap_SendServerCommand(victim->s.number, va("complaint %i", attacker->s.number));
	to->complaintClient = attacker->s.clientNum;
	to->complaintDeadline = level.time + 15500;
}

// Retail 0x1004429f runs this before processing the same normal vote.
void G_ComplaintVote( gentity_t *victim ) {
	clientPersistant_t *to = &victim->client->pers, *from;
	int target = to->complaintClient;
	char answer[64] = {0};
	if ( to->complaintDeadline <= level.time || level.warmupTime || !g_complaintLimit.integer
		|| target < 0 || target >= level.maxclients ) goto done;
	from = &level.clients[target].pers;
	if ( from->connected != CON_CONNECTED ) goto done;
	if ( from->privileges == 2 ) {
		trap_SendServerCommand(victim->s.number, "complaint -3");
		return;
	}
	trap_Argv(1, answer, sizeof(answer));
	// Retail's comparisons deliberately documented at 0x10043b85..96:
	// lower-case y is checked in byte 0, uppercase Y and 1 in byte 1.
	if ( answer[0] == 'y' || answer[1] == 'Y' || answer[1] == '1' ) {
		if ( from->complaintCount < INT_MAX ) ++from->complaintCount;
		if ( !from->localClient && from->complaintCount >= g_complaintLimit.integer ) {
			if ( !G_SetAccess(trap_GetSteamId64(target), -1, qtrue) )
				G_Printf("Could not retain temporary complaint ban for client %i\n", target);
			trap_DropClient(target, "kicked after too many complaints.");
		} else {
			trap_SendServerCommand(target, va("print \"^1Warning^7: Complaint filed against you by %s.\n\"", to->netname));
		}
		trap_SendServerCommand(victim->s.number, "complaint -1");
	} else {
		trap_SendServerCommand(target, "print\"No complaint filed against you.\n\"");
		trap_SendServerCommand(victim->s.number, "complaint -2");
	}
done:
	to->complaintClient = to->complaintDeadline = -1;
}

void G_ComplaintDisconnect( int clientNum ) {
	int i;
	// Retail 0x1003c943 clears pending prompts before disconnect effects.
	for ( i = 0; i < level.maxclients; ++i ) {
		clientPersistant_t *to = &level.clients[i].pers;
		if ( to->connected != CON_DISCONNECTED && to->complaintClient == clientNum ) {
			to->complaintClient = -1;
			to->complaintDeadline = 0;
			trap_SendServerCommand(i, "complaint -2");
		}
	}
}
