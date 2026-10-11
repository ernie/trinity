// Copyright (C) 1999-2000 Id Software, Inc.
//
#include "ui_local.h"
#include "../game/ui_displaymodes.h"

void GraphicsOptions_MenuInit( void );

/*
=======================================================================

DRIVER INFORMATION MENU

=======================================================================
*/


#define DRIVERINFO_FRAMEL	"menu/art/frame2_l"
#define DRIVERINFO_FRAMER	"menu/art/frame1_r"
#define DRIVERINFO_BACK0	"menu/art/back_0"
#define DRIVERINFO_BACK1	"menu/art/back_1"

static char* driverinfo_artlist[] = 
{
	DRIVERINFO_FRAMEL,
	DRIVERINFO_FRAMER,
	DRIVERINFO_BACK0,
	DRIVERINFO_BACK1,
	NULL,
};

#define ID_DRIVERINFOBACK	100

typedef struct
{
	menuframework_s	menu;
	menutext_s		banner;
	menubitmap_s	back;
	menubitmap_s	framel;
	menubitmap_s	framer;
	char			stringbuff[1024];
	char*			strings[64];
	int				numstrings;
} driverinfo_t;

static driverinfo_t	s_driverinfo;

/*
=================
DriverInfo_Event
=================
*/
static void DriverInfo_Event( void* ptr, int event )
{
	if (event != QM_ACTIVATED)
		return;

	switch (((menucommon_s*)ptr)->id)
	{
		case ID_DRIVERINFOBACK:
			UI_PopMenu();
			break;
	}
}

/*
=================
DriverInfo_MenuDraw
=================
*/
static void DriverInfo_MenuDraw( void )
{
	int	i;
	int	y;

	Menu_Draw( &s_driverinfo.menu );

	UI_DrawString( 320, 80, "VENDOR", UI_CENTER|UI_SMALLFONT, color_red );
	UI_DrawString( 320, 152, "PIXELFORMAT", UI_CENTER|UI_SMALLFONT, color_red );
	UI_DrawString( 320, 192, "EXTENSIONS", UI_CENTER|UI_SMALLFONT, color_red );

	UI_DrawString( 320, 80+16, uis.glconfig.vendor_string, UI_CENTER|UI_SMALLFONT, text_color_normal );
	UI_DrawString( 320, 96+16, uis.glconfig.version_string, UI_CENTER|UI_SMALLFONT, text_color_normal );
	UI_DrawString( 320, 112+16, uis.glconfig.renderer_string, UI_CENTER|UI_SMALLFONT, text_color_normal );
	UI_DrawString( 320, 152+16, va ("color(%d-bits) Z(%d-bits) stencil(%d-bits)", uis.glconfig.colorBits, uis.glconfig.depthBits, uis.glconfig.stencilBits), UI_CENTER|UI_SMALLFONT, text_color_normal );

	// double column
	y = 192+16;
	for (i=0; i<s_driverinfo.numstrings/2; i++) {
		UI_DrawString( 320-4, y, s_driverinfo.strings[i*2], UI_RIGHT|UI_SMALLFONT, text_color_normal );
		UI_DrawString( 320+4, y, s_driverinfo.strings[i*2+1], UI_LEFT|UI_SMALLFONT, text_color_normal );
		y += SMALLCHAR_HEIGHT;
	}

	if (s_driverinfo.numstrings & 1)
		UI_DrawString( 320, y, s_driverinfo.strings[s_driverinfo.numstrings-1], UI_CENTER|UI_SMALLFONT, text_color_normal );
}

/*
=================
DriverInfo_Cache
=================
*/
void DriverInfo_Cache( void )
{
	int	i;

	// touch all our pics
	for (i=0; ;i++)
	{
		if (!driverinfo_artlist[i])
			break;
		trap_R_RegisterShaderNoMip(driverinfo_artlist[i]);
	}
}

/*
=================
UI_DriverInfo_Menu
=================
*/
static void UI_DriverInfo_Menu( void )
{
	char*	eptr;
	int		i;
	int		len;

	// zero set all our globals
	memset( &s_driverinfo, 0 ,sizeof(driverinfo_t) );

	DriverInfo_Cache();

	s_driverinfo.menu.fullscreen = qtrue;
	s_driverinfo.menu.draw       = DriverInfo_MenuDraw;

	s_driverinfo.banner.generic.type  = MTYPE_BTEXT;
	s_driverinfo.banner.generic.x	  = 320;
	s_driverinfo.banner.generic.y	  = 16;
	s_driverinfo.banner.string		  = "DRIVER INFO";
	s_driverinfo.banner.color	      = color_white;
	s_driverinfo.banner.style	      = UI_CENTER;

	s_driverinfo.framel.generic.type  = MTYPE_BITMAP;
	s_driverinfo.framel.generic.name  = DRIVERINFO_FRAMEL;
	s_driverinfo.framel.generic.flags = QMF_INACTIVE;
	s_driverinfo.framel.generic.x	  = 0;
	s_driverinfo.framel.generic.y	  = 78;
	s_driverinfo.framel.width  	      = 256;
	s_driverinfo.framel.height  	  = 329;

	s_driverinfo.framer.generic.type  = MTYPE_BITMAP;
	s_driverinfo.framer.generic.name  = DRIVERINFO_FRAMER;
	s_driverinfo.framer.generic.flags = QMF_INACTIVE;
	s_driverinfo.framer.generic.x	  = 376;
	s_driverinfo.framer.generic.y	  = 76;
	s_driverinfo.framer.width  	      = 256;
	s_driverinfo.framer.height  	  = 334;

	s_driverinfo.back.generic.type	   = MTYPE_BITMAP;
	s_driverinfo.back.generic.name     = DRIVERINFO_BACK0;
	s_driverinfo.back.generic.flags    = QMF_LEFT_JUSTIFY|QMF_PULSEIFFOCUS;
	s_driverinfo.back.generic.callback = DriverInfo_Event;
	s_driverinfo.back.generic.id	   = ID_DRIVERINFOBACK;
	s_driverinfo.back.generic.x		   = 0;
	s_driverinfo.back.generic.y		   = 480-64;
	s_driverinfo.back.width  		   = 128;
	s_driverinfo.back.height  		   = 64;
	s_driverinfo.back.focuspic         = DRIVERINFO_BACK1;

  // TTimo: overflow with particularly long GL extensions (such as the gf3)
  // https://zerowing.idsoftware.com/bugzilla/show_bug.cgi?id=399
  // NOTE: could have pushed the size of stringbuff, but the list is already out of the screen
  // (no matter what your resolution)
  Q_strncpyz(s_driverinfo.stringbuff, uis.glconfig.extensions_string, 1024);

	// build null terminated extension strings
	eptr = s_driverinfo.stringbuff;
	while ( s_driverinfo.numstrings<40 && *eptr )
	{
		while ( *eptr == ' ' )
			*eptr++ = '\0';

		// track start of valid string
		if (*eptr && *eptr != ' ')
			s_driverinfo.strings[s_driverinfo.numstrings++] = eptr;

		while ( *eptr && *eptr != ' ' )
			eptr++;
	}

	// safety length strings for display
	for (i=0; i<s_driverinfo.numstrings; i++) {
		len = strlen(s_driverinfo.strings[i]);
		if (len > 32) {
			s_driverinfo.strings[i][len-1] = '>';
			s_driverinfo.strings[i][len]   = '\0';
		}
	}

	Menu_AddItem( &s_driverinfo.menu, &s_driverinfo.banner );
	Menu_AddItem( &s_driverinfo.menu, &s_driverinfo.framel );
	Menu_AddItem( &s_driverinfo.menu, &s_driverinfo.framer );
	Menu_AddItem( &s_driverinfo.menu, &s_driverinfo.back );

	UI_PushMenu( &s_driverinfo.menu );
}

/*
=======================================================================

GRAPHICS OPTIONS MENU

=======================================================================
*/

#define GRAPHICSOPTIONS_FRAMEL	"menu/art/frame2_l"
#define GRAPHICSOPTIONS_FRAMER	"menu/art/frame1_r"
#define GRAPHICSOPTIONS_BACK0	"menu/art/back_0"
#define GRAPHICSOPTIONS_BACK1	"menu/art/back_1"
#define GRAPHICSOPTIONS_ACCEPT0	"menu/art/accept_0"
#define GRAPHICSOPTIONS_ACCEPT1	"menu/art/accept_1"

#define ID_BACK2		101
#define ID_FULLSCREEN	102
#define ID_LIST			103
#define ID_MODE			104
#define ID_DRIVERINFO	105
#define ID_GRAPHICS		106
#define ID_DISPLAY		107
#define ID_SOUND		108
#define ID_NETWORK		109
#define ID_SHADOWS		112
#define ID_MSAA			115
#define ID_REFRESH		116
#define ID_CUSTOMSIZE	117

#define NUM_SHADOWS 3
#define NUM_MSAA		4

typedef struct {
	menuframework_s	menu;

	menutext_s		banner;
	menubitmap_s	framel;
	menubitmap_s	framer;

	menutext_s		graphics;
	menutext_s		display;
	menutext_s		sound;
	menutext_s		network;

	menulist_s		list;
	menulist_s		mode;
	menuslider_s	tq;
	menulist_s  	fs;
	menulist_s  	lighting;
	menulist_s  	texturebits;
	menulist_s  	colordepth;
	menuradiobutton_s compressed;
	menulist_s  	modeldetail;
	menulist_s  	curvedetail;
	menulist_s  	filter;
	menutext_s		driverinfo;

	menulist_s		shadows;
	menulist_s		msaa;
	menulist_s		refresh;
	menuaction_s	customsize;

	menubitmap_s	apply;
	menubitmap_s	back;
} graphicsoptions_t;

typedef struct
{
	int mode;
	qboolean fullscreen;
	int tq;
	int lighting;
	int colordepth;
	int texturebits;
	int modeldetail;
	int curvedetail;
	int filter;
	// the fields below carry no template values: s_ivo_templates' positional
	// initializers stop at filter, so every preset leaves them zero, and
	// preset selection neither applies nor matches them
	int shadows;
	int msaa;
	int compressed;
	int refresh;
	int customW, customH;
} InitialVideoOptions_s;

static InitialVideoOptions_s	s_ivo;
static graphicsoptions_t		s_graphicsoptions;	

static InitialVideoOptions_s s_ivo_templates[] =
{
	{
		4, qtrue, 2, 0, 2, 2, 1, 1, 1	// JDC: this was tq 3
	},
	{
		3, qtrue, 2, 0, 0, 0, 1, 1, 0
	},
	{
		2, qtrue, 1, 0, 1, 0, 1, 0, 0
	},
	{
		2, qtrue, 1, 1, 1, 0, 1, 0, 0
	},
	{
		3, qtrue, 1, 0, 0, 0, 1, 1, 0
	}
};

#define NUM_IVO_TEMPLATES ( ARRAY_LEN( s_ivo_templates ) )

static qboolean graphicsOptions_vr;

// Resolution rows: Desktop, the display's own modes, Custom
static dmList_t		s_dmList;
static const char	*s_resNames[DM_MAX_MODES + 3];
static char			s_customName[40];
static int			s_customW, s_customH;
static int			s_desktopW, s_desktopH;
static int			s_resBeforeBorderless = -1;

#define RES_DESKTOP		0
#define RES_CUSTOM		( s_dmList.numModes + 1 )

// Refresh Rate rows: Default, then the selected size's rates
static const char	*s_rateNames[DM_MAX_RATES + 2];
static char			s_rateText[DM_MAX_RATES][16];
static int			s_selectedHz;	// the chosen rate, kept across size changes; 0 is Default

// Window Mode rows in order, as r_fullscreen values
static const int	s_fsValue[] = { 0, 2, 1 };
#define FS_WINDOWED		0
#define FS_BORDERLESS	1
#define FS_FULLSCREEN	2

static void GraphicsOptions_BuildModeList( void )
{
	char modes[MAX_STRING_CHARS], desktop[32];
	int i, n = 0;

	trap_Cvar_VariableStringBuffer( "r_availableModes", modes, sizeof( modes ) );
	trap_Cvar_VariableStringBuffer( "r_desktopMode", desktop, sizeof( desktop ) );
	DM_DesktopSize( desktop, modes, &s_desktopW, &s_desktopH );
	DM_BuildList( &s_dmList, modes, s_desktopW, s_desktopH );
	s_resNames[n++] = "Desktop";
	for ( i = 0; i < s_dmList.numModes; i++ )
		s_resNames[n++] = s_dmList.modes[i].label;
	s_resNames[n++] = s_customName;
	s_resNames[n] = NULL;
}

static void GraphicsOptions_SetCustom( int w, int h )
{
	s_customW = w;
	s_customH = h;
	if ( w > 0 && h > 0 )
		Com_sprintf( s_customName, sizeof( s_customName ), "Custom (%dx%d)", w, h );
	else
		Q_strncpyz( s_customName, "Custom", sizeof( s_customName ) );
}

// the saved size: r_modeFullscreen when fullscreen reads it, else r_mode
static int GraphicsOptions_ResolutionIndex( void )
{
	char fs[8];
	int mode, row, w, h;
	qboolean borderless = trap_Cvar_VariableValue( "r_fullscreen" ) == 2;

	trap_Cvar_VariableStringBuffer( "r_modeFullscreen", fs, sizeof( fs ) );
	if ( trap_Cvar_VariableValue( "r_fullscreen" ) && fs[0] )
		mode = atoi( fs );
	else
		mode = trap_Cvar_VariableValue( "r_mode" );
	w = trap_Cvar_VariableValue( "r_customWidth" );
	h = trap_Cvar_VariableValue( "r_customHeight" );
	// Borderless runs at the desktop size, which says nothing about a table index
	row = DM_SavedRow( &s_dmList, mode, &w, &h, borderless ? 0 : uis.glconfig.vidWidth, borderless ? 0 : uis.glconfig.vidHeight );
	GraphicsOptions_SetCustom( w, h );
	return row;
}

// the detected mode a Resolution row stands for; Desktop uses the desktop's own entry
static const dmMode_t *GraphicsOptions_SelectedMode( void )
{
	int res = s_graphicsoptions.mode.curvalue;
	int i;

	if ( res == RES_DESKTOP ) {
		i = DM_FindMode( &s_dmList, s_desktopW, s_desktopH );
		return i >= 0 ? &s_dmList.modes[i] : NULL;
	}
	if ( res < RES_CUSTOM )
		return &s_dmList.modes[res - 1];
	i = DM_FindMode( &s_dmList, s_customW, s_customH );
	return i >= 0 ? &s_dmList.modes[i] : NULL;
}

static void GraphicsOptions_BuildRateList( void )
{
	const dmMode_t *m = GraphicsOptions_SelectedMode();
	int i, n = 0;

	s_rateNames[n++] = "Default";
	for ( i = 0; m && i < m->numRates; i++ ) {
		Com_sprintf( s_rateText[i], sizeof( s_rateText[i] ), "%d Hz", m->rates[i] );
		s_rateNames[n++] = s_rateText[i];
	}
	s_rateNames[n] = NULL;
	s_graphicsoptions.refresh.numitems = n;
	// the same rate if this size offers it, else Default
	s_graphicsoptions.refresh.curvalue = 0;
	for ( i = 0; m && s_selectedHz > 0 && i < m->numRates; i++ ) {
		if ( m->rates[i] == s_selectedHz )
			s_graphicsoptions.refresh.curvalue = i + 1;
	}
}

static void GraphicsOptions_CustomDone( int w, int h )
{
	GraphicsOptions_SetCustom( w, h );
	s_graphicsoptions.mode.curvalue = RES_CUSTOM;
	GraphicsOptions_BuildRateList();
}

/*
=================
GraphicsOptions_GetInitialVideo
=================
*/
static void GraphicsOptions_GetInitialVideo( void )
{
	s_ivo.colordepth  = s_graphicsoptions.colordepth.curvalue;
	s_ivo.mode        = s_graphicsoptions.mode.curvalue;
	s_ivo.fullscreen  = s_graphicsoptions.fs.curvalue;
	s_ivo.tq          = s_graphicsoptions.tq.curvalue;
	s_ivo.lighting    = s_graphicsoptions.lighting.curvalue;
	s_ivo.modeldetail = s_graphicsoptions.modeldetail.curvalue;
	s_ivo.curvedetail = s_graphicsoptions.curvedetail.curvalue;
	s_ivo.filter      = s_graphicsoptions.filter.curvalue;
	s_ivo.texturebits = s_graphicsoptions.texturebits.curvalue;
	s_ivo.compressed  = s_graphicsoptions.compressed.curvalue;
	s_ivo.shadows     = s_graphicsoptions.shadows.curvalue;
	s_ivo.msaa        = s_graphicsoptions.msaa.curvalue;
	s_ivo.refresh     = s_graphicsoptions.refresh.curvalue;
	s_ivo.customW     = s_customW;
	s_ivo.customH     = s_customH;
}

/*
=================
GraphicsOptions_CheckConfig
=================
*/
static void GraphicsOptions_CheckConfig( void )
{
	int i;

	for ( i = 0; i < NUM_IVO_TEMPLATES; i++ )
	{
		// the r_mode-class rows are hidden under VR, so presets neither
		// apply nor match them there
		if ( !graphicsOptions_vr )
		{
			if ( s_ivo_templates[i].colordepth != s_graphicsoptions.colordepth.curvalue )
				continue;
		}
		if ( s_ivo_templates[i].tq != s_graphicsoptions.tq.curvalue )
			continue;
		if ( s_ivo_templates[i].lighting != s_graphicsoptions.lighting.curvalue )
			continue;
		if ( s_ivo_templates[i].modeldetail != s_graphicsoptions.modeldetail.curvalue )
			continue;
		if ( s_ivo_templates[i].curvedetail != s_graphicsoptions.curvedetail.curvalue )
			continue;
		if ( s_ivo_templates[i].filter != s_graphicsoptions.filter.curvalue )
			continue;
//		if ( s_ivo_templates[i].texturebits != s_graphicsoptions.texturebits.curvalue )
//			continue;
		s_graphicsoptions.list.curvalue = i;
		return;
	}
	s_graphicsoptions.list.curvalue = 4;
}

// the Custom Size row: drawn like a spin control, but only Enter or a click opens the builder
static void GraphicsOptions_DrawCustomSize( void *self )
{
	menuaction_s *a = (menuaction_s *)self;
	qboolean focus = ( a->generic.parent->cursor == a->generic.menuPosition );
	int style = UI_SMALLFONT;
	float *color;

	if ( a->generic.flags & QMF_GRAYED )
		color = text_color_disabled;
	else if ( focus ) {
		color = text_color_highlight;
		style |= UI_PULSE;
	} else
		color = text_color_normal;

	if ( focus && !( a->generic.flags & QMF_GRAYED ) ) {
		UI_FillRect( a->generic.left, a->generic.top, a->generic.right - a->generic.left + 1, a->generic.bottom - a->generic.top + 1, listbar_color );
		UI_DrawChar( a->generic.x, a->generic.y, 13, UI_CENTER|UI_BLINK|UI_SMALLFONT, color );
	}
	UI_DrawString( a->generic.x - SMALLCHAR_WIDTH, a->generic.y, a->generic.name, style|UI_RIGHT, color );
	UI_DrawString( a->generic.x + SMALLCHAR_WIDTH, a->generic.y,
		( s_customW > 0 && s_customH > 0 ) ? va( "%dx%d", s_customW, s_customH ) : "Set...", style|UI_LEFT, color );
}

/*
=================
GraphicsOptions_UpdateMenuItems
=================
*/
static void GraphicsOptions_UpdateMenuItems( void )
{
	if ( !graphicsOptions_vr )
	{
		// only exclusive fullscreen sets a display mode; the others take the desktop's
		if ( s_graphicsoptions.fs.curvalue != FS_FULLSCREEN )
		{
			s_graphicsoptions.colordepth.curvalue = 0;
			s_graphicsoptions.colordepth.generic.flags |= QMF_GRAYED;
		}
		else
		{
			s_graphicsoptions.colordepth.generic.flags &= ~QMF_GRAYED;
		}
		if ( s_graphicsoptions.fs.curvalue == FS_FULLSCREEN && s_graphicsoptions.refresh.numitems > 1 )
			s_graphicsoptions.refresh.generic.flags &= ~QMF_GRAYED;
		else
			s_graphicsoptions.refresh.generic.flags |= QMF_GRAYED;
		// borderless always covers the desktop at its own size
		if ( s_graphicsoptions.fs.curvalue == FS_BORDERLESS )
			s_graphicsoptions.mode.generic.flags |= QMF_GRAYED;
		else
			s_graphicsoptions.mode.generic.flags &= ~QMF_GRAYED;
		if ( s_graphicsoptions.fs.curvalue != FS_BORDERLESS && s_graphicsoptions.mode.curvalue == RES_CUSTOM )
			s_graphicsoptions.customsize.generic.flags &= ~QMF_GRAYED;
		else
			s_graphicsoptions.customsize.generic.flags |= QMF_GRAYED;
	}

	// Apply lights on any pending change to a row that needs the restart;
	// the instant row (shadows) writes its live cvar
	// and never lights it, while MSAA writes its latched cvar immediately
	// and still lights it for the restart
	s_graphicsoptions.apply.generic.flags |= QMF_HIDDEN|QMF_INACTIVE;

	if ( !graphicsOptions_vr )
	{
		if ( s_ivo.mode != s_graphicsoptions.mode.curvalue )
		{
			s_graphicsoptions.apply.generic.flags &= ~(QMF_HIDDEN|QMF_INACTIVE);
		}
		if ( s_ivo.fullscreen != s_graphicsoptions.fs.curvalue )
		{
			s_graphicsoptions.apply.generic.flags &= ~(QMF_HIDDEN|QMF_INACTIVE);
		}
		if ( s_ivo.colordepth != s_graphicsoptions.colordepth.curvalue )
		{
			s_graphicsoptions.apply.generic.flags &= ~(QMF_HIDDEN|QMF_INACTIVE);
		}
		if ( s_ivo.refresh != s_graphicsoptions.refresh.curvalue || s_ivo.customW != s_customW || s_ivo.customH != s_customH )
		{
			s_graphicsoptions.apply.generic.flags &= ~(QMF_HIDDEN|QMF_INACTIVE);
		}
	}
	if ( s_ivo.tq != s_graphicsoptions.tq.curvalue )
	{
		s_graphicsoptions.apply.generic.flags &= ~(QMF_HIDDEN|QMF_INACTIVE);
	}
	if ( s_ivo.lighting != s_graphicsoptions.lighting.curvalue )
	{
		s_graphicsoptions.apply.generic.flags &= ~(QMF_HIDDEN|QMF_INACTIVE);
	}
	if ( s_ivo.texturebits != s_graphicsoptions.texturebits.curvalue )
	{
		s_graphicsoptions.apply.generic.flags &= ~(QMF_HIDDEN|QMF_INACTIVE);
	}
	if ( s_ivo.modeldetail != s_graphicsoptions.modeldetail.curvalue )
	{
		s_graphicsoptions.apply.generic.flags &= ~(QMF_HIDDEN|QMF_INACTIVE);
	}
	if ( s_ivo.curvedetail != s_graphicsoptions.curvedetail.curvalue )
	{
		s_graphicsoptions.apply.generic.flags &= ~(QMF_HIDDEN|QMF_INACTIVE);
	}
	if ( s_ivo.filter != s_graphicsoptions.filter.curvalue )
	{
		s_graphicsoptions.apply.generic.flags &= ~(QMF_HIDDEN|QMF_INACTIVE);
	}
	if ( s_ivo.compressed != s_graphicsoptions.compressed.curvalue )
	{
		s_graphicsoptions.apply.generic.flags &= ~(QMF_HIDDEN|QMF_INACTIVE);
	}
	if ( s_ivo.msaa != s_graphicsoptions.msaa.curvalue )
	{
		s_graphicsoptions.apply.generic.flags &= ~(QMF_HIDDEN|QMF_INACTIVE);
	}

	GraphicsOptions_CheckConfig();
}

/*
=================
GraphicsOptions_ApplyChanges
=================
*/
static void GraphicsOptions_ApplyChanges( void *unused, int notification )
{
	static const int subdivisions[] = { 20, 12, 4, 2, 1 };

	if (notification != QM_ACTIVATED)
		return;

	// each detail control writes only its own cvar
	trap_Cvar_SetValue( "r_lodBias", 2 - s_graphicsoptions.modeldetail.curvalue );
	trap_Cvar_SetValue( "r_subdivisions", subdivisions[ s_graphicsoptions.curvedetail.curvalue ] );

	trap_Cvar_SetValue( "r_picmip", 3 - s_graphicsoptions.tq.curvalue );
	trap_Cvar_SetValue( "r_vertexLight", s_graphicsoptions.lighting.curvalue );

	switch ( s_graphicsoptions.texturebits.curvalue  )
	{
	case 0:
		trap_Cvar_Reset( "r_texturebits" );
		break;
	case 1:
		trap_Cvar_SetValue( "r_texturebits", 16 );
		break;
	case 2:
		trap_Cvar_SetValue( "r_texturebits", 32 );
		break;
	}

	if ( s_graphicsoptions.filter.curvalue )
	{
		trap_Cvar_Set( "r_textureMode", "GL_LINEAR_MIPMAP_LINEAR" );
	}
	else
	{
		trap_Cvar_Set( "r_textureMode", "GL_LINEAR_MIPMAP_NEAREST" );
	}

	trap_Cvar_SetValue( "r_ext_compressed_textures", s_graphicsoptions.compressed.curvalue );

	// the r_mode-class rows exist only on flatscreen; the runtime owns the
	// display under VR
	if ( !graphicsOptions_vr )
	{
		int res = s_graphicsoptions.mode.curvalue;
		const dmMode_t *m = GraphicsOptions_SelectedMode();
		const char *mode = "-2";

		// borderless keeps the saved size for a later switch back to Fullscreen
		if ( s_graphicsoptions.fs.curvalue == FS_BORDERLESS && s_resBeforeBorderless >= 0 )
			res = s_resBeforeBorderless;
		if ( res == RES_CUSTOM ) {
			mode = "-1";
			trap_Cvar_SetValue( "r_customWidth", s_customW );
			trap_Cvar_SetValue( "r_customHeight", s_customH );
		} else if ( res > RES_DESKTOP ) {
			mode = "-1";
			trap_Cvar_SetValue( "r_customWidth", s_dmList.modes[res - 1].width );
			trap_Cvar_SetValue( "r_customHeight", s_dmList.modes[res - 1].height );
		}
		// both, so the choice holds whether the window is fullscreen or not
		trap_Cvar_Set( "r_mode", mode );
		trap_Cvar_Set( "r_modeFullscreen", mode );
		trap_Cvar_SetValue( "r_fullscreen", s_fsValue[s_graphicsoptions.fs.curvalue] );
		// only exclusive fullscreen sets a rate; the saved one waits for it
		if ( s_graphicsoptions.fs.curvalue == FS_FULLSCREEN )
			trap_Cvar_SetValue( "r_displayRefresh", ( m && s_graphicsoptions.refresh.curvalue > 0 ) ?
				m->rates[s_graphicsoptions.refresh.curvalue - 1] : 0 );

		switch ( s_graphicsoptions.colordepth.curvalue )
		{
		case 0:
			trap_Cvar_Reset( "r_colorbits" );
			trap_Cvar_Reset( "r_depthbits" );
			trap_Cvar_Reset( "r_stencilbits" );
			break;
		case 1:
			trap_Cvar_SetValue( "r_colorbits", 16 );
			trap_Cvar_SetValue( "r_depthbits", 16 );
			trap_Cvar_SetValue( "r_stencilbits", 0 );
			break;
		case 2:
			trap_Cvar_SetValue( "r_colorbits", 32 );
			trap_Cvar_SetValue( "r_depthbits", 24 );
			break;
		}
	}

	trap_Cmd_ExecuteText( EXEC_APPEND, "vid_restart\n" );
}

/*
=================
GraphicsOptions_Event
=================
*/
static void GraphicsOptions_Event( void* ptr, int event ) {
	InitialVideoOptions_s *ivo;

	if( event != QM_ACTIVATED ) {
	 	return;
	}

	switch( ((menucommon_s*)ptr)->id ) {
	case ID_MODE:
		GraphicsOptions_BuildRateList();
		break;

	case ID_CUSTOMSIZE:
		UI_CustomResolutionMenu( s_customW, s_customH, GraphicsOptions_CustomDone );
		break;

	case ID_REFRESH: {
			const dmMode_t *m = GraphicsOptions_SelectedMode();
			s_selectedHz = ( m && s_graphicsoptions.refresh.curvalue > 0 ) ? m->rates[s_graphicsoptions.refresh.curvalue - 1] : 0;
		}
		break;

	case ID_FULLSCREEN:
		// borderless shows Desktop, keeping the chosen size for a switch back
		if ( s_graphicsoptions.fs.curvalue == FS_BORDERLESS ) {
			s_resBeforeBorderless = s_graphicsoptions.mode.curvalue;
			s_graphicsoptions.mode.curvalue = RES_DESKTOP;
		} else if ( s_resBeforeBorderless >= 0 ) {
			s_graphicsoptions.mode.curvalue = s_resBeforeBorderless;
			s_resBeforeBorderless = -1;
		}
		GraphicsOptions_BuildRateList();
		break;

	case ID_LIST:
		ivo = &s_ivo_templates[s_graphicsoptions.list.curvalue];

		s_graphicsoptions.tq.curvalue          = ivo->tq;
		s_graphicsoptions.lighting.curvalue    = ivo->lighting;
		s_graphicsoptions.texturebits.curvalue = ivo->texturebits;
		s_graphicsoptions.modeldetail.curvalue = ivo->modeldetail;
		s_graphicsoptions.curvedetail.curvalue = ivo->curvedetail;
		s_graphicsoptions.filter.curvalue      = ivo->filter;
		// presets carry no values for the newer rows (compress textures,
		// the instant rows), which stay untouched by preset selection
		// a quality preset leaves how the window is shown alone
		if ( !graphicsOptions_vr ) {
			s_graphicsoptions.colordepth.curvalue  = ivo->colordepth;
		}
		break;

	case ID_SHADOWS: {
			int shadows;
			switch (s_graphicsoptions.shadows.curvalue) {
				case 0:
					shadows = 0;
					break;
				case 1:
					shadows = 1;
					break;
				default:
					shadows = 2;
					break;
			}
			trap_Cvar_SetValue("cg_shadows", shadows);
		}
		break;

	case ID_MSAA: {
			int msaa;
			switch (s_graphicsoptions.msaa.curvalue) {
				case 0:
					msaa = 0;
					break;
				case 1:
					msaa = 2;
					break;
				case 2:
					msaa = 4;
					break;
				case 3:
					msaa = 8;
					break;
				default:
					msaa = 4;
					break;
			}
			trap_Cvar_SetValue("r_ext_multisample", msaa);
		}
		break;

	case ID_DRIVERINFO:
		UI_DriverInfo_Menu();
		break;

	case ID_BACK2:
		UI_PopMenu();
		break;

	case ID_GRAPHICS:
		break;

	case ID_DISPLAY:
		UI_PopMenu();
		UI_DisplayOptionsMenu();
		break;

	case ID_SOUND:
		UI_PopMenu();
		UI_SoundOptionsMenu();
		break;

	case ID_NETWORK:
		UI_PopMenu();
		UI_NetworkOptionsMenu();
		break;
	}
}


/*
================
GraphicsOptions_TQEvent
================
*/
static void GraphicsOptions_TQEvent( void *ptr, int event ) {
	if( event != QM_ACTIVATED ) {
	 	return;
	}
	s_graphicsoptions.tq.curvalue = (int)(s_graphicsoptions.tq.curvalue + 0.5);
}


/*
================
GraphicsOptions_MenuDraw
================
*/
void GraphicsOptions_MenuDraw (void)
{
//APSFIX - rework this
	GraphicsOptions_UpdateMenuItems();

	Menu_Draw( &s_graphicsoptions.menu );
}

/*
=================
GraphicsOptions_SetMenuItems
=================
*/
static void GraphicsOptions_SetMenuItems( void )
{
	int	lodbias;
	int	subdivisions;

	lodbias = trap_Cvar_VariableValue( "r_lodBias" );
	if ( lodbias < -2 )
	{
		lodbias = -2;
	}
	else if ( lodbias > 2 )
	{
		lodbias = 2;
	}
	s_graphicsoptions.modeldetail.curvalue = 2 - lodbias;

	subdivisions = trap_Cvar_VariableValue( "r_subdivisions" );
	if ( subdivisions <= 1 )
	{
		s_graphicsoptions.curvedetail.curvalue = 4;
	}
	else if ( subdivisions <= 2 )
	{
		s_graphicsoptions.curvedetail.curvalue = 3;
	}
	else if ( subdivisions <= 4 )
	{
		s_graphicsoptions.curvedetail.curvalue = 2;
	}
	else if ( subdivisions <= 12 )
	{
		s_graphicsoptions.curvedetail.curvalue = 1;
	}
	else
	{
		s_graphicsoptions.curvedetail.curvalue = 0;
	}

	s_graphicsoptions.tq.curvalue = 3-trap_Cvar_VariableValue( "r_picmip");
	if ( s_graphicsoptions.tq.curvalue < 0 )
	{
		s_graphicsoptions.tq.curvalue = 0;
	}
	else if ( s_graphicsoptions.tq.curvalue > 3 )
	{
		s_graphicsoptions.tq.curvalue = 3;
	}

	s_graphicsoptions.lighting.curvalue = trap_Cvar_VariableValue( "r_vertexLight" ) != 0;
	switch ( ( int ) trap_Cvar_VariableValue( "r_texturebits" ) )
	{
	default:
	case 0:
		s_graphicsoptions.texturebits.curvalue = 0;
		break;
	case 16:
		s_graphicsoptions.texturebits.curvalue = 1;
		break;
	case 32:
		s_graphicsoptions.texturebits.curvalue = 2;
		break;
	}

	if ( !Q_stricmp( UI_Cvar_VariableString( "r_textureMode" ), "GL_LINEAR_MIPMAP_NEAREST" ) )
	{
		s_graphicsoptions.filter.curvalue = 0;
	}
	else
	{
		s_graphicsoptions.filter.curvalue = 1;
	}

	s_graphicsoptions.compressed.curvalue = trap_Cvar_VariableValue( "r_ext_compressed_textures" ) != 0;

	switch ( (int) trap_Cvar_VariableValue( "cg_shadows" ) )
	{
		case 0:
			s_graphicsoptions.shadows.curvalue = 0;
			break;
		case 1:
			s_graphicsoptions.shadows.curvalue = 1;
			break;
		default:
			s_graphicsoptions.shadows.curvalue = 2;
			break;
	}

	switch ( (int) trap_Cvar_VariableValue( "r_ext_multisample" ) )
	{
		case 0:
			s_graphicsoptions.msaa.curvalue = 0;
			break;
		case 2:
			s_graphicsoptions.msaa.curvalue = 1;
			break;
		case 4:
			s_graphicsoptions.msaa.curvalue = 2;
			break;
		case 8:
			s_graphicsoptions.msaa.curvalue = 3;
			break;
		default:
			s_graphicsoptions.msaa.curvalue = 2;
			break;
	}

	// the r_mode-class rows are hidden under VR
	if ( graphicsOptions_vr ) {
		return;
	}

	s_graphicsoptions.mode.curvalue = GraphicsOptions_ResolutionIndex();
	switch ( (int)trap_Cvar_VariableValue( "r_fullscreen" ) ) {
	case 0:
		s_graphicsoptions.fs.curvalue = FS_WINDOWED;
		break;
	case 1:
		s_graphicsoptions.fs.curvalue = FS_FULLSCREEN;
		break;
	default:
		s_graphicsoptions.fs.curvalue = FS_BORDERLESS;
		s_resBeforeBorderless = s_graphicsoptions.mode.curvalue;
		s_graphicsoptions.mode.curvalue = RES_DESKTOP;
		break;
	}
	s_selectedHz = trap_Cvar_VariableValue( "r_displayRefresh" );
	GraphicsOptions_BuildRateList();

	switch ( ( int ) trap_Cvar_VariableValue( "r_colorbits" ) )
	{
	default:
	case 0:
		s_graphicsoptions.colordepth.curvalue = 0;
		break;
	case 16:
		s_graphicsoptions.colordepth.curvalue = 1;
		break;
	case 32:
		s_graphicsoptions.colordepth.curvalue = 2;
		break;
	}

	if ( s_graphicsoptions.fs.curvalue != FS_FULLSCREEN )
	{
		s_graphicsoptions.colordepth.curvalue = 0;
	}
}

/*
================
GraphicsOptions_MenuInit
================
*/
void GraphicsOptions_MenuInit( void )
{

	static const char *tq_names[] =
	{
		"Default",
		"16 bit",
		"32 bit",
		NULL
	};

	static const char *s_graphics_options_names[] =
	{
		"High Quality",
		"Normal",
		"Fast",
		"Fastest",
		"Custom",
		0
	};

	static const char *lighting_names[] =
	{
		"Lightmap",
		"Vertex",
		0
	};

	static const char *colordepth_names[] =
	{
		"Default",
		"16 bit",
		"32 bit",
		0
	};

	static const char *windowmode_names[] =
	{
		"Windowed",
		"Borderless",
		"Fullscreen",
		NULL
	};
	static const char *filter_names[] =
	{
		"Bilinear",
		"Trilinear",
		NULL
	};
	static const char *detail_names[] =
	{
		"Very Low",
		"Low",
		"Medium",
		"High",
		"Very High",
		NULL
	};
	static const char *s_shadows[] =
	{
		"None",
		"Low",
		"High",
		NULL
	};

	static const char *s_msaa[] =
	{
		"Off",
		"2x",
		"4x",
		"8x",
		NULL
	};

	int y;

	// zero set all our globals
	memset( &s_graphicsoptions, 0 ,sizeof(graphicsoptions_t) );
	s_resBeforeBorderless = -1;
	GraphicsOptions_SetCustom( 0, 0 );
	GraphicsOptions_BuildModeList();
	s_rateNames[0] = "Default";
	s_rateNames[1] = NULL;

	GraphicsOptions_Cache();

	graphicsOptions_vr = ( UI_VR_Platform() != VRP_NONE );

	s_graphicsoptions.menu.wrapAround = qtrue;
	s_graphicsoptions.menu.fullscreen = qtrue;
	s_graphicsoptions.menu.draw       = GraphicsOptions_MenuDraw;

	s_graphicsoptions.banner.generic.type  = MTYPE_BTEXT;
	s_graphicsoptions.banner.generic.x	   = 320;
	s_graphicsoptions.banner.generic.y	   = 16;
	s_graphicsoptions.banner.string  	   = "SYSTEM SETUP";
	s_graphicsoptions.banner.color         = color_white;
	s_graphicsoptions.banner.style         = UI_CENTER;

	s_graphicsoptions.framel.generic.type  = MTYPE_BITMAP;
	s_graphicsoptions.framel.generic.name  = GRAPHICSOPTIONS_FRAMEL;
	s_graphicsoptions.framel.generic.flags = QMF_INACTIVE;
	s_graphicsoptions.framel.generic.x	   = 0;
	s_graphicsoptions.framel.generic.y	   = 78;
	s_graphicsoptions.framel.width  	   = 256;
	s_graphicsoptions.framel.height  	   = 329;

	s_graphicsoptions.framer.generic.type  = MTYPE_BITMAP;
	s_graphicsoptions.framer.generic.name  = GRAPHICSOPTIONS_FRAMER;
	s_graphicsoptions.framer.generic.flags = QMF_INACTIVE;
	s_graphicsoptions.framer.generic.x	   = 376;
	s_graphicsoptions.framer.generic.y	   = 76;
	s_graphicsoptions.framer.width  	   = 256;
	s_graphicsoptions.framer.height  	   = 334;

	s_graphicsoptions.graphics.generic.type		= MTYPE_PTEXT;
	s_graphicsoptions.graphics.generic.flags	= QMF_RIGHT_JUSTIFY;
	s_graphicsoptions.graphics.generic.id		= ID_GRAPHICS;
	s_graphicsoptions.graphics.generic.callback	= GraphicsOptions_Event;
	s_graphicsoptions.graphics.generic.x		= 216;
	s_graphicsoptions.graphics.generic.y		= 240 - 2 * PROP_HEIGHT;
	s_graphicsoptions.graphics.string			= "GRAPHICS";
	s_graphicsoptions.graphics.style			= UI_RIGHT;
	s_graphicsoptions.graphics.color			= color_red;

	s_graphicsoptions.display.generic.type		= MTYPE_PTEXT;
	s_graphicsoptions.display.generic.flags		= QMF_RIGHT_JUSTIFY|QMF_PULSEIFFOCUS;
	s_graphicsoptions.display.generic.id		= ID_DISPLAY;
	s_graphicsoptions.display.generic.callback	= GraphicsOptions_Event;
	s_graphicsoptions.display.generic.x			= 216;
	s_graphicsoptions.display.generic.y			= 240 - PROP_HEIGHT;
	s_graphicsoptions.display.string			= "DISPLAY";
	s_graphicsoptions.display.style				= UI_RIGHT;
	s_graphicsoptions.display.color				= color_red;

	s_graphicsoptions.sound.generic.type		= MTYPE_PTEXT;
	s_graphicsoptions.sound.generic.flags		= QMF_RIGHT_JUSTIFY|QMF_PULSEIFFOCUS;
	s_graphicsoptions.sound.generic.id			= ID_SOUND;
	s_graphicsoptions.sound.generic.callback	= GraphicsOptions_Event;
	s_graphicsoptions.sound.generic.x			= 216;
	s_graphicsoptions.sound.generic.y			= 240;
	s_graphicsoptions.sound.string				= "SOUND";
	s_graphicsoptions.sound.style				= UI_RIGHT;
	s_graphicsoptions.sound.color				= color_red;

	s_graphicsoptions.network.generic.type		= MTYPE_PTEXT;
	s_graphicsoptions.network.generic.flags		= QMF_RIGHT_JUSTIFY|QMF_PULSEIFFOCUS;
	s_graphicsoptions.network.generic.id		= ID_NETWORK;
	s_graphicsoptions.network.generic.callback	= GraphicsOptions_Event;
	s_graphicsoptions.network.generic.x			= 216;
	s_graphicsoptions.network.generic.y			= 240 + PROP_HEIGHT;
	s_graphicsoptions.network.string			= "NETWORK";
	s_graphicsoptions.network.style				= UI_RIGHT;
	s_graphicsoptions.network.color				= color_red;

	if( graphicsOptions_vr ) {
		// 10 settings rows + Driver Info link = 11 slots centered on the frame
		y = 242 - ( 11 * (BIGCHAR_HEIGHT + 2) ) / 2;
	}
	else {
		// 15 settings rows + Driver Info link = 16 slots centered on the frame
		y = 242 - ( 16 * (BIGCHAR_HEIGHT + 2) ) / 2;
	}

	s_graphicsoptions.list.generic.type     = MTYPE_SPINCONTROL;
	s_graphicsoptions.list.generic.name     = "Graphics Settings:";
	s_graphicsoptions.list.generic.flags    = QMF_PULSEIFFOCUS|QMF_SMALLFONT;
	s_graphicsoptions.list.generic.x        = 400;
	s_graphicsoptions.list.generic.y        = y;
	s_graphicsoptions.list.generic.callback = GraphicsOptions_Event;
	s_graphicsoptions.list.generic.id       = ID_LIST;
	s_graphicsoptions.list.itemnames        = s_graphics_options_names;
	y += BIGCHAR_HEIGHT+2;

	if( !graphicsOptions_vr ) {
		// references/modifies "r_mode", "r_modeFullscreen", "r_customWidth" and "r_customHeight"
		s_graphicsoptions.mode.generic.type     = MTYPE_SPINCONTROL;
		s_graphicsoptions.mode.generic.name     = "Resolution:";
		s_graphicsoptions.mode.generic.flags    = QMF_PULSEIFFOCUS|QMF_SMALLFONT|QMF_WRAP;
		s_graphicsoptions.mode.generic.x        = 400;
		s_graphicsoptions.mode.generic.y        = y;
		s_graphicsoptions.mode.itemnames        = s_resNames;
		s_graphicsoptions.mode.generic.callback = GraphicsOptions_Event;
		s_graphicsoptions.mode.generic.id       = ID_MODE;
		y += BIGCHAR_HEIGHT+2;

		// opens the builder for "r_customWidth" and "r_customHeight"; bounds set here as a spin row's
		s_graphicsoptions.customsize.generic.type      = MTYPE_ACTION;
		s_graphicsoptions.customsize.generic.name      = "Custom Size:";
		s_graphicsoptions.customsize.generic.flags     = QMF_NODEFAULTINIT|QMF_SMALLFONT;
		s_graphicsoptions.customsize.generic.x         = 400;
		s_graphicsoptions.customsize.generic.y         = y;
		s_graphicsoptions.customsize.generic.left      = 400 - SMALLCHAR_WIDTH * ( 1 + strlen( "Custom Size:" ) );
		s_graphicsoptions.customsize.generic.right     = 400 + SMALLCHAR_WIDTH * ( 1 + strlen( "16384x16384" ) );
		s_graphicsoptions.customsize.generic.top       = y;
		s_graphicsoptions.customsize.generic.bottom    = y + SMALLCHAR_HEIGHT;
		s_graphicsoptions.customsize.generic.callback  = GraphicsOptions_Event;
		s_graphicsoptions.customsize.generic.ownerdraw = GraphicsOptions_DrawCustomSize;
		s_graphicsoptions.customsize.generic.id        = ID_CUSTOMSIZE;
		y += BIGCHAR_HEIGHT+2;

		// references "r_colorbits"
		s_graphicsoptions.colordepth.generic.type     = MTYPE_SPINCONTROL;
		s_graphicsoptions.colordepth.generic.name     = "Color Depth:";
		s_graphicsoptions.colordepth.generic.flags    = QMF_PULSEIFFOCUS|QMF_SMALLFONT;
		s_graphicsoptions.colordepth.generic.x        = 400;
		s_graphicsoptions.colordepth.generic.y        = y;
		s_graphicsoptions.colordepth.itemnames        = colordepth_names;
		y += BIGCHAR_HEIGHT+2;

		// references/modifies "r_fullscreen"
		s_graphicsoptions.fs.generic.type     = MTYPE_SPINCONTROL;
		s_graphicsoptions.fs.generic.name	  = "Window Mode:";
		s_graphicsoptions.fs.generic.flags	  = QMF_PULSEIFFOCUS|QMF_SMALLFONT;
		s_graphicsoptions.fs.generic.x	      = 400;
		s_graphicsoptions.fs.generic.y	      = y;
		s_graphicsoptions.fs.generic.callback = GraphicsOptions_Event;
		s_graphicsoptions.fs.generic.id       = ID_FULLSCREEN;
		s_graphicsoptions.fs.itemnames	      = windowmode_names;
		y += BIGCHAR_HEIGHT+2;

		// references/modifies "r_displayRefresh"
		s_graphicsoptions.refresh.generic.type     = MTYPE_SPINCONTROL;
		s_graphicsoptions.refresh.generic.name     = "Refresh Rate:";
		s_graphicsoptions.refresh.generic.flags    = QMF_PULSEIFFOCUS|QMF_SMALLFONT;
		s_graphicsoptions.refresh.generic.x        = 400;
		s_graphicsoptions.refresh.generic.y        = y;
		s_graphicsoptions.refresh.generic.callback = GraphicsOptions_Event;
		s_graphicsoptions.refresh.generic.id       = ID_REFRESH;
		s_graphicsoptions.refresh.itemnames        = s_rateNames;
		y += BIGCHAR_HEIGHT+2;
	}

	// references "r_ext_multisample"
	s_graphicsoptions.msaa.generic.type			= MTYPE_SPINCONTROL;
	s_graphicsoptions.msaa.generic.name			= "MSAA:";
	s_graphicsoptions.msaa.generic.flags		= QMF_PULSEIFFOCUS|QMF_SMALLFONT;
	s_graphicsoptions.msaa.generic.x			= 400;
	s_graphicsoptions.msaa.generic.y			= y;
	s_graphicsoptions.msaa.itemnames			= s_msaa;
	s_graphicsoptions.msaa.generic.callback		= GraphicsOptions_Event;
	s_graphicsoptions.msaa.generic.id			= ID_MSAA;
	s_graphicsoptions.msaa.numitems				= NUM_MSAA;
	// MSAA renders into the frame buffer, so it does nothing while that is off
	if ( UI_HDR_FBOOff() ) {
		s_graphicsoptions.msaa.generic.flags |= QMF_GRAYED;
	}
	y += BIGCHAR_HEIGHT+2;

	// references/modifies "r_vertexLight"
	s_graphicsoptions.lighting.generic.type  = MTYPE_SPINCONTROL;
	s_graphicsoptions.lighting.generic.name	 = "Lighting:";
	s_graphicsoptions.lighting.generic.flags = QMF_PULSEIFFOCUS|QMF_SMALLFONT;
	s_graphicsoptions.lighting.generic.x	 = 400;
	s_graphicsoptions.lighting.generic.y	 = y;
	s_graphicsoptions.lighting.itemnames     = lighting_names;
	y += BIGCHAR_HEIGHT+2;

	// references "cg_shadows"
	s_graphicsoptions.shadows.generic.type		= MTYPE_SPINCONTROL;
	s_graphicsoptions.shadows.generic.name		= "Shadows:";
	s_graphicsoptions.shadows.generic.flags		= QMF_PULSEIFFOCUS|QMF_SMALLFONT;
	s_graphicsoptions.shadows.generic.x			= 400;
	s_graphicsoptions.shadows.generic.y			= y;
	s_graphicsoptions.shadows.itemnames	        = s_shadows;
	s_graphicsoptions.shadows.generic.callback	= GraphicsOptions_Event;
	s_graphicsoptions.shadows.generic.id		= ID_SHADOWS;
	s_graphicsoptions.shadows.numitems			= NUM_SHADOWS;
	y += BIGCHAR_HEIGHT+2;

	// references/modifies "r_lodBias"
	s_graphicsoptions.modeldetail.generic.type  = MTYPE_SPINCONTROL;
	s_graphicsoptions.modeldetail.generic.name	 = "Model Detail:";
	s_graphicsoptions.modeldetail.generic.flags = QMF_PULSEIFFOCUS|QMF_SMALLFONT;
	s_graphicsoptions.modeldetail.generic.x	 = 400;
	s_graphicsoptions.modeldetail.generic.y	 = y;
	s_graphicsoptions.modeldetail.itemnames     = detail_names;
	y += BIGCHAR_HEIGHT+2;

	// references/modifies "r_subdivisions"
	s_graphicsoptions.curvedetail.generic.type  = MTYPE_SPINCONTROL;
	s_graphicsoptions.curvedetail.generic.name	 = "Curve Detail:";
	s_graphicsoptions.curvedetail.generic.flags = QMF_PULSEIFFOCUS|QMF_SMALLFONT;
	s_graphicsoptions.curvedetail.generic.x	 = 400;
	s_graphicsoptions.curvedetail.generic.y	 = y;
	s_graphicsoptions.curvedetail.itemnames     = detail_names;
	y += BIGCHAR_HEIGHT+2;

	// references/modifies "r_picmip"
	s_graphicsoptions.tq.generic.type	= MTYPE_SLIDER;
	s_graphicsoptions.tq.generic.name	= "Texture Detail:";
	s_graphicsoptions.tq.generic.flags	= QMF_PULSEIFFOCUS|QMF_SMALLFONT;
	s_graphicsoptions.tq.generic.x		= 400;
	s_graphicsoptions.tq.generic.y		= y;
	s_graphicsoptions.tq.minvalue       = 0;
	s_graphicsoptions.tq.maxvalue       = 3;
	s_graphicsoptions.tq.generic.callback = GraphicsOptions_TQEvent;
	y += BIGCHAR_HEIGHT+2;

	// references/modifies "r_textureBits"
	s_graphicsoptions.texturebits.generic.type  = MTYPE_SPINCONTROL;
	s_graphicsoptions.texturebits.generic.name	= "Texture Quality:";
	s_graphicsoptions.texturebits.generic.flags	= QMF_PULSEIFFOCUS|QMF_SMALLFONT;
	s_graphicsoptions.texturebits.generic.x	    = 400;
	s_graphicsoptions.texturebits.generic.y	    = y;
	s_graphicsoptions.texturebits.itemnames     = tq_names;
	y += BIGCHAR_HEIGHT+2;

	// references/modifies "r_textureMode"
	s_graphicsoptions.filter.generic.type   = MTYPE_SPINCONTROL;
	s_graphicsoptions.filter.generic.name	= "Texture Filter:";
	s_graphicsoptions.filter.generic.flags	= QMF_PULSEIFFOCUS|QMF_SMALLFONT;
	s_graphicsoptions.filter.generic.x	    = 400;
	s_graphicsoptions.filter.generic.y	    = y;
	s_graphicsoptions.filter.itemnames      = filter_names;
	y += BIGCHAR_HEIGHT+2;

	// references/modifies "r_ext_compressed_textures"
	s_graphicsoptions.compressed.generic.type	= MTYPE_RADIOBUTTON;
	s_graphicsoptions.compressed.generic.name	= "Compress Textures:";
	s_graphicsoptions.compressed.generic.flags	= QMF_PULSEIFFOCUS|QMF_SMALLFONT;
	s_graphicsoptions.compressed.generic.x		= 400;
	s_graphicsoptions.compressed.generic.y		= y;
	y += BIGCHAR_HEIGHT+2;

	s_graphicsoptions.driverinfo.generic.type     = MTYPE_PTEXT;
	s_graphicsoptions.driverinfo.generic.flags    = QMF_CENTER_JUSTIFY|QMF_PULSEIFFOCUS;
	s_graphicsoptions.driverinfo.generic.callback = GraphicsOptions_Event;
	s_graphicsoptions.driverinfo.generic.id       = ID_DRIVERINFO;
	s_graphicsoptions.driverinfo.generic.x        = 320;
	s_graphicsoptions.driverinfo.generic.y        = y;
	s_graphicsoptions.driverinfo.string           = "Driver Info";
	s_graphicsoptions.driverinfo.style            = UI_CENTER|UI_SMALLFONT;
	s_graphicsoptions.driverinfo.color            = color_red;
	y += BIGCHAR_HEIGHT+2;

	s_graphicsoptions.back.generic.type	    = MTYPE_BITMAP;
	s_graphicsoptions.back.generic.name     = GRAPHICSOPTIONS_BACK0;
	s_graphicsoptions.back.generic.flags    = QMF_LEFT_JUSTIFY|QMF_PULSEIFFOCUS;
	s_graphicsoptions.back.generic.callback = GraphicsOptions_Event;
	s_graphicsoptions.back.generic.id	    = ID_BACK2;
	s_graphicsoptions.back.generic.x		= 0;
	s_graphicsoptions.back.generic.y		= 480-64;
	s_graphicsoptions.back.width  		    = 128;
	s_graphicsoptions.back.height  		    = 64;
	s_graphicsoptions.back.focuspic         = GRAPHICSOPTIONS_BACK1;

	s_graphicsoptions.apply.generic.type     = MTYPE_BITMAP;
	s_graphicsoptions.apply.generic.name     = GRAPHICSOPTIONS_ACCEPT0;
	s_graphicsoptions.apply.generic.flags    = QMF_RIGHT_JUSTIFY|QMF_PULSEIFFOCUS|QMF_HIDDEN|QMF_INACTIVE;
	s_graphicsoptions.apply.generic.callback = GraphicsOptions_ApplyChanges;
	s_graphicsoptions.apply.generic.x        = 640;
	s_graphicsoptions.apply.generic.y        = 480-64;
	s_graphicsoptions.apply.width  		     = 128;
	s_graphicsoptions.apply.height  		 = 64;
	s_graphicsoptions.apply.focuspic         = GRAPHICSOPTIONS_ACCEPT1;

	Menu_AddItem( &s_graphicsoptions.menu, ( void * ) &s_graphicsoptions.banner );
	Menu_AddItem( &s_graphicsoptions.menu, ( void * ) &s_graphicsoptions.framel );
	Menu_AddItem( &s_graphicsoptions.menu, ( void * ) &s_graphicsoptions.framer );

	Menu_AddItem( &s_graphicsoptions.menu, ( void * ) &s_graphicsoptions.graphics );
	Menu_AddItem( &s_graphicsoptions.menu, ( void * ) &s_graphicsoptions.display );
	Menu_AddItem( &s_graphicsoptions.menu, ( void * ) &s_graphicsoptions.sound );
	Menu_AddItem( &s_graphicsoptions.menu, ( void * ) &s_graphicsoptions.network );

	Menu_AddItem( &s_graphicsoptions.menu, ( void * ) &s_graphicsoptions.list );
	if( !graphicsOptions_vr ) {
		Menu_AddItem( &s_graphicsoptions.menu, ( void * ) &s_graphicsoptions.mode );
		Menu_AddItem( &s_graphicsoptions.menu, ( void * ) &s_graphicsoptions.customsize );
		Menu_AddItem( &s_graphicsoptions.menu, ( void * ) &s_graphicsoptions.colordepth );
		Menu_AddItem( &s_graphicsoptions.menu, ( void * ) &s_graphicsoptions.fs );
		Menu_AddItem( &s_graphicsoptions.menu, ( void * ) &s_graphicsoptions.refresh );
	}
	Menu_AddItem( &s_graphicsoptions.menu, ( void * ) &s_graphicsoptions.msaa );
	Menu_AddItem( &s_graphicsoptions.menu, ( void * ) &s_graphicsoptions.lighting );
	Menu_AddItem( &s_graphicsoptions.menu, ( void * ) &s_graphicsoptions.shadows );
	Menu_AddItem( &s_graphicsoptions.menu, ( void * ) &s_graphicsoptions.modeldetail );
	Menu_AddItem( &s_graphicsoptions.menu, ( void * ) &s_graphicsoptions.curvedetail );
	Menu_AddItem( &s_graphicsoptions.menu, ( void * ) &s_graphicsoptions.tq );
	Menu_AddItem( &s_graphicsoptions.menu, ( void * ) &s_graphicsoptions.texturebits );
	Menu_AddItem( &s_graphicsoptions.menu, ( void * ) &s_graphicsoptions.filter );
	Menu_AddItem( &s_graphicsoptions.menu, ( void * ) &s_graphicsoptions.compressed );
	Menu_AddItem( &s_graphicsoptions.menu, ( void * ) &s_graphicsoptions.driverinfo );

	Menu_AddItem( &s_graphicsoptions.menu, ( void * ) &s_graphicsoptions.back );
	Menu_AddItem( &s_graphicsoptions.menu, ( void * ) &s_graphicsoptions.apply );

	GraphicsOptions_SetMenuItems();
	GraphicsOptions_GetInitialVideo();

}


/*
=================
GraphicsOptions_Cache
=================
*/
void GraphicsOptions_Cache( void ) {
	trap_R_RegisterShaderNoMip( GRAPHICSOPTIONS_FRAMEL );
	trap_R_RegisterShaderNoMip( GRAPHICSOPTIONS_FRAMER );
	trap_R_RegisterShaderNoMip( GRAPHICSOPTIONS_BACK0 );
	trap_R_RegisterShaderNoMip( GRAPHICSOPTIONS_BACK1 );
	trap_R_RegisterShaderNoMip( GRAPHICSOPTIONS_ACCEPT0 );
	trap_R_RegisterShaderNoMip( GRAPHICSOPTIONS_ACCEPT1 );
}


/*
=================
UI_GraphicsOptionsMenu
=================
*/
void UI_GraphicsOptionsMenu( void ) {
	GraphicsOptions_MenuInit();
	UI_PushMenu( &s_graphicsoptions.menu );
	Menu_SetCursorToItem( &s_graphicsoptions.menu, &s_graphicsoptions.graphics );
}

