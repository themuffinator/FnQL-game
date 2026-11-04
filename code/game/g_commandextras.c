/* FnQL-game, Copyright (C) 2026 contributors. GPL-2.0-or-later. */
#include "g_local.h"
#include <limits.h>

/* Retail 10041cc0: only bots may send the timed training-chat packet. */
void Cmd_BotSay_f( gentity_t *ent ) {
	char argument[MAX_TOKEN_CHARS], name[64], text[MAX_SAY_TEXT];
	unsigned long long target, seconds;
	if ( !(ent->r.svFlags & SVF_BOT) || trap_Argc() < 3 ) return;
	trap_Argv(1, argument, sizeof(argument));
	if ( !G_ParseAccount(argument, &target) || target >= (unsigned)level.maxclients
		|| !g_entities[(int)target].inuse || !g_entities[(int)target].client ) return;
	trap_Argv(2, argument, sizeof(argument));
	if ( !G_ParseAccount(argument, &seconds) || seconds > INT_MAX ) return;
	Q_strncpyz(text, ConcatArgs(3), sizeof(text));
	Com_sprintf(name, sizeof(name), "%s^7", ent->client->pers.netname);
	G_LogPrintf("botSay: %s for %i seconds to client %i\n", text, (int)seconds, (int)target);
	trap_SendServerCommand((int)target, va("bchat \"%s^2%s\" %i", name, text, (int)seconds));
}

/* Retail 10045770: case-sensitive selectors cycle from the current follow
 * slot. They never move a playing client to the spectator team. */
void Cmd_Spec_f( gentity_t *ent ) {
	char selector[MAX_TOKEN_CHARS];
	int slot = ent->client->sess.spectatorClient, visited, powerup;
	if ( trap_Argc() != 2 ) {
		G_Printf("SPEC command error: Not enough arguments...\n");
		return;
	}
	trap_Argv(1, selector, sizeof(selector));
	if ( slot < 0 || slot >= level.maxclients ) slot = 0;
	for ( visited = 0; visited < level.maxclients; ++visited ) {
		gclient_t *candidate;
		qboolean match = qfalse;
		slot = (slot + 1) % level.maxclients;
		candidate = &level.clients[slot];
		if ( candidate->pers.connected != CON_CONNECTED || candidate->ps.pm_type == PM_SPECTATOR
			|| (candidate->ps.pm_flags & PMF_FOLLOW) ) continue;
		if ( ent->client->sess.sessionTeam != TEAM_SPECTATOR && g_gametype.integer >= GT_TEAM
			&& ent->client->sess.sessionTeam != candidate->sess.sessionTeam ) continue;
		if ( !strcmp(selector, "fc") )
			match = candidate->ps.powerups[PW_REDFLAG] || candidate->ps.powerups[PW_BLUEFLAG] || candidate->ps.powerups[PW_NEUTRALFLAG];
		else if ( !strcmp(selector, "red") || !strcmp(selector, "r") ) match = candidate->sess.sessionTeam == TEAM_RED;
		else if ( !strcmp(selector, "blue") || !strcmp(selector, "b") ) match = candidate->sess.sessionTeam == TEAM_BLUE;
		else if ( !strcmp(selector, "pu") ) {
			for ( powerup = PW_QUAD; powerup <= PW_INVULNERABILITY; ++powerup )
				if ( candidate->ps.powerups[powerup] ) match = qtrue;
		}
		if ( !match ) continue;
		ent->client->sess.spectatorClient = slot;
		ent->client->sess.spectatorState = SPECTATOR_FOLLOW;
		return;
	}
}
