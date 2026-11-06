/* FnQL-game, Copyright (C) 2026 contributors. GPL-2.0-or-later. */
#include "../code/cgame/cg_local.h"
#include "../code/ui/ui_shared.h"
#include <limits.h>
#include <math.h>
#define CHECK(x) do { if (!(x)) { fprintf(stderr,"line %d: %s\n",__LINE__,#x); exit(1); } } while (0)
#define NEAR(a,b) CHECK(fabs((a)-(b))<.001)
cg_t cg;
cgs_t cgs;
vmCvar_t cg_scoreRedTeamName,cg_scoreBlueTeamName;
static snapshot_t snapshot;
static const char *configs[MAX_CONFIGSTRINGS];
static struct { char text[128]; float x,y,alpha; int style; } paints[8];
static int count,font,registrations,pictures;
static char asset[MAX_QPATH];
static vec4_t color={.2f,.4f,.6f,.8f};
static float clipEdge;
static qboolean unchangedEdge;
const char *CG_ConfigString(int id) { CHECK(id>=0 && id<MAX_CONFIGSTRINGS); return configs[id] ? configs[id] : ""; }
void QDECL Com_Printf(const char *format,...) { (void)format; }
void QDECL Com_Error(int code,const char *format,...) { (void)code; (void)format; abort(); }
void CG_SetTextFont(int value) { font=value; }
int CG_Text_Width(const char *text,float scale,int limit) {
    int width=0;
    CHECK(scale==.25f && !limit);
    while (*text) {
        if (Q_IsColorString(text)) { text+=2; continue; }
        if (((unsigned char)*text&0xc0)!=0x80) width+=7;
        ++text;
    }
    return width;
}
void CG_Text_Paint(float x,float y,float scale,vec4_t ink,const char *text,float adjust,int limit,int style) {
    CHECK(count<ARRAY_LEN(paints) && scale==.25f && !adjust && !limit);
    CHECK(ink[0]==color[0] && ink[1]==color[1] && ink[2]==color[2]);
    Q_strncpyz(paints[count].text,text,sizeof(paints[count].text));
    paints[count].x=x; paints[count].y=y; paints[count].alpha=ink[3]; paints[count].style=style; ++count;
}
void CG_Text_PaintClipped(float *maxX,float x,float y,float scale,vec4_t ink,const char *text) {
    clipEdge=*maxX; CG_Text_Paint(x,y,scale,ink,text,0,0,0);
    if (!unchangedEdge) *maxX-=1;
}
qhandle_t trap_R_RegisterShader(const char *name) { Q_strncpyz(asset,name,sizeof(asset)); ++registrations; return 42; }
void trap_R_SetColor(const float *ink) { CHECK(!ink); }
void CG_DrawPic(float x,float y,float w,float h,qhandle_t shader) {
    CHECK(shader==42); NEAR(x,40.75); NEAR(y,19); NEAR(w,136); NEAR(h,40); ++pictures;
}
static void Config(int id,const char *value) { configs[id]=value; CG_StandingsConfigChanged(id); }
static void Reset(int mode) {
    int i;
    memset(&cg,0,sizeof(cg)); memset(&cgs,0,sizeof(cgs)); memset(&snapshot,0,sizeof(snapshot));
    for (i=0;i<MAX_CONFIGSTRINGS;++i) configs[i]=NULL;
    memset(&cg_scoreRedTeamName,0,sizeof(cg_scoreRedTeamName));
    memset(&cg_scoreBlueTeamName,0,sizeof(cg_scoreBlueTeamName));
    cg.snap=&snapshot; cgs.gametype=mode; cgs.scores1=10; cgs.scores2=4;
    cgs.clientinfo[0].team=TEAM_RED; Q_strncpyz(cgs.clientinfo[0].name,"Local",MAX_NAME_LENGTH);
    snapshot.ps.persistant[PERS_TEAM]=TEAM_FREE;
    count=registrations=pictures=0; font=2; unchangedEdge=qfalse; CG_InitStandings();
    Config(CS_FIRST_PLACE,"Leader"); Config(CS_SECOND_PLACE,"Runner");
}
static void Draw(int owner) {
    count=0; CHECK(CG_DrawStandings(owner,40.75f,19,136,40,ITEM_ALIGN_LEFT,.25f,color,3));
    NEAR(color[3],.8); CHECK(font==0);
}
static void Text(int at,const char *expected) {
    CHECK(at<count);
    if (strcmp(paints[at].text,expected)) { fprintf(stderr,"text %d: expected [%s], got [%s]\n",at,expected,paints[at].text); exit(1); }
    NEAR(paints[at].y,19);
}
static void Modes(void) {
    static const char *const icons[]={"ffa","duel","race","tdm","ca","ctf","1f","ffa","har","ft","dom","ad","rr"};
    static const char *const limits[]={"20","20","6","20","8","3","3","3","3","8","150","150","8"};
    int mode;
    for (mode=0;mode<13;++mode) {
        char expected[MAX_QPATH]; Reset(mode);
        Config(CS_SERVERINFO,"\\fraglimit\\20\\capturelimit\\3\\roundlimit\\8\\scorelimit\\150");
        Config(CS_RACE_RECORDS,"8"); cg.race.pointsPassed=2;
        Draw(CG_CAPFRAGLIMIT); CHECK(count==1); Text(0,limits[mode]); NEAR(paints[0].x,40.75);
        Draw(CG_GAME_TYPE_ICON); snprintf(expected,sizeof(expected),"ui/assets/hud/%s.tga",icons[mode]);
        CHECK(!strcmp(asset,expected) && registrations==1 && pictures==1 && count==0);
        Draw(CG_GAME_TYPE_ICON); CHECK(registrations==1 && pictures==2);
    }
    Reset(GT_FFA); Config(CS_SERVERINFO,"\\fraglimit\\25");
    Config(CS_SERVERINFO,"\\fraglimit\\9\\scorelimit\\oops"); Draw(CG_CAPFRAGLIMIT); Text(0,"25");
    Config(CS_SERVERINFO,"\\fraglimit\\2147483648"); Draw(CG_CAPFRAGLIMIT); Text(0,"25");
    count=0; CHECK(CG_DrawStandings(CG_CAPFRAGLIMIT,40.75f,19,136,40,ITEM_ALIGN_RIGHT,.25f,color,3)); NEAR(paints[0].x,26.75);
    count=0; CHECK(CG_DrawStandings(CG_CAPFRAGLIMIT,40.75f,19,136,40,ITEM_ALIGN_CENTER,.25f,color,3)); NEAR(paints[0].x,33.75);
    Config(CS_SERVERINFO,""); Draw(CG_CAPFRAGLIMIT); Text(0,"0");
    Reset(GT_RACE); Config(CS_RACE_RECORDS,"3"); cg.race.pointsPassed=5; Draw(CG_CAPFRAGLIMIT); Text(0,"-2");
    Config(CS_RACE_RECORDS,"-2147483648"); cg.race.pointsPassed=INT_MAX; Draw(CG_CAPFRAGLIMIT); Text(0,"-4294967295");
    Config(CS_RACE_RECORDS,"3 junk"); Draw(CG_CAPFRAGLIMIT); Text(0,"-4294967295");
}
static void TeamRows(void) {
    Reset(GT_TEAM); snapshot.ps.persistant[PERS_TEAM]=TEAM_RED;
    Draw(CG_1ST_PLACE_SCORE); CHECK(count==2); Text(0,"      Red Team^7"); Text(1,"10"); NEAR(paints[1].x,162.75); NEAR(paints[1].alpha,.8);
    CHECK(CG_StandingsVisible(CG_SHOW_IF_PLYR_IS_FIRST_PLACE));
    Draw(CG_2ND_PLACE_SCORE); Text(0,"      Blue Team^7"); Text(1,"4");
    cgs.scores2=11; Draw(CG_1ST_PLACE_SCORE); Text(0,"      Blue Team^7"); Text(1,"11");
    Draw(CG_2ND_PLACE_SCORE); Text(0,"      Red Team^7"); Text(1,"10");
    CHECK(!CG_StandingsVisible(CG_SHOW_IF_RED_IS_FIRST_PLACE)); CHECK(CG_StandingsVisible(CG_SHOW_IF_BLUE_IS_FIRST_PLACE));
    CHECK(!CG_StandingsVisible(CG_SHOW_IF_PLYR_IS_FIRST_PLACE));
    cgs.clientinfo[0].team=TEAM_BLUE; CHECK(CG_StandingsVisible(CG_SHOW_IF_PLYR_IS_FIRST_PLACE));
    cgs.scores1=11; Draw(CG_1ST_PLACE_SCORE); Text(0,"      Red Team^7");
    CHECK(!CG_StandingsVisible(CG_SHOW_IF_PLYR_IS_FIRST_PLACE));
    CHECK(CG_StandingsVisible(CG_SHOW_IF_RED_IS_FIRST_PLACE)); CHECK(!CG_StandingsVisible(CG_SHOW_IF_BLUE_IS_FIRST_PLACE));
    Q_strncpyz(cg_scoreRedTeamName.string,"A very long custom red team",sizeof(cg_scoreRedTeamName.string));
    cgs.scores1=12345; Draw(CG_1ST_PLACE_SCORE); CHECK(count==2); Text(0,"      A very long custom red team"); Text(1,"1234");
    memset(cg_scoreRedTeamName.string,'R',100); cg_scoreRedTeamName.string[100]=0;
    Draw(CG_1ST_PLACE_SCORE); CHECK(strlen(paints[0].text)==106);
    cgs.scores2=INT_MIN; Draw(CG_2ND_PLACE_SCORE); Text(1,"-214");
    cgs.scores1=cgs.scores2=SCORE_NOT_PRESENT;
    Draw(CG_1ST_PLACE_SCORE); CHECK(count==1); Text(0,"      Red Team^7");
    Draw(CG_2ND_PLACE_SCORE); CHECK(count==1); Text(0,"      ");
    CHECK(CG_StandingsVisible(CG_SHOW_IF_RED_IS_FIRST_PLACE)); CHECK(!CG_StandingsVisible(CG_SHOW_IF_BLUE_IS_FIRST_PLACE));
}
static void IndividualRows(void) {
    Reset(GT_FFA); Draw(CG_1ST_PLACE_SCORE); CHECK(count==3); Text(0,"1. "); Text(1,"Leader"); Text(2," 10");
    NEAR(paints[0].x,40); NEAR(paints[1].x,61); NEAR(paints[2].x,155);
    CHECK(paints[0].style==3 && paints[1].style==0 && paints[2].style==3); NEAR(paints[2].alpha,1);
    CHECK(CG_StandingsVisible(CG_SHOW_IF_PLYR_IS_FIRST_PLACE));
    Draw(CG_2ND_PLACE_SCORE); Text(0,"2. "); Text(1,"Runner"); Text(2," 4");
    snapshot.ps.persistant[PERS_RANK]=4; snapshot.ps.persistant[PERS_SCORE]=-3;
    Draw(CG_2ND_PLACE_SCORE); Text(0,"5. "); Text(1,"Local"); Text(2," -3");
    snapshot.ps.persistant[PERS_RANK]=RANK_TIED_FLAG; Draw(CG_2ND_PLACE_SCORE); Text(0,"1. "); Text(1,"Local");
    CHECK(!CG_StandingsVisible(CG_SHOW_IF_PLYR_IS_FIRST_PLACE)); CHECK(CG_StandingsVisible(CG_SHOW_IF_PLYR_IS_NOT_FIRST_PLACE));
    snapshot.ps.persistant[PERS_TEAM]=TEAM_SPECTATOR; cgs.scores2=cgs.scores1;
    Draw(CG_2ND_PLACE_SCORE); Text(0,"1. "); Text(1,"Runner");
    snapshot.ps.pm_flags=PMF_FOLLOW; Draw(CG_2ND_PLACE_SCORE); Text(1,"Runner");
    snapshot.ps.persistant[PERS_TEAM]=TEAM_FREE; snapshot.ps.clientNum=MAX_CLIENTS;
    Draw(CG_2ND_PLACE_SCORE); CHECK(!count); snapshot.ps.clientNum=0;
    snapshot.ps.persistant[PERS_RANK]=-1; Draw(CG_2ND_PLACE_SCORE); CHECK(!count);
    snapshot.ps.persistant[PERS_RANK]=MAX_CLIENTS; Draw(CG_2ND_PLACE_SCORE); CHECK(!count);
    cgs.scores1=SCORE_NOT_PRESENT; Draw(CG_1ST_PLACE_SCORE); CHECK(count==1); Text(0,"1.");
    cgs.scores2=SCORE_NOT_PRESENT; Draw(CG_2ND_PLACE_SCORE); CHECK(!count);
    Reset(GT_RED_ROVER); Draw(CG_1ST_PLACE_SCORE); Text(1,"Leader"); // individual standings in RR
    Reset(GT_RACE); cgs.scores1=0; Draw(CG_1ST_PLACE_SCORE); Text(count-1," 0:00.000");
    cgs.scores2=0; Draw(CG_2ND_PLACE_SCORE); Text(2," -");
    cgs.scores1=61234; Draw(CG_1ST_PLACE_SCORE); Text(count-1," 1:01.234");
    cgs.scores1=INT_MAX; Draw(CG_1ST_PLACE_SCORE); Text(2," -");
    cgs.scores1=-1; Draw(CG_1ST_PLACE_SCORE); Text(2," -");
    snapshot.ps.persistant[PERS_RANK]=2; cgs.clientinfo[0].score=2345;
    Draw(CG_2ND_PLACE_SCORE); Text(count-1," 0:02.345");
}
static void Clipping(void) {
    Reset(GT_FFA); Config(CS_FIRST_PLACE,"^1abcdefghijklmno"); Draw(CG_1ST_PLACE_SCORE);
    CHECK(count==3); Text(1,"^1abcdefghijklmno"); NEAR(clipEdge,134);
    unchangedEdge=qtrue; Draw(CG_1ST_PLACE_SCORE); CHECK(count==3); Text(2," 10");
    unchangedEdge=qfalse;
    Config(CS_FIRST_PLACE,"123456789\xc3\xa9Z"); Draw(CG_1ST_PLACE_SCORE); Text(1,"123456789\xc3\xa9Z");
    Config(CS_FIRST_PLACE,"123456789\xf0\x9f\x98\x80Z"); Draw(CG_1ST_PLACE_SCORE); Text(1,"123456789\xf0\x9f\x98\x80Z");
    Config(CS_FIRST_PLACE,"1234567890"); Draw(CG_1ST_PLACE_SCORE); CHECK(count==3); Text(1,"1234567890");
    cgs.scores1=INT_MIN; Draw(CG_1ST_PLACE_SCORE); CHECK(count==3); Text(count-1," -2147483648");
    Config(CS_FIRST_PLACE,"abcdefghijklmnopqrstuvwxyzabcdefghijklmnopqrstuvwxyz");
    count=0; CHECK(CG_DrawStandings(CG_1ST_PLACE_SCORE,40.75f,19,1000,40,0,.25f,color,3)); CHECK(strlen(paints[1].text)==39);
    count=0; CHECK(CG_DrawStandings(CG_1ST_PLACE_SCORE,40.75f,19,0,40,0,.25f,color,3)); CHECK(count==3); CHECK(clipEdge<paints[1].x);
}
static void CountersAndBounds(void) {
    int mode;
    for (mode=0;mode<13;++mode) {
        qboolean round=mode==4 || mode==9 || mode==11 || mode==12;
        Reset(mode); Config(CS_RED_PLAYERS_LEFT,"3"); Config(CS_BLUE_PLAYERS_LEFT,"7");
        Config(CS_RACE_SCORES,"2"); Config(CS_RACE_INFO,"4"); snapshot.ps.persistant[PERS_TEAM]=TEAM_BLUE;
        CHECK(CG_StandingsVisible(CG_SHOW_PLAYERS_REMAINING)==round);
        CHECK(CG_StandingsVisible(CG_SHOW_DOMINATION)==(mode==10));
        Draw(CG_RED_CLAN_PLYRS); CHECK(count==(int)round); if (round) Text(0,"3");
        Draw(CG_BLUE_CLAN_PLYRS); if (round) Text(0,"7");
        Draw(CG_TEAM_PLYR_COUNT); if (round) Text(0,"7");
        Draw(CG_ENEMY_PLYR_COUNT); if (round) Text(0,"3");
        snapshot.ps.persistant[PERS_TEAM]=TEAM_SPECTATOR;
        Draw(CG_TEAM_PLYR_COUNT); if (round) Text(0,"0");
        Draw(CG_RED_OWNED_FLAGS); CHECK(count==(mode==10 || mode==11)); if (count) Text(0,"2");
        Draw(CG_BLUE_OWNED_FLAGS); if (count) Text(0,"4");
    }
    Reset(GT_CLAN_ARENA); Config(CS_RED_PLAYERS_LEFT,"5"); Config(CS_RED_PLAYERS_LEFT,"5x");
    Draw(CG_RED_CLAN_PLYRS); Text(0,"5");
    count=0; CHECK(CG_DrawStandings(CG_RED_CLAN_PLYRS,40.75f,19,136,40,ITEM_ALIGN_RIGHT,.25f,color,3)); NEAR(paints[0].x,40.75);
    configs[CS_RED_PLAYERS_LEFT]="8"; CG_InitStandings(); Draw(CG_RED_CLAN_PLYRS); Text(0,"8");
    count=0; CHECK(!CG_DrawStandings(-1,0,0,0,0,0,.25f,color,3)); CHECK(!count);
    CHECK(CG_DrawStandings(CG_1ST_PLACE_SCORE,NAN,19,136,40,0,.25f,color,3)); CHECK(!count);
    CHECK(CG_DrawStandings(CG_1ST_PLACE_SCORE,INFINITY,19,136,40,0,.25f,color,3)); CHECK(!count);
    CHECK(CG_DrawStandings(CG_1ST_PLACE_SCORE,2147483648.f,19,136,40,0,.25f,color,3)); CHECK(!count);
    CHECK(CG_DrawStandings(CG_1ST_PLACE_SCORE,40,19,-1,40,0,.25f,color,3)); CHECK(!count);
    cg.snap=NULL; Draw(CG_1ST_PLACE_SCORE); CHECK(!count); CHECK(!CG_StandingsVisible(CG_SHOW_IF_PLYR_IS_FIRST_PLACE));
}
int main(void) { Modes(); TeamRows(); IndividualRows(); Clipping(); CountersAndBounds(); puts("cgame standings: PASS"); return 0; }
