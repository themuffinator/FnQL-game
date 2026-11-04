/*
===========================================================================
FnQL-game -- clean-room Quake Live compatible game modules.

This file is part of FnQL-game and is free software; you can redistribute
it and/or modify it under the terms of the GNU General Public License as
published by the Free Software Foundation; either version 2 of the License,
or (at your option) any later version.
===========================================================================
*/
#include "../code/game/g_local.h"
#include <limits.h>
#include <float.h>
#define CHECK(x) do { if (!(x)) { fprintf(stderr, "%s:%d: %s\n", __FILE__, __LINE__, #x); exit(1); } } while (0)
level_locals_t level;
gentity_t g_entities[MAX_GENTITIES];
vec3_t vec3_origin;
vmCvar_t g_gametype, g_domScoreRate, g_domCapTime, g_domNeutralFlag, g_domEnableContention;
vmCvar_t g_domTeammateCapScale, g_domDistressThreshold, g_spawnMinDistance;
static gclient_t clients[2];
static gentity_t event;
static int freed, randomValue;
int rand(void) { return randomValue; }
float Com_Clamp(float low, float high, float v) { return v < low ? low : v > high ? high : v; }
int Q_stricmp(const char *a, const char *b) { return strcmp(a,b); }
char *QDECL va(const char *format, ...) {
 static char b[1024]; va_list ap; va_start(ap,format); vsnprintf(b,sizeof(b),format,ap); va_end(ap); return b;
}
void QDECL G_Printf(const char *format, ...) { (void)format; }
void G_FreeEntity(gentity_t *ent) { ent->inuse=qfalse; freed++; }
void G_SetOrigin(gentity_t *ent, vec3_t origin) { VectorCopy(origin,ent->r.currentOrigin); VectorCopy(origin,ent->s.pos.trBase); }
qboolean G_SpawnInt(const char *key, const char *fallback, int *out) { *out=0; return qfalse; }
int G_SoundIndex(const char *name) { return 1; }
int G_ModelIndex(const char *name) { return 1; }
void trap_LinkEntity(gentity_t *ent) { ent->r.linked=qtrue; }
void trap_Trace(trace_t *tr, const vec3_t start, const vec3_t mins, const vec3_t maxs, const vec3_t end, int pass, int mask) {
 memset(tr,0,sizeof(*tr)); tr->fraction=1;
}
void trap_SetConfigstring(int index, const char *value) { (void)index; (void)value; }
qboolean trap_InPVS(const vec3_t a, const vec3_t b) { return qtrue; }
gentity_t *G_TempEntity(vec3_t origin, int type) { memset(&event,0,sizeof(event)); return &event; }
void AddScore(gentity_t *ent, vec3_t origin, int score) { ent->client->ps.persistant[PERS_SCORE]+=score; }
void CalculateRanks(void) { }
gentity_t *G_Find(gentity_t *from, int field, const char *name) {
 int i=from ? (int)(from-g_entities)+1 : 0;
 for (; i<level.num_entities; ++i) if (g_entities[i].inuse && g_entities[i].classname && !strcmp(g_entities[i].classname,name)) return &g_entities[i];
 return NULL;
}
int main(void) {
 int i; vec3_t origin,angles; gentity_t *point=&g_entities[2];
 level.maxclients=2; level.num_entities=40; level.time=1000; level.msec=25;
 g_gametype.integer=GT_DOMINATION; g_domScoreRate.integer=5; g_domCapTime.integer=5;
 g_domTeammateCapScale.value=0.5f;
 for (i=2;i<8;i++) { g_entities[i].inuse=qtrue; SP_team_dom_point(&g_entities[i]); }
 CHECK(level.domPointCount==5 && freed==1 && !g_entities[7].inuse);
 g_gametype.integer=GT_FFA; g_entities[8].inuse=qtrue; SP_team_dom_point(&g_entities[8]);
 CHECK(!g_entities[8].inuse && level.domPointCount==5); g_gametype.integer=GT_DOMINATION;
 point->think(point); CHECK(point->frameThink && point->nextthink==6000);
 for (i=0;i<2;i++) {
  g_entities[i].inuse=qtrue; g_entities[i].client=&clients[i]; g_entities[i].health=100;
  clients[i].pers.connected=CON_CONNECTED; clients[i].sess.sessionTeam=TEAM_RED;
 }
 // Malformed cap times never divide by zero; extreme scales stay bounded.
 g_domCapTime.integer=0; point->frameThink(point); CHECK(point->health==0);
 g_domCapTime.integer=-1; point->frameThink(point); CHECK(point->health==0);
 level.warmupTime=1; clients[0].ps.clientNum=7;
 g_domCapTime.integer=5; g_domTeammateCapScale.value=FLT_MAX; point->frameThink(point);
 CHECK(point->health==-1000 && point->s.modelindex2==TEAM_RED);
 CHECK(event.r.svFlags==(SVF_BROADCAST|SVF_NOTSINGLECLIENT) && event.r.singleClient==7);
 CHECK(clients[0].ps.persistant[PERS_CAPTURES]==1 && clients[1].ps.persistant[PERS_ASSIST_COUNT]==1);
 level.warmupTime=0;
 // Long timers saturate instead of wrapping into immediate repeated ticks.
 level.time=INT_MAX-10; g_domScoreRate.integer=INT_MAX; point->think(point);
 CHECK(point->nextthink==INT_MAX);
 // More than 25 map associations cannot overflow the spawn pool.
 point->classname="team_dom_point"; point->target="owned";
 for (i=9;i<40;i++) {
  g_entities[i].inuse=qtrue; g_entities[i].classname="info_player_deathmatch";
  g_entities[i].targetname="owned"; g_entities[i].r.currentOrigin[0]=1000.0f+i;
  g_entities[i].s.origin[0]=1000.0f+i;
 }
 randomValue=0; CHECK(G_DominationSpawn(&g_entities[0],origin,angles)==&g_entities[9]);
 randomValue=32766; CHECK(G_DominationSpawn(&g_entities[0],origin,angles)==&g_entities[33]);
 randomValue=32767; CHECK(G_DominationSpawn(&g_entities[0],origin,angles)==NULL);
 puts("Domination bounds OK"); return 0;
}
