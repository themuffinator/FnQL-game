/* FnQL-game, Copyright (C) 2026 contributors. GPL-2.0-or-later. */
#include "cg_local.h"
#include "../ui/ui_shared.h"
#include <errno.h>
#include <limits.h>
#include <stdint.h>

/* Retail 10048c80/10048d10 and 1000f090. Keep presentation state private;
 * the snapshot remains authoritative for the viewed player and match. */
static struct {
    int time, mode, round, turn, state, count;
    int minimum, balance, freezeDelay, deadline, intermission;
    uint32_t settings;
    qboolean ready[MAX_CLIENTS];
} pregame;

static qboolean RoundMode(int mode) {
    return mode==GT_CLAN_ARENA || mode==GT_FREEZE || mode==GT_ATTACK_DEFEND || mode==GT_RED_ROVER;
}
static qboolean Number(const char *text,int *value) {
    char *end; long number;
    if (!text || !*text) { *value=0; return qtrue; }
    errno=0; number=strtol(text,&end,10);
    if (end==text || *end || errno || number<INT_MIN || number>INT_MAX) return qfalse;
    *value=(int)number; return qtrue;
}
static qboolean Field(const char *text,const char *key,int *value) {
    return Number(Info_ValueForKey(text,key),value);
}
static void Voice(const char *name) {
    trap_S_StartLocalSoundVolume(CG_QLVoice(name),CHAN_ANNOUNCER,cg_announcerVolume.value);
}
void CG_PregameConfigChanged(int index) {
    const char *text=CG_ConfigString(index);
    int a,b,c,d;
    if (index>=CS_PLAYERS && index<CS_PLAYERS+MAX_CLIENTS) {
        if (Field(text,"rp",&a)) pregame.ready[index-CS_PLAYERS]=a!=0;
        return;
    }
    switch (index) {
    case CS_SERVERINFO:
        /* Shipping 10048993 reads g_teamSizeMin, not the teamsize alias. */
        if (!Field(text,"g_teamSizeMin",&a) || a<0 ||
            !Field(text,"g_teamForceBalance",&b) || !Field(text,"g_freezeRoundDelay",&c) || c<0) return;
        pregame.minimum=a; pregame.balance=b; pregame.freezeDelay=c; break;
    case CS_WARMUP:
        if (!Field(text,"time",&a)) return;
        text=Info_ValueForKey(text,"g_gametype");
        if (*text) { if (!Number(text,&b)) return; } else b=-1;
        /* 10048d70: preparation is a transition, not an every-frame sound. */
        if (a>0 && pregame.time<=0) {
            qboolean solo=b<GT_TEAM && (cgs.gametype<GT_TEAM || b==GT_TOURNAMENT ||
                cgs.gametype==GT_RED_ROVER || b==GT_RED_ROVER);
            Voice(solo ? "prepare_to_fight" : "prepare_your_team");
        }
        pregame.time=cg.warmup=a; pregame.mode=b; pregame.round=0;
        if (a) { cg.timelimitWarnings|=7; cg.fraglimitWarnings|=7; }
        trap_Cvar_Set("ui_warmup",va("%d",a)); break;
    case CS_MATCH_STATE:
        if (!Field(text,"time",&a) || !Field(text,"round",&b) ||
            !Field(text,"turn",&c) || !Field(text,"state",&d)) return;
        if (cgs.gametype==GT_ATTACK_DEFEND) { if (b==INT_MAX) return; ++b; }
        pregame.time=cg.warmup=a; pregame.round=b; pregame.turn=c; pregame.state=d; break;
    case CS_READY_UP:
        if (Number(text,&a)) pregame.deadline=a; break;
    case CS_INTERMISSION:
        if (Number(text,&a)) pregame.intermission=a; break;
    case CS_SETTINGS_MASK: {
        char *end; long long value;
        if (!*text) { pregame.settings=0; break; }
        errno=0; value=strtoll(text,&end,10);
        if (end!=text && !*end && !errno && value>=INT_MIN && value<=UINT32_MAX)
            pregame.settings=(uint32_t)value;
        break;
    }
    default: break;
    }
}
void CG_InitPregame(void) {
    int i;
    static const int configs[]={CS_SERVERINFO,CS_READY_UP,CS_INTERMISSION,CS_SETTINGS_MASK,CS_WARMUP};
    memset(&pregame,0,sizeof(pregame)); pregame.mode=pregame.count=-1;
    for (i=0;i<ARRAY_LEN(configs);++i) CG_PregameConfigChanged(configs[i]);
    if (RoundMode(cgs.gametype)) CG_PregameConfigChanged(CS_MATCH_STATE);
    for (i=0;i<MAX_CLIENTS;++i) CG_PregameConfigChanged(CS_PLAYERS+i);
}
void CG_PregameRestart(void) {
    pregame.count=-1; pregame.deadline=0;
    /* 1004a3f4: non-round modes announce the warmup-free map restart. */
    if (!pregame.time && !RoundMode(cgs.gametype)) CG_AnnounceMatchStart();
}

static void Line(const char *text,float y,float scale,int font,int style,vec4_t color) {
    CG_SetTextFont(font);
    CG_Text_Paint(320-CG_Text_Width(text,scale,0)/2,y,scale,color,text,0,0,style);
}

static void Waiting(int mode,const int *counts,int local) {
    char first[128]="The match will begin",second[128]="when more players are ready.";
    qboolean enough;
    int missing=0,team=0,minimum=pregame.minimum;
    float offset=cgs.gametype==GT_RACE ? 10.f : 0.f; /* retail float 10078fbc */
    vec4_t white={1,1,1,1};
    if (mode==GT_RED_ROVER) {
        enough=(pregame.settings&(1u<<26)) ? counts[TEAM_RED]+counts[TEAM_BLUE]>=minimum :
            counts[TEAM_RED]>=minimum && counts[TEAM_BLUE]>=minimum;
        if (!enough) Q_strncpyz(second,"when more players join.",sizeof(second));
    } else if (mode>=GT_TEAM) {
        if (counts[TEAM_RED]<minimum && counts[TEAM_BLUE]<minimum) {
            Q_strncpyz(first,"Waiting for more players.",sizeof(first));
            snprintf(second,sizeof(second),"The match requires %i player%s per team.",minimum,minimum==1 ? "" : "s");
        } else if (counts[TEAM_RED]<minimum || counts[TEAM_BLUE]<minimum) {
            team=counts[TEAM_RED]<minimum ? TEAM_RED : TEAM_BLUE;
            missing=minimum-counts[team];
            snprintf(first,sizeof(first),"Waiting for %i more player%s",missing,missing==1 ? "" : "s");
            snprintf(second,sizeof(second),"to join the %s.",team==TEAM_RED ? "Red Team" : "Blue Team");
        } else if (pregame.balance && abs(counts[TEAM_RED]-counts[TEAM_BLUE])>1) {
            Q_strncpyz(first,"The teams must be balanced",sizeof(first));
            Q_strncpyz(second,"before the match can begin.",sizeof(second));
        }
    } else if (mode==GT_TOURNAMENT && counts[TEAM_FREE]>2) {
        Q_strncpyz(first,"The match will begin when",sizeof(first));
        Q_strncpyz(second,"fewer players are in the match.",sizeof(second));
    } else if (counts[TEAM_FREE]<2) Q_strncpyz(second,"when more players join.",sizeof(second));
    else if (pregame.deadline) {
        int64_t seconds=((int64_t)pregame.deadline-cg.time)/1000+1;
        if (seconds<0) seconds=0;
        Q_strncpyz(first,"Players must ready",sizeof(first));
        snprintf(second,sizeof(second),"within %lld second%s.",(long long)seconds,seconds==1 ? "" : "s");
    }
    if (cg_drawPregameMessages.integer) {
        Line(first,88+offset,.35f,1,ITEM_TEXTSTYLE_SHADOWED,white);
        Line(second,108+offset,.35f,1,ITEM_TEXTSTYLE_SHADOWED,white);
    }
    /* 1000f571 uses the local clientinfo team/ready field, not EF_READY. */
    enough=cgs.gametype<GT_TEAM ? counts[TEAM_FREE]>1 :
        counts[TEAM_RED]>=minimum && counts[TEAM_BLUE]>=minimum;
    if (cgs.gametype==GT_RED_ROVER && (pregame.settings&(1u<<26)) &&
        counts[TEAM_RED]+counts[TEAM_BLUE]>=minimum) enough=qtrue;
    if (enough && local>=0 && local<MAX_CLIENTS && cgs.clientinfo[local].infoValid &&
        cgs.clientinfo[local].team!=TEAM_SPECTATOR) {
        char key[32]={0},text[96];
        vec4_t ink={1,0,0,1};
        trap_Key_KeynumToStringBuf(trap_Key_GetKey("readyup"),key,sizeof(key));
        key[sizeof(key)-1]=0;
        if (pregame.ready[local]) { ink[0]=0; ink[1]=1; }
        snprintf(text,sizeof(text),"Press %s to %s yourself",key,pregame.ready[local] ? "unready" : "ready");
        Line(text,124+offset,.25f,0,ITEM_TEXTSTYLE_SHADOWEDMORE,ink);
    }
}

static void Countdown(int mode,const int *duel) {
    static const char *const modes[]={"Free For All","Duel","Race","Team Deathmatch","Clan Arena",
        "Capture the Flag","One Flag CTF","Overload","Harvester","Freeze Tag","Domination","Attack and Defend","Red Rover"};
    char title[128],count[48];
    const char *name=mode>=0 && mode<ARRAY_LEN(modes) ? modes[mode] : "Unknown Gametype";
    float offset=cgs.gametype==GT_RACE ? 10.f : 0.f;
    vec4_t white={1,1,1,1};
    if (mode==GT_TOURNAMENT) {
        if (duel[0]>=0 && duel[1]>=0) {
            snprintf(title,sizeof(title),"%s vs %s",cgs.clientinfo[duel[0]].name,cgs.clientinfo[duel[1]].name);
            name=title;
        } else name="";
    } else if (mode==GT_FFA && (pregame.settings&(1u<<18))) name="Quad Hog";
    else if (mode==GT_RED_ROVER && !pregame.round && (pregame.settings&(1u<<26))) name="Infected";
    else if (mode==GT_ATTACK_DEFEND && pregame.state==1) {
        int team=cg.clientNum>=0 && cg.clientNum<MAX_CLIENTS ? cgs.clientinfo[cg.clientNum].team : TEAM_SPECTATOR;
        name=team==TEAM_RED || team==TEAM_BLUE ?
            ((team==TEAM_RED)==(pregame.turn==0) ? "ATTACK THE FLAG" : "DEFEND THE FLAG") :
            (pregame.turn==0 ? "Red is on offense" : "Blue is on offense");
    } else if (mode==GT_FREEZE && !pregame.freezeDelay) name="Freeze Tag";
    else if (RoundMode(mode) && pregame.round>0) {
        name="Round Begins in";
        if (cgs.timelimit>0 && (cg.timelimitWarnings&4) &&
            (int64_t)cg.time-cgs.levelStartTime>(int64_t)cgs.timelimit*60000) name="Sudden Death";
    }
    Line(name,90+offset,.6f,0,ITEM_TEXTSTYLE_SHADOWEDMORE,white);
    if (RoundMode(mode) && pregame.round>0) snprintf(count,sizeof(count),"%d",pregame.count);
    else snprintf(count,sizeof(count),"Starts in: %d",pregame.count);
    Line(count,125+offset,.45f,0,ITEM_TEXTSTYLE_SHADOWEDMORE,white);
}

void CG_DrawPregame(void) {
    int i,counts[4]={0},duel[2]={-1,-1},mode;
    int64_t remaining;
    if (!cg.snap || pregame.intermission) return;
    if (pregame.time!=-1 && (pregame.time || pregame.count!=-1)) {
        remaining=((int64_t)pregame.time-cg.time+1000)/1000;
        if (remaining<0) remaining=0;
        if (remaining!=pregame.count) {
            pregame.count=(int)remaining;
            if (!RoundMode(cgs.gametype) || pregame.round>0 || (cgs.gametype==GT_FREEZE && !pregame.freezeDelay)) {
                switch (pregame.count) {
                case 5:
                    if (RoundMode(cgs.gametype) && (cgs.gametype!=GT_FREEZE || pregame.freezeDelay>=5000)) Voice("round_begins_in");
                    break;
                case 3: Voice("three"); break;
                case 2: Voice("two"); break;
                case 1: Voice("one"); break;
                case 0: if (RoundMode(cgs.gametype)) CG_AnnounceMatchStart(); break;
                default: break;
                }
            }
        }
    } else pregame.count=-1;
    /* 1000f1c4 hides text while scores or the join menu are displayed;
     * countdown audio above still advances. Never acquire input here. */
    if (cg.showScores || (trap_Key_GetCatcher()&KEYCATCH_CGAME)) return;
    mode=pregame.mode==-1 ? cgs.gametype : pregame.mode;
    for (i=0;i<cgs.maxclients && i<MAX_CLIENTS;++i) if (cgs.clientinfo[i].infoValid) {
        int team=cgs.clientinfo[i].team;
        if (team>=TEAM_FREE && team<=TEAM_SPECTATOR) ++counts[team];
        if (team==TEAM_FREE) { if (duel[0]<0) duel[0]=i; else duel[1]=i; }
    }
    if (pregame.count>0) Countdown(mode,duel);
    else if (pregame.time<0) Waiting(mode,counts,cg.clientNum);
}
