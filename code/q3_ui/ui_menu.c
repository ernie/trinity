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

#define BLOCK_CX			105
#define WORDMARK_W			90
#define VERSION_CHARW		4
#define UPDATE_W			64
#define UPDATE_H			13
#define UPDATE_Y			389
#define UPDATE_GAP			6
#define UPDATE_CHARW		5
#define UPDATE_CHARH		9
#define ACCOUNT_Y			404
#define ACCOUNT_MAXCHARS	16
// the charset's glyphs sit high in their cells
#define ACCOUNT_TEXT_DROP	2
#define PORTRAIT_SIZE		16
#define PORTRAIT_GAP		4

static char      accountLabel[ACCOUNT_MAXCHARS + 1];
static qhandle_t accountPortrait;
static char      portraitModel[MAX_QPATH];
static qhandle_t portraitIcon;

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
MainMenu_UpdateLabel
=================
*/
static const char *MainMenu_UpdateLabel( float **color, qboolean *clickable ) {
	*clickable = qfalse;

	switch ( (int)trap_Cvar_VariableValue( "update_state" ) ) {
	case 0:
		if ( !trap_Cvar_VariableValue( "update_check" ) ) {
			return NULL;
		}
		*color = colorGreen;
		return "Latest";
	case 1:
		*color = colorMdGrey;
		return "Checking...";
	case 6:
		*color = color_red;
		*clickable = qtrue;
		return "Error";
	default:
		*color = color_yellow;
		*clickable = qtrue;
		return "Update";
	}
}


/*
=================
MainMenu_Portrait
=================
*/
static qhandle_t MainMenu_Portrait( void ) {
	char	model[MAX_QPATH];
	char	*skin;

	trap_Cvar_VariableStringBuffer( "model", model, sizeof( model ) );
	if ( !Q_stricmp( model, portraitModel ) ) {
		return portraitIcon;
	}
	Q_strncpyz( portraitModel, model, sizeof( portraitModel ) );

	skin = strchr( model, '/' );
	if ( skin ) {
		*skin++ = '\0';
	} else {
		skin = "default";
	}

	portraitIcon = trap_R_RegisterShaderNoMip( va( "models/players/%s/icon_%s.tga", model, skin ) );
	if ( !portraitIcon && Q_stricmp( skin, "default" ) ) {
		portraitIcon = trap_R_RegisterShaderNoMip( va( "models/players/%s/icon_default.tga", model ) );
	}
	return portraitIcon;
}


/*
=================
Main_DrawLoginButton
=================
*/
static void Main_DrawLoginButton( void *self ) {
	menutext_s	*t = (menutext_s *)self;
	int			x = t->generic.x;
	int			style = UI_LEFT | UI_SMALLFONT;

	if ( Menu_ItemAtCursor( t->generic.parent ) == t ) {
		style |= UI_PULSE;
	}

	if ( accountPortrait ) {
		UI_DrawHandlePic( x, t->generic.y, PORTRAIT_SIZE, PORTRAIT_SIZE, accountPortrait );
		x += PORTRAIT_SIZE + PORTRAIT_GAP;
	}
	UI_DrawString( x, t->generic.y + ACCOUNT_TEXT_DROP, accountLabel, style, t->color );
}


/*
=================
Main_DrawUpdateButton
=================
*/
static void Main_DrawUpdateButton( void *self ) {
	menutext_s	*t = (menutext_s *)self;
	float		*color;
	qboolean	clickable;
	const char	*label;
	int			x = t->generic.x;
	int			y = t->generic.y;

	label = MainMenu_UpdateLabel( &color, &clickable );
	if ( !label ) {
		return;
	}

	if ( Menu_ItemAtCursor( t->generic.parent ) == t ) {
		vec4_t fill;

		Vector4Copy( color, fill );
		fill[3] = 0.25f;
		UI_FillRect( x, y, UPDATE_W, UPDATE_H, fill );
	}
	UI_FillRect( x, y, UPDATE_W, 1, color );
	UI_FillRect( x, y + UPDATE_H - 1, UPDATE_W, 1, color );
	UI_FillRect( x, y + 1, 1, UPDATE_H - 2, color );
	UI_FillRect( x + UPDATE_W - 1, y + 1, 1, UPDATE_H - 2, color );
	UI_DrawString2( x + ( UPDATE_W - (int)strlen( label ) * UPDATE_CHARW ) / 2,
		y + ( UPDATE_H - UPDATE_CHARH ) / 2, label, color, UPDATE_CHARW, UPDATE_CHARH );
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
	const int w = VERSION_CHARW, h = 7;
	vec4_t shadow = { 0, 0, 0, 0.8f };

	UI_DrawString2( x + 1, y + 1, str, shadow, w, h );
	UI_DrawString2( x, y, str, color, w, h );
}

static const char *MainMenu_VersionLine( void ) {
	return trinityEngine[0] ? va( "%s / %s", trinityEngine, trinityVersion ) : trinityVersion;
}

/*
===============
MainMenu_LayoutTrinityBlock

The button and the account line follow the widths of the version line and
the name, so their positions and hit rects are set every frame.
===============
*/
static void MainMenu_LayoutTrinityBlock( void ) {
	char			user[64];
	float			*color;
	qboolean		clickable;
	menucommon_s	*item;
	int				x, w;

	trap_Cvar_VariableStringBuffer( "cl_trinityUser", user, sizeof( user ) );
	Q_strncpyz( accountLabel, user[0] ? user : s_main.login.string, sizeof( accountLabel ) );
	accountPortrait = MainMenu_Portrait();

	w = strlen( accountLabel ) * SMALLCHAR_WIDTH;
	if ( accountPortrait ) {
		w += PORTRAIT_SIZE + PORTRAIT_GAP;
	}
	x = BLOCK_CX - w / 2;
	item = &s_main.login.generic;
	item->x = x;
	item->y = ACCOUNT_Y;
	item->left = x;
	item->right = x + w;
	item->top = ACCOUNT_Y;
	item->bottom = ACCOUNT_Y + SMALLCHAR_HEIGHT;

	w = (int)strlen( MainMenu_VersionLine() ) * VERSION_CHARW;
	if ( w < WORDMARK_W ) {
		w = WORDMARK_W;
	}
	x = BLOCK_CX + w / 2 + UPDATE_GAP;
	item = &s_main.update.generic;
	item->x = x;
	item->y = UPDATE_Y;
	item->left = x;
	item->right = x + UPDATE_W;
	item->top = UPDATE_Y;
	item->bottom = UPDATE_Y + UPDATE_H;

	if ( !MainMenu_UpdateLabel( &color, &clickable ) ) {
		item->flags |= QMF_HIDDEN | QMF_INACTIVE;
	} else if ( !clickable ) {
		item->flags = ( item->flags & ~QMF_HIDDEN ) | QMF_INACTIVE;
	} else {
		item->flags &= ~( QMF_HIDDEN | QMF_INACTIVE );
	}
	if ( item->flags & QMF_INACTIVE ) {
		item->flags &= ~QMF_HASMOUSEFOCUS;
		if ( Menu_ItemAtCursor( &s_main.menu ) == &s_main.update ) {
			Menu_AdjustCursor( &s_main.menu, 1 );
		}
	}
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

	if ( !trinityVersionLoaded ) {
		MainMenu_LoadTrinityVersion();
	}

	if (strlen(s_errorMessage.errorMessage))
	{
		UI_DrawProportionalString_AutoWrapped( 320, 192, 600, 20, s_errorMessage.errorMessage, UI_CENTER|UI_SMALLFONT|UI_DROPSHADOW, menu_text_color );
	}
	else
	{
		MainMenu_LayoutTrinityBlock();
		// standard menu drawing
		Menu_Draw( &s_main.menu );
	}

	if (uis.demoversion) {
		UI_DrawProportionalString( 320, 372, "DEMO      FOR MATURE AUDIENCES      DEMO", UI_CENTER|UI_SMALLFONT, color );
		UI_DrawString( 320, 400, "Quake III Arena(c) 1999-2000, Id Software, Inc.  All Rights Reserved", UI_CENTER|UI_SMALLFONT, color );
	} else {
		UI_DrawString( 320, 444, "Quake III Arena(c) 1999-2000, Id Software, Inc.  All Rights Reserved", UI_CENTER|UI_SMALLFONT, color );
	}

	if ( trinityVersion[0] ) {
		vec4_t versionColor = { 1.0f, 1.0f, 1.0f, 0.8f };
		const char *line = MainMenu_VersionLine();

		// the line's bottom is level with the Exit item's
		if ( trinityWordmark ) {
			UI_DrawHandlePic( BLOCK_CX - WORDMARK_W / 2, 371, WORDMARK_W, 20, trinityWordmark );
		}
		MainMenu_DrawTinyString( BLOCK_CX - (int)strlen( line ) * VERSION_CHARW / 2, 392, line, versionColor );
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

	s_main.login.generic.type			= MTYPE_PTEXT;
	s_main.login.generic.flags			= QMF_LEFT_JUSTIFY;
	s_main.login.generic.id				= ID_LOGIN;
	s_main.login.generic.callback		= Main_MenuEvent;
	s_main.login.generic.ownerdraw		= Main_DrawLoginButton;
	s_main.login.string					= "Sign In";
	s_main.login.color					= color_white;

	s_main.update.generic.type			= MTYPE_PTEXT;
	s_main.update.generic.flags			= QMF_LEFT_JUSTIFY|QMF_INACTIVE;
	s_main.update.generic.id			= ID_UPDATE;
	s_main.update.generic.callback		= Main_MenuEvent;
	s_main.update.generic.ownerdraw		= Main_DrawUpdateButton;
	s_main.update.string				= "Update";

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
	Menu_AddItem( &s_main.menu,	&s_main.update );
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

	trap_Key_SetCatcher( KEYCATCH_UI );
	uis.menusp = 0;
	UI_PushMenu ( &s_main.menu );
		
}
