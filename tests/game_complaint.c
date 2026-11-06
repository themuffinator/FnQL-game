/* FnQL-game, Copyright (C) 2026 contributors. GPL-2.0-or-later. */
#include "../code/game/g_local.h"
#include <limits.h>
#define CHECK(x) do { if (!(x)) { fprintf(stderr, "%s:%d: %s\n", __FILE__, __LINE__, #x); exit(1); } } while (0)
level_locals_t level;
gentity_t g_entities[MAX_GENTITIES];
vmCvar_t g_complaintLimit, g_gametype;
static gclient_t clients[3];
static const char *answer;
static int replies, dropped, banned, warnings;
static qboolean denyAccess;
static char messages[8][256];
static int targets[8];

char *QDECL va(const char *fmt, ...) {
	static char out[1024]; va_list ap;
	va_start(ap,fmt); vsnprintf(out,sizeof(out),fmt,ap); va_end(ap); return out;
}
void trap_SendServerCommand(int target, const char *text) {
	CHECK(replies < 8); targets[replies] = target;
	snprintf(messages[replies++], 256, "%s", text);
}
void trap_Argv(int n, char *out, int size) { CHECK(n == 1); snprintf(out, size, "%s", answer); }
void trap_DropClient(int target, const char *reason) {
	CHECK(target == 0 && !strcmp(reason, "kicked after too many complaints."));
	++dropped; G_ComplaintDisconnect(target);
}
unsigned long long trap_GetSteamId64(int target) { CHECK(target == 0); return 76561198000000000ULL; }
qboolean G_SetAccess(unsigned long long id, int privilege, qboolean temporary) {
	CHECK(id == 76561198000000000ULL && privilege == -1 && temporary);
	++banned; return !denyAccess;
}
void QDECL G_Printf(const char *fmt, ...) { CHECK(strstr(fmt,"temporary complaint ban")); ++warnings; }
qboolean OnSameTeam(gentity_t *a, gentity_t *b) { return a->client->sess.sessionTeam == b->client->sess.sessionTeam; }
qboolean G_IsTeamGametype(int mode) { return mode == GT_TEAM; }

static void Reset(void) {
	int i;
	memset(clients, 0, sizeof(clients)); memset(g_entities, 0, sizeof(g_entities));
	memset(&level, 0, sizeof(level));
	level.clients = clients; level.maxclients = 3; level.time = 20000;
	g_gametype.integer = GT_TEAM; g_complaintLimit.integer = 2;
	replies = dropped = banned = warnings = 0; denyAccess = qfalse;
	answer = "yes";
	for (i = 0; i < 3; ++i) {
		g_entities[i].client = &clients[i]; g_entities[i].s.number = g_entities[i].s.clientNum = i;
		clients[i].sess.sessionTeam = TEAM_RED; clients[i].pers.connected = CON_CONNECTED;
		clients[i].pers.complaintClient = clients[i].pers.complaintDeadline = -1;
	}
	strcpy(clients[1].pers.netname, "Victim");
}
static void Kill(void) { replies = 0; G_ComplaintKill(&g_entities[1], &g_entities[0], MOD_RAILGUN); }
static void Vote(const char *value) { replies = 0; answer = value; G_ComplaintVote(&g_entities[1]); }

int main(void) {
	int i;
	const char *no[] = {"no", "1", "Y", "YES", "", "0"};
	const char *yes[] = {"yes", "y", "x1", "xY"};
	Reset(); Kill();
	CHECK(replies == 1 && targets[0] == 1 && !strcmp(messages[0], "complaint 0"));
	CHECK(clients[1].pers.complaintDeadline == 35500);
	Vote("yes");
	CHECK(clients[0].pers.complaintCount == 1 && replies == 2 && !dropped);
	CHECK(!strcmp(messages[0], "print \"^1Warning^7: Complaint filed against you by Victim.\n\""));
	Kill(); Vote("yes");
	CHECK(dropped == 1 && banned == 1 && replies == 2);
	CHECK(!strcmp(messages[0], "complaint -2") && !strcmp(messages[1], "complaint -1"));
	for (i = 0; i < ARRAY_LEN(no); ++i) { Reset(); Kill(); Vote(no[i]); CHECK(!clients[0].pers.complaintCount && !strcmp(messages[1], "complaint -2")); }
	for (i = 0; i < ARRAY_LEN(yes); ++i) { Reset(); Kill(); Vote(yes[i]); CHECK(clients[0].pers.complaintCount == 1 && !strcmp(messages[1], "complaint -1")); }
	Reset(); clients[0].pers.privileges = 2; Kill();
	CHECK(!strcmp(messages[0], "complaint -4") && clients[1].pers.complaintClient == -1);
	Reset(); Kill(); clients[0].pers.privileges = 2; Vote("yes");
	CHECK(!strcmp(messages[0], "complaint -3") && clients[1].pers.complaintClient == 0);
	Reset(); Kill(); level.time = 35500; Vote("yes"); CHECK(!replies && clients[1].pers.complaintClient == -1);
	Reset(); Kill(); clients[0].pers.connected = CON_DISCONNECTED; Vote("yes"); CHECK(!replies && clients[1].pers.complaintClient == -1);
	Reset(); Kill(); clients[1].pers.complaintClient = INT_MAX; Vote("yes"); CHECK(!replies);
	Reset(); level.warmupTime = -1; Kill(); CHECK(!replies);
	Reset(); g_complaintLimit.integer = 0; Kill(); CHECK(!replies);
	Reset(); g_entities[0].r.svFlags = SVF_BOT; Kill(); CHECK(!replies);
	Reset(); clients[0].sess.sessionTeam = TEAM_BLUE; Kill(); CHECK(!replies);
	Reset(); g_gametype.integer = GT_FFA; Kill(); CHECK(!replies);
	Reset(); G_ComplaintKill(&g_entities[1], &g_entities[0], MOD_TELEFRAG); CHECK(!replies);
	Reset(); G_ComplaintKill(&g_entities[1], &g_entities[1], MOD_SUICIDE); CHECK(!replies);
	Reset(); g_complaintLimit.integer = 1; clients[0].pers.localClient = qtrue; Kill(); Vote("yes"); CHECK(!dropped && !banned);
	Reset(); clients[0].pers.complaintCount = INT_MAX; clients[0].pers.localClient = qtrue;
	Kill(); Vote("yes"); CHECK(clients[0].pers.complaintCount == INT_MAX && !dropped);
	Reset(); g_complaintLimit.integer = 1; denyAccess = qtrue; Kill(); Vote("yes"); CHECK(dropped && warnings == 1);
	puts("complaint policy, response, expiry, disconnect and ban-failure checks passed");
	return 0;
}
