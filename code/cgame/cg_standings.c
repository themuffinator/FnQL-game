/* FnQL-game, Copyright (C) 2026 contributors. GPL-2.0-or-later. */
#include "cg_local.h"
#include "../ui/ui_shared.h"
#include <errno.h>
#include <limits.h>
#include <stdint.h>
#include <float.h>

/* Presentation inputs from retail configstrings, not scoreboard row order.
 * Names: 100494af/10049b0f; counts: 10049aca/10049aee/1004a036/1004a04f. */
static struct {
    char names[2][40];
    int limits[4], remaining[2], owned[2], racePoints;
    qhandle_t icons[13];
} standings;

static qboolean Number(const char *text,int *result) {
    char *end;
    long value;
    if (!text || !*text) { *result=0; return qtrue; }
    errno=0; value=strtol(text,&end,10);
    if (end==text || *end || errno || value<INT_MIN || value>INT_MAX) return qfalse;
    *result=(int)value; return qtrue;
}

void CG_StandingsConfigChanged(int index) {
    int value[4], i;
    const char *text=CG_ConfigString(index);
    static const char *const keys[]={"fraglimit","capturelimit","roundlimit","scorelimit"};
    switch (index) {
    case CS_FIRST_PLACE: case CS_SECOND_PLACE:
        Q_strncpyz(standings.names[index-CS_FIRST_PLACE],text,sizeof(standings.names[0])); break;
    case CS_SERVERINFO:
        for (i=0;i<4;++i) if (!Number(Info_ValueForKey(text,keys[i]),&value[i])) return;
        memcpy(standings.limits,value,sizeof(value)); break;
    case CS_RED_PLAYERS_LEFT: case CS_BLUE_PLAYERS_LEFT:
        if (Number(text,&value[0])) standings.remaining[index-CS_RED_PLAYERS_LEFT]=value[0]; break;
    case CS_RACE_SCORES: case CS_RACE_INFO:
        /* These slots hold owned-point counts in Domination/AD. */
        if (Number(text,&value[0])) standings.owned[index-CS_RACE_SCORES]=value[0]; break;
    case CS_RACE_RECORDS:
        if (Number(text,&value[0])) standings.racePoints=value[0]; break;
    default: break;
    }
}

void CG_InitStandings(void) {
    static const int inputs[]={CS_SERVERINFO,CS_FIRST_PLACE,CS_SECOND_PLACE,
        CS_RED_PLAYERS_LEFT,CS_BLUE_PLAYERS_LEFT,CS_RACE_SCORES,CS_RACE_INFO,CS_RACE_RECORDS};
    int i;
    memset(&standings,0,sizeof(standings));
    for (i=0;i<ARRAY_LEN(inputs);++i) CG_StandingsConfigChanged(inputs[i]);
}

/* First/second place names (retail 0x10a40400/0x10a40428). */
const char *CG_StandingsName(int slot) { return standings.names[slot ? 1 : 0]; }

static qboolean TeamStandings(void) { return cgs.gametype>=GT_TEAM && cgs.gametype!=GT_RED_ROVER; }
static qboolean RoundMode(void) {
    return cgs.gametype==GT_CLAN_ARENA || cgs.gametype==GT_FREEZE ||
        cgs.gametype==GT_ATTACK_DEFEND || cgs.gametype==GT_RED_ROVER;
}
static qboolean LocalFirst(void) {
    int client,team;
    if (!cg.snap || cg.snap->ps.persistant[PERS_TEAM]==TEAM_SPECTATOR) return qfalse;
    client=cg.snap->ps.clientNum;
    if (client<0 || client>=MAX_CLIENTS) return qfalse;
    /* 1002a500: ties do not make an individual first; red wins team ties. */
    if (!TeamStandings()) return cg.snap->ps.persistant[PERS_RANK]==0;
    team=cgs.clientinfo[client].team;
    return (team==TEAM_RED && cgs.scores1>=cgs.scores2) ||
        (team==TEAM_BLUE && cgs.scores2>cgs.scores1);
}

qboolean CG_StandingsVisible(int flags) {
    /* 10031790: these tests also apply outside team modes. */
    if ((flags & CG_SHOW_PLAYERS_REMAINING) && RoundMode()) return qtrue;
    if ((flags & CG_SHOW_DOMINATION) && cgs.gametype==GT_DOMINATION) return qtrue;
    if (flags & CG_SHOW_IF_PLYR_IS_FIRST_PLACE) return LocalFirst();
    if (flags & CG_SHOW_IF_PLYR_IS_NOT_FIRST_PLACE) return !LocalFirst();
    if (flags & CG_SHOW_IF_RED_IS_FIRST_PLACE)
        return cgs.scores1==SCORE_NOT_PRESENT || cgs.scores1>=cgs.scores2;
    if (flags & CG_SHOW_IF_BLUE_IS_FIRST_PLACE)
        return cgs.scores2!=SCORE_NOT_PRESENT && cgs.scores2>cgs.scores1;
    return qfalse;
}

typedef struct {
    int value, rank, team;
    qboolean present;
    char name[MAX_CVAR_VALUE_STRING];
} hudRow_t;

static qboolean SelectRow(int slot,hudRow_t *row) {
    int client,rank;
    memset(row,0,sizeof(*row));
    row->value=slot ? cgs.scores2 : cgs.scores1;
    row->present=row->value!=SCORE_NOT_PRESENT;
    if (TeamStandings()) {
        const char *custom;
        row->team=slot ? TEAM_BLUE : TEAM_RED;
        if (row->present && cgs.scores1<cgs.scores2) row->team=slot ? TEAM_RED : TEAM_BLUE;
        row->value=row->team==TEAM_RED ? cgs.scores1 : cgs.scores2;
        custom=row->team==TEAM_RED ? cg_scoreRedTeamName.string : cg_scoreBlueTeamName.string;
        /* Shipping absent-score branch reverses the usual empty-name test. */
        if ((row->present && !*custom) || (!row->present && *custom))
            custom=row->team==TEAM_RED ? "Red Team^7" : "Blue Team^7";
        Q_strncpyz(row->name,custom,sizeof(row->name));
        return qtrue;
    }
    row->rank=slot && cgs.scores1!=cgs.scores2 ? 2 : 1;
    if (!row->present) return !slot;
    Q_strncpyz(row->name,standings.names[slot],sizeof(row->name));
    if (!slot || !cg.snap || cg.snap->ps.persistant[PERS_TEAM]==TEAM_SPECTATOR || LocalFirst()) return qtrue;
    client=cg.snap->ps.clientNum;
    rank=cg.snap->ps.persistant[PERS_RANK] & ~RANK_TIED_FLAG;
    if (client<0 || client>=MAX_CLIENTS || rank<0 || rank>=MAX_CLIENTS) return qfalse;
    row->rank=rank+1;
    row->value=cgs.gametype==GT_RACE ? cgs.clientinfo[client].score : cg.snap->ps.persistant[PERS_SCORE];
    Q_strncpyz(row->name,cgs.clientinfo[client].name,sizeof(standings.names[0]));
    return qtrue;
}

static void PaintRow(int slot,float x,float y,float w,float scale,vec4_t color,int style) {
    hudRow_t row;
    char prefix[8],value[16],label[MAX_CVAR_VALUE_STRING+8];
    vec4_t ink;
    float start,edge,scoreX,returnedEdge;
    int rankWidth,scoreWidth,dotsWidth;
    if (!SelectRow(slot,&row)) return;
    if (row.team) {
        snprintf(label,sizeof(label),"      %s",row.name);
        CG_Text_Paint(x,y,scale,color,label,0,0,style);
        if (row.present) {
            /* 10032701/10032b79: at most four score characters. */
            snprintf(value,5,"%d",row.value);
            CG_Text_Paint(x+w-CG_Text_Width(value,scale,0),y,scale,color,value,0,0,style);
        }
        return;
    }
    if (!row.present) { CG_Text_Paint(x,y,scale,color,"1.",0,0,style); return; }
    snprintf(prefix,sizeof(prefix),"%d. ",row.rank);
    if (cgs.gametype==GT_RACE) {
        if (row.value==INT_MAX || row.value<0 || (slot && !row.value)) Q_strncpyz(value," -",sizeof(value));
        else snprintf(value,sizeof(value)," %d:%02d.%03d",row.value/60000,row.value/1000%60,row.value%1000);
    } else snprintf(value,sizeof(value)," %d",row.value);
    rankWidth=CG_Text_Width(prefix,scale,0); scoreWidth=CG_Text_Width(value,scale,0);
    dotsWidth=CG_Text_Width("...",scale,0);
    /* 10032852/10032dcf reserve ellipsis width. Shipping 100087c0 passes
     * a LOCAL boundary copy to the renderer, leaving the caller's value
     * unchanged. The equal-boundary branches 100328f1/10032e77 therefore
     * hide the dots even on clipped names (confirmed by native captures). */
    start=(float)(int)x+rankWidth;
    edge=start+truncf(w-rankWidth-scoreWidth-dotsWidth);
    scoreX=edge+dotsWidth;
    CG_Text_Paint((float)(int)x,y,scale,color,prefix,0,0,style);
    Vector4Copy(color,ink);
    returnedEdge=edge;
    CG_Text_PaintClipped(&returnedEdge,start,y,scale,color,row.name);
    ink[3]=1;
    CG_Text_Paint(scoreX,y,scale,ink,value,0,0,style);
}

qboolean CG_DrawStandings(int owner,float x,float y,float w,float h,int align,float scale,vec4_t color,int style) {
    char text[32];
    int team=0,index=0;
    int64_t value=0;
    switch (owner) {
    case CG_GAME_TYPE_ICON: case CG_CAPFRAGLIMIT: case CG_1ST_PLACE_SCORE: case CG_2ND_PLACE_SCORE:
    case CG_RED_CLAN_PLYRS: case CG_BLUE_CLAN_PLYRS: case CG_TEAM_PLYR_COUNT: case CG_ENEMY_PLYR_COUNT:
    case CG_RED_OWNED_FLAGS: case CG_BLUE_OWNED_FLAGS: break;
    default: return qfalse;
    }
    if (!cg.snap || !(x>=INT_MIN && (double)x<=INT_MAX && y>=-FLT_MAX && y<=FLT_MAX &&
        w>=0 && w<=INT_MAX && h>=0 && h<=INT_MAX && scale>0 && scale<=FLT_MAX)) return qtrue;
    CG_SetTextFont(0);
    if (owner==CG_GAME_TYPE_ICON) {
        static const char *const icons[]={"ffa","duel","race","tdm","ca","ctf","1f","ffa","har","ft","dom","ad","rr"};
        int mode=cgs.gametype>=0 && cgs.gametype<13 ? cgs.gametype : 0;
        if (!standings.icons[mode]) {
            snprintf(text,sizeof(text),"ui/assets/hud/%s.tga",icons[mode]);
            standings.icons[mode]=trap_R_RegisterShader(text);
        }
        trap_R_SetColor(NULL); CG_DrawPic(x,y,w,h,standings.icons[mode]); return qtrue;
    }
    if (owner==CG_1ST_PLACE_SCORE || owner==CG_2ND_PLACE_SCORE) {
        PaintRow(owner==CG_2ND_PLACE_SCORE,x,y,w,scale,color,style); return qtrue;
    }
    if (owner==CG_CAPFRAGLIMIT) {
        /* 10032260: distinct capture/round/score buckets, race checkpoints. */
        if (cgs.gametype==GT_RACE) value=(int64_t)standings.racePoints-cg.race.pointsPassed;
        else {
            if (cgs.gametype>=GT_CTF && cgs.gametype<=GT_HARVESTER) index=1;
            else if (cgs.gametype==GT_DOMINATION || cgs.gametype==GT_ATTACK_DEFEND) index=3;
            else if (RoundMode()) index=2;
            value=standings.limits[index];
        }
    } else {
        if (owner==CG_RED_OWNED_FLAGS || owner==CG_BLUE_OWNED_FLAGS) {
            if (cgs.gametype!=GT_DOMINATION && cgs.gametype!=GT_ATTACK_DEFEND) return qtrue;
            value=standings.owned[owner==CG_BLUE_OWNED_FLAGS];
        } else {
            if (!RoundMode()) return qtrue;
            if (owner==CG_RED_CLAN_PLYRS) team=TEAM_RED;
            else if (owner==CG_BLUE_CLAN_PLYRS) team=TEAM_BLUE;
            else {
                team=cg.snap->ps.persistant[PERS_TEAM];
                if (owner==CG_ENEMY_PLYR_COUNT && (team==TEAM_RED || team==TEAM_BLUE)) team=3-team;
            }
            value=team==TEAM_RED || team==TEAM_BLUE ? standings.remaining[team-TEAM_RED] : 0;
        }
        align=ITEM_ALIGN_LEFT; /* 100335f0/10033650/100337a0 use baseline origin. */
    }
    snprintf(text,sizeof(text),"%lld",(long long)value);
    if (align==ITEM_ALIGN_CENTER) x-=CG_Text_Width(text,scale,0)*.5f;
    else if (align==ITEM_ALIGN_RIGHT) x-=CG_Text_Width(text,scale,0);
    CG_Text_Paint(x,y,scale,color,text,0,0,style);
    return qtrue;
}
