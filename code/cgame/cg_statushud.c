/* FnQL-game, Copyright (C) 2026 contributors. GPL-2.0-or-later. */
#include "cg_local.h"
#include "../ui/ui_shared.h"
#include <float.h>
#include <limits.h>

static qboolean HudFinite(float value) { return value >= -FLT_MAX && value <= FLT_MAX; }

/* Retail 10012b10 and the identical tables in 1002ed50..1002f0c0.
 * Keep this HUD palette separate from inherited world/team effects. */
static void HudTeamColor(vec4_t color, float alpha) {
    static const float rgb[5][3] = {
        {1,.8f,.2f}, {1,.2f,.1f}, {.2f,.4f,1}, {.75f,.75f,.75f}, {1,1,1}
    };
    int team = cg.snap->ps.persistant[PERS_TEAM];
    if (team < TEAM_FREE || team > TEAM_SPECTATOR) team = 4;
    memcpy(color, rgb[team], sizeof(rgb[0])); color[3] = alpha;
}

static int HudWeapon(void) {
    int client, weapon;
    if (!cg.snap) return WP_NONE;
    client = cg.snap->ps.clientNum;
    if (client < 0 || client >= MAX_CLIENTS) return WP_NONE;
    weapon = cg_entities[client].currentState.weapon;
    return weapon > WP_NONE && weapon < WP_NUM_WEAPONS ? weapon : WP_NONE;
}

int CG_StatusAmmo(void) {
    int weapon = HudWeapon();
    return weapon ? cg.snap->ps.ammo[weapon] : -1;
}

static void HudPic(float x, float y, float w, float h, const vec4_t color, qhandle_t shader) {
    if (!(w > 0 && h > 0) || !HudFinite(x) || !HudFinite(y) ||
        !HudFinite(w) || !HudFinite(h)) return;
    trap_R_SetColor(color);
    CG_DrawPic(x,y,w,h,shader ? shader : cgs.media.whiteShader);
    trap_R_SetColor(NULL);
}

static void HudBar(int owner, float x, float y, float w, float h, qhandle_t shader) {
    double amount, maximum, fraction;
    float s=0, t=0, s2=1, t2=1;
    vec4_t color;
    qboolean health = owner == CG_PLAYER_HEALTH_BAR_100 || owner == CG_PLAYER_HEALTH_BAR_200;
    qboolean excess = owner == CG_PLAYER_HEALTH_BAR_200 || owner == CG_PLAYER_ARMOR_BAR_200;
    if (!(w > 0 && h > 0)) return;
    amount = cg.snap->ps.stats[health ? STAT_HEALTH : STAT_ARMOR];
    maximum = health ? cg.snap->ps.stats[STAT_MAX_HEALTH] : 100;
    if (maximum <= 0) maximum = 100; /* Invalid server state: retain a usable HUD. */
    if (excess) amount -= maximum;
    if (amount <= 0) return;
    /* Retail caps health's bonus segment, but does not cap armor above 200.
     * Double arithmetic avoids overflow on malformed signed stat values. */
    if ((!excess || health) && amount > maximum) amount = maximum;
    fraction = amount / maximum;
    if (excess) {
        /* Shipping x87 at 1002eebf/1002f10a: only 22/32 of the texture
         * height is the bonus meter. The HLIL loses the live max on ST(2). */
        float filled = (float)(h * .6875 * fraction);
        t = 1 - filled/h; y += h-filled; h = filled;
    } else {
        float filled = (float)(w * fraction);
        if (health) s2 = (float)fraction;
        else { s = 1-(float)fraction; x += w-filled; }
        w = filled;
    }
    if (!HudFinite(x) || !HudFinite(y) || !HudFinite(w) || !HudFinite(h) || !HudFinite(t)) return;
    CG_AdjustFrom640(&x,&y,&w,&h);
    if (!HudFinite(x) || !HudFinite(y) || !HudFinite(w) || !HudFinite(h)) return;
    HudTeamColor(color,1);
    trap_R_SetColor(color);
    trap_R_DrawStretchPic(x,y,w,h,s,t,s2,t2,shader ? shader : cgs.media.whiteShader);
    trap_R_SetColor(NULL);
}

/* Ownerdraw routing stays here so the gate exercises the same dispatch as
 * the installed HUD. Return false for unrelated/inherited ownerdraws. */
qboolean CG_DrawStatusHud(int owner, float x, float y, float w, float h,
        int align, float scale, vec4_t color, qhandle_t shader, int style) {
    char text[16];
    int value, weapon;
    float width;
    vec4_t ink;
    switch (owner) {
    case CG_PLAYER_HEALTH: case CG_PLAYER_ARMOR_VALUE: case CG_PLAYER_AMMO_VALUE:
    case CG_PLAYER_HEALTH_BAR_100: case CG_PLAYER_HEALTH_BAR_200:
    case CG_PLAYER_ARMOR_BAR_100: case CG_PLAYER_ARMOR_BAR_200: case CG_TEAM_COLORIZED:
        break;
    default: return qfalse;
    }
    if (!cg.snap || !HudFinite(x) || !HudFinite(y) || !HudFinite(w) || !HudFinite(h)) return qtrue;
    if (owner == CG_TEAM_COLORIZED) {
        /* 1002f5d0 tints the supplied image, preserving the caller's alpha. */
        HudTeamColor(ink,color[3]); HudPic(x,y,w,h,ink,shader); return qtrue;
    }
    if (owner == CG_PLAYER_HEALTH_BAR_100 || owner == CG_PLAYER_HEALTH_BAR_200 ||
        owner == CG_PLAYER_ARMOR_BAR_100 || owner == CG_PLAYER_ARMOR_BAR_200) {
        HudBar(owner,x,y,w,h,shader); return qtrue;
    }
    if (owner == CG_PLAYER_AMMO_VALUE) {
        weapon = HudWeapon();
        if (!weapon || weapon == WP_GAUNTLET || weapon == WP_GRAPPLING_HOOK) return qtrue;
        value = CG_StatusAmmo();
        if (value == -1) {
            /* 1002e90d/928/963: truncate the anchor, align using HEIGHT,
             * then truncate again and draw a square of that height. */
            if ((double)x < INT_MIN || (double)x > INT_MAX) return qtrue;
            x = (float)(int)x;
            if (align == ITEM_ALIGN_CENTER) x -= h*.5f;
            else if (align == ITEM_ALIGN_RIGHT) x -= h;
            if ((double)x < INT_MIN || (double)x > INT_MAX) return qtrue;
            if (cgs.media.infiniteAmmoShader)
                HudPic((float)(int)x,y,h,h,color,cgs.media.infiniteAmmoShader);
            return qtrue;
        }
        if (value < 0) return qtrue;
    } else value = cg.snap->ps.stats[owner == CG_PLAYER_HEALTH ? STAT_HEALTH : STAT_ARMOR];
    if (shader) { HudPic(x,y,w,h,color,shader); return qtrue; }
    /* 1002e500/1002e7c0/1002fdf0: item origin is an alignment anchor;
     * baseline is origin plus ascent, independent of rectangle height. */
    Com_sprintf(text,sizeof(text),"%i",value);
    width = CG_Text_Width(text,scale,0);
    if (align == ITEM_ALIGN_CENTER) x -= width*.5f;
    else if (align == ITEM_ALIGN_RIGHT) x -= width;
    CG_Text_Paint(x,y+CG_Text_Height(text,scale,0),scale,color,text,0,0,style);
    return qtrue;
}
