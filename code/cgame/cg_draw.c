// Copyright (C) 1999-2000 Id Software, Inc.
//
// cg_draw.c -- draw all of the graphical elements during
// active (after loading) gameplay

#include "cg_local.h"

#ifdef MISSIONPACK
#include "../ui/ui_shared.h"

// used for scoreboard
extern displayContextDef_t cgDC;
menuDef_t *menuScoreboard = NULL;
#else
int drawTeamOverlayModificationCount = -1;
#endif

int sortedTeamPlayers[TEAM_MAXOVERLAY];
int	numSortedTeamPlayers;

char systemChat[256];
char teamChat1[256];
char teamChat2[256];


/*
==============
CG_DrawField

Draws large numbers for status bar and powerups
==============
*/
#ifndef MISSIONPACK
static void CG_DrawField (int x, int y, int width, int value) {
	char	num[16], *ptr;
	int		l;
	int		frame;

	if ( width < 1 ) {
		return;
	}

	// draw number string
	if ( width > 5 ) {
		width = 5;
	}

	switch ( width ) {
	case 1:
		value = value > 9 ? 9 : value;
		value = value < 0 ? 0 : value;
		break;
	case 2:
		value = value > 99 ? 99 : value;
		value = value < -9 ? -9 : value;
		break;
	case 3:
		value = value > 999 ? 999 : value;
		value = value < -99 ? -99 : value;
		break;
	case 4:
		value = value > 9999 ? 9999 : value;
		value = value < -999 ? -999 : value;
		break;
	}

	Com_sprintf (num, sizeof(num), "%i", value);
	l = strlen(num);
	if (l > width)
		l = width;
	x += 2 + CHAR_WIDTH*(width - l);

	ptr = num;
	while (*ptr && l)
	{
		if (*ptr == '-')
			frame = STAT_MINUS;
		else
			frame = *ptr -'0';

		CG_DrawPic( x,y, CHAR_WIDTH, CHAR_HEIGHT, cgs.media.numberShaders[frame] );
		x += CHAR_WIDTH;
		ptr++;
		l--;
	}
}
#endif // MISSIONPACK


/*
================
CG_Draw3DModel
================
*/
void CG_Draw3DModel( float x, float y, float w, float h, qhandle_t model, qhandle_t skin, vec3_t origin, vec3_t angles ) {
	refdef_t		refdef;
	refEntity_t		ent;

	if ( !cg_draw3dIcons.integer || !cg_drawIcons.integer ) {
		return;
	}

	CG_AdjustFrom640( &x, &y, &w, &h );

	memset( &refdef, 0, sizeof( refdef ) );

	memset( &ent, 0, sizeof( ent ) );
	AnglesToAxis( angles, ent.axis );
	VectorCopy( origin, ent.origin );
	ent.hModel = model;
	ent.customSkin = skin;
	ent.renderfx = RF_NOSHADOW;		// no stencil shadows

	refdef.rdflags = RDF_NOWORLDMODEL;

	AxisClear( refdef.viewaxis );

	refdef.fov_x = 30;
	refdef.fov_y = 30;

	refdef.x = x;
	refdef.y = y;
	refdef.width = w;
	refdef.height = h;

	refdef.time = cg.time;

	trap_R_ClearScene();
	trap_R_AddRefEntityToScene( &ent );
	trap_R_RenderScene( &refdef );
}



/*
================
CG_Draw3DModel
================
*/
void CG_Draw3DModelColor( float x, float y, float w, float h, qhandle_t model, qhandle_t skin, vec3_t origin, vec3_t angles, vec3_t color ) {
	refdef_t		refdef;
	refEntity_t		ent;

	if ( !cg_draw3dIcons.integer || !cg_drawIcons.integer ) {
		return;
	}

	CG_AdjustFrom640( &x, &y, &w, &h );

	memset( &refdef, 0, sizeof( refdef ) );

	memset( &ent, 0, sizeof( ent ) );
	AnglesToAxis( angles, ent.axis );
	VectorCopy( origin, ent.origin );
	ent.hModel = model;
	ent.customSkin = skin;
	ent.renderfx = RF_NOSHADOW;		// no stencil shadows

	refdef.rdflags = RDF_NOWORLDMODEL;

	AxisClear( refdef.viewaxis );

	refdef.fov_x = 30;
	refdef.fov_y = 30;

	refdef.x = x;
	refdef.y = y;
	refdef.width = w;
	refdef.height = h;

	refdef.time = cg.time;

	ent.shaderRGBA[0] = color[0] * 255;
	ent.shaderRGBA[1] = color[1] * 255;
	ent.shaderRGBA[2] = color[2] * 255;
	ent.shaderRGBA[3] = 255;

	trap_R_ClearScene();
	trap_R_AddRefEntityToScene( &ent );
	trap_R_RenderScene( &refdef );
}


/*
================
CG_DrawHead

Used for both the status bar and the scoreboard
================
*/
static void CG_DrawHeadColored( float x, float y, float w, float h, int clientNum, vec3_t headAngles, const float *tint );

void CG_DrawHead( float x, float y, float w, float h, int clientNum, vec3_t headAngles ) {
	CG_DrawHeadColored( x, y, w, h, clientNum, headAngles, NULL );
}

/* The head with a given tint instead of the client's head colour (the
 * retail attacker head, 0x10009490). */
void CG_DrawHeadTinted( float x, float y, float w, float h, int clientNum, vec3_t headAngles, const vec3_t tint ) {
	CG_DrawHeadColored( x, y, w, h, clientNum, headAngles, tint );
}

static void CG_DrawHeadColored( float x, float y, float w, float h, int clientNum, vec3_t headAngles, const float *tint ) {
	clipHandle_t	cm;
	clientInfo_t	*ci;
	float			len;
	vec3_t			origin;
	vec3_t			mins, maxs;

	ci = &cgs.clientinfo[ clientNum ];

	if ( cg_draw3dIcons.integer ) {
		cm = ci->headModel;
		if ( !cm ) {
			return;
		}

		// offset the origin y and z to center the head
		trap_R_ModelBounds( cm, mins, maxs );

		origin[2] = -0.5 * ( mins[2] + maxs[2] );
		origin[1] = 0.5 * ( mins[1] + maxs[1] );

		// calculate distance so the head nearly fills the box
		// assume heads are taller than wide
		len = 0.7 * ( maxs[2] - mins[2] );		
		origin[0] = len / 0.268;	// len / tan( fov/2 )

		// allow per-model tweaking
		VectorAdd( origin, ci->headOffset, origin );

		CG_Draw3DModelColor( x, y, w, h, ci->headModel, ci->headSkin, origin, headAngles, tint ? (float *)tint : ci->headColor );
	} else if ( cg_drawIcons.integer ) {
		trap_R_SetColor ( NULL );
		CG_DrawPic( x, y, w, h, ci->modelIcon );
	}

	// if they are deferred, draw a cross out
	if ( ci->deferred ) {
		trap_R_SetColor ( NULL );
		CG_DrawPic( x, y, w, h, cgs.media.deferShader );
	}
}

/*
================
CG_DrawFlagModel

Used for both the status bar and the scoreboard
================
*/
void CG_DrawFlagModel( float x, float y, float w, float h, int team, qboolean force2D ) {
	qhandle_t		cm;
	float			len;
	vec3_t			origin, angles;
	vec3_t			mins, maxs;
	qhandle_t		handle;

	if ( !force2D && cg_draw3dIcons.integer ) {

		VectorClear( angles );

		cm = cgs.media.redFlagModel;

		// offset the origin y and z to center the flag
		trap_R_ModelBounds( cm, mins, maxs );

		origin[2] = -0.5 * ( mins[2] + maxs[2] );
		origin[1] = 0.5 * ( mins[1] + maxs[1] );

		// calculate distance so the flag nearly fills the box
		// assume heads are taller than wide
		len = 0.5 * ( maxs[2] - mins[2] );		
		origin[0] = len / 0.268;	// len / tan( fov/2 )

		angles[YAW] = 60 * sin( ( cg.time % TMOD_2000 ) / 2000.0 );;

		if( team == TEAM_RED ) {
			handle = cgs.media.redFlagModel;
		} else if( team == TEAM_BLUE ) {
			handle = cgs.media.blueFlagModel;
		} else if( team == TEAM_FREE ) {
			handle = cgs.media.neutralFlagModel;
		} else {
			return;
		}
		CG_Draw3DModel( x, y, w, h, handle, 0, origin, angles );
	} else if ( cg_drawIcons.integer ) {
		gitem_t *item;

		if( team == TEAM_RED ) {
			item = BG_FindItemForPowerup( PW_REDFLAG );
		} else if( team == TEAM_BLUE ) {
			item = BG_FindItemForPowerup( PW_BLUEFLAG );
		} else if( team == TEAM_FREE ) {
			item = BG_FindItemForPowerup( PW_NEUTRALFLAG );
		} else {
			return;
		}
		if (item) {
		  CG_DrawPic( x, y, w, h, cg_items[ ITEM_INDEX(item) ].icon );
		}
	}
}

/*
================
CG_DrawStatusBarHead

================
*/
#ifndef MISSIONPACK

static void CG_DrawStatusBarHead( float x ) {
	vec3_t		angles;
	float		size, stretch;
	float		frac;

	VectorClear( angles );

	if ( cg.damageTime && cg.time - cg.damageTime < DAMAGE_TIME ) {
		frac = (float)(cg.time - cg.damageTime ) / DAMAGE_TIME;
		size = ICON_SIZE * 1.25 * ( 1.5 - frac * 0.5 );

		stretch = size - ICON_SIZE * 1.25;
		// kick in the direction of damage
		x -= stretch * 0.5 + cg.damageX * stretch * 0.5;

		cg.headStartYaw = 180 + cg.damageX * 45;

		cg.headEndYaw = 180 + 20 * cos( crandom()*M_PI );
		cg.headEndPitch = 5 * cos( crandom()*M_PI );

		cg.headStartTime = cg.time;
		cg.headEndTime = cg.time + 100 + random() * 2000;
	} else {
		if ( cg.time >= cg.headEndTime ) {
			// select a new head angle
			cg.headStartYaw = cg.headEndYaw;
			cg.headStartPitch = cg.headEndPitch;
			cg.headStartTime = cg.headEndTime;
			cg.headEndTime = cg.time + 100 + random() * 2000;

			cg.headEndYaw = 180 + 20 * cos( crandom()*M_PI );
			cg.headEndPitch = 5 * cos( crandom()*M_PI );
		}

		size = ICON_SIZE * 1.25;
	}

	// if the server was frozen for a while we may have a bad head start time
	if ( cg.headStartTime > cg.time ) {
		cg.headStartTime = cg.time;
	}

	frac = ( cg.time - cg.headStartTime ) / (float)( cg.headEndTime - cg.headStartTime );
	frac = frac * frac * ( 3 - 2 * frac );
	angles[YAW] = cg.headStartYaw + ( cg.headEndYaw - cg.headStartYaw ) * frac;
	angles[PITCH] = cg.headStartPitch + ( cg.headEndPitch - cg.headStartPitch ) * frac;

	CG_DrawHead( x, cgs.screenYmax + 1 - size, size, size, cg.snap->ps.clientNum, angles );
}
#endif // MISSIONPACK


/*
================
CG_DrawStatusBarFlag

================
*/
#ifndef MISSIONPACK
static void CG_DrawStatusBarFlag( float x, int team ) {
	CG_DrawFlagModel( x, cgs.screenYmax + 1 - ICON_SIZE, ICON_SIZE, ICON_SIZE, team, qfalse );
}
#endif // MISSIONPACK


/*
================
CG_DrawTeamBackground

================
*/
void CG_DrawTeamBackground( int x, int y, int w, int h, float alpha, int team )
{
	vec4_t		hcolor;

	hcolor[3] = alpha;
	if ( team == TEAM_RED ) {
		hcolor[0] = 1.0f;
		hcolor[1] = 0.0f;
		hcolor[2] = 0.0f;
	} else if ( team == TEAM_BLUE ) {
		hcolor[0] = 0.0f;
		hcolor[1] = 0.1f;
		hcolor[2] = 1.0f;
	} else {
		return;
	}
	trap_R_SetColor( hcolor );
	CG_DrawPic( x, y, w, h, cgs.media.teamStatusBar );
	trap_R_SetColor( NULL );
}


/*
================
CG_DrawStatusBar
================
*/
#ifndef MISSIONPACK
#define STATUSBAR_HEIGHT 60
static void CG_DrawStatusBar( void ) {
	int			color;
	centity_t	*cent;
	playerState_t	*ps;
	int			value;
	vec4_t		hcolor;
	vec3_t		angles;
	vec3_t		origin;
	float		y;
#ifdef MISSIONPACK
	qhandle_t	handle;
#endif
	static float colors[4][4] = { 
//		{ 0.2, 1.0, 0.2, 1.0 } , { 1.0, 0.2, 0.2, 1.0 }, {0.5, 0.5, 0.5, 1} };
		{ 1.0f, 0.69f, 0.0f, 1.0f },    // normal
		{ 1.0f, 0.2f, 0.2f, 1.0f },     // low health
		{ 0.5f, 0.5f, 0.5f, 1.0f },     // weapon firing
		{ 1.0f, 1.0f, 1.0f, 1.0f } };   // health > 100

	if ( cg_drawStatus.integer == 0 ) {
		return;
	}

	// draw the team background
	CG_DrawTeamBackground( cgs.screenXmin, cgs.screenYmax - STATUSBAR_HEIGHT + 1,
		cgs.screenXmax - cgs.screenXmin + 1, STATUSBAR_HEIGHT, 
		0.33f, cg.snap->ps.persistant[ PERS_TEAM ] );

	y = cgs.screenYmax + 1 - ICON_SIZE;

	cent = &cg_entities[cg.snap->ps.clientNum];
	ps = &cg.snap->ps;

	VectorClear( angles );

	// draw any 3D icons first, so the changes back to 2D are minimized
	if ( cent->currentState.weapon && cg_weapons[ cent->currentState.weapon ].ammoModel ) {
		origin[0] = 70;
		origin[1] = 0;
		origin[2] = 0;
		angles[YAW] = 90 + 20 * sin( ( cg.time % TMOD_1000 ) / 1000.0 );
		CG_Draw3DModel( CHAR_WIDTH*3 + TEXT_ICON_SPACE, y, ICON_SIZE, ICON_SIZE,
					   cg_weapons[ cent->currentState.weapon ].ammoModel, 0, origin, angles );
	}

	CG_DrawStatusBarHead( 185 + CHAR_WIDTH*3 + TEXT_ICON_SPACE );

	if( cg.predictedPlayerState.powerups[PW_REDFLAG] ) {
		CG_DrawStatusBarFlag( 185 + CHAR_WIDTH*3 + TEXT_ICON_SPACE + ICON_SIZE, TEAM_RED );
	} else if( cg.predictedPlayerState.powerups[PW_BLUEFLAG] ) {
		CG_DrawStatusBarFlag( 185 + CHAR_WIDTH*3 + TEXT_ICON_SPACE + ICON_SIZE, TEAM_BLUE );
	} else if( cg.predictedPlayerState.powerups[PW_NEUTRALFLAG] ) {
		CG_DrawStatusBarFlag( 185 + CHAR_WIDTH*3 + TEXT_ICON_SPACE + ICON_SIZE, TEAM_FREE );
	}

	if ( ps->stats[ STAT_ARMOR ] ) {
		origin[0] = 90;
		origin[1] = 0;
		origin[2] = -10;
		angles[YAW] = ( cg.time & 2047 ) * 360 / 2048.0;
		CG_Draw3DModel( 370 + CHAR_WIDTH*3 + TEXT_ICON_SPACE, y, ICON_SIZE, ICON_SIZE,
					   cgs.media.armorModel, 0, origin, angles );
	}
#ifdef MISSIONPACK
	if( cgs.gametype == GT_HARVESTER ) {
		origin[0] = 90;
		origin[1] = 0;
		origin[2] = -10;
		angles[YAW] = ( cg.time & 2047 ) * 360 / 2048.0;
		if( cg.snap->ps.persistant[PERS_TEAM] == TEAM_BLUE ) {
			handle = cgs.media.redCubeModel;
		} else {
			handle = cgs.media.blueCubeModel;
		}
		CG_Draw3DModel( 640 - (TEXT_ICON_SPACE + ICON_SIZE), 416, ICON_SIZE, ICON_SIZE, handle, 0, origin, angles );
	}
#endif
	//
	// ammo
	//
	if ( cent->currentState.weapon ) {
		value = ps->ammo[cent->currentState.weapon];
		if ( value > -1 ) {
			if ( cg.predictedPlayerState.weaponstate == WEAPON_FIRING
				&& cg.predictedPlayerState.weaponTime > 100 ) {
				// draw as dark grey when reloading
				color = 2;	// dark grey
			} else {
				if ( value >= 0 ) {
					color = 0;	// yellow
				} else {
					color = 1;	// red
				}
			}
#ifdef USE_NEW_FONT_RENDERER
			CG_SelectFont( 1 );
			CG_DrawString( CHAR_WIDTH*3, y, va( "%i", value ), colors[ color ], CHAR_WIDTH, CHAR_HEIGHT, 0, DS_RIGHT | DS_PROPORTIONAL );
			CG_SelectFont( 0 );
#else
			trap_R_SetColor( colors[color] );
			CG_DrawField( 0, y, 3, value );
#endif
			trap_R_SetColor( NULL );

			// if we didn't draw a 3D icon, draw a 2D icon for ammo
			if ( !cg_draw3dIcons.integer && cg_drawIcons.integer ) {
				qhandle_t	icon;

				icon = cg_weapons[ cg.predictedPlayerState.weapon ].ammoIcon;
				if ( icon ) {
					CG_DrawPic( CHAR_WIDTH*3 + TEXT_ICON_SPACE, y, ICON_SIZE, ICON_SIZE, icon );
				}
			}
		}
	}

	//
	// health
	//
	value = ps->stats[STAT_HEALTH];
	if ( value > 100 ) {
		color = 3; // white
	} else if ( value > 25 ) {
		color = 0; 	// yellow
	} else if (value > 0) {
		color = (cg.time >> 8) & 1;	// red/yellow flashing
	} else {
		color = 1; // red
	}

#ifdef USE_NEW_FONT_RENDERER
	CG_SelectFont( 1 );
	CG_DrawString( 185 + CHAR_WIDTH*3, y, va( "%i", value ), colors[ color ], CHAR_WIDTH, CHAR_HEIGHT, 0, DS_RIGHT | DS_PROPORTIONAL );
	CG_SelectFont( 0 );
#else
	trap_R_SetColor( colors[ color ] );
	// stretch the health up when taking damage
	CG_DrawField( 185, y, 3, value );
#endif
	
	CG_ColorForHealth( hcolor );
	trap_R_SetColor( hcolor );

	//
	// armor
	//
	value = ps->stats[STAT_ARMOR];
	if ( value > 100 ) {
		color = 3; // white
	} else {
		color = 0; 	// yellow
	}
	if ( value > 0 ) {
#ifdef USE_NEW_FONT_RENDERER
		CG_SelectFont( 1 );
		CG_DrawString( 370 + CHAR_WIDTH*3, y, va( "%i", value ), colors[ color ], CHAR_WIDTH, CHAR_HEIGHT, 0, DS_RIGHT | DS_PROPORTIONAL );
		CG_SelectFont( 0 );
#else
		trap_R_SetColor( colors[0] );
		CG_DrawField( 370, y, 3, value );
#endif
		trap_R_SetColor( NULL );
		// if we didn't draw a 3D icon, draw a 2D icon for armor
		if ( !cg_draw3dIcons.integer && cg_drawIcons.integer ) {
			CG_DrawPic( 370 + CHAR_WIDTH*3 + TEXT_ICON_SPACE, y, ICON_SIZE, ICON_SIZE, cgs.media.armorIcon );
		}
	}

#ifdef MISSIONPACK
	//
	// cubes
	//
	if( cgs.gametype == GT_HARVESTER ) {
		value = ps->generic1;
		if( value > 99 ) {
			value = 99;
		}
		trap_R_SetColor( colors[0] );
		CG_DrawField (640 - (CHAR_WIDTH*2 + TEXT_ICON_SPACE + ICON_SIZE), y, 2, value);
		trap_R_SetColor( NULL );
		// if we didn't draw a 3D icon, draw a 2D icon for armor
		if ( !cg_draw3dIcons.integer && cg_drawIcons.integer ) {
			if( cg.snap->ps.persistant[PERS_TEAM] == TEAM_BLUE ) {
				handle = cgs.media.redCubeIcon;
			} else {
				handle = cgs.media.blueCubeIcon;
			}
			CG_DrawPic( 640 - (TEXT_ICON_SPACE + ICON_SIZE), y, ICON_SIZE, ICON_SIZE, handle );
		}
	}
#endif
}
#endif

/*
===========================================================================================

  UPPER RIGHT CORNER

===========================================================================================
*/

/*
================
CG_DrawAttacker

================
*/
/* Retail 0x10009930 (upper-right block, right placement): for ten seconds
 * after damage, while alive, the attacker (another client) as a 36-unit
 * head at x 604 tinted by cg_enemyHeadColor, or cg_teamHeadColor for a
 * teammate in team modes, and the name (font 1, 0.18, white) right-aligned
 * to 630, 10 below the head. Returns 54 below the start. */
static float CG_DrawAttacker( float y ) {
	static vec4_t white = { 1, 1, 1, 1 };
	vec3_t		angles, tint;
	const char	*name;
	int			clientNum, color, me, placement;

	if ( cg.predictedPlayerState.stats[STAT_HEALTH] <= 0 ) {
		return y;
	}
	if ( !cg.damageTime || cg.time - cg.damageTime > 10000 ) {
		return y;
	}
	clientNum = cg.predictedPlayerState.persistant[PERS_ATTACKER];
	if ( clientNum < 0 || clientNum >= MAX_CLIENTS || clientNum == cg.snap->ps.clientNum ) {
		return y;
	}
	me = cg.predictedPlayerState.clientNum;
	color = ( cgs.gametype < GT_TEAM || me < 0 || me >= MAX_CLIENTS ||
		cgs.clientinfo[me].team != cgs.clientinfo[clientNum].team ) ? cg_enemyHeadColor.integer : cg_teamHeadColor.integer;
	tint[0] = ( ( color >> 24 ) & 0xff ) / 255.0f;
	tint[1] = ( ( color >> 16 ) & 0xff ) / 255.0f;
	tint[2] = ( ( color >> 8 ) & 0xff ) / 255.0f;

	placement = CG_SetScreenPlacement( CG_PLACE_RIGHT );
	VectorSet( angles, 0, 180, 0 );
	CG_DrawHeadTinted( 604, y, 36, 36, clientNum, angles, tint );
	name = Info_ValueForKey( CG_ConfigString( CS_PLAYERS + clientNum ), "n" );
	y += 36;
	CG_SetTextFont( 1 );
	CG_Text_Paint( 640 - ( 10 + CG_Text_WidthInFont( name, 1, 0.18f, 0 ) ), y + 10, 0.18f, white, name, 0, 0, 0 );
	CG_SetTextFont( 0 );
	CG_SetScreenPlacement( placement );
	return y + 16 + 2;
}


/*
================
CG_DrawSpeedMeter
================
*/
static float CG_DrawSpeedMeter( float y ) {
	const char *s;

	/* speed meter can get in the way of the scoreboard */
	if ( cg.scoreBoardShowing ) {
		return y;
	}

	s = va( "%1.0fups", cg.xyspeed );

	if ( cg_drawSpeed.integer == 1 ) {
		/* top left-hand corner of screen */
		CG_DrawString( cgs.screenXmax - 4, y + 2, s, colorWhite, BIGCHAR_WIDTH, BIGCHAR_HEIGHT, 0, DS_SHADOW | DS_RIGHT | DS_PROPORTIONAL );
		return y + BIGCHAR_HEIGHT + 4;
	} else {
		/* center of screen */
		CG_DrawString( 320, 300, s, colorWhite, BIGCHAR_WIDTH, BIGCHAR_HEIGHT, 0, DS_SHADOW | DS_CENTER | DS_PROPORTIONAL );
		return y;
	}
}


/*
==================
CG_DrawSnapshot
==================
*/
static float CG_DrawSnapshot( float y ) {
	const char *s;

	s = va( "time:%i snap:%i cmd:%i", cg.snap->serverTime, 
		cg.latestSnapshotNum, cgs.serverCommandSequence );

	CG_DrawString( cgs.screenXmax - 4, y + 2, s, colorWhite, BIGCHAR_WIDTH, BIGCHAR_HEIGHT, 0, DS_SHADOW | DS_RIGHT );

	return y + BIGCHAR_HEIGHT + 4;
}




/*
=================
CG_DrawTimer
=================
*/
static float CG_DrawTimer( float y ) {
	const char	*s;
	int			mins, seconds;
	int			msec;

	msec = cg.time - cgs.levelStartTime;

	seconds = msec / 1000;
	mins = seconds / 60;
	seconds -= mins * 60;

	s = va( "%i:%02i", mins, seconds );
	CG_DrawString( cgs.screenXmax - 4, y + 2, s, colorWhite, BIGCHAR_WIDTH, BIGCHAR_HEIGHT, 0, DS_SHADOW | DS_RIGHT | DS_PROPORTIONAL );

	return y + BIGCHAR_HEIGHT + 4;
}


/*
=====================
CG_DrawUpperRight

=====================
*/
static void CG_DrawUpperRight(stereoFrame_t stereoFrame)
{
	float	y;
	int		placement;

	// retail 0x1000a770, at the right placement (0x1001111a): the team
	// overlay (mode 1) from y 0, the snapshot line, frame rate and attacker;
	// retail has no timer or speed line here (cg_drawTimer and cg_drawSpeed
	// are FnQL-game locals, off by default). The inherited lines below
	// position themselves from cgs.screenXmax, so only the overlay takes the
	// right placement.
	y = 0;

	if ( cgs.gametype >= GT_TEAM && cg_drawTeamOverlay.integer == 1 ) {
		placement = CG_SetScreenPlacement( CG_PLACE_RIGHT );
		y = CG_DrawTeamOverlay( y, qtrue, qtrue );
		CG_SetScreenPlacement( placement );
	}
	if ( cg_drawSnapshot.integer ) {
		y = CG_DrawSnapshot( y );
	}
	if (cg_drawFPS.integer && (stereoFrame == STEREO_CENTER || stereoFrame == STEREO_RIGHT)) {
		y = CG_DrawFrameRate( y );	// retail 0x10009c10
	}
	if ( cg_drawSpeed.integer ) {
		y = CG_DrawSpeedMeter( y );
	}
	if ( cg_drawTimer.integer ) {
		y = CG_DrawTimer( y );
	}
	if ( cg_drawAttacker.integer ) {
		y = CG_DrawAttacker( y );
	}
	// retail 0x10011133: mode 2 sits above y 408, still at the right placement
	if ( cgs.gametype >= GT_TEAM && cg_drawTeamOverlay.integer == 2 ) {
		placement = CG_SetScreenPlacement( CG_PLACE_RIGHT );
		CG_DrawTeamOverlay( 408, qtrue, qfalse );
		CG_SetScreenPlacement( placement );
	}
}


/*
===========================================================================================

  LOWER RIGHT CORNER

===========================================================================================
*/

/*
=================
CG_DrawScores

Draw the small two score display
=================
*/
#ifndef MISSIONPACK
static float CG_DrawScores( float y ) {
	const char	*s;
	int			s1, s2, score;
	int			x, x0, w;
	int			v;
	vec4_t		color;
	float		y1;
	gitem_t		*item;

	s1 = cgs.scores1;
	s2 = cgs.scores2;

	y -=  BIGCHAR_HEIGHT + 8 - 4;

	y1 = y;

	// draw from the right side to left
	if ( cgs.gametype >= GT_TEAM ) {
		x0 = cgs.screenXmax + 1;
		color[0] = 0.0f;
		color[1] = 0.1f;
		color[2] = 1.0f;
		color[3] = 0.33f;
		// second score
		s = va( "%2i", s2 );
		w = CG_DrawStrlen( s ) * BIGCHAR_WIDTH + 8;
		x = x0 - w;
		CG_FillRect( x, y-4,  w, BIGCHAR_HEIGHT+8, color );
		if ( cg.snap->ps.persistant[PERS_TEAM] == TEAM_BLUE ) {
			CG_DrawPic( x, y-4, w, BIGCHAR_HEIGHT+8, cgs.media.selectShader );
		}
		CG_DrawString( x0-4, y, s, colorWhite, BIGCHAR_WIDTH, BIGCHAR_HEIGHT, 0, DS_SHADOW | DS_RIGHT );

		if ( cgs.gametype == GT_CTF ) {
			// Display flag status
			item = BG_FindItemForPowerup( PW_BLUEFLAG );

			if (item) {
				y1 = y - BIGCHAR_HEIGHT - 8;
				if( cgs.blueflag >= 0 && cgs.blueflag <= 2 ) {
					CG_DrawPic( x, y1-4, w, BIGCHAR_HEIGHT+8, cgs.media.blueFlagShader[cgs.blueflag] );
				}
			}
		}
		color[0] = 1.0f;
		color[1] = 0.0f;
		color[2] = 0.0f;
		color[3] = 0.33f;
		// first score
		x0 = x;
		s = va( "%2i", s1 );
		w = CG_DrawStrlen( s ) * BIGCHAR_WIDTH + 8;
		x -= w;
		CG_FillRect( x, y-4,  w, BIGCHAR_HEIGHT+8, color );
		if ( cg.snap->ps.persistant[PERS_TEAM] == TEAM_RED ) {
			CG_DrawPic( x, y-4, w, BIGCHAR_HEIGHT+8, cgs.media.selectShader );
		}

		CG_DrawString( x0-4, y, s, colorWhite, BIGCHAR_WIDTH, BIGCHAR_HEIGHT, 0, DS_SHADOW | DS_RIGHT );

		if ( cgs.gametype == GT_CTF ) {
			// Display flag status
			item = BG_FindItemForPowerup( PW_REDFLAG );

			if (item) {
				y1 = y - BIGCHAR_HEIGHT - 8;
				if( cgs.redflag >= 0 && cgs.redflag <= 2 ) {
					CG_DrawPic( x, y1-4, w, BIGCHAR_HEIGHT+8, cgs.media.redFlagShader[cgs.redflag] );
				}
			}
		}

#ifdef MISSIONPACK
		if ( cgs.gametype == GT_1FCTF ) {
			// Display flag status
			item = BG_FindItemForPowerup( PW_NEUTRALFLAG );

			if (item) {
				y1 = y - BIGCHAR_HEIGHT - 8;
				if( cgs.flagStatus >= 0 && cgs.flagStatus <= 3 ) {
					CG_DrawPic( x, y1-4, w, BIGCHAR_HEIGHT+8, cgs.media.flagShader[cgs.flagStatus] );
				}
			}
		}
#endif
		if ( cgs.gametype >= GT_CTF ) {
			v = cgs.capturelimit;
		} else {
			v = cgs.fraglimit;
		}
		if ( v ) {
			s = va( "%2i", v );
			CG_DrawString( x-4, y, s, colorWhite, BIGCHAR_WIDTH, BIGCHAR_HEIGHT, 0, DS_SHADOW | DS_RIGHT );
		}

	} else {
		qboolean	spectator;

		x = cgs.screenXmax + 1;

		score = cg.snap->ps.persistant[PERS_SCORE];
		spectator = ( cg.snap->ps.persistant[PERS_TEAM] == TEAM_SPECTATOR );

		// always show your score in the second box if not in first place
		if ( s1 != score ) {
			s2 = score;
		}
		if ( s2 != SCORE_NOT_PRESENT ) {
			x0 = x;
			s = va( "%2i", s2 );
			w = CG_DrawStrlen( s ) * BIGCHAR_WIDTH + 8;
			x -= w;
			if ( !spectator && score == s2 && score != s1 ) {
				color[0] = 1.0f;
				color[1] = 0.0f;
				color[2] = 0.0f;
				color[3] = 0.33f;
				CG_FillRect( x, y-4,  w, BIGCHAR_HEIGHT+8, color );
				CG_DrawPic( x, y-4, w, BIGCHAR_HEIGHT+8, cgs.media.selectShader );
			} else {
				color[0] = 0.5f;
				color[1] = 0.5f;
				color[2] = 0.5f;
				color[3] = 0.33f;
				CG_FillRect( x, y-4,  w, BIGCHAR_HEIGHT+8, color );
			}	
			CG_DrawString( x0-4, y, s, colorWhite, BIGCHAR_WIDTH, BIGCHAR_HEIGHT, 0, DS_SHADOW | DS_RIGHT );
		}

		// first place
		if ( s1 != SCORE_NOT_PRESENT ) {
			x0 = x;
			s = va( "%2i", s1 );
			w = CG_DrawStrlen( s ) * BIGCHAR_WIDTH + 8;
			x -= w;
			if ( !spectator && score == s1 ) {
				color[0] = 0.0f;
				color[1] = 0.1f;
				color[2] = 1.0f;
				color[3] = 0.33f;
				CG_FillRect( x, y-4,  w, BIGCHAR_HEIGHT+8, color );
				CG_DrawPic( x, y-4, w, BIGCHAR_HEIGHT+8, cgs.media.selectShader );
			} else {
				color[0] = 0.5f;
				color[1] = 0.5f;
				color[2] = 0.5f;
				color[3] = 0.33f;
				CG_FillRect( x, y-4,  w, BIGCHAR_HEIGHT+8, color );
			}
			CG_DrawString( x0-4, y, s, colorWhite, BIGCHAR_WIDTH, BIGCHAR_HEIGHT, 0, DS_SHADOW | DS_RIGHT );
		}

		if ( cgs.fraglimit ) {
			s = va( "%2i", cgs.fraglimit );
			CG_DrawString( x-4, y, s, colorWhite, BIGCHAR_WIDTH, BIGCHAR_HEIGHT, 0, DS_SHADOW | DS_RIGHT );
		}
	}

	return y1 - 8;
}
#endif // MISSIONPACK


/*
================
CG_DrawPowerups
================
*/
#ifndef MISSIONPACK
static float CG_DrawPowerups( float y ) {
	int		sorted[MAX_POWERUPS];
	int		sortedTime[MAX_POWERUPS];
	int		i, j, k;
	int		active;
	playerState_t	*ps;
	int		t;
	gitem_t	*item;
	int		x;
	int		color;
	float	size;
	float	f;
	static const float colors[2][4] = { 
		{ 0.2f, 1.0f, 0.2f, 1.0f },
		{ 1.0f, 0.2f, 0.2f, 1.0f } 
	};

	ps = &cg.snap->ps;

	if ( ps->stats[STAT_HEALTH] <= 0 ) {
		return y;
	}

	// sort the list by time remaining
	active = 0;
	for ( i = 0 ; i < MAX_POWERUPS ; i++ ) {
		if ( !ps->powerups[ i ] ) {
			continue;
		}
		t = ps->powerups[ i ] - cg.time;
		// ZOID--don't draw if the power up has unlimited time (999 seconds)
		// This is true of the CTF flags
		if ( t < 0 || t > 999000) {
			continue;
		}

		// insert into the list
		for ( j = 0 ; j < active ; j++ ) {
			if ( sortedTime[j] >= t ) {
				for ( k = active - 1 ; k >= j ; k-- ) {
					sorted[k+1] = sorted[k];
					sortedTime[k+1] = sortedTime[k];
				}
				break;
			}
		}
		sorted[j] = i;
		sortedTime[j] = t;
		active++;
	}

	// draw the icons and timers
	x = cgs.screenXmax + 1 - ICON_SIZE - CHAR_WIDTH * 2;
	for ( i = 0 ; i < active ; i++ ) {
		item = BG_FindItemForPowerup( sorted[i] );

		if ( item ) {

			color = 1;

			y -= ICON_SIZE;

			trap_R_SetColor( colors[color] );
			CG_DrawField( x, y, 2, sortedTime[ i ] / 1000 );

			t = ps->powerups[ sorted[i] ];
			if ( t - cg.time >= POWERUP_BLINKS * POWERUP_BLINK_TIME ) {
				trap_R_SetColor( NULL );
			} else {
				vec4_t	modulate;

				f = (float)( t - cg.time ) / POWERUP_BLINK_TIME;
				f -= (int)f;
				modulate[0] = modulate[1] = modulate[2] = modulate[3] = f;
				trap_R_SetColor( modulate );
			}

			if ( cg.powerupActive == sorted[i] && 
				cg.time - cg.powerupTime < PULSE_TIME ) {
				f = 1.0 - ( (float)( cg.time - cg.powerupTime ) / PULSE_TIME );
				size = ICON_SIZE * ( 1.0 + ( PULSE_SCALE - 1.0 ) * f );
			} else {
				size = ICON_SIZE;
			}

			CG_DrawPic( cgs.screenXmax + 1 - size, y + ICON_SIZE / 2 - size / 2, 
				size, size, trap_R_RegisterShader( item->icon ) );
		} // if ( item )
	}
	trap_R_SetColor( NULL );

	return y;
}
#endif // MISSIONPACK


/*
=====================
CG_DrawLowerRight

=====================
*/
#ifndef MISSIONPACK
static void CG_DrawLowerRight( void ) {
	float	y;

	y = cgs.screenYmax + 1 - STATUSBAR_HEIGHT;


	y = CG_DrawScores( y );
	y = CG_DrawPowerups( y );
}
#endif // MISSIONPACK


/*
===================
CG_DrawPickupItem
===================
*/
#ifndef MISSIONPACK
static int CG_DrawPickupItem( int y ) {
	int		value;
	float	*fadeColor;
	const char *text;

	if ( cg.snap->ps.stats[STAT_HEALTH] <= 0 ) {
		return y;
	}

	y -= PICKUP_ICON_SIZE;

	value = cg.itemPickup;
	if ( value ) {
		fadeColor = CG_FadeColorTime( cg.itemPickupTime, 3000, 250 );
		if ( fadeColor ) {
			CG_RegisterItemVisuals( value );
			trap_R_SetColor( fadeColor );
			CG_DrawPic( cgs.screenXmin + 8, y, PICKUP_ICON_SIZE, PICKUP_ICON_SIZE, cg_items[ value ].icon );
			if ( cg.itemPickupCount > 1 ) {
				text = va( "%s x%i", bg_itemlist[ value ].pickup_name, cg.itemPickupCount );
			} else {
				text = bg_itemlist[ value ].pickup_name;
			}

			CG_DrawString( cgs.screenXmin + PICKUP_ICON_SIZE + 16, y + (PICKUP_ICON_SIZE/2 - PICKUP_TEXT_SIZE/2), 
				text, fadeColor, PICKUP_TEXT_SIZE, PICKUP_TEXT_SIZE, 0, DS_SHADOW | DS_PROPORTIONAL );
			
			trap_R_SetColor( NULL );
		} else {
			cg.itemPickupCount = 0;
		}
	}
	
	return y;
}
#endif // MISSIONPACK


/*
=====================
CG_DrawLowerLeft
=====================
*/
#ifndef MISSIONPACK
static void CG_DrawLowerLeft( void ) {
	float	y;

	y = cgs.screenYmax + 1 - STATUSBAR_HEIGHT;


	y = CG_DrawPickupItem( y );
}
#endif // MISSIONPACK


//===========================================================================================

/*
=================
CG_DrawTeamInfo
=================
*/
#ifndef MISSIONPACK
static void CG_DrawTeamInfo( void ) {
	int w, h;
	int i, len;
	vec4_t		hcolor;
	int		chatHeight;

	#define CHATLOC_Y (cgs.screenYmax + 1 - STATUSBAR_HEIGHT ) // bottom end
	#define CHATLOC_X (cgs.screenXmin)

	if (cg_teamChatHeight.integer < TEAMCHAT_HEIGHT)
		chatHeight = cg_teamChatHeight.integer;
	else
		chatHeight = TEAMCHAT_HEIGHT;
	if (chatHeight <= 0)
		return; // disabled

	if (cgs.teamLastChatPos != cgs.teamChatPos) {
		if (cg.time - cgs.teamChatMsgTimes[cgs.teamLastChatPos % chatHeight] > cg_teamChatTime.integer) {
			cgs.teamLastChatPos++;
		}

		h = (cgs.teamChatPos - cgs.teamLastChatPos) * TINYCHAR_HEIGHT;

		w = 0;

		for (i = cgs.teamLastChatPos; i < cgs.teamChatPos; i++) {
			len = CG_DrawStrlen(cgs.teamChatMsgs[i % chatHeight]);
			if (len > w)
				w = len;
		}
		w *= TINYCHAR_WIDTH;
		w += TINYCHAR_WIDTH * 2;

		if ( cg.snap->ps.persistant[PERS_TEAM] == TEAM_RED ) {
			hcolor[0] = 1.0f;
			hcolor[1] = 0.0f;
			hcolor[2] = 0.0f;
			hcolor[3] = 0.33f;
		} else if ( cg.snap->ps.persistant[PERS_TEAM] == TEAM_BLUE ) {
			hcolor[0] = 0.0f;
			hcolor[1] = 0.0f;
			hcolor[2] = 1.0f;
			hcolor[3] = 0.33f;
		} else {
			hcolor[0] = 0.0f;
			hcolor[1] = 1.0f;
			hcolor[2] = 0.0f;
			hcolor[3] = 0.33f;
		}

		trap_R_SetColor( hcolor );
		CG_DrawPic( CHATLOC_X, CHATLOC_Y - h, w, h, cgs.media.teamStatusBar );
		trap_R_SetColor( NULL );

		hcolor[0] = hcolor[1] = hcolor[2] = 1.0f;
		hcolor[3] = 1.0f;

		for ( i = cgs.teamChatPos - 1; i >= cgs.teamLastChatPos; i-- ) {
			CG_DrawString( CHATLOC_X + TINYCHAR_WIDTH, CHATLOC_Y - (cgs.teamChatPos - i)*TINYCHAR_HEIGHT, 
				cgs.teamChatMsgs[i % chatHeight], hcolor, TINYCHAR_WIDTH, TINYCHAR_HEIGHT, 0, DS_PROPORTIONAL );
		}
	}
}
#endif // MISSIONPACK


/*
===================
CG_DrawHoldableItem
===================
*/
#ifndef MISSIONPACK
static void CG_DrawHoldableItem( void ) { 
	int		value;

	value = cg.snap->ps.stats[STAT_HOLDABLE_ITEM];
	if ( value ) {
		CG_RegisterItemVisuals( value );
		CG_DrawPic( cgs.screenXmax + 1 - ICON_SIZE, (SCREEN_HEIGHT-ICON_SIZE)/2, ICON_SIZE, ICON_SIZE, cg_items[ value ].icon );
	}
}
#endif // MISSIONPACK

#ifdef MISSIONPACK
/*
===================
CG_DrawPersistantPowerup
===================
*/
#if 0 // sos001208 - DEAD
static void CG_DrawPersistantPowerup( void ) { 
	int		value;

	value = cg.snap->ps.stats[STAT_PERSISTANT_POWERUP];
	if ( value ) {
		CG_RegisterItemVisuals( value );
		CG_DrawPic( 640-ICON_SIZE, (SCREEN_HEIGHT-ICON_SIZE)/2 - ICON_SIZE, ICON_SIZE, ICON_SIZE, cg_items[ value ].icon );
	}
}
#endif
#endif // MISSIONPACK




/*
===============================================================================

LAGOMETER

===============================================================================
*/

#define	LAG_SAMPLES		256


typedef struct {
	int		frameSamples[LAG_SAMPLES];
	int		frameCount;
	int		snapshotFlags[LAG_SAMPLES];
	int		snapshotSamples[LAG_SAMPLES];
	int		snapshotCount;
} lagometer_t;

lagometer_t		lagometer;

/*
==============
CG_AddLagometerFrameInfo

Adds the current interpolate / extrapolate bar for this frame
==============
*/
void CG_AddLagometerFrameInfo( void ) {
	int			offset;

	offset = cg.time - cg.latestSnapshotTime;
	lagometer.frameSamples[ lagometer.frameCount & ( LAG_SAMPLES - 1) ] = offset;
	lagometer.frameCount++;
}


/*
==============
CG_AddLagometerSnapshotInfo

Each time a snapshot is received, log its ping time and
the number of snapshots that were dropped before it.

Pass NULL for a dropped packet.
==============
*/
void CG_AddLagometerSnapshotInfo( snapshot_t *snap ) {
	// dropped packet
	if ( !snap ) {
		lagometer.snapshotSamples[ lagometer.snapshotCount & ( LAG_SAMPLES - 1) ] = -1;
		lagometer.snapshotCount++;
		return;
	}

	// add this snapshot's info
	lagometer.snapshotSamples[ lagometer.snapshotCount & ( LAG_SAMPLES - 1) ] = snap->ping;
	lagometer.snapshotFlags[ lagometer.snapshotCount & ( LAG_SAMPLES - 1) ] = snap->snapFlags;
	lagometer.snapshotCount++;
}


/*
==============
CG_DrawDisconnect

Should we draw something differnet for long lag vs no packets?
==============
*/
/* Retail 0x1000aef0/0x10012790: white, scale 0.25, font 0, shadow style
 * 3, centered on x 320 with its baseline at y 116; the net icon blinks at
 * (592, 432) in the centered canvas. */
static void CG_DrawDisconnect( void ) {
	static vec4_t white = { 1, 1, 1, 1 };
	const char *s = "Connection Interrupted";

	if (!CG_IsConnectionInterrupted()) {
		return;
	}
	CG_SetTextFont( 0 );
	CG_Text_Paint( 320 - CG_Text_Width( s, 0.25f, 0 ) / 2, 100 + 16, 0.25f, white, s, 0, 0, ITEM_TEXTSTYLE_SHADOWED );

	// blink the icon
	if ( cg.time & 0x200 ) {
		return;
	}
	CG_DrawPic( 592, 432, 48, 48, trap_R_RegisterShader( "gfx/2d/net.tga" ) );
}


#define	MAX_LAGOMETER_PING	900
#define	MAX_LAGOMETER_RANGE	300

/*
==============
CG_DrawLagometer
==============
*/
static void CG_DrawLagometer( void ) {
	int		a, x, y, i;
	float	v;
	float	ax, ay, aw, ah, mid, range;
	int		color, d;
	float	vscale;

	// retail 0x1000b037: off, on a local server or in a demo, no graph
	if ( !cg_lagometer.integer || cgs.localServer || cg.demoPlayback ) {
		return;
	}

	//
	// draw the graph
	//
#ifdef MISSIONPACK
	x = cgs.screenXmax + 1 - 48;
	y = cgs.screenYmax + 1 - 144;
#else
	x = cgs.screenXmax + 1 - 48;
	y = cgs.screenYmax + 1 - 48;
#endif

	trap_R_SetColor( NULL );
	CG_DrawPic( x, y, 48, 48, cgs.media.lagometerShader );

	ax = x;
	ay = y;
	aw = 48;
	ah = 48;
	CG_AdjustFrom640( &ax, &ay, &aw, &ah );

	color = -1;
	range = ah / 3;
	mid = ay + range;

	vscale = range / MAX_LAGOMETER_RANGE;

	d = (int)(ax + aw);

	// draw the frame interpoalte / extrapolate graph
	for ( a = 0 ; a < aw ; a++ ) {
		i = ( lagometer.frameCount - 1 - a ) & (LAG_SAMPLES - 1);
		v = lagometer.frameSamples[i];
		v *= vscale;
		if ( v > 0 ) {
			if ( color != 1 ) {
				color = 1;
				trap_R_SetColor( g_color_table[ColorIndex(COLOR_YELLOW)] );
			}
			if ( v > range ) {
				v = range;
			}
			trap_R_DrawStretchPic ( d - a, mid - v, 1, v, 0, 0, 0, 0, cgs.media.whiteShader );
		} else if ( v < 0 ) {
			if ( color != 2 ) {
				color = 2;
				trap_R_SetColor( g_color_table[ColorIndex(COLOR_BLUE)] );
			}
			v = -v;
			if ( v > range ) {
				v = range;
			}
			trap_R_DrawStretchPic( d - a, mid, 1, v, 0, 0, 0, 0, cgs.media.whiteShader );
		}
	}

	// draw the snapshot latency / drop graph
	range = ah / 2;
	vscale = range / MAX_LAGOMETER_PING;

	for ( a = 0 ; a < aw ; a++ ) {
		i = ( lagometer.snapshotCount - 1 - a ) & (LAG_SAMPLES - 1);
		v = lagometer.snapshotSamples[i];
		if ( v > 0 ) {
			if ( lagometer.snapshotFlags[i] & SNAPFLAG_RATE_DELAYED ) {
				if ( color != 5 ) {
					color = 5;	// YELLOW for rate delay
					trap_R_SetColor( g_color_table[ColorIndex(COLOR_YELLOW)] );
				}
			} else {
				if ( color != 3 ) {
					color = 3;
					trap_R_SetColor( g_color_table[ColorIndex(COLOR_GREEN)] );
				}
			}
			v = v * vscale;
			if ( v > range ) {
				v = range;
			}
			trap_R_DrawStretchPic( d - a, ay + ah - v, 1, v, 0, 0, 0, 0, cgs.media.whiteShader );
		} else if ( v < 0 ) {
			if ( color != 4 ) {
				color = 4;		// RED for dropped snapshots
				trap_R_SetColor( g_color_table[ColorIndex(COLOR_RED)] );
			}
			trap_R_DrawStretchPic( d - a, ay + ah - range, 1, range, 0, 0, 0, 0, cgs.media.whiteShader );
		}
	}

	trap_R_SetColor( NULL );

	if ( cg_nopredict.integer || cgs.synchronousClients ) {
		CG_DrawString( cgs.screenXmax-1, y, "snc", colorWhite, 5, 10, 0, DS_PROPORTIONAL | DS_RIGHT );
	}

	if ( !cg.demoPlayback ) {
		CG_DrawString( x+1, y, va( "%ims", cg.meanPing ), colorWhite, 5, 10, 0, DS_PROPORTIONAL );
	}
}



/*
===============================================================================

CENTER PRINTING

===============================================================================
*/


/*
==============
CG_CenterPrint

Called for important messages that should stay in the center of the screen
for a few moments
==============
*/
void CG_CenterPrint( const char *str, int y, int charWidth ) {
	char	*s;
	CG_ClearMatchNotice();

	Q_strncpyz( cg.centerPrint, str, sizeof(cg.centerPrint) );

	cg.centerPrintTime = cg.time;
	cg.centerPrintY = y;
	cg.centerPrintCharWidth = charWidth;

	// count the number of lines for centering
	cg.centerPrintLines = 1;
	s = cg.centerPrint;
	while( *s ) {
		if (*s == '\n')
			cg.centerPrintLines++;
		s++;
	}
}


/*
===================
CG_DrawCenterString
===================
*/
static void CG_DrawCenterString( void ) {
	char	*start;
	int		l;
	int		y;
#ifdef MISSIONPACK // bk010221 - unused else
  int h, w, x;
#endif
	float	*color;
	if (CG_DrawMatchNotice()) return;

	if ( !cg.centerPrintTime ) {
		return;
	}

	color = CG_FadeColor( cg.centerPrintTime, 1000 * cg_centertime.value );
	if ( !color ) {
		return;
	}

	trap_R_SetColor( color );

	start = cg.centerPrint;

	y = cg.centerPrintY - cg.centerPrintLines * BIGCHAR_HEIGHT / 2;

	while ( 1 ) {
		char linebuffer[1024];

		for ( l = 0; l < 50; l++ ) {
			if ( !start[l] || start[l] == '\n' ) {
				break;
			}
			linebuffer[l] = start[l];
		}
		linebuffer[l] = 0;

#ifdef MISSIONPACK
		w = CG_Text_Width(linebuffer, 0.5, 0);
		h = CG_Text_Height(linebuffer, 0.5, 0);
		x = (SCREEN_WIDTH - w) / 2;
		CG_Text_Paint(x, y + h, 0.5, color, linebuffer, 0, 0, ITEM_TEXTSTYLE_SHADOWEDMORE);
		y += h + 6;
#else
		CG_DrawString( 320, y, linebuffer, color, cg.centerPrintCharWidth, cg.centerPrintCharWidth * 1.5, 0, DS_SHADOW | DS_CENTER | DS_PROPORTIONAL );

		y += cg.centerPrintCharWidth * 1.5;
#endif
		while ( *start && ( *start != '\n' ) ) {
			start++;
		}
		if ( !*start ) {
			break;
		}
		start++;
	}

	trap_R_SetColor( NULL );
}



/*
================================================================================

CROSSHAIR

================================================================================
*/


/*
=================
CG_DrawCrosshair
=================
*/
static void CG_DrawCrosshair( void ) {
	// retail 0x1000c2a0: colour, brightness, pulse and hit styles
	CG_DrawQLCrosshair();
}



/*
=================
CG_ScanForCrosshairEntity
=================
*/
static void CG_ScanForCrosshairEntity( void ) {
	trace_t		trace;
	vec3_t		start, end;
	int			content;

	VectorCopy( cg.refdef.vieworg, start );
	VectorMA( start, 131072, cg.refdef.viewaxis[0], end );

	CG_Trace( &trace, start, vec3_origin, vec3_origin, end, 
		cg.snap->ps.clientNum, CONTENTS_SOLID|CONTENTS_BODY );
	if ( trace.entityNum >= MAX_CLIENTS ) {
		return;
	}

	// `CG_Trace` might trace based on a very old entity's position
	// (`cg_solidEntities` is build from next snap,
	// but `CG_ClipMoveToEntities` checks `currentState`),
	// so we hack around it here.
	// See https://github.com/ioquake/ioq3/issues/732.
	if ( !cg_entities[ trace.entityNum ].currentValid ) {
		return;
	}

	// if the player is in fog, don't show it
	content = CG_PointContents( trace.endpos, 0 );
	if ( content & CONTENTS_FOG ) {
		return;
	}

	// if the player is invisible, don't show it
	if ( cg_entities[ trace.entityNum ].currentState.powerups & ( 1 << PW_INVIS ) ) {
		return;
	}

	// update the fade timer
	cg.crosshairClientNum = trace.entityNum;
	cg.crosshairClientTime = cg.time;
}


/*
=====================
CG_DrawCrosshairNames
=====================
*/
static void CG_DrawCrosshairNames( void ) {
	float		*color;

	// retail 0x1000c670: cg_drawCrosshair gates names; never in third person
	if ( !cg_drawCrosshair.integer || cg.renderingThirdPerson ) {
		return;
	}

	// scan the known entities to see if the crosshair is sighted on one
	CG_ScanForCrosshairEntity();

	color = CG_FadeColor( cg.crosshairClientTime, 1000 );
	if ( color ) {
		CG_DrawCrosshairNameText( color );
	}
	trap_R_SetColor( NULL );
}


//==============================================================================

/*
=================
CG_DrawSpectator
=================
*/


/*
=================
CG_DrawTeamVote
=================
*/
static void CG_DrawTeamVote(void) {
	char	*s;
	int		sec, cs_offset;

	if ( cgs.clientinfo[ cg.clientNum ].team == TEAM_RED )
		cs_offset = 0;
	else if ( cgs.clientinfo[ cg.clientNum ].team == TEAM_BLUE )
		cs_offset = 1;
	else
		return;

	if ( !cgs.teamVoteTime[cs_offset] ) {
		return;
	}

	// play a talk beep whenever it is modified
	if ( cgs.teamVoteModified[cs_offset] ) {
		cgs.teamVoteModified[cs_offset] = qfalse;
		trap_S_StartLocalSound( cgs.media.talkSound, CHAN_LOCAL_SOUND );
	}

	sec = ( VOTE_TIME - ( cg.time - cgs.teamVoteTime[cs_offset] ) ) / 1000;
	if ( sec < 0 ) {
		sec = 0;
	}
	s = va("TEAMVOTE(%i):%s yes:%i no:%i", sec, cgs.teamVoteString[cs_offset],
							cgs.teamVoteYes[cs_offset], cgs.teamVoteNo[cs_offset] );

	CG_DrawString( cgs.screenXmin - 0, 90, s, colorWhite, SMALLCHAR_WIDTH, SMALLCHAR_HEIGHT, 0, DS_PROPORTIONAL ); // DS_SHADOW?
}


static qboolean CG_DrawScoreboard( void ) {
#ifdef MISSIONPACK
	static qboolean firstTime = qtrue;
	float fade, *fadeColor;

	if (menuScoreboard) {
		menuScoreboard->window.flags &= ~WINDOW_FORCED;
	}
	if (cg_paused.integer) {
		cg.deferredPlayerLoading = 0;
		firstTime = qtrue;
		return qfalse;
	}

	// Retail 0x1000dfd0: the scoreboard shows while held, while dead (in
	// warmup too) and at intermission, with no fade after it is released.
	if ( cg.showScores || cg.predictedPlayerState.pm_type == PM_DEAD || cg.predictedPlayerState.pm_type == PM_INTERMISSION ) {
		fade = 1.0;
		fadeColor = colorWhite;
	} else {
		// next time scoreboard comes up, don't print killer
		cg.deferredPlayerLoading = 0;
		cg.killerName[0] = 0;
		firstTime = qtrue;
		return qfalse;
	}																					  


    {
        menuDef_t *next = Menus_FindByName(CG_ScoreMenuName(cg.predictedPlayerState.pm_type == PM_INTERMISSION));
        if (next != menuScoreboard) firstTime = qtrue;
        menuScoreboard = next;
    }

	if (menuScoreboard) {
		if (firstTime) {
			CG_SetScoreSelection(menuScoreboard);
			firstTime = qfalse;
		}
		Menu_Paint(menuScoreboard, qtrue);
	}

	// load any models that have been deferred
	if ( ++cg.deferredPlayerLoading > 10 ) {
		CG_LoadDeferredPlayers();
	}

	return qtrue;
#else
	return CG_DrawOldScoreboard();
#endif
}


/*
=================
CG_DrawIntermission
=================
*/
static void CG_DrawIntermission( void ) {
	cg.scoreFadeTime = cg.time;
	cg.scoreBoardShowing = CG_DrawScoreboard();
}


/*
=================
CG_DrawFollow
=================
*/
static qboolean CG_DrawFollow( void ) {
	// Retail draws no follow banner: the spectator follow HUD names the
	// player (CG_FOLLOW_PLAYER_NAME_EX). Following still holds back the
	// pregame text.
	return ( cg.snap->ps.pm_flags & PMF_FOLLOW ) != 0;
}






#ifdef MISSIONPACK
/*
=================
CG_DrawProxWarning
=================
*/
static void CG_DrawProxWarning( void ) {
	char s [32];
	static int proxTime;
	static int proxCounter;
	static int proxTick;

	if( !(cg.snap->ps.eFlags & EF_TICKING ) ) {
		proxTime = 0;
		return;
	}

	if ( proxTime == 0 ) {
		proxTime = cg.time + 5000;
		proxCounter = 5;
		proxTick = 0;
	}

	if (cg.time > proxTime) {
		proxTick = proxCounter--;
		proxTime = cg.time + 1000;
	}

	if (proxTick != 0) {
		Com_sprintf( s, sizeof(s), "INTERNAL COMBUSTION IN: %i", proxTick );
	} else {
		Com_sprintf( s, sizeof(s), "YOU HAVE BEEN MINED" );
	}

	CG_DrawString( 320, 64 + 64 + BIGCHAR_HEIGHT, s, g_color_table[ColorIndex(COLOR_RED)], BIGCHAR_WIDTH, BIGCHAR_HEIGHT, 0, DS_SHADOW | DS_FORCE_COLOR | DS_CENTER );
}
#endif


//==================================================================================
#ifdef MISSIONPACK
/* 
=================
CG_DrawTimedMenus
=================
*/
void CG_DrawTimedMenus( void ) {
	if (cg.voiceTime) {
		int t = cg.time - cg.voiceTime;
		if ( t > 2500 ) {
			Menus_CloseByName("voiceMenu");
			trap_Cvar_Set("cl_conXOffset", "0");
			cg.voiceTime = 0;
		}
	}
}
#endif


/*
=================
CG_Draw2D
=================
*/
static void CG_Draw2D( stereoFrame_t stereoFrame )
{
	// retail 0x10010de3: the message and notice flags lapse after their time
	if ( cg.time > cg.messageTime ) {
		cg.messageTime = 0;
	}
	if ( cg.time > cg.noticeTime ) {
		cg.noticeTime = 0;
	}
#ifdef MISSIONPACK
	if (cgs.orderPending && cg.time > cgs.orderTime) {
		CG_CheckOrderPending();
	}
#endif
	// if we are taking a levelshot for the menu, don't draw anything
	if ( cg.levelShot ) {
		return;
	}

	// Retail 0x10010d90: the engine drops mouse input while
	// cg_ignoreMouseInput is set; during a timeout it follows the inverse of
	// cl_paused, otherwise it is cleared. (Retail's g_training branch is not
	// hosted.)
	if ( atoi( CG_ConfigString( CS_TIMEOUT_START_TIME ) ) ) {
		if ( cg_ignoreMouseInput.integer == cg_paused.integer ) {
			trap_Cvar_Set( "cg_ignoreMouseInput", va( "%d", !cg_paused.integer ) );
		}
	} else if ( cg_ignoreMouseInput.integer ) {
		trap_Cvar_Set( "cg_ignoreMouseInput", "0" );
	}

	// retail: with the 2D view off or the client paused, only the forced
	// crosshair with its names remains (centre placement; retail's snapshot
	// line for cg_drawSnapshot 2 is not drawn here)
	if ( cg_draw2D.integer == 0 || cg_paused.integer == 1 ) {
		if ( cg_forceDrawCrosshair.integer ) {
			int placement = CG_SetScreenPlacement( CG_PLACE_CENTER );
			CG_DrawCrosshair();
			CG_DrawCrosshairNames();
			CG_SetScreenPlacement( placement );
		}
		return;
	}

	CG_CheckInitialScoreboard();	// retail 0x1000e9c0

	if ( cg.snap->ps.pm_type == PM_INTERMISSION ) {
		CG_DrawIntermission();
		return;
	}

	CG_DrawDamagePlums();
	CG_DrawRaceMarkers();
	CG_DrawPointsOfInterest();
	CG_DrawOverheadNames();

/*
	if (cg.cameraMode) {
		return;
	}
*/
	if ( CG_SpectatorHudActive() ) {
		// retail 0x10010feb: the spectator menus; a followed view adds its
		// crosshair, names, weapon list and rewards (0x1000ac30, inferred)
		CG_DrawSpectatorHud();
		if ( cg.snap->ps.pm_flags & PMF_FOLLOW ) {
			CG_DrawCrosshair();
			CG_DrawCrosshairNames();
			CG_DrawWeaponSelect();
			{	// retail 0x1000ac30 at the centre placement
				int placement = CG_SetScreenPlacement( CG_PLACE_CENTER );
				CG_DrawRewards();
				CG_SetScreenPlacement( placement );
			}
		}
		CG_SetScreenPlacement( CG_PLACE_LEFT );	// retail 0x100110af..0x100110ba
		CG_DrawItemTimers();
		CG_SetScreenPlacement( CG_PLACE_DEFAULT );
		CG_DrawTeamVitals();	// retail 0x100110c0
	} else {
		// don't draw any status if dead or the scoreboard is being explicitly shown
		if ( !cg.showScores && cg.snap->ps.stats[STAT_HEALTH] > 0 ) {

#ifdef MISSIONPACK
			if ( cg_drawStatus.integer ) {
				Menu_PaintAll();
				CG_DrawTimedMenus();
			}
#else
			CG_DrawStatusBar();
#endif
      
			CG_DrawAmmoWarning();

#ifdef MISSIONPACK
			CG_DrawProxWarning();
#endif      
			CG_DrawCrosshair();
			CG_DrawCrosshairNames();
			CG_DrawWeaponSelect();

#ifndef MISSIONPACK
			CG_DrawHoldableItem();
#else
			//CG_DrawPersistantPowerup();
#endif
			{	// retail 0x1000ac30 at the centre placement
				int placement = CG_SetScreenPlacement( CG_PLACE_CENTER );
				CG_DrawRewards();
				CG_SetScreenPlacement( placement );
			}
		}
    
		if ( cgs.gametype >= GT_TEAM ) {
#ifndef MISSIONPACK
			CG_DrawTeamInfo();
#endif
		}
	}

	CG_DrawVote();
	CG_DrawTeamVote();

	// retail 0x100110d3..0x100110f2: the lagometer, then the connection
	// warning, which demo playback skips
	CG_DrawLagometer();
	if ( !cg.demoPlayback ) {
		CG_DrawDisconnect();
	}
	// retail 0x100110f7 / 0x10011102: input commands (centre placement)
	// and the speedometer
	{
		int placement = CG_SetScreenPlacement( CG_PLACE_CENTER );
		CG_DrawInputCommands();
		CG_SetScreenPlacement( placement );
	}
	CG_DrawSpeedometer();

#ifdef MISSIONPACK
	if (!cg_paused.integer) {
		CG_DrawUpperRight(stereoFrame);
		// retail 0x1000abf0 at the left placement: the item pickup line
		// above y 408
		{
			int placement = CG_SetScreenPlacement( CG_PLACE_LEFT );
			CG_DrawItemPickups( 408 );
			CG_SetScreenPlacement( placement );
		}
	}
#else
	CG_DrawUpperRight(stereoFrame);
#endif

#ifndef MISSIONPACK
	CG_DrawLowerRight();
	CG_DrawLowerLeft();
#endif

	CG_DrawMatchPause();
	CG_AdvanceMatchClocks();

	// don't draw center string if scoreboard is up
	// retail 0x10011186..0x100111c9: without a scoreboard, the centre print
	// (0x1000beb0, held back until the initial scoreboard is released).
	// Retail also shows no warmup lines under a scoreboard (a dead player in
	// warmup sees the scoreboard and no countdown); where retail draws them
	// is not identified, so placing them here is an inference.
	cg.scoreBoardShowing = CG_DrawScoreboard();
	if ( !cg.scoreBoardShowing ) {
		if ( !CG_DrawFollow() ) {
			CG_DrawPregame();
		}
		if ( !cg.initialScoreboard ) {
			CG_DrawCenterString();
		}
	}
	CG_DrawVoiceIndicator();	// retail 0x10010d90, after the centre print
	if (CG_AccuracyOverlayVisible()) {
		menuDef_t *stats = Menus_FindByName("stats_menu");
		CG_SetAccuracyOverlay(qtrue);
		if (stats) Menu_Paint(stats,qtrue);
	}

	// retail 0x10011418: the demo controls are the last 2D element, over
	// the scoreboard and the chat it shows
	CG_DrawDemoHud();

	// Retail cgame registers menu/art/3_cursor2 (0x10024ce1) but never
	// draws it: the engine shows the system pointer while the cgame key
	// catcher owns input, so the scoreboard catcher draws no sprite cursor.
}


static void CG_DrawTourneyScoreboard( void ) {
#ifdef MISSIONPACK
#else
	CG_DrawOldTourneyScoreboard();
#endif
}


static void CG_CalculatePing( void ) {
	int count, i, v;

	cg.meanPing = 0;

	for ( i = 0, count = 0; i < LAG_SAMPLES; i++ ) {

		v = lagometer.snapshotSamples[i];
		if ( v >= 0 ) {
			cg.meanPing += v;
			count++;
		}

	}

	if ( count ) {
		cg.meanPing /= count;
	}
}


// will be called on warmup end and when client changed
void CG_WarmupEvent( void ) {

	cg.attackerTime = 0;
	cg.attackerName[0] = '\0';

	cg.itemPickupTime = 0;
	cg.itemPickupBlendTime = 0;
	cg.itemPickupCount = 0;

	cg.killerTime = 0;
	cg.killerName[0] = '\0';
	
	cg.damageTime = 0;

	cg.rewardStack = 0;
	cg.rewardTime = 0;
	
	cg.weaponSelectTime = cg.time;

	cg.lowAmmoWarning = 0;

	cg.followTime = 0;
}


// called each time client team changed
static void CG_ApplyClientChange( void )
{
	CG_WarmupEvent();
	CG_ForceModelChange();
}


/*
=====================
CG_TrackClientTeamChange
=====================
*/
void CG_TrackClientTeamChange( void ) 
{
	static int spec_client = -1;
	static int spec_team = -1;
	static int curr_team = -1;

	int		ti; // team from clientinfo 
	int		tp; // persistant team from snapshot

	if ( !cg.snap )
		return;

	tp = cg.snap->ps.persistant[ PERS_TEAM ];
	ti = cgs.clientinfo[ cg.snap->ps.clientNum ].team;

	if ( !(cg.snap->ps.pm_flags & PMF_FOLLOW) && tp != TEAM_SPECTATOR ) {
		ti = tp; // use team from persistant info
	}

	// team changed
	if ( curr_team != ti )
	{
		curr_team = ti;
		spec_client = cg.snap->ps.clientNum;
		spec_team = tp;

		if ( spec_team == TEAM_SPECTATOR )
			spec_team = curr_team;

		CG_ApplyClientChange();
		CG_ResetPlayerEntity( &cg.predictedPlayerEntity );
		return;
	}

	if ( curr_team == TEAM_SPECTATOR )
	{
		if ( spec_team != tp )
		{
			spec_team  = tp;
			spec_client = cg.snap->ps.clientNum;

			CG_ApplyClientChange();
			CG_ResetPlayerEntity( &cg.predictedPlayerEntity );
			return;
		}

		if ( cgs.gametype >= GT_TEAM ) 
		{
			spec_client = cg.snap->ps.clientNum;
			return;
		}
		// pass through to spec client checks
	}
	
	if ( spec_client != cg.snap->ps.clientNum ) 
	{
		spec_client = cg.snap->ps.clientNum;
		spec_team = tp;

		if ( spec_team == TEAM_SPECTATOR )
			spec_team = cgs.clientinfo[ cg.snap->ps.clientNum ].team;

		CG_ApplyClientChange();
		CG_ResetPlayerEntity( &cg.predictedPlayerEntity );
	}
}


/*
=====================
CG_DrawActive

Perform all drawing needed to completely fill the screen
=====================
*/
void CG_DrawActive( stereoFrame_t stereoView ) {
	// optionally draw the info screen instead
	if ( !cg.snap ) {
		CG_DrawInformation();
		return;
	}

	if ( !cg.demoPlayback ) {
		CG_CalculatePing();
	}

	// optionally draw the tournement scoreboard instead
	if ( cg.snap->ps.persistant[PERS_TEAM] == TEAM_SPECTATOR &&
		( cg.snap->ps.pm_flags & PMF_SCOREBOARD ) ) {
		CG_DrawTourneyScoreboard();
		return;
	}

	// Retail 10011520 resets zoom after this frame's view/command calculation.
	CG_UpdateZoomState();

	// clear around the rendered view if sized down
	CG_TileClear();

	// draw 3D view; retail 0x10011490 offsets each stereo eye by half of
	// cg_stereoSeparation along the view's left axis for the scene only
	{
		float separation = 0;
		vec3_t baseOrg;
		if ( stereoView == STEREO_LEFT ) {
			separation = -cg_stereoSeparation.value * 0.5f;
		} else if ( stereoView == STEREO_RIGHT ) {
			separation = cg_stereoSeparation.value * 0.5f;
		}
		VectorCopy( cg.refdef.vieworg, baseOrg );
		if ( separation != 0 ) {
			VectorMA( cg.refdef.vieworg, -separation, cg.refdef.viewaxis[1], cg.refdef.vieworg );
		}
		trap_R_RenderScene( &cg.refdef );
		VectorCopy( baseOrg, cg.refdef.vieworg );
	}

	// retail 0x10011666: the vignette, unless the web UI is up or the view
	// is third person
	if ( !cg.renderingThirdPerson && trap_Cvar_VariableValue( "web_browserActive" ) == 0 ) {
		CG_DrawVignette();
	}

	// draw status bar and other floating elements
 	CG_Draw2D( stereoView );
}
