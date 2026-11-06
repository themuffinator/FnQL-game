/* FnQL-game, Copyright (C) 2026 contributors. GPL-2.0-or-later. */
#include "cg_local.h"
#include <limits.h>
#include <stdint.h>
#include <float.h>
#define AMMO_FONT_SIZE 12

/* Legacy selector retained from cg_weapons.c, Copyright (C) 1999-2000
 * Id Software, Inc., including baseq3a's alternate layouts and counters. */
static void LegacyWeaponSelect( void ) {
	int		i;
	int		bits;
	int		count;
	int		x, y;
	int		dx, dy;
	int		weaponSelect;
	const char *name;
	float	*color;
	char	buf[16];

	// don't display if dead
	if ( cg.predictedPlayerState.stats[STAT_HEALTH] <= 0 || cg_drawWeaponSelect.integer == 0 ) {
		return;
	}

	if ( cg_drawWeaponSelect.integer < 0 ) {
		color = colorWhite;
	} else {
		color = CG_FadeColor( cg.weaponSelectTime, WEAPON_SELECT_TIME );
		if ( !color ) {
			return;
		}
	}
	trap_R_SetColor( color );

	if (cg_drawWeaponSelect.integer == INT_MIN) { trap_R_SetColor(NULL); return; }
	weaponSelect = abs( cg_drawWeaponSelect.integer );

	// showing weapon select clears pickup item display, but not the blend blob
	cg.itemPickupTime = 0;

	// count the number of weapons owned
	bits = cg.predictedPlayerState.stats[ STAT_WEAPONS ];
	count = 0;
	for ( i = WP_GAUNTLET ; i < WP_NUM_WEAPONS ; i++ ) {
		if ( bits & ( 1 << i ) ) {
			count++;
		}
	}

	if ( weaponSelect < 3 ) {
		x = 320 - count * 20;
		y = cgs.screenYmax + 1 - 100; // - STATUSBAR_HEIGHT - 40
		dx = 40;
		dy = 0;
	} else {
		x = cgs.screenXmin + 6;
		y = 240 - count * 20;
		dx = 0;
		dy = 40;
	}

	for ( i = WP_GAUNTLET ; i < WP_NUM_WEAPONS ; i++ ) {
		if ( !( bits & ( 1 << i ) ) ) {
			continue;
		}

		CG_RegisterWeapon( i );

		// draw weapon icon
		CG_DrawPic( x, y, 32, 32, cg_weapons[i].weaponIcon );

		// draw selection marker
		if ( i == cg.weaponSelect ) {
			CG_DrawPic( x-4, y-4, 32+8, 32+8, cgs.media.selectShader );
		}

		// no ammo cross on top
		if ( !cg.predictedPlayerState.ammo[ i ] ) {
			CG_DrawPic( x, y, 32, 32, cgs.media.noammoShader );
		} else if ( weaponSelect > 1 && cg.predictedPlayerState.ammo[ i ] > 0 ) {
			// ammo counter
			BG_sprintf( buf, "%i", cg.predictedPlayerState.ammo[ i ] );
			if ( weaponSelect == 2 ) {
				// horizontal ammo counters
				CG_DrawString( x + 32/2, y - 20, buf, color, AMMO_FONT_SIZE, AMMO_FONT_SIZE, 0, DS_CENTER | DS_PROPORTIONAL );
			} else {
				// vectical ammo counters
				CG_DrawString( x + 39 + (3*AMMO_FONT_SIZE), y + (32-AMMO_FONT_SIZE)/2, buf, color, AMMO_FONT_SIZE, AMMO_FONT_SIZE, 0, DS_RIGHT );
			}
		}

		x += dx;
		y += dy;
	}

	// draw the selected name
	if ( cg.weaponSelect > WP_NONE && cg.weaponSelect < WP_NUM_WEAPONS && cg_weapons[ cg.weaponSelect ].item && weaponSelect == 1 ) {
		name = cg_weapons[ cg.weaponSelect ].item->pickup_name;
		if ( name ) {
			CG_DrawString( 320, y - 22, name, color, BIGCHAR_WIDTH, BIGCHAR_HEIGHT, 0, DS_SHADOW | DS_PROPORTIONAL | DS_CENTER | DS_FORCE_COLOR );
		}
	}

	trap_R_SetColor( NULL );
}

/* Native menu text and images already share a centered HUD canvas. Translate
 * edge-anchored lists in logical units instead of changing global draw state. */
static float WeaponBarAnchor(int mode) {
    if (!(cgs.screenXScale > 0 && cgs.screenXScale <= FLT_MAX) ||
        !(cgs.screenXBias >= 0 && cgs.screenXBias <= FLT_MAX)) return 0;
    return mode == 1 ? -cgs.screenXBias/cgs.screenXScale :
        mode == 2 ? cgs.screenXBias/cgs.screenXScale : 0;
}

static qboolean WeaponBarPractice(void) {
    const char *p = CG_ConfigString(CS_PRACTICE_FLAGS);
    uint32_t flags=0;
    if (!p) return qfalse;
    while (*p && (unsigned char)*p <= ' ') ++p;
    if (*p=='+') ++p;
    if (*p<'0' || *p>'9') return qfalse;
    do {
        unsigned digit=(unsigned)(*p++-'0');
        if (flags>(UINT32_MAX-digit)/10) return qfalse;
        flags=flags*10+digit;
    } while (*p>='0' && *p<='9');
    while (*p && (unsigned char)*p <= ' ') ++p;
    return !*p && (flags&1); /* 1005338e: hide unowned icons in practice. */
}

static void WeaponStrip(void) {
    int weapon, count=0, selected=cg.weaponSelect;
    unsigned owned=(unsigned)cg.snap->ps.stats[STAT_WEAPONS];
    int64_t elapsed=(int64_t)cg.time-cg.weaponSelectTime;
    float x;
    vec4_t ink={1,1,1,1};
    if (!cg.weaponSelectTime || elapsed >= WEAPON_SELECT_TIME) return;
    if (elapsed > WEAPON_SELECT_TIME-FADE_TIME)
        ink[3]=(float)(WEAPON_SELECT_TIME-elapsed)/FADE_TIME;
    for (weapon=WP_GAUNTLET;weapon<WP_NUM_WEAPONS;++weapon)
        if (owned&(1u<<weapon)) ++count;
    x=320-count*20;
    cg.itemPickupTime=0; /* Retail 100539c1: only the timed strip clears this. */
    trap_R_SetColor(ink);
    for (weapon=WP_GAUNTLET;weapon<WP_NUM_WEAPONS;++weapon) {
        if (!(owned&(1u<<weapon))) continue;
        CG_RegisterWeapon(weapon);
        CG_DrawPic(x,380,32,32,cg_weapons[weapon].weaponIcon);
        if (weapon==selected) CG_DrawPic(x-4,376,40,40,cgs.media.selectShader);
        if (!cg.snap->ps.ammo[weapon]) CG_DrawPic(x,380,32,32,cgs.media.noammoShader);
        x+=40;
    }
    if (selected>WP_NONE && selected<WP_NUM_WEAPONS && cg_weapons[selected].item &&
        cg_weapons[selected].item->pickup_name) {
        const char *name=cg_weapons[selected].item->pickup_name;
        /* 10053b4b/100128b0: integer centering, .25 text, baseline 358+16. */
        CG_Text_Paint((640-CG_Text_Width(name,.25f,0))/2,374,.25f,ink,name,0,0,3);
    }
    trap_R_SetColor(NULL);
}

void CG_DrawWeaponSelect(void) {
    int mode=cg_weaponBar.integer, weapons[WP_NUM_WEAPONS], count=0, weapon, i;
    unsigned owned;
    float x,y,stepX=0,stepY=22,ammoOffset=20,infiniteOffset=18,selectOffset=-4;
    qboolean followed, practice;
    if (!cg.snap || cg.predictedPlayerState.stats[STAT_HEALTH]<=0 || !cg_drawWeaponSelect.integer) return;
    /* Preserve explicit baseq3a counter/vertical/always-on modes. The old
     * default value 1 now delegates to retail's cg_weaponBar controls. */
    if (cg_drawWeaponSelect.integer!=1) { LegacyWeaponSelect(); return; }
    if (mode<=0) return;
    if (mode>4) mode=1;
    CG_SetTextFont(0);
    if (mode==4) { WeaponStrip(); return; }
    followed=(cg.predictedPlayerState.pm_flags&PMF_FOLLOW) || cg.predictedPlayerState.pm_type==PM_SPECTATOR;
    if (followed && cg.scoreBoardShowing) return;
    owned=(unsigned)cg.snap->ps.stats[STAT_WEAPONS];
    practice=WeaponBarPractice();
    /* 100531ac..10053206 counts registered map icons for full-bar spacing,
     * but follow views count only owned weapons. Never register unowned ones. */
    for (weapon=WP_MACHINEGUN;weapon<WP_NUM_WEAPONS;++weapon) {
        if (owned&(1u<<weapon)) CG_RegisterWeapon(weapon);
        else if (!cg_drawFullWeaponBar.integer || followed) continue;
        if (cg_weapons[weapon].weaponIcon) weapons[count++]=weapon;
    }
    if (followed) mode=3;
    x=mode==2 ? 623 : mode==3 ? 320-count*27.5f : 1;
    y=followed ? 452 : mode==3 ? 413 : 94;
    if (mode==2) { ammoOffset=-3; infiniteOffset=-19; selectOffset=-33; }
    if (mode==3) { stepX=55; stepY=0; infiniteOffset=12; }
    x+=WeaponBarAnchor(mode);
    trap_R_SetColor(NULL);
    for (i=0;i<count;++i,x+=stepX,y+=stepY) {
        vec4_t ink={1,1,1,1};
        char text[16];
        int ammo, threshold;
        float tx, percentile=cg_lowAmmoWarningPercentile.value;
        qboolean selected;
        weapon=weapons[i];
        if (practice && !(owned&(1u<<weapon))) continue;
        selected=weapon==((followed || cg.demoPlayback) ? cg.snap->ps.weapon : cg.weaponSelect);
        if (selected) CG_DrawPic(x+selectOffset,y-2,52,20,
            cgs.media.weaponBarLitShader ? cgs.media.weaponBarLitShader : cgs.media.selectShader);
        CG_DrawPic(x,y,16,16,cg_weapons[weapon].weaponIcon);
        if (!(owned&(1u<<weapon))) continue;
        ammo=cg.snap->ps.ammo[weapon];
        if (ammo<0) {
            /* Retail 10053505 treats every negative value as unlimited here. */
            if (cgs.media.infiniteAmmoShader) CG_DrawPic(x+infiniteOffset,y,16,16,cgs.media.infiniteAmmoShader);
            continue;
        }
        if (!(percentile>=.01f && percentile<=1)) percentile=percentile<.01f ? .01f : percentile>1 ? 1 : .2f;
        /* Unlike the main ammo warning, 100536e6 truncates this threshold. */
        threshold=(int)(bg_weaponMaxAmmo[weapon]*percentile);
        if (!ammo && cg_lowAmmoWeaponBarWarning.integer) {
            ink[1]=ink[2]=0; ink[3]=followed ? (selected ? .6f : .4f) : .8f;
        } else {
            if (ammo<threshold && cg_lowAmmoWeaponBarWarning.value==2) { ink[1]=.8f; ink[2]=.2f; }
            if (followed && !selected) ink[3]=.6f;
        }
        Com_sprintf(text,sizeof(text),"%i",ammo);
        tx=x+ammoOffset;
        if (mode==2) tx-=CG_Text_Width(text,.24f,0);
        CG_Text_Paint(tx,y+13,.24f,ink,text,0,0,0);
    }
    trap_R_SetColor(NULL);
}

