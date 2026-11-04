/* FnQL-game, Copyright (C) 2026 contributors. GPL-2.0-or-later. */
#include "../code/game/g_local.h"
#include <limits.h>
#define CHECK(x) do { if (!(x)) { fprintf(stderr, "%s:%d: %s\n", __FILE__, __LINE__, #x); exit(1); } } while (0)
level_locals_t level;
gentity_t g_entities[MAX_GENTITIES];
vmCvar_t g_gametype, g_timeoutLen, g_timeoutCount, g_cheats;
static gclient_t clients[4];
static const char *args[8];
static int argc, dropped = -1, replies, forced, consoleCommands;
static char message[2048], fileData[4096], output[4096];
static qboolean fileMissing, fileUnwritable, local;

char *QDECL va(const char *fmt, ...) {
	static char buffers[8][2048]; static int n; char *b = buffers[n++ & 7]; va_list ap;
	va_start(ap,fmt); vsnprintf(b,2048,fmt,ap); va_end(ap); return b;
}
int QDECL Com_sprintf(char *b, int size, const char *fmt, ...) {
	va_list ap; va_start(ap,fmt); int result=vsnprintf(b,size,fmt,ap); va_end(ap); return result;
}
void Q_strncpyz(char *b,const char *s,int size) { snprintf(b,size,"%s",s); }
int Q_stricmp(const char *a,const char *b) { while (*a && tolower((unsigned char)*a)==tolower((unsigned char)*b)) { ++a; ++b; } return tolower((unsigned char)*a)-tolower((unsigned char)*b); }
char *Info_ValueForKey(const char *info,const char *key) { (void)info; (void)key; return local ? "localhost" : "192.0.2.1"; }
void QDECL G_Printf(const char *fmt,...) { (void)fmt; }
void trap_SendServerCommand(int client,const char *text) { CHECK(client >= -1 && client < 4); Q_strncpyz(message,text,sizeof(message)); ++replies; }
void trap_SetConfigstring(int index,const char *text) { (void)index; (void)text; }
void trap_Cvar_Set(const char *key,const char *text) { (void)key; (void)text; }
void trap_SendConsoleCommand(int when,const char *text) { CHECK(when==EXEC_APPEND); CHECK(!strcmp(text,"map_restart 3\n")); ++consoleCommands; }
int trap_Argc(void) { return argc; }
void trap_Argv(int n,char *b,int size) { Q_strncpyz(b,n<argc?args[n]:"",size); }
char *ConcatArgs(int start) { static char b[256]; int i; b[0]=0; for(i=start;i<argc;i++) { if(i>start) strcat(b," "); strcat(b,args[i]); } return b; }
unsigned long long trap_GetSteamId64(int client) { return 76561198000000000ULL + client; }
const char *G_SteamIdString(unsigned long long id) { static char b[32]; snprintf(b,sizeof(b),"%llu",id); return b; }
void trap_GetUserinfo(int client,char *b,int size) { (void)client; Q_strncpyz(b,"",size); }
qboolean ClientUserinfoChanged(int client) { (void)client; return qtrue; }
void trap_DropClient(int client,const char *reason) { CHECK(!strcmp(reason,"was kicked")); dropped=client; }
qboolean G_IsRoundBasedGametype(int mode) { (void)mode; return qfalse; }
qboolean G_RoundCountingDown(void) { return qfalse; }
void G_AutoAction(void) {}
void CalculateRanks(void) {}
const char *TeamName(team_t team) { static const char *names[]={"FREE","RED","BLUE","SPECTATOR"}; return names[team]; }
int G_ForceTeam(gentity_t *ent,const char *team) { (void)ent; CHECK(!strcmp(team,"b")); ++forced; return TEAM_BLUE; }
void AddScore(gentity_t *ent,vec3_t origin,int score) { (void)origin; ent->client->ps.persistant[PERS_SCORE]+=score; }
void AddTeamScore(team_t team,int score) { level.teamScores[team]+=score; }
void trap_Cvar_VariableStringBuffer(const char *key,char *b,int size) { CHECK(!strcmp(key,"g_accessFile")); Q_strncpyz(b,"access.txt",size); }
int trap_FS_FOpenFile(const char *path,fileHandle_t *file,fsMode_t mode) {
	CHECK(!strcmp(path,"access.txt")); *file=(fileMissing && mode==FS_READ)||(fileUnwritable && mode==FS_WRITE)?FS_INVALID_HANDLE:1;
	if(mode==FS_WRITE) output[0]=0;
	return (int)strlen(fileData);
}
void trap_FS_Read(void *b,int length,fileHandle_t file) { CHECK(file==1); memcpy(b,fileData,length); }
void trap_FS_Write(const void *b,int length,fileHandle_t file) { CHECK(file==1); CHECK(strlen(output)+length<sizeof(output)); strncat(output,b,length); }
void trap_FS_FCloseFile(fileHandle_t file) { CHECK(file==1); }

void QDECL G_LogPrintf(const char *fmt,...) { (void)fmt; }
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
// Race touch callbacks are outside this command-policy harness. Fail if a
// command unexpectedly enters them instead of silently simulating gameplay.
gentity_t *G_PickTarget(const char *target) { (void)target; CHECK(0); return NULL; }
gentity_t *G_TempEntity(vec3_t origin,int event) { (void)origin; (void)event; CHECK(0); return NULL; }
char *G_NewString(const char *text) { static char names[4096];static int used;char *out=names+used;used+=(int)strlen(text)+1;CHECK(used<=sizeof(names));strcpy(out,text);return out; }
int G_ModelIndex(const char *name) { CHECK(strstr(name,"models/flag3/"));return 1; }
void trap_LinkEntity(gentity_t *ent) { ent->r.linked=qtrue; }
void trap_Trace(trace_t *tr,const vec3_t start,const vec3_t mins,const vec3_t maxs,const vec3_t end,int pass,int mask) {
	(void)start;(void)mins;(void)maxs;(void)pass;(void)mask;memset(tr,0,sizeof(*tr));tr->fraction=0.5f;VectorCopy(end,tr->endpos);tr->endpos[2]=15;
}
char *vtos(const vec3_t v) { (void)v;return "(0 0 0)"; }
extern void SP_race_point(gentity_t *ent);

static void command(int caller,const char *name,const char *a,const char *b) {
	args[0]=name;args[1]=a;args[2]=b;argc=b?3:a?2:1;message[0]=0;
	CHECK(G_AdminCommand(caller<0?NULL:&g_entities[caller],name));
}

int main(void) {
	unsigned long long id;
	int i;
	level.clients=clients;level.maxclients=4;level.time=10000;g_gametype.integer=GT_TEAM;g_timeoutLen.integer=60;
	for(i=0;i<4;++i) { g_entities[i].client=&clients[i];g_entities[i].inuse=qtrue;clients[i].pers.connected=CON_CONNECTED;clients[i].sess.sessionTeam=TEAM_RED; }
	CHECK(G_ParseAccount("18446744073709551615",&id)&&id==~0ULL);
	CHECK(!G_ParseAccount("18446744073709551616",&id));CHECK(!G_ParseAccount("-1",&id));CHECK(!G_ParseAccount("1x",&id));CHECK(!G_ParseAccount("",&id));
	strcpy(fileData,"# policy\n76561198000000000|admin\r\n76561198000000001|mod\n76561198000000002|ban\n76561198000000003|bad\n18446744073709551616|admin\n");
	G_LoadAccess();CHECK(G_ClientPrivilege(0)==2);CHECK(G_ClientPrivilege(1)==1);CHECK(G_ClientPrivilege(2)==-1);CHECK(G_ClientPrivilege(3)==0);
	fileMissing=qtrue;G_LoadAccess();CHECK(G_ClientPrivilege(0)==2);fileMissing=qfalse;
	local=qtrue;CHECK(G_ClientPrivilege(2)==3);local=qfalse;
	command(0,"pause",NULL,NULL);CHECK(!level.timeoutStartTime);CHECK(strstr(message,"privileges required"));
	command(-1,"addadmin","0",NULL);CHECK(clients[0].pers.privileges==2);
	command(0,"addmod","1",NULL);CHECK(clients[1].pers.privileges==1);
	command(1,"demote","0",NULL);CHECK(clients[0].pers.privileges==2);
	command(0,"demote","0",NULL);CHECK(clients[0].pers.privileges==2);
	command(0,"mute","1",NULL);CHECK(clients[1].sess.muted);
	command(1,"mute","0",NULL);CHECK(!clients[0].sess.muted);
	command(0,"unmute","1",NULL);CHECK(!clients[1].sess.muted);
	command(0,"ban","1",NULL);CHECK(dropped==-1);
	command(0,"tempban","3",NULL);CHECK(dropped==3);CHECK(G_ClientPrivilege(3)==-1);
	command(0,"unban","76561198000000003",NULL);CHECK(G_ClientPrivilege(3)==0);
	command(0,"ban","3",NULL);CHECK(G_ClientPrivilege(3)==-1);
	command(0,"lock",NULL,NULL);CHECK(level.teamLocked[TEAM_RED]&&level.teamLocked[TEAM_BLUE]);
	command(0,"unlock","b",NULL);CHECK(level.teamLocked[TEAM_RED]&&!level.teamLocked[TEAM_BLUE]);
	command(0,"put","1","b");CHECK(forced==1);
	command(0,"put","1garbage","b");CHECK(forced==1);
	command(0,"put","18446744073709551616","b");CHECK(forced==1);
	command(0,"addscore","1","3.9");CHECK(clients[1].ps.persistant[PERS_SCORE]==3);
	command(0,"addscore","1","2147483647");CHECK(clients[1].ps.persistant[PERS_SCORE]==3);
	level.teamScores[TEAM_FREE]=INT_MAX;command(0,"addscore","1","1");CHECK(clients[1].ps.persistant[PERS_SCORE]==3);level.teamScores[TEAM_FREE]=0;
	command(0,"addteamscore","r","-2");CHECK(level.teamScores[TEAM_RED]==-2);
	command(0,"setmatchtime","60",NULL);CHECK(level.startTime==-50000);
	command(0,"setmatchtime","2147483647",NULL);CHECK(level.startTime==-50000);
	g_timeoutLen.integer=INT_MAX;command(-1,"timeout",NULL,NULL);CHECK(!level.timeoutStartTime);g_timeoutLen.integer=60;
	command(-1,"pause",NULL,NULL);CHECK(level.timeoutStartTime==10000&&level.timeoutByServer);
	command(0,"timein",NULL,NULL);CHECK(!level.timeoutEndTime);
	command(-1,"unpause",NULL,NULL);CHECK(level.timeoutEndTime==15000);
	command(0,"abort",NULL,NULL);CHECK(!consoleCommands);
	level.timeoutStartTime=0;command(0,"abort",NULL,NULL);CHECK(consoleCommands==1&&level.warmupTime==-1);
	command(0,"allready",NULL,NULL);CHECK(clients[3].pers.readyToPlay);
	command(-1,"players",NULL,NULL);command(-1,"?",NULL,NULL);command(0,"opsay","hello",NULL);
	command(0,"listaccess","1",NULL);CHECK(strstr(message,"PERM"));
	CHECK(!G_AdminCommand(&g_entities[0],"rcon"));CHECK(!G_AdminCommand(NULL,"unknown"));
	G_SaveAccess();CHECK(strstr(output,"76561198000000003|ban\r\n"));
	CHECK(G_SetAccess(trap_GetSteamId64(3),-1,qtrue));G_SaveAccess();CHECK(!strstr(output,"76561198000000003"));
	fileUnwritable=qtrue;CHECK(G_SetAccess(trap_GetSteamId64(1),2,qfalse));G_SaveAccess();
	fileUnwritable=qfalse;G_SaveAccess();CHECK(strstr(output,"76561198000000001|admin"));
	CHECK(!G_SetAccess(42,3,qfalse));CHECK(!G_SetAccess(42,-2,qfalse));
	strcpy(fileData,output);G_ReloadAccess();CHECK(clients[1].pers.privileges==2);
	G_ShutdownAccess();CHECK(G_ClientPrivilege(0)==0);
	// Spectator selection skips non-playing clients and respects team access.
	clients[0].sess.sessionTeam=TEAM_SPECTATOR;clients[0].sess.spectatorClient=0;
	clients[1].sess.sessionTeam=TEAM_BLUE;clients[2].sess.sessionTeam=TEAM_RED;
	args[1]="b";argc=2;Cmd_Spec_f(&g_entities[0]);CHECK(clients[0].sess.spectatorClient==1);
	clients[0].sess.sessionTeam=TEAM_RED;Cmd_Spec_f(&g_entities[0]);CHECK(clients[0].sess.spectatorClient==1);
	clients[2].ps.powerups[PW_QUAD]=1;args[1]="pu";Cmd_Spec_f(&g_entities[0]);CHECK(clients[0].sess.spectatorClient==2);
	args[1]="Blue";Cmd_Spec_f(&g_entities[0]);CHECK(clients[0].sess.spectatorClient==2);
	// Training chat is restricted to bots and strictly bounded targets/times.
	args[1]="1";args[2]="5";args[3]="hello";argc=4;replies=0;
	Cmd_BotSay_f(&g_entities[0]);CHECK(!replies);
	g_entities[0].r.svFlags|=SVF_BOT;Cmd_BotSay_f(&g_entities[0]);CHECK(strstr(message,"bchat \"^7^2hello\" 5"));
	args[1]="-1";replies=0;Cmd_BotSay_f(&g_entities[0]);CHECK(!replies);
	args[1]="1";args[2]="2147483648";Cmd_BotSay_f(&g_entities[0]);CHECK(!replies);
	// Race editor gates, 64-point bound, ownership reset, floor placement.
	argc=1;level.num_entities=4;Cmd_RacePoint_f(&g_entities[0]);CHECK(strstr(message,"Race gametype"));
	g_gametype.integer=GT_RACE;Cmd_RacePoint_f(&g_entities[0]);CHECK(strstr(message,"Cheats"));
	g_cheats.integer=1;Cmd_RacePoint_f(&g_entities[0]);CHECK(strstr(message,"alive"));
	g_entities[0].health=100;Cmd_RacePoint_f(&g_entities[0]);CHECK(level.raceEditorCount==1);
	CHECK(g_entities[4].r.linked&&g_entities[4].r.contents==CONTENTS_TRIGGER);
	g_entities[4].think(&g_entities[4]);CHECK(g_entities[4].r.currentOrigin[2]==16);
	for(i=1;i<64;++i) Cmd_RacePoint_f(&g_entities[0]);CHECK(level.raceEditorCount==64);
	Cmd_RacePoint_f(&g_entities[0]);CHECK(level.raceEditorCount==64&&strstr(message,"too many"));
	clients[0].raceActive=qtrue;clients[0].raceNext=&g_entities[4];G_ResetRace(&clients[0]);CHECK(!clients[0].raceActive&&!clients[0].raceNext);
	args[1]="clear";argc=2;Cmd_RacePoint_f(&g_entities[0]);CHECK(!level.raceEditorPrevious&&!level.raceEditorCount);
	CHECK(!G_Find(NULL,FOFS(classname),"race_point"));
	puts("admin command and account policy tests passed");return 0;
}
