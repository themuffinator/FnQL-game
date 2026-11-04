/* FnQL-game, Copyright (C) 2026 contributors. GPL-2.0-or-later. */
#include "../code/game/g_local.h"
#include <limits.h>
#define CHECK(x) do { if (!(x)) { fprintf(stderr, "%s:%d: %s\n", __FILE__, __LINE__, #x); exit(1); } } while (0)
level_locals_t level;
gentity_t g_entities[MAX_GENTITIES];
vmCvar_t g_gametype, g_cheats;
static gclient_t clients[2];
static gentity_t *lastEvent;
static int ranks, argc;
static const char *action;
static char message[2048], recordCount[32];
char *QDECL va(const char *fmt, ...) {
	static char buffers[8][2048]; static int n; char *b = buffers[n++ & 7]; va_list ap;
	va_start(ap,fmt); vsnprintf(b,2048,fmt,ap); va_end(ap); return b;
}
int QDECL Com_sprintf(char *b, int size, const char *fmt, ...) {
	va_list ap; int n; va_start(ap,fmt); n=vsnprintf(b,size,fmt,ap); va_end(ap); return n;
}
void Q_strncpyz(char *b,const char *s,int size) { snprintf(b,size,"%s",s); }
int Q_stricmp(const char *a,const char *b) { while (*a && tolower((unsigned char)*a)==tolower((unsigned char)*b)) { ++a; ++b; } return tolower((unsigned char)*a)-tolower((unsigned char)*b); }
void QDECL G_Printf(const char *fmt,...) { (void)fmt; }
void trap_SendServerCommand(int client,const char *text) { CHECK(client>=-1 && client<2); Q_strncpyz(message,text,sizeof(message)); }
void trap_SetConfigstring(int index,const char *text) { CHECK(index==CS_RACE_RECORDS); Q_strncpyz(recordCount,text,sizeof(recordCount)); }
void trap_Argv(int n,char *b,int size) { Q_strncpyz(b,n==1 && argc>1?action:"",size); }
void CalculateRanks(void) { ++ranks; }
void G_FreeEntity(gentity_t *ent) { ent->inuse=qfalse; }
gentity_t *G_Find(gentity_t *from,int offset,const char *match) {
	int i=from?(int)(from-g_entities)+1:0;
	for(;i<level.num_entities;++i) {
		char *value=*(char **)((char *)&g_entities[i]+offset);
		if(g_entities[i].inuse&&value&&!strcmp(value,match)) return &g_entities[i];
	}
	return NULL;
}
gentity_t *G_Spawn(void) {
	int i;
	for(i=level.maxclients;i<MAX_GENTITIES;++i) if(!g_entities[i].inuse) {
		memset(&g_entities[i],0,sizeof(g_entities[i]));g_entities[i].inuse=qtrue;
		if(i>=level.num_entities) level.num_entities=i+1;
		return &g_entities[i];
	}
	CHECK(0);return NULL;
}
void G_SetOrigin(gentity_t *ent,vec3_t origin) {
	VectorCopy(origin,ent->s.origin);VectorCopy(origin,ent->s.pos.trBase);VectorCopy(origin,ent->r.currentOrigin);
}
gentity_t *G_PickTarget(const char *target) { return G_Find(NULL, FOFS(targetname), target); }
gentity_t *G_TempEntity(vec3_t origin,int event) { gentity_t *e=G_Spawn(); G_SetOrigin(e,origin); e->s.eType=ET_EVENTS+event; lastEvent=e; return e; }
char *G_NewString(const char *text) { static char names[4096];static int used;char *out=names+used;used+=(int)strlen(text)+1;CHECK(used<=sizeof(names));strcpy(out,text);return out; }
int G_ModelIndex(const char *name) { CHECK(strstr(name,"models/flag3/"));return 1; }
void trap_LinkEntity(gentity_t *ent) { ent->r.linked=qtrue; }
void trap_Trace(trace_t *tr,const vec3_t start,const vec3_t mins,const vec3_t maxs,const vec3_t end,int pass,int mask) {
	(void)start;(void)mins;(void)maxs;(void)pass;(void)mask;memset(tr,0,sizeof(*tr));tr->fraction=0.5f;VectorCopy(end,tr->endpos);tr->endpos[2]=15;
}
char *vtos(const vec3_t v) { (void)v;return "(0 0 0)"; }
extern void SP_race_point(gentity_t *ent);


static gentity_t *point(char *name,char *target) {
	gentity_t *p=G_Spawn(); p->classname="race_point"; p->targetname=name; p->target=target;
	p->spawnflags=1; p->s.number=p-g_entities; SP_race_point(p); return p;
}
static void touch(gentity_t *p,int time) { clients[0].flagCarryTime=time; p->touch(p,&g_entities[0],NULL); }
int main(void) {
	gentity_t *start,*split,*finish,*bad;
	int i;
	level.clients=clients; level.maxclients=2; level.num_entities=2; level.numPlayingClients=2;
	level.sortedClients[0]=0; level.sortedClients[1]=1; level.time=1000; g_gametype.integer=GT_RACE;
	for(i=0;i<2;++i) { g_entities[i].client=&clients[i]; g_entities[i].inuse=qtrue; g_entities[i].health=100; }
	start=point(NULL,"split"); split=point("split","finish"); finish=point("finish",NULL);
	CHECK(level.racePointCount==3 && start->s.generic1==1 && start->touch);
	G_BeginRace(&g_entities[0]); G_BeginRace(&g_entities[1]);
	CHECK(clients[0].ps.persistant[PERS_SCORE]==INT_MAX && !strcmp(recordCount,"3") && !strcmp(message,"race_init"));
	level.warmupTime=1; touch(start,0); CHECK(!clients[0].raceActive); level.warmupTime=0;
	touch(finish,100); CHECK(!ranks);
	touch(start,0); CHECK(clients[0].raceActive && clients[0].raceNext==split && lastEvent->s.eType==ET_EVENTS+EV_RACE_START);
	CHECK(lastEvent->s.time==1000 && lastEvent->s.powerups==finish->s.number);
	touch(finish,50); CHECK(clients[0].raceNext==split && !ranks);
	touch(start,999); CHECK(clients[0].flagCarryTime==999);
	touch(split,500); CHECK(clients[0].raceSplitCount==1 && lastEvent->s.time==0 && !lastEvent->s.generic1);
	touch(finish,1250); CHECK(ranks==1 && clients[0].ps.persistant[PERS_SCORE]==1250 && clients[0].raceBestSplits[0]==500);
	CHECK(!clients[0].raceActive && !clients[0].raceNext && strstr(message,"Personal best") && strstr(message,"0:01.250"));
	touch(start,0); touch(split,700); CHECK(lastEvent->s.time==200 && lastEvent->s.generic1==1);
	touch(finish,1500); CHECK(clients[0].ps.persistant[PERS_SCORE]==1250 && clients[0].raceBestSplits[0]==500 && !strstr(message,"Personal best"));
	clients[1].ps.persistant[PERS_SCORE]=1000;
	touch(start,0); touch(split,400); touch(finish,1000);
	CHECK(clients[0].raceLastTime==1001 && clients[0].ps.persistant[PERS_SCORE]==1001 && clients[0].raceBestSplits[0]==400);
	G_RaceInfo(&g_entities[1],&clients[0]); CHECK(strstr(message,"race_info 0 1000 1001 1 -1"));
	touch(start,0); touch(split,61000); touch(finish,63001); CHECK(strstr(message,"1:03.001") && clients[0].ps.persistant[PERS_SCORE]==1001);
	touch(start,0); touch(split,100); touch(finish,INT_MAX); CHECK(!clients[0].raceActive && clients[0].ps.persistant[PERS_SCORE]==1001);
	touch(start,0); G_ResetRace(&clients[0]); CHECK(!clients[0].raceActive && !clients[0].raceFollowing && clients[0].ps.persistant[PERS_SCORE]==1001);
	bad=point(NULL,"missing"); touch(bad,0); CHECK(!clients[0].raceActive);
	bad->target="split"; split->target="split"; touch(bad,0);
	for(i=0;i<64;++i) touch(split,100+i);
	CHECK(clients[0].raceSplitCount==64 && clients[0].raceActive);
	touch(split,200); CHECK(!clients[0].raceActive);
	// Clearing a course invalidates every runner's pointers. The lifetime
	// registration cap prevents reusing exhausted editor storage after clear.
	clients[1].raceNext=split; clients[1].raceActive=qtrue;
	level.racePointCount=64; g_cheats.integer=1; argc=2; action="clear";
	Cmd_RacePoint_f(&g_entities[0]); CHECK(!clients[1].raceNext && !clients[1].raceActive);
	argc=1; Cmd_RacePoint_f(&g_entities[0]); CHECK(!level.raceEditorPrevious && strstr(message,"too many"));
	G_BeginRace(&g_entities[0]); CHECK(clients[0].ps.persistant[PERS_SCORE]==INT_MAX && !clients[0].raceBestSplits[0]);
	puts("Race records, checkpoint order and bounds tests passed"); return 0;
}
