/* FnQL-game, GPL-2.0-or-later. Retail rewind behavior and safety boundaries. */
#include "../code/game/g_local.h"
#define CHECK(x) do { if (!(x)) { fprintf(stderr, "%s:%d: %s\n", __FILE__, __LINE__, #x); exit(1); } } while (0)
level_locals_t level;
gentity_t g_entities[MAX_GENTITIES];
vmCvar_t g_lagHaxHistory, g_lagHaxMs, g_unlagged;
static gclient_t clients[2];
static int links, warnings;
void trap_LinkEntity(gentity_t *ent) { ent->r.linked = qtrue; links++; }
void trap_UnlinkEntity(gentity_t *ent) { ent->r.linked = qfalse; }
void QDECL G_Printf(const char *format, ...) { (void)format; warnings++; }
static void Setup(int depth, int window) {
	int i;
	memset(&level, 0, sizeof(level));
	memset(g_entities, 0, sizeof(g_entities));
	memset(clients, 0, sizeof(clients));
	level.maxclients = 2;
	for (i = 0; i < 2; ++i) {
		g_entities[i].client = &clients[i];
		g_entities[i].inuse = qtrue;
		g_entities[i].s.eType = ET_PLAYER;
		clients[i].pers.connected = CON_CONNECTED;
	}
	g_entities[0].s.weapon = WP_RAILGUN;
	g_lagHaxHistory.integer = depth;
	g_lagHaxMs.integer = window;
	g_unlagged.integer = 1;
	links = warnings = 0;
	G_InitLagHax();
}
static void Sample(int time, float x, float height) {
	level.time = time;
	g_entities[1].s.pos.trBase[0] = x;
	g_entities[1].r.maxs[2] = height;
	G_StoreLagHax(&g_entities[1]);
}
static void Shoot(int now, int command) {
	level.time = now;
	clients[0].ps.commandTime = command;
	g_entities[1].r.currentOrigin[0] = 99;
	g_entities[1].r.maxs[2] = 32;
	G_BeginLagHax(&g_entities[0]);
}
int main(void) {
	Setup(4, 80);
	Sample(100, 0, 32); Sample(125, 10, 16); Sample(150, 20, 32);
	Shoot(170, 120);
	CHECK(g_entities[1].r.currentOrigin[0] == 8);
	CHECK(g_entities[1].r.maxs[2] == 16); // newer stance, not interpolated
	G_EndLagHax(); CHECK(g_entities[1].r.currentOrigin[0] == 99);
	CHECK(g_entities[1].r.maxs[2] == 32);
	links = 0; G_EndLagHax(); CHECK(!links); // restoration is consumed once
	g_lagHaxMs.integer = 20;
	Shoot(170, 120); CHECK(g_entities[1].r.currentOrigin[0] == 20); G_EndLagHax();
	Shoot(170, 160); CHECK(g_entities[1].r.currentOrigin[0] == 99); G_EndLagHax();
	G_ResetLagHax(&g_entities[1]);
	Shoot(170, 120); CHECK(g_entities[1].r.currentOrigin[0] == 99); G_EndLagHax();
	Setup(2, 1000);
	Sample(100, 0, 32); Sample(125, 10, 16); Sample(150, 20, 32);
	Shoot(170, 1); CHECK(g_entities[1].r.currentOrigin[0] == 10); G_EndLagHax();
	Setup(1, 80); Sample(150, 20, 16);
	Shoot(170, 100); CHECK(g_entities[1].r.currentOrigin[0] == 20); G_EndLagHax();
	g_entities[0].r.svFlags = SVF_BOT;
	Shoot(170, 100); CHECK(g_entities[1].r.currentOrigin[0] == 99); G_EndLagHax();
	g_entities[0].r.svFlags = 0;
	g_entities[0].s.weapon = WP_ROCKET_LAUNCHER;
	Shoot(170, 100); CHECK(g_entities[1].r.currentOrigin[0] == 99); G_EndLagHax();
	g_entities[0].s.weapon = WP_RAILGUN;
	g_unlagged.integer = 2;
	Shoot(170, 100); CHECK(g_entities[1].r.currentOrigin[0] == 99); G_EndLagHax();
	Setup(0, 80); Sample(150, 20, 16);
	G_ResetLagHax(&g_entities[1]); // retail setviewpos crashes here; ours is safe
	Shoot(170, 100); CHECK(g_entities[1].r.currentOrigin[0] == 99); G_EndLagHax();
	Setup(4, 0); Sample(150, 20, 16);
	Shoot(170, 100); CHECK(g_entities[1].r.currentOrigin[0] == 99); G_EndLagHax();
	Setup(INT_MAX, 80); CHECK(warnings == 1);
	Sample(150, 20, 16); Shoot(170, 150);
	CHECK(g_entities[1].r.currentOrigin[0] == 20); G_EndLagHax();
	puts("game laghax tests passed");
	return 0;
}
