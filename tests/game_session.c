/* FnQL-game, Copyright (C) 2026 contributors. GPL-2.0-or-later. */
#include "../code/game/g_local.h"
#define CHECK(x) do { if (!(x)) { fprintf(stderr, "%s:%d: %s\n", __FILE__, __LINE__, #x); exit(1); } } while (0)

level_locals_t level;
vmCvar_t g_gametype, g_teamAutoJoin;
static gclient_t clients[2];
static char record[MAX_STRING_CHARS];
static char world[32];
static int writes;

char *QDECL va(const char *fmt, ...) {
	static char buffers[4][MAX_STRING_CHARS];
	static int next;
	char *out = buffers[next++ & 3];
	va_list ap;
	va_start(ap, fmt); vsnprintf(out, MAX_STRING_CHARS, fmt, ap); va_end(ap);
	return out;
}
void trap_Cvar_Set(const char *name, const char *value) {
	CHECK(!strcmp(name, "session") || !strcmp(name, "session1"));
	if (!strcmp(name, "session")) snprintf(world, sizeof(world), "%s", value);
	else { snprintf(record, sizeof(record), "%s", value); ++writes; }
}
void trap_Cvar_VariableStringBuffer(const char *name, char *out, int size) {
	CHECK(!strcmp(name, "session") || !strcmp(name, "session1"));
	snprintf(out, size, "%s", !strcmp(name, "session") ? world : record);
}
void QDECL G_Printf(const char *fmt, ...) { (void)fmt; }
team_t PickTeam(int ignore) { CHECK(ignore == -1); return TEAM_BLUE; }
void BroadcastTeamChange(gclient_t *client, team_t oldTeam) { CHECK(client == &clients[1] && oldTeam == -1); }
int trap_RealTime(qtime_t *time) { CHECK(!time); return 1770000000; }

static void Read(const char *value) {
	snprintf(record, sizeof(record), "%s", value);
	G_ReadClientSessionData(&clients[1]);
}

int main(void) {
	gclient_t *c = &clients[1];
	const char *retail = "1 1770000000 2 -2 7 11 12 1 2 1 3 1 12345";
	const char *bad[] = { "", "1 2", "1 2 3 rubbish", "1 2 1 0 14 0 0 0 0 0 0 0 2147483648",
		"1 2 1 0 14 0 0 0 0 0 0 0 -2147483649", "1 2 1 0 14 0 0 0 0 0 0 0 0 extra" };
	int i;
	level.clients = clients; level.maxclients = 2;
	Read(retail);
	CHECK(c->sess.sessionTeam == TEAM_RED && c->sess.spectatorState == SPECTATOR_FOLLOW);
	CHECK(c->sess.spectatorClient == -2 && c->sess.loadoutWeapon == WP_RAILGUN);
	CHECK(c->sess.wins == 11 && c->sess.losses == 12 && c->sess.teamLeader);
	CHECK(c->pers.privileges == 2 && c->sess.spectateOnly == 1 && c->sess.queuePosition == 3);
	CHECK(c->sess.muted && c->sess.sessionReservedTail == 12345);
	c->sess.queueChanged = qtrue;
	G_WriteClientSessionData(c);
	CHECK(!strcmp(record, retail));
	memset(&c->sess, 0, sizeof(c->sess));
	G_ReadClientSessionData(c);
	CHECK(c->sess.sessionReservedTail == 12345 && !c->sess.queueChanged);

	Read("1 1770000000 2 -2 11 12 1 7 1 3 1 2");
	G_WriteClientSessionData(c);
	CHECK(!strcmp(record, "1 1770000000 2 -2 7 11 12 1 2 1 3 1 0"));
	Read("1 1770000000 2 -2 11 12 1 7 1 3");
	CHECK(c->sess.loadoutWeapon == 7 && c->sess.queuePosition == 3 && !c->sess.muted);
	Read("1 1770000000 2 -2 11 12 1");
	CHECK(c->sess.wins == 11 && !c->sess.loadoutWeapon && !c->sess.queuePosition);
	Read("99 1770000000 -1 -2 2147483647 11 12 1 2 1 3 1 -2147483648");
	CHECK(c->sess.sessionTeam == TEAM_SPECTATOR && c->sess.spectatorState == SPECTATOR_FREE);
	CHECK(c->sess.loadoutWeapon == 0 && c->sess.sessionReservedTail == (-2147483647 - 1));
	Read("\t1 1770000000 02 -2 0x7 11 12 1 2 1 3 1 0  \n");
	CHECK(c->sess.loadoutWeapon == WP_RAILGUN && c->sess.spectatorState == SPECTATOR_FOLLOW);
	for (i = 0; i < ARRAY_LEN(bad); ++i) {
		Read(retail); Read(bad[i]);
		CHECK(c->sess.sessionTeam == TEAM_SPECTATOR && c->sess.spectatorState == SPECTATOR_FREE);
		CHECK(!c->sess.muted && !c->sess.sessionReservedTail && c->pers.privileges == 2);
	}
	memset(c, 0, sizeof(*c));
	g_gametype.integer = GT_TOURNAMENT;
	G_InitSessionData(c, "", qfalse);
	CHECK(c->sess.sessionTeam == TEAM_SPECTATOR && c->sess.spectateOnly && c->sess.spectatorTime == 1770000000);
	g_gametype.integer = GT_TEAM; g_teamAutoJoin.integer = 1;
	G_InitSessionData(c, "", qtrue);
	CHECK(c->sess.sessionTeam == TEAM_BLUE && !c->sess.spectateOnly);
	c->pers.connected = CON_CONNECTED;
	writes = 0; G_WriteSessionData();
	CHECK(writes == 1 && !strcmp(world, "3"));
	G_InitWorldSession(); CHECK(!level.newSession);
	g_gametype.integer = GT_CTF;
	G_InitWorldSession(); CHECK(level.newSession);
	G_ClearClientSessionData(c); CHECK(!record[0]);
	puts("session format, migration, restart and malformed-record checks passed");
	return 0;
}
