// Copyright (C) 1999-2000 Id Software, Inc.
//
/*
=======================================================================

USER INTERFACE MAIN

=======================================================================
*/

// use this to get a demo build without an explicit demo build, i.e. to get the demo ui files to build
//#define PRE_RELEASE_TADEMO

#include "ui_local.h"
#include "../game/ui_swatches.h"
#include "../game/bg_mode.h"
#include "../game/bg_hostlabels.h"

extern displayContextDef_t *DC;

uiInfo_t uiInfo;

static const char *MonthAbbrev[] = {
	"Jan","Feb","Mar",
	"Apr","May","Jun",
	"Jul","Aug","Sep",
	"Oct","Nov","Dec"
};


static const char *skillLevels[] = {
  "I Can Win",
  "Bring It On",
  "Hurt Me Plenty",
  "Hardcore",
  "Nightmare"
};

static const int numSkillLevels = sizeof(skillLevels) / sizeof(const char*);


// Browser sources are slot-driven: Local, Trinity (sv_master2), Internet
// (all masters), one source per remaining non-empty sv_master slot, then
// Favorites. Empty slots produce no source.
#define MAX_MASTER_SLOTS		5
#define TRINITY_MASTER_SLOT		2
#define MAX_NETSOURCES			(3 + MAX_MASTER_SLOTS - 1 + 1)
#define MAX_SOURCESERVERS		512
#define MAX_SOURCELABEL			23

// a master query is complete once the count stops growing for this long (ms)
#define MASTER_QUIET_TIME		2500

#define SRC_LOCAL				0
#define SRC_TRINITY				1
#define SRC_INTERNET			2
#define SRC_MASTER				3
#define SRC_FAVORITES			4

typedef struct {
	int		type;			// SRC_*
	int		masterSlot;		// globalservers argument: 0 = all masters
	char	label[MAX_SOURCELABEL+1];
} uiNetSource_t;

// one browser row, snapshotted out of the engine's transfer buckets while
// its tab is the one being refreshed
typedef struct {
	char	adrstr[64];
	char	hostname[36];
	char	mapname[36];
	char	game[36];
	int		ping;
	int		clients;
	int		maxClients;
	int		humanClients;
	int		gameType;
	int		netType;
	int		mode;		// Trinity g_mode (MODE_*), or -1
} uiServerRow_t;

static uiNetSource_t	uiNetSources[MAX_NETSOURCES];
static int				uiNumNetSources;

// per-source snapshot caches: tab switches render these, never live trap
// reads; only a refresh of a tab rewrites its own array
static uiServerRow_t	uiSourceServers[MAX_NETSOURCES][MAX_SOURCESERVERS];
static int				uiNumSourceServers[MAX_NETSOURCES];

// which engine bucket answers a "globalservers 2" Trinity query: quake3e
// routes slot 2 to AS_MPLAYER, trinity-vr retired that bucket and fills
// AS_GLOBAL; -1 while the current refresh is still probing
static int				uiTrinityBucket = -1;
static int				uiTrinityProbeGlobal;

static const serverFilter_t serverFilters[] = {
	{"All", "" },
	{"Quake 3 Arena", "" },
	{"Team Arena", "missionpack" },
	{"Rocket Arena", "arena" },
	{"Alliance", "alliance20" },
	{"WFA", "wfa" },
	{"OSP", "osp" },
};

static const char *teamArenaGameTypes[] = {
	"FFA",
	"TOURNAMENT",
	"SP",
	"TEAM DM",
	"CTF",
	"1FCTF",
	"OVERLOAD",
	"HARVESTER",
	"TEAMTOURNAMENT"
};

static int const numTeamArenaGameTypes = sizeof(teamArenaGameTypes) / sizeof(const char*);


static const char *teamArenaGameNames[] = {
	"Free For All",
	"Tournament",
	"Single Player",
	"Team Deathmatch",
	"Capture the Flag",
	"One Flag CTF",
	"Overload",
	"Harvester",
	"Team Tournament",
};

static int const numTeamArenaGameNames = sizeof(teamArenaGameNames) / sizeof(const char*);


static const int numServerFilters = sizeof(serverFilters) / sizeof(serverFilter_t);

static const char *sortKeys[] = {
	"Server Name",
	"Map Name",
	"Open Player Spots",
	"Game Type",
	"Ping Time"
};
static const int numSortKeys = sizeof(sortKeys) / sizeof(const char*);

static qhandle_t uiModeIcons[MODE_COUNT];	// Trinity mode profile icons, indexed by MODE_*

static char* netnames[] = {
	"???",
	"UDP",
	"UDP6"
};

#ifndef MISSIONPACK // bk001206
static char quake3worldMessage[] = "Visit www.quake3world.com - News, Community, Events, Files";
#endif

static void UI_StartServerRefresh(qboolean full);
static void UI_DrawTrinitySigil( rectDef_t *rect );
static void UI_StopServerRefresh( void );
static void UI_FavoriteAddressAdd( const char *addr );
static void UI_FavoriteAddressRemove( const char *addr );
static void UI_DoServerRefresh( void );
static void UI_FeederSelection(float feederID, int index);
static void UI_BuildServerDisplayList(qboolean force);
static void UI_BuildServerStatus(qboolean force);
static void UI_BuildFindPlayerList(qboolean force);
static int QDECL UI_ServersQsortCompare( const void *arg1, const void *arg2 );
static int UI_MapCountByGameType(qboolean singlePlayer);
static int UI_HeadCountByTeam( void );
static void UI_ParseGameInfo(const char *teamFile);
static void UI_ParseTeamInfo(const char *teamFile);
static const char *UI_SelectedMap(int index, int *actual);
static const char *UI_SelectedHead(int index, int *actual);
static int UI_GetIndexFromSelection(int actual);

int ProcessNewUI( int command, int arg0, int arg1, int arg2, int arg3, int arg4, int arg5, int arg6 );

/*
================
vmMain

This is the only way control passes into the module.
This must be the very first function compiled into the .qvm file
================
*/
vmCvar_t  ui_new;
vmCvar_t  ui_debug;
vmCvar_t  ui_initialized;
vmCvar_t  ui_teamArenaFirstRun;
vmCvar_t  ui_serverFilterType;

void _UI_Init( qboolean );
void _UI_Shutdown( void );
void _UI_KeyEvent( int key, qboolean down );
void _UI_MouseEvent( int dx, int dy );
void _UI_Refresh( int realtime );
qboolean _UI_IsFullscreen( void );
void UI_VideoCheck( int time );
static void UI_CIN_SetExtents( int handle, int x, int y, int w, int h );
DLLEXPORT intptr_t vmMain( int command, int arg0, int arg1, int arg2, int arg3, int arg4, int arg5, int arg6, int arg7, int arg8, int arg9, int arg10, int arg11  ) {
  switch ( command ) {
	  case UI_GETAPIVERSION:
		  return UI_API_VERSION;

	  case UI_INIT:
		  _UI_Init(arg0);
		  return 0;

	  case UI_SHUTDOWN:
		  UI_VR_Shutdown();
		  _UI_Shutdown();
		  return 0;

	  case UI_KEY_EVENT:
		  _UI_KeyEvent( arg0, arg1 );
		  return 0;

	  case UI_MOUSE_EVENT:
		  _UI_MouseEvent( arg0, arg1 );
		  return 0;

	  case UI_REFRESH:
		  _UI_Refresh( arg0 );
		  return 0;

	  case UI_IS_FULLSCREEN:
		  return _UI_IsFullscreen();

	  case UI_SET_ACTIVE_MENU:
		  _UI_SetActiveMenu( arg0 );
		  return 0;

	  case UI_CONSOLE_COMMAND:
		  return UI_ConsoleCommand(arg0);

	  case UI_DRAW_CONNECT_SCREEN:
		  UI_DrawConnectScreen( arg0 );
		  return 0;
	  case UI_HASUNIQUECDKEY: // mod authors need to observe this
	    return qtrue; // bk010117 - change this to qfalse for mods!

	}

	return -1;
}


/*
=================
UI_ActiveNetSource

ui_netSource clamped to the built source list.
=================
*/
static int UI_ActiveNetSource( void ) {
	if ( ui_netSource.integer < 0 || ui_netSource.integer >= uiNumNetSources ) {
		ui_netSource.integer = 0;
	}
	return ui_netSource.integer;
}

/*
=================
UI_NetSourceType

SRC_* kind of the active source.
=================
*/
static int UI_NetSourceType( void ) {
	return uiNetSources[UI_ActiveNetSource()].type;
}

/*
=================
UI_BuildBrowserSources

Local, Trinity, Internet, one source per remaining non-empty sv_master slot,
Favorites. Labels come from the shared hostname table; the draw code fits
them to the widget by pixel measurement.
=================
*/
static void UI_BuildBrowserSources( void ) {
	char			master[64];
	uiNetSource_t	*src;
	int				slot;

	uiNumNetSources = 0;

	src = &uiNetSources[uiNumNetSources++];
	src->type = SRC_LOCAL;
	src->masterSlot = 0;
	Q_strncpyz( src->label, "Local", sizeof( src->label ) );

	trap_Cvar_VariableStringBuffer( va( "sv_master%d", TRINITY_MASTER_SLOT ), master, sizeof( master ) );
	if ( master[0] ) {
		src = &uiNetSources[uiNumNetSources++];
		src->type = SRC_TRINITY;
		src->masterSlot = TRINITY_MASTER_SLOT;
		BG_ServerSourceName( master, src->label, sizeof( src->label ), MAX_SOURCELABEL );
	}

	src = &uiNetSources[uiNumNetSources++];
	src->type = SRC_INTERNET;
	src->masterSlot = 0;
	Q_strncpyz( src->label, "Internet", sizeof( src->label ) );

	for ( slot = 1; slot <= MAX_MASTER_SLOTS; slot++ ) {
		if ( slot == TRINITY_MASTER_SLOT ) {
			continue;
		}
		trap_Cvar_VariableStringBuffer( va( "sv_master%d", slot ), master, sizeof( master ) );
		if ( !master[0] ) {
			continue;
		}
		src = &uiNetSources[uiNumNetSources++];
		src->type = SRC_MASTER;
		src->masterSlot = slot;
		BG_ServerSourceName( master, src->label, sizeof( src->label ), MAX_SOURCELABEL );
	}

	src = &uiNetSources[uiNumNetSources++];
	src->type = SRC_FAVORITES;
	src->masterSlot = 0;
	Q_strncpyz( src->label, "Favorites", sizeof( src->label ) );
}

/*
=================
UI_ProbeTrinityBucket

Engine-agnostic Trinity read: after "globalservers 2" is issued, quake3e
parks the reset/results in AS_MPLAYER while trinity-vr's retired-MPLAYER
client uses AS_GLOBAL. Any MPLAYER activity is authoritative (only slot-2
queries ever touch it); AS_GLOBAL is chosen once it visibly reacts to our
query. Until either happens the refresh keeps waiting.
=================
*/
static void UI_ProbeTrinityBucket( void ) {
	int		count;

	if ( uiTrinityBucket == AS_MPLAYER ) {
		return;
	}

	count = trap_LAN_GetServerCount( AS_MPLAYER );
	if ( count != 0 ) {
		uiTrinityBucket = AS_MPLAYER;
		return;
	}

	if ( uiTrinityBucket != -1 ) {
		return;
	}

	count = trap_LAN_GetServerCount( AS_GLOBAL );
	if ( count == -1 || count != uiTrinityProbeGlobal ) {
		uiTrinityBucket = AS_GLOBAL;
	}
}

/*
=================
UI_SourceLANBuckets

The engine bucket(s) the active source's live query fills. The Internet
source spans two buckets on quake3e, where the "globalservers 0" walk sends
slot 2 to AS_MPLAYER and everything else to AS_GLOBAL.
=================
*/
static int UI_SourceLANBuckets( int *buckets ) {
	switch ( UI_NetSourceType() ) {
	case SRC_LOCAL:
		buckets[0] = AS_LOCAL;
		return 1;
	case SRC_FAVORITES:
		buckets[0] = AS_FAVORITES;
		return 1;
	case SRC_TRINITY:
		buckets[0] = ( uiTrinityBucket == AS_MPLAYER ) ? AS_MPLAYER : AS_GLOBAL;
		return 1;
	case SRC_INTERNET:
		buckets[0] = AS_GLOBAL;
		buckets[1] = AS_MPLAYER;
		return 2;
	default:	// SRC_MASTER
		buckets[0] = AS_GLOBAL;
		return 1;
	}
}

/*
=================
UI_SourceLANCount

Combined result count for the active source, -1 while its query is pending.
=================
*/
static int UI_SourceLANCount( void ) {
	int		count;
	int		mcount;

	switch ( UI_NetSourceType() ) {
	case SRC_LOCAL:
		return trap_LAN_GetServerCount( AS_LOCAL );
	case SRC_FAVORITES:
		return trap_LAN_GetServerCount( AS_FAVORITES );
	case SRC_TRINITY:
		if ( uiTrinityBucket == -1 ) {
			return -1;
		}
		return trap_LAN_GetServerCount( uiTrinityBucket );
	case SRC_INTERNET:
		count = trap_LAN_GetServerCount( AS_GLOBAL );
		if ( count < 0 ) {
			return -1;
		}
		mcount = trap_LAN_GetServerCount( AS_MPLAYER );
		if ( mcount > 0 ) {
			count += mcount;
		}
		return count;
	default:	// SRC_MASTER
		return trap_LAN_GetServerCount( AS_GLOBAL );
	}
}

/*
=================
UI_SourceQueriesMasters

Whether the active source's refresh polls master servers, so lists can still
be arriving after the ping queue drains.
=================
*/
static qboolean UI_SourceQueriesMasters( void ) {
	int		type;

	type = UI_NetSourceType();
	if ( type == SRC_TRINITY || type == SRC_INTERNET || type == SRC_MASTER ) {
		return qtrue;
	}
	return qfalse;
}

/*
=================
UI_SourceRow

The snapshot row behind a display-list index of the active source.
=================
*/
static uiServerRow_t *UI_SourceRow( int displayIndex ) {
	int		tab;
	int		row;

	tab = UI_ActiveNetSource();
	if ( displayIndex < 0 || displayIndex >= uiInfo.serverStatus.numDisplayServers ) {
		return NULL;
	}
	row = uiInfo.serverStatus.displayServers[displayIndex];
	if ( row < 0 || row >= uiNumSourceServers[tab] ) {
		return NULL;
	}
	return &uiSourceServers[tab][row];
}



void AssetCache() {
	int n;
	//if (Assets.textFont == NULL) {
	//}
	//Assets.background = trap_R_RegisterShaderNoMip( ASSET_BACKGROUND );
	//Com_Printf("Menu Size: %i bytes\n", sizeof(Menus));
	uiInfo.uiDC.Assets.gradientBar = trap_R_RegisterShaderNoMip( ASSET_GRADIENTBAR );
	uiInfo.uiDC.Assets.fxBasePic = trap_R_RegisterShaderNoMip( ART_FX_BASE );
	uiInfo.uiDC.Assets.fxPic[0] = trap_R_RegisterShaderNoMip( ART_FX_RED );
	uiInfo.uiDC.Assets.fxPic[1] = trap_R_RegisterShaderNoMip( ART_FX_YELLOW );
	uiInfo.uiDC.Assets.fxPic[2] = trap_R_RegisterShaderNoMip( ART_FX_GREEN );
	uiInfo.uiDC.Assets.fxPic[3] = trap_R_RegisterShaderNoMip( ART_FX_TEAL );
	uiInfo.uiDC.Assets.fxPic[4] = trap_R_RegisterShaderNoMip( ART_FX_BLUE );
	uiInfo.uiDC.Assets.fxPic[5] = trap_R_RegisterShaderNoMip( ART_FX_CYAN );
	uiInfo.uiDC.Assets.fxPic[6] = trap_R_RegisterShaderNoMip( ART_FX_WHITE );
	uiInfo.uiDC.Assets.scrollBar = trap_R_RegisterShaderNoMip( ASSET_SCROLLBAR );
	uiInfo.uiDC.Assets.scrollBarArrowDown = trap_R_RegisterShaderNoMip( ASSET_SCROLLBAR_ARROWDOWN );
	uiInfo.uiDC.Assets.scrollBarArrowUp = trap_R_RegisterShaderNoMip( ASSET_SCROLLBAR_ARROWUP );
	uiInfo.uiDC.Assets.scrollBarArrowLeft = trap_R_RegisterShaderNoMip( ASSET_SCROLLBAR_ARROWLEFT );
	uiInfo.uiDC.Assets.scrollBarArrowRight = trap_R_RegisterShaderNoMip( ASSET_SCROLLBAR_ARROWRIGHT );
	uiInfo.uiDC.Assets.scrollBarThumb = trap_R_RegisterShaderNoMip( ASSET_SCROLL_THUMB );
	uiInfo.uiDC.Assets.sliderBar = trap_R_RegisterShaderNoMip( ASSET_SLIDER_BAR );
	uiInfo.uiDC.Assets.sliderThumb = trap_R_RegisterShaderNoMip( ASSET_SLIDER_THUMB );

	for( n = 0; n < NUM_CROSSHAIRS; n++ ) {
		uiInfo.uiDC.Assets.crosshairShader[n] = trap_R_RegisterShaderNoMip( va("gfx/2d/crosshair%c", 'a' + n ) );
	}

	uiModeIcons[MODE_VQ3] = trap_R_RegisterShaderNoMip( "gfx/2d/mode_vq3" );
	uiModeIcons[MODE_CPM] = trap_R_RegisterShaderNoMip( "gfx/2d/mode_cpm" );
	uiModeIcons[MODE_QL]  = trap_R_RegisterShaderNoMip( "gfx/2d/mode_ql" );
	uiModeIcons[MODE_QLT] = trap_R_RegisterShaderNoMip( "gfx/2d/mode_qlt" );

	uiInfo.newHighScoreSound = trap_S_RegisterSound("sound/feedback/voc_newhighscore.wav", qfalse);
}

void _UI_DrawSides(float x, float y, float w, float h, float size) {
	UI_AdjustFrom640( &x, &y, &w, &h );
	size *= uiInfo.uiDC.xscale;
	trap_R_DrawStretchPic( x, y, size, h, 0, 0, 0, 0, uiInfo.uiDC.whiteShader );
	trap_R_DrawStretchPic( x + w - size, y, size, h, 0, 0, 0, 0, uiInfo.uiDC.whiteShader );
}

void _UI_DrawTopBottom(float x, float y, float w, float h, float size) {
	UI_AdjustFrom640( &x, &y, &w, &h );
	size *= uiInfo.uiDC.yscale;
	trap_R_DrawStretchPic( x, y, w, size, 0, 0, 0, 0, uiInfo.uiDC.whiteShader );
	trap_R_DrawStretchPic( x, y + h - size, w, size, 0, 0, 0, 0, uiInfo.uiDC.whiteShader );
}
/*
================
UI_DrawRect

Coordinates are 640*480 virtual values
=================
*/
void _UI_DrawRect( float x, float y, float width, float height, float size, const float *color ) {
	trap_R_SetColor( color );

  _UI_DrawTopBottom(x, y, width, height, size);
  _UI_DrawSides(x, y, width, height, size);

	trap_R_SetColor( NULL );
}

int Text_Width(const char *text, float scale, int limit) {
  int count,len;
	float out;
	glyphInfo_t *glyph;
	float useScale;
	const char *s = text;
	fontInfo_t *font = &uiInfo.uiDC.Assets.textFont;
	if (scale <= ui_smallFont.value) {
		font = &uiInfo.uiDC.Assets.smallFont;
	} else if (scale >= ui_bigFont.value) {
		font = &uiInfo.uiDC.Assets.bigFont;
	}
	useScale = scale * font->glyphScale;
  out = 0;
  if (text) {
    len = strlen(text);
		if (limit > 0 && len > limit) {
			len = limit;
		}
		count = 0;
		while (s && *s && count < len) {
			if ( Q_IsColorString(s) ) {
				s += 2;
				continue;
			} else {
				glyph = &font->glyphs[(int)*s];
				out += glyph->xSkip;
				s++;
				count++;
			}
    }
  }
  return out * useScale;
}

int Text_Height(const char *text, float scale, int limit) {
  int len, count;
	float max;
	glyphInfo_t *glyph;
	float useScale;
	const char *s = text; // bk001206 - unsigned
	fontInfo_t *font = &uiInfo.uiDC.Assets.textFont;
	if (scale <= ui_smallFont.value) {
		font = &uiInfo.uiDC.Assets.smallFont;
	} else if (scale >= ui_bigFont.value) {
		font = &uiInfo.uiDC.Assets.bigFont;
	}
	useScale = scale * font->glyphScale;
  max = 0;
  if (text) {
    len = strlen(text);
		if (limit > 0 && len > limit) {
			len = limit;
		}
		count = 0;
		while (s && *s && count < len) {
			if ( Q_IsColorString(s) ) {
				s += 2;
				continue;
			} else {
				glyph = &font->glyphs[(int)*s]; // TTimo: FIXME: getting nasty warnings without the cast, hopefully this doesn't break the VM build
	      if (max < glyph->height) {
		      max = glyph->height;
			  }
				s++;
				count++;
			}
    }
  }
  return max * useScale;
}

void Text_PaintChar(float x, float y, float width, float height, float scale, float s, float t, float s2, float t2, qhandle_t hShader) {
  float w, h;
  w = width * scale;
  h = height * scale;
  UI_AdjustFrom640( &x, &y, &w, &h );
  trap_R_DrawStretchPic( x, y, w, h, s, t, s2, t2, hShader );
}

void Text_Paint(float x, float y, float scale, vec4_t color, const char *text, float adjust, int limit, int style) {
  int len, count;
	vec4_t newColor;
	glyphInfo_t *glyph;
	float useScale;
	fontInfo_t *font = &uiInfo.uiDC.Assets.textFont;
	if (scale <= ui_smallFont.value) {
		font = &uiInfo.uiDC.Assets.smallFont;
	} else if (scale >= ui_bigFont.value) {
		font = &uiInfo.uiDC.Assets.bigFont;
	}
	useScale = scale * font->glyphScale;
  if (text) {
    const char *s = text; // bk001206 - unsigned
		trap_R_SetColor( color );
		memcpy(&newColor[0], &color[0], sizeof(vec4_t));
    len = strlen(text);
		if (limit > 0 && len > limit) {
			len = limit;
		}
		count = 0;
		while (s && *s && count < len) {
			glyph = &font->glyphs[(int)*s]; // TTimo: FIXME: getting nasty warnings without the cast, hopefully this doesn't break the VM build
      //int yadj = Assets.textFont.glyphs[text[i]].bottom + Assets.textFont.glyphs[text[i]].top;
      //float yadj = scale * (Assets.textFont.glyphs[text[i]].imageHeight - Assets.textFont.glyphs[text[i]].height);
			if ( Q_IsColorString( s ) ) {
				memcpy( newColor, g_color_table[ColorIndex(*(s+1))], sizeof( newColor ) );
				newColor[3] = color[3];
				trap_R_SetColor( newColor );
				s += 2;
				continue;
			} else {
				float yadj = useScale * glyph->top;
				if (style == ITEM_TEXTSTYLE_SHADOWED || style == ITEM_TEXTSTYLE_SHADOWEDMORE) {
					int ofs = style == ITEM_TEXTSTYLE_SHADOWED ? 1 : 2;
					colorBlack[3] = newColor[3];
					trap_R_SetColor( colorBlack );
					Text_PaintChar(x + ofs, y - yadj + ofs, 
														glyph->imageWidth,
														glyph->imageHeight,
														useScale, 
														glyph->s,
														glyph->t,
														glyph->s2,
														glyph->t2,
														glyph->glyph);
					trap_R_SetColor( newColor );
					colorBlack[3] = 1.0;
				}
				Text_PaintChar(x, y - yadj, 
													glyph->imageWidth,
													glyph->imageHeight,
													useScale, 
													glyph->s,
													glyph->t,
													glyph->s2,
													glyph->t2,
													glyph->glyph);

				x += (glyph->xSkip * useScale) + adjust;
				s++;
				count++;
			}
    }
	  trap_R_SetColor( NULL );
  }
}

void Text_PaintWithCursor(float x, float y, float scale, vec4_t color, const char *text, int cursorPos, char cursor, int limit, int style) {
  int len, count;
	vec4_t newColor;
	glyphInfo_t *glyph, *glyph2;
	float yadj;
	float useScale;
	fontInfo_t *font = &uiInfo.uiDC.Assets.textFont;
	if (scale <= ui_smallFont.value) {
		font = &uiInfo.uiDC.Assets.smallFont;
	} else if (scale >= ui_bigFont.value) {
		font = &uiInfo.uiDC.Assets.bigFont;
	}
	useScale = scale * font->glyphScale;
  if (text) {
    const char *s = text; // bk001206 - unsigned
		trap_R_SetColor( color );
		memcpy(&newColor[0], &color[0], sizeof(vec4_t));
    len = strlen(text);
		if (limit > 0 && len > limit) {
			len = limit;
		}
		count = 0;
		glyph2 = &font->glyphs[ (int) cursor]; // bk001206 - possible signed char
		while (s && *s && count < len) {
			glyph = &font->glyphs[(int)*s]; // TTimo: FIXME: getting nasty warnings without the cast, hopefully this doesn't break the VM build
      //int yadj = Assets.textFont.glyphs[text[i]].bottom + Assets.textFont.glyphs[text[i]].top;
      //float yadj = scale * (Assets.textFont.glyphs[text[i]].imageHeight - Assets.textFont.glyphs[text[i]].height);
			if ( Q_IsColorString( s ) ) {
				memcpy( newColor, g_color_table[ColorIndex(*(s+1))], sizeof( newColor ) );
				newColor[3] = color[3];
				trap_R_SetColor( newColor );
				s += 2;
				continue;
			} else {
				yadj = useScale * glyph->top;
				if (style == ITEM_TEXTSTYLE_SHADOWED || style == ITEM_TEXTSTYLE_SHADOWEDMORE) {
					int ofs = style == ITEM_TEXTSTYLE_SHADOWED ? 1 : 2;
					colorBlack[3] = newColor[3];
					trap_R_SetColor( colorBlack );
					Text_PaintChar(x + ofs, y - yadj + ofs, 
														glyph->imageWidth,
														glyph->imageHeight,
														useScale, 
														glyph->s,
														glyph->t,
														glyph->s2,
														glyph->t2,
														glyph->glyph);
					colorBlack[3] = 1.0;
					trap_R_SetColor( newColor );
				}
				Text_PaintChar(x, y - yadj, 
													glyph->imageWidth,
													glyph->imageHeight,
													useScale, 
													glyph->s,
													glyph->t,
													glyph->s2,
													glyph->t2,
													glyph->glyph);

	      yadj = useScale * glyph2->top;
		    if (count == cursorPos && !((uiInfo.uiDC.realTime/BLINK_DIVISOR) & 1)) {
					Text_PaintChar(x, y - yadj, 
														glyph2->imageWidth,
														glyph2->imageHeight,
														useScale, 
														glyph2->s,
														glyph2->t,
														glyph2->s2,
														glyph2->t2,
														glyph2->glyph);
				}

				x += (glyph->xSkip * useScale);
				s++;
				count++;
			}
    }
    // need to paint cursor at end of text
    if (cursorPos == len && !((uiInfo.uiDC.realTime/BLINK_DIVISOR) & 1)) {
        yadj = useScale * glyph2->top;
        Text_PaintChar(x, y - yadj, 
                          glyph2->imageWidth,
                          glyph2->imageHeight,
                          useScale, 
                          glyph2->s,
                          glyph2->t,
                          glyph2->s2,
                          glyph2->t2,
                          glyph2->glyph);

    }

	  trap_R_SetColor( NULL );
  }
}

// Version of Text_PaintWithCursor that shows color codes literally (^1, ^2, etc)
// instead of interpreting them. Used when virtual keyboard is active.
void Text_PaintWithCursor_NoColorEscape(float x, float y, float scale, vec4_t color, const char *text, int cursorPos, char cursor, int limit, int style) {
  int len, count;
	glyphInfo_t *glyph, *glyph2;
	float yadj;
	float useScale;
	fontInfo_t *font = &uiInfo.uiDC.Assets.textFont;
	if (scale <= ui_smallFont.value) {
		font = &uiInfo.uiDC.Assets.smallFont;
	} else if (scale >= ui_bigFont.value) {
		font = &uiInfo.uiDC.Assets.bigFont;
	}
	useScale = scale * font->glyphScale;
  if (text) {
    const char *s = text;
		trap_R_SetColor( color );
    len = strlen(text);
		if (limit > 0 && len > limit) {
			len = limit;
		}
		count = 0;
		glyph2 = &font->glyphs[cursor & 255];
		while (s && *s && count < len) {
			glyph = &font->glyphs[*s & 255];
			// No color code interpretation: just draw each character
			yadj = useScale * glyph->top;
			if (style == ITEM_TEXTSTYLE_SHADOWED || style == ITEM_TEXTSTYLE_SHADOWEDMORE) {
				int ofs = style == ITEM_TEXTSTYLE_SHADOWED ? 1 : 2;
				colorBlack[3] = color[3];
				trap_R_SetColor( colorBlack );
				Text_PaintChar(x + ofs, y - yadj + ofs,
											glyph->imageWidth,
											glyph->imageHeight,
											useScale,
											glyph->s,
											glyph->t,
											glyph->s2,
											glyph->t2,
											glyph->glyph);
				colorBlack[3] = 1.0;
				trap_R_SetColor( color );
			}
			Text_PaintChar(x, y - yadj,
										glyph->imageWidth,
										glyph->imageHeight,
										useScale,
										glyph->s,
										glyph->t,
										glyph->s2,
										glyph->t2,
										glyph->glyph);

      yadj = useScale * glyph2->top;
	    if (count == cursorPos) {
				// Show solid cursor, no blinking when keyboard is active
				Text_PaintChar(x, y - yadj,
											glyph2->imageWidth,
											glyph2->imageHeight,
											useScale,
											glyph2->s,
											glyph2->t,
											glyph2->s2,
											glyph2->t2,
											glyph2->glyph);
			}

			x += (glyph->xSkip * useScale);
			s++;
			count++;
    }
    // need to paint cursor at end of text
    if (cursorPos == len) {
        yadj = useScale * glyph2->top;
        Text_PaintChar(x, y - yadj,
                          glyph2->imageWidth,
                          glyph2->imageHeight,
                          useScale,
                          glyph2->s,
                          glyph2->t,
                          glyph2->s2,
                          glyph2->t2,
                          glyph2->glyph);

    }

	  trap_R_SetColor( NULL );
  }
}


static void Text_Paint_Limit(float *maxX, float x, float y, float scale, vec4_t color, const char* text, float adjust, int limit) {
  int len, count;
	vec4_t newColor;
	glyphInfo_t *glyph;
  if (text) {
    const char *s = text; // bk001206 - unsigned
		float max = *maxX;
		float useScale;
		fontInfo_t *font = &uiInfo.uiDC.Assets.textFont;
		if (scale <= ui_smallFont.value) {
			font = &uiInfo.uiDC.Assets.smallFont;
		} else if (scale > ui_bigFont.value) {
			font = &uiInfo.uiDC.Assets.bigFont;
		}
		useScale = scale * font->glyphScale;
		trap_R_SetColor( color );
    len = strlen(text);					 
		if (limit > 0 && len > limit) {
			len = limit;
		}
		count = 0;
		while (s && *s && count < len) {
			glyph = &font->glyphs[(int)*s]; // TTimo: FIXME: getting nasty warnings without the cast, hopefully this doesn't break the VM build
			if ( Q_IsColorString( s ) ) {
				memcpy( newColor, g_color_table[ColorIndex(*(s+1))], sizeof( newColor ) );
				newColor[3] = color[3];
				trap_R_SetColor( newColor );
				s += 2;
				continue;
			} else {
	      float yadj = useScale * glyph->top;
				if (Text_Width(s, useScale, 1) + x > max) {
					*maxX = 0;
					break;
				}
		    Text_PaintChar(x, y - yadj, 
			                 glyph->imageWidth,
				               glyph->imageHeight,
				               useScale, 
						           glyph->s,
								       glyph->t,
								       glyph->s2,
									     glyph->t2,
										   glyph->glyph);
	      x += (glyph->xSkip * useScale) + adjust;
				*maxX = x;
				count++;
				s++;
	    }
		}
	  trap_R_SetColor( NULL );
  }

}


void UI_ShowPostGame(qboolean newHigh) {
	trap_Cvar_Set ("cg_cameraOrbit", "0");
	trap_Cvar_Set("cg_thirdPerson", "0");
	trap_Cvar_Set( "sv_killserver", "1" );
	uiInfo.soundHighScore = newHigh;
  _UI_SetActiveMenu(UIMENU_POSTGAME);
}
/*
=================
_UI_Refresh
=================
*/

void UI_DrawCenteredPic(qhandle_t image, int w, int h) {
  int x, y;
  x = (SCREEN_WIDTH - w) / 2;
  y = (SCREEN_HEIGHT - h) / 2;
  UI_DrawHandlePic(x, y, w, h, image);
}

int frameCount = 0;
int startTime;

static char      trinityVersion[64];
static char      trinityEngine[64];
static qhandle_t trinityWordmark;
static qhandle_t trinityFlareShader;
static sfxHandle_t trinityFlareSound;
static int       trinityFlareTime;
static qboolean  trinityVersionLoaded = qfalse;

/*
===============
UI_SigilFlare
===============
*/
static float UI_SigilFlare( int now ) {
	int dt = now - trinityFlareTime;

	if ( !trinityFlareTime || dt < 0 || dt >= 1000 ) {
		return 0;
	}
	return dt < 300 ? 1.0f : 1.0f - ( dt - 300 ) / 700.0f;
}

/*
===============
UI_StripVersionPrefix
===============
*/
static void UI_StripVersionPrefix( char *s, int size ) {
	if ( s[0] == 'v' || s[0] == 'V' ) {
		char buf[64];
		Q_strncpyz( buf, s + 1, sizeof( buf ) );
		Q_strncpyz( s, buf, size );
	}
}
static qhandle_t trinityModel;

static void UI_LoadTrinityVersion( void ) {
	trinityVersionLoaded = qtrue;
	Q_strncpyz( trinityVersion, TRINITY_VERSION, sizeof( trinityVersion ) );
	UI_StripVersionPrefix( trinityVersion, sizeof( trinityVersion ) );
	trinityWordmark = trap_R_RegisterShaderNoMip( "gfx/trinity/wordmark" );
	trinityFlareShader = trap_R_RegisterShaderNoMip( "gfx/trinity/flare" );
	trinityFlareSound = trap_S_RegisterSound( "sound/items/poweruprespawn.wav", qfalse );
	// "trinity-engine/vX.Y.Z" or similar on Trinity engines
	{
		char buf[64];
		const char *p;
		trap_Cvar_VariableStringBuffer( "com_engine", buf, sizeof( buf ) );
		p = strchr( buf, '/' );
		Q_strncpyz( trinityEngine, p ? p + 1 : buf, sizeof( trinityEngine ) );
		UI_StripVersionPrefix( trinityEngine, sizeof( trinityEngine ) );
	}

	trinityModel = trap_R_RegisterModel( "models/trinity/trinity.md3" );
}

#define	UI_FPS_FRAMES	4
void _UI_Refresh( int realtime )
{
	static int index;
	static int	previousTimes[UI_FPS_FRAMES];

	//if ( !( trap_Key_GetCatcher() & KEYCATCH_UI ) ) {
	//	return;
	//}

	UI_VR_CursorOverride( &uiInfo.uiDC.cursorx, &uiInfo.uiDC.cursory );

	uiInfo.uiDC.frameTime = realtime - uiInfo.uiDC.realTime;
	uiInfo.uiDC.realTime = realtime;

	previousTimes[index % UI_FPS_FRAMES] = uiInfo.uiDC.frameTime;
	index++;
	if ( index > UI_FPS_FRAMES ) {
		int i, total;
		// average multiple frames together to smooth changes out a bit
		total = 0;
		for ( i = 0 ; i < UI_FPS_FRAMES ; i++ ) {
			total += previousTimes[i];
		}
		if ( !total ) {
			total = 1;
		}
		uiInfo.uiDC.FPS = 1000 * UI_FPS_FRAMES / total;
	}



	UI_UpdateCvars();

	// Poll trinity login status: switch views on success
	{
		static char lastLoginStatus[32] = "";
		char loginStatus[32];
		trap_Cvar_VariableStringBuffer( "cl_trinityLoginStatus", loginStatus, sizeof(loginStatus) );
		if ( Q_stricmp( loginStatus, lastLoginStatus ) != 0 ) {
			Q_strncpyz( lastLoginStatus, loginStatus, sizeof(lastLoginStatus) );
			if ( !Q_stricmp( loginStatus, "success" ) ) {
				menuDef_t *loginMenu = Menus_FindByName("login_menu");
				if ( loginMenu && ( loginMenu->window.flags & WINDOW_VISIBLE ) ) {
					Menu_ShowItemByName(loginMenu, "grpLoggedIn", qtrue);
					Menu_ShowItemByName(loginMenu, "grpLoginForm", qfalse);
				}
			}
		}
	}

	if (Menu_Count() > 0) {
		// paint all the menus
		Menu_PaintAll();
		// refresh server browser list
		UI_DoServerRefresh();
		// refresh server status
		UI_BuildServerStatus(qfalse);
		// refresh find player list
		UI_BuildFindPlayerList(qfalse);
	}

	if ( !trinityVersionLoaded ) {
		UI_LoadTrinityVersion();
	}
	if ( trinityVersion[0] ) {
		menuDef_t *main = Menus_FindByName( "main" );
		if ( main && ( main->window.flags & WINDOW_VISIBLE ) ) {
			float scale = 0.15f;
			float cx = 593;
			vec4_t versionColor = { 1.0f, 1.0f, 1.0f, 0.8f };
			const char *line = trinityEngine[0] ? va( "%s / %s", trinityEngine, trinityVersion ) : trinityVersion;

			if ( trinityWordmark ) {
				UI_DrawHandlePic( cx - 45, 288, 90, 20, trinityWordmark );
			}
			Text_Paint( cx - Text_Width( line, scale, 0 ) / 2, 316, scale, versionColor, line, 0, 0, ITEM_TEXTSTYLE_SHADOWED );
		}
	}

	// draw cursor (hidden while the virtual keyboard draws its own cursors,
	// while thumbstick nav owns selection, or when the menu isn't catching keys)
	if ( !UI_VKeyboardIsActive() ) {
		UI_SetColor( NULL );
		if (Menu_Count() > 0 && (trap_Key_GetCatcher() & KEYCATCH_UI) && !UI_VR_HideCursor()) {
			UI_DrawHandlePic( uiInfo.uiDC.cursorx-16, uiInfo.uiDC.cursory-16, 32, 32, uiInfo.uiDC.Assets.cursor);
		}
	}

#ifndef NDEBUG
	if (uiInfo.uiDC.debug)
	{
		// cursor coordinates
		//FIXME
		//UI_DrawString( 0, 0, va("(%d,%d)",uis.cursorx,uis.cursory), UI_LEFT|UI_SMALLFONT, colorRed );
	}
#endif

}

/*
=================
_UI_Shutdown
=================
*/
void _UI_Shutdown( void ) {
	trap_LAN_SaveCachedServers();
}

char *defaultMenu = NULL;

char *GetMenuBuffer(const char *filename) {
	int	len;
	fileHandle_t	f;
	static char buf[MAX_MENUFILE];

	len = trap_FS_FOpenFile( filename, &f, FS_READ );
	if ( !f ) {
		trap_Print( va( S_COLOR_RED "menu file not found: %s, using default\n", filename ) );
		return defaultMenu;
	}
	if ( len >= MAX_MENUFILE ) {
		trap_Print( va( S_COLOR_RED "menu file too large: %s is %i, max allowed is %i", filename, len, MAX_MENUFILE ) );
		trap_FS_FCloseFile( f );
		return defaultMenu;
	}

	trap_FS_Read( buf, len, f );
	buf[len] = 0;
	trap_FS_FCloseFile( f );
	//COM_Compress(buf);
  return buf;

}

qboolean Asset_Parse(int handle) {
	pc_token_t token;
	const char *tempStr;

	if (!trap_PC_ReadToken(handle, &token))
		return qfalse;
	if (Q_stricmp(token.string, "{") != 0) {
		return qfalse;
	}
    
	while ( 1 ) {

		memset(&token, 0, sizeof(pc_token_t));

		if (!trap_PC_ReadToken(handle, &token))
			return qfalse;

		if (Q_stricmp(token.string, "}") == 0) {
			return qtrue;
		}

		// font
		if (Q_stricmp(token.string, "font") == 0) {
			int pointSize;
			if (!PC_String_Parse(handle, &tempStr) || !PC_Int_Parse(handle,&pointSize)) {
				return qfalse;
			}
			trap_R_RegisterFont(tempStr, pointSize, &uiInfo.uiDC.Assets.textFont);
			uiInfo.uiDC.Assets.fontRegistered = qtrue;
			continue;
		}

		if (Q_stricmp(token.string, "smallFont") == 0) {
			int pointSize;
			if (!PC_String_Parse(handle, &tempStr) || !PC_Int_Parse(handle,&pointSize)) {
				return qfalse;
			}
			trap_R_RegisterFont(tempStr, pointSize, &uiInfo.uiDC.Assets.smallFont);
			continue;
		}

		if (Q_stricmp(token.string, "bigFont") == 0) {
			int pointSize;
			if (!PC_String_Parse(handle, &tempStr) || !PC_Int_Parse(handle,&pointSize)) {
				return qfalse;
			}
			trap_R_RegisterFont(tempStr, pointSize, &uiInfo.uiDC.Assets.bigFont);
			continue;
		}


		// gradientbar
		if (Q_stricmp(token.string, "gradientbar") == 0) {
			if (!PC_String_Parse(handle, &tempStr)) {
				return qfalse;
			}
			uiInfo.uiDC.Assets.gradientBar = trap_R_RegisterShaderNoMip(tempStr);
			continue;
		}

		// enterMenuSound
		if (Q_stricmp(token.string, "menuEnterSound") == 0) {
			if (!PC_String_Parse(handle, &tempStr)) {
				return qfalse;
			}
			uiInfo.uiDC.Assets.menuEnterSound = trap_S_RegisterSound( tempStr, qfalse );
			continue;
		}

		// exitMenuSound
		if (Q_stricmp(token.string, "menuExitSound") == 0) {
			if (!PC_String_Parse(handle, &tempStr)) {
				return qfalse;
			}
			uiInfo.uiDC.Assets.menuExitSound = trap_S_RegisterSound( tempStr, qfalse );
			continue;
		}

		// itemFocusSound
		if (Q_stricmp(token.string, "itemFocusSound") == 0) {
			if (!PC_String_Parse(handle, &tempStr)) {
				return qfalse;
			}
			uiInfo.uiDC.Assets.itemFocusSound = trap_S_RegisterSound( tempStr, qfalse );
			continue;
		}

		// menuBuzzSound
		if (Q_stricmp(token.string, "menuBuzzSound") == 0) {
			if (!PC_String_Parse(handle, &tempStr)) {
				return qfalse;
			}
			uiInfo.uiDC.Assets.menuBuzzSound = trap_S_RegisterSound( tempStr, qfalse );
			continue;
		}

		if (Q_stricmp(token.string, "cursor") == 0) {
			if (!PC_String_Parse(handle, &uiInfo.uiDC.Assets.cursorStr)) {
				return qfalse;
			}
			uiInfo.uiDC.Assets.cursor = trap_R_RegisterShaderNoMip( uiInfo.uiDC.Assets.cursorStr);
			continue;
		}

		if (Q_stricmp(token.string, "fadeClamp") == 0) {
			if (!PC_Float_Parse(handle, &uiInfo.uiDC.Assets.fadeClamp)) {
				return qfalse;
			}
			continue;
		}

		if (Q_stricmp(token.string, "fadeCycle") == 0) {
			if (!PC_Int_Parse(handle, &uiInfo.uiDC.Assets.fadeCycle)) {
				return qfalse;
			}
			continue;
		}

		if (Q_stricmp(token.string, "fadeAmount") == 0) {
			if (!PC_Float_Parse(handle, &uiInfo.uiDC.Assets.fadeAmount)) {
				return qfalse;
			}
			continue;
		}

		if (Q_stricmp(token.string, "shadowX") == 0) {
			if (!PC_Float_Parse(handle, &uiInfo.uiDC.Assets.shadowX)) {
				return qfalse;
			}
			continue;
		}

		if (Q_stricmp(token.string, "shadowY") == 0) {
			if (!PC_Float_Parse(handle, &uiInfo.uiDC.Assets.shadowY)) {
				return qfalse;
			}
			continue;
		}

		if (Q_stricmp(token.string, "shadowColor") == 0) {
			if (!PC_Color_Parse(handle, &uiInfo.uiDC.Assets.shadowColor)) {
				return qfalse;
			}
			uiInfo.uiDC.Assets.shadowFadeClamp = uiInfo.uiDC.Assets.shadowColor[3];
			continue;
		}

	}
	return qfalse;
}

void Font_Report() {
  int i;
  Com_Printf("Font Info\n");
  Com_Printf("=========\n");
  for ( i = 32; i < 96; i++) {
    Com_Printf("Glyph handle %i: %i\n", i, uiInfo.uiDC.Assets.textFont.glyphs[i].glyph);
  }
}

void UI_Report() {
  String_Report();
  //Font_Report();

}

void UI_ParseMenu(const char *menuFile) {
	int handle;
	pc_token_t token;

	Com_Printf("Parsing menu file:%s\n", menuFile);

	handle = trap_PC_LoadSource(menuFile);
	if (!handle) {
		return;
	}

	while ( 1 ) {
		memset(&token, 0, sizeof(pc_token_t));
		if (!trap_PC_ReadToken( handle, &token )) {
			break;
		}

		//if ( Q_stricmp( token, "{" ) ) {
		//	Com_Printf( "Missing { in menu file\n" );
		//	break;
		//}

		//if ( menuCount == MAX_MENUS ) {
		//	Com_Printf( "Too many menus!\n" );
		//	break;
		//}

		if ( token.string[0] == '}' ) {
			break;
		}

		if (Q_stricmp(token.string, "assetGlobalDef") == 0) {
			if (Asset_Parse(handle)) {
				continue;
			} else {
				break;
			}
		}

		if (Q_stricmp(token.string, "menudef") == 0) {
			// start a new menu
			Menu_New(handle);
		}
	}
	trap_PC_FreeSource(handle);
}

qboolean Load_Menu(int handle) {
	pc_token_t token;

	if (!trap_PC_ReadToken(handle, &token))
		return qfalse;
	if (token.string[0] != '{') {
		return qfalse;
	}

	while ( 1 ) {

		if (!trap_PC_ReadToken(handle, &token))
			return qfalse;
    
		if ( token.string[0] == 0 ) {
			return qfalse;
		}

		if ( token.string[0] == '}' ) {
			return qtrue;
		}

		UI_ParseMenu(token.string); 
	}
	return qfalse;
}

void UI_LoadMenus(const char *menuFile, qboolean reset) {
	pc_token_t token;
	int handle;
	int start;

	start = trap_Milliseconds();

	handle = trap_PC_LoadSource( menuFile );
	if (!handle) {
		trap_Error( va( S_COLOR_YELLOW "menu file not found: %s, using default\n", menuFile ) );
		handle = trap_PC_LoadSource( "ui/menus.txt" );
		if (!handle) {
			trap_Error( va( S_COLOR_RED "default menu file not found: ui/menus.txt, unable to continue!\n", menuFile ) );
		}
	}

	ui_new.integer = 1;

	if (reset) {
		Menu_Reset();
	}

	while ( 1 ) {
		if (!trap_PC_ReadToken(handle, &token))
			break;
		if( token.string[0] == 0 || token.string[0] == '}') {
			break;
		}

		if ( token.string[0] == '}' ) {
			break;
		}

		if (Q_stricmp(token.string, "loadmenu") == 0) {
			if (Load_Menu(handle)) {
				continue;
			} else {
				break;
			}
		}
	}

	Com_Printf("UI menu load time = %d milli seconds\n", trap_Milliseconds() - start);

	trap_PC_FreeSource( handle );
}

void UI_Load() {
	char lastName[1024];
  menuDef_t *menu = Menu_GetFocused();
	char *menuSet = UI_Cvar_VariableString("ui_menuFiles");
	if (menu && menu->window.name) {
		strcpy(lastName, menu->window.name);
	}
	if (menuSet == NULL || menuSet[0] == '\0') {
		menuSet = "ui/menus.txt";
	}

	String_Init();

#ifdef PRE_RELEASE_TADEMO
	UI_ParseGameInfo("demogameinfo.txt");
#else
	UI_ParseGameInfo("gameinfo.txt");
	UI_LoadArenas();
#endif

	UI_LoadMenus(menuSet, qtrue);
	UI_VR_LoadMenus();
	Menus_CloseAll();
	Menus_ActivateByName(lastName);

}

static const char *handicapValues[] = {"None","95","90","85","80","75","70","65","60","55","50","45","40","35","30","25","20","15","10","5",NULL};
#ifndef MISSIONPACK // bk001206
static int numHandicaps = sizeof(handicapValues) / sizeof(const char*);
#endif

static void UI_DrawHandicap(rectDef_t *rect, float scale, vec4_t color, int textStyle) {
  int i, h;

  h = Com_Clamp( 5, 100, trap_Cvar_VariableValue("handicap") );
  i = 20 - h / 5;

  Text_Paint(rect->x, rect->y, scale, color, handicapValues[i], 0, 0, textStyle);
}

static void UI_DrawClanName(rectDef_t *rect, float scale, vec4_t color, int textStyle) {
  Text_Paint(rect->x, rect->y, scale, color, UI_Cvar_VariableString("ui_teamName"), 0, 0, textStyle);
}


static void UI_SetCapFragLimits(qboolean uiVars) {
	int cap = 5;
	int frag = 10;
	if (uiInfo.gameTypes[ui_gameType.integer].gtEnum == GT_OBELISK) {
		cap = 4;
	} else if (uiInfo.gameTypes[ui_gameType.integer].gtEnum == GT_HARVESTER) {
		cap = 15;
	}
	if (uiVars) {
		trap_Cvar_Set("ui_captureLimit", va("%d", cap));
		trap_Cvar_Set("ui_fragLimit", va("%d", frag));
	} else {
		trap_Cvar_Set("capturelimit", va("%d", cap));
		trap_Cvar_Set("fraglimit", va("%d", frag));
	}
}
// ui_gameType assumes gametype 0 is -1 ALL and will not show
static void UI_DrawGameType(rectDef_t *rect, float scale, vec4_t color, int textStyle) {
  Text_Paint(rect->x, rect->y, scale, color, uiInfo.gameTypes[ui_gameType.integer].gameType, 0, 0, textStyle);
}

static void UI_DrawNetGameType(rectDef_t *rect, float scale, vec4_t color, int textStyle) {
	if (ui_netGameType.integer < 0 || ui_netGameType.integer > uiInfo.numGameTypes) {
		trap_Cvar_Set("ui_netGameType", "0");
		trap_Cvar_Set("ui_actualNetGameType", "0");
	}
  Text_Paint(rect->x, rect->y, scale, color, uiInfo.gameTypes[ui_netGameType.integer].gameType , 0, 0, textStyle);
}

static void UI_DrawJoinGameType(rectDef_t *rect, float scale, vec4_t color, int textStyle) {
	if (ui_joinGameType.integer < 0 || ui_joinGameType.integer > uiInfo.numJoinGameTypes) {
		trap_Cvar_Set("ui_joinGameType", "0");
	}
  Text_Paint(rect->x, rect->y, scale, color, uiInfo.joinGameTypes[ui_joinGameType.integer].gameType , 0, 0, textStyle);
}



static int UI_TeamIndexFromName(const char *name) {
  int i;

  if (name && *name) {
    for (i = 0; i < uiInfo.teamCount; i++) {
      if (Q_stricmp(name, uiInfo.teamList[i].teamName) == 0) {
        return i;
      }
    }
  } 

  return 0;

}

static void UI_DrawClanLogo(rectDef_t *rect, float scale, vec4_t color) {
  int i;
  i = UI_TeamIndexFromName(UI_Cvar_VariableString("ui_teamName"));
  if (i >= 0 && i < uiInfo.teamCount) {
  	trap_R_SetColor( color );

		if (uiInfo.teamList[i].teamIcon == -1) {
      uiInfo.teamList[i].teamIcon = trap_R_RegisterShaderNoMip(uiInfo.teamList[i].imageName);
      uiInfo.teamList[i].teamIcon_Metal = trap_R_RegisterShaderNoMip(va("%s_metal",uiInfo.teamList[i].imageName));
      uiInfo.teamList[i].teamIcon_Name = trap_R_RegisterShaderNoMip(va("%s_name", uiInfo.teamList[i].imageName));
		}

  	UI_DrawHandlePic( rect->x, rect->y, rect->w, rect->h, uiInfo.teamList[i].teamIcon);
    trap_R_SetColor(NULL);
  }
}

static void UI_DrawClanCinematic(rectDef_t *rect, float scale, vec4_t color) {
  int i;
  i = UI_TeamIndexFromName(UI_Cvar_VariableString("ui_teamName"));
  if (i >= 0 && i < uiInfo.teamCount) {

		if (uiInfo.teamList[i].cinematic >= -2) {
			if (uiInfo.teamList[i].cinematic == -1) {
				uiInfo.teamList[i].cinematic = trap_CIN_PlayCinematic(va("%s.roq", uiInfo.teamList[i].imageName), 0, 0, 0, 0, (CIN_loop | CIN_silent) );
			}
			if (uiInfo.teamList[i].cinematic >= 0) {
			  trap_CIN_RunCinematic(uiInfo.teamList[i].cinematic);
				UI_CIN_SetExtents(uiInfo.teamList[i].cinematic, rect->x, rect->y, rect->w, rect->h);
	 			trap_CIN_DrawCinematic(uiInfo.teamList[i].cinematic);
			} else {
			  	trap_R_SetColor( color );
				UI_DrawHandlePic( rect->x, rect->y, rect->w, rect->h, uiInfo.teamList[i].teamIcon_Metal);
				trap_R_SetColor(NULL);
				uiInfo.teamList[i].cinematic = -2;
			}
		} else {
	  	trap_R_SetColor( color );
			UI_DrawHandlePic( rect->x, rect->y, rect->w, rect->h, uiInfo.teamList[i].teamIcon);
			trap_R_SetColor(NULL);
		}
	}

}

static void UI_DrawPreviewCinematic(rectDef_t *rect, float scale, vec4_t color) {
	if (uiInfo.previewMovie > -2) {
		uiInfo.previewMovie = trap_CIN_PlayCinematic(va("%s.roq", uiInfo.movieList[uiInfo.movieIndex]), 0, 0, 0, 0, (CIN_loop | CIN_silent) );
		if (uiInfo.previewMovie >= 0) {
		  trap_CIN_RunCinematic(uiInfo.previewMovie);
			UI_CIN_SetExtents(uiInfo.previewMovie, rect->x, rect->y, rect->w, rect->h);
 			trap_CIN_DrawCinematic(uiInfo.previewMovie);
		} else {
			uiInfo.previewMovie = -2;
		}
	} 

}



static void UI_DrawSkill(rectDef_t *rect, float scale, vec4_t color, int textStyle) {
  int i;
	i = trap_Cvar_VariableValue( "g_spSkill" );
  if (i < 1 || i > numSkillLevels) {
    i = 1;
  }
  Text_Paint(rect->x, rect->y, scale, color, skillLevels[i-1],0, 0, textStyle);
}


static void UI_DrawTeamName(rectDef_t *rect, float scale, vec4_t color, qboolean blue, int textStyle) {
  int i;
  i = UI_TeamIndexFromName(UI_Cvar_VariableString((blue) ? "ui_blueTeam" : "ui_redTeam"));
  if (i >= 0 && i < uiInfo.teamCount) {
    Text_Paint(rect->x, rect->y, scale, color, va("%s: %s", (blue) ? "Blue" : "Red", uiInfo.teamList[i].teamName),0, 0, textStyle);
  }
}

/*
===============
UI_ClanForTeamSlot

Strict teaminfo lookup for the createserver bot slots: the team's parsed
clan index, or -1 for a custom team name (UI_TeamIndexFromName can't
signal a miss). Slots fall back to the generic character list on -1.
===============
*/
static int UI_ClanForTeamSlot(qboolean blue) {
	const char *name = UI_Cvar_VariableString(blue ? "ui_blueTeam" : "ui_redTeam");
	int i;

	for (i = 0; i < uiInfo.teamCount; i++) {
		if (Q_stricmp(name, uiInfo.teamList[i].teamName) == 0) {
			return i;
		}
	}
	return -1;
}

static void UI_DrawTeamMember(rectDef_t *rect, float scale, vec4_t color, qboolean blue, int num, int textStyle) {
	// 0 - None
	// 1 - Human
	// 2..NumCharacters - Bot
	int value = trap_Cvar_VariableValue(va(blue ? "ui_blueteam%i" : "ui_redteam%i", num));
	int t;
	const char *text;
	if (value <= 0) {
		text = "Closed";
	} else if (value == 1) {
		text = "Human";
	} else {
		value -= 2;

		if (ui_actualNetGameType.integer >= GT_TEAM) {
			t = UI_ClanForTeamSlot(blue);
			if (t >= 0) {
				if (value >= TEAM_MEMBERS) {
					value = 0;
				}
				text = uiInfo.teamList[t].teamMembers[value];
			} else {
				if (value >= uiInfo.characterCount) {
					value = 0;
				}
				text = uiInfo.characterList[value].name;
			}
		} else {
			if (value >= UI_GetNumBots()) {
				value = 0;
			}
			text = UI_GetBotNameByNumber(value);
		}
	}
  Text_Paint(rect->x, rect->y, scale, color, text, 0, 0, textStyle);
}

static void UI_DrawEffects(rectDef_t *rect, float scale, vec4_t color) {
	int markerX = rect->x + (uiInfo.effectsColor * 112 / 6);

	UI_DrawHandlePic( rect->x, rect->y - 14, 128, 8, uiInfo.uiDC.Assets.fxBasePic );
	UI_DrawHandlePic( markerX, rect->y - 16, 16, 12, uiInfo.uiDC.Assets.fxPic[uiInfo.effectsColor] );
}

static void UI_DrawTrinitySigil( rectDef_t *rect ) {
	refdef_t refdef;
	refEntity_t ent;
	vec3_t mins, maxs, origin;
	float x, y, w, h, len;
	float desFovX = 30.0f;
	float desFovY = 30.0f;

	if ( !trinityVersionLoaded ) {
		UI_LoadTrinityVersion();
	}
	if ( !trinityModel ) {
		return;
	}

	x = rect->x;
	y = rect->y;
	w = rect->w;
	h = rect->h;

	memset( &refdef, 0, sizeof( refdef ) );
	refdef.rdflags = RDF_NOWORLDMODEL;
	AxisClear( refdef.viewaxis );

	// Pre-widen the fov under VR so the Vulkan renderer's 4:3 cropFactor
	// rescale of NOWORLDMODEL scenes restores the intended aspect. Origin
	// math stays on the DESIRED fov. Flatscreen keeps the desired fov.
	UI_VR_CompensateModelFov( &refdef, desFovX, desFovY );

	UI_AdjustFrom640( &x, &y, &w, &h );
	refdef.x = x;
	refdef.y = y;
	refdef.width = w;
	refdef.height = h;

	trap_R_ModelBounds( trinityModel, mins, maxs );
	len = 0.5 * ( maxs[2] - mins[2] );
	origin[0] = len / tan( DEG2RAD( desFovX ) * 0.5 );
	// shift model to the rect's right edge
	origin[1] = 0.5 * ( mins[1] + maxs[1] ) - origin[0] * tan( DEG2RAD( desFovX ) * 0.5 ) * 0.25;
	origin[2] = -0.5 * ( mins[2] + maxs[2] );

	refdef.time = uiInfo.uiDC.realTime;

	trap_R_ClearScene();

	{
		vec3_t angles, lightPos;

		lightPos[0] = origin[0] - 140.0f;
		lightPos[1] = origin[1] + 40.0f;
		lightPos[2] = origin[2] + 80.0f;
		trap_R_AddLightToScene( lightPos, 420.0f, 0.70f, 0.80f, 1.0f );

		memset( &ent, 0, sizeof( ent ) );
		{
			float t = uiInfo.uiDC.realTime * 0.001f;
			float flare = UI_SigilFlare( uiInfo.uiDC.realTime );
			float shake = 5.0f * flare;
			VectorSet( angles, 3.0f * sin( t * 0.31f ) + shake * sin( t * 88.0f ),
				12.0f * sin( t * 0.45f ) + 5.0f * sin( t * 0.17f ) + shake * sin( t * 95.0f + 1.0f ),
				3.0f * sin( t * 0.23f ) + shake * sin( t * 77.0f + 2.0f ) );
		}
		AnglesToAxis( angles, ent.axis );
		ent.hModel = trinityModel;
		VectorCopy( origin, ent.origin );
		VectorCopy( origin, ent.lightingOrigin );
		ent.renderfx = RF_LIGHTING_ORIGIN | RF_NOSHADOW;
		VectorCopy( ent.origin, ent.oldorigin );
		trap_R_AddRefEntityToScene( &ent );

		if ( trinityFlareShader && UI_SigilFlare( uiInfo.uiDC.realTime ) > 0 ) {
			byte level = (byte)( 255 * UI_SigilFlare( uiInfo.uiDC.realTime ) );
			ent.customShader = trinityFlareShader;
			ent.shaderRGBA.rgba[0] = ent.shaderRGBA.rgba[1] = ent.shaderRGBA.rgba[2] = level;
			ent.shaderRGBA.rgba[3] = 255;
			trap_R_AddRefEntityToScene( &ent );
		}
	}
	trap_R_RenderScene( &refdef );
}

static void UI_DrawMapPreview(rectDef_t *rect, float scale, vec4_t color, qboolean net) {
	int map = (net) ? ui_currentNetMap.integer : ui_currentMap.integer;
	if (map < 0 || map > uiInfo.mapCount) {
		if (net) {
			ui_currentNetMap.integer = 0;
			trap_Cvar_Set("ui_currentNetMap", "0");
		} else {
			ui_currentMap.integer = 0;
			trap_Cvar_Set("ui_currentMap", "0");
		}
		map = 0;
	}

	if (uiInfo.mapList[map].levelShot == -1) {
		uiInfo.mapList[map].levelShot = trap_R_RegisterShaderNoMip(uiInfo.mapList[map].imageName);
	}

	if (uiInfo.mapList[map].levelShot > 0) {
		UI_DrawHandlePic( rect->x, rect->y, rect->w, rect->h, uiInfo.mapList[map].levelShot);
	} else {
		UI_DrawHandlePic( rect->x, rect->y, rect->w, rect->h, trap_R_RegisterShaderNoMip("menu/art/unknownmap"));
	}
}						 


static void UI_DrawMapTimeToBeat(rectDef_t *rect, float scale, vec4_t color, int textStyle) {
	int minutes, seconds, time;
	if (ui_currentMap.integer < 0 || ui_currentMap.integer > uiInfo.mapCount) {
		ui_currentMap.integer = 0;
		trap_Cvar_Set("ui_currentMap", "0");
	}

	time = uiInfo.mapList[ui_currentMap.integer].timeToBeat[uiInfo.gameTypes[ui_gameType.integer].gtEnum];

	minutes = time / 60;
	seconds = time % 60;

  Text_Paint(rect->x, rect->y, scale, color, va("%02i:%02i", minutes, seconds), 0, 0, textStyle);
}



static void UI_DrawMapCinematic(rectDef_t *rect, float scale, vec4_t color, qboolean net) {

	int map = (net) ? ui_currentNetMap.integer : ui_currentMap.integer;
	if (map < 0 || map >= uiInfo.mapCount) {
		if (net) {
			ui_currentNetMap.integer = 0;
			trap_Cvar_Set("ui_currentNetMap", "0");
		} else {
			ui_currentMap.integer = 0;
			trap_Cvar_Set("ui_currentMap", "0");
		}
		map = 0;
	}

	if (uiInfo.mapList[map].cinematic >= -1) {
		if (uiInfo.mapList[map].cinematic == -1) {
			uiInfo.mapList[map].cinematic = trap_CIN_PlayCinematic(va("%s.roq", uiInfo.mapList[map].mapLoadName), 0, 0, 0, 0, (CIN_loop | CIN_silent) );
		}
		if (uiInfo.mapList[map].cinematic >= 0) {
		  trap_CIN_RunCinematic(uiInfo.mapList[map].cinematic);
		  UI_CIN_SetExtents(uiInfo.mapList[map].cinematic, rect->x, rect->y, rect->w, rect->h);
 			trap_CIN_DrawCinematic(uiInfo.mapList[map].cinematic);
		} else {
			uiInfo.mapList[map].cinematic = -2;
		}
	} else {
		UI_DrawMapPreview(rect, scale, color, net);
	}
}



static qboolean updateModel = qtrue;
static qboolean q3Model = qfalse;

static void UI_DrawPlayerModel(rectDef_t *rect) {
  static playerInfo_t info;
  char model[MAX_QPATH];
  char team[256];
	char head[256];
	vec3_t	viewangles;
	vec3_t	moveangles;

	  if (trap_Cvar_VariableValue("ui_Q3Model")) {
	  strcpy(model, UI_Cvar_VariableString("model"));
		strcpy(head, UI_Cvar_VariableString("headmodel"));
		if (!q3Model) {
			q3Model = qtrue;
			updateModel = qtrue;
		}
		team[0] = '\0';
	} else {

		strcpy(team, UI_Cvar_VariableString("ui_teamName"));
		strcpy(model, UI_Cvar_VariableString("team_model"));
		strcpy(head, UI_Cvar_VariableString("team_headmodel"));
		if (q3Model) {
			q3Model = qfalse;
			updateModel = qtrue;
		}
	}
  if (updateModel) {
  	memset( &info, 0, sizeof(playerInfo_t) );
  	viewangles[YAW]   = 180 - 10;
  	viewangles[PITCH] = 0;
  	viewangles[ROLL]  = 0;
  	VectorClear( moveangles );
    UI_PlayerInfo_SetModel( &info, model, head, team);
    UI_PlayerInfo_SetInfo( &info, LEGS_IDLE, TORSO_STAND, viewangles, vec3_origin, WP_MACHINEGUN, qfalse );
//		UI_RegisterClientModelname( &info, model, head, team);
    updateModel = qfalse;
  }

  UI_DrawPlayer( rect->x, rect->y, rect->w, rect->h, &info, uiInfo.uiDC.realTime / 2);

}

static void UI_DrawNetSource(rectDef_t *rect, float scale, vec4_t color, int textStyle) {
	char	label[MAX_SOURCELABEL+1];
	char	text[64];
	int		len;

	Q_strncpyz( label, uiNetSources[UI_ActiveNetSource()].label, sizeof( label ) );

	// fit the label to this widget's actual metrics: chop characters until
	// the painted string stays inside the rect (joinserver's itemDef gives
	// the source row 134px at .32 scale)
	Com_sprintf( text, sizeof( text ), "Source: %s", label );
	len = strlen( label );
	while ( len > 1 && rect->w > 0 && Text_Width( text, scale, 0 ) > rect->w ) {
		len--;
		label[len] = '\0';
		Com_sprintf( text, sizeof( text ), "Source: %s", label );
	}

	Text_Paint(rect->x, rect->y, scale, color, text, 0, 0, textStyle);
}

static void UI_DrawNetMapPreview(rectDef_t *rect, float scale, vec4_t color) {

	if (uiInfo.serverStatus.currentServerPreview > 0) {
		UI_DrawHandlePic( rect->x, rect->y, rect->w, rect->h, uiInfo.serverStatus.currentServerPreview);
	} else {
		UI_DrawHandlePic( rect->x, rect->y, rect->w, rect->h, trap_R_RegisterShaderNoMip("menu/art/unknownmap"));
	}
}

static void UI_DrawNetMapCinematic(rectDef_t *rect, float scale, vec4_t color) {
	if (ui_currentNetMap.integer < 0 || ui_currentNetMap.integer > uiInfo.mapCount) {
		ui_currentNetMap.integer = 0;
		trap_Cvar_Set("ui_currentNetMap", "0");
	}

	if (uiInfo.serverStatus.currentServerCinematic >= 0) {
	  trap_CIN_RunCinematic(uiInfo.serverStatus.currentServerCinematic);
	  UI_CIN_SetExtents(uiInfo.serverStatus.currentServerCinematic, rect->x, rect->y, rect->w, rect->h);
 	  trap_CIN_DrawCinematic(uiInfo.serverStatus.currentServerCinematic);
	} else {
		UI_DrawNetMapPreview(rect, scale, color);
	}
}



static void UI_DrawNetFilter(rectDef_t *rect, float scale, vec4_t color, int textStyle) {
	if (ui_serverFilterType.integer < 0 || ui_serverFilterType.integer >= numServerFilters) {
		ui_serverFilterType.integer = 0;
	}
	Text_Paint(rect->x, rect->y, scale, color, va("Filter: %s", serverFilters[ui_serverFilterType.integer].description), 0, 0, textStyle);
}


static void UI_DrawTier(rectDef_t *rect, float scale, vec4_t color, int textStyle) {
  int i;
	i = trap_Cvar_VariableValue( "ui_currentTier" );
  if (i < 0 || i >= uiInfo.tierCount) {
    i = 0;
  }
  Text_Paint(rect->x, rect->y, scale, color, va("Tier: %s", uiInfo.tierList[i].tierName),0, 0, textStyle);
}

static void UI_DrawTierMap(rectDef_t *rect, int index) {
  int i;
	i = trap_Cvar_VariableValue( "ui_currentTier" );
  if (i < 0 || i >= uiInfo.tierCount) {
    i = 0;
  }

	if (uiInfo.tierList[i].mapHandles[index] == -1) {
		uiInfo.tierList[i].mapHandles[index] = trap_R_RegisterShaderNoMip(va("levelshots/%s", uiInfo.tierList[i].maps[index]));
	}
												 
	UI_DrawHandlePic( rect->x, rect->y, rect->w, rect->h, uiInfo.tierList[i].mapHandles[index]);
}

static const char *UI_EnglishMapName(const char *map) {
	int i;
	for (i = 0; i < uiInfo.mapCount; i++) {
		if (Q_stricmp(map, uiInfo.mapList[i].mapLoadName) == 0) {
			return uiInfo.mapList[i].mapName;
		}
	}
	return "";
}

static void UI_DrawTierMapName(rectDef_t *rect, float scale, vec4_t color, int textStyle) {
  int i, j;
	i = trap_Cvar_VariableValue( "ui_currentTier" );
  if (i < 0 || i >= uiInfo.tierCount) {
    i = 0;
  }
	j = trap_Cvar_VariableValue("ui_currentMap");
	if (j < 0 || j > MAPS_PER_TIER) {
		j = 0;
	}

  Text_Paint(rect->x, rect->y, scale, color, UI_EnglishMapName(uiInfo.tierList[i].maps[j]), 0, 0, textStyle);
}

static void UI_DrawTierGameType(rectDef_t *rect, float scale, vec4_t color, int textStyle) {
  int i, j;
	i = trap_Cvar_VariableValue( "ui_currentTier" );
  if (i < 0 || i >= uiInfo.tierCount) {
    i = 0;
  }
	j = trap_Cvar_VariableValue("ui_currentMap");
	if (j < 0 || j > MAPS_PER_TIER) {
		j = 0;
	}

  Text_Paint(rect->x, rect->y, scale, color, uiInfo.gameTypes[uiInfo.tierList[i].gameTypes[j]].gameType , 0, 0, textStyle);
}


#ifndef MISSIONPACK // bk001206
static const char *UI_OpponentLeaderName() {
  int i = UI_TeamIndexFromName(UI_Cvar_VariableString("ui_opponentName"));
	return uiInfo.teamList[i].teamMembers[0];
}
#endif

static const char *UI_AIFromName(const char *name) {
	int j;
	for (j = 0; j < uiInfo.aliasCount; j++) {
		if (Q_stricmp(uiInfo.aliasList[j].name, name) == 0) {
			return uiInfo.aliasList[j].ai;
		}
	}
	return "James";
}

#ifndef MISSIONPACK // bk001206
static const int UI_AIIndex(const char *name) {
	int j;
	for (j = 0; j < uiInfo.characterCount; j++) {
		if (Q_stricmp(name, uiInfo.characterList[j].name) == 0) {
			return j;
		}
	}
	return 0;
}
#endif

#ifndef MISSIONPACK // bk001206
static const int UI_AIIndexFromName(const char *name) {
	int j;
	for (j = 0; j < uiInfo.aliasCount; j++) {
		if (Q_stricmp(uiInfo.aliasList[j].name, name) == 0) {
			return UI_AIIndex(uiInfo.aliasList[j].ai);
		}
	}
	return 0;
}
#endif


#ifndef MISSIONPACK // bk001206
static const char *UI_OpponentLeaderHead() {
	const char *leader = UI_OpponentLeaderName();
	return UI_AIFromName(leader);
}
#endif

#ifndef MISSIONPACK // bk001206
static const char *UI_OpponentLeaderModel() {
	int i;
	const char *head = UI_OpponentLeaderHead();
	for (i = 0; i < uiInfo.characterCount; i++) {
		if (Q_stricmp(head, uiInfo.characterList[i].name) == 0) {
			return uiInfo.characterList[i].base;
		}
	}
	return "James";
}
#endif


static qboolean updateOpponentModel = qtrue;
static void UI_DrawOpponent(rectDef_t *rect) {
  static playerInfo_t info2;
  char model[MAX_QPATH];
  char headmodel[MAX_QPATH];
  char team[256];
	vec3_t	viewangles;
	vec3_t	moveangles;
  
	if (updateOpponentModel) {
		
		strcpy(model, UI_Cvar_VariableString("ui_opponentModel"));
	  strcpy(headmodel, UI_Cvar_VariableString("ui_opponentModel"));
		team[0] = '\0';

  	memset( &info2, 0, sizeof(playerInfo_t) );
  	viewangles[YAW]   = 180 - 10;
  	viewangles[PITCH] = 0;
  	viewangles[ROLL]  = 0;
  	VectorClear( moveangles );
    UI_PlayerInfo_SetModel( &info2, model, headmodel, "");
    UI_PlayerInfo_SetInfo( &info2, LEGS_IDLE, TORSO_STAND, viewangles, vec3_origin, WP_MACHINEGUN, qfalse );
		UI_RegisterClientModelname( &info2, model, headmodel, team);
    updateOpponentModel = qfalse;
  }

  UI_DrawPlayer( rect->x, rect->y, rect->w, rect->h, &info2, uiInfo.uiDC.realTime / 2);

}

static void UI_NextOpponent() {
  int i = UI_TeamIndexFromName(UI_Cvar_VariableString("ui_opponentName"));
  int j = UI_TeamIndexFromName(UI_Cvar_VariableString("ui_teamName"));
	i++;
	if (i >= uiInfo.teamCount) {
		i = 0;
	}
	if (i == j) {
		i++;
		if ( i >= uiInfo.teamCount) {
			i = 0;
		}
	}
 	trap_Cvar_Set( "ui_opponentName", uiInfo.teamList[i].teamName );
}

static void UI_PriorOpponent() {
  int i = UI_TeamIndexFromName(UI_Cvar_VariableString("ui_opponentName"));
  int j = UI_TeamIndexFromName(UI_Cvar_VariableString("ui_teamName"));
	i--;
	if (i < 0) {
		i = uiInfo.teamCount - 1;
	}
	if (i == j) {
		i--;
		if ( i < 0) {
			i = uiInfo.teamCount - 1;
		}
	}
 	trap_Cvar_Set( "ui_opponentName", uiInfo.teamList[i].teamName );
}

static void	UI_DrawPlayerLogo(rectDef_t *rect, vec3_t color) {
  int i = UI_TeamIndexFromName(UI_Cvar_VariableString("ui_teamName"));

	if (uiInfo.teamList[i].teamIcon == -1) {
    uiInfo.teamList[i].teamIcon = trap_R_RegisterShaderNoMip(uiInfo.teamList[i].imageName);
    uiInfo.teamList[i].teamIcon_Metal = trap_R_RegisterShaderNoMip(va("%s_metal",uiInfo.teamList[i].imageName));
    uiInfo.teamList[i].teamIcon_Name = trap_R_RegisterShaderNoMip(va("%s_name", uiInfo.teamList[i].imageName));
	}

 	trap_R_SetColor( color );
	UI_DrawHandlePic( rect->x, rect->y, rect->w, rect->h, uiInfo.teamList[i].teamIcon );
 	trap_R_SetColor( NULL );
}

static void	UI_DrawPlayerLogoMetal(rectDef_t *rect, vec3_t color) {
  int i = UI_TeamIndexFromName(UI_Cvar_VariableString("ui_teamName"));
	if (uiInfo.teamList[i].teamIcon == -1) {
    uiInfo.teamList[i].teamIcon = trap_R_RegisterShaderNoMip(uiInfo.teamList[i].imageName);
    uiInfo.teamList[i].teamIcon_Metal = trap_R_RegisterShaderNoMip(va("%s_metal",uiInfo.teamList[i].imageName));
    uiInfo.teamList[i].teamIcon_Name = trap_R_RegisterShaderNoMip(va("%s_name", uiInfo.teamList[i].imageName));
	}

 	trap_R_SetColor( color );
	UI_DrawHandlePic( rect->x, rect->y, rect->w, rect->h, uiInfo.teamList[i].teamIcon_Metal );
 	trap_R_SetColor( NULL );
}

static void	UI_DrawPlayerLogoName(rectDef_t *rect, vec3_t color) {
  int i = UI_TeamIndexFromName(UI_Cvar_VariableString("ui_teamName"));
	if (uiInfo.teamList[i].teamIcon == -1) {
    uiInfo.teamList[i].teamIcon = trap_R_RegisterShaderNoMip(uiInfo.teamList[i].imageName);
    uiInfo.teamList[i].teamIcon_Metal = trap_R_RegisterShaderNoMip(va("%s_metal",uiInfo.teamList[i].imageName));
    uiInfo.teamList[i].teamIcon_Name = trap_R_RegisterShaderNoMip(va("%s_name", uiInfo.teamList[i].imageName));
	}

 	trap_R_SetColor( color );
	UI_DrawHandlePic( rect->x, rect->y, rect->w, rect->h, uiInfo.teamList[i].teamIcon_Name );
 	trap_R_SetColor( NULL );
}

static void	UI_DrawOpponentLogo(rectDef_t *rect, vec3_t color) {
  int i = UI_TeamIndexFromName(UI_Cvar_VariableString("ui_opponentName"));
	if (uiInfo.teamList[i].teamIcon == -1) {
    uiInfo.teamList[i].teamIcon = trap_R_RegisterShaderNoMip(uiInfo.teamList[i].imageName);
    uiInfo.teamList[i].teamIcon_Metal = trap_R_RegisterShaderNoMip(va("%s_metal",uiInfo.teamList[i].imageName));
    uiInfo.teamList[i].teamIcon_Name = trap_R_RegisterShaderNoMip(va("%s_name", uiInfo.teamList[i].imageName));
	}

 	trap_R_SetColor( color );
	UI_DrawHandlePic( rect->x, rect->y, rect->w, rect->h, uiInfo.teamList[i].teamIcon );
 	trap_R_SetColor( NULL );
}

static void	UI_DrawOpponentLogoMetal(rectDef_t *rect, vec3_t color) {
  int i = UI_TeamIndexFromName(UI_Cvar_VariableString("ui_opponentName"));
	if (uiInfo.teamList[i].teamIcon == -1) {
    uiInfo.teamList[i].teamIcon = trap_R_RegisterShaderNoMip(uiInfo.teamList[i].imageName);
    uiInfo.teamList[i].teamIcon_Metal = trap_R_RegisterShaderNoMip(va("%s_metal",uiInfo.teamList[i].imageName));
    uiInfo.teamList[i].teamIcon_Name = trap_R_RegisterShaderNoMip(va("%s_name", uiInfo.teamList[i].imageName));
	}

 	trap_R_SetColor( color );
	UI_DrawHandlePic( rect->x, rect->y, rect->w, rect->h, uiInfo.teamList[i].teamIcon_Metal );
 	trap_R_SetColor( NULL );
}

static void	UI_DrawOpponentLogoName(rectDef_t *rect, vec3_t color) {
  int i = UI_TeamIndexFromName(UI_Cvar_VariableString("ui_opponentName"));
	if (uiInfo.teamList[i].teamIcon == -1) {
    uiInfo.teamList[i].teamIcon = trap_R_RegisterShaderNoMip(uiInfo.teamList[i].imageName);
    uiInfo.teamList[i].teamIcon_Metal = trap_R_RegisterShaderNoMip(va("%s_metal",uiInfo.teamList[i].imageName));
    uiInfo.teamList[i].teamIcon_Name = trap_R_RegisterShaderNoMip(va("%s_name", uiInfo.teamList[i].imageName));
	}

 	trap_R_SetColor( color );
	UI_DrawHandlePic( rect->x, rect->y, rect->w, rect->h, uiInfo.teamList[i].teamIcon_Name );
 	trap_R_SetColor( NULL );
}

static void UI_DrawAllMapsSelection(rectDef_t *rect, float scale, vec4_t color, int textStyle, qboolean net) {
	int map = (net) ? ui_currentNetMap.integer : ui_currentMap.integer;
	if (map >= 0 && map < uiInfo.mapCount) {
	  Text_Paint(rect->x, rect->y, scale, color, uiInfo.mapList[map].mapName, 0, 0, textStyle);
	}
}

static void UI_DrawOpponentName(rectDef_t *rect, float scale, vec4_t color, int textStyle) {
  Text_Paint(rect->x, rect->y, scale, color, UI_Cvar_VariableString("ui_opponentName"), 0, 0, textStyle);
}

/*
=================
UI_VR_CancelButtonName

Name of the control that synthesizes K_ESCAPE under a VR engine, asked
once, for prompts that tell the player how to cancel. NULL on flatscreen
or when the engine doesn't answer, so callers keep the stock ESC text.
=================
*/
static const char *UI_VR_CancelButtonName( void ) {
	static char name[32];
	static qboolean resolved = qfalse;

	if ( !resolved ) {
		resolved = qtrue;
		if ( UI_VR_Platform() != VRP_NONE ) {
			if ( !trap_GetValue( name, sizeof( name ), "vr_menu_cancel_button" ) ) {
				name[0] = '\0';
			}
		}
	}
	return name[0] ? name : NULL;
}


static int UI_OwnerDrawWidth(int ownerDraw, float scale) {
	int i, h, value;
	const char *text;
	const char *s = NULL;

  switch (ownerDraw) {
    case UI_HANDICAP:
			  h = Com_Clamp( 5, 100, trap_Cvar_VariableValue("handicap") );
				i = 20 - h / 5;
				s = handicapValues[i];
      break;
    case UI_CLANNAME:
				s = UI_Cvar_VariableString("ui_teamName");
      break;
    case UI_GAMETYPE:
				s = uiInfo.gameTypes[ui_gameType.integer].gameType;
      break;
    case UI_SKILL:
				i = trap_Cvar_VariableValue( "g_spSkill" );
				if (i < 1 || i > numSkillLevels) {
					i = 1;
				}
			  s = skillLevels[i-1];
      break;
    case UI_BLUETEAMNAME:
			  i = UI_TeamIndexFromName(UI_Cvar_VariableString("ui_blueTeam"));
			  if (i >= 0 && i < uiInfo.teamCount) {
			    s = va("%s: %s", "Blue", uiInfo.teamList[i].teamName);
			  }
      break;
    case UI_REDTEAMNAME:
			  i = UI_TeamIndexFromName(UI_Cvar_VariableString("ui_redTeam"));
			  if (i >= 0 && i < uiInfo.teamCount) {
			    s = va("%s: %s", "Red", uiInfo.teamList[i].teamName);
			  }
      break;
    case UI_BLUETEAM1:
		case UI_BLUETEAM2:
		case UI_BLUETEAM3:
		case UI_BLUETEAM4:
		case UI_BLUETEAM5:
			value = trap_Cvar_VariableValue(va("ui_blueteam%i", ownerDraw-UI_BLUETEAM1 + 1));
			if (value <= 0) {
				text = "Closed";
			} else if (value == 1) {
				text = "Human";
			} else {
				value -= 2;
				if (value >= uiInfo.aliasCount) {
					value = 0;
				}
				text = uiInfo.aliasList[value].name;
			}
			s = va("%i. %s", ownerDraw-UI_BLUETEAM1 + 1, text);
      break;
    case UI_REDTEAM1:
		case UI_REDTEAM2:
		case UI_REDTEAM3:
		case UI_REDTEAM4:
		case UI_REDTEAM5:
			value = trap_Cvar_VariableValue(va("ui_redteam%i", ownerDraw-UI_REDTEAM1 + 1));
			if (value <= 0) {
				text = "Closed";
			} else if (value == 1) {
				text = "Human";
			} else {
				value -= 2;
				if (value >= uiInfo.aliasCount) {
					value = 0;
				}
				text = uiInfo.aliasList[value].name;
			}
			s = va("%i. %s", ownerDraw-UI_REDTEAM1 + 1, text);
      break;
		case UI_NETSOURCE:
			s = va("Source: %s", uiNetSources[UI_ActiveNetSource()].label);
			break;
		case UI_NETFILTER:
			if (ui_serverFilterType.integer < 0 || ui_serverFilterType.integer >= numServerFilters) {
				ui_serverFilterType.integer = 0;
			}
			s = va("Filter: %s", serverFilters[ui_serverFilterType.integer].description );
			break;
		case UI_TIER:
			break;
		case UI_TIER_MAPNAME:
			break;
		case UI_TIER_GAMETYPE:
			break;
		case UI_ALLMAPS_SELECTION:
			break;
		case UI_OPPONENT_NAME:
			break;
		case UI_KEYBINDSTATUS:
			if (Display_KeyBindPending()) {
				const char *cancelName = UI_VR_CancelButtonName();
				if (cancelName) {
					s = va("Waiting for new key... Press %s to cancel", cancelName);
				} else {
					s = "Waiting for new key... Press ESCAPE to cancel";
				}
			} else {
				s = "Press ENTER or CLICK to change, Press BACKSPACE to clear";
			}
			break;
		case UI_SERVERREFRESHDATE:
			s = UI_Cvar_VariableString(va("ui_lastServerRefresh_%i", ui_netSource.integer));
			break;
		case UI_TRINITYLOGIN:
			{
				char trinityUser[64];
				trap_Cvar_VariableStringBuffer( "cl_trinityUser", trinityUser, sizeof( trinityUser ) );
				s = trinityUser[0] ? va( "%s", trinityUser ) : "Account";
			}
			break;
		case UI_TRINITYUPDATE:
			if ( (int)trap_Cvar_VariableValue( "update_available" ) == 1 )
				s = "Update Available";
			break;
		case UI_UPDATEPROGRESS:
		case UI_UPDATESTATUS:
			return 0;
		case UI_HDR_PEAK_NITS:
			s = va( "%i nits", (int)( trap_Cvar_VariableValue( "r_hdrPeak" ) + 0.5f ) );
			break;
    default:
      break;
  }

	if (s) {
		return Text_Width(s, scale, 0);
	}
	return 0;
}

static void UI_DrawBotName(rectDef_t *rect, float scale, vec4_t color, int textStyle) {
	int value = uiInfo.botIndex;
	int game = trap_Cvar_VariableValue("g_gametype");
	const char *text = "";
	if (game >= GT_TEAM) {
		if (value >= uiInfo.characterCount) {
			value = 0;
		}
		text = uiInfo.characterList[value].name;
	} else {
		if (value >= UI_GetNumBots()) {
			value = 0;
		}
		text = UI_GetBotNameByNumber(value);
	}
  Text_Paint(rect->x, rect->y, scale, color, text, 0, 0, textStyle);
}

static void UI_DrawBotSkill(rectDef_t *rect, float scale, vec4_t color, int textStyle) {
	if (uiInfo.skillIndex >= 0 && uiInfo.skillIndex < numSkillLevels) {
	  Text_Paint(rect->x, rect->y, scale, color, skillLevels[uiInfo.skillIndex], 0, 0, textStyle);
	}
}

static void UI_DrawRedBlue(rectDef_t *rect, float scale, vec4_t color, int textStyle) {
  Text_Paint(rect->x, rect->y, scale, color, (uiInfo.redBlue == 0) ? "Red" : "Blue", 0, 0, textStyle);
}

static void UI_DrawCrosshair(rectDef_t *rect, float scale, vec4_t color) {
	vec4_t crosshairColor;
	int colorCode;

	if (!uiInfo.currentCrosshair) {
		return;
	}

	// Get crosshair color from cvar
	colorCode = (int)trap_Cvar_VariableValue("cg_crosshairColor");

	// Convert color code to RGB (same logic as CG_CrosshairColorFromInt)
	if ( colorCode < 1 || colorCode > 7 ) {
		crosshairColor[0] = 1.0f;
		crosshairColor[1] = 1.0f;
		crosshairColor[2] = 1.0f;
	} else {
		crosshairColor[0] = (colorCode & 4) ? 1.0f : 0.0f;
		crosshairColor[1] = (colorCode & 2) ? 1.0f : 0.0f;
		crosshairColor[2] = (colorCode & 1) ? 1.0f : 0.0f;
	}
	crosshairColor[3] = 1.0f;

	trap_R_SetColor( crosshairColor );
	UI_DrawHandlePic( rect->x, rect->y - rect->h, rect->w, rect->h, uiInfo.uiDC.Assets.crosshairShader[uiInfo.currentCrosshair]);
	trap_R_SetColor( NULL );
}

static void UI_DrawCrosshairColor(rectDef_t *rect, float scale, vec4_t color) {
	int gameColorCode;
	int uiColorIndex;
	int markerX;

	gameColorCode = (int)trap_Cvar_VariableValue("cg_crosshairColor");
	if (gameColorCode < 1 || gameColorCode > 7) {
		gameColorCode = 7;
	}
	uiColorIndex = gamecodetoui[gameColorCode - 1];
	markerX = rect->x + (uiColorIndex * 112 / 6);

	UI_DrawHandlePic( rect->x, rect->y - 10, 128, 8, uiInfo.uiDC.Assets.fxBasePic );
	UI_DrawHandlePic( markerX, rect->y - 12, 16, 12, uiInfo.uiDC.Assets.fxPic[uiColorIndex] );
}

/*
===============
UI_BuildPlayerList
===============
*/
static void UI_BuildPlayerList() {
	uiClientState_t	cs;
	int		n, count, team, team2, playerTeamNumber;
	char	info[MAX_INFO_STRING];

	trap_GetClientState( &cs );
	trap_GetConfigString( CS_PLAYERS + cs.clientNum, info, MAX_INFO_STRING );
	uiInfo.playerNumber = cs.clientNum;
	uiInfo.teamLeader = atoi(Info_ValueForKey(info, "tl"));
	team = atoi(Info_ValueForKey(info, "t"));
	trap_GetConfigString( CS_SERVERINFO, info, sizeof(info) );
	count = atoi( Info_ValueForKey( info, "sv_maxclients" ) );
	uiInfo.playerCount = 0;
	uiInfo.myTeamCount = 0;
	playerTeamNumber = 0;
	for( n = 0; n < count; n++ ) {
		trap_GetConfigString( CS_PLAYERS + n, info, MAX_INFO_STRING );

		if (info[0]) {
			Q_strncpyz( uiInfo.playerNames[uiInfo.playerCount], Info_ValueForKey( info, "n" ), MAX_NAME_LENGTH );
			Q_CleanStr( uiInfo.playerNames[uiInfo.playerCount] );
			uiInfo.playerCount++;
			team2 = atoi(Info_ValueForKey(info, "t"));
			if (team2 == team) {
				Q_strncpyz( uiInfo.teamNames[uiInfo.myTeamCount], Info_ValueForKey( info, "n" ), MAX_NAME_LENGTH );
				Q_CleanStr( uiInfo.teamNames[uiInfo.myTeamCount] );
				uiInfo.teamClientNums[uiInfo.myTeamCount] = n;
				if (uiInfo.playerNumber == n) {
					playerTeamNumber = uiInfo.myTeamCount;
				}
				uiInfo.myTeamCount++;
			}
		}
	}

	if (!uiInfo.teamLeader) {
		trap_Cvar_Set("cg_selectedPlayer", va("%d", playerTeamNumber));
	}

	n = trap_Cvar_VariableValue("cg_selectedPlayer");
	if (n < 0 || n > uiInfo.myTeamCount) {
		n = 0;
	}
	if (n < uiInfo.myTeamCount) {
		trap_Cvar_Set("cg_selectedPlayerName", uiInfo.teamNames[n]);
	}
}


static void UI_DrawSelectedPlayer(rectDef_t *rect, float scale, vec4_t color, int textStyle) {
	if (uiInfo.uiDC.realTime > uiInfo.playerRefresh) {
		uiInfo.playerRefresh = uiInfo.uiDC.realTime + 3000;
		UI_BuildPlayerList();
	}
  Text_Paint(rect->x, rect->y, scale, color, (uiInfo.teamLeader) ? UI_Cvar_VariableString("cg_selectedPlayerName") : UI_Cvar_VariableString("name") , 0, 0, textStyle);
}

static void UI_DrawServerRefreshDate(rectDef_t *rect, float scale, vec4_t color, int textStyle) {
	if (uiInfo.serverStatus.refreshActive) {
		vec4_t lowLight, newColor;
		char status[64];
		int count;
		lowLight[0] = 0.8 * color[0];
		lowLight[1] = 0.8 * color[1];
		lowLight[2] = 0.8 * color[2];
		lowLight[3] = 0.8 * color[3];
		LerpColor(color,lowLight,newColor,0.5+0.5*sin(uiInfo.uiDC.realTime / PULSE_DIVISOR));
		count = UI_SourceLANCount();
		if (count < 0) {
			// the server list itself is still in flight
			Q_strncpyz(status, "Awaiting server list", sizeof(status));
		} else {
			Com_sprintf(status, sizeof(status), "Getting info for %d servers", count);
		}
		if (UI_VR_CancelButtonName()) {
			Text_Paint(rect->x, rect->y, scale, newColor, va("%s (%s to cancel)", status, UI_VR_CancelButtonName()), 0, 0, textStyle);
		} else {
			Text_Paint(rect->x, rect->y, scale, newColor, va("%s (ESC to cancel)", status), 0, 0, textStyle);
		}
	} else {
		char buff[64];
		Q_strncpyz(buff, UI_Cvar_VariableString(va("ui_lastServerRefresh_%i", ui_netSource.integer)), 64);
	  Text_Paint(rect->x, rect->y, scale, color, va("Refresh Time: %s", buff), 0, 0, textStyle);
	}
}

static void UI_DrawServerMOTD(rectDef_t *rect, float scale, vec4_t color) {
	if (uiInfo.serverStatus.motdLen) {
		float maxX;
	 
		if (uiInfo.serverStatus.motdWidth == -1) {
			uiInfo.serverStatus.motdWidth = 0;
			uiInfo.serverStatus.motdPaintX = rect->x + 1;
			uiInfo.serverStatus.motdPaintX2 = -1;
		}

		if (uiInfo.serverStatus.motdOffset > uiInfo.serverStatus.motdLen) {
			uiInfo.serverStatus.motdOffset = 0;
			uiInfo.serverStatus.motdPaintX = rect->x + 1;
			uiInfo.serverStatus.motdPaintX2 = -1;
		}

		if (uiInfo.uiDC.realTime > uiInfo.serverStatus.motdTime) {
			uiInfo.serverStatus.motdTime = uiInfo.uiDC.realTime + 10;
			if (uiInfo.serverStatus.motdPaintX <= rect->x + 2) {
				if (uiInfo.serverStatus.motdOffset < uiInfo.serverStatus.motdLen) {
					uiInfo.serverStatus.motdPaintX += Text_Width(&uiInfo.serverStatus.motd[uiInfo.serverStatus.motdOffset], scale, 1) - 1;
					uiInfo.serverStatus.motdOffset++;
				} else {
					uiInfo.serverStatus.motdOffset = 0;
					if (uiInfo.serverStatus.motdPaintX2 >= 0) {
						uiInfo.serverStatus.motdPaintX = uiInfo.serverStatus.motdPaintX2;
					} else {
						uiInfo.serverStatus.motdPaintX = rect->x + rect->w - 2;
					}
					uiInfo.serverStatus.motdPaintX2 = -1;
				}
			} else {
				//serverStatus.motdPaintX--;
				uiInfo.serverStatus.motdPaintX -= 2;
				if (uiInfo.serverStatus.motdPaintX2 >= 0) {
					//serverStatus.motdPaintX2--;
					uiInfo.serverStatus.motdPaintX2 -= 2;
				}
			}
		}

		maxX = rect->x + rect->w - 2;
		Text_Paint_Limit(&maxX, uiInfo.serverStatus.motdPaintX, rect->y + rect->h - 3, scale, color, &uiInfo.serverStatus.motd[uiInfo.serverStatus.motdOffset], 0, 0); 
		if (uiInfo.serverStatus.motdPaintX2 >= 0) {
			float maxX2 = rect->x + rect->w - 2;
			Text_Paint_Limit(&maxX2, uiInfo.serverStatus.motdPaintX2, rect->y + rect->h - 3, scale, color, uiInfo.serverStatus.motd, 0, uiInfo.serverStatus.motdOffset); 
		}
		if (uiInfo.serverStatus.motdOffset && maxX > 0) {
			// if we have an offset ( we are skipping the first part of the string ) and we fit the string
			if (uiInfo.serverStatus.motdPaintX2 == -1) {
						uiInfo.serverStatus.motdPaintX2 = rect->x + rect->w - 2;
			}
		} else {
			uiInfo.serverStatus.motdPaintX2 = -1;
		}

	}
}

static void UI_DrawKeyBindStatus(rectDef_t *rect, float scale, vec4_t color, int textStyle) {
//	int ofs = 0; TTimo: unused
	if (Display_KeyBindPending()) {
		const char *cancelName = UI_VR_CancelButtonName();
		if (cancelName) {
			Text_Paint(rect->x, rect->y, scale, color, va("Waiting for new key... Press %s to cancel", cancelName), 0, 0, textStyle);
		} else {
			Text_Paint(rect->x, rect->y, scale, color, "Waiting for new key... Press ESCAPE to cancel", 0, 0, textStyle);
		}
	} else {
		Text_Paint(rect->x, rect->y, scale, color, "Press ENTER or CLICK to change, Press BACKSPACE to clear", 0, 0, textStyle);
	}
}

static void UI_DrawGLInfo(rectDef_t *rect, float scale, vec4_t color, int textStyle) {
	char * eptr;
	char buff[1024];
	const char *lines[64];
	int y, numLines, i;

	Text_Paint(rect->x + 2, rect->y, scale, color, va("VENDOR: %s", uiInfo.uiDC.glconfig.vendor_string), 0, 30, textStyle);
	Text_Paint(rect->x + 2, rect->y + 15, scale, color, va("VERSION: %s: %s", uiInfo.uiDC.glconfig.version_string,uiInfo.uiDC.glconfig.renderer_string), 0, 30, textStyle);
	Text_Paint(rect->x + 2, rect->y + 30, scale, color, va ("PIXELFORMAT: color(%d-bits) Z(%d-bits) stencil(%d-bits)", uiInfo.uiDC.glconfig.colorBits, uiInfo.uiDC.glconfig.depthBits, uiInfo.uiDC.glconfig.stencilBits), 0, 30, textStyle);

	// build null terminated extension strings
  // TTimo: https://zerowing.idsoftware.com/bugzilla/show_bug.cgi?id=399
  // in TA this was not directly crashing, but displaying a nasty broken shader right in the middle
  // brought down the string size to 1024, there's not much that can be shown on the screen anyway
	Q_strncpyz(buff, uiInfo.uiDC.glconfig.extensions_string, 1024);
	eptr = buff;
	y = rect->y + 45;
	numLines = 0;
	while ( y < rect->y + rect->h && *eptr )
	{
		while ( *eptr && *eptr == ' ' )
			*eptr++ = '\0';

		// track start of valid string
		if (*eptr && *eptr != ' ') {
			lines[numLines++] = eptr;
		}

		while ( *eptr && *eptr != ' ' )
			eptr++;
	}

	i = 0;
	while (i < numLines) {
		Text_Paint(rect->x + 2, y, scale, color, lines[i++], 0, 20, textStyle);
		if (i < numLines) {
			Text_Paint(rect->x + rect->w / 2, y, scale, color, lines[i++], 0, 20, textStyle);
		}
		y += 10;
		if (y > rect->y + rect->h - 11) {
			break;
		}
	}


}

// FIXME: table drive
//
static void UI_DrawHDRPeakNits(rectDef_t *rect, float scale, vec4_t color, int textStyle) {
	Text_Paint(rect->x, rect->y, scale, color, va("%i nits", (int)(trap_Cvar_VariableValue("r_hdrPeak") + 0.5f)), 0, 0, textStyle);
}

static const char *dynamicLightsSettings[] = {
	"Off",
	"Low",
	"Medium",
	"High"
};

// r_dynamiclight is the master switch and r_dlightMode picks the quality behind
// it; a declarative cvarFloatList binds only one cvar, so this row is an
// ownerdraw to fold both into a single off/low/medium/high step
static int UI_DynamicLightsIndex(void) {
	if (trap_Cvar_VariableValue("r_dynamiclight") == 0) {
		return 0;
	}
	return Com_Clamp(1, 3, trap_Cvar_VariableValue("r_dlightMode") + 1);
}

static void UI_DrawDynamicLights(rectDef_t *rect, float scale, vec4_t color, int textStyle) {
	Text_Paint(rect->x, rect->y, scale, color, dynamicLightsSettings[UI_DynamicLightsIndex()], 0, 0, textStyle);
}

static void UI_OwnerDraw(float x, float y, float w, float h, float text_x, float text_y, int ownerDraw, int ownerDrawFlags, int align, float special, float scale, vec4_t color, qhandle_t shader, int textStyle) {
	rectDef_t rect;

  rect.x = x + text_x;
  rect.y = y + text_y;
  rect.w = w;
  rect.h = h;

  switch (ownerDraw) {
    case UI_HANDICAP:
      UI_DrawHandicap(&rect, scale, color, textStyle);
      break;
    case UI_EFFECTS:
      UI_DrawEffects(&rect, scale, color);
      break;
    case UI_PLAYERMODEL:
      UI_DrawPlayerModel(&rect);
      break;
    case UI_TRINITYSIGIL:
      UI_DrawTrinitySigil(&rect);
      break;
    case UI_CLANNAME:
      UI_DrawClanName(&rect, scale, color, textStyle);
      break;
    case UI_CLANLOGO:
      UI_DrawClanLogo(&rect, scale, color);
      break;
    case UI_CLANCINEMATIC:
      UI_DrawClanCinematic(&rect, scale, color);
      break;
    case UI_PREVIEWCINEMATIC:
      UI_DrawPreviewCinematic(&rect, scale, color);
      break;
    case UI_GAMETYPE:
      UI_DrawGameType(&rect, scale, color, textStyle);
      break;
    case UI_NETGAMETYPE:
      UI_DrawNetGameType(&rect, scale, color, textStyle);
      break;
    case UI_JOINGAMETYPE:
	  UI_DrawJoinGameType(&rect, scale, color, textStyle);
	  break;
    case UI_MAPPREVIEW:
      UI_DrawMapPreview(&rect, scale, color, qtrue);
      break;
    case UI_MAP_TIMETOBEAT:
      UI_DrawMapTimeToBeat(&rect, scale, color, textStyle);
      break;
    case UI_MAPCINEMATIC:
      UI_DrawMapCinematic(&rect, scale, color, qfalse);
      break;
    case UI_STARTMAPCINEMATIC:
      UI_DrawMapCinematic(&rect, scale, color, qtrue);
      break;
    case UI_SKILL:
      UI_DrawSkill(&rect, scale, color, textStyle);
      break;
    case UI_BLUETEAMNAME:
      UI_DrawTeamName(&rect, scale, color, qtrue, textStyle);
      break;
    case UI_REDTEAMNAME:
      UI_DrawTeamName(&rect, scale, color, qfalse, textStyle);
      break;
    case UI_BLUETEAM1:
		case UI_BLUETEAM2:
		case UI_BLUETEAM3:
		case UI_BLUETEAM4:
		case UI_BLUETEAM5:
      UI_DrawTeamMember(&rect, scale, color, qtrue, ownerDraw - UI_BLUETEAM1 + 1, textStyle);
      break;
    case UI_REDTEAM1:
		case UI_REDTEAM2:
		case UI_REDTEAM3:
		case UI_REDTEAM4:
		case UI_REDTEAM5:
      UI_DrawTeamMember(&rect, scale, color, qfalse, ownerDraw - UI_REDTEAM1 + 1, textStyle);
      break;
		case UI_NETSOURCE:
      UI_DrawNetSource(&rect, scale, color, textStyle);
			break;
    case UI_NETMAPPREVIEW:
      UI_DrawNetMapPreview(&rect, scale, color);
      break;
    case UI_NETMAPCINEMATIC:
      UI_DrawNetMapCinematic(&rect, scale, color);
      break;
		case UI_NETFILTER:
      UI_DrawNetFilter(&rect, scale, color, textStyle);
			break;
		case UI_TIER:
			UI_DrawTier(&rect, scale, color, textStyle);
			break;
		case UI_OPPONENTMODEL:
			UI_DrawOpponent(&rect);
			break;
		case UI_TIERMAP1:
			UI_DrawTierMap(&rect, 0);
			break;
		case UI_TIERMAP2:
			UI_DrawTierMap(&rect, 1);
			break;
		case UI_TIERMAP3:
			UI_DrawTierMap(&rect, 2);
			break;
		case UI_PLAYERLOGO:
			UI_DrawPlayerLogo(&rect, color);
			break;
		case UI_PLAYERLOGO_METAL:
			UI_DrawPlayerLogoMetal(&rect, color);
			break;
		case UI_PLAYERLOGO_NAME:
			UI_DrawPlayerLogoName(&rect, color);
			break;
		case UI_OPPONENTLOGO:
			UI_DrawOpponentLogo(&rect, color);
			break;
		case UI_OPPONENTLOGO_METAL:
			UI_DrawOpponentLogoMetal(&rect, color);
			break;
		case UI_OPPONENTLOGO_NAME:
			UI_DrawOpponentLogoName(&rect, color);
			break;
		case UI_TIER_MAPNAME:
			UI_DrawTierMapName(&rect, scale, color, textStyle);
			break;
		case UI_TIER_GAMETYPE:
			UI_DrawTierGameType(&rect, scale, color, textStyle);
			break;
		case UI_ALLMAPS_SELECTION:
			UI_DrawAllMapsSelection(&rect, scale, color, textStyle, qtrue);
			break;
		case UI_MAPS_SELECTION:
			UI_DrawAllMapsSelection(&rect, scale, color, textStyle, qfalse);
			break;
		case UI_OPPONENT_NAME:
			UI_DrawOpponentName(&rect, scale, color, textStyle);
			break;
		case UI_BOTNAME:
			UI_DrawBotName(&rect, scale, color, textStyle);
			break;
		case UI_BOTSKILL:
			UI_DrawBotSkill(&rect, scale, color, textStyle);
			break;
		case UI_REDBLUE:
			UI_DrawRedBlue(&rect, scale, color, textStyle);
			break;
		case UI_CROSSHAIR:
			UI_DrawCrosshair(&rect, scale, color);
			break;
		case UI_CROSSHAIRCOLOR:
			UI_DrawCrosshairColor(&rect, scale, color);
			break;
		case UI_SELECTEDPLAYER:
			UI_DrawSelectedPlayer(&rect, scale, color, textStyle);
			break;
		case UI_SERVERREFRESHDATE:
			UI_DrawServerRefreshDate(&rect, scale, color, textStyle);
			break;
		case UI_SERVERMOTD:
			UI_DrawServerMOTD(&rect, scale, color);
			break;
		case UI_GLINFO:
			UI_DrawGLInfo(&rect,scale, color, textStyle);
			break;
		case UI_KEYBINDSTATUS:
			UI_DrawKeyBindStatus(&rect,scale, color, textStyle);
			break;
		case UI_HDR_PEAK_NITS:
			UI_DrawHDRPeakNits(&rect, scale, color, textStyle);
			break;
		case UI_DYNAMICLIGHTS:
			UI_DrawDynamicLights(&rect, scale, color, textStyle);
			break;
		case UI_TRINITYLOGIN:
			{
				char trinityUser[64];
				const char *label;
				float tw;
				trap_Cvar_VariableStringBuffer( "cl_trinityUser", trinityUser, sizeof( trinityUser ) );
				label = trinityUser[0] ? trinityUser : "Account";
				tw = Text_Width( label, scale, 0 );
				Text_Paint( rect.x - tw, rect.y, scale, color, label, 0, 0, textStyle );
			}
			break;
		case UI_TRINITYUPDATE:
			if ( (int)trap_Cvar_VariableValue( "update_available" ) == 1 ) {
				const char *label = "Update Available";
				float tw = Text_Width( label, scale, 0 );
				Text_Paint( rect.x - tw, rect.y, scale, color, label, 0, 0, textStyle );
			}
			break;
		case UI_UPDATEPROGRESS:
			{
				int state = (int)trap_Cvar_VariableValue( "update_state" );
				vec4_t barBg = { 0.1f, 0.1f, 0.1f, 0.8f };
				vec4_t barFg = { 0.1f, 0.5f, 0.1f, 0.9f };
				vec4_t barBorder = { 0.5f, 0.5f, 0.5f, 0.5f };

				// background
				DC->fillRect( rect.x, rect.y, rect.w, rect.h, barBg );
				DC->drawRect( rect.x, rect.y, rect.w, rect.h, 1, barBorder );

				if ( state == 3 ) {
					// downloading: show fill
					int progress = (int)trap_Cvar_VariableValue( "update_progress" );
					float fillW;
					if ( progress < 0 ) progress = 0;
					if ( progress > 100 ) progress = 100;
					fillW = ( rect.w * progress ) / 100.0f;
					if ( fillW > 0 )
						DC->fillRect( rect.x, rect.y, fillW, rect.h, barFg );
				} else if ( state == 5 ) {
					// staged: full green bar
					DC->fillRect( rect.x, rect.y, rect.w, rect.h, barFg );
				}
			}
			break;
		case UI_UPDATESTATUS:
			{
				int state = (int)trap_Cvar_VariableValue( "update_state" );
				const char *label = "";
				vec4_t white = { 1.0f, 1.0f, 1.0f, 1.0f };
				vec4_t yellow = { 1.0f, 0.75f, 0.0f, 1.0f };
				vec4_t green = { 0.0f, 1.0f, 0.0f, 1.0f };
				vec4_t red = { 1.0f, 0.0f, 0.0f, 1.0f };
				vec4_t *col = &white;
				char buf[128];

				switch ( state ) {
				case 0: label = "Up to date"; break;
				case 1: label = "Checking for updates..."; break;
				case 2: label = "Update available"; col = &yellow; break;
				case 3:
					Com_sprintf( buf, sizeof( buf ), "Downloading... %i%%",
						(int)trap_Cvar_VariableValue( "update_progress" ) );
					label = buf;
					break;
				case 4: label = "Extracting..."; break;
				case 5: label = "Update ready! Restart to apply."; col = &green; break;
				case 6:
					trap_Cvar_VariableStringBuffer( "update_error", buf, sizeof( buf ) );
					label = buf;
					col = &red;
					break;
				}
				{
					float tw = Text_Width( label, scale, 0 );
					Text_Paint( rect.x + ( rect.w - tw ) / 2, rect.y, scale, *col, label, 0, 0, textStyle );
				}
			}
			break;
    default:
      break;
  }

}

static qboolean UI_OwnerDrawVisible(int flags) {
	qboolean vis = qtrue;

	while (flags) {

		if (flags & UI_SHOW_FFA) {
			if (trap_Cvar_VariableValue("g_gametype") != GT_FFA) {
				vis = qfalse;
			}
			flags &= ~UI_SHOW_FFA;
		}

		if (flags & UI_SHOW_NOTFFA) {
			if (trap_Cvar_VariableValue("g_gametype") == GT_FFA) {
				vis = qfalse;
			}
			flags &= ~UI_SHOW_NOTFFA;
		}

		if (flags & UI_SHOW_LEADER) {
			// these need to show when this client can give orders to a player or a group
			if (!uiInfo.teamLeader) {
				vis = qfalse;
			} else {
				// if showing yourself
				if (ui_selectedPlayer.integer < uiInfo.myTeamCount && uiInfo.teamClientNums[ui_selectedPlayer.integer] == uiInfo.playerNumber) { 
					vis = qfalse;
				}
			}
			flags &= ~UI_SHOW_LEADER;
		} 
		if (flags & UI_SHOW_NOTLEADER) {
			// these need to show when this client is assigning their own status or they are NOT the leader
			if (uiInfo.teamLeader) {
				// if not showing yourself
				if (!(ui_selectedPlayer.integer < uiInfo.myTeamCount && uiInfo.teamClientNums[ui_selectedPlayer.integer] == uiInfo.playerNumber)) { 
					vis = qfalse;
				}
				// these need to show when this client can give orders to a player or a group
			}
			flags &= ~UI_SHOW_NOTLEADER;
		} 
		if (flags & UI_SHOW_FAVORITESERVERS) {
			// this assumes you only put this type of display flag on something showing in the proper context
			if (UI_NetSourceType() != SRC_FAVORITES) {
				vis = qfalse;
			}
			flags &= ~UI_SHOW_FAVORITESERVERS;
		}
		if (flags & UI_SHOW_NOTFAVORITESERVERS) {
			// this assumes you only put this type of display flag on something showing in the proper context
			if (UI_NetSourceType() == SRC_FAVORITES) {
				vis = qfalse;
			}
			flags &= ~UI_SHOW_NOTFAVORITESERVERS;
		}
		if (flags & UI_SHOW_ANYTEAMGAME) {
			if (uiInfo.gameTypes[ui_gameType.integer].gtEnum <= GT_TEAM ) {
				vis = qfalse;
			}
			flags &= ~UI_SHOW_ANYTEAMGAME;
		} 
		if (flags & UI_SHOW_ANYNONTEAMGAME) {
			if (uiInfo.gameTypes[ui_gameType.integer].gtEnum > GT_TEAM ) {
				vis = qfalse;
			}
			flags &= ~UI_SHOW_ANYNONTEAMGAME;
		} 
		if (flags & UI_SHOW_NETANYTEAMGAME) {
			if (uiInfo.gameTypes[ui_netGameType.integer].gtEnum <= GT_TEAM ) {
				vis = qfalse;
			}
			flags &= ~UI_SHOW_NETANYTEAMGAME;
		} 
		if (flags & UI_SHOW_NETANYNONTEAMGAME) {
			if (uiInfo.gameTypes[ui_netGameType.integer].gtEnum > GT_TEAM ) {
				vis = qfalse;
			}
			flags &= ~UI_SHOW_NETANYNONTEAMGAME;
		} 
		if (flags & UI_SHOW_NEWHIGHSCORE) {
			if (uiInfo.newHighScoreTime < uiInfo.uiDC.realTime) {
				vis = qfalse;
			} else {
				if (uiInfo.soundHighScore) {
					if (trap_Cvar_VariableValue("sv_killserver") == 0) {
						// wait on server to go down before playing sound
						trap_S_StartLocalSound(uiInfo.newHighScoreSound, CHAN_ANNOUNCER);
						uiInfo.soundHighScore = qfalse;
					}
				}
			}
			flags &= ~UI_SHOW_NEWHIGHSCORE;
		} 
		if (flags & UI_SHOW_NEWBESTTIME) {
			if (uiInfo.newBestTime < uiInfo.uiDC.realTime) {
				vis = qfalse;
			}
			flags &= ~UI_SHOW_NEWBESTTIME;
		} 
		if (flags & UI_SHOW_DEMOAVAILABLE) {
			if (!uiInfo.demoAvailable) {
				vis = qfalse;
			}
			flags &= ~UI_SHOW_DEMOAVAILABLE;
		} else {
			flags = 0;
		}
	}
  return vis;
}

static qboolean UI_Handicap_HandleKey(int flags, float *special, int key) {
  int select = UI_SelectForKey(key);
  if (select != 0) {
    int h;
    h = Com_Clamp( 5, 100, trap_Cvar_VariableValue("handicap") );
    h += 5 * select;
    if (h > 100) {
      h = 5;
    } else if (h < 5) {
			h = 100;
		}
  	trap_Cvar_Set( "handicap", va( "%i", h) );
    return qtrue;
  }
  return qfalse;
}

static qboolean UI_DynamicLights_HandleKey(int flags, float *special, int key) {
  int select = UI_SelectForKey(key);
  if (select != 0) {
    int i = UI_DynamicLightsIndex() + select;

    if (i > 3) {
      i = 0;
    } else if (i < 0) {
      i = 3;
    }

    trap_Cvar_SetValue( "r_dynamiclight", i != 0 );
    // leave the mode alone when switching off so the chosen quality
    // survives a trip through Off and back
    if (i != 0) {
      trap_Cvar_SetValue( "r_dlightMode", i - 1 );
    }
    return qtrue;
  }
  return qfalse;
}

static qboolean UI_Effects_HandleKey(int flags, float *special, int key) {
  int select = UI_SelectForKey(key);
  if (select != 0) {
    uiInfo.effectsColor += select;

    if( uiInfo.effectsColor > 6 ) {
	  	uiInfo.effectsColor = 0;
		} else if (uiInfo.effectsColor < 0) {
	  	uiInfo.effectsColor = 6;
		}

	  trap_Cvar_SetValue( "color1", uitogamecode[uiInfo.effectsColor] );
    return qtrue;
  }
  return qfalse;
}

static qboolean UI_ClanName_HandleKey(int flags, float *special, int key) {
  int select = UI_SelectForKey(key);
  if (select != 0) {
    int i;
    i = UI_TeamIndexFromName(UI_Cvar_VariableString("ui_teamName"));
		if (uiInfo.teamList[i].cinematic >= 0) {
		  trap_CIN_StopCinematic(uiInfo.teamList[i].cinematic);
			uiInfo.teamList[i].cinematic = -1;
		}
    i += select;
    if (i >= uiInfo.teamCount) {
      i = 0;
    } else if (i < 0) {
			i = uiInfo.teamCount - 1;
		}
  	trap_Cvar_Set( "ui_teamName", uiInfo.teamList[i].teamName);
	UI_HeadCountByTeam();
	UI_FeederSelection(FEEDER_HEADS, 0);
	updateModel = qtrue;
    return qtrue;
  }
  return qfalse;
}

static qboolean UI_GameType_HandleKey(int flags, float *special, int key, qboolean resetMap) {
  int select = UI_SelectForKey(key);
  if (select != 0) {
		int oldCount = UI_MapCountByGameType(qtrue);

		// hard coded mess here
		if (select < 0) {
			ui_gameType.integer--;
			if (ui_gameType.integer == 2) {
				ui_gameType.integer = 1;
			} else if (ui_gameType.integer < 2) {
				ui_gameType.integer = uiInfo.numGameTypes - 1;
			}
		} else {
			ui_gameType.integer++;
			if (ui_gameType.integer >= uiInfo.numGameTypes) {
				ui_gameType.integer = 1;
			} else if (ui_gameType.integer == 2) {
				ui_gameType.integer = 3;
			}
		}
    
		if (uiInfo.gameTypes[ui_gameType.integer].gtEnum == GT_TOURNAMENT) {
			trap_Cvar_Set("ui_Q3Model", "1");
		} else {
			trap_Cvar_Set("ui_Q3Model", "0");
		}

		trap_Cvar_Set("ui_gameType", va("%d", ui_gameType.integer));
		UI_SetCapFragLimits(qtrue);
		UI_LoadBestScores(uiInfo.mapList[ui_currentMap.integer].mapLoadName, uiInfo.gameTypes[ui_gameType.integer].gtEnum);
		if (resetMap && oldCount != UI_MapCountByGameType(qtrue)) {
	  	trap_Cvar_Set( "ui_currentMap", "0");
			Menu_SetFeederSelection(NULL, FEEDER_MAPS, 0, NULL);
		}
    return qtrue;
  }
  return qfalse;
}

static qboolean UI_NetGameType_HandleKey(int flags, float *special, int key) {
  int select = UI_SelectForKey(key);
  if (select != 0) {
    ui_netGameType.integer += select;

    if (ui_netGameType.integer < 0) {
      ui_netGameType.integer = uiInfo.numGameTypes - 1;
		} else if (ui_netGameType.integer >= uiInfo.numGameTypes) {
      ui_netGameType.integer = 0;
    } 

  	trap_Cvar_Set( "ui_netGameType", va("%d", ui_netGameType.integer));
  	trap_Cvar_Set( "ui_actualnetGameType", va("%d", uiInfo.gameTypes[ui_netGameType.integer].gtEnum));
  	trap_Cvar_Set( "ui_currentNetMap", "0");
		UI_MapCountByGameType(qfalse);
		Menu_SetFeederSelection(NULL, FEEDER_ALLMAPS, 0, NULL);
    return qtrue;
  }
  return qfalse;
}

static qboolean UI_JoinGameType_HandleKey(int flags, float *special, int key) {
	int select = UI_SelectForKey(key);
	if (select != 0) {
		ui_joinGameType.integer += select;

		if (ui_joinGameType.integer < 0) {
			ui_joinGameType.integer = uiInfo.numJoinGameTypes - 1;
		} else if (ui_joinGameType.integer >= uiInfo.numJoinGameTypes) {
			ui_joinGameType.integer = 0;
		}

		trap_Cvar_Set( "ui_joinGameType", va("%d", ui_joinGameType.integer));
		UI_BuildServerDisplayList(qtrue);
		return qtrue;
	}
	return qfalse;
}



static qboolean UI_Skill_HandleKey(int flags, float *special, int key) {
  int select = UI_SelectForKey(key);
  if (select != 0) {
  	int i = trap_Cvar_VariableValue( "g_spSkill" );

    i += select;

    if (i < 1) {
			i = numSkillLevels;
		} else if (i > numSkillLevels) {
      i = 1;
    }

    trap_Cvar_Set("g_spSkill", va("%i", i));
    return qtrue;
  }
  return qfalse;
}

static qboolean UI_TeamName_HandleKey(int flags, float *special, int key, qboolean blue) {
  int select = UI_SelectForKey(key);
  if (select != 0) {
    int i;
    i = UI_TeamIndexFromName(UI_Cvar_VariableString((blue) ? "ui_blueTeam" : "ui_redTeam"));

    i += select;

    if (i >= uiInfo.teamCount) {
      i = 0;
    } else if (i < 0) {
			i = uiInfo.teamCount - 1;
		}

    trap_Cvar_Set( (blue) ? "ui_blueTeam" : "ui_redTeam", uiInfo.teamList[i].teamName);

    return qtrue;
  }
  return qfalse;
}

static qboolean UI_TeamMember_HandleKey(int flags, float *special, int key, qboolean blue, int num) {
  int select = UI_SelectForKey(key);
  if (select != 0) {
		// 0 - None
		// 1 - Human
		// 2..NumCharacters - Bot
		char *cvar = va(blue ? "ui_blueteam%i" : "ui_redteam%i", num);
		int value = trap_Cvar_VariableValue(cvar);

		value += select;

		if (ui_actualNetGameType.integer >= GT_TEAM) {
			int count = (UI_ClanForTeamSlot(blue) >= 0) ? TEAM_MEMBERS : uiInfo.characterCount;
			if (value >= count + 2) {
				value = 0;
			} else if (value < 0) {
				value = count + 2 - 1;
			}
		} else {
			if (value >= UI_GetNumBots() + 2) {
				value = 0;
			} else if (value < 0) {
				value = UI_GetNumBots() + 2 - 1;
			}
		}

		trap_Cvar_Set(cvar, va("%i", value));
    return qtrue;
  }
  return qfalse;
}

static qboolean UI_NetSource_HandleKey(int flags, float *special, int key) {
  int select = UI_SelectForKey(key);
  if (select != 0) {
    ui_netSource.integer += select;

		if (ui_netSource.integer >= uiNumNetSources) {
      ui_netSource.integer = 0;
    } else if (ui_netSource.integer < 0) {
      ui_netSource.integer = uiNumNetSources - 1;
		}

		// tab switches render the tab's snapshot; only an empty snapshot (or
		// an explicit refresh) queries the network
		UI_StopServerRefresh();
		UI_BuildServerDisplayList(qtrue);
		if (uiNumSourceServers[UI_ActiveNetSource()] == 0) {
			UI_StartServerRefresh(qtrue);
		}
  	trap_Cvar_Set( "ui_netSource", va("%d", ui_netSource.integer));
    return qtrue;
  }
  return qfalse;
}

static qboolean UI_NetFilter_HandleKey(int flags, float *special, int key) {
  int select = UI_SelectForKey(key);
  if (select != 0) {
    ui_serverFilterType.integer += select;

		if (ui_serverFilterType.integer >= numServerFilters) {
			ui_serverFilterType.integer = 0;
		} else if (ui_serverFilterType.integer < 0) {
			ui_serverFilterType.integer = numServerFilters - 1;
		}
		UI_BuildServerDisplayList(qtrue);
    return qtrue;
  }
  return qfalse;
}

static qboolean UI_OpponentName_HandleKey(int flags, float *special, int key) {
  int select = UI_SelectForKey(key);
  if (select != 0) {
		if (select < 0) {
			UI_PriorOpponent();
		} else {
			UI_NextOpponent();
		}
    return qtrue;
  }
  return qfalse;
}

static qboolean UI_BotName_HandleKey(int flags, float *special, int key) {
  int select = UI_SelectForKey(key);
  if (select != 0) {
		int game = trap_Cvar_VariableValue("g_gametype");
		int value = uiInfo.botIndex;

		value += select;

		if (game >= GT_TEAM) {
			if (value >= uiInfo.characterCount + 2) {
				value = 0;
			} else if (value < 0) {
				value = uiInfo.characterCount + 2 - 1;
			}
		} else {
			if (value >= UI_GetNumBots() + 2) {
				value = 0;
			} else if (value < 0) {
				value = UI_GetNumBots() + 2 - 1;
			}
		}
		uiInfo.botIndex = value;
    return qtrue;
  }
  return qfalse;
}

static qboolean UI_BotSkill_HandleKey(int flags, float *special, int key) {
  int select = UI_SelectForKey(key);
  if (select != 0) {
    uiInfo.skillIndex += select;
		if (uiInfo.skillIndex >= numSkillLevels) {
			uiInfo.skillIndex = 0;
		} else if (uiInfo.skillIndex < 0) {
			uiInfo.skillIndex = numSkillLevels-1;
		}
    return qtrue;
  }
	return qfalse;
}

static qboolean UI_RedBlue_HandleKey(int flags, float *special, int key) {
  int select = UI_SelectForKey(key);
  if (select != 0) {
		uiInfo.redBlue ^= 1;
		return qtrue;
	}
	return qfalse;
}

static qboolean UI_Crosshair_HandleKey(int flags, float *special, int key) {
  int select = UI_SelectForKey(key);
  if (select != 0) {
    uiInfo.currentCrosshair += select;

		if (uiInfo.currentCrosshair >= NUM_CROSSHAIRS) {
			uiInfo.currentCrosshair = 0;
		} else if (uiInfo.currentCrosshair < 0) {
			uiInfo.currentCrosshair = NUM_CROSSHAIRS - 1;
		}
		trap_Cvar_Set("cg_drawCrosshair", va("%d", uiInfo.currentCrosshair));
		return qtrue;
	}
	return qfalse;
}

static qboolean UI_CrosshairColor_HandleKey(int flags, float *special, int key) {
	int select = UI_SelectForKey(key);
	if (select != 0) {
		int currentColor;
		int uiIndex;

		currentColor = (int)trap_Cvar_VariableValue("cg_crosshairColor");
		if (currentColor < 1 || currentColor > 7) {
			currentColor = 7;
		}
		uiIndex = gamecodetoui[currentColor - 1];

		uiIndex += select;

		if (uiIndex > 6) {
			uiIndex = 0;
		} else if (uiIndex < 0) {
			uiIndex = 6;
		}
		currentColor = uitogamecode[uiIndex];
		trap_Cvar_SetValue("cg_crosshairColor", currentColor);
		return qtrue;
	}
	return qfalse;
}

static qboolean UI_SelectedPlayer_HandleKey(int flags, float *special, int key) {
  int select = UI_SelectForKey(key);
  if (select != 0) {
		int selected;

		UI_BuildPlayerList();
		if (!uiInfo.teamLeader) {
			return qfalse;
		}
		selected = trap_Cvar_VariableValue("cg_selectedPlayer");

		selected += select;

		if (selected > uiInfo.myTeamCount) {
			selected = 0;
		} else if (selected < 0) {
			selected = uiInfo.myTeamCount;
		}

		if (selected == uiInfo.myTeamCount) {
		 	trap_Cvar_Set( "cg_selectedPlayerName", "Everyone");
		} else {
		 	trap_Cvar_Set( "cg_selectedPlayerName", uiInfo.teamNames[selected]);
		}
	 	trap_Cvar_Set( "cg_selectedPlayer", va("%d", selected));
	}
	return qfalse;
}


static qboolean UI_OwnerDrawHandleKey(int ownerDraw, int flags, float *special, int key) {
  switch (ownerDraw) {
    case UI_HANDICAP:
      return UI_Handicap_HandleKey(flags, special, key);
      break;
    case UI_EFFECTS:
      return UI_Effects_HandleKey(flags, special, key);
      break;
    case UI_DYNAMICLIGHTS:
      return UI_DynamicLights_HandleKey(flags, special, key);
      break;
    case UI_CLANNAME:
      return UI_ClanName_HandleKey(flags, special, key);
      break;
    case UI_GAMETYPE:
      return UI_GameType_HandleKey(flags, special, key, qtrue);
      break;
    case UI_NETGAMETYPE:
      return UI_NetGameType_HandleKey(flags, special, key);
      break;
    case UI_JOINGAMETYPE:
      return UI_JoinGameType_HandleKey(flags, special, key);
      break;
    case UI_SKILL:
      return UI_Skill_HandleKey(flags, special, key);
      break;
    case UI_BLUETEAMNAME:
      return UI_TeamName_HandleKey(flags, special, key, qtrue);
      break;
    case UI_REDTEAMNAME:
      return UI_TeamName_HandleKey(flags, special, key, qfalse);
      break;
    case UI_BLUETEAM1:
		case UI_BLUETEAM2:
		case UI_BLUETEAM3:
		case UI_BLUETEAM4:
		case UI_BLUETEAM5:
      UI_TeamMember_HandleKey(flags, special, key, qtrue, ownerDraw - UI_BLUETEAM1 + 1);
      break;
    case UI_REDTEAM1:
		case UI_REDTEAM2:
		case UI_REDTEAM3:
		case UI_REDTEAM4:
		case UI_REDTEAM5:
      UI_TeamMember_HandleKey(flags, special, key, qfalse, ownerDraw - UI_REDTEAM1 + 1);
      break;
		case UI_NETSOURCE:
      UI_NetSource_HandleKey(flags, special, key);
			break;
		case UI_NETFILTER:
      UI_NetFilter_HandleKey(flags, special, key);
			break;
		case UI_OPPONENT_NAME:
			UI_OpponentName_HandleKey(flags, special, key);
			break;
		case UI_BOTNAME:
			return UI_BotName_HandleKey(flags, special, key);
			break;
		case UI_BOTSKILL:
			return UI_BotSkill_HandleKey(flags, special, key);
			break;
		case UI_REDBLUE:
			UI_RedBlue_HandleKey(flags, special, key);
			break;
		case UI_CROSSHAIR:
			UI_Crosshair_HandleKey(flags, special, key);
			break;
		case UI_CROSSHAIRCOLOR:
			UI_CrosshairColor_HandleKey(flags, special, key);
			break;
		case UI_SELECTEDPLAYER:
			UI_SelectedPlayer_HandleKey(flags, special, key);
			break;
    default:
      break;
  }

  return qfalse;
}


static float UI_GetValue(int ownerDraw) {
  return 0;
}

/*
=================
UI_CompareServerRows

Snapshot-row ordering for the current sort key/direction; mirrors the
engine's LAN_CompareServers so sorting behaves the same as it always did.
=================
*/
static int UI_CompareServerRows( const uiServerRow_t *r1, const uiServerRow_t *r2 ) {
	int		res;
	int		c1, c2;

	res = 0;
	switch ( uiInfo.serverStatus.sortKey ) {
	case SORT_HOST:
		res = Q_stricmp( r1->hostname, r2->hostname );
		break;
	case SORT_MAP:
		res = Q_stricmp( r1->mapname, r2->mapname );
		break;
	case SORT_CLIENTS:
		c1 = ui_browserExcludeBots.integer ? r1->humanClients : r1->clients;
		c2 = ui_browserExcludeBots.integer ? r2->humanClients : r2->clients;
		if ( c1 < c2 ) {
			res = -1;
		} else if ( c1 > c2 ) {
			res = 1;
		}
		break;
	case SORT_GAME:
		if ( r1->gameType < r2->gameType ) {
			res = -1;
		} else if ( r1->gameType > r2->gameType ) {
			res = 1;
		}
		break;
	case SORT_PING:
		if ( r1->ping < r2->ping ) {
			res = -1;
		} else if ( r1->ping > r2->ping ) {
			res = 1;
		}
		break;
	}

	if ( res < 0 ) {
		res = -1;
	} else if ( res > 0 ) {
		res = 1;
	}

	if ( uiInfo.serverStatus.sortDir ) {
		return -res;
	}
	return res;
}

/*
=================
UI_ServersQsortCompare
=================
*/
static int QDECL UI_ServersQsortCompare( const void *arg1, const void *arg2 ) {
	int		tab = UI_ActiveNetSource();

	return UI_CompareServerRows( &uiSourceServers[tab][*(int*)arg1], &uiSourceServers[tab][*(int*)arg2] );
}


/*
=================
UI_ServersSort
=================
*/
void UI_ServersSort(int column, qboolean force) {

	if ( !force ) {
		if ( uiInfo.serverStatus.sortKey == column ) {
			return;
		}
	}

	uiInfo.serverStatus.sortKey = column;
	qsort( &uiInfo.serverStatus.displayServers[0], uiInfo.serverStatus.numDisplayServers, sizeof(int), UI_ServersQsortCompare);
}

/*
static void UI_StartSinglePlayer() {
	int i,j, k, skill;
	char buff[1024];
	i = trap_Cvar_VariableValue( "ui_currentTier" );
  if (i < 0 || i >= tierCount) {
    i = 0;
  }
	j = trap_Cvar_VariableValue("ui_currentMap");
	if (j < 0 || j > MAPS_PER_TIER) {
		j = 0;
	}

 	trap_Cvar_SetValue( "singleplayer", 1 );
 	trap_Cvar_SetValue( "g_gametype", Com_Clamp( 0, 7, tierList[i].gameTypes[j] ) );
	trap_Cmd_ExecuteText( EXEC_APPEND, va( "wait ; wait ; map %s\n", tierList[i].maps[j] ) );
	skill = trap_Cvar_VariableValue( "g_spSkill" );

	if (j == MAPS_PER_TIER-1) {
		k = UI_TeamIndexFromName(UI_Cvar_VariableString("ui_opponentName"));
		Com_sprintf( buff, sizeof(buff), "wait ; addbot %s %i %s 250 %s\n", UI_AIFromName(teamList[k].teamMembers[0]), skill, "", teamList[k].teamMembers[0]);
	} else {
		k = UI_TeamIndexFromName(UI_Cvar_VariableString("ui_opponentName"));
		for (i = 0; i < PLAYERS_PER_TEAM; i++) {
			Com_sprintf( buff, sizeof(buff), "wait ; addbot %s %i %s 250 %s\n", UI_AIFromName(teamList[k].teamMembers[i]), skill, "Blue", teamList[k].teamMembers[i]);
			trap_Cmd_ExecuteText( EXEC_APPEND, buff );
		}

		k = UI_TeamIndexFromName(UI_Cvar_VariableString("ui_teamName"));
		for (i = 1; i < PLAYERS_PER_TEAM; i++) {
			Com_sprintf( buff, sizeof(buff), "wait ; addbot %s %i %s 250 %s\n", UI_AIFromName(teamList[k].teamMembers[i]), skill, "Red", teamList[k].teamMembers[i]);
			trap_Cmd_ExecuteText( EXEC_APPEND, buff );
		}
		trap_Cmd_ExecuteText( EXEC_APPEND, "wait 5; team Red\n" );
	}
	

}
*/

/*
===============
UI_LoadMods
===============
*/
static void UI_LoadMods() {
	int		numdirs;
	char	dirlist[2048];
	char	*dirptr;
  char  *descptr;
	int		i;
	int		dirlen;

	uiInfo.modCount = 0;
	numdirs = trap_FS_GetFileList( "$modlist", "", dirlist, sizeof(dirlist) );
	dirptr  = dirlist;
	for( i = 0; i < numdirs; i++ ) {
		dirlen = strlen( dirptr ) + 1;
    descptr = dirptr + dirlen;
		uiInfo.modList[uiInfo.modCount].modName = String_Alloc(dirptr);
		uiInfo.modList[uiInfo.modCount].modDescr = String_Alloc(descptr);
    dirptr += dirlen + strlen(descptr) + 1;
		uiInfo.modCount++;
		if (uiInfo.modCount >= MAX_MODS) {
			break;
		}
	}

}


/*
===============
UI_LoadTeams
===============
*/
static void UI_LoadTeams() {
	char	teamList[4096];
	char	*teamName;
	int		i, len, count;

	count = trap_FS_GetFileList( "", "team", teamList, 4096 );

	if (count) {
		teamName = teamList;
		for ( i = 0; i < count; i++ ) {
			len = strlen( teamName );
			UI_ParseTeamInfo(teamName);
			teamName += len + 1;
		}
	}

}


/*
===============
UI_LoadMovies
===============
*/
static void UI_LoadMovies() {
	char	movielist[4096];
	char	*moviename;
	int		i, len;

	uiInfo.movieCount = trap_FS_GetFileList( "video", "roq", movielist, 4096 );

	if (uiInfo.movieCount) {
		if (uiInfo.movieCount > MAX_MOVIES) {
			uiInfo.movieCount = MAX_MOVIES;
		}
		moviename = movielist;
		for ( i = 0; i < uiInfo.movieCount; i++ ) {
			len = strlen( moviename );
			if (!Q_stricmp(moviename +  len - 4,".roq")) {
				moviename[len-4] = '\0';
			}
			Q_strupr(moviename);
			uiInfo.movieList[i] = String_Alloc(moviename);
			moviename += len + 1;
		}
	}

}



/*
===============
UI_LoadDemos
===============
*/
static void UI_LoadDemos() {
	char	demolist[4096];
	char demoExt[32];
	char	*demoname;
	int		i, len, count, bufUsed;
	int		protocol, protocolLegacy;

	protocolLegacy = trap_Cvar_VariableValue("com_legacyprotocol");
	protocol = trap_Cvar_VariableValue("com_protocol");

	if(!protocol)
		protocol = trap_Cvar_VariableValue("protocol");
	if(protocolLegacy == protocol)
		protocolLegacy = 0;

	Com_sprintf(demoExt, sizeof(demoExt), "dm_%d", protocol);

	uiInfo.demoCount = trap_FS_GetFileList( "demos", demoExt, demolist, 4096 );
	if (uiInfo.demoCount > MAX_DEMOS) {
		uiInfo.demoCount = MAX_DEMOS;
	}

	bufUsed = 0;
	demoname = demolist;
	for ( i = 0; i < uiInfo.demoCount; i++ ) {
		len = strlen( demoname ) + 1;
		bufUsed += len;
		demoname += len;
	}

	// append legacy-protocol demos to the same list
	if ( protocolLegacy > 0 && uiInfo.demoCount < MAX_DEMOS && bufUsed < 4096 ) {
		Com_sprintf(demoExt, sizeof(demoExt), "dm_%d", protocolLegacy);
		count = trap_FS_GetFileList( "demos", demoExt, demolist + bufUsed, 4096 - bufUsed );
		if ( uiInfo.demoCount + count > MAX_DEMOS )
			count = MAX_DEMOS - uiInfo.demoCount;
		uiInfo.demoCount += count;
		for ( i = 0; i < count; i++ ) {
			len = strlen( demoname ) + 1;
			bufUsed += len;
			demoname += len;
		}
	}

	// append TV demos to the same list
	if ( uiInfo.demoCount < MAX_DEMOS && bufUsed < 4096 ) {
		count = trap_FS_GetFileList( "demos", "tvd", demolist + bufUsed, 4096 - bufUsed );
		if ( uiInfo.demoCount + count > MAX_DEMOS )
			count = MAX_DEMOS - uiInfo.demoCount;
		uiInfo.demoCount += count;
	}

	if (uiInfo.demoCount) {
		demoname = demolist;
		for ( i = 0; i < uiInfo.demoCount; i++ ) {
			len = strlen( demoname );
			// strip .dm_XX extension
			if ( len > 5 && !Q_stricmpn( demoname + len - 6, ".dm_", 4 ) ) {
				demoname[len-6] = '\0';
			}
			Q_strupr(demoname);
			uiInfo.demoList[i] = String_Alloc(demoname);
			demoname += len + 1;
		}
	}

}


static qboolean UI_SetNextMap(int actual, int index) {
	int i;
	for (i = actual + 1; i < uiInfo.mapCount; i++) {
		if (uiInfo.mapList[i].active) {
			Menu_SetFeederSelection(NULL, FEEDER_MAPS, index + 1, "skirmish");
			return qtrue;
		}
	}
	return qfalse;
}


static void UI_StartSkirmish(qboolean next) {
	int i, k, g, delay, temp;
	float skill;
	char buff[MAX_STRING_CHARS];

	if (next) {
		int actual;
		int index = trap_Cvar_VariableValue("ui_mapIndex");
	 	UI_MapCountByGameType(qtrue);
		UI_SelectedMap(index, &actual);
		if (UI_SetNextMap(actual, index)) {
		} else {
			UI_GameType_HandleKey(0, 0, K_MOUSE1, qfalse);
			UI_MapCountByGameType(qtrue);
			Menu_SetFeederSelection(NULL, FEEDER_MAPS, 0, "skirmish");
		}
	}

	g = uiInfo.gameTypes[ui_gameType.integer].gtEnum;
	trap_Cvar_SetValue( "g_gametype", g );
	trap_Cvar_SetValue( "g_mode", Com_Clamp( 0, MODE_COUNT - 1, ui_mode.integer ) );
	trap_Cvar_SetValue( "g_grapple", ui_grapple.integer ? 1 : 0 );
	trap_Cmd_ExecuteText( EXEC_APPEND, va( "wait ; wait ; map %s\n", uiInfo.mapList[ui_currentMap.integer].mapLoadName) );
	skill = trap_Cvar_VariableValue( "g_spSkill" );
	trap_Cvar_Set("ui_scoreMap", uiInfo.mapList[ui_currentMap.integer].mapName);

	k = UI_TeamIndexFromName(UI_Cvar_VariableString("ui_opponentName"));

	trap_Cvar_Set("ui_singlePlayerActive", "1");

	// set up sp overrides, will be replaced on postgame
	temp = trap_Cvar_VariableValue( "capturelimit" );
	trap_Cvar_Set("ui_saveCaptureLimit", va("%i", temp));
	temp = trap_Cvar_VariableValue( "fraglimit" );
	trap_Cvar_Set("ui_saveFragLimit", va("%i", temp));

	UI_SetCapFragLimits(qfalse);

	temp = trap_Cvar_VariableValue( "cg_drawTimer" );
	trap_Cvar_Set("ui_drawTimer", va("%i", temp));
	temp = trap_Cvar_VariableValue( "g_doWarmup" );
	trap_Cvar_Set("ui_doWarmup", va("%i", temp));
	temp = trap_Cvar_VariableValue( "g_friendlyFire" );
	trap_Cvar_Set("ui_friendlyFire", va("%i", temp));
	temp = trap_Cvar_VariableValue( "sv_maxClients" );
	trap_Cvar_Set("ui_maxClients", va("%i", temp));
	temp = trap_Cvar_VariableValue( "g_warmup" );
	trap_Cvar_Set("ui_Warmup", va("%i", temp));
	temp = trap_Cvar_VariableValue( "sv_pure" );
	trap_Cvar_Set("ui_pure", va("%i", temp));

	trap_Cvar_Set("cg_cameraOrbit", "0");
	trap_Cvar_Set("cg_thirdPerson", "0");
	trap_Cvar_Set("cg_drawTimer", "1");
	trap_Cvar_Set("g_doWarmup", "1");
	trap_Cvar_Set("g_warmup", "15");
	trap_Cvar_Set("sv_pure", "0");
	trap_Cvar_Set("g_friendlyFire", "0");
	trap_Cvar_Set("g_redTeam", UI_Cvar_VariableString("ui_teamName"));
	trap_Cvar_Set("g_blueTeam", UI_Cvar_VariableString("ui_opponentName"));

	if (trap_Cvar_VariableValue("ui_recordSPDemo")) {
		Com_sprintf(buff, MAX_STRING_CHARS, "%s_%i", uiInfo.mapList[ui_currentMap.integer].mapLoadName, g);
		trap_Cvar_Set("ui_recordSPDemoName", buff);
	}

	delay = 500;

	if (g == GT_TOURNAMENT) {
		trap_Cvar_Set("sv_maxClients", "2");
		Com_sprintf( buff, sizeof(buff), "wait ; addbot %s %f "", %i \n", uiInfo.mapList[ui_currentMap.integer].opponentName, skill, delay);
		trap_Cmd_ExecuteText( EXEC_APPEND, buff );
	} else {
		temp = uiInfo.mapList[ui_currentMap.integer].teamMembers * 2;
		trap_Cvar_Set("sv_maxClients", va("%d", temp));
		for (i =0; i < uiInfo.mapList[ui_currentMap.integer].teamMembers; i++) {
			Com_sprintf( buff, sizeof(buff), "addbot %s %f %s %i %s\n", UI_AIFromName(uiInfo.teamList[k].teamMembers[i]), skill, (g == GT_FFA) ? "" : "Blue", delay, uiInfo.teamList[k].teamMembers[i]);
			trap_Cmd_ExecuteText( EXEC_APPEND, buff );
			delay += 500;
		}
		k = UI_TeamIndexFromName(UI_Cvar_VariableString("ui_teamName"));
		for (i =0; i < uiInfo.mapList[ui_currentMap.integer].teamMembers-1; i++) {
			Com_sprintf( buff, sizeof(buff), "addbot %s %f %s %i %s\n", UI_AIFromName(uiInfo.teamList[k].teamMembers[i]), skill, (g == GT_FFA) ? "" : "Red", delay, uiInfo.teamList[k].teamMembers[i]);
			trap_Cmd_ExecuteText( EXEC_APPEND, buff );
			delay += 500;
		}
	}
	if (g >= GT_TEAM ) {
		trap_Cmd_ExecuteText( EXEC_APPEND, "wait 5; team Red\n" );
	}
}

static void UI_Update(const char *name) {
	int	val = trap_Cvar_VariableValue(name);

 	if (Q_stricmp(name, "ui_SetName") == 0) {
		trap_Cvar_Set( "name", UI_Cvar_VariableString("ui_Name"));
 	} else if (Q_stricmp(name, "ui_setRate") == 0) {
		float rate = trap_Cvar_VariableValue("rate");
		if (rate >= 5000) {
#if 0
			trap_Cvar_Set("cl_maxpackets", "30");
#endif
			trap_Cvar_Set("cl_packetdup", "1");
		} else if (rate >= 4000) {
#if 0
			trap_Cvar_Set("cl_maxpackets", "15");
#endif
			trap_Cvar_Set("cl_packetdup", "2");		// favor less prediction errors when there's packet loss
		} else {
#if 0
			trap_Cvar_Set("cl_maxpackets", "15");
#endif
			trap_Cvar_Set("cl_packetdup", "1");		// favor lower bandwidth
		}
 	} else if (Q_stricmp(name, "ui_GetName") == 0) {
		trap_Cvar_Set( "ui_Name", UI_Cvar_VariableString("name"));
 	} else if (Q_stricmp(name, "r_colorbits") == 0) {
		switch (val) {
			case 0:
				trap_Cvar_SetValue( "r_depthbits", 0 );
				trap_Cvar_SetValue( "r_stencilbits", 0 );
			break;
			case 16:
				trap_Cvar_SetValue( "r_depthbits", 16 );
				trap_Cvar_SetValue( "r_stencilbits", 0 );
			break;
			case 32:
				trap_Cvar_SetValue( "r_depthbits", 24 );
			break;
		}
	} else if (Q_stricmp(name, "ui_glCustom") == 0) {
		// the preset tuples split on the VR mirror: VR-tuned values under
		// VR, the stock tables on a flatscreen engine
		if (vrActive) {
			switch (val) {
				case 0:	// high quality
					trap_Cvar_SetValue( "r_fullScreen", 1 );
					trap_Cvar_SetValue( "r_subdivisions", 1 );
					trap_Cvar_SetValue( "r_vertexlight", 0 );
					trap_Cvar_SetValue( "r_lodbias", -2 );
					trap_Cvar_SetValue( "r_colorbits", 32 );
					trap_Cvar_SetValue( "r_depthbits", 24 );
					trap_Cvar_SetValue( "r_stencilbits", 8 );
					trap_Cvar_SetValue( "r_picmip", 0 );
					trap_Cvar_SetValue( "r_mode", 4 );
					trap_Cvar_SetValue( "r_texturebits", 32 );
					trap_Cvar_SetValue( "r_fastSky", 0 );
					trap_Cvar_SetValue( "r_inGameVideo", 1 );
					trap_Cvar_SetValue( "cg_shadows", 1 );
					trap_Cvar_SetValue( "cg_brassTime", 2500 );
					trap_Cvar_Set( "r_texturemode", "GL_LINEAR_MIPMAP_LINEAR" );
				break;
				case 1: // normal
					trap_Cvar_SetValue( "r_fullScreen", 1 );
					trap_Cvar_SetValue( "r_subdivisions", 2 );
					trap_Cvar_SetValue( "r_vertexlight", 0 );
					trap_Cvar_SetValue( "r_lodbias", -1 );
					trap_Cvar_SetValue( "r_colorbits", 0 );
					trap_Cvar_SetValue( "r_depthbits", 0 );
					trap_Cvar_Reset( "r_stencilbits" );
					trap_Cvar_SetValue( "r_picmip", 1 );
					trap_Cvar_SetValue( "r_mode", 3 );
					trap_Cvar_SetValue( "r_texturebits", 0 );
					trap_Cvar_SetValue( "r_fastSky", 0 );
					trap_Cvar_SetValue( "r_inGameVideo", 1 );
					trap_Cvar_SetValue( "cg_brassTime", 2500 );
					trap_Cvar_Set( "r_texturemode", "GL_LINEAR_MIPMAP_LINEAR" );
					trap_Cvar_SetValue( "cg_shadows", 0 );
				break;
				case 2: // fast
					trap_Cvar_SetValue( "r_fullScreen", 1 );
					trap_Cvar_SetValue( "r_subdivisions", 4 );
					trap_Cvar_SetValue( "r_vertexlight", 0 );
					trap_Cvar_SetValue( "r_lodbias", 0 );
					trap_Cvar_SetValue( "r_colorbits", 0 );
					trap_Cvar_SetValue( "r_depthbits", 0 );
					trap_Cvar_Reset( "r_stencilbits" );
					trap_Cvar_SetValue( "r_picmip", 1 );
					trap_Cvar_SetValue( "r_mode", 3 );
					trap_Cvar_SetValue( "r_texturebits", 0 );
					trap_Cvar_SetValue( "cg_shadows", 0 );
					trap_Cvar_SetValue( "r_fastSky", 1 );
					trap_Cvar_SetValue( "r_inGameVideo", 0 );
					trap_Cvar_SetValue( "cg_brassTime", 0 );
					trap_Cvar_Set( "r_texturemode", "GL_LINEAR_MIPMAP_NEAREST" );
				break;
				case 3: // fastest
					trap_Cvar_SetValue( "r_fullScreen", 1 );
					trap_Cvar_SetValue( "r_subdivisions", 12 );
					trap_Cvar_SetValue( "r_vertexlight", 1 );
					trap_Cvar_SetValue( "r_lodbias", 1 );
					trap_Cvar_SetValue( "r_colorbits", 16 );
					trap_Cvar_SetValue( "r_depthbits", 16 );
					trap_Cvar_SetValue( "r_stencilbits", 0 );
					trap_Cvar_SetValue( "r_mode", 3 );
					trap_Cvar_SetValue( "r_picmip", 2 );
					trap_Cvar_SetValue( "r_texturebits", 16 );
					trap_Cvar_SetValue( "cg_shadows", 0 );
					trap_Cvar_SetValue( "cg_brassTime", 0 );
					trap_Cvar_SetValue( "r_fastSky", 1 );
					trap_Cvar_SetValue( "r_inGameVideo", 0 );
					trap_Cvar_Set( "r_texturemode", "GL_LINEAR_MIPMAP_NEAREST" );
				break;
			}
		} else {
			switch (val) {
				case 0:	// high quality
					trap_Cvar_SetValue( "r_fullScreen", 1 );
					trap_Cvar_SetValue( "r_subdivisions", 4 );
					trap_Cvar_SetValue( "r_vertexlight", 0 );
					trap_Cvar_SetValue( "r_lodbias", 0 );
					trap_Cvar_SetValue( "r_colorbits", 32 );
					trap_Cvar_SetValue( "r_depthbits", 24 );
					trap_Cvar_SetValue( "r_picmip", 0 );
					trap_Cvar_SetValue( "r_mode", 4 );
					trap_Cvar_SetValue( "r_texturebits", 32 );
					trap_Cvar_SetValue( "r_fastSky", 0 );
					trap_Cvar_SetValue( "r_inGameVideo", 1 );
					trap_Cvar_SetValue( "cg_shadows", 1 );
					trap_Cvar_SetValue( "cg_brassTime", 2500 );
					trap_Cvar_Set( "r_texturemode", "GL_LINEAR_MIPMAP_LINEAR" );
				break;
				case 1: // normal
					trap_Cvar_SetValue( "r_fullScreen", 1 );
					trap_Cvar_SetValue( "r_subdivisions", 12 );
					trap_Cvar_SetValue( "r_vertexlight", 0 );
					trap_Cvar_SetValue( "r_lodbias", 0 );
					trap_Cvar_SetValue( "r_colorbits", 0 );
					trap_Cvar_SetValue( "r_depthbits", 24 );
					trap_Cvar_SetValue( "r_picmip", 1 );
					trap_Cvar_SetValue( "r_mode", 3 );
					trap_Cvar_SetValue( "r_texturebits", 0 );
					trap_Cvar_SetValue( "r_fastSky", 0 );
					trap_Cvar_SetValue( "r_inGameVideo", 1 );
					trap_Cvar_SetValue( "cg_brassTime", 2500 );
					trap_Cvar_Set( "r_texturemode", "GL_LINEAR_MIPMAP_LINEAR" );
					trap_Cvar_SetValue( "cg_shadows", 0 );
				break;
				case 2: // fast
					trap_Cvar_SetValue( "r_fullScreen", 1 );
					trap_Cvar_SetValue( "r_subdivisions", 8 );
					trap_Cvar_SetValue( "r_vertexlight", 0 );
					trap_Cvar_SetValue( "r_lodbias", 1 );
					trap_Cvar_SetValue( "r_colorbits", 0 );
					trap_Cvar_SetValue( "r_depthbits", 0 );
					trap_Cvar_SetValue( "r_picmip", 1 );
					trap_Cvar_SetValue( "r_mode", 3 );
					trap_Cvar_SetValue( "r_texturebits", 0 );
					trap_Cvar_SetValue( "cg_shadows", 0 );
					trap_Cvar_SetValue( "r_fastSky", 1 );
					trap_Cvar_SetValue( "r_inGameVideo", 0 );
					trap_Cvar_SetValue( "cg_brassTime", 0 );
					trap_Cvar_Set( "r_texturemode", "GL_LINEAR_MIPMAP_NEAREST" );
				break;
				case 3: // fastest
					trap_Cvar_SetValue( "r_fullScreen", 1 );
					trap_Cvar_SetValue( "r_subdivisions", 20 );
					trap_Cvar_SetValue( "r_vertexlight", 1 );
					trap_Cvar_SetValue( "r_lodbias", 2 );
					trap_Cvar_SetValue( "r_colorbits", 16 );
					trap_Cvar_SetValue( "r_depthbits", 16 );
					trap_Cvar_SetValue( "r_mode", 3 );
					trap_Cvar_SetValue( "r_picmip", 2 );
					trap_Cvar_SetValue( "r_texturebits", 16 );
					trap_Cvar_SetValue( "cg_shadows", 0 );
					trap_Cvar_SetValue( "cg_brassTime", 0 );
					trap_Cvar_SetValue( "r_fastSky", 1 );
					trap_Cvar_SetValue( "r_inGameVideo", 0 );
					trap_Cvar_Set( "r_texturemode", "GL_LINEAR_MIPMAP_NEAREST" );
				break;
			}
		}
	} else if (Q_stricmp(name, "ui_mousePitch") == 0) {
		if (val == 0) {
			trap_Cvar_SetValue( "m_pitch", 0.022f );
		} else {
			trap_Cvar_SetValue( "m_pitch", -0.022f );
		}
	} else if ( UI_VR_UpdateSettingsCvar( name, val ) ) {
		// handled
	}
}

static void UI_UpdateHDRAvail( void ) {
	char buf[64];
	qboolean avail;
	trap_Cvar_VariableStringBuffer( "r_hdrDisplay", buf, sizeof( buf ) );
	avail = (qboolean)( buf[0] != '\0' );
	if ( avail ) {
		trap_Cvar_VariableStringBuffer( "cl_renderer", buf, sizeof( buf ) );
		// the single-renderer build never registers cl_renderer, so an empty
		// readback is the compiled-in Vulkan renderer; a present-but-non-vulkan
		// name still fails
		avail = (qboolean)( buf[0] == '\0' || Q_stricmp( buf, "vulkan" ) == 0 );
	}
	trap_Cvar_Set( "ui_hdrAvail", avail ? "1" : "0" );
}

// ROM availability cvar for menu-script cvarTest gating (ui_hdrAvail's
// pattern): reflects the negotiated VR mirror, never a raw archived cvar,
// so a flatscreen engine always reads 0
static void UI_UpdateVRActive( void ) {
	trap_Cvar_Set( "ui_vrActive", vrActive ? "1" : "0" );
}

static void UI_RunMenuScript(char **args) {
	const char *name, *name2;
	char buff[1024];

	if (String_Parse(args, &name)) {
		if (Q_stricmp(name, "StartServer") == 0) {
			int i, clients, oldclients;
			float skill;
			trap_Cvar_Set("cg_thirdPerson", "0");
			trap_Cvar_Set("cg_cameraOrbit", "0");
			trap_Cvar_Set("ui_singlePlayerActive", "0");
			trap_Cvar_SetValue( "dedicated", Com_Clamp( 0, 2, ui_dedicated.integer ) );
			trap_Cvar_SetValue( "g_gametype", Com_Clamp( 0, 8, uiInfo.gameTypes[ui_netGameType.integer].gtEnum ) );
			trap_Cvar_SetValue( "g_mode", Com_Clamp( 0, MODE_COUNT - 1, ui_mode.integer ) );
			trap_Cvar_SetValue( "g_grapple", ui_grapple.integer ? 1 : 0 );
			// ui_redTeam/ui_blueTeam are what the createserver selectors edit;
			// ui_teamName/ui_opponentName (vanilla read those) are SP-skirmish-only
			trap_Cvar_Set("g_redTeam", UI_Cvar_VariableString("ui_redTeam"));
			trap_Cvar_Set("g_blueTeam", UI_Cvar_VariableString("ui_blueTeam"));
			trap_Cmd_ExecuteText( EXEC_APPEND, va( "wait ; wait ; map %s\n", uiInfo.mapList[ui_currentNetMap.integer].mapLoadName ) );
			skill = trap_Cvar_VariableValue( "g_spSkill" );
			// set max clients based on spots
			oldclients = trap_Cvar_VariableValue( "sv_maxClients" );
			clients = 0;
			for (i = 0; i < PLAYERS_PER_TEAM; i++) {
				int bot = trap_Cvar_VariableValue( va("ui_blueteam%i", i+1));
				if (bot >= 0) {
					clients++;
				}
				bot = trap_Cvar_VariableValue( va("ui_redteam%i", i+1));
				if (bot >= 0) {
					clients++;
				}
			}
			if (clients == 0) {
				clients = 8;
			}
			
			if (oldclients > clients) {
				clients = oldclients;
			}

			trap_Cvar_Set("sv_maxClients", va("%d",clients));

			for (i = 0; i < PLAYERS_PER_TEAM; i++) {
				int t;
				int bot = trap_Cvar_VariableValue( va("ui_blueteam%i", i+1));
				if (bot > 1) {
					if (ui_actualNetGameType.integer >= GT_TEAM) {
						t = UI_ClanForTeamSlot( qtrue );
						if (t >= 0) {
							// mirror the draw code's clamp for stale slot values
							Com_sprintf( buff, sizeof(buff), "addbot %s %f %s\n", uiInfo.teamList[t].teamMembers[(bot-2 < TEAM_MEMBERS) ? bot-2 : 0], skill, "Blue");
						} else {
							Com_sprintf( buff, sizeof(buff), "addbot %s %f %s\n", uiInfo.characterList[bot-2].name, skill, "Blue");
						}
					} else {
						Com_sprintf( buff, sizeof(buff), "addbot %s %f \n", UI_GetBotNameByNumber(bot-2), skill);
					}
					trap_Cmd_ExecuteText( EXEC_APPEND, buff );
				}
				bot = trap_Cvar_VariableValue( va("ui_redteam%i", i+1));
				if (bot > 1) {
					if (ui_actualNetGameType.integer >= GT_TEAM) {
						t = UI_ClanForTeamSlot( qfalse );
						if (t >= 0) {
							Com_sprintf( buff, sizeof(buff), "addbot %s %f %s\n", uiInfo.teamList[t].teamMembers[(bot-2 < TEAM_MEMBERS) ? bot-2 : 0], skill, "Red");
						} else {
							Com_sprintf( buff, sizeof(buff), "addbot %s %f %s\n", uiInfo.characterList[bot-2].name, skill, "Red");
						}
					} else {
						Com_sprintf( buff, sizeof(buff), "addbot %s %f \n", UI_GetBotNameByNumber(bot-2), skill);
					}
					trap_Cmd_ExecuteText( EXEC_APPEND, buff );
				}
			}
		} else if (Q_stricmp(name, "updateSPMenu") == 0) {
			UI_SetCapFragLimits(qtrue);
			UI_MapCountByGameType(qtrue);
			ui_mapIndex.integer = UI_GetIndexFromSelection(ui_currentMap.integer);
			trap_Cvar_Set("ui_mapIndex", va("%d", ui_mapIndex.integer));
			Menu_SetFeederSelection(NULL, FEEDER_MAPS, ui_mapIndex.integer, "skirmish");
			UI_GameType_HandleKey(0, 0, K_MOUSE1, qfalse);
			UI_GameType_HandleKey(0, 0, K_MOUSE2, qfalse);
		} else if (Q_stricmp(name, "resetDefaults") == 0) {
			trap_Cmd_ExecuteText( EXEC_APPEND, "exec default.cfg\n");
			trap_Cmd_ExecuteText( EXEC_APPEND, "cvar_restart\n");
			Controls_SetDefaults();
			trap_Cvar_Set("com_introPlayed", "1" );
			trap_Cmd_ExecuteText( EXEC_APPEND, "vid_restart\n" );
		} else if (Q_stricmp(name, "getCDKey") == 0) {
			char out[17];
			trap_GetCDKey(buff, 17);
			trap_Cvar_Set("cdkey1", "");
			trap_Cvar_Set("cdkey2", "");
			trap_Cvar_Set("cdkey3", "");
			trap_Cvar_Set("cdkey4", "");
			if (strlen(buff) == CDKEY_LEN) {
				Q_strncpyz(out, buff, 5);
				trap_Cvar_Set("cdkey1", out);
				Q_strncpyz(out, buff + 4, 5);
				trap_Cvar_Set("cdkey2", out);
				Q_strncpyz(out, buff + 8, 5);
				trap_Cvar_Set("cdkey3", out);
				Q_strncpyz(out, buff + 12, 5);
				trap_Cvar_Set("cdkey4", out);
			}

		} else if (Q_stricmp(name, "verifyCDKey") == 0) {
			buff[0] = '\0';
			Q_strcat(buff, 1024, UI_Cvar_VariableString("cdkey1")); 
			Q_strcat(buff, 1024, UI_Cvar_VariableString("cdkey2")); 
			Q_strcat(buff, 1024, UI_Cvar_VariableString("cdkey3")); 
			Q_strcat(buff, 1024, UI_Cvar_VariableString("cdkey4")); 
			trap_Cvar_Set("cdkey", buff);
			if (trap_VerifyCDKey(buff, UI_Cvar_VariableString("cdkeychecksum"))) {
				trap_Cvar_Set("ui_cdkeyvalid", "CD Key Appears to be valid.");
				trap_SetCDKey(buff);
			} else {
				trap_Cvar_Set("ui_cdkeyvalid", "CD Key does not appear to be valid.");
			}
		} else if (Q_stricmp(name, "loadArenas") == 0) {
			UI_LoadArenas();
			UI_MapCountByGameType(qfalse);
			Menu_SetFeederSelection(NULL, FEEDER_ALLMAPS, 0, "createserver");
		} else if (Q_stricmp(name, "saveControls") == 0) {
			Controls_SetConfig(qtrue);
		} else if (Q_stricmp(name, "loadControls") == 0) {
			Controls_GetConfig();
		} else if (Q_stricmp(name, "clearError") == 0) {
			trap_Cvar_Set("com_errorMessage", "");
		} else if (Q_stricmp(name, "loadGameInfo") == 0) {
#ifdef PRE_RELEASE_TADEMO
			UI_ParseGameInfo("demogameinfo.txt");
#else
			UI_ParseGameInfo("gameinfo.txt");
#endif
			UI_LoadBestScores(uiInfo.mapList[ui_currentMap.integer].mapLoadName, uiInfo.gameTypes[ui_gameType.integer].gtEnum);
		} else if (Q_stricmp(name, "resetScores") == 0) {
			UI_ClearScores();
		} else if (Q_stricmp(name, "RefreshServers") == 0) {
			UI_StartServerRefresh(qtrue);
			UI_BuildServerDisplayList(qtrue);
		} else if (Q_stricmp(name, "RefreshFilter") == 0) {
			UI_StartServerRefresh(qfalse);
			UI_BuildServerDisplayList(qtrue);
		} else if (Q_stricmp(name, "RunSPDemo") == 0) {
			if (uiInfo.demoAvailable) {
			  trap_Cmd_ExecuteText( EXEC_APPEND, va("demo %s_%i\n", uiInfo.mapList[ui_currentMap.integer].mapLoadName, uiInfo.gameTypes[ui_gameType.integer].gtEnum));
			}
		} else if (Q_stricmp(name, "LoadDemos") == 0) {
			UI_LoadDemos();
		} else if (Q_stricmp(name, "LoadMovies") == 0) {
			UI_LoadMovies();
		} else if (Q_stricmp(name, "LoadMods") == 0) {
			UI_LoadMods();
		} else if (Q_stricmp(name, "playMovie") == 0) {
			if (uiInfo.previewMovie >= 0) {
			  trap_CIN_StopCinematic(uiInfo.previewMovie);
			}
			trap_Cmd_ExecuteText( EXEC_APPEND, va("cinematic %s.roq 2\n", uiInfo.movieList[uiInfo.movieIndex]));
		} else if (Q_stricmp(name, "RunMod") == 0) {
			trap_Cvar_Set( "fs_game", uiInfo.modList[uiInfo.modIndex].modName);
			trap_Cmd_ExecuteText( EXEC_APPEND, "vid_restart;" );
		} else if (Q_stricmp(name, "RunDemo") == 0) {
			// quoted: a demo filename may contain spaces, which would otherwise
			// split into extra arguments and fail to play
			trap_Cmd_ExecuteText( EXEC_APPEND, va("demo \"%s\"\n", uiInfo.demoList[uiInfo.demoIndex]));
		} else if (Q_stricmp(name, "Quake3") == 0) {
			trap_Cvar_Set( "fs_game", "");
			trap_Cmd_ExecuteText( EXEC_APPEND, "vid_restart;" );
		} else if (Q_stricmp(name, "closeJoin") == 0) {
			if (uiInfo.serverStatus.refreshActive) {
				UI_StopServerRefresh();
				uiInfo.serverStatus.nextDisplayRefresh = 0;
				uiInfo.nextServerStatusRefresh = 0;
				uiInfo.nextFindPlayerRefresh = 0;
				UI_BuildServerDisplayList(qtrue);
			} else {
				Menus_CloseByName("joinserver");
				Menus_OpenByName("main");
			}
		} else if (Q_stricmp(name, "StopRefresh") == 0) {
			UI_StopServerRefresh();
			uiInfo.serverStatus.nextDisplayRefresh = 0;
			uiInfo.nextServerStatusRefresh = 0;
			uiInfo.nextFindPlayerRefresh = 0;
		} else if (Q_stricmp(name, "UpdateFilter") == 0) {
			// like a tab switch: a local scan or an empty snapshot queries,
			// a reopen with results keeps the cached snapshot
			if (UI_NetSourceType() == SRC_LOCAL || uiNumSourceServers[UI_ActiveNetSource()] == 0) {
				UI_StartServerRefresh(qtrue);
			}
			UI_BuildServerDisplayList(qtrue);
			UI_FeederSelection(FEEDER_SERVERS, 0);
		} else if (Q_stricmp(name, "ServerStatus") == 0) {
			uiServerRow_t *row = UI_SourceRow(uiInfo.serverStatus.currentServer);
			if (row) {
				Q_strncpyz(uiInfo.serverStatusAddress, row->adrstr, sizeof(uiInfo.serverStatusAddress));
				UI_BuildServerStatus(qtrue);
			}
		} else if (Q_stricmp(name, "FoundPlayerServerStatus") == 0) {
			Q_strncpyz(uiInfo.serverStatusAddress, uiInfo.foundPlayerServerAddresses[uiInfo.currentFoundPlayerServer], sizeof(uiInfo.serverStatusAddress));
			UI_BuildServerStatus(qtrue);
			Menu_SetFeederSelection(NULL, FEEDER_FINDPLAYER, 0, NULL);
		} else if (Q_stricmp(name, "FindPlayer") == 0) {
			UI_BuildFindPlayerList(qtrue);
			// clear the displayed server status info
			uiInfo.serverStatusInfo.numLines = 0;
			Menu_SetFeederSelection(NULL, FEEDER_FINDPLAYER, 0, NULL);
		} else if (Q_stricmp(name, "JoinServer") == 0) {
			uiServerRow_t *row;
			trap_Cvar_Set("cg_thirdPerson", "0");
			trap_Cvar_Set("cg_cameraOrbit", "0");
			trap_Cvar_Set("ui_singlePlayerActive", "0");
			row = UI_SourceRow(uiInfo.serverStatus.currentServer);
			if (row) {
				trap_Cmd_ExecuteText( EXEC_APPEND, va( "connect %s\n", row->adrstr ) );
			}
		} else if (Q_stricmp(name, "FoundPlayerJoinServer") == 0) {
			trap_Cvar_Set("ui_singlePlayerActive", "0");
			if (uiInfo.currentFoundPlayerServer >= 0 && uiInfo.currentFoundPlayerServer < uiInfo.numFoundPlayerServers) {
				trap_Cmd_ExecuteText( EXEC_APPEND, va( "connect %s\n", uiInfo.foundPlayerServerAddresses[uiInfo.currentFoundPlayerServer] ) );
			}
		} else if (Q_stricmp(name, "Quit") == 0) {
			trap_Cvar_Set("ui_singlePlayerActive", "0");
			trap_Cmd_ExecuteText( EXEC_NOW, "quit");
		} else if (Q_stricmp(name, "trinityLoginInit") == 0) {
			// Show the right group based on login state
			char trinityUser[64];
			menuDef_t *loginMenu = Menus_FindByName("login_menu");
			trap_Cvar_VariableStringBuffer( "cl_trinityUser", trinityUser, sizeof(trinityUser) );
			if ( loginMenu ) {
				if ( trinityUser[0] ) {
					Menu_ShowItemByName(loginMenu, "grpLoggedIn", qtrue);
					Menu_ShowItemByName(loginMenu, "grpLoggedInBtn", qtrue);
					Menu_ShowItemByName(loginMenu, "grpLoginForm", qfalse);
					Menu_ShowItemByName(loginMenu, "grpLoginBtn", qfalse);
				} else {
					Menu_ShowItemByName(loginMenu, "grpLoggedIn", qfalse);
					Menu_ShowItemByName(loginMenu, "grpLoggedInBtn", qfalse);
					Menu_ShowItemByName(loginMenu, "grpLoginForm", qtrue);
					Menu_ShowItemByName(loginMenu, "grpLoginBtn", qtrue);
				}
			}
		} else if (Q_stricmp(name, "trinityLogin") == 0) {
			// Copy UI cvars to engine-protected cvars and trigger login
			trap_Cvar_Set( "cl_trinityLoginUser", UI_Cvar_VariableString("ui_trinityLoginUser") );
			trap_Cvar_Set( "cl_trinityLoginPass", UI_Cvar_VariableString("ui_trinityLoginPass") );
			trap_Cvar_Set( "ui_trinityLoginPass", "" );
			trap_Cmd_ExecuteText( EXEC_APPEND, "trinity_login\n" );
		} else if (Q_stricmp(name, "updateDownload") == 0) {
			trap_Cmd_ExecuteText( EXEC_APPEND, "updatedownload\n" );
		} else if (Q_stricmp(name, "updateCancel") == 0) {
			trap_Cmd_ExecuteText( EXEC_APPEND, "updatecancel\n" );
		} else if (Q_stricmp(name, "updateRestart") == 0) {
			trap_Cmd_ExecuteText( EXEC_NOW, "updaterestart\n" );
		} else if (Q_stricmp(name, "trinityLogout") == 0) {
			trap_Cmd_ExecuteText( EXEC_NOW, "trinity_logout\n" );
			Menus_CloseByName("login_menu");
			Menus_OpenByName("login_menu");
		} else if (Q_stricmp(name, "Controls") == 0) {
		  trap_Cvar_Set( "cl_paused", "1" );
			trap_Key_SetCatcher( KEYCATCH_UI );
			Menus_CloseAll();
			Menus_ActivateByName("setup_menu2");
		} else if (Q_stricmp(name, "Leave") == 0) {
			trap_Cmd_ExecuteText( EXEC_APPEND, "disconnect\n" );
			trap_Key_SetCatcher( KEYCATCH_UI );
			Menus_CloseAll();
			Menus_ActivateByName("main");
		} else if (Q_stricmp(name, "ServerSort") == 0) {
			int sortColumn;
			if (Int_Parse(args, &sortColumn)) {
				// if same column we're already sorting on then flip the direction
				if (sortColumn == uiInfo.serverStatus.sortKey) {
					uiInfo.serverStatus.sortDir = !uiInfo.serverStatus.sortDir;
				}
				// make sure we sort again
				UI_ServersSort(sortColumn, qtrue);
			}
		} else if (Q_stricmp(name, "nextSkirmish") == 0) {
			UI_StartSkirmish(qtrue);
		} else if (Q_stricmp(name, "SkirmishStart") == 0) {
			UI_StartSkirmish(qfalse);
		} else if (Q_stricmp(name, "closeingame") == 0) {
			trap_Key_SetCatcher( trap_Key_GetCatcher() & ~KEYCATCH_UI );
			trap_Key_ClearStates();
			trap_Cvar_Set( "cl_paused", "0" );
			Menus_CloseAll();
		} else if (Q_stricmp(name, "sigilFlare") == 0) {
			trinityFlareTime = uiInfo.uiDC.realTime;
			trap_S_StartLocalSound( trinityFlareSound, CHAN_LOCAL_SOUND );
		} else if (Q_stricmp(name, "voteMap") == 0) {
			if (ui_currentNetMap.integer >=0 && ui_currentNetMap.integer < uiInfo.mapCount) {
				trap_Cmd_ExecuteText( EXEC_APPEND, va("callvote map %s\n",uiInfo.mapList[ui_currentNetMap.integer].mapLoadName) );
			}
		} else if (Q_stricmp(name, "voteKick") == 0) {
			if (uiInfo.playerIndex >= 0 && uiInfo.playerIndex < uiInfo.playerCount) {
				trap_Cmd_ExecuteText( EXEC_APPEND, va("callvote kick %s\n",uiInfo.playerNames[uiInfo.playerIndex]) );
			}
		} else if (Q_stricmp(name, "voteGame") == 0) {
			if (ui_netGameType.integer >= 0 && ui_netGameType.integer < uiInfo.numGameTypes) {
				trap_Cmd_ExecuteText( EXEC_APPEND, va("callvote g_gametype %i\n",uiInfo.gameTypes[ui_netGameType.integer].gtEnum) );
			}
		} else if (Q_stricmp(name, "voteLeader") == 0) {
			if (uiInfo.teamIndex >= 0 && uiInfo.teamIndex < uiInfo.myTeamCount) {
				trap_Cmd_ExecuteText( EXEC_APPEND, va("callteamvote leader %s\n",uiInfo.teamNames[uiInfo.teamIndex]) );
			}
		} else if (Q_stricmp(name, "addBot") == 0) {
			if (trap_Cvar_VariableValue("g_gametype") >= GT_TEAM) {
				trap_Cmd_ExecuteText( EXEC_APPEND, va("addbot %s %i %s\n", uiInfo.characterList[uiInfo.botIndex].name, uiInfo.skillIndex+1, (uiInfo.redBlue == 0) ? "Red" : "Blue") );
			} else {
				trap_Cmd_ExecuteText( EXEC_APPEND, va("addbot %s %i %s\n", UI_GetBotNameByNumber(uiInfo.botIndex), uiInfo.skillIndex+1, (uiInfo.redBlue == 0) ? "Red" : "Blue") );
			}
		} else if (Q_stricmp(name, "addFavorite") == 0) {
			if (UI_NetSourceType() != SRC_FAVORITES) {
				uiServerRow_t *row;
				int res;

				row = UI_SourceRow(uiInfo.serverStatus.currentServer);
				if (row && row->hostname[0] && row->adrstr[0]) {
					res = trap_LAN_AddServer(AS_FAVORITES, row->hostname, row->adrstr);
					UI_FavoriteAddressAdd(row->adrstr);
					if (res == 0) {
						// server already in the list
						Com_Printf("Favorite already in list\n");
					}
					else if (res == -1) {
						// list full
						Com_Printf("Favorite list full\n");
					}
					else {
						// successfully added
						Com_Printf("Added favorite server %s\n", row->adrstr);
					}
				}
			}
		} else if (Q_stricmp(name, "deleteFavorite") == 0) {
			if (UI_NetSourceType() == SRC_FAVORITES) {
				uiServerRow_t *row;
				int tab, j;

				row = UI_SourceRow(uiInfo.serverStatus.currentServer);
				if (row && row->adrstr[0]) {
					trap_LAN_RemoveServer(AS_FAVORITES, row->adrstr);
					UI_FavoriteAddressRemove(row->adrstr);
					// drop the row from this tab's snapshot too
					tab = UI_ActiveNetSource();
					j = row - uiSourceServers[tab];
					for (; j < uiNumSourceServers[tab] - 1; j++) {
						uiSourceServers[tab][j] = uiSourceServers[tab][j+1];
					}
					uiNumSourceServers[tab]--;
					UI_BuildServerDisplayList(qtrue);
				}
			}
		} else if (Q_stricmp(name, "createFavorite") == 0) {
			if (UI_NetSourceType() == SRC_FAVORITES) {
				char name[MAX_NAME_LENGTH];
				char addr[MAX_NAME_LENGTH];
				int res;

				name[0] = addr[0] = '\0';
				Q_strncpyz(name, 	UI_Cvar_VariableString("ui_favoriteName"), MAX_NAME_LENGTH);
				Q_strncpyz(addr, 	UI_Cvar_VariableString("ui_favoriteAddress"), MAX_NAME_LENGTH);
				if (strlen(name) > 0 && strlen(addr) > 0) {
					res = trap_LAN_AddServer(AS_FAVORITES, name, addr);
					UI_FavoriteAddressAdd(addr);
					if (res == 0) {
						// server already in the list
						Com_Printf("Favorite already in list\n");
					}
					else if (res == -1) {
						// list full
						Com_Printf("Favorite list full\n");
					}
					else {
						// successfully added
						Com_Printf("Added favorite server %s\n", addr);
						// pick it up in the tab
						UI_StartServerRefresh(qtrue);
					}
				}
			}
		} else if (Q_stricmp(name, "orders") == 0) {
			const char *orders;
			if (String_Parse(args, &orders)) {
				int selectedPlayer = trap_Cvar_VariableValue("cg_selectedPlayer");
				if (selectedPlayer < uiInfo.myTeamCount) {
					strcpy(buff, orders);
					trap_Cmd_ExecuteText( EXEC_APPEND, va(buff, uiInfo.teamClientNums[selectedPlayer]) );
					trap_Cmd_ExecuteText( EXEC_APPEND, "\n" );
				} else {
					int i;
					for (i = 0; i < uiInfo.myTeamCount; i++) {
						if (Q_stricmp(UI_Cvar_VariableString("name"), uiInfo.teamNames[i]) == 0) {
							continue;
						}
						strcpy(buff, orders);
						trap_Cmd_ExecuteText( EXEC_APPEND, va(buff, uiInfo.teamNames[i]) );
						trap_Cmd_ExecuteText( EXEC_APPEND, "\n" );
					}
				}
				trap_Key_SetCatcher( trap_Key_GetCatcher() & ~KEYCATCH_UI );
				trap_Key_ClearStates();
				trap_Cvar_Set( "cl_paused", "0" );
				Menus_CloseAll();
			}
		} else if (Q_stricmp(name, "voiceOrdersTeam") == 0) {
			const char *orders;
			if (String_Parse(args, &orders)) {
				int selectedPlayer = trap_Cvar_VariableValue("cg_selectedPlayer");
				if (selectedPlayer == uiInfo.myTeamCount) {
					trap_Cmd_ExecuteText( EXEC_APPEND, orders );
					trap_Cmd_ExecuteText( EXEC_APPEND, "\n" );
				}
				trap_Key_SetCatcher( trap_Key_GetCatcher() & ~KEYCATCH_UI );
				trap_Key_ClearStates();
				trap_Cvar_Set( "cl_paused", "0" );
				Menus_CloseAll();
			}
		} else if (Q_stricmp(name, "voiceOrders") == 0) {
			const char *orders;
			if (String_Parse(args, &orders)) {
				int selectedPlayer = trap_Cvar_VariableValue("cg_selectedPlayer");
				if (selectedPlayer < uiInfo.myTeamCount) {
					strcpy(buff, orders);
					trap_Cmd_ExecuteText( EXEC_APPEND, va(buff, uiInfo.teamClientNums[selectedPlayer]) );
					trap_Cmd_ExecuteText( EXEC_APPEND, "\n" );
				}
				trap_Key_SetCatcher( trap_Key_GetCatcher() & ~KEYCATCH_UI );
				trap_Key_ClearStates();
				trap_Cvar_Set( "cl_paused", "0" );
				Menus_CloseAll();
			}
		} else if (Q_stricmp(name, "glCustom") == 0) {
			trap_Cvar_Set("ui_glCustom", "4");
		} else if (Q_stricmp(name, "update") == 0) {
			if (String_Parse(args, &name2)) {
				UI_Update(name2);
			}
		} else if ( Q_stricmp( name, "updateHDRAvail" ) == 0 ) {
			UI_UpdateHDRAvail();
		} else if ( Q_stricmp( name, "snapHDRCvars" ) == 0 ) {
			// Sliders write raw floats ("1000.000000") that never equal the default
			// string, so CVAR_ARCHIVE_ND persists them even at default. Re-set via
			// Cvar_SetValue (clean "%i") to keep defaults unwritten and snap Peak.
			trap_Cvar_SetValue( "r_hdrPeak", (int)( trap_Cvar_VariableValue( "r_hdrPeak" ) + 0.5f ) );
			trap_Cvar_SetValue( "r_hdrHighlight", trap_Cvar_VariableValue( "r_hdrHighlight" ) );
			trap_Cvar_SetValue( "r_hdrPaperWhite", 0 );  // keep paper-white on auto; experts set it via console
		} else if ( UI_VR_RunMenuScript( name ) ) {
			// handled
		}
		else {
			Com_Printf("unknown UI script %s\n", name);
		}
	}
}

static void UI_GetTeamColor(vec4_t *color) {
}

/*
==================
UI_MapCountByGameType
==================
*/
static int UI_MapCountByGameType(qboolean singlePlayer) {
	int i, c, game;
	c = 0;
	game = singlePlayer ? uiInfo.gameTypes[ui_gameType.integer].gtEnum : uiInfo.gameTypes[ui_netGameType.integer].gtEnum;
	if (game == GT_SINGLE_PLAYER) {
		game++;
	} 
	if (game == GT_TEAM) {
		game = GT_FFA;
	}

	for (i = 0; i < uiInfo.mapCount; i++) {
		uiInfo.mapList[i].active = qfalse;
		if ( uiInfo.mapList[i].typeBits & (1 << game)) {
			if (singlePlayer) {
				if (!(uiInfo.mapList[i].typeBits & (1 << GT_SINGLE_PLAYER))) {
					continue;
				}
			}
			c++;
			uiInfo.mapList[i].active = qtrue;
		}
	}
	return c;
}

qboolean UI_hasSkinForBase(const char *base, const char *team) {
	char	test[1024];
	
	Com_sprintf( test, sizeof( test ), "models/players/%s/%s/lower_default.skin", base, team );

	if (trap_FS_FOpenFile(test, 0, FS_READ)) {
		return qtrue;
	}
	Com_sprintf( test, sizeof( test ), "models/players/characters/%s/%s/lower_default.skin", base, team );

	if (trap_FS_FOpenFile(test, 0, FS_READ)) {
		return qtrue;
	}
	return qfalse;
}

/*
==================
UI_MapCountByTeam
==================
*/
static int UI_HeadCountByTeam() {
	static int init = 0;
	int i, j, k, c, tIndex;
	
	c = 0;
	if (!init) {
		for (i = 0; i < uiInfo.characterCount; i++) {
			uiInfo.characterList[i].reference = 0;
			for (j = 0; j < uiInfo.teamCount; j++) {
			  if (UI_hasSkinForBase(uiInfo.characterList[i].base, uiInfo.teamList[j].teamName)) {
					uiInfo.characterList[i].reference |= (1<<j);
			  }
			}
		}
		init = 1;
	}

	tIndex = UI_TeamIndexFromName(UI_Cvar_VariableString("ui_teamName"));

	// do names
	for (i = 0; i < uiInfo.characterCount; i++) {
		uiInfo.characterList[i].active = qfalse;
		for(j = 0; j < TEAM_MEMBERS; j++) {
			if (uiInfo.teamList[tIndex].teamMembers[j] != NULL) {
				if (uiInfo.characterList[i].reference&(1<<tIndex)) {// && Q_stricmp(uiInfo.teamList[tIndex].teamMembers[j], uiInfo.characterList[i].name)==0) {
					uiInfo.characterList[i].active = qtrue;
					c++;
					break;
				}
			}
		}
	}

	// and then aliases
	for(j = 0; j < TEAM_MEMBERS; j++) {
		for(k = 0; k < uiInfo.aliasCount; k++) {
			if (uiInfo.aliasList[k].name != NULL) {
				if (Q_stricmp(uiInfo.teamList[tIndex].teamMembers[j], uiInfo.aliasList[k].name)==0) {
					for (i = 0; i < uiInfo.characterCount; i++) {
						if (uiInfo.characterList[i].headImage != -1 && uiInfo.characterList[i].reference&(1<<tIndex) && Q_stricmp(uiInfo.aliasList[k].ai, uiInfo.characterList[i].name)==0) {
							if (uiInfo.characterList[i].active == qfalse) {
								uiInfo.characterList[i].active = qtrue;
								c++;
							}
							break;
						}
					}
				}
			}
		}
	}
	return c;
}

/*
==================
UI_InsertServerIntoDisplayList
==================
*/
static void UI_InsertServerIntoDisplayList(int num, int position) {
	int i;

	if (position < 0 || position > uiInfo.serverStatus.numDisplayServers ) {
		return;
	}
	//
	uiInfo.serverStatus.numDisplayServers++;
	for (i = uiInfo.serverStatus.numDisplayServers; i > position; i--) {
		uiInfo.serverStatus.displayServers[i] = uiInfo.serverStatus.displayServers[i-1];
	}
	uiInfo.serverStatus.displayServers[position] = num;
}

/*
==================
UI_BinaryServerInsertion
==================
*/
static void UI_BinaryServerInsertion(int num) {
	int mid, offset, res, len;
	int tab = UI_ActiveNetSource();

	// use binary search to insert server
	len = uiInfo.serverStatus.numDisplayServers;
	mid = len;
	offset = 0;
	res = 0;
	while(mid > 0) {
		mid = len >> 1;
		//
		res = UI_CompareServerRows( &uiSourceServers[tab][num],
					&uiSourceServers[tab][uiInfo.serverStatus.displayServers[offset+mid]] );
		// if equal
		if (res == 0) {
			UI_InsertServerIntoDisplayList(num, offset+mid);
			return;
		}
		// if larger
		else if (res == 1) {
			offset += mid;
			len -= mid;
		}
		// if smaller
		else {
			len -= mid;
		}
	}
	if (res == 1) {
		offset++;
	}
	UI_InsertServerIntoDisplayList(num, offset);
}

/*
==================
UI_HarvestServers

Pulls ping-completed results out of the engine bucket(s) into the active
tab's snapshot. The engine list is only a transfer buffer: this runs solely
while the active tab is the one being refreshed.
==================
*/
static void UI_HarvestServers( void ) {
	int				buckets[2];
	int				numBuckets;
	int				tab;
	int				b, i, j, count, ping;
	char			info[MAX_STRING_CHARS];
	uiServerRow_t	*row;
	const char		*str;

	tab = UI_ActiveNetSource();
	numBuckets = UI_SourceLANBuckets( buckets );

	for ( b = 0; b < numBuckets; b++ ) {
		count = trap_LAN_GetServerCount( buckets[b] );
		for ( i = 0; i < count; i++ ) {
			if ( !trap_LAN_ServerIsVisible( buckets[b], i ) ) {
				// already harvested
				continue;
			}
			ping = trap_LAN_GetServerPing( buckets[b], i );
			if ( ping <= 0 && UI_NetSourceType() != SRC_FAVORITES ) {
				// still waiting; favorites keep an address row meanwhile
				continue;
			}

			trap_LAN_GetServerInfo( buckets[b], i, info, MAX_STRING_CHARS );

			// one row per address: dedupe across the two buckets the
			// Internet source spans on quake3e, and update favorites rows
			// in place as their pings arrive
			row = NULL;
			str = Info_ValueForKey( info, "addr" );
			for ( j = 0; j < uiNumSourceServers[tab]; j++ ) {
				if ( !Q_stricmp( uiSourceServers[tab][j].adrstr, str ) ) {
					row = &uiSourceServers[tab][j];
					break;
				}
			}
			if ( !row ) {
				if ( uiNumSourceServers[tab] >= MAX_SOURCESERVERS ) {
					continue;
				}
				row = &uiSourceServers[tab][uiNumSourceServers[tab]];
				uiNumSourceServers[tab]++;
			}

			Q_strncpyz( row->adrstr, str, sizeof( row->adrstr ) );
			Q_strncpyz( row->hostname, Info_ValueForKey( info, "hostname" ), sizeof( row->hostname ) );
			Q_strncpyz( row->mapname, Info_ValueForKey( info, "mapname" ), sizeof( row->mapname ) );
			Q_strncpyz( row->game, Info_ValueForKey( info, "game" ), sizeof( row->game ) );
			row->ping = ping;
			row->clients = atoi( Info_ValueForKey( info, "clients" ) );
			row->maxClients = atoi( Info_ValueForKey( info, "sv_maxclients" ) );
			str = Info_ValueForKey( info, "g_humanplayers" );
			row->humanClients = str[0] ? atoi( str ) : row->clients;
			row->gameType = atoi( Info_ValueForKey( info, "gametype" ) );
			row->netType = atoi( Info_ValueForKey( info, "nettype" ) );
			row->mode = -1;
			str = Info_ValueForKey( info, "g_mode" );
			if ( str[0] ) {
				j = atoi( str );
				if ( j >= MODE_VQ3 && j < MODE_COUNT ) {
					row->mode = j;
				}
			}

			if ( ping > 0 ) {
				// harvested: drop it from the ping walk
				trap_LAN_MarkServerVisible( buckets[b], i, qfalse );
			}
		}
	}
}

/*
==================
UI_BuildServerDisplayList

Filters and sorts the active tab's snapshot into the display list. Pure
snapshot work: no engine reads, so it is safe on any tab at any time.
==================
*/
static void UI_BuildServerDisplayList(qboolean force) {
	int				i, count, len, effClients, tab;
	uiServerRow_t	*row;

	if (!(force || uiInfo.uiDC.realTime > uiInfo.serverStatus.nextDisplayRefresh)) {
		return;
	}
	// legacy distinction between reset flavors; both rebuild fully now
	if ( force == 2 ) {
		force = 1;
	}

	// do motd updates here too
	trap_Cvar_VariableStringBuffer( "cl_motdString", uiInfo.serverStatus.motd, sizeof(uiInfo.serverStatus.motd) );
	len = strlen(uiInfo.serverStatus.motd);
	if (len == 0) {
		strcpy(uiInfo.serverStatus.motd, "Welcome to Team Arena!");
		len = strlen(uiInfo.serverStatus.motd);
	}
	if (len != uiInfo.serverStatus.motdLen) {
		uiInfo.serverStatus.motdLen = len;
		uiInfo.serverStatus.motdWidth = -1;
	}

	if (force) {
		// set list box index to zero
		Menu_SetFeederSelection(NULL, FEEDER_SERVERS, 0, NULL);
	}

	tab = UI_ActiveNetSource();
	count = uiNumSourceServers[tab];
	uiInfo.serverStatus.numDisplayServers = 0;
	uiInfo.serverStatus.numPlayersOnServers = 0;

	for (i = 0; i < count; i++) {
		row = &uiSourceServers[tab][i];
		effClients = ui_browserExcludeBots.integer ? row->humanClients : row->clients;

		if (ui_browserShowEmpty.integer == 0 && effClients == 0) {
			continue;
		}

		if (ui_browserShowFull.integer == 0 && row->clients == row->maxClients) {
			continue;
		}

		if (uiInfo.joinGameTypes[ui_joinGameType.integer].gtEnum != -1 &&
				row->gameType != uiInfo.joinGameTypes[ui_joinGameType.integer].gtEnum) {
			continue;
		}

		if (ui_serverFilterType.integer > 0 &&
				Q_stricmp(row->game, serverFilters[ui_serverFilterType.integer].basedir) != 0) {
			continue;
		}

		// insert the server into the list
		UI_BinaryServerInsertion(i);
		uiInfo.serverStatus.numPlayersOnServers += effClients;
	}

	uiInfo.serverStatus.nextDisplayRefresh = uiInfo.uiDC.realTime + 500;
}

typedef struct
{
	char *name, *altName;
} serverStatusCvar_t;

serverStatusCvar_t serverStatusCvars[] = {
	{"sv_hostname", "Name"},
	{"Address", ""},
	{"gamename", "Game name"},
	{"g_gametype", "Game type"},
	{"mapname", "Map"},
	{"version", ""},
	{"protocol", ""},
	{"timelimit", ""},
	{"fraglimit", ""},
	{NULL, NULL}
};

/*
==================
UI_SortServerStatusInfo
==================
*/
static void UI_SortServerStatusInfo( serverStatusInfo_t *info ) {
	int i, j, index;
	char *tmp1, *tmp2;

	// FIXME: if "gamename" == "baseq3" or "missionpack" then
	// replace the gametype number by FFA, CTF etc.
	//
	index = 0;
	for (i = 0; serverStatusCvars[i].name; i++) {
		for (j = 0; j < info->numLines; j++) {
			if ( !info->lines[j][1] || info->lines[j][1][0] ) {
				continue;
			}
			if ( !Q_stricmp(serverStatusCvars[i].name, info->lines[j][0]) ) {
				// swap lines
				tmp1 = info->lines[index][0];
				tmp2 = info->lines[index][3];
				info->lines[index][0] = info->lines[j][0];
				info->lines[index][3] = info->lines[j][3];
				info->lines[j][0] = tmp1;
				info->lines[j][3] = tmp2;
				//
				if ( strlen(serverStatusCvars[i].altName) ) {
					info->lines[index][0] = serverStatusCvars[i].altName;
				}
				index++;
			}
		}
	}
}

/*
==================
UI_GetServerStatusInfo
==================
*/
static int UI_GetServerStatusInfo( const char *serverAddress, serverStatusInfo_t *info ) {
	char *p, *score, *ping, *name;
	int i, len;

	if (!info) {
		trap_LAN_ServerStatus( serverAddress, NULL, 0);
		return qfalse;
	}
	memset(info, 0, sizeof(*info));
	if ( trap_LAN_ServerStatus( serverAddress, info->text, sizeof(info->text)) ) {
		Q_strncpyz(info->address, serverAddress, sizeof(info->address));
		p = info->text;
		info->numLines = 0;
		info->lines[info->numLines][0] = "Address";
		info->lines[info->numLines][1] = "";
		info->lines[info->numLines][2] = "";
		info->lines[info->numLines][3] = info->address;
		info->numLines++;
		// get the cvars
		while (p && *p) {
			p = strchr(p, '\\');
			if (!p) break;
			*p++ = '\0';
			if (*p == '\\')
				break;
			info->lines[info->numLines][0] = p;
			info->lines[info->numLines][1] = "";
			info->lines[info->numLines][2] = "";
			p = strchr(p, '\\');
			if (!p) break;
			*p++ = '\0';
			info->lines[info->numLines][3] = p;

			info->numLines++;
			if (info->numLines >= MAX_SERVERSTATUS_LINES)
				break;
		}
		// get the player list
		if (info->numLines < MAX_SERVERSTATUS_LINES-3) {
			// empty line
			info->lines[info->numLines][0] = "";
			info->lines[info->numLines][1] = "";
			info->lines[info->numLines][2] = "";
			info->lines[info->numLines][3] = "";
			info->numLines++;
			// header
			info->lines[info->numLines][0] = "num";
			info->lines[info->numLines][1] = "score";
			info->lines[info->numLines][2] = "ping";
			info->lines[info->numLines][3] = "name";
			info->numLines++;
			// parse players
			i = 0;
			len = 0;
			while (p && *p) {
				if (*p == '\\')
					*p++ = '\0';
				if (!p)
					break;
				score = p;
				p = strchr(p, ' ');
				if (!p)
					break;
				*p++ = '\0';
				ping = p;
				p = strchr(p, ' ');
				if (!p)
					break;
				*p++ = '\0';
				name = p;
				Com_sprintf(&info->pings[len], sizeof(info->pings)-len, "%d", i);
				info->lines[info->numLines][0] = &info->pings[len];
				len += strlen(&info->pings[len]) + 1;
				info->lines[info->numLines][1] = score;
				info->lines[info->numLines][2] = ping;
				info->lines[info->numLines][3] = name;
				info->numLines++;
				if (info->numLines >= MAX_SERVERSTATUS_LINES)
					break;
				p = strchr(p, '\\');
				if (!p)
					break;
				*p++ = '\0';
				//
				i++;
			}
		}
		UI_SortServerStatusInfo( info );
		return qtrue;
	}
	return qfalse;
}

/*
==================
stristr
==================
*/
static char *stristr(char *str, char *charset) {
	int i;

	while(*str) {
		for (i = 0; charset[i] && str[i]; i++) {
			if (toupper(charset[i]) != toupper(str[i])) break;
		}
		if (!charset[i]) return str;
		str++;
	}
	return NULL;
}

/*
==================
UI_BuildFindPlayerList
==================
*/
static void UI_BuildFindPlayerList(qboolean force) {
	static int numFound, numTimeOuts;
	int i, j, resend;
	serverStatusInfo_t info;
	char name[MAX_NAME_LENGTH+2];

	if (!force) {
		if (!uiInfo.nextFindPlayerRefresh || uiInfo.nextFindPlayerRefresh > uiInfo.uiDC.realTime) {
			return;
		}
	}
	else {
		memset(&uiInfo.pendingServerStatus, 0, sizeof(uiInfo.pendingServerStatus));
		uiInfo.numFoundPlayerServers = 0;
		uiInfo.currentFoundPlayerServer = 0;
		trap_Cvar_VariableStringBuffer( "ui_findPlayer", uiInfo.findPlayerName, sizeof(uiInfo.findPlayerName));
		Q_CleanStr(uiInfo.findPlayerName);
		// should have a string of some length
		if (!strlen(uiInfo.findPlayerName)) {
			uiInfo.nextFindPlayerRefresh = 0;
			return;
		}
		// set resend time
		resend = ui_serverStatusTimeOut.integer / 2 - 10;
		if (resend < 50) {
			resend = 50;
		}
		trap_Cvar_Set("cl_serverStatusResendTime", va("%d", resend));
		// reset all server status requests
		trap_LAN_ServerStatus( NULL, NULL, 0);
		//
		uiInfo.numFoundPlayerServers = 1;
		Com_sprintf(uiInfo.foundPlayerServerNames[uiInfo.numFoundPlayerServers-1],
						sizeof(uiInfo.foundPlayerServerNames[uiInfo.numFoundPlayerServers-1]),
							"searching %d...", uiInfo.pendingServerStatus.num);
		numFound = 0;
		numTimeOuts++;
	}
	for (i = 0; i < MAX_SERVERSTATUSREQUESTS; i++) {
		// if this pending server is valid
		if (uiInfo.pendingServerStatus.server[i].valid) {
			// try to get the server status for this server
			if (UI_GetServerStatusInfo( uiInfo.pendingServerStatus.server[i].adrstr, &info ) ) {
				//
				numFound++;
				// parse through the server status lines
				for (j = 0; j < info.numLines; j++) {
					// should have ping info
					if ( !info.lines[j][2] || !info.lines[j][2][0] ) {
						continue;
					}
					// clean string first
					Q_strncpyz(name, info.lines[j][3], sizeof(name));
					Q_CleanStr(name);
					// if the player name is a substring
					if (stristr(name, uiInfo.findPlayerName)) {
						// add to found server list if we have space (always leave space for a line with the number found)
						if (uiInfo.numFoundPlayerServers < MAX_FOUNDPLAYER_SERVERS-1) {
							//
							Q_strncpyz(uiInfo.foundPlayerServerAddresses[uiInfo.numFoundPlayerServers-1],
										uiInfo.pendingServerStatus.server[i].adrstr,
											sizeof(uiInfo.foundPlayerServerAddresses[0]));
							Q_strncpyz(uiInfo.foundPlayerServerNames[uiInfo.numFoundPlayerServers-1],
										uiInfo.pendingServerStatus.server[i].name,
											sizeof(uiInfo.foundPlayerServerNames[0]));
							uiInfo.numFoundPlayerServers++;
						}
						else {
							// can't add any more so we're done
							uiInfo.pendingServerStatus.num = uiInfo.serverStatus.numDisplayServers;
						}
					}
				}
				Com_sprintf(uiInfo.foundPlayerServerNames[uiInfo.numFoundPlayerServers-1],
								sizeof(uiInfo.foundPlayerServerNames[uiInfo.numFoundPlayerServers-1]),
									"searching %d/%d...", uiInfo.pendingServerStatus.num, numFound);
				// retrieved the server status so reuse this spot
				uiInfo.pendingServerStatus.server[i].valid = qfalse;
			}
		}
		// if empty pending slot or timed out
		if (!uiInfo.pendingServerStatus.server[i].valid ||
			uiInfo.pendingServerStatus.server[i].startTime < uiInfo.uiDC.realTime - ui_serverStatusTimeOut.integer) {
			if (uiInfo.pendingServerStatus.server[i].valid) {
				numTimeOuts++;
			}
			// reset server status request for this address
			UI_GetServerStatusInfo( uiInfo.pendingServerStatus.server[i].adrstr, NULL );
			// reuse pending slot
			uiInfo.pendingServerStatus.server[i].valid = qfalse;
			// if we didn't try to get the status of all servers in the main browser yet
			if (uiInfo.pendingServerStatus.num < uiInfo.serverStatus.numDisplayServers) {
				const uiServerRow_t *row = UI_SourceRow(uiInfo.pendingServerStatus.num);
				uiInfo.pendingServerStatus.server[i].startTime = uiInfo.uiDC.realTime;
				if (row) {
					Q_strncpyz(uiInfo.pendingServerStatus.server[i].adrstr, row->adrstr, sizeof(uiInfo.pendingServerStatus.server[i].adrstr));
					Q_strncpyz(uiInfo.pendingServerStatus.server[i].name, row->hostname, sizeof(uiInfo.pendingServerStatus.server[0].name));
				} else {
					uiInfo.pendingServerStatus.server[i].adrstr[0] = '\0';
					uiInfo.pendingServerStatus.server[i].name[0] = '\0';
				}
				uiInfo.pendingServerStatus.server[i].valid = qtrue;
				uiInfo.pendingServerStatus.num++;
				Com_sprintf(uiInfo.foundPlayerServerNames[uiInfo.numFoundPlayerServers-1],
								sizeof(uiInfo.foundPlayerServerNames[uiInfo.numFoundPlayerServers-1]),
									"searching %d/%d...", uiInfo.pendingServerStatus.num, numFound);
			}
		}
	}
	for (i = 0; i < MAX_SERVERSTATUSREQUESTS; i++) {
		if (uiInfo.pendingServerStatus.server[i].valid) {
			break;
		}
	}
	// if still trying to retrieve server status info
	if (i < MAX_SERVERSTATUSREQUESTS) {
		uiInfo.nextFindPlayerRefresh = uiInfo.uiDC.realTime + 25;
	}
	else {
		// add a line that shows the number of servers found
		if (!uiInfo.numFoundPlayerServers) {
			Com_sprintf(uiInfo.foundPlayerServerNames[uiInfo.numFoundPlayerServers-1], sizeof(uiInfo.foundPlayerServerAddresses[0]), "no servers found");
		}
		else {
			Com_sprintf(uiInfo.foundPlayerServerNames[uiInfo.numFoundPlayerServers-1], sizeof(uiInfo.foundPlayerServerAddresses[0]),
						"%d server%s found with player %s", uiInfo.numFoundPlayerServers-1,
						uiInfo.numFoundPlayerServers == 2 ? "":"s", uiInfo.findPlayerName);
		}
		uiInfo.nextFindPlayerRefresh = 0;
		// show the server status info for the selected server
		UI_FeederSelection(FEEDER_FINDPLAYER, uiInfo.currentFoundPlayerServer);
	}
}

/*
==================
UI_BuildServerStatus
==================
*/
static void UI_BuildServerStatus(qboolean force) {

	if (uiInfo.nextFindPlayerRefresh) {
		return;
	}
	if (!force) {
		if (!uiInfo.nextServerStatusRefresh || uiInfo.nextServerStatusRefresh > uiInfo.uiDC.realTime) {
			return;
		}
	}
	else {
		Menu_SetFeederSelection(NULL, FEEDER_SERVERSTATUS, 0, NULL);
		uiInfo.serverStatusInfo.numLines = 0;
		// reset all server status requests
		trap_LAN_ServerStatus( NULL, NULL, 0);
	}
	if (uiInfo.serverStatus.currentServer < 0 || uiInfo.serverStatus.currentServer > uiInfo.serverStatus.numDisplayServers || uiInfo.serverStatus.numDisplayServers == 0) {
		return;
	}
	if (UI_GetServerStatusInfo( uiInfo.serverStatusAddress, &uiInfo.serverStatusInfo ) ) {
		uiInfo.nextServerStatusRefresh = 0;
		UI_GetServerStatusInfo( uiInfo.serverStatusAddress, NULL );
	}
	else {
		uiInfo.nextServerStatusRefresh = uiInfo.uiDC.realTime + 500;
	}
}

/*
==================
UI_FeederCount
==================
*/
static int UI_FeederCount(float feederID) {
	if (feederID == FEEDER_HEADS) {
		return UI_HeadCountByTeam();
	} else if (feederID == FEEDER_Q3HEADS) {
		return uiInfo.q3HeadCount;
	} else if (feederID == FEEDER_CINEMATICS) {
		return uiInfo.movieCount;
	} else if (feederID == FEEDER_MAPS || feederID == FEEDER_ALLMAPS) {
		return UI_MapCountByGameType(feederID == FEEDER_MAPS ? qtrue : qfalse);
	} else if (feederID == FEEDER_SERVERS) {
		return uiInfo.serverStatus.numDisplayServers;
	} else if (feederID == FEEDER_SERVERSTATUS) {
		return uiInfo.serverStatusInfo.numLines;
	} else if (feederID == FEEDER_FINDPLAYER) {
		return uiInfo.numFoundPlayerServers;
	} else if (feederID == FEEDER_PLAYER_LIST) {
		if (uiInfo.uiDC.realTime > uiInfo.playerRefresh) {
			uiInfo.playerRefresh = uiInfo.uiDC.realTime + 3000;
			UI_BuildPlayerList();
		}
		return uiInfo.playerCount;
	} else if (feederID == FEEDER_TEAM_LIST) {
		if (uiInfo.uiDC.realTime > uiInfo.playerRefresh) {
			uiInfo.playerRefresh = uiInfo.uiDC.realTime + 3000;
			UI_BuildPlayerList();
		}
		return uiInfo.myTeamCount;
	} else if (feederID == FEEDER_MODS) {
		return uiInfo.modCount;
	} else if (feederID == FEEDER_DEMOS) {
		return uiInfo.demoCount;
	}
	return 0;
}

static const char *UI_SelectedMap(int index, int *actual) {
	int i, c;
	c = 0;
	*actual = 0;
	for (i = 0; i < uiInfo.mapCount; i++) {
		if (uiInfo.mapList[i].active) {
			if (c == index) {
				*actual = i;
				return uiInfo.mapList[i].mapName;
			} else {
				c++;
			}
		}
	}
	return "";
}

static const char *UI_SelectedHead(int index, int *actual) {
	int i, c;
	c = 0;
	*actual = 0;
	for (i = 0; i < uiInfo.characterCount; i++) {
		if (uiInfo.characterList[i].active) {
			if (c == index) {
				*actual = i;
				return uiInfo.characterList[i].name;
			} else {
				c++;
			}
		}
	}
	return "";
}

static int UI_GetIndexFromSelection(int actual) {
	int i, c;
	c = 0;
	for (i = 0; i < uiInfo.mapCount; i++) {
		if (uiInfo.mapList[i].active) {
			if (i == actual) {
				return c;
			}
				c++;
		}
	}
  return 0;
}

static void UI_UpdatePendingPings() {
	int		buckets[2];
	int		nb, b;

	nb = UI_SourceLANBuckets( buckets );
	for ( b = 0; b < nb; b++ ) {
		trap_LAN_ResetPings( buckets[b] );
	}
	uiInfo.serverStatus.refreshActive = qtrue;
	uiInfo.serverStatus.refreshSource = UI_ActiveNetSource();
	uiInfo.serverStatus.refreshtime = uiInfo.uiDC.realTime + 1000;

}

/*
=================
UI_SyncFavoritesToEngine

The archived server1..64 cvars are the favorites address book; rebuild the
engine's favorites bucket from them so ping updates flow like any other
source.
=================
*/
static void UI_SyncFavoritesToEngine( void ) {
	char	addr[MAX_ADDRESSLENGTH];
	int		i, count;

	count = trap_LAN_GetServerCount( AS_FAVORITES );
	for ( i = 0; i < count; i++ ) {
		trap_LAN_GetServerAddressString( AS_FAVORITES, 0, addr, sizeof( addr ) );
		if ( !addr[0] ) {
			break;
		}
		trap_LAN_RemoveServer( AS_FAVORITES, addr );
	}

	for ( i = 1; i <= MAX_FAVORITESERVERS; i++ ) {
		trap_Cvar_VariableStringBuffer( va( "server%d", i ), addr, sizeof( addr ) );
		if ( addr[0] ) {
			trap_LAN_AddServer( AS_FAVORITES, addr, addr );
		}
	}
}

/*
=================
UI_FavoriteAddressMatch

A stored slot matches an engine-normalized address either exactly or when
the slot was written without the :port suffix the engine appends.
=================
*/
static qboolean UI_FavoriteAddressMatch( const char *slot, const char *addr ) {
	int		len;

	if ( !slot[0] ) {
		return qfalse;
	}
	if ( !Q_stricmp( slot, addr ) ) {
		return qtrue;
	}
	len = strlen( slot );
	if ( !strchr( slot, ':' ) && !Q_stricmpn( slot, addr, len ) && addr[len] == ':' ) {
		return qtrue;
	}
	return qfalse;
}

/*
=================
UI_FavoriteAddressAdd

Store an address in the first free server<N> slot (no-op if present).
=================
*/
static void UI_FavoriteAddressAdd( const char *addr ) {
	char	slot[MAX_ADDRESSLENGTH];
	int		i, best;

	best = 0;
	for ( i = 1; i <= MAX_FAVORITESERVERS; i++ ) {
		trap_Cvar_VariableStringBuffer( va( "server%d", i ), slot, sizeof( slot ) );
		if ( UI_FavoriteAddressMatch( slot, addr ) ) {
			return;
		}
		if ( !slot[0] && !best ) {
			best = i;
		}
	}
	if ( best ) {
		trap_Cvar_Set( va( "server%d", best ), addr );
	}
}

/*
=================
UI_FavoriteAddressRemove
=================
*/
static void UI_FavoriteAddressRemove( const char *addr ) {
	char	slot[MAX_ADDRESSLENGTH];
	int		i;

	for ( i = 1; i <= MAX_FAVORITESERVERS; i++ ) {
		trap_Cvar_VariableStringBuffer( va( "server%d", i ), slot, sizeof( slot ) );
		if ( UI_FavoriteAddressMatch( slot, addr ) ) {
			trap_Cvar_Set( va( "server%d", i ), "" );
		}
	}
}

static const char *UI_FeederItemText(float feederID, int index, int column, qhandle_t *handle) {
	static char hostname[1024];
	static char clientBuff[32];
	static char pingBuff[32];
	*handle = -1;
	if (feederID == FEEDER_HEADS) {
		int actual;
		return UI_SelectedHead(index, &actual);
	} else if (feederID == FEEDER_Q3HEADS) {
		if (index >= 0 && index < uiInfo.q3HeadCount) {
			return uiInfo.q3HeadNames[index];
		}
	} else if (feederID == FEEDER_MAPS || feederID == FEEDER_ALLMAPS) {
		int actual;
		return UI_SelectedMap(index, &actual);
	} else if (feederID == FEEDER_SERVERS) {
		// rows come from the tab's snapshot, never from live engine reads
		uiServerRow_t *row = UI_SourceRow(index);
		if (row) {
			switch (column) {
				case SORT_HOST :
					if (row->ping <= 0) {
						return row->adrstr;
					} else {
						if ( UI_NetSourceType() == SRC_LOCAL ) {
							int nettype = row->netType;

							if (nettype < 0 || nettype >= ARRAY_LEN(netnames)) {
								nettype = 0;
							}

							Com_sprintf( hostname, sizeof(hostname), "%s [%s]",
											row->hostname,
											netnames[nettype] );
							return hostname;
						}
						else {
							Com_sprintf( hostname, sizeof(hostname), "%s", row->hostname);
							return hostname;
						}
					}
				case SORT_MAP : return row->mapname;
				case SORT_CLIENTS :
					Com_sprintf( clientBuff, sizeof(clientBuff), "%i (%i)",
									ui_browserExcludeBots.integer ? row->humanClients : row->clients,
									row->maxClients);
					return clientBuff;
				case SORT_GAME :
					if (row->gameType >= 0 && row->gameType < numTeamArenaGameTypes) {
						return teamArenaGameTypes[row->gameType];
					} else {
						return "Unknown";
					}
				case SORT_PING :
					if (row->ping <= 0) {
						return "...";
					} else {
						Com_sprintf( pingBuff, sizeof(pingBuff), "%i", row->ping);
						return pingBuff;
					}
				case SORT_MODE:
					// Trinity mode icon. The engine only carries g_mode through
					// LAN_GetServerInfo for servers it verified as Trinity, so its
					// presence in the snapshot already means "definitely Trinity".
					if ( row->mode >= MODE_VQ3 && row->mode < MODE_COUNT ) {
						*handle = uiModeIcons[row->mode];
					}
					return "";
			}
		}
	} else if (feederID == FEEDER_SERVERSTATUS) {
		if ( index >= 0 && index < uiInfo.serverStatusInfo.numLines ) {
			if ( column >= 0 && column < 4 ) {
				return uiInfo.serverStatusInfo.lines[index][column];
			}
		}
	} else if (feederID == FEEDER_FINDPLAYER) {
		if ( index >= 0 && index < uiInfo.numFoundPlayerServers ) {
			//return uiInfo.foundPlayerServerAddresses[index];
			return uiInfo.foundPlayerServerNames[index];
		}
	} else if (feederID == FEEDER_PLAYER_LIST) {
		if (index >= 0 && index < uiInfo.playerCount) {
			return uiInfo.playerNames[index];
		}
	} else if (feederID == FEEDER_TEAM_LIST) {
		if (index >= 0 && index < uiInfo.myTeamCount) {
			return uiInfo.teamNames[index];
		}
	} else if (feederID == FEEDER_MODS) {
		if (index >= 0 && index < uiInfo.modCount) {
			if (uiInfo.modList[index].modDescr && *uiInfo.modList[index].modDescr) {
				return uiInfo.modList[index].modDescr;
			} else {
				return uiInfo.modList[index].modName;
			}
		}
	} else if (feederID == FEEDER_CINEMATICS) {
		if (index >= 0 && index < uiInfo.movieCount) {
			return uiInfo.movieList[index];
		}
	} else if (feederID == FEEDER_DEMOS) {
		if (index >= 0 && index < uiInfo.demoCount) {
			return uiInfo.demoList[index];
		}
	}
	return "";
}


static qhandle_t UI_FeederItemImage(float feederID, int index) {
  if (feederID == FEEDER_HEADS) {
	int actual;
	UI_SelectedHead(index, &actual);
	index = actual;
	if (index >= 0 && index < uiInfo.characterCount) {
		if (uiInfo.characterList[index].headImage == -1) {
			uiInfo.characterList[index].headImage = trap_R_RegisterShaderNoMip(uiInfo.characterList[index].imageName);
		}
		return uiInfo.characterList[index].headImage;
	}
  } else if (feederID == FEEDER_Q3HEADS) {
    if (index >= 0 && index < uiInfo.q3HeadCount) {
      return uiInfo.q3HeadIcons[index];
    }
	} else if (feederID == FEEDER_ALLMAPS || feederID == FEEDER_MAPS) {
		int actual;
		UI_SelectedMap(index, &actual);
		index = actual;
		if (index >= 0 && index < uiInfo.mapCount) {
			if (uiInfo.mapList[index].levelShot == -1) {
				uiInfo.mapList[index].levelShot = trap_R_RegisterShaderNoMip(uiInfo.mapList[index].imageName);
			}
			return uiInfo.mapList[index].levelShot;
		}
	}
  return 0;
}

static void UI_FeederSelection(float feederID, int index) {
  if (feederID == FEEDER_HEADS) {
	int actual;
	UI_SelectedHead(index, &actual);
	index = actual;
    if (index >= 0 && index < uiInfo.characterCount) {
		trap_Cvar_Set( "team_model", va("%s", uiInfo.characterList[index].base));
		trap_Cvar_Set( "team_headmodel", va("*%s", uiInfo.characterList[index].name));
		updateModel = qtrue;
    }
  } else if (feederID == FEEDER_Q3HEADS) {
    if (index >= 0 && index < uiInfo.q3HeadCount) {
      trap_Cvar_Set( "model", uiInfo.q3HeadNames[index]);
      trap_Cvar_Set( "headmodel", uiInfo.q3HeadNames[index]);
			updateModel = qtrue;
		}
  } else if (feederID == FEEDER_MAPS || feederID == FEEDER_ALLMAPS) {
		int actual, map;
		map = (feederID == FEEDER_ALLMAPS) ? ui_currentNetMap.integer : ui_currentMap.integer;
		if (uiInfo.mapList[map].cinematic >= 0) {
		  trap_CIN_StopCinematic(uiInfo.mapList[map].cinematic);
		  uiInfo.mapList[map].cinematic = -1;
		}
		UI_SelectedMap(index, &actual);
		trap_Cvar_Set("ui_mapIndex", va("%d", index));
		ui_mapIndex.integer = index;

		if (feederID == FEEDER_MAPS) {
			ui_currentMap.integer = actual;
			trap_Cvar_Set("ui_currentMap", va("%d", actual));
	  	uiInfo.mapList[ui_currentMap.integer].cinematic = trap_CIN_PlayCinematic(va("%s.roq", uiInfo.mapList[ui_currentMap.integer].mapLoadName), 0, 0, 0, 0, (CIN_loop | CIN_silent) );
			UI_LoadBestScores(uiInfo.mapList[ui_currentMap.integer].mapLoadName, uiInfo.gameTypes[ui_gameType.integer].gtEnum);
			trap_Cvar_Set("ui_opponentModel", uiInfo.mapList[ui_currentMap.integer].opponentName);
			updateOpponentModel = qtrue;
		} else {
			ui_currentNetMap.integer = actual;
			trap_Cvar_Set("ui_currentNetMap", va("%d", actual));
	  	uiInfo.mapList[ui_currentNetMap.integer].cinematic = trap_CIN_PlayCinematic(va("%s.roq", uiInfo.mapList[ui_currentNetMap.integer].mapLoadName), 0, 0, 0, 0, (CIN_loop | CIN_silent) );
		}

  } else if (feederID == FEEDER_SERVERS) {
		const uiServerRow_t *row;
		uiInfo.serverStatus.currentServer = index;
		row = UI_SourceRow(index);
		if (uiInfo.serverStatus.currentServerCinematic >= 0) {
		  trap_CIN_StopCinematic(uiInfo.serverStatus.currentServerCinematic);
			uiInfo.serverStatus.currentServerCinematic = -1;
		}
		if (row) {
			uiInfo.serverStatus.currentServerPreview = trap_R_RegisterShaderNoMip(va("levelshots/%s", row->mapname));
			if (row->mapname[0]) {
				uiInfo.serverStatus.currentServerCinematic = trap_CIN_PlayCinematic(va("%s.roq", row->mapname), 0, 0, 0, 0, (CIN_loop | CIN_silent) );
			}
		}
  } else if (feederID == FEEDER_SERVERSTATUS) {
		//
  } else if (feederID == FEEDER_FINDPLAYER) {
	  uiInfo.currentFoundPlayerServer = index;
	  //
	  if ( index < uiInfo.numFoundPlayerServers-1) {
			// build a new server status for this server
			Q_strncpyz(uiInfo.serverStatusAddress, uiInfo.foundPlayerServerAddresses[uiInfo.currentFoundPlayerServer], sizeof(uiInfo.serverStatusAddress));
			Menu_SetFeederSelection(NULL, FEEDER_SERVERSTATUS, 0, NULL);
			UI_BuildServerStatus(qtrue);
	  }
  } else if (feederID == FEEDER_PLAYER_LIST) {
		uiInfo.playerIndex = index;
  } else if (feederID == FEEDER_TEAM_LIST) {
		uiInfo.teamIndex = index;
  } else if (feederID == FEEDER_MODS) {
		uiInfo.modIndex = index;
  } else if (feederID == FEEDER_CINEMATICS) {
		uiInfo.movieIndex = index;
		if (uiInfo.previewMovie >= 0) {
		  trap_CIN_StopCinematic(uiInfo.previewMovie);
		}
		uiInfo.previewMovie = -1;
  } else if (feederID == FEEDER_DEMOS) {
		uiInfo.demoIndex = index;
	}
}

static qboolean Team_Parse(char **p) {
  char *token;
  const char *tempStr;
	int i;

  token = COM_ParseExt(p, qtrue);

  if (token[0] != '{') {
    return qfalse;
  }

  while ( 1 ) {

    token = COM_ParseExt(p, qtrue);
    
    if (Q_stricmp(token, "}") == 0) {
      return qtrue;
    }

    if ( !token || token[0] == 0 ) {
      return qfalse;
    }

    if (token[0] == '{') {
      // seven tokens per line, team name and icon, and 5 team member names
      if (!String_Parse(p, &uiInfo.teamList[uiInfo.teamCount].teamName) || !String_Parse(p, &tempStr)) {
        return qfalse;
      }
    

			uiInfo.teamList[uiInfo.teamCount].imageName = tempStr;
	    uiInfo.teamList[uiInfo.teamCount].teamIcon = trap_R_RegisterShaderNoMip(uiInfo.teamList[uiInfo.teamCount].imageName);
		  uiInfo.teamList[uiInfo.teamCount].teamIcon_Metal = trap_R_RegisterShaderNoMip(va("%s_metal",uiInfo.teamList[uiInfo.teamCount].imageName));
			uiInfo.teamList[uiInfo.teamCount].teamIcon_Name = trap_R_RegisterShaderNoMip(va("%s_name", uiInfo.teamList[uiInfo.teamCount].imageName));

			uiInfo.teamList[uiInfo.teamCount].cinematic = -1;

			for (i = 0; i < TEAM_MEMBERS; i++) {
				uiInfo.teamList[uiInfo.teamCount].teamMembers[i] = NULL;
				if (!String_Parse(p, &uiInfo.teamList[uiInfo.teamCount].teamMembers[i])) {
					return qfalse;
				}
			}

      Com_Printf("Loaded team %s with team icon %s.\n", uiInfo.teamList[uiInfo.teamCount].teamName, tempStr);
      if (uiInfo.teamCount < MAX_TEAMS) {
        uiInfo.teamCount++;
      } else {
        Com_Printf("Too many teams, last team replaced!\n");
      }
      token = COM_ParseExt(p, qtrue);
      if (token[0] != '}') {
        return qfalse;
      }
    }
  }

  return qfalse;
}

static qboolean Character_Parse(char **p) {
  char *token;
  const char *tempStr;

  token = COM_ParseExt(p, qtrue);

  if (token[0] != '{') {
    return qfalse;
  }


  while ( 1 ) {
    token = COM_ParseExt(p, qtrue);

    if (Q_stricmp(token, "}") == 0) {
      return qtrue;
    }

    if ( !token || token[0] == 0 ) {
      return qfalse;
    }

    if (token[0] == '{') {
      // two tokens per line, character name and sex
      if (!String_Parse(p, &uiInfo.characterList[uiInfo.characterCount].name) || !String_Parse(p, &tempStr)) {
        return qfalse;
      }
    
      uiInfo.characterList[uiInfo.characterCount].headImage = -1;
			uiInfo.characterList[uiInfo.characterCount].imageName = String_Alloc(va("models/players/heads/%s/icon_default.tga", uiInfo.characterList[uiInfo.characterCount].name));

	  if (tempStr && (!Q_stricmp(tempStr, "female"))) {
        uiInfo.characterList[uiInfo.characterCount].base = String_Alloc(va("Janet"));
      } else if (tempStr && (!Q_stricmp(tempStr, "male"))) {
        uiInfo.characterList[uiInfo.characterCount].base = String_Alloc(va("James"));
	  } else {
        uiInfo.characterList[uiInfo.characterCount].base = String_Alloc(va("%s",tempStr));
	  }

      Com_Printf("Loaded %s character %s.\n", uiInfo.characterList[uiInfo.characterCount].base, uiInfo.characterList[uiInfo.characterCount].name);
      if (uiInfo.characterCount < MAX_HEADS) {
        uiInfo.characterCount++;
      } else {
        Com_Printf("Too many characters, last character replaced!\n");
      }
     
      token = COM_ParseExt(p, qtrue);
      if (token[0] != '}') {
        return qfalse;
      }
    }
  }

  return qfalse;
}


static qboolean Alias_Parse(char **p) {
  char *token;

  token = COM_ParseExt(p, qtrue);

  if (token[0] != '{') {
    return qfalse;
  }

  while ( 1 ) {
    token = COM_ParseExt(p, qtrue);

    if (Q_stricmp(token, "}") == 0) {
      return qtrue;
    }

    if ( !token || token[0] == 0 ) {
      return qfalse;
    }

    if (token[0] == '{') {
      // three tokens per line, character name, bot alias, and preferred action a - all purpose, d - defense, o - offense
      if (!String_Parse(p, &uiInfo.aliasList[uiInfo.aliasCount].name) || !String_Parse(p, &uiInfo.aliasList[uiInfo.aliasCount].ai) || !String_Parse(p, &uiInfo.aliasList[uiInfo.aliasCount].action)) {
        return qfalse;
      }
    
      Com_Printf("Loaded character alias %s using character ai %s.\n", uiInfo.aliasList[uiInfo.aliasCount].name, uiInfo.aliasList[uiInfo.aliasCount].ai);
      if (uiInfo.aliasCount < MAX_ALIASES) {
        uiInfo.aliasCount++;
      } else {
        Com_Printf("Too many aliases, last alias replaced!\n");
      }
     
      token = COM_ParseExt(p, qtrue);
      if (token[0] != '}') {
        return qfalse;
      }
    }
  }

  return qfalse;
}



// mode 
// 0 - high level parsing
// 1 - team parsing
// 2 - character parsing
static void UI_ParseTeamInfo(const char *teamFile) {
	char	*token;
  char *p;
  char *buff = NULL;
  //static int mode = 0; TTimo: unused

  buff = GetMenuBuffer(teamFile);
  if (!buff) {
    return;
  }

  p = buff;

	while ( 1 ) {
		token = COM_ParseExt( &p, qtrue );
		if( !token || token[0] == 0 || token[0] == '}') {
			break;
		}

		if ( Q_stricmp( token, "}" ) == 0 ) {
      break;
    }

    if (Q_stricmp(token, "teams") == 0) {

      if (Team_Parse(&p)) {
        continue;
      } else {
        break;
      }
    }

    if (Q_stricmp(token, "characters") == 0) {
      Character_Parse(&p);
    }

    if (Q_stricmp(token, "aliases") == 0) {
      Alias_Parse(&p);
    }

  }

}


static qboolean GameType_Parse(char **p, qboolean join) {
	char *token;

	token = COM_ParseExt(p, qtrue);

	if (token[0] != '{') {
		return qfalse;
	}

	if (join) {
		uiInfo.numJoinGameTypes = 0;
	} else {
		uiInfo.numGameTypes = 0;
	}

	while ( 1 ) {
		token = COM_ParseExt(p, qtrue);

		if (Q_stricmp(token, "}") == 0) {
			return qtrue;
		}

		if ( !token || token[0] == 0 ) {
			return qfalse;
		}

		if (token[0] == '{') {
			// two tokens per line, character name and sex
			if (join) {
				if (!String_Parse(p, &uiInfo.joinGameTypes[uiInfo.numJoinGameTypes].gameType) || !Int_Parse(p, &uiInfo.joinGameTypes[uiInfo.numJoinGameTypes].gtEnum)) {
					return qfalse;
				}
			} else {
				if (!String_Parse(p, &uiInfo.gameTypes[uiInfo.numGameTypes].gameType) || !Int_Parse(p, &uiInfo.gameTypes[uiInfo.numGameTypes].gtEnum)) {
					return qfalse;
				}
			}
    
			if (join) {
				if (uiInfo.numJoinGameTypes < MAX_GAMETYPES) {
					uiInfo.numJoinGameTypes++;
				} else {
					Com_Printf("Too many net game types, last one replace!\n");
				}		
			} else {
				if (uiInfo.numGameTypes < MAX_GAMETYPES) {
					uiInfo.numGameTypes++;
				} else {
					Com_Printf("Too many game types, last one replace!\n");
				}		
			}
     
			token = COM_ParseExt(p, qtrue);
			if (token[0] != '}') {
				return qfalse;
			}
		}
	}
	return qfalse;
}

static qboolean MapList_Parse(char **p) {
	char *token;

	token = COM_ParseExt(p, qtrue);

	if (token[0] != '{') {
		return qfalse;
	}

	uiInfo.mapCount = 0;

	while ( 1 ) {
		token = COM_ParseExt(p, qtrue);

		if (Q_stricmp(token, "}") == 0) {
			return qtrue;
		}

		if ( !token || token[0] == 0 ) {
			return qfalse;
		}

		if (token[0] == '{') {
			if (!String_Parse(p, &uiInfo.mapList[uiInfo.mapCount].mapName) || !String_Parse(p, &uiInfo.mapList[uiInfo.mapCount].mapLoadName) 
				||!Int_Parse(p, &uiInfo.mapList[uiInfo.mapCount].teamMembers) ) {
				return qfalse;
			}

			if (!String_Parse(p, &uiInfo.mapList[uiInfo.mapCount].opponentName)) {
				return qfalse;
			}

			uiInfo.mapList[uiInfo.mapCount].typeBits = 0;

			while (1) {
				token = COM_ParseExt(p, qtrue);
				if (token[0] >= '0' && token[0] <= '9') {
					uiInfo.mapList[uiInfo.mapCount].typeBits |= (1 << (token[0] - 0x030));
					if (!Int_Parse(p, &uiInfo.mapList[uiInfo.mapCount].timeToBeat[token[0] - 0x30])) {
						return qfalse;
					}
				} else {
					break;
				} 
			}

			//mapList[mapCount].imageName = String_Alloc(va("levelshots/%s", mapList[mapCount].mapLoadName));
			//if (uiInfo.mapCount == 0) {
			  // only load the first cinematic, selection loads the others
  			//  uiInfo.mapList[uiInfo.mapCount].cinematic = trap_CIN_PlayCinematic(va("%s.roq",uiInfo.mapList[uiInfo.mapCount].mapLoadName), qfalse, qfalse, qtrue, 0, 0, 0, 0);
			//}
  		uiInfo.mapList[uiInfo.mapCount].cinematic = -1;
			uiInfo.mapList[uiInfo.mapCount].levelShot = trap_R_RegisterShaderNoMip(va("levelshots/%s_small", uiInfo.mapList[uiInfo.mapCount].mapLoadName));

			if (uiInfo.mapCount < MAX_MAPS) {
				uiInfo.mapCount++;
			} else {
				Com_Printf("Too many maps, last one replaced!\n");
			}
		}
	}
	return qfalse;
}

static void UI_ParseGameInfo(const char *teamFile) {
	char	*token;
	char *p;
	char *buff = NULL;
	//int mode = 0; TTimo: unused

	buff = GetMenuBuffer(teamFile);
	if (!buff) {
		return;
	}

	p = buff;

	while ( 1 ) {
		token = COM_ParseExt( &p, qtrue );
		if( !token || token[0] == 0 || token[0] == '}') {
			break;
		}

		if ( Q_stricmp( token, "}" ) == 0 ) {
			break;
		}

		if (Q_stricmp(token, "gametypes") == 0) {

			if (GameType_Parse(&p, qfalse)) {
				continue;
			} else {
				break;
			}
		}

		if (Q_stricmp(token, "joingametypes") == 0) {

			if (GameType_Parse(&p, qtrue)) {
				continue;
			} else {
				break;
			}
		}

		if (Q_stricmp(token, "maps") == 0) {
			// start a new menu
			MapList_Parse(&p);
		}

	}
}

static void UI_Pause(qboolean b) {
	if (b) {
		// pause the game and set the ui keycatcher
	  trap_Cvar_Set( "cl_paused", "1" );
		trap_Key_SetCatcher( KEYCATCH_UI );
	} else {
		// unpause the game and clear the ui keycatcher
		trap_Key_SetCatcher( trap_Key_GetCatcher() & ~KEYCATCH_UI );
		trap_Key_ClearStates();
		trap_Cvar_Set( "cl_paused", "0" );
	}
}

#ifndef MISSIONPACK // bk001206
static int UI_OwnerDraw_Width(int ownerDraw) {
  // bk001205 - LCC missing return value
  return 0;
}
#endif

static int UI_PlayCinematic(const char *name, float x, float y, float w, float h) {
  return trap_CIN_PlayCinematic(name, x, y, w, h, (CIN_loop | CIN_silent));
}

static void UI_StopCinematic(int handle) {
	if (handle >= 0) {
	  trap_CIN_StopCinematic(handle);
	} else {
		handle = abs(handle);
		if (handle == UI_MAPCINEMATIC) {
			if (uiInfo.mapList[ui_currentMap.integer].cinematic >= 0) {
			  trap_CIN_StopCinematic(uiInfo.mapList[ui_currentMap.integer].cinematic);
			  uiInfo.mapList[ui_currentMap.integer].cinematic = -1;
			}
		} else if (handle == UI_NETMAPCINEMATIC) {
			if (uiInfo.serverStatus.currentServerCinematic >= 0) {
			  trap_CIN_StopCinematic(uiInfo.serverStatus.currentServerCinematic);
				uiInfo.serverStatus.currentServerCinematic = -1;
			}
		} else if (handle == UI_CLANCINEMATIC) {
		  int i = UI_TeamIndexFromName(UI_Cvar_VariableString("ui_teamName"));
		  if (i >= 0 && i < uiInfo.teamCount) {
				if (uiInfo.teamList[i].cinematic >= 0) {
				  trap_CIN_StopCinematic(uiInfo.teamList[i].cinematic);
					uiInfo.teamList[i].cinematic = -1;
				}
			}
		}
	}
}

/*
=================
UI_CIN_SetExtents

Wrapper for trap_CIN_SetExtents that adjusts width for widescreen
to prevent horizontal stretching of cinematic videos.
=================
*/
static void UI_CIN_SetExtents(int handle, int x, int y, int w, int h) {
	// Adjust for widescreen to prevent horizontal stretching
	if ( uiInfo.uiDC.glconfig.vidWidth * 480 > uiInfo.uiDC.glconfig.vidHeight * 640 ) {
		float aspectCorrection = (uiInfo.uiDC.glconfig.vidHeight * 640.0f) / (uiInfo.uiDC.glconfig.vidWidth * 480.0f);
		float newW = w * aspectCorrection;
		float newX = x * aspectCorrection;
		// Offset to account for the centered 4:3 area
		newX += (640.0f - 640.0f * aspectCorrection) * 0.5f;
		x = (int)(newX + 0.5f);
		w = (int)(newW + 0.5f);
	}
	trap_CIN_SetExtents(handle, x, y, w, h);
}

static void UI_DrawCinematic(int handle, float x, float y, float w, float h) {
	if ( vrActive ) {
		// The engine re-virtualizes CIN extents through its own 640->framebuffer
		// transform (virtual-screen 4:3 box + optical center), so it expects
		// 640-virtual coords: apply the UI transform, then inverse-map back.
		UI_AdjustFrom640( &x, &y, &w, &h );

		x *= SCREEN_WIDTH / (float)uiInfo.uiDC.glconfig.vidWidth;
		w *= SCREEN_WIDTH / (float)uiInfo.uiDC.glconfig.vidWidth;

		if ( vr->virtual_screen ) {
			float viewableHeight = uiInfo.uiDC.glconfig.vidWidth * 0.75f;
			float yoffset = ( uiInfo.uiDC.glconfig.vidHeight - viewableHeight ) / 2.0f;
			float tanUp = tan( vr->fov_angle_up );
			float tanDown = tan( vr->fov_angle_down );
			float tanHeight = tanUp - tanDown;
			if ( fabs( tanHeight ) > 0.001f ) {
				float m9 = ( tanUp + tanDown ) / tanHeight;
				float opticalOffsetVirtual = 240.0f * m9;
				float yscale = viewableHeight / 480.0f;
				yoffset += opticalOffsetVirtual * yscale;
			}
			y = ( y - yoffset ) * ( SCREEN_HEIGHT / viewableHeight );
			h *= SCREEN_HEIGHT / viewableHeight;
		} else {
			y *= SCREEN_HEIGHT / (float)uiInfo.uiDC.glconfig.vidHeight;
			h *= SCREEN_HEIGHT / (float)uiInfo.uiDC.glconfig.vidHeight;
		}

		trap_CIN_SetExtents( handle, x, y, w, h );
	} else {
		UI_CIN_SetExtents( handle, x, y, w, h );
	}
	trap_CIN_DrawCinematic( handle );
}

static void UI_RunCinematicFrame(int handle) {
  trap_CIN_RunCinematic(handle);
}



/*
=================
PlayerModel_BuildList
=================
*/
static void UI_BuildQ3Model_List( void )
{
	int		numdirs;
	int		numfiles;
	char	dirlist[2048];
	char	filelist[2048];
	char	skinname[64];
	char	scratch[256];
	char*	dirptr;
	char*	fileptr;
	int		i;
	int		j, k, dirty;
	int		dirlen;
	int		filelen;

	uiInfo.q3HeadCount = 0;

	// iterate directory of all player models
	numdirs = trap_FS_GetFileList("models/players", "/", dirlist, 2048 );
	dirptr  = dirlist;
	for (i=0; i<numdirs && uiInfo.q3HeadCount < MAX_PLAYERMODELS; i++,dirptr+=dirlen+1)
	{
		dirlen = strlen(dirptr);
		
		if (dirlen && dirptr[dirlen-1]=='/') dirptr[dirlen-1]='\0';

		if (!strcmp(dirptr,".") || !strcmp(dirptr,".."))
			continue;
			
		// iterate all skin files in directory
		numfiles = trap_FS_GetFileList( va("models/players/%s",dirptr), "tga", filelist, 2048 );
		fileptr  = filelist;
		for (j=0; j<numfiles && uiInfo.q3HeadCount < MAX_PLAYERMODELS;j++,fileptr+=filelen+1)
		{
			filelen = strlen(fileptr);

			COM_StripExtension(fileptr, skinname, sizeof(skinname));

			// look for icon_????
			if (Q_stricmpn(skinname, "icon_", 5) == 0 && !(Q_stricmp(skinname,"icon_blue") == 0 || Q_stricmp(skinname,"icon_red") == 0))
			{
				if (Q_stricmp(skinname, "icon_default") == 0) {
					Com_sprintf( scratch, sizeof(scratch), dirptr);
				} else {
					Com_sprintf( scratch, sizeof(scratch), "%s/%s",dirptr, skinname + 5);
				}
				dirty = 0;
				for(k=0;k<uiInfo.q3HeadCount;k++) {
					if (!Q_stricmp(scratch, uiInfo.q3HeadNames[uiInfo.q3HeadCount])) {
						dirty = 1;
						break;
					}
				}
				if (!dirty) {
					Com_sprintf( uiInfo.q3HeadNames[uiInfo.q3HeadCount], sizeof(uiInfo.q3HeadNames[uiInfo.q3HeadCount]), scratch);
					uiInfo.q3HeadIcons[uiInfo.q3HeadCount++] = trap_R_RegisterShaderNoMip(va("models/players/%s/%s",dirptr,skinname));
				}
			}

		}
	}	

}



/*
=================
UI_Init
=================
*/
void _UI_Init( qboolean inGameLoad ) {
	const char *menuSet;
	int start;

	//uiInfo.inGameLoad = inGameLoad;

	UI_VR_Init();

	UI_RegisterCvars();
	UI_InitMemory();

	// the browser's source list must exist before anything draws it
	UI_BuildBrowserSources();

	// cache redundant calulations
	UI_VideoCheck( -99999 );

  //UI_Load();
	uiInfo.uiDC.registerShaderNoMip = &trap_R_RegisterShaderNoMip;
	uiInfo.uiDC.setColor = &UI_SetColor;
	uiInfo.uiDC.drawHandlePic = &UI_DrawHandlePic;
	uiInfo.uiDC.drawStretchPic = &trap_R_DrawStretchPic;
	uiInfo.uiDC.drawText = &Text_Paint;
	uiInfo.uiDC.textWidth = &Text_Width;
	uiInfo.uiDC.textHeight = &Text_Height;
	uiInfo.uiDC.registerModel = &trap_R_RegisterModel;
	uiInfo.uiDC.modelBounds = &trap_R_ModelBounds;
	uiInfo.uiDC.fillRect = &UI_FillRect;
	uiInfo.uiDC.drawRect = &_UI_DrawRect;
	uiInfo.uiDC.drawSides = &_UI_DrawSides;
	uiInfo.uiDC.drawTopBottom = &_UI_DrawTopBottom;
	uiInfo.uiDC.clearScene = &trap_R_ClearScene;
	uiInfo.uiDC.addRefEntityToScene = &trap_R_AddRefEntityToScene;
	uiInfo.uiDC.renderScene = &trap_R_RenderScene;
	uiInfo.uiDC.registerFont = &trap_R_RegisterFont;
	uiInfo.uiDC.ownerDrawItem = &UI_OwnerDraw;
	uiInfo.uiDC.getValue = &UI_GetValue;
	uiInfo.uiDC.ownerDrawVisible = &UI_OwnerDrawVisible;
	uiInfo.uiDC.runScript = &UI_RunMenuScript;
	uiInfo.uiDC.getTeamColor = &UI_GetTeamColor;
	uiInfo.uiDC.setCVar = trap_Cvar_Set;
	uiInfo.uiDC.getCVarString = trap_Cvar_VariableStringBuffer;
	uiInfo.uiDC.getClipboardData = trap_GetClipboardData;
	uiInfo.uiDC.getCVarValue = trap_Cvar_VariableValue;
	uiInfo.uiDC.drawTextWithCursor = &Text_PaintWithCursor;
	uiInfo.uiDC.setOverstrikeMode = &trap_Key_SetOverstrikeMode;
	uiInfo.uiDC.getOverstrikeMode = &trap_Key_GetOverstrikeMode;
	uiInfo.uiDC.startLocalSound = &trap_S_StartLocalSound;
	uiInfo.uiDC.ownerDrawHandleKey = &UI_OwnerDrawHandleKey;
	uiInfo.uiDC.feederCount = &UI_FeederCount;
	uiInfo.uiDC.feederItemImage = &UI_FeederItemImage;
	uiInfo.uiDC.feederItemText = &UI_FeederItemText;
	uiInfo.uiDC.feederSelection = &UI_FeederSelection;
	uiInfo.uiDC.setBinding = &trap_Key_SetBinding;
	uiInfo.uiDC.getBindingBuf = &trap_Key_GetBindingBuf;
	uiInfo.uiDC.keynumToStringBuf = &trap_Key_KeynumToStringBuf;
	uiInfo.uiDC.executeText = &trap_Cmd_ExecuteText;
	uiInfo.uiDC.Error = &Com_Error; 
	uiInfo.uiDC.Print = &Com_Printf; 
	uiInfo.uiDC.Pause = &UI_Pause;
	uiInfo.uiDC.ownerDrawWidth = &UI_OwnerDrawWidth;
	uiInfo.uiDC.registerSound = &trap_S_RegisterSound;
	uiInfo.uiDC.startBackgroundTrack = &trap_S_StartBackgroundTrack;
	uiInfo.uiDC.stopBackgroundTrack = &trap_S_StopBackgroundTrack;
	uiInfo.uiDC.playCinematic = &UI_PlayCinematic;
	uiInfo.uiDC.stopCinematic = &UI_StopCinematic;
	uiInfo.uiDC.drawCinematic = &UI_DrawCinematic;
	uiInfo.uiDC.runCinematicFrame = &UI_RunCinematicFrame;
	uiInfo.uiDC.drawTextWithCursor_NoColorEscape = &Text_PaintWithCursor_NoColorEscape;
	uiInfo.uiDC.vkeyboardShow = &UI_VKeyboardShow;
	uiInfo.uiDC.vkeyboardHide = &UI_VKeyboardHide;
	uiInfo.uiDC.vkeyboardIsActive = &UI_VKeyboardIsActive;
	uiInfo.uiDC.vkeyboardHandleKey = &UI_VKeyboardHandleKey;
	uiInfo.uiDC.vrMenuMove = &UI_VR_OnMenuMove;

	Init_Display(&uiInfo.uiDC);

	String_Init();
  
	uiInfo.uiDC.cursor	= trap_R_RegisterShaderNoMip( "menu/art/3_cursor2" );
	uiInfo.uiDC.whiteShader = trap_R_RegisterShaderNoMip( "white" );

	AssetCache();

	start = trap_Milliseconds();

  uiInfo.teamCount = 0;
  uiInfo.characterCount = 0;
  uiInfo.aliasCount = 0;

#ifdef PRE_RELEASE_TADEMO
	UI_ParseTeamInfo("demoteaminfo.txt");
	UI_ParseGameInfo("demogameinfo.txt");
#else
	UI_ParseTeamInfo("teaminfo.txt");
	UI_LoadTeams();
	UI_ParseGameInfo("gameinfo.txt");
#endif

	menuSet = UI_Cvar_VariableString("ui_menuFiles");
	if (menuSet == NULL || menuSet[0] == '\0') {
		menuSet = "ui/menus.txt";
	}

#if 0
	if (uiInfo.inGameLoad) {
		UI_LoadMenus("ui/ingame.txt", qtrue);
	} else { // bk010222: left this: UI_LoadMenus(menuSet, qtrue);
	}
#else 
	UI_LoadMenus(menuSet, qtrue);
	UI_LoadMenus("ui/ingame.txt", qfalse);
#endif
	UI_VR_LoadMenus();

	Menus_CloseAll();

	trap_LAN_LoadCachedServers();
	UI_LoadBestScores(uiInfo.mapList[ui_currentMap.integer].mapLoadName, uiInfo.gameTypes[ui_gameType.integer].gtEnum);

	UI_BuildQ3Model_List();
	UI_LoadBots();

	// sets defaults for ui temp cvars
	uiInfo.effectsColor = gamecodetoui[(int)trap_Cvar_VariableValue("color1")-1];
	uiInfo.currentCrosshair = (int)trap_Cvar_VariableValue("cg_drawCrosshair");
	trap_Cvar_Set("ui_mousePitch", (trap_Cvar_VariableValue("m_pitch") >= 0) ? "0" : "1");

	uiInfo.serverStatus.currentServerCinematic = -1;
	uiInfo.previewMovie = -1;

	if (trap_Cvar_VariableValue("ui_TeamArenaFirstRun") == 0) {
		trap_Cvar_Set("s_volume", "0.8");
		trap_Cvar_Set("s_musicvolume", "0.5");
		trap_Cvar_Set("ui_TeamArenaFirstRun", "1");
	}

	trap_Cvar_Register(NULL, "debug_protocol", "", 0 );

	trap_Cvar_Set("ui_actualNetGameType", va("%d", ui_netGameType.integer));

	UI_UpdateHDRAvail();
	UI_UpdateVRActive();
}


/*
=================
UI_KeyEvent
=================
*/
void _UI_KeyEvent( int key, qboolean down ) {

  if (Menu_Count() > 0) {
    menuDef_t *menu = Menu_GetFocused();
		if (menu) {
			if (key == K_ESCAPE && down && !Menus_AnyFullScreenVisible()) {
				Menus_CloseAll();
			} else {
				Menu_HandleKey(menu, key, down );
			}
		} else {
			trap_Key_SetCatcher( trap_Key_GetCatcher() & ~KEYCATCH_UI );
			trap_Key_ClearStates();
			trap_Cvar_Set( "cl_paused", "0" );
		}
  }

  //if ((s > 0) && (s != menu_null_sound)) {
	//  trap_S_StartLocalSound( s, CHAN_LOCAL_SOUND );
  //}
}

/*
=================
UI_MouseEvent
=================
*/
void _UI_MouseEvent( int dx, int dy )
{
	if ( UI_VR_StickNavActive() ) {
		return;   // thumbstick nav owns selection; ignore ray hover
	}

	UI_VR_CursorOverride( &uiInfo.uiDC.cursorx, &uiInfo.uiDC.cursory );

	// update virtual mouse cursor coordinates
	uiInfo.uiDC.cursorx += dx * uiInfo.uiDC.cursorScaleR;
	uiInfo.uiDC.cursory += dy * uiInfo.uiDC.cursorScaleR;

	// clamp virtual coordinates
	if ( uiInfo.uiDC.cursorx < uiInfo.uiDC.screenXmin )
		uiInfo.uiDC.cursorx = uiInfo.uiDC.screenXmin;
	else if ( uiInfo.uiDC.cursorx > uiInfo.uiDC.screenXmax )
		uiInfo.uiDC.cursorx = uiInfo.uiDC.screenXmax;

	if ( uiInfo.uiDC.cursory < uiInfo.uiDC.screenYmin )
		uiInfo.uiDC.cursory = uiInfo.uiDC.screenYmin;
	else if ( uiInfo.uiDC.cursory > uiInfo.uiDC.screenYmax )
		uiInfo.uiDC.cursory = uiInfo.uiDC.screenYmax;

  if (Menu_Count() > 0) {
    //menuDef_t *menu = Menu_GetFocused();
    //Menu_HandleMouseMove(menu, uiInfo.uiDC.cursorx, uiInfo.uiDC.cursory);
		Display_MouseMove(NULL, uiInfo.uiDC.cursorx, uiInfo.uiDC.cursory);
  }

}

void UI_LoadNonIngame() {
	const char *menuSet = UI_Cvar_VariableString("ui_menuFiles");
	if (menuSet == NULL || menuSet[0] == '\0') {
		menuSet = "ui/menus.txt";
	}
	UI_LoadMenus(menuSet, qfalse);
	uiInfo.inGameLoad = qfalse;
}

void _UI_SetActiveMenu( uiMenuCommand_t menu ) {
	char buf[256];

	// this should be the ONLY way the menu system is brought up
	// enusure minumum menu data is cached
  if (Menu_Count() > 0) {
		vec3_t v;
		v[0] = v[1] = v[2] = 0;
	  switch ( menu ) {
	  case UIMENU_NONE:
			trap_Key_SetCatcher( trap_Key_GetCatcher() & ~KEYCATCH_UI );
			trap_Key_ClearStates();
			trap_Cvar_Set( "cl_paused", "0" );
			Menus_CloseAll();

		  return;
	  case UIMENU_MAIN:
			//trap_Cvar_Set( "sv_killserver", "1" );
			trap_Key_SetCatcher( KEYCATCH_UI );
			//trap_S_StartLocalSound( trap_S_RegisterSound("sound/misc/menu_background.wav", qfalse) , CHAN_LOCAL_SOUND );
			//trap_S_StartBackgroundTrack("sound/misc/menu_background.wav", NULL);
			if (uiInfo.inGameLoad) {
				UI_LoadNonIngame();
			}
			Menus_CloseAll();
			Menus_ActivateByName("main");
			trap_Cvar_VariableStringBuffer("com_errorMessage", buf, sizeof(buf));
			if (strlen(buf)) {
				if (!ui_singlePlayerActive.integer) {
					Menus_ActivateByName("error_popmenu");
				} else {
					trap_Cvar_Set("com_errorMessage", "");
				}
			}
		  return;
	  case UIMENU_TEAM:
			trap_Key_SetCatcher( KEYCATCH_UI );
      Menus_ActivateByName("team");
		  return;
	  case UIMENU_NEED_CD:
			// no cd check in TA
			//trap_Key_SetCatcher( KEYCATCH_UI );
      //Menus_ActivateByName("needcd");
		  //UI_ConfirmMenu( "Insert the CD", NULL, NeedCDAction );
		  return;
	  case UIMENU_BAD_CD_KEY:
			// no cd check in TA
			//trap_Key_SetCatcher( KEYCATCH_UI );
      //Menus_ActivateByName("badcd");
		  //UI_ConfirmMenu( "Bad CD Key", NULL, NeedCDKeyAction );
		  return;
	  case UIMENU_POSTGAME:
			//trap_Cvar_Set( "sv_killserver", "1" );
			trap_Key_SetCatcher( KEYCATCH_UI );
			if (uiInfo.inGameLoad) {
				UI_LoadNonIngame();
			}
			Menus_CloseAll();
			Menus_ActivateByName("endofgame");
		  //UI_ConfirmMenu( "Bad CD Key", NULL, NeedCDKeyAction );
		  return;
	  case UIMENU_INGAME:
		  trap_Cvar_Set( "cl_paused", "1" );
			trap_Key_SetCatcher( KEYCATCH_UI );
			UI_BuildPlayerList();
			Menus_CloseAll();
			Menus_ActivateByName("ingame");
		  return;
	  }
  }
}

qboolean _UI_IsFullscreen( void ) {
	return Menus_AnyFullScreenVisible();
}



static connstate_t	lastConnState;
static char			lastLoadingText[MAX_INFO_VALUE];

static void UI_ReadableSize ( char *buf, int bufsize, int value )
{
	if (value > 1024*1024*1024 ) { // gigs
		Com_sprintf( buf, bufsize, "%d", value / (1024*1024*1024) );
		Com_sprintf( buf+strlen(buf), bufsize-strlen(buf), ".%02d GB", 
			(value % (1024*1024*1024))*100 / (1024*1024*1024) );
	} else if (value > 1024*1024 ) { // megs
		Com_sprintf( buf, bufsize, "%d", value / (1024*1024) );
		Com_sprintf( buf+strlen(buf), bufsize-strlen(buf), ".%02d MB", 
			(value % (1024*1024))*100 / (1024*1024) );
	} else if (value > 1024 ) { // kilos
		Com_sprintf( buf, bufsize, "%d KB", value / 1024 );
	} else { // bytes
		Com_sprintf( buf, bufsize, "%d bytes", value );
	}
}

// Assumes time is in msec
static void UI_PrintTime ( char *buf, int bufsize, int time ) {
	time /= 1000;  // change to seconds

	if (time > 3600) { // in the hours range
		Com_sprintf( buf, bufsize, "%d hr %d min", time / 3600, (time % 3600) / 60 );
	} else if (time > 60) { // mins
		Com_sprintf( buf, bufsize, "%d min %d sec", time / 60, time % 60 );
	} else  { // secs
		Com_sprintf( buf, bufsize, "%d sec", time );
	}
}

void Text_PaintCenter(float x, float y, float scale, vec4_t color, const char *text, float adjust) {
	int len = Text_Width(text, scale, 0);
	Text_Paint(x - len / 2, y, scale, color, text, 0, 0, ITEM_TEXTSTYLE_SHADOWEDMORE);
}

void Text_PaintCenter_AutoWrapped(float x, float y, float xmax, float ystep, float scale, vec4_t color, const char *str, float adjust) {
	int width;
	char *s1,*s2,*s3;
	char c_bcp;
	char buf[1024];

	if (!str || str[0]=='\0')
		return;

	Q_strncpyz(buf, str, sizeof(buf));
	s1 = s2 = s3 = buf;

	while (1) {
		do {
			s3++;
		} while (*s3!=' ' && *s3!='\0');
		c_bcp = *s3;
		*s3 = '\0';
		width = Text_Width(s1, scale, 0);
		*s3 = c_bcp;
		if (width > xmax) {
			if (s1==s2)
			{
				// fuck, don't have a clean cut, we'll overflow
				s2 = s3;
			}
			*s2 = '\0';
			Text_PaintCenter(x, y, scale, color, s1, adjust);
			y += ystep;
			if (c_bcp == '\0')
      {
				// that was the last word
        // we could start a new loop, but that wouldn't be much use
        // even if the word is too long, we would overflow it (see above)
        // so just print it now if needed
        s2++;
        if (*s2 != '\0') // if we are printing an overflowing line we have s2 == s3
          Text_PaintCenter(x, y, scale, color, s2, adjust);
        break;
      }
			s2++;
			s1 = s2;
			s3 = s2;
		}
		else
		{
			s2 = s3;
			if (c_bcp == '\0') // we reached the end
			{
				Text_PaintCenter(x, y, scale, color, s1, adjust);
				break;
			}
		}
	}
}

static void UI_DisplayDownloadInfo( const char *downloadName, float centerPoint, float yStart, float scale ) {
	static char dlText[]	= "Downloading:";
	static char etaText[]	= "Estimated time left:";
	static char xferText[]	= "Transfer rate:";

	int downloadSize, downloadCount, downloadTime;
	char dlSizeBuf[64], totalSizeBuf[64], xferRateBuf[64], dlTimeBuf[64];
	int xferRate;
	int leftWidth;
	const char *s;

	downloadSize = trap_Cvar_VariableValue( "cl_downloadSize" );
	downloadCount = trap_Cvar_VariableValue( "cl_downloadCount" );
	downloadTime = trap_Cvar_VariableValue( "cl_downloadTime" );

	leftWidth = 320;

	UI_SetColor(colorWhite);
	Text_PaintCenter(centerPoint, yStart + 112, scale, colorWhite, dlText, 0);
	Text_PaintCenter(centerPoint, yStart + 192, scale, colorWhite, etaText, 0);
	Text_PaintCenter(centerPoint, yStart + 248, scale, colorWhite, xferText, 0);

	if (downloadSize > 0) {
		s = va( "%s (%d%%)", downloadName, downloadCount * 100 / downloadSize );
	} else {
		s = downloadName;
	}

	Text_PaintCenter(centerPoint, yStart+136, scale, colorWhite, s, 0);

	UI_ReadableSize( dlSizeBuf,		sizeof dlSizeBuf,		downloadCount );
	UI_ReadableSize( totalSizeBuf,	sizeof totalSizeBuf,	downloadSize );

	if (downloadCount < 4096 || !downloadTime) {
		Text_PaintCenter(leftWidth, yStart+216, scale, colorWhite, "estimating", 0);
		Text_PaintCenter(leftWidth, yStart+160, scale, colorWhite, va("(%s of %s copied)", dlSizeBuf, totalSizeBuf), 0);
	} else {
		if ((uiInfo.uiDC.realTime - downloadTime) / 1000) {
			xferRate = downloadCount / ((uiInfo.uiDC.realTime - downloadTime) / 1000);
		} else {
			xferRate = 0;
		}
		UI_ReadableSize( xferRateBuf, sizeof xferRateBuf, xferRate );

		// Extrapolate estimated completion time
		if (downloadSize && xferRate) {
			int n = downloadSize / xferRate; // estimated time for entire d/l in secs

			// We do it in K (/1024) because we'd overflow around 4MB
			UI_PrintTime ( dlTimeBuf, sizeof dlTimeBuf, 
				(n - (((downloadCount/1024) * n) / (downloadSize/1024))) * 1000);

			Text_PaintCenter(leftWidth, yStart+216, scale, colorWhite, dlTimeBuf, 0);
			Text_PaintCenter(leftWidth, yStart+160, scale, colorWhite, va("(%s of %s copied)", dlSizeBuf, totalSizeBuf), 0);
		} else {
			Text_PaintCenter(leftWidth, yStart+216, scale, colorWhite, "estimating", 0);
			if (downloadSize) {
				Text_PaintCenter(leftWidth, yStart+160, scale, colorWhite, va("(%s of %s copied)", dlSizeBuf, totalSizeBuf), 0);
			} else {
				Text_PaintCenter(leftWidth, yStart+160, scale, colorWhite, va("(%s copied)", dlSizeBuf), 0);
			}
		}

		if (xferRate) {
			Text_PaintCenter(leftWidth, yStart+272, scale, colorWhite, va("%s/Sec", xferRateBuf), 0);
		}
	}
}

/*
========================
UI_DrawConnectScreen

This will also be overlaid on the cgame info screen during loading
to prevent it from blinking away too rapidly on local or lan games.
========================
*/
void UI_DrawConnectScreen( qboolean overlay ) {
	char			*s;
	uiClientState_t	cstate;
	char			info[MAX_INFO_VALUE];
	char text[256];
	float centerPoint, yStart, scale;
	
	menuDef_t *menu = Menus_FindByName("Connect");


	if ( !overlay && menu ) {
		// VR: cover the physical framebuffer edge-to-edge before the stock
		// paint, so the letterbox bars outside the centered 4:3 box (see
		// UI_VR_AdjustFrom640) don't show stale eye-buffer content. Gated
		// to VR only: on flatscreen the stock Menu_Paint background draw
		// is already letterboxed/pillarboxed to the 4:3 box via the normal
		// scale/bias transform, so an ungated fill would visibly paint the
		// window's black bars with the menu background, a flatscreen
		// visual change, not just harmless overdraw.
		if ( vrActive && menu->window.background ) {
			UI_VR_FillScreen( menu->window.background );
		}
		Menu_Paint(menu, qtrue);
	}

	if (!overlay) {
		centerPoint = 320;
		yStart = 130;
		scale = 0.5f;
	} else {
		centerPoint = 320;
		yStart = 32;
		scale = 0.6f;
		return;
	}

	// see what information we should display
	trap_GetClientState( &cstate );

	info[0] = '\0';
	if( trap_GetConfigString( CS_SERVERINFO, info, sizeof(info) ) ) {
		Text_PaintCenter(centerPoint, yStart, scale, colorWhite, va( "Loading %s", Info_ValueForKey( info, "mapname" )), 0);
	}

	if (!Q_stricmp(cstate.servername,"localhost")) {
		Text_PaintCenter(centerPoint, yStart + 48, scale, colorWhite, va("Starting up..."), ITEM_TEXTSTYLE_SHADOWEDMORE);
	} else {
		strcpy(text, va("Connecting to %s", cstate.servername));
		Text_PaintCenter(centerPoint, yStart + 48, scale, colorWhite,text , ITEM_TEXTSTYLE_SHADOWEDMORE);
	}

	// display global MOTD at bottom
	Text_PaintCenter(centerPoint, 600, scale, colorWhite, Info_ValueForKey( cstate.updateInfoString, "motd" ), 0);
	// print any server info (server full, bad version, etc)
	if ( cstate.connState < CA_CONNECTED ) {
		Text_PaintCenter_AutoWrapped(centerPoint, yStart + 176, 630, 20, scale, colorWhite, cstate.messageString, 0);
	}

	if ( lastConnState > cstate.connState ) {
		lastLoadingText[0] = '\0';
	}
	lastConnState = cstate.connState;

	switch ( cstate.connState ) {
	case CA_CONNECTING:
		s = va("Awaiting connection...%i", cstate.connectPacketCount);
		break;
	case CA_CHALLENGING:
		s = va("Awaiting challenge...%i", cstate.connectPacketCount);
		break;
	case CA_CONNECTED: {
		char downloadName[MAX_INFO_VALUE];

			trap_Cvar_VariableStringBuffer( "cl_downloadName", downloadName, sizeof(downloadName) );
			if (*downloadName) {
				UI_DisplayDownloadInfo( downloadName, centerPoint, yStart, scale );
				return;
			}
		}
		s = "Awaiting gamestate...";
		break;
	case CA_LOADING:
		return;
	case CA_PRIMED:
		return;
	default:
		return;
	}


	if (Q_stricmp(cstate.servername,"localhost")) {
		Text_PaintCenter(centerPoint, yStart + 80, scale, colorWhite, s, 0);
	}

	// password required / connection rejected information goes here
}


/*
================
cvars
================
*/

typedef struct {
	vmCvar_t	*vmCvar;
	char		*cvarName;
	char		*defaultString;
	int			cvarFlags;
} cvarTable_t;

#define DECLARE_UI_CVAR
	#include "ui_cvar.h"
#undef DECLARE_UI_CVAR

// bk001129 - made static to avoid aliasing
static cvarTable_t		cvarTable[] = {

#define UI_CVAR_LIST
	#include "ui_cvar.h"
#undef UI_CVAR_LIST

};

// bk001129 - made static to avoid aliasing
static int		cvarTableSize = sizeof(cvarTable) / sizeof(cvarTable[0]);


/*
=================
UI_RegisterCvars
=================
*/
void UI_RegisterCvars( void ) {
	int			i;
	cvarTable_t	*cv;
	const char	*defaultString;

	for ( i = 0, cv = cvarTable ; i < cvarTableSize ; i++, cv++ ) {
		defaultString = cv->defaultString;
		// crosshair target names are HUD clutter in a headset: default them
		// off under VR (ui registers this cvar before cgame loads), stock on
		// flatscreen
		if ( vrActive && cv->vmCvar == &ui_drawCrosshairNames ) {
			defaultString = "0";
		}
		trap_Cvar_Register( cv->vmCvar, cv->cvarName, defaultString, cv->cvarFlags );
	}

	// favorites address book: server1..server64 must be archived to survive
	// restarts, and 64 slots outgrew the X-macro table
	for ( i = 1 ; i <= MAX_FAVORITESERVERS ; i++ ) {
		trap_Cvar_Register( NULL, va( "server%d", i ), "", CVAR_ARCHIVE );
	}
}

/*
=================
UI_UpdateCvars
=================
*/
void UI_UpdateCvars( void ) {
	int			i;
	cvarTable_t	*cv;

	for ( i = 0, cv = cvarTable ; i < cvarTableSize ; i++, cv++ ) {
		trap_Cvar_Update( cv->vmCvar );
	}
}


/*
=================
UI_VideoCheck
=================
*/
void UI_VideoCheck( int time )
{
	if ( abs( time - uiInfo.uiDC.lastVideoCheck ) > 1000 ) {

		int oldWidth, oldHeight;
		oldWidth = uiInfo.uiDC.glconfig.vidWidth;
		oldHeight = uiInfo.uiDC.glconfig.vidHeight;

		trap_GetGlconfig( &uiInfo.uiDC.glconfig );

		if ( uiInfo.uiDC.glconfig.vidWidth != oldWidth || uiInfo.uiDC.glconfig.vidHeight != oldHeight ) {
			uiInfo.uiDC.biasY = 0.0;
			uiInfo.uiDC.biasX = 0.0;
			// for 640x480 virtualized screen
			uiInfo.uiDC.yscale = uiInfo.uiDC.glconfig.vidHeight * (1.0/480.0);
			uiInfo.uiDC.xscale = uiInfo.uiDC.glconfig.vidWidth * (1.0/640.0);
			if ( uiInfo.uiDC.glconfig.vidWidth * 480 > uiInfo.uiDC.glconfig.vidHeight * 640 ) {
				// wide screen, scale by height
				uiInfo.uiDC.scale = uiInfo.uiDC.glconfig.vidHeight * (1.0/480.0);
				uiInfo.uiDC.biasX = 0.5 * ( uiInfo.uiDC.glconfig.vidWidth - ( uiInfo.uiDC.glconfig.vidHeight * (640.0/480.0) ) );
				uiInfo.uiDC.xscale = uiInfo.uiDC.yscale;
			} else {
				// no wide screen, scale by width
				uiInfo.uiDC.scale = uiInfo.uiDC.glconfig.vidWidth * (1.0/640.0);
				uiInfo.uiDC.biasY = 0.5 * ( uiInfo.uiDC.glconfig.vidHeight - ( uiInfo.uiDC.glconfig.vidWidth * (480.0/640) ) );
			}

			uiInfo.uiDC.screenXmin = 0.0 - (uiInfo.uiDC.biasX / uiInfo.uiDC.scale);
			uiInfo.uiDC.screenXmax = 640.0 + (uiInfo.uiDC.biasX / uiInfo.uiDC.scale);

			uiInfo.uiDC.screenYmin = 0.0 - (uiInfo.uiDC.biasY / uiInfo.uiDC.scale);
			uiInfo.uiDC.screenYmax = 480.0 + (uiInfo.uiDC.biasY / uiInfo.uiDC.scale);

			uiInfo.uiDC.cursorScaleR = 1.0 / uiInfo.uiDC.scale;
			if ( uiInfo.uiDC.cursorScaleR < 0.5 ) {
				uiInfo.uiDC.cursorScaleR = 0.5;
			}
		}

		uiInfo.uiDC.lastVideoCheck = time;
	}
}


/*
=================
ArenaServers_StopRefresh
=================
*/
static void UI_StopServerRefresh( void )
{
	int count;

	if (!uiInfo.serverStatus.refreshActive) {
		// not currently refreshing
		return;
	}
	uiInfo.serverStatus.refreshActive = qfalse;
	Com_Printf("%d servers listed in browser with %d players.\n",
					uiInfo.serverStatus.numDisplayServers,
					uiInfo.serverStatus.numPlayersOnServers);
	count = UI_SourceLANCount();
	if (count - uiInfo.serverStatus.numDisplayServers > 0) {
		Com_Printf("%d servers not listed due to packet loss or pings higher than %d\n",
						count - uiInfo.serverStatus.numDisplayServers,
						(int) trap_Cvar_VariableValue("cl_maxPing"));
	}

}

/*
=================
ArenaServers_MaxPing
=================
*/
#ifndef MISSIONPACK // bk001206
static int ArenaServers_MaxPing( void ) {
	int		maxPing;

	maxPing = (int)trap_Cvar_VariableValue( "cl_maxPing" );
	if( maxPing < 100 ) {
		maxPing = 100;
	}
	return maxPing;
}
#endif

/*
=================
UI_DoServerRefresh
=================
*/
static void UI_DoServerRefresh( void )
{
	qboolean wait = qfalse;
	qboolean pinging;
	int buckets[2];
	int nb, b, type, count;

	if (!uiInfo.serverStatus.refreshActive) {
		return;
	}
	if (uiInfo.serverStatus.refreshSource != UI_ActiveNetSource()) {
		// the tab changed under the refresh; its results no longer belong here
		UI_StopServerRefresh();
		return;
	}

	type = UI_NetSourceType();
	if (type == SRC_TRINITY) {
		// resolve which engine bucket answers the slot-2 query
		UI_ProbeTrinityBucket();
	}

	if (type != SRC_FAVORITES) {
		if (type == SRC_LOCAL) {
			if (!trap_LAN_GetServerCount(AS_LOCAL)) {
				wait = qtrue;
			}
		} else {
			if (UI_SourceLANCount() < 0) {
				wait = qtrue;
			}
		}
	}

	if (uiInfo.uiDC.realTime < uiInfo.serverStatus.refreshtime) {
		if (wait) {
			return;
		}
	}

	// if still trying to retrieve pings; this also flushes each completed
	// ping's annotated info (nettype) into the bucket, which the raw
	// getinfo response the bucket was first written from doesn't carry
	pinging = qfalse;
	nb = UI_SourceLANBuckets(buckets);
	for (b = 0; b < nb; b++) {
		if (trap_LAN_UpdateVisiblePings(buckets[b])) {
			pinging = qtrue;
		}
	}

	// snapshot whatever answered into this tab's cache, after the ping
	// update above so the rows carry the annotated values
	UI_HarvestServers();

	// a changed count means a master's list (or another packet of one)
	// just landed
	count = UI_SourceLANCount();
	if (count != uiInfo.serverStatus.lastCount) {
		uiInfo.serverStatus.lastCount = count;
		uiInfo.serverStatus.lastCountTime = uiInfo.uiDC.realTime;
	}

	if (pinging) {
		uiInfo.serverStatus.refreshtime = uiInfo.uiDC.realTime + 1000;
	} else if (!wait) {
		if (!UI_SourceQueriesMasters() ||
				uiInfo.uiDC.realTime - uiInfo.serverStatus.lastCountTime >= MASTER_QUIET_TIME) {
			// get the last servers in the list
			UI_BuildServerDisplayList(2);
			// stop the refresh
			UI_StopServerRefresh();
		}
		// otherwise the pings drained but a slower master's list can still
		// be in flight: the refresh stays open until the count goes quiet
	}
	//
	UI_BuildServerDisplayList(qfalse);
}

/*
=================
UI_StartServerRefresh
=================
*/
static void UI_StartServerRefresh(qboolean full)
{
	int		buckets[2];
	int		nb, b, type, tab;
	char	*ptr;

	qtime_t q;

	// pick up sv_master edits and keep ui_netSource inside the list
	UI_BuildBrowserSources();
	tab = UI_ActiveNetSource();
	type = UI_NetSourceType();

	trap_RealTime(&q);
 	trap_Cvar_Set( va("ui_lastServerRefresh_%i", tab), va("%s-%i, %i at %i:%i", MonthAbbrev[q.tm_mon],q.tm_mday, 1900+q.tm_year,q.tm_hour,q.tm_min));

	if (type == SRC_FAVORITES) {
		// the archived server1..64 address book is the single source of truth
		UI_SyncFavoritesToEngine();
	}

	// a ping-only refresh needs a result list to re-ping; a source whose
	// query never ran (or never answered) escalates to a full query instead
	// of arming a refresh that can never finish
	if (!full && UI_SourceLANCount() >= 0) {
		UI_UpdatePendingPings();
		return;
	}

	uiInfo.serverStatus.refreshActive = qtrue;
	uiInfo.serverStatus.refreshSource = tab;
	uiInfo.serverStatus.nextDisplayRefresh = uiInfo.uiDC.realTime + 1000;
	uiInfo.serverStatus.lastCount = -1;
	uiInfo.serverStatus.lastCountTime = uiInfo.uiDC.realTime;
	// a refresh rewrites this tab's snapshot
	uiNumSourceServers[tab] = 0;
	// clear number of displayed servers
	uiInfo.serverStatus.numDisplayServers = 0;
	uiInfo.serverStatus.numPlayersOnServers = 0;

	if (type == SRC_TRINITY) {
		// until the probe resolves, either bucket may carry the answer
		nb = 2;
		buckets[0] = AS_GLOBAL;
		buckets[1] = AS_MPLAYER;
	} else {
		nb = UI_SourceLANBuckets(buckets);
	}
	for (b = 0; b < nb; b++) {
		// mark all servers as visible so we store ping updates for them
		trap_LAN_MarkServerVisible(buckets[b], -1, qtrue);
		// reset all the pings
		trap_LAN_ResetPings(buckets[b]);
	}
	//
	if( type == SRC_LOCAL ) {
		trap_Cmd_ExecuteText( EXEC_NOW, "localservers\n" );
		uiInfo.serverStatus.refreshtime = uiInfo.uiDC.realTime + 5000;
		return;
	}

	uiInfo.serverStatus.refreshtime = uiInfo.uiDC.realTime + 5000;
	if( type == SRC_TRINITY || type == SRC_INTERNET || type == SRC_MASTER ) {
		if( type == SRC_TRINITY && uiTrinityBucket != AS_MPLAYER ) {
			// re-probe which bucket this engine fills for slot 2
			uiTrinityBucket = -1;
			uiTrinityProbeGlobal = trap_LAN_GetServerCount( AS_GLOBAL );
		}

		ptr = UI_Cvar_VariableString("debug_protocol");
		if (strlen(ptr)) {
			trap_Cmd_ExecuteText( EXEC_NOW, va( "globalservers %d %s full empty\n", uiNetSources[tab].masterSlot, ptr));
		}
		else {
			trap_Cmd_ExecuteText( EXEC_NOW, va( "globalservers %d %d full empty\n", uiNetSources[tab].masterSlot, (int)trap_Cvar_VariableValue( "protocol" ) ) );
		}
	}
}

