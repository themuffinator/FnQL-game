/* FnQL-game, Copyright (C) 2026 contributors. GPL-2.0-or-later. */
#include "../code/cgame/cg_local.h"
#include "../code/ui/ui_shared.h"
#include <stdio.h>
#include <stdlib.h>
#include <limits.h>
#include <math.h>
#define CHECK(x) do { if (!(x)) { fprintf(stderr,"line %d: %s\n",__LINE__,#x); exit(1); } } while (0)
#define NEAR(a,b) CHECK(fabs((a)-(b))<.001)
cg_t cg;
cgs_t cgs;
vmCvar_t cg_levelTimerDirection, cg_centertime, cg_announcerVolume;
static int localSounds, voiceSounds, clearCalls;
void CG_ClearMatchAnnouncements(void) { ++clearCalls; }
static char lastVoice[64];
static snapshot_t snapshot;
sfxHandle_t CG_QLVoice(const char *file) { snprintf(lastVoice,sizeof(lastVoice),"%s",file); return 42; }
static int bufferedVoices;
void CG_AddBufferedSound(sfxHandle_t sound) { CHECK(sound==42); ++bufferedVoices; }
sfxHandle_t trap_S_RegisterSound(const char *file,qboolean compressed) {
    CHECK(!strcmp(file,"sound/world/klaxon1.ogg") || !strcmp(file,"sound/items/kamikazerespawn.wav"));
    CHECK(compressed); return 41;
}
void trap_S_StartLocalSound(sfxHandle_t sound,int channel) {
    CHECK(sound==41 && channel==CHAN_LOCAL_SOUND); ++localSounds;
}
void trap_S_StartLocalSoundVolume(sfxHandle_t sound,int channel,float volume) {
    CHECK(sound==42 && channel==CHAN_ANNOUNCER && volume==.7f); ++voiceSounds;
}
static const char *argument;
static char printed[2048];
static float paintedAlpha;
static int paintedStyle;
static int glyphHeight=16;
const char *CG_Argv(int index) { CHECK(index==1); return argument; }
void QDECL CG_Printf(const char *format,...) {
    va_list args; va_start(args,format); vsnprintf(printed,sizeof(printed),format,args); va_end(args);
}
static const char *configs[MAX_CONFIGSTRINGS];
static qtime_t wallTime;
static int paints,font,align;
static char painted[128];
static float paintedX,paintedY;
static vec4_t inputColor={.2f,.4f,.6f,.8f};
const char *CG_ConfigString(int id) {
    CHECK(id>=0 && id<MAX_CONFIGSTRINGS); return configs[id] ? configs[id] : "";
}
void QDECL Com_Printf(const char *format,...) { (void)format; }
void QDECL Com_Error(int code,const char *format,...) { (void)code; (void)format; abort(); }
int trap_RealTime(qtime_t *value) { *value=wallTime; return 0; }
void CG_SetTextFont(int value) { font=value; }
int CG_Text_Width(const char *text,float scale,int limit) {
    CHECK((scale==.25f || scale==.4f || scale==.5f) && !limit); return Q_PrintStrlen(text)*7;
}
int CG_Text_Height(const char *text,float scale,int limit) {
    CHECK((scale==.4f || scale==.5f) && !limit); return *text ? glyphHeight : 0;
}
void CG_Text_Paint(float x,float y,float scale,vec4_t color,const char *text,float adjust,int limit,int style) {
    CHECK(!adjust && !limit);
    if (scale==.25f) CHECK(style==3 && !memcmp(color,inputColor,sizeof(vec4_t)));
    else { CHECK(scale==.4f || scale==.5f); CHECK(color[0]==1 && color[1]==1 && color[2]==1); }
    paintedAlpha=color[3]; paintedStyle=style;
    ++paints; paintedX=x; paintedY=y; snprintf(painted,sizeof(painted),"%s",text);
}
static void Reset(int mode) {
    int i;
    memset(&cg,0,sizeof(cg)); memset(&cgs,0,sizeof(cgs));
    for (i=0;i<MAX_CONFIGSTRINGS;++i) configs[i]=NULL;
    cgs.gametype=mode; cg.time=10000; cg_levelTimerDirection.integer=1;
    wallTime.tm_hour=13; wallTime.tm_min=7; wallTime.tm_mon=10; wallTime.tm_mday=5; wallTime.tm_year=126;
    paints=0; font=2; align=ITEM_ALIGN_LEFT;
    cg_centertime.value=3; argument=""; printed[0]=0;
    cg_announcerVolume.value=.7f; localSounds=voiceSounds=clearCalls=0; lastVoice[0]=0;
    memset(&snapshot,0,sizeof(snapshot));
    glyphHeight=16;
    CG_InitMatchClocks();
}
static void Config(int id,const char *text) {
    configs[id]=text; CG_MatchClockConfigChanged(id);
}
static void Draw(int owner,const char *expected) {
    int before=paints;
    CHECK(CG_DrawMatchClock(owner,123,45,align,.25f,inputColor,3));
    if (!expected) { CHECK(paints==before); return; }
    if (paints!=before+1 || strcmp(painted,expected)) { fprintf(stderr,"owner %d at %d: expected %s, got %s (paints %d)\n",owner,cg.time,expected,painted,paints-before); exit(1); }
    NEAR(paintedY,45);
    NEAR(paintedX,123-(align==ITEM_ALIGN_CENTER ? Q_PrintStrlen(expected)*3.5f :
        align==ITEM_ALIGN_RIGHT ? Q_PrintStrlen(expected)*7.f : 0));
}

static void Level(void) {
    Reset(GT_FFA); Config(CS_LEVEL_START_TIME,"1000"); Draw(CG_LEVELTIMER,"0:09"); CHECK(font==0);
    Config(CS_SERVERINFO,"\\timelimit\\2\\roundtimelimit\\180\\g_overtime\\120");
    cg.time=62000; Draw(CG_LEVELTIMER,"0:59");
    align=ITEM_ALIGN_CENTER; Draw(CG_LEVELTIMER,"0:59"); align=ITEM_ALIGN_RIGHT; Draw(CG_LEVELTIMER,"0:59");
    cg_levelTimerDirection.integer=0; Draw(CG_LEVELTIMER,"1:01");
    cg.time=121000; Draw(CG_LEVELTIMER,"2:00"); cg_levelTimerDirection.integer=1; Draw(CG_LEVELTIMER,"0:00");
    cg.time=121999; Draw(CG_LEVELTIMER,"0:00"); cg.time=122000; Draw(CG_LEVELTIMER,"0:01");
    cg_levelTimerDirection.integer=0; Draw(CG_LEVELTIMER,"0:01"); // overtime resets elapsed in either direction
    Config(CS_TIMEOUT_START_TIME,"122000"); cg.time=180000; Draw(CG_LEVELTIMER,"0:01");
    Config(CS_LEVEL_START_TIME,"59000"); Config(CS_TIMEOUT_START_TIME,""); Draw(CG_LEVELTIMER,"0:01");
    Config(CS_WARMUP,"\\time\\-1"); Draw(CG_LEVELTIMER,"0:00");
    Config(CS_WARMUP,"\\time\\0"); cg.time=58000; Draw(CG_LEVELTIMER,"0:00");
    Config(CS_SERVERINFO,"\\timelimit\\2147483647"); cg.time=59000; cg_levelTimerDirection.integer=1;
    Draw(CG_LEVELTIMER,"2147483647:00");
    Config(CS_SERVERINFO,"\\timelimit\\1\\roundtimelimit\\oops"); Draw(CG_LEVELTIMER,"2147483647:00");
    Config(CS_SERVERINFO,"\\timelimit\\-1"); Draw(CG_LEVELTIMER,"2147483647:00");
    Config(CS_LEVEL_START_TIME,"2147483648"); Draw(CG_LEVELTIMER,"2147483647:00");
    Config(CS_SERVERINFO,""); Config(CS_LEVEL_START_TIME,"-2147483648"); cg.time=INT_MAX;
    Draw(CG_LEVELTIMER,"71582:47");
}

static void Rounds(void) {
    int mode;
    for(mode=GT_FFA;mode<=GT_RED_ROVER;++mode) {
        qboolean round=mode==GT_CLAN_ARENA || mode==GT_FREEZE || mode==GT_ATTACK_DEFEND || mode==GT_RED_ROVER;
        Reset(mode); Config(CS_SERVERINFO,"\\timelimit\\5\\roundtimelimit\\30");
        Draw(CG_ROUND,round ? mode==GT_ATTACK_DEFEND ? "Round 1" : "Warmup" : NULL);
        Config(CS_ROUND_START_TIME,"1000"); cg.time=2000;
        Config(CS_MATCH_STATE,"\\round\\2"); Draw(CG_ROUND,round ? mode==GT_ATTACK_DEFEND ? "Round 3" : "Round 2" : NULL);
        Draw(CG_ROUNDTIMER,round ? "^129" : NULL);
        Config(CS_MATCH_STATE,"\\time\\10000\\round\\2"); Draw(CG_ROUNDTIMER,NULL);
        Draw(CG_LEVELTIMER,round ? "4:58" : "0:00");
        Config(CS_MATCH_STATE,"\\time\\0\\round\\2"); cg.time=1000; Draw(CG_ROUNDTIMER,NULL);
        cg.time=1001; Draw(CG_ROUNDTIMER,round ? "^129" : NULL);
        cg.time=30000; Draw(CG_ROUNDTIMER,round ? "^11" : NULL);
        cg.time=30001; Draw(CG_ROUNDTIMER,round ? "^10" : NULL);
        cg.time=31000; Draw(CG_ROUNDTIMER,NULL);
    }
    Reset(GT_CLAN_ARENA); Config(CS_SERVERINFO,"\\roundtimelimit\\30");
    Config(CS_ROUND_START_TIME,"1000"); Config(CS_MATCH_STATE,"\\round\\7");
    Config(CS_MATCH_STATE,"\\time\\1000\\round\\2147483648"); Draw(CG_ROUND,"Round 7"); Draw(CG_ROUNDTIMER,"^121");
    Config(CS_MATCH_STATE,"\\round\\3\\turn\\bad"); Draw(CG_ROUND,"Round 7");
    Config(CS_WARMUP,"\\time\\-1"); Draw(CG_ROUND,"Warmup"); Draw(CG_LEVELTIMER,"0:00");
    Config(CS_MATCH_STATE,"\\round\\7");
    Config(CS_ROUND_START_TIME,"1000 bad"); Draw(CG_ROUNDTIMER,"^121");
    Config(CS_TIMEOUT_START_TIME,"10000"); cg.frametime=1000;
    CG_AdvanceMatchClocks(); cg.time=11000; Draw(CG_ROUNDTIMER,"^121");
    CG_AdvanceMatchClocks(); CG_AdvanceMatchClocks(); cg.time=12000; Draw(CG_ROUNDTIMER,"^121"); // stereo doesn't double-adjust
    Config(CS_TIMEOUT_END_TIME,"15000"); CG_AdvanceMatchClocks(); cg.time=13000; Draw(CG_ROUNDTIMER,"^120");
    Config(CS_TIMEOUT_START_TIME,""); cg.time=15000; CG_AdvanceMatchClocks();
    Draw(CG_ROUNDTIMER,"^117"); // shipping final correction is -1 ms here
    Config(CS_ROUND_START_TIME,"6000"); Draw(CG_ROUNDTIMER,"^121"); // authoritative server replacement
    cg.time=10000; CG_AdvanceMatchClocks(); Draw(CG_ROUNDTIMER,"^126"); // rewind doesn't add a negative frame
    Reset(GT_ATTACK_DEFEND); Config(CS_MATCH_STATE,"\\turn\\1\\state\\2"); Draw(CG_ROUND,"Round 1");
    Config(CS_MATCH_STATE,"\\round\\2147483647"); Draw(CG_ROUND,"Round 1");
    configs[CS_WARMUP]="\\time\\-1"; configs[CS_MATCH_STATE]="\\round\\4";
    CG_InitMatchClocks(); Draw(CG_ROUND,"Round 5");
    cgs.gametype=GT_FFA; CG_InitMatchClocks(); Draw(CG_LEVELTIMER,"0:00"); // initialization honors mode, warmup first
}

static void OvertimeAndWallClock(void) {
    Reset(GT_FFA); Config(CS_SERVERINFO,"\\timelimit\\1\\g_overtime\\120");
    cg.time=60000; Draw(CG_OVERTIME,NULL); cg.time=60999; Draw(CG_OVERTIME,NULL);
    cg.time=61000; Draw(CG_OVERTIME,"Overtime"); cg.time=179999; Draw(CG_OVERTIME,"Overtime");
    cg.time=180000; align=ITEM_ALIGN_CENTER; Draw(CG_OVERTIME,"Overtime x2");
    cg.time=300000; align=ITEM_ALIGN_RIGHT; Draw(CG_OVERTIME,"Overtime x3");
    Config(CS_TIMEOUT_START_TIME,"180000"); Draw(CG_OVERTIME,"Overtime x2");
    Config(CS_WARMUP,"\\time\\-1"); Draw(CG_OVERTIME,NULL); Config(CS_WARMUP,"");
    cgs.gametype=GT_CLAN_ARENA; Draw(CG_OVERTIME,NULL); cgs.gametype=GT_FFA;
    Config(CS_SERVERINFO,"\\timelimit\\1"); Draw(CG_OVERTIME,NULL);
    Config(CS_SERVERINFO,"\\g_overtime\\120"); Draw(CG_OVERTIME,NULL);
    font=2; Draw(CG_LOCALTIME,"13:07 (Nov 05, 2026)"); CHECK(font==2);
    wallTime.tm_mon=0; wallTime.tm_hour=0; wallTime.tm_min=0; wallTime.tm_mday=1;
    Draw(CG_LOCALTIME,"00:00 (Jan 01, 2026)");
    wallTime.tm_mon=12; Draw(CG_LOCALTIME,NULL); wallTime.tm_mon=0;
    wallTime.tm_year=INT_MAX; Draw(CG_LOCALTIME,NULL);
    CHECK(!CG_DrawMatchClock(CG_PLAYER_HEALTH,0,0,0,.25f,inputColor,3));
}

static void Notices(void) {
    int before;
    char longText[1200];
    Reset(GT_FFA);
    CG_DrawMatchPause(); CHECK(!paints);
    Config(CS_TIMEOUT_START_TIME,"9000"); CG_DrawMatchPause();
    CHECK(!strcmp(painted,"Match Paused") && font==0 && paintedStyle==ITEM_TEXTSTYLE_SHADOWED);
    NEAR(paintedY,128); NEAR(paintedX,320-12*3.5f);
    Config(CS_TIMEOUT_END_TIME,"70000"); CG_DrawMatchPause();
    CHECK(!strcmp(painted,"Match resuming in ^560^7 seconds"));
    Config(CS_TIMEOUT_END_TIME,"11999"); CG_DrawMatchPause();
    CHECK(!strcmp(painted,"Match resuming in ^51^7 seconds"));
    cg.time=11000; before=paints; CG_DrawMatchPause(); CHECK(paints==before);
    cg.time=13000; CG_DrawMatchPause(); CHECK(!strcmp(painted,"Match resuming in ^5-1^7 seconds"));
    Config(CS_TIMEOUT_START_TIME,""); before=paints; CG_DrawMatchPause(); CHECK(paints==before);
    CHECK(!CG_MatchNoticeCommand(NULL) && !CG_MatchNoticeCommand("print"));
    argument="Paused ^1player\n"; cg.centerPrintTime=123;
    CHECK(CG_MatchNoticeCommand("pcp") && !cg.centerPrintTime && !strcmp(printed,argument));
    CHECK(CG_DrawMatchNotice()); CHECK(paintedStyle==ITEM_TEXTSTYLE_SHADOWEDMORE);
    /* Last empty line: top=int(90-12.8)=77, first ascent=16, gap=6. */
    NEAR(paintedY,99); NEAR(paintedAlpha,1);
    cg.time+=2800; CHECK(CG_DrawMatchNotice()); NEAR(paintedAlpha,1);
    cg.time+=100; CHECK(CG_DrawMatchNotice()); NEAR(paintedAlpha,.5f);
    cg.time+=100; before=paints; CHECK(!CG_DrawMatchNotice() && paints==before);
    argument="Hello"; Config(CS_WARMUP,"\\time\\-1");
    CHECK(CG_MatchNoticeCommand("cp") && CG_DrawMatchNotice());
    NEAR(paintedY,153); NEAR(paintedX,(640-35)/2); // integer centering
    CHECK(!strcmp(printed,"Paused ^1player\n"));
    CHECK(CG_MatchNoticeCommand("pcp") && CG_DrawMatchNotice()); NEAR(paintedY,99);
    cgs.gametype=GT_RACE; CHECK(CG_MatchNoticeCommand("pcp") && CG_DrawMatchNotice()); NEAR(paintedY,109);
    CG_ClearMatchNotice(); CHECK(!CG_DrawMatchNotice());
    cg.time=0; CHECK(CG_MatchNoticeCommand("pcp") && CG_DrawMatchNotice());
    cg.time=-1; CHECK(!CG_DrawMatchNotice());
    memset(longText,'x',sizeof(longText)); longText[sizeof(longText)-1]=0; argument=longText;
    CHECK(CG_MatchNoticeCommand("pcp") && CG_DrawMatchNotice()); CHECK(strlen(painted)==50);
    argument=""; CHECK(CG_MatchNoticeCommand("cp") && CG_DrawMatchNotice()); CHECK(!painted[0]);
    glyphHeight=INT_MAX; argument="one\ntwo";
    CHECK(CG_MatchNoticeCommand("pcp") && CG_DrawMatchNotice()); CHECK((double)paintedY>INT_MAX);
    glyphHeight=16;
    cg_centertime.value=INFINITY; CHECK(!CG_DrawMatchNotice());
    cg_centertime.value=3; CHECK(CG_MatchNoticeCommand("pcp"));
    CG_InitMatchClocks(); CHECK(!CG_DrawMatchNotice());
}

static void TimeoutAudio(void) {
    int mode;
    Reset(GT_CLAN_ARENA); Config(CS_TIMEOUT_START_TIME,"10000");
    CG_DrawMatchPause(); CHECK(localSounds==1 && !voiceSounds);
    CG_DrawMatchPause(); CHECK(localSounds==1);
    Config(CS_TIMEOUT_END_TIME,"16000");
    cg.time=11000; CG_DrawMatchPause(); CHECK(!strcmp(lastVoice,"prepare_to_fight") && voiceSounds==1);
    CG_DrawMatchPause(); CHECK(voiceSounds==1);
    cg.time=12000; CG_DrawMatchPause(); CHECK(voiceSounds==1);
    cg.time=13000; CG_DrawMatchPause(); CHECK(!strcmp(lastVoice,"three"));
    cg.time=14000; CG_DrawMatchPause(); CHECK(!strcmp(lastVoice,"two"));
    cg.time=15000; CG_DrawMatchPause(); CHECK(!strcmp(lastVoice,"one"));
    cg.time=15001; CG_DrawMatchPause(); CHECK(voiceSounds==4);
    cg.time=16000; CG_DrawMatchPause(); CHECK(!strcmp(lastVoice,"fight") && voiceSounds==5);
    CHECK(clearCalls==1);
    CHECK(CG_DrawMatchNotice() && !strcmp(painted,"FIGHT!")); NEAR(paintedY,128);
    for (mode=GT_FFA;mode<=GT_RED_ROVER;++mode) {
        qboolean round=mode==GT_CLAN_ARENA || mode==GT_FREEZE || mode==GT_ATTACK_DEFEND || mode==GT_RED_ROVER;
        Reset(mode); Config(CS_TIMEOUT_START_TIME,"9000"); Config(CS_TIMEOUT_END_TIME,"10500");
        CG_DrawMatchPause(); CHECK(voiceSounds==round); CHECK(CG_DrawMatchNotice()==round);
    }
    Reset(GT_ATTACK_DEFEND); cg.snap=&snapshot; snapshot.ps.persistant[PERS_TEAM]=TEAM_RED;
    Config(CS_MATCH_STATE,"\\round\\1\\turn\\0");
    Config(CS_TIMEOUT_START_TIME,"9000"); Config(CS_TIMEOUT_END_TIME,"10500");
    CG_DrawMatchPause(); CHECK(!strcmp(lastVoice,"attack_the_flag"));
    CHECK(CG_DrawMatchNotice() && !strcmp(painted,"ATTACK THE FLAG!"));
    Config(CS_MATCH_STATE,"\\round\\1\\turn\\1"); cg.time+=1000; Config(CS_TIMEOUT_END_TIME,"11500");
    CG_DrawMatchPause(); CHECK(!strcmp(lastVoice,"defend_the_flag"));
    snapshot.ps.persistant[PERS_TEAM]=TEAM_BLUE; cg.time+=1000; Config(CS_TIMEOUT_END_TIME,"12500");
    CG_DrawMatchPause(); CHECK(!strcmp(lastVoice,"attack_the_flag"));
    Reset(GT_RED_ROVER); cg.snap=&snapshot; snapshot.ps.pm_flags=PMF_INFECTED;
    Config(CS_MATCH_STATE,"\\round\\1"); Config(CS_TIMEOUT_START_TIME,"9000"); Config(CS_TIMEOUT_END_TIME,"10500");
    CG_DrawMatchPause(); CHECK(!strcmp(lastVoice,"bite") && localSounds==2);
    CHECK(CG_DrawMatchNotice() && !strcmp(painted,"BITE!"));
}

/* Retail 0x1004395d time warnings. */
static void Warn(int time,const char *expected) {
    int before=bufferedVoices;
    lastVoice[0]=0; cg.time=time; CG_MatchClockWarnings();
    if (expected) CHECK(bufferedVoices==before+1 && !strcmp(lastVoice,expected));
    else CHECK(bufferedVoices==before);
}
static void TimeWarnings(void) {
    Reset(GT_FFA); bufferedVoices=0;
    Config(CS_SERVERINFO,"\\timelimit\\10\\roundtimelimit\\0\\g_overtime\\0");
    Config(CS_LEVEL_START_TIME,"1000");
    Warn(1000+300000,NULL);                 /* strictly after the moment */
    Warn(1000+300001,"5_minute"); CHECK(cg.timelimitWarnings==1);
    Warn(1000+400000,NULL);
    Warn(1000+540001,"1_minute"); CHECK(cg.timelimitWarnings==3);
    Warn(1000+602000,NULL);
    Warn(1000+602001,"sudden_death"); CHECK(cg.timelimitWarnings==7);
    Warn(1000+700000,NULL);
    /* Overtime replaces sudden death outside round modes, still marking it. */
    cg.timelimitWarnings=0; Config(CS_SERVERINFO,"\\timelimit\\10\\roundtimelimit\\0\\g_overtime\\120");
    Warn(1000+602001,NULL); CHECK(cg.timelimitWarnings==7);
    /* Moments before the local player joined stay silent; later ones play. */
    cg.timelimitWarnings=0; cg.teamJoinTime=1000+450000;
    Warn(1000+450000,NULL); CHECK(cg.timelimitWarnings==0);
    Warn(1000+540001,"1_minute");
    cg.teamJoinTime=0;
    /* No five-minute warning at five minutes or less; none in warmup,
     * timeouts or without a limit. */
    cg.timelimitWarnings=0; Config(CS_SERVERINFO,"\\timelimit\\5\\roundtimelimit\\0\\g_overtime\\0");
    Warn(1000+10000,NULL); Warn(1000+240001,"1_minute");
    cg.timelimitWarnings=0; Config(CS_WARMUP,"\\time\\5000"); Warn(1000+240001,NULL);
    Config(CS_WARMUP,"\\time\\0"); Config(CS_TIMEOUT_START_TIME,"200000"); Warn(1000+240001,NULL);
    Config(CS_TIMEOUT_START_TIME,"0"); Config(CS_SERVERINFO,"\\timelimit\\0\\roundtimelimit\\0\\g_overtime\\0");
    Warn(1000+240001,NULL);
    /* Round modes: 30 seconds before the round limit, re-armed by each
     * round-state update; sudden death plays even with overtime. */
    Reset(GT_CLAN_ARENA); bufferedVoices=0;
    Config(CS_SERVERINFO,"\\timelimit\\10\\roundtimelimit\\180\\g_overtime\\120");
    Config(CS_LEVEL_START_TIME,"1000"); Config(CS_ROUND_START_TIME,"5000");
    Warn(154998,NULL);
    Warn(154999,"30_second_warning"); CHECK(cg.timelimitWarnings==8);
    Warn(160000,NULL);
    Config(CS_MATCH_STATE,"\\time\\0\\round\\2\\turn\\0"); CHECK(cg.timelimitWarnings==0);
    Warn(160000,"30_second_warning");
    cg.timelimitWarnings=0; Warn(185005,NULL);  /* past the round limit */
    Config(CS_ROUND_START_TIME,"0"); Warn(150000,"30_second_warning"); /* no -1 without a start */
    cg.timelimitWarnings=0; Warn(1000+602001,"sudden_death");
}

int main(void) {
    Level(); Rounds(); OvertimeAndWallClock(); Notices(); TimeoutAudio(); TimeWarnings();
    puts("cgame match clocks: config transitions, level/round/overtime/timeouts, local time, alignment, bounds and time warnings OK");
    return 0;
}
