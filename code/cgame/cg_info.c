// Copyright (C) 1999-2000 Id Software, Inc.
//
// cg_info.c -- display information while data is being loading
//
// FnQL-game: the Quake Live loading screen, rewritten from the retail
// cgamex86 (addresses below). Observed:
//
// - CG_LoadingString (0x1001cb80) copies the text into cg.infoScreenText
//   unchanged, then calls trap_UpdateScreen, the advertisement bridge's
//   UpdateLoadingViewParameters (import 77) and SetFrameTime(1000) (64).
// - Items (0x100249d0) and clients (0x1001cbe0) still register their icons
//   (up to 38 items and 16 clients) but the screen never draws them. A
//   client's text is its "n" key cut to 63 characters with ^0-^7 codes and
//   control characters removed (0x100574c0); an empty name stays empty.
// - CG_Init (0x10029c58) bumps a progress counter after the collision map,
//   sounds, graphics and clients stages and once more before the final
//   empty loading string. The counter lives for the module, never reset.
// - CG_DrawInformation (0x1001ce10) rereads CS_SETTINGS_MASK, then draws
//   either:
//   * with no loading text but a snapshot (one flagged not active),
//     backscreen_smoke full screen and "Awaiting gamestate..." centred at
//     y 178 (font 0, 0.3, shadowed); or
//   * the levelshot (menu/art/unknownmap when missing) full screen; on the
//     left a 0.75 dark-red bar (-400, 0, 1440, 64) under logo_background and
//     ql_logo at (0, 0, 256, 64), a 0.75 black bar (-400, 400, 1440, 80) with
//     CS_MESSAGE at x 10 (font 0, 0.8) and CS_MAP_AUTHOR / CS_MAP_AUTHOR2
//     below it (font 1, 0.25); on the right gt_background at (384, 0, 256,
//     64) and the gametype name right-aligned to x 620 (font 0, 0.8); in the
//     centre "Loading %s" or "Awaiting Snapshot" (font 1, 0.3, shadowed) at
//     y 370, its x halving the width with integer division, over four 8x8
//     squares from x 292 every 16 at y 380, opaque dark red once their stage
//     is done and 0.75 black before.
//   Each text's y is its own measured height plus 400, 447, 462 or 10.
//   Full-screen images keep their 16:9 centre: the horizontal texture
//   window is cropped by (1920 - 1080 / height * width) / 1920 / 2 per side.

#include "cg_local.h"
#include "../ui/ui_shared.h"
#include <limits.h>

#define MAX_LOADING_PLAYER_ICONS	16
#define MAX_LOADING_ITEM_ICONS		38
#define LOADING_PROGRESS_SQUARES	4

static int			loadingPlayerIconCount;
static int			loadingItemIconCount;
static qhandle_t	loadingPlayerIcons[MAX_LOADING_PLAYER_ICONS];
static qhandle_t	loadingItemIcons[MAX_LOADING_ITEM_ICONS];
static int			loadingStagesDone;

static const char *const loadingGametypeNames[] = {
	"Free For All", "Duel", "Race", "Team Deathmatch", "Clan Arena", "Capture the Flag",
	"One Flag CTF", "Overload", "Harvester", "Freeze Tag", "Domination", "Attack and Defend",
	"Red Rover"
};

static vec4_t loadingWhite = { 1, 1, 1, 1 };
static vec4_t loadingTopBar = { 0.375f, 0.125f, 0.125f, 0.75f };
static vec4_t loadingShade = { 0, 0, 0, 0.75f };
static vec4_t loadingDone = { 0.375f, 0.125f, 0.125f, 1 };

/*
======================
CG_LoadingString
======================
*/
void CG_LoadingString( const char *s ) {
	Q_strncpyz( cg.infoScreenText, s ? s : "", sizeof( cg.infoScreenText ) );
	trap_UpdateScreen();
	trap_AdvertisementBridge_UpdateLoadingViewParameters();
	trap_AdvertisementBridge_SetFrameTime( 1000 );
}

void CG_LoadingStageDone( void ) {
	if ( loadingStagesDone < INT_MAX ) {
		loadingStagesDone++;
	}
}

int CG_LoadingStagesDone( void ) {
	return loadingStagesDone;
}

/*
===================
CG_LoadingItem
===================
*/
void CG_LoadingItem( int itemNum ) {
	const gitem_t *item;

	if ( itemNum < 0 || itemNum >= bg_numItems ) {
		return;
	}
	item = &bg_itemlist[itemNum];
	if ( item->icon && loadingItemIconCount < MAX_LOADING_ITEM_ICONS ) {
		loadingItemIcons[loadingItemIconCount++] = trap_R_RegisterShaderNoMip( item->icon );
	}
	CG_LoadingString( item->pickup_name );
}

/* Retail 0x100574c0: drop ^0-^7 colour codes and bytes below 0x20; bytes
 * from 0x80 stay. */
void CG_LoadingCleanName( char *name ) {
	const char *in = name;
	char *out = name;

	if ( !name ) {
		return;
	}
	for ( ; *in; ++in ) {
		if ( in[0] == '^' && in[1] >= '0' && in[1] <= '7' ) {
			++in;
		} else if ( (unsigned char)*in >= 0x20 ) {
			*out++ = *in;
		}
	}
	*out = '\0';
}

/*
===================
CG_LoadingClient
===================
*/
void CG_LoadingClient( int clientNum ) {
	const char		*info;
	char			*skin;
	char			name[64];
	char			model[64];
	char			iconName[64];

	if ( clientNum < 0 || clientNum >= MAX_CLIENTS ) {
		return;
	}
	info = CG_ConfigString( CS_PLAYERS + clientNum );

	if ( loadingPlayerIconCount < MAX_LOADING_PLAYER_ICONS ) {
		qhandle_t *icon = &loadingPlayerIcons[loadingPlayerIconCount];

		Q_strncpyz( model, Info_ValueForKey( info, "model" ), sizeof( model ) );
		skin = Q_strrchr( model, '/' );
		if ( skin ) {
			*skin++ = '\0';
		} else {
			skin = "default";
		}

		Com_sprintf( iconName, sizeof( iconName ), "models/players/%s/icon_%s.tga", model, skin );
		*icon = trap_R_RegisterShaderNoMip( iconName );
		if ( !*icon ) {
			Com_sprintf( iconName, sizeof( iconName ), "models/players/characters/%s/icon_%s.tga", model, skin );
			*icon = trap_R_RegisterShaderNoMip( iconName );
		}
		if ( !*icon ) {
			*icon = trap_R_RegisterShaderNoMip( "models/players/sarge/icon_default.tga" );
		}
		if ( *icon ) {
			loadingPlayerIconCount++;
		}
	}

	Q_strncpyz( name, Info_ValueForKey( info, "n" ), sizeof( name ) );
	CG_LoadingCleanName( name );
	CG_LoadingString( name );
}

const char *CG_LoadingGametypeName( int gametype ) {
	if ( gametype < 0 || gametype >= (int)ARRAY_LEN( loadingGametypeNames ) ) {
		return "Unknown Gametype";
	}
	return loadingGametypeNames[gametype];
}

/* Retail draws full-screen images with the texture window cropped to the
 * screen's aspect around a 1920x1080 centre. */
float CG_LoadingImageCrop( int width, int height ) {
	if ( width <= 0 || height <= 0 ) {
		return 0;
	}
	return (float)( ( 1920.0 - 1080.0 / height * width ) / 1920.0 * 0.5 );
}

static void CG_DrawLoadingImage( qhandle_t shader ) {
	const float crop = CG_LoadingImageCrop( cgs.glconfig.vidWidth, cgs.glconfig.vidHeight );
	trap_R_DrawStretchPic( 0, 0, (float)cgs.glconfig.vidWidth, (float)cgs.glconfig.vidHeight,
		crop, 0, 1 - crop, 1, shader );
}

static void CG_DrawLoadingText( float x, float y, int font, float scale, const char *text, int style ) {
	CG_SetTextFont( font );
	CG_Text_Paint( x, y, scale, loadingWhite, text, 0, 0, style );
}

static void CG_DrawLoadingLine( const char *text, int font, float scale, int top ) {
	const int height = CG_Text_HeightInFont( text, font, scale, 0 );
	CG_DrawLoadingText( 10, (float)( height + top ), font, scale, text, ITEM_TEXTSTYLE_NORMAL );
}

/*
====================
CG_DrawInformation

Draw all the status / pacifier stuff during level loading
====================
*/
void CG_DrawInformation( void ) {
	const char	*text;
	const char	*gametype;
	qhandle_t	levelshot;
	int			previous;
	int			width, height, i;

	trap_R_SetColor( NULL );
	CG_UpdatePlayerPresentationSettings();

	levelshot = trap_R_RegisterShaderNoMip( va( "levelshots/%s.tga",
		Info_ValueForKey( CG_ConfigString( CS_SERVERINFO ), "mapname" ) ) );
	if ( !levelshot ) {
		levelshot = trap_R_RegisterShaderNoMip( "menu/art/unknownmap" );
	}
	trap_R_SetColor( NULL );

	if ( !cg.infoScreenText[0] && cg.snap ) {
		static const char awaiting[] = "Awaiting gamestate...";
		previous = CG_SetScreenPlacement( CG_PLACE_CENTER );
		CG_DrawLoadingImage( cgs.media.loadingSmokeShader );
		width = CG_Text_WidthInFont( awaiting, 0, 0.3f, 0 );
		CG_DrawLoadingText( (float)( 320.0 - width * 0.5 ), 178, 0, 0.3f, awaiting, ITEM_TEXTSTYLE_SHADOWED );
		CG_SetScreenPlacement( previous );
		return;
	}

	CG_DrawLoadingImage( levelshot );

	previous = CG_SetScreenPlacement( CG_PLACE_LEFT );
	CG_FillRect( -400, 0, 1440, 64, loadingTopBar );
	CG_DrawPic( 0, 0, 256, 64, cgs.media.loadingLogoBackShader );
	CG_DrawPic( 0, 0, 256, 64, cgs.media.loadingLogoShader );
	CG_FillRect( -400, 400, 1440, 80, loadingShade );
	CG_DrawLoadingLine( CG_ConfigString( CS_MESSAGE ), 0, 0.8f, 400 );
	text = CG_ConfigString( CS_MAP_AUTHOR );
	if ( text[0] ) {
		CG_DrawLoadingLine( text, 1, 0.25f, 447 );
		CG_DrawLoadingLine( CG_ConfigString( CS_MAP_AUTHOR2 ), 1, 0.25f, 462 );
	}

	CG_SetScreenPlacement( CG_PLACE_RIGHT );
	CG_DrawPic( 384, 0, 256, 64, cgs.media.loadingGametypeBackShader );
	gametype = CG_LoadingGametypeName( cgs.gametype );
	height = CG_Text_HeightInFont( gametype, 0, 0.8f, 0 );
	width = CG_Text_WidthInFont( gametype, 0, 0.8f, 0 );
	CG_DrawLoadingText( (float)( 620.0 - width ), (float)( height + 10 ), 0, 0.8f, gametype, ITEM_TEXTSTYLE_NORMAL );

	CG_SetScreenPlacement( CG_PLACE_CENTER );
	text = cg.infoScreenText[0] ? va( "Loading %s", cg.infoScreenText ) : "Awaiting Snapshot";
	width = CG_Text_WidthInFont( text, 1, 0.3f, 0 );
	CG_DrawLoadingText( (float)( 320 - width / 2 ), 370, 1, 0.3f, text, ITEM_TEXTSTYLE_SHADOWED );
	for ( i = 0; i < LOADING_PROGRESS_SQUARES; i++ ) {
		CG_FillRect( (float)( 292 + i * 16 ), 380, 8, 8, i < loadingStagesDone ? loadingDone : loadingShade );
	}
	CG_SetScreenPlacement( previous );
}
