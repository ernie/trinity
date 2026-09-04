// Copyright (C) 1999-2000 Id Software, Inc.
//
/*
=======================================================================

MAIN MENU

=======================================================================
*/


#include "ui_local.h"


#define ID_SINGLEPLAYER			10
#define ID_MULTIPLAYER			11
#define ID_SETUP				12
#define ID_DEMOS				13
#define ID_CINEMATICS			14
#define ID_TEAMARENA		15
#define ID_MODS					16
#define ID_EXIT					17
#define ID_LOGIN				18
#define ID_UPDATE				19

#define MAIN_MENU_VERTICAL_SPACING		34


typedef struct {
	menuframework_s	menu;

	menutext_s		singleplayer;
	menutext_s		multiplayer;
	menutext_s		setup;
	menutext_s		demos;
	menutext_s		cinematics;
	menutext_s		teamArena;
	menutext_s		mods;
	menutext_s		login;
	menutext_s		update;
	menutext_s		exit;
	menubitmap_s	sigil; // For clicks

	qhandle_t		bannerModel;
} mainmenu_t;


static mainmenu_t s_main;

static char      trinityVersion[64];
static char      trinityEngine[64];
static qhandle_t trinityWordmark;
static qhandle_t trinityFlareShader;
static sfxHandle_t trinityFlareSound;
static int       trinityFlareTime;
static qboolean  trinityVersionLoaded = qfalse;
static qhandle_t trinityModel;

#define SIGIL_X		45
#define SIGIL_Y		247
#define SIGIL_SIZE	120

/*
===============
MainMenu_SigilFlare
===============
*/
static float MainMenu_SigilFlare( int now ) {
	int dt = now - trinityFlareTime;

	if ( !trinityFlareTime || dt < 0 || dt >= 1000 ) {
		return 0;
	}
	return dt < 300 ? 1.0f : 1.0f - ( dt - 300 ) / 700.0f;
}

static void MainMenu_SigilEvent( void *ptr, int event ) {
	if ( event == QM_ACTIVATED ) {
		trinityFlareTime = uis.realtime;
		trap_S_StartLocalSound( trinityFlareSound, CHAN_LOCAL_SOUND );
	}
}

typedef struct {
	menuframework_s menu;	
	char errorMessage[4096];
} errorMessage_t;

#define MAIN_BANNER_MODEL				"models/mapobjects/banner/banner5.md3"

static errorMessage_t s_errorMessage;

/*
=================
Main_DrawLoginButton

Draws the Account button using the small bitmap font.
=================
*/
static void Main_DrawLoginButton( void *self ) {
	menutext_s	*t = (menutext_s *)self;
	char		trinityUser[64];
	const char	*label;
	int			style;
	int			w;

	style = UI_RIGHT | UI_SMALLFONT;
	if ( t->generic.flags & QMF_PULSEIFFOCUS && Menu_ItemAtCursor( t->generic.parent ) == t ) {
		style |= UI_PULSE;
	}

	trap_Cvar_VariableStringBuffer( "cl_trinityUser", trinityUser, sizeof( trinityUser ) );
	label = trinityUser[0] ? trinityUser : t->string;

	UI_DrawString( t->generic.x, t->generic.y, label, style,
		( t->generic.flags & QMF_GRAYED ) ? text_color_disabled : t->color );

	// Update hit rect to match current label width
	w = strlen( label ) * SMALLCHAR_WIDTH;
	t->generic.left = t->generic.x - w;
}


/*
=================
Main_DrawUpdateButton

Draws the Update Available button using the small bitmap font.
=================
*/
static void Main_DrawUpdateButton( void *self ) {
	menutext_s	*t = (menutext_s *)self;
	int			style;
	int			w;

	style = UI_RIGHT | UI_SMALLFONT;
	if ( t->generic.flags & QMF_PULSEIFFOCUS && Menu_ItemAtCursor( t->generic.parent ) == t ) {
		style |= UI_PULSE;
	}

	UI_DrawString( t->generic.x, t->generic.y, t->string, style, t->color );

	w = strlen( t->string ) * SMALLCHAR_WIDTH;
	t->generic.left = t->generic.x - w;
}


/*
=================
MainMenu_ExitAction
=================
*/
static void MainMenu_ExitAction( qboolean result ) {
	if( !result ) {
		return;
	}
	UI_PopMenu();
	UI_CreditMenu();
}



/*
=================
Main_MenuEvent
=================
*/
void Main_MenuEvent (void* ptr, int event) {
	if( event != QM_ACTIVATED ) {
		return;
	}

	switch( ((menucommon_s*)ptr)->id ) {
	case ID_SINGLEPLAYER:
		UI_SPLevelMenu();
		break;

	case ID_MULTIPLAYER:
		UI_ArenaServersMenu();
		break;

	case ID_SETUP:
		UI_SetupMenu();
		break;

	case ID_DEMOS:
		UI_DemosMenu();
		break;

	case ID_CINEMATICS:
		UI_CinematicsMenu();
		break;

	case ID_MODS:
		UI_ModsMenu();
		break;

	case ID_TEAMARENA:
		trap_Cvar_Set( "fs_game", "missionpack");
		trap_Cmd_ExecuteText( EXEC_APPEND, "vid_restart;" );
		break;

	case ID_LOGIN:
		UI_LoginMenu();
		break;

	case ID_UPDATE:
		UI_UpdateMenu();
		break;

	case ID_EXIT:
		UI_ConfirmMenu( "EXIT GAME?", NULL, MainMenu_ExitAction );
		break;
	}
}


/*
===============
MainMenu_Cache
===============
*/
void MainMenu_Cache( void ) {
	s_main.bannerModel = trap_R_RegisterModel( MAIN_BANNER_MODEL );
	trinityModel = trap_R_RegisterModel( "models/trinity/trinity.md3" );
	trinityFlareShader = trap_R_RegisterShaderNoMip( "gfx/trinity/flare" );
	trinityFlareSound = trap_S_RegisterSound( "sound/items/poweruprespawn.wav", qfalse );
}

sfxHandle_t ErrorMessage_Key(int key)
{
	trap_Cvar_Set( "com_errorMessage", "" );
	UI_MainMenu();
	return (menu_null_sound);
}

/*
===============
MainMenu_StripVersionPrefix
===============
*/
static void MainMenu_StripVersionPrefix( char *s, int size ) {
	if ( s[0] == 'v' || s[0] == 'V' ) {
		char buf[64];
		Q_strncpyz( buf, s + 1, sizeof( buf ) );
		Q_strncpyz( s, buf, size );
	}
}

static void MainMenu_LoadTrinityVersion( void ) {
	char buf[64];
	const char *p;

	trinityVersionLoaded = qtrue;
	Q_strncpyz( trinityVersion, TRINITY_VERSION, sizeof( trinityVersion ) );
	MainMenu_StripVersionPrefix( trinityVersion, sizeof( trinityVersion ) );
	trinityWordmark = trap_R_RegisterShaderNoMip( "gfx/trinity/wordmark" );
	// "trinity-engine/vX.Y.Z" or similar on Trinity engines
	trap_Cvar_VariableStringBuffer( "com_engine", buf, sizeof( buf ) );
	p = strchr( buf, '/' );
	Q_strncpyz( trinityEngine, p ? p + 1 : buf, sizeof( trinityEngine ) );
	MainMenu_StripVersionPrefix( trinityEngine, sizeof( trinityEngine ) );

	trinityModel = trap_R_RegisterModel( "models/trinity/trinity.md3" );
	trinityFlareShader = trap_R_RegisterShaderNoMip( "gfx/trinity/flare" );
	trinityFlareSound = trap_S_RegisterSound( "sound/items/poweruprespawn.wav", qfalse );
}

/*
===============
MainMenu_DrawTinyString

The menu's own strings start at 8x16, too big for a version line.
===============
*/
static void MainMenu_DrawTinyString( int x, int y, const char *str, vec4_t color ) {
	const int w = 4, h = 7;
	vec4_t shadow = { 0, 0, 0, 0.8f };

	UI_DrawString2( x + 1, y + 1, str, shadow, w, h );
	UI_DrawString2( x, y, str, color, w, h );
}

/*
===============
MainMenu_AddSigil
===============
*/
static void MainMenu_AddSigil( const vec3_t origin ) {
	refEntity_t	ent;
	vec3_t		angles, lightPos;

	lightPos[0] = origin[0] - 140.0f;
	lightPos[1] = origin[1] + 40.0f;
	lightPos[2] = origin[2] + 80.0f;
	trap_R_AddLightToScene( lightPos, 420.0f, 0.70f, 0.80f, 1.0f );

	memset( &ent, 0, sizeof( ent ) );
	{
		float t = uis.realtime * 0.001f;
		float shake = 5.0f * MainMenu_SigilFlare( uis.realtime );
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

	if ( trinityFlareShader && MainMenu_SigilFlare( uis.realtime ) > 0 ) {
		byte level = (byte)( 255 * MainMenu_SigilFlare( uis.realtime ) );
		ent.customShader = trinityFlareShader;
		ent.shaderRGBA.rgba[0] = ent.shaderRGBA.rgba[1] = ent.shaderRGBA.rgba[2] = level;
		ent.shaderRGBA.rgba[3] = 255;
		trap_R_AddRefEntityToScene( &ent );
	}
}

/*
===============
Main_MenuDraw
TTimo: this function is common to the main menu and errorMessage menu
===============
*/

static void Main_MenuDraw( void ) {
	refdef_t		refdef;
	refEntity_t		ent;
	vec3_t			origin;
	vec3_t			angles;
	float			adjust;
	float			x, y, w, h;
	vec4_t			color = {0.5, 0, 0, 1};

	if ( trinityModel ) {
		refdef_t		rd;
		vec3_t			mins, maxs, org;
		float			rx, ry, rw, rh, len;
		float			desFov = 30.0f;

		rx = SIGIL_X;
		ry = SIGIL_Y;
		rw = SIGIL_SIZE;
		rh = SIGIL_SIZE;
		memset( &rd, 0, sizeof( rd ) );
		rd.rdflags = RDF_NOWORLDMODEL;
		AxisClear( rd.viewaxis );
		UI_VR_CompensateModelFov( &rd, desFov, desFov );
		UI_AdjustFrom640( &rx, &ry, &rw, &rh );
		rd.x = rx;
		rd.y = ry;
		rd.width = rw;
		rd.height = rh;
		rd.time = uis.realtime;

		trap_R_ModelBounds( trinityModel, mins, maxs );
		len = 0.5 * ( maxs[2] - mins[2] );
		org[0] = len / tan( DEG2RAD( desFov ) * 0.5 );
		org[1] = 0.5 * ( mins[1] + maxs[1] );
		org[2] = -0.5 * ( mins[2] + maxs[2] );

		trap_R_ClearScene();
		MainMenu_AddSigil( org );
		trap_R_RenderScene( &rd );
	}

	// setup the refdef

	memset( &refdef, 0, sizeof( refdef ) );

	refdef.rdflags = RDF_NOWORLDMODEL;

	AxisClear( refdef.viewaxis );

	x = 0;
	y = 0;
	w = 640;
	h = 120;
	UI_AdjustFrom640( &x, &y, &w, &h );
	refdef.x = x;
	refdef.y = y;
	refdef.width = w;
	refdef.height = h;

	adjust = 0; // JDC: Kenneth asked me to stop this 1.0 * sin( (float)uis.realtime / 1000 );
	UI_VR_CompensateModelFov( &refdef, 60 + adjust, 19.6875 + adjust );

	refdef.time = uis.realtime;

	origin[0] = 300;
	origin[1] = 0;
	origin[2] = -32;

	trap_R_ClearScene();

	// add the model

	memset( &ent, 0, sizeof(ent) );

	adjust = 5.0 * sin( (float)uis.realtime / 5000 );
	VectorSet( angles, 0, 180 + adjust, 0 );
	AnglesToAxis( angles, ent.axis );
	ent.hModel = s_main.bannerModel;
	VectorCopy( origin, ent.origin );
	VectorCopy( origin, ent.lightingOrigin );
	ent.renderfx = RF_LIGHTING_ORIGIN | RF_NOSHADOW;
	VectorCopy( ent.origin, ent.oldorigin );

	trap_R_AddRefEntityToScene( &ent );

	trap_R_RenderScene( &refdef );
	
	if (strlen(s_errorMessage.errorMessage))
	{
		UI_DrawProportionalString_AutoWrapped( 320, 192, 600, 20, s_errorMessage.errorMessage, UI_CENTER|UI_SMALLFONT|UI_DROPSHADOW, menu_text_color );
	}
	else
	{
		// standard menu drawing
		Menu_Draw( &s_main.menu );		
	}

	if (uis.demoversion) {
		UI_DrawProportionalString( 320, 372, "DEMO      FOR MATURE AUDIENCES      DEMO", UI_CENTER|UI_SMALLFONT, color );
		UI_DrawString( 320, 400, "Quake III Arena(c) 1999-2000, Id Software, Inc.  All Rights Reserved", UI_CENTER|UI_SMALLFONT, color );
	} else {
		UI_DrawString( 320, 444, "Quake III Arena(c) 1999-2000, Id Software, Inc.  All Rights Reserved", UI_CENTER|UI_SMALLFONT, color );
	}

	// if update became available after menu was built, rebuild it
	if ( !s_main.update.string && (int)trap_Cvar_VariableValue( "update_available" ) == 1 ) {
		UI_MainMenu();
		return;
	}

	if ( !trinityVersionLoaded ) {
		MainMenu_LoadTrinityVersion();
	}
	if ( trinityVersion[0] ) {
		vec4_t versionColor = { 1.0f, 1.0f, 1.0f, 0.8f };
		const char *line = trinityEngine[0] ? va( "%s / %s", trinityEngine, trinityVersion ) : trinityVersion;
		int cx = 105;

		// the line's bottom is level with the Exit item's
		if ( trinityWordmark ) {
			UI_DrawHandlePic( cx - 45, 371, 90, 20, trinityWordmark );
		}
		MainMenu_DrawTinyString( cx - (int)strlen( line ) * 4 / 2, 392, line, versionColor );
	}
}


/*
===============
UI_TeamArenaExists
===============
*/
static qboolean UI_TeamArenaExists( void ) {
	int		numdirs;
	char	dirlist[2048];
	char	*dirptr;
  char  *descptr;
	int		i;
	int		dirlen;

	numdirs = trap_FS_GetFileList( "$modlist", "", dirlist, sizeof(dirlist) );
	dirptr  = dirlist;
	for( i = 0; i < numdirs; i++ ) {
		dirlen = strlen( dirptr ) + 1;
    descptr = dirptr + dirlen;
		if (Q_stricmp(dirptr, "missionpack") == 0) {
			return qtrue;
		}
    dirptr += dirlen + strlen(descptr) + 1;
	}
	return qfalse;
}


/*
===============
UI_MainMenu

The main menu only comes up when not in a game,
so make sure that the attract loop server is down
and that local cinematics are killed
===============
*/
void UI_MainMenu( void ) {
	int		y;
	qboolean teamArena = qfalse;
	int		style = UI_CENTER | UI_DROPSHADOW;

	trap_Cvar_Set( "sv_killserver", "1" );

	if( !uis.demoversion && !ui_cdkeychecked.integer ) {
		char	key[17];

		trap_GetCDKey( key, sizeof(key) );
		if( trap_VerifyCDKey( key, NULL ) == qfalse ) {
			UI_CDKeyMenu();
			return;
		}
	}
	
	memset( &s_main, 0 ,sizeof(mainmenu_t) );
	memset( &s_errorMessage, 0 ,sizeof(errorMessage_t) );
	trinityVersionLoaded = qfalse;

	// com_errorMessage would need that too
	MainMenu_Cache();
	
	trap_Cvar_VariableStringBuffer( "com_errorMessage", s_errorMessage.errorMessage, sizeof(s_errorMessage.errorMessage) );
	if ( s_errorMessage.errorMessage[0] )
	{	
		s_errorMessage.menu.draw = Main_MenuDraw;
		s_errorMessage.menu.key = ErrorMessage_Key;
		s_errorMessage.menu.fullscreen = qtrue;
		s_errorMessage.menu.wrapAround = qtrue;
		s_errorMessage.menu.showlogo = qtrue;		

		trap_Key_SetCatcher( KEYCATCH_UI );
		uis.menusp = 0;
		UI_PushMenu ( &s_errorMessage.menu );
		
		return;
	}

	s_main.menu.draw = Main_MenuDraw;
	s_main.menu.fullscreen = qtrue;
	s_main.menu.wrapAround = qtrue;
	s_main.menu.showlogo = qtrue;

	y = 134;
	s_main.singleplayer.generic.type		= MTYPE_PTEXT;
	s_main.singleplayer.generic.flags		= QMF_CENTER_JUSTIFY|QMF_PULSEIFFOCUS;
	s_main.singleplayer.generic.x			= 320;
	s_main.singleplayer.generic.y			= y;
	s_main.singleplayer.generic.id			= ID_SINGLEPLAYER;
	s_main.singleplayer.generic.callback	= Main_MenuEvent; 
	s_main.singleplayer.string				= "SINGLE PLAYER";
	s_main.singleplayer.color				= color_red;
	s_main.singleplayer.style				= style;

	y += MAIN_MENU_VERTICAL_SPACING;
	s_main.multiplayer.generic.type			= MTYPE_PTEXT;
	s_main.multiplayer.generic.flags		= QMF_CENTER_JUSTIFY|QMF_PULSEIFFOCUS;
	s_main.multiplayer.generic.x			= 320;
	s_main.multiplayer.generic.y			= y;
	s_main.multiplayer.generic.id			= ID_MULTIPLAYER;
	s_main.multiplayer.generic.callback		= Main_MenuEvent; 
	s_main.multiplayer.string				= "MULTIPLAYER";
	s_main.multiplayer.color				= color_red;
	s_main.multiplayer.style				= style;

	y += MAIN_MENU_VERTICAL_SPACING;
	s_main.setup.generic.type				= MTYPE_PTEXT;
	s_main.setup.generic.flags				= QMF_CENTER_JUSTIFY|QMF_PULSEIFFOCUS;
	s_main.setup.generic.x					= 320;
	s_main.setup.generic.y					= y;
	s_main.setup.generic.id					= ID_SETUP;
	s_main.setup.generic.callback			= Main_MenuEvent; 
	s_main.setup.string						= "SETUP";
	s_main.setup.color						= color_red;
	s_main.setup.style						= style;

	y += MAIN_MENU_VERTICAL_SPACING;
	s_main.demos.generic.type				= MTYPE_PTEXT;
	s_main.demos.generic.flags				= QMF_CENTER_JUSTIFY|QMF_PULSEIFFOCUS;
	s_main.demos.generic.x					= 320;
	s_main.demos.generic.y					= y;
	s_main.demos.generic.id					= ID_DEMOS;
	s_main.demos.generic.callback			= Main_MenuEvent; 
	s_main.demos.string						= "DEMOS";
	s_main.demos.color						= color_red;
	s_main.demos.style						= style;

	y += MAIN_MENU_VERTICAL_SPACING;
	s_main.cinematics.generic.type			= MTYPE_PTEXT;
	s_main.cinematics.generic.flags			= QMF_CENTER_JUSTIFY|QMF_PULSEIFFOCUS;
	s_main.cinematics.generic.x				= 320;
	s_main.cinematics.generic.y				= y;
	s_main.cinematics.generic.id			= ID_CINEMATICS;
	s_main.cinematics.generic.callback		= Main_MenuEvent; 
	s_main.cinematics.string				= "CINEMATICS";
	s_main.cinematics.color					= color_red;
	s_main.cinematics.style					= style;

	if (UI_TeamArenaExists()) {
		teamArena = qtrue;
		y += MAIN_MENU_VERTICAL_SPACING;
		s_main.teamArena.generic.type			= MTYPE_PTEXT;
		s_main.teamArena.generic.flags			= QMF_CENTER_JUSTIFY|QMF_PULSEIFFOCUS;
		s_main.teamArena.generic.x				= 320;
		s_main.teamArena.generic.y				= y;
		s_main.teamArena.generic.id				= ID_TEAMARENA;
		s_main.teamArena.generic.callback		= Main_MenuEvent; 
		s_main.teamArena.string					= "TEAM ARENA";
		s_main.teamArena.color					= color_red;
		s_main.teamArena.style					= style;
	}

	y += MAIN_MENU_VERTICAL_SPACING;
	s_main.mods.generic.type			= MTYPE_PTEXT;
	s_main.mods.generic.flags			= QMF_CENTER_JUSTIFY|QMF_PULSEIFFOCUS;
	s_main.mods.generic.x				= 320;
	s_main.mods.generic.y				= y;
	s_main.mods.generic.id				= ID_MODS;
	s_main.mods.generic.callback		= Main_MenuEvent; 
	s_main.mods.string					= "MODS";
	s_main.mods.color					= color_red;
	s_main.mods.style					= style;

	// Account button: right-aligned, small bitmap font
	s_main.login.generic.type			= MTYPE_PTEXT;
	s_main.login.generic.flags			= QMF_RIGHT_JUSTIFY|QMF_PULSEIFFOCUS;
	s_main.login.generic.x				= 610;
	s_main.login.generic.y				= 134;
	s_main.login.generic.id				= ID_LOGIN;
	s_main.login.generic.callback		= Main_MenuEvent;
	s_main.login.generic.ownerdraw		= Main_DrawLoginButton;
	s_main.login.string					= "Account";
	s_main.login.color					= color_white;
	s_main.login.style					= UI_RIGHT|UI_SMALLFONT;

	if ( (int)trap_Cvar_VariableValue( "update_available" ) == 1 ) {
		s_main.update.generic.type			= MTYPE_PTEXT;
		s_main.update.generic.flags			= QMF_RIGHT_JUSTIFY|QMF_PULSEIFFOCUS;
		s_main.update.generic.x				= 610;
		s_main.update.generic.y				= 134 + SMALLCHAR_HEIGHT + 2;
		s_main.update.generic.id			= ID_UPDATE;
		s_main.update.generic.callback		= Main_MenuEvent;
		s_main.update.generic.ownerdraw		= Main_DrawUpdateButton;
		s_main.update.string				= "Update Available";
		s_main.update.color					= color_yellow;
		s_main.update.style					= UI_RIGHT|UI_SMALLFONT;
	}

	y += MAIN_MENU_VERTICAL_SPACING;
	s_main.exit.generic.type				= MTYPE_PTEXT;
	s_main.exit.generic.flags				= QMF_CENTER_JUSTIFY|QMF_PULSEIFFOCUS;
	s_main.exit.generic.x					= 320;
	s_main.exit.generic.y					= y;
	s_main.exit.generic.id					= ID_EXIT;
	s_main.exit.generic.callback			= Main_MenuEvent;
	s_main.exit.string						= "EXIT";
	s_main.exit.color						= color_red;
	s_main.exit.style						= style;

	Menu_AddItem( &s_main.menu,	&s_main.singleplayer );
	Menu_AddItem( &s_main.menu,	&s_main.multiplayer );
	Menu_AddItem( &s_main.menu,	&s_main.setup );
	Menu_AddItem( &s_main.menu,	&s_main.demos );
	Menu_AddItem( &s_main.menu,	&s_main.cinematics );
	if (teamArena) {
		Menu_AddItem( &s_main.menu,	&s_main.teamArena );
	}
	Menu_AddItem( &s_main.menu,	&s_main.mods );
	if ( s_main.update.string ) {
		Menu_AddItem( &s_main.menu,	&s_main.update );
	}
	Menu_AddItem( &s_main.menu,	&s_main.exit );
	Menu_AddItem( &s_main.menu,	&s_main.login );

	// just for clicks
	s_main.sigil.generic.type		= MTYPE_BITMAP;
	s_main.sigil.generic.flags		= QMF_LEFT_JUSTIFY|QMF_SILENT|QMF_MOUSEONLY;
	s_main.sigil.generic.x			= SIGIL_X;
	s_main.sigil.generic.y			= SIGIL_Y;
	s_main.sigil.width				= SIGIL_SIZE;
	s_main.sigil.height				= SIGIL_SIZE;
	s_main.sigil.generic.callback	= MainMenu_SigilEvent;
	Menu_AddItem( &s_main.menu,	&s_main.sigil );
	// Override hit rect to match bitmap font size (PText_Init sized it for proportional font)
	{
		int w = strlen( s_main.login.string ) * SMALLCHAR_WIDTH;
		s_main.login.generic.left   = s_main.login.generic.x - w;
		s_main.login.generic.right  = s_main.login.generic.x;
		s_main.login.generic.top    = s_main.login.generic.y;
		s_main.login.generic.bottom = s_main.login.generic.y + SMALLCHAR_HEIGHT;
	}
	if ( s_main.update.string ) {
		int w = strlen( s_main.update.string ) * SMALLCHAR_WIDTH;
		s_main.update.generic.left   = s_main.update.generic.x - w;
		s_main.update.generic.right  = s_main.update.generic.x;
		s_main.update.generic.top    = s_main.update.generic.y;
		s_main.update.generic.bottom = s_main.update.generic.y + SMALLCHAR_HEIGHT;
	}

	trap_Key_SetCatcher( KEYCATCH_UI );
	uis.menusp = 0;
	UI_PushMenu ( &s_main.menu );
		
}
