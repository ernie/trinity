// Copyright (C) 1999-2000 Id Software, Inc.
//
#include "ui_local.h"

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
#define ID_HDR			110
#define ID_SHADOWS		112
#define ID_MSAA			115

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
	menuradiobutton_s hdr;
	menulist_s  	modeldetail;
	menulist_s  	curvedetail;
	menulist_s  	filter;
	menutext_s		driverinfo;

	menulist_s		shadows;
	menulist_s		msaa;

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
	int hdr;
	int shadows;
	int msaa;
	int compressed;
} InitialVideoOptions_s;

static InitialVideoOptions_s	s_ivo;
static graphicsoptions_t		s_graphicsoptions;	

static InitialVideoOptions_s s_ivo_templates[] =
{
	{
		4, qtrue, 2, 0, 2, 2, 3, 3, 1	// JDC: this was tq 3
	},
	{
		3, qtrue, 2, 0, 0, 0, 3, 3, 0
	},
	{
		2, qtrue, 1, 0, 1, 0, 3, 4, 0
	},
	{
		2, qtrue, 1, 1, 1, 0, 3, 4, 0
	},
	{
		3, qtrue, 1, 0, 0, 0, 3, 3, 0
	}
};

#define NUM_IVO_TEMPLATES ( ARRAY_LEN( s_ivo_templates ) )

static qboolean graphicsOptions_vr;

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
	s_ivo.hdr         = s_graphicsoptions.hdr.curvalue;
	s_ivo.shadows     = s_graphicsoptions.shadows.curvalue;
	s_ivo.msaa        = s_graphicsoptions.msaa.curvalue;
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
			if ( s_ivo_templates[i].mode != s_graphicsoptions.mode.curvalue )
				continue;
			if ( s_ivo_templates[i].fullscreen != s_graphicsoptions.fs.curvalue )
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

/*
=================
GraphicsOptions_UpdateMenuItems
=================
*/
static void GraphicsOptions_UpdateMenuItems( void )
{
	if ( !graphicsOptions_vr )
	{
		if ( s_graphicsoptions.fs.curvalue == 0 )
		{
			s_graphicsoptions.colordepth.curvalue = 0;
			s_graphicsoptions.colordepth.generic.flags |= QMF_GRAYED;
		}
		else
		{
			s_graphicsoptions.colordepth.generic.flags &= ~QMF_GRAYED;
		}
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
	if ( s_ivo.hdr != s_graphicsoptions.hdr.curvalue )
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
	static const int subdivisions[] = { 1, 2, 4, 12, 20 };

	if (notification != QM_ACTIVATED)
		return;

	// each detail control writes only its own cvar
	trap_Cvar_SetValue( "r_lodBias", s_graphicsoptions.modeldetail.curvalue - 2 );
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
	trap_Cvar_SetValue( "r_hdrDisplay", s_graphicsoptions.hdr.curvalue );

	// the r_mode-class rows exist only on flatscreen; the runtime owns the
	// display under VR
	if ( !graphicsOptions_vr )
	{
		trap_Cvar_SetValue( "r_mode", s_graphicsoptions.mode.curvalue );
		trap_Cvar_SetValue( "r_fullscreen", s_graphicsoptions.fs.curvalue );

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
		// HDR, the instant rows), which stay untouched by preset selection
		if ( !graphicsOptions_vr ) {
			s_graphicsoptions.mode.curvalue        = ivo->mode;
			s_graphicsoptions.colordepth.curvalue  = ivo->colordepth;
			s_graphicsoptions.fs.curvalue          = ivo->fullscreen;
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
	s_graphicsoptions.modeldetail.curvalue = lodbias + 2;

	subdivisions = trap_Cvar_VariableValue( "r_subdivisions" );
	if ( subdivisions <= 1 )
	{
		s_graphicsoptions.curvedetail.curvalue = 0;
	}
	else if ( subdivisions <= 2 )
	{
		s_graphicsoptions.curvedetail.curvalue = 1;
	}
	else if ( subdivisions <= 4 )
	{
		s_graphicsoptions.curvedetail.curvalue = 2;
	}
	else if ( subdivisions <= 12 )
	{
		s_graphicsoptions.curvedetail.curvalue = 3;
	}
	else
	{
		s_graphicsoptions.curvedetail.curvalue = 4;
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
	s_graphicsoptions.hdr.curvalue = trap_Cvar_VariableValue( "r_hdrDisplay" ) != 0;

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

	s_graphicsoptions.mode.curvalue = trap_Cvar_VariableValue( "r_mode" );
	if ( s_graphicsoptions.mode.curvalue < 0 )
	{
		s_graphicsoptions.mode.curvalue = 3;
	}
	s_graphicsoptions.fs.curvalue = trap_Cvar_VariableValue("r_fullscreen");

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

	if ( s_graphicsoptions.fs.curvalue == 0 )
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

	static const char *resolutions[] = 
	{
		"320x240",
		"400x300",
		"512x384",
		"640x480",
		"800x600",
		"960x720",
		"1024x768",
		"1152x864",
		"1280x1024",
		"1600x1200",
		"2048x1536",
		"856x480 wide screen",
		0
	};
	static const char *filter_names[] =
	{
		"Bilinear",
		"Trilinear",
		NULL
	};
	static const char *detail_names[] =
	{
		"Very High",
		"High",
		"Medium",
		"Low",
		"Very Low",
		NULL
	};
	static const char *enabled_names[] =
	{
		"Off",
		"On",
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
		// 11 settings rows + Driver Info link = 12 slots centered on the frame
		y = 242 - ( 12 * (BIGCHAR_HEIGHT + 2) ) / 2;
	}
	else {
		// 14 settings rows + Driver Info link = 15 slots centered on the frame
		y = 242 - ( 15 * (BIGCHAR_HEIGHT + 2) ) / 2;
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
		// references/modifies "r_mode"
		s_graphicsoptions.mode.generic.type     = MTYPE_SPINCONTROL;
		s_graphicsoptions.mode.generic.name     = "Video Mode:";
		s_graphicsoptions.mode.generic.flags    = QMF_PULSEIFFOCUS|QMF_SMALLFONT;
		s_graphicsoptions.mode.generic.x        = 400;
		s_graphicsoptions.mode.generic.y        = y;
		s_graphicsoptions.mode.itemnames        = resolutions;
		s_graphicsoptions.mode.generic.callback = GraphicsOptions_Event;
		s_graphicsoptions.mode.generic.id       = ID_MODE;
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
		s_graphicsoptions.fs.generic.name	  = "Fullscreen:";
		s_graphicsoptions.fs.generic.flags	  = QMF_PULSEIFFOCUS|QMF_SMALLFONT;
		s_graphicsoptions.fs.generic.x	      = 400;
		s_graphicsoptions.fs.generic.y	      = y;
		s_graphicsoptions.fs.itemnames	      = enabled_names;
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

	// references/modifies "r_hdrDisplay"
	s_graphicsoptions.hdr.generic.type		= MTYPE_RADIOBUTTON;
	s_graphicsoptions.hdr.generic.name		= "HDR Display:";
	s_graphicsoptions.hdr.generic.flags		= QMF_PULSEIFFOCUS|QMF_SMALLFONT;
	s_graphicsoptions.hdr.generic.callback	= GraphicsOptions_Event;
	s_graphicsoptions.hdr.generic.id		= ID_HDR;
	s_graphicsoptions.hdr.generic.x		= 400;
	s_graphicsoptions.hdr.generic.y		= y;
	if ( !UI_HDR_Available() ) {
		s_graphicsoptions.hdr.generic.flags |= QMF_GRAYED;
	}
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
		Menu_AddItem( &s_graphicsoptions.menu, ( void * ) &s_graphicsoptions.colordepth );
		Menu_AddItem( &s_graphicsoptions.menu, ( void * ) &s_graphicsoptions.fs );
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
	Menu_AddItem( &s_graphicsoptions.menu, ( void * ) &s_graphicsoptions.hdr );
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

