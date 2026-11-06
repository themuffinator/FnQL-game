/* FnQL-game, Copyright (C) 2026 contributors. GPL-2.0-or-later. */
#include "../code/cgame/cg_local.h"
#include "../code/ui/ui_shared.h"
#include <limits.h>
#include <math.h>
#define CHECK(x) do { if (!(x)) { fprintf(stderr,"line %d: %s\n",__LINE__,#x); exit(1); } } while (0)
#define NEAR(a,b) CHECK(fabs((a)-(b))<.001)
cg_t cg;
cgs_t cgs;
vmCvar_t cg_drawPregameMessages,cg_announcerVolume;
static snapshot_t snapshot;
static const char *configs[MAX_CONFIGSTRINGS];
static int count,font,voices,starts,catcher,keys;
static char voice[64],uiWarmup[32];
static struct { char text[160]; float x,y,scale; int font,style; vec4_t color; } paints[4];
const char *CG_ConfigString(int id) { CHECK(id>=0 && id<MAX_CONFIGSTRINGS); return configs[id] ? configs[id] : ""; }
void QDECL Com_Printf(const char *format,...) { (void)format; }
void QDECL Com_Error(int code,const char *format,...) { (void)code; (void)format; abort(); }
void CG_SetTextFont(int value) { font=value; }
int CG_Text_Width(const char *text,float scale,int limit) { CHECK(scale>0 && !limit); return Q_PrintStrlen(text)*7; }
void CG_Text_Paint(float x,float y,float scale,vec4_t color,const char *text,float adjust,int limit,int style) {
    CHECK(count<4 && !adjust && !limit); Q_strncpyz(paints[count].text,text,sizeof(paints[count].text));
    paints[count].x=x; paints[count].y=y; paints[count].scale=scale; paints[count].font=font; paints[count].style=style;
    Vector4Copy(color,paints[count].color); ++count;
}
sfxHandle_t CG_QLVoice(const char *name) { Q_strncpyz(voice,name,sizeof(voice)); return 42; }
void trap_S_StartLocalSoundVolume(sfxHandle_t sound,int channel,float volume) {
    CHECK(sound==42 && channel==CHAN_ANNOUNCER && volume==.75f); ++voices;
}
void CG_AnnounceMatchStart(void) { ++starts; }
void trap_Cvar_Set(const char *name,const char *value) { CHECK(!strcmp(name,"ui_warmup")); Q_strncpyz(uiWarmup,value,sizeof(uiWarmup)); }
int trap_Key_GetCatcher(void) { return catcher; }
int trap_Key_GetKey(const char *name) { CHECK(!strcmp(name,"readyup")); ++keys; return 75; }
void trap_Key_KeynumToStringBuf(int key,char *buffer,int size) { CHECK(key==75); Q_strncpyz(buffer,"F3",size); }
static void Config(int id,const char *text) { configs[id]=text; CG_PregameConfigChanged(id); }
static void Player(int id,int team,const char *name) {
    cgs.clientinfo[id].infoValid=qtrue; cgs.clientinfo[id].team=team;
    Q_strncpyz(cgs.clientinfo[id].name,name,sizeof(cgs.clientinfo[id].name));
}
static void Reset(int mode) {
    int i; memset(&cg,0,sizeof(cg)); memset(&cgs,0,sizeof(cgs)); memset(&snapshot,0,sizeof(snapshot));
    for (i=0;i<MAX_CONFIGSTRINGS;++i) configs[i]=NULL;
    cg.snap=&snapshot; cg.time=10000; cgs.maxclients=MAX_CLIENTS; cgs.gametype=mode;
    cg_announcerVolume.value=.75f; cg_drawPregameMessages.integer=1;
    count=voices=starts=keys=catcher=0; voice[0]=0;
    CG_InitPregame();
    Config(CS_SERVERINFO,"\\g_teamSizeMin\\1\\g_teamForceBalance\\1\\g_freezeRoundDelay\\7000");
}
static void Draw(void) { count=0; CG_DrawPregame(); }
static void Text(int at,const char *expected) {
    CHECK(at<count);
    if (strcmp(paints[at].text,expected)) { fprintf(stderr,"expected [%s], got [%s]\n",expected,paints[at].text); exit(1); }
    NEAR(paints[at].x,320-(Q_PrintStrlen(expected)*7)/2);
}
static void Waiting(void) {
    Reset(GT_FFA); Player(0,TEAM_FREE,"Alpha"); Config(CS_WARMUP,"\\time\\-1"); Draw();
    CHECK(count==2 && !keys); Text(0,"The match will begin"); Text(1,"when more players join.");
    NEAR(paints[0].y,88); NEAR(paints[1].y,108); NEAR(paints[0].scale,.35); CHECK(paints[0].font==1 && paints[0].style==3);
    Player(1,TEAM_FREE,"Beta"); Draw(); CHECK(count==3); Text(1,"when more players are ready."); Text(2,"Press F3 to ready yourself");
    CHECK(paints[2].font==0 && paints[2].style==6 && paints[2].color[0]==1 && paints[2].color[1]==0);
    NEAR(paints[2].y,124); NEAR(paints[2].scale,.25);
    Config(CS_PLAYERS,"\\rp\\1"); Draw(); Text(2,"Press F3 to unready yourself"); CHECK(paints[2].color[0]==0 && paints[2].color[1]==1);
    Config(CS_PLAYERS,"\\rp\\bad"); Draw(); Text(2,"Press F3 to unready yourself");
    cg_drawPregameMessages.integer=0; Draw(); CHECK(count==1); Text(0,"Press F3 to unready yourself");
    cg_drawPregameMessages.integer=1; Config(CS_READY_UP,"14000"); Draw(); Text(0,"Players must ready"); Text(1,"within 5 seconds.");
    cg.time=14000; Draw(); Text(1,"within 1 second."); cg.time=16000; Draw(); Text(1,"within 0 seconds.");
    Config(CS_READY_UP,"2147483648"); Draw(); Text(1,"within 0 seconds.");
    Config(CS_WARMUP,"\\time\\oops"); CHECK(cg.warmup==-1); Draw(); Text(0,"Players must ready");
    cgs.clientinfo[0].team=TEAM_SPECTATOR; Draw(); CHECK(count==2 && keys>0);
    Reset(GT_TOURNAMENT); Config(CS_WARMUP,"\\time\\-1"); Player(0,TEAM_FREE,"A"); Player(1,TEAM_FREE,"B"); Player(2,TEAM_FREE,"C");
    Draw(); Text(0,"The match will begin when"); Text(1,"fewer players are in the match.");
    Reset(GT_RACE); Config(CS_WARMUP,"\\time\\-1"); Player(0,TEAM_FREE,"A"); Player(1,TEAM_FREE,"B"); Draw();
    NEAR(paints[0].y,98); NEAR(paints[1].y,118); NEAR(paints[2].y,134);
}
static void Teams(void) {
    Reset(GT_CLAN_ARENA); Config(CS_WARMUP,"\\time\\-1"); Draw(); Text(0,"Waiting for more players."); Text(1,"The match requires 1 player per team.");
    Player(0,TEAM_RED,"A"); Draw(); Text(0,"Waiting for 1 more player"); Text(1,"to join the Blue Team."); CHECK(count==2);
    cgs.clientinfo[0].team=TEAM_BLUE; Draw(); Text(1,"to join the Red Team.");
    Config(CS_SERVERINFO,"\\g_teamSizeMin\\3"); Player(1,TEAM_RED,"B"); Player(2,TEAM_BLUE,"C"); Player(3,TEAM_BLUE,"D");
    Draw(); Text(0,"Waiting for 2 more players"); Text(1,"to join the Red Team.");
    Config(CS_SERVERINFO,"\\g_teamSizeMin\\1\\g_teamForceBalance\\1"); Draw(); Text(0,"The teams must be balanced"); Text(1,"before the match can begin."); CHECK(count==3);
    Config(CS_SERVERINFO,"\\g_teamSizeMin\\-1"); Draw(); Text(0,"The teams must be balanced");
    Config(CS_SERVERINFO,"\\g_teamSizeMin\\1\\g_teamForceBalance\\0"); Draw(); Text(1,"when more players are ready.");
    Reset(GT_RED_ROVER); Player(0,TEAM_RED,"A"); Config(CS_WARMUP,"\\time\\-1"); Draw(); Text(1,"when more players join."); CHECK(count==2);
    Config(CS_SETTINGS_MASK,"67108864"); Draw(); Text(1,"when more players are ready."); CHECK(count==3);
    Config(CS_SETTINGS_MASK,"4294967296"); Draw(); CHECK(count==3);
    Config(CS_SETTINGS_MASK,""); Draw(); CHECK(count==2);
}
static void Countdowns(void) {
    int mode;
    static const char *const titles[]={"Free For All","","Race","Team Deathmatch","Clan Arena","Capture the Flag","One Flag CTF","Overload","Harvester","Freeze Tag","Domination","Attack and Defend","Red Rover"};
    for (mode=0;mode<13;++mode) {
        Reset(mode); Config(CS_WARMUP,"\\time\\14000"); CHECK(voices==1 && !strcmp(uiWarmup,"14000"));
        Draw(); CHECK(count==2); Text(0,titles[mode]); Text(1,"Starts in: 5");
        NEAR(paints[0].scale,.6); NEAR(paints[1].scale,.45); CHECK(paints[0].style==6);
        NEAR(paints[0].y,mode==GT_RACE ? 100 : 90);
        CHECK(voices==1); // no round-begins cue during the initial countdown
        cg.time=12000; Draw(); Text(1,"Starts in: 3");
        CHECK(voices==(mode==4 || mode==9 || mode==11 || mode==12 ? 1 : 2));
        cg.time=14000; Draw(); Text(1,"Starts in: 1"); cg.time=14001; Draw(); CHECK(!count && !starts);
    }
    Reset(GT_TOURNAMENT); Player(0,TEAM_FREE,"Alpha"); Player(1,TEAM_FREE,"Beta"); Config(CS_WARMUP,"\\time\\14000"); Draw(); Text(0,"Alpha vs Beta");
    Reset(GT_FFA); Config(CS_SETTINGS_MASK,"262144"); Config(CS_WARMUP,"\\time\\14000"); Draw(); Text(0,"Quad Hog");
    Config(CS_WARMUP,"\\time\\14000\\g_gametype\\2"); Draw(); Text(0,"Race");
    Config(CS_WARMUP,"\\time\\15000\\g_gametype\\bad"); Draw(); Text(1,"Starts in: 5");
    Reset(GT_CLAN_ARENA); Config(CS_MATCH_STATE,"\\time\\14000\\round\\2"); Draw(); Text(0,"Round Begins in"); Text(1,"5"); CHECK(!strcmp(voice,"round_begins_in"));
    Draw(); CHECK(voices==1); cg.time=12000; Draw(); CHECK(!strcmp(voice,"three")); cg.time=14001; Draw(); CHECK(starts==1 && !count); Draw(); CHECK(starts==1);
    cg.time=13000; Draw(); Text(1,"2"); CHECK(!strcmp(voice,"two")); // demo rewind resumes countdown
    Config(CS_MATCH_STATE,"\\time\\14000\\round\\2147483648"); CHECK(cg.warmup==14000);
    cgs.timelimit=1; cgs.levelStartTime=0; cg.timelimitWarnings=4; cg.time=70000;
    Config(CS_MATCH_STATE,"\\time\\74000\\round\\3"); Draw(); Text(0,"Sudden Death");
    Reset(GT_FREEZE); Config(CS_SERVERINFO,"\\g_freezeRoundDelay\\0"); Config(CS_MATCH_STATE,"\\time\\14000\\round\\2"); Draw(); Text(0,"Freeze Tag"); CHECK(!voices);
    cg.time=12000; Draw(); CHECK(!strcmp(voice,"three"));
    Reset(GT_ATTACK_DEFEND); Player(0,TEAM_RED,"A"); Config(CS_MATCH_STATE,"\\time\\14000\\round\\0\\turn\\0\\state\\1"); Draw(); Text(0,"ATTACK THE FLAG"); Text(1,"5");
    Config(CS_MATCH_STATE,"\\time\\14000\\round\\0\\turn\\1\\state\\1"); Draw(); Text(0,"DEFEND THE FLAG");
    cgs.clientinfo[0].team=TEAM_BLUE; Draw(); Text(0,"ATTACK THE FLAG"); cgs.clientinfo[0].team=TEAM_SPECTATOR; Draw(); Text(0,"Blue is on offense");
    Config(CS_MATCH_STATE,"\\time\\0\\round\\2147483647"); Draw(); CHECK(count==2);
    Reset(GT_RED_ROVER); Config(CS_SETTINGS_MASK,"67108864"); Config(CS_WARMUP,"\\time\\14000"); Draw(); Text(0,"Infected");
}
static void VisibilityAndRestart(void) {
    Reset(GT_FFA); Draw(); CHECK(!count && !voices); Config(CS_WARMUP,"\\time\\14000");
    cg.showScores=qtrue; cg.time=12000; Draw(); CHECK(!count && !strcmp(voice,"three"));
    cg.showScores=qfalse; catcher=KEYCATCH_CGAME; Draw(); CHECK(!count); catcher=0; Draw(); CHECK(count==2);
    Config(CS_INTERMISSION,"1"); cg.time=13000; Draw(); CHECK(!count && voices==2);
    Config(CS_INTERMISSION,"0"); Draw(); CHECK(!strcmp(voice,"one") || !strcmp(voice,"two"));
    Config(CS_WARMUP,"\\time\\0"); CG_PregameRestart(); CHECK(starts==1); Draw(); CHECK(!count);
    Reset(GT_CLAN_ARENA); CG_PregameRestart(); CHECK(!starts);
    Config(CS_MATCH_STATE,"\\time\\-1"); cgs.maxclients=INT_MAX; cg.clientNum=INT_MAX; Draw(); CHECK(count==2 && !keys);
    cg.snap=NULL; Draw(); CHECK(!count);
    Reset(GT_FFA); cg.time=INT_MIN; Config(CS_WARMUP,"\\time\\2147483647"); Draw(); Text(1,"Starts in: 4294968");
    configs[CS_WARMUP]="\\time\\-1"; configs[CS_PLAYERS]="\\rp\\1"; CG_InitPregame();
    Player(0,TEAM_FREE,"A"); Player(1,TEAM_FREE,"B"); Draw(); Text(2,"Press F3 to unready yourself");
}
int main(void) { Waiting(); Teams(); Countdowns(); VisibilityAndRestart(); puts("cgame pregame: PASS"); return 0; }
