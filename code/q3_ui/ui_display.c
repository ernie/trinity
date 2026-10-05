// Copyright (C) 1999-2000 Id Software, Inc.
//
/*
=======================================================================

DISPLAY OPTIONS MENU

=======================================================================
*/

#include "ui_local.h"


#define ART_FRAMEL			"menu/art/frame2_l"
#define ART_FRAMER			"menu/art/frame1_r"
#define ART_BACK0			"menu/art/back_0"
#define ART_BACK1			"menu/art/back_1"

#define ID_GRAPHICS			10
#define ID_DISPLAY			11
#define ID_SOUND			12
#define ID_NETWORK			13
#define ID_BRIGHTNESS		14
#define ID_SCREENSIZE		15
#define ID_BACK				16
#define ID_HDRCALIB			17
#define ID_VRMODE			18
#define ID_APPLY			19
#define ID_FBO				20
#define ID_HDR				21
#define ID_BLOOM			22
#define ID_FLARES			23
#define ART_APPLY0 "menu/art/accept_0"
#define ART_APPLY1 "menu/art/accept_1"


typedef struct {
	menuframework_s	menu;

	menutext_s		banner;
	menubitmap_s	framel;
	menubitmap_s	framer;

	menutext_s		graphics;
	menutext_s		display;
	menutext_s		sound;
	menutext_s		network;

	menuslider_s	brightness;
	menuradiobutton_s	fbo;
	menuradiobutton_s	hdr;
	menuradiobutton_s	bloom;
	menuradiobutton_s	flares;
	menuslider_s	screensize;

	menutext_s		hdrcalib;

	menulist_s mode;
	menubitmap_s apply;
	menubitmap_s	back;

	// latched values as the page opened; Apply lights when a row differs
	int				initialFbo;
	int				initialHdr;
	int				initialBloom;
} displayOptionsInfo_t;

static displayOptionsInfo_t	displayOptionsInfo;
static const char *displayModes[] = { "Flatscreen", "VR", NULL };

static qboolean				displayOptions_vr;


/*
=================
UI_DisplayOptionsMenu_UpdateItems

Gates the frame-buffer rows on the pending Frame Buffer value and lights Apply
=================
*/
static void UI_DisplayOptionsMenu_UpdateItems( void ) {
	qboolean fbo = displayOptionsInfo.fbo.curvalue != 0;
	qboolean hdrAvail = fbo && UI_HDR_Available();

	// HDR output never takes effect without the frame buffer or an output that can show it, so it reads off
	if ( !hdrAvail ) {
		displayOptionsInfo.hdr.curvalue = 0;
	}

	if ( hdrAvail ) {
		displayOptionsInfo.hdr.generic.flags &= ~QMF_GRAYED;
	} else {
		displayOptionsInfo.hdr.generic.flags |= QMF_GRAYED;
	}
	// calibration tunes HDR output, so it needs the HDR Display choice on as well
	if ( hdrAvail && displayOptionsInfo.hdr.curvalue && UI_HDR_CalibrationAvailable() ) {
		displayOptionsInfo.hdrcalib.generic.flags &= ~QMF_GRAYED;
	} else {
		displayOptionsInfo.hdrcalib.generic.flags |= QMF_GRAYED;
	}
	if ( fbo ) {
		displayOptionsInfo.bloom.generic.flags &= ~QMF_GRAYED;
	} else {
		displayOptionsInfo.bloom.generic.flags |= QMF_GRAYED;
	}

	if ( displayOptionsInfo.fbo.curvalue != displayOptionsInfo.initialFbo ||
		displayOptionsInfo.hdr.curvalue != displayOptionsInfo.initialHdr ||
		displayOptionsInfo.bloom.curvalue != displayOptionsInfo.initialBloom ||
		UI_VR_ModePending() ) {
		displayOptionsInfo.apply.generic.flags &= ~(QMF_HIDDEN|QMF_INACTIVE);
	} else {
		displayOptionsInfo.apply.generic.flags |= QMF_HIDDEN|QMF_INACTIVE;
	}
}


/*
=================
UI_DisplayOptionsMenu_ApplyChanges
=================
*/
static void UI_DisplayOptionsMenu_ApplyChanges( void ) {
	// only changed rows are written, so a console value outside the toggle's 0/1 survives
	if ( displayOptionsInfo.fbo.curvalue != displayOptionsInfo.initialFbo ) {
		trap_Cvar_SetValue( "r_fbo", displayOptionsInfo.fbo.curvalue );
	}
	if ( displayOptionsInfo.hdr.curvalue != displayOptionsInfo.initialHdr ) {
		trap_Cvar_SetValue( "r_hdrDisplay", displayOptionsInfo.hdr.curvalue );
	}
	if ( displayOptionsInfo.bloom.curvalue != displayOptionsInfo.initialBloom ) {
		trap_Cvar_SetValue( "r_bloom", displayOptionsInfo.bloom.curvalue );
	}
	trap_Cmd_ExecuteText( EXEC_APPEND, "vid_restart\n" );
}


/*
=================
UI_DisplayOptionsMenu_Event
=================
*/
static void UI_DisplayOptionsMenu_Event( void* ptr, int event ) {
	if( event != QM_ACTIVATED ) {
		return;
	}

	switch( ((menucommon_s*)ptr)->id ) {
	case ID_GRAPHICS:
		UI_PopMenu();
		UI_GraphicsOptionsMenu();
		break;

	case ID_DISPLAY:
		break;

	case ID_SOUND:
		UI_PopMenu();
		UI_SoundOptionsMenu();
		break;

	case ID_NETWORK:
		UI_PopMenu();
		UI_NetworkOptionsMenu();
		break;

	case ID_BRIGHTNESS:
		trap_Cvar_SetValue( "r_gamma", displayOptionsInfo.brightness.curvalue / 10.0f );
		break;

	case ID_SCREENSIZE:
		if ( !displayOptions_vr ) {
			trap_Cvar_SetValue( "cg_viewsize", displayOptionsInfo.screensize.curvalue * 10 );
		}
		break;

	case ID_FBO:
	case ID_HDR:
	case ID_BLOOM:
		UI_DisplayOptionsMenu_UpdateItems();
		break;

	case ID_FLARES:
		trap_Cvar_SetValue( "r_flares", displayOptionsInfo.flares.curvalue );
		break;

	case ID_VRMODE:
		UI_VR_ChooseMode( displayOptionsInfo.mode.curvalue != 0 );
		UI_DisplayOptionsMenu_UpdateItems();
		break;

	case ID_APPLY:
		UI_DisplayOptionsMenu_ApplyChanges();
		break;

	case ID_HDRCALIB:
		UI_HDRCalibrationMenu();
		break;

	case ID_BACK:
		UI_PopMenu();
		break;
	}
}


/*
===============
UI_DisplayOptionsMenu_Init
===============
*/
static void UI_DisplayOptionsMenu_Init( void ) {
	int		y;
	int		rows;

	memset( &displayOptionsInfo, 0, sizeof(displayOptionsInfo) );

	UI_DisplayOptionsMenu_Cache();

	displayOptions_vr = ( UI_VR_Platform() != VRP_NONE );

	displayOptionsInfo.menu.wrapAround = qtrue;
	displayOptionsInfo.menu.fullscreen = qtrue;

	displayOptionsInfo.banner.generic.type		= MTYPE_BTEXT;
	displayOptionsInfo.banner.generic.flags		= QMF_CENTER_JUSTIFY;
	displayOptionsInfo.banner.generic.x			= 320;
	displayOptionsInfo.banner.generic.y			= 16;
	displayOptionsInfo.banner.string			= "SYSTEM SETUP";
	displayOptionsInfo.banner.color				= color_white;
	displayOptionsInfo.banner.style				= UI_CENTER;

	displayOptionsInfo.framel.generic.type		= MTYPE_BITMAP;
	displayOptionsInfo.framel.generic.name		= ART_FRAMEL;
	displayOptionsInfo.framel.generic.flags		= QMF_INACTIVE;
	displayOptionsInfo.framel.generic.x			= 0;
	displayOptionsInfo.framel.generic.y			= 78;
	displayOptionsInfo.framel.width				= 256;
	displayOptionsInfo.framel.height			= 329;

	displayOptionsInfo.framer.generic.type		= MTYPE_BITMAP;
	displayOptionsInfo.framer.generic.name		= ART_FRAMER;
	displayOptionsInfo.framer.generic.flags		= QMF_INACTIVE;
	displayOptionsInfo.framer.generic.x			= 376;
	displayOptionsInfo.framer.generic.y			= 76;
	displayOptionsInfo.framer.width				= 256;
	displayOptionsInfo.framer.height			= 334;

	displayOptionsInfo.graphics.generic.type		= MTYPE_PTEXT;
	displayOptionsInfo.graphics.generic.flags		= QMF_RIGHT_JUSTIFY|QMF_PULSEIFFOCUS;
	displayOptionsInfo.graphics.generic.id			= ID_GRAPHICS;
	displayOptionsInfo.graphics.generic.callback	= UI_DisplayOptionsMenu_Event;
	displayOptionsInfo.graphics.generic.x			= 216;
	displayOptionsInfo.graphics.generic.y			= 240 - 2 * PROP_HEIGHT;
	displayOptionsInfo.graphics.string				= "GRAPHICS";
	displayOptionsInfo.graphics.style				= UI_RIGHT;
	displayOptionsInfo.graphics.color				= color_red;

	displayOptionsInfo.display.generic.type			= MTYPE_PTEXT;
	displayOptionsInfo.display.generic.flags		= QMF_RIGHT_JUSTIFY;
	displayOptionsInfo.display.generic.id			= ID_DISPLAY;
	displayOptionsInfo.display.generic.callback		= UI_DisplayOptionsMenu_Event;
	displayOptionsInfo.display.generic.x			= 216;
	displayOptionsInfo.display.generic.y			= 240 - PROP_HEIGHT;
	displayOptionsInfo.display.string				= "DISPLAY";
	displayOptionsInfo.display.style				= UI_RIGHT;
	displayOptionsInfo.display.color				= color_red;

	displayOptionsInfo.sound.generic.type			= MTYPE_PTEXT;
	displayOptionsInfo.sound.generic.flags			= QMF_RIGHT_JUSTIFY|QMF_PULSEIFFOCUS;
	displayOptionsInfo.sound.generic.id				= ID_SOUND;
	displayOptionsInfo.sound.generic.callback		= UI_DisplayOptionsMenu_Event;
	displayOptionsInfo.sound.generic.x				= 216;
	displayOptionsInfo.sound.generic.y				= 240;
	displayOptionsInfo.sound.string					= "SOUND";
	displayOptionsInfo.sound.style					= UI_RIGHT;
	displayOptionsInfo.sound.color					= color_red;

	displayOptionsInfo.network.generic.type			= MTYPE_PTEXT;
	displayOptionsInfo.network.generic.flags		= QMF_RIGHT_JUSTIFY|QMF_PULSEIFFOCUS;
	displayOptionsInfo.network.generic.id			= ID_NETWORK;
	displayOptionsInfo.network.generic.callback		= UI_DisplayOptionsMenu_Event;
	displayOptionsInfo.network.generic.x			= 216;
	displayOptionsInfo.network.generic.y			= 240 + PROP_HEIGHT;
	displayOptionsInfo.network.string				= "NETWORK";
	displayOptionsInfo.network.style				= UI_RIGHT;
	displayOptionsInfo.network.color				= color_red;

	// rows counted for centering the content column, including the optional Display Mode and Screen Size
	rows = 7;
	if ( UI_VR_CanSwitchMode() ) {
		rows++;
	}
	if ( !displayOptions_vr ) {
		rows++;
	}
	y = 242 - ( rows * (BIGCHAR_HEIGHT+2) ) / 2;

	if (UI_VR_CanSwitchMode()) {
		displayOptionsInfo.mode.generic.type = MTYPE_SPINCONTROL;
		displayOptionsInfo.mode.generic.flags = QMF_PULSEIFFOCUS|QMF_SMALLFONT;
		displayOptionsInfo.mode.generic.x = 400;
		displayOptionsInfo.mode.generic.y = y;
		displayOptionsInfo.mode.generic.id = ID_VRMODE;
		displayOptionsInfo.mode.generic.callback = UI_DisplayOptionsMenu_Event;
		displayOptionsInfo.mode.generic.name = "Display Mode:";
		displayOptionsInfo.mode.itemnames = displayModes;
		displayOptionsInfo.mode.curvalue = UI_VR_ModeChoice();
		y += BIGCHAR_HEIGHT+2;
	}

	displayOptionsInfo.brightness.generic.type		= MTYPE_SLIDER;
	displayOptionsInfo.brightness.generic.name		= "Brightness:";
	displayOptionsInfo.brightness.generic.flags		= QMF_PULSEIFFOCUS|QMF_SMALLFONT;
	displayOptionsInfo.brightness.generic.callback	= UI_DisplayOptionsMenu_Event;
	displayOptionsInfo.brightness.generic.id		= ID_BRIGHTNESS;
	displayOptionsInfo.brightness.generic.x			= 400;
	displayOptionsInfo.brightness.generic.y			= y;
	displayOptionsInfo.brightness.minvalue			= 5;
	displayOptionsInfo.brightness.maxvalue			= 20;
	// the brightness slider sets r_gamma, which is always valid, so the row
	// is never grayed
	y += BIGCHAR_HEIGHT+2;

	displayOptionsInfo.fbo.generic.type			= MTYPE_RADIOBUTTON;
	displayOptionsInfo.fbo.generic.name			= "Frame Buffer:";
	displayOptionsInfo.fbo.generic.flags		= QMF_PULSEIFFOCUS|QMF_SMALLFONT;
	displayOptionsInfo.fbo.generic.callback		= UI_DisplayOptionsMenu_Event;
	displayOptionsInfo.fbo.generic.id			= ID_FBO;
	displayOptionsInfo.fbo.generic.x			= 400;
	displayOptionsInfo.fbo.generic.y			= y;
	y += BIGCHAR_HEIGHT+2;

	displayOptionsInfo.hdr.generic.type			= MTYPE_RADIOBUTTON;
	displayOptionsInfo.hdr.generic.name			= "HDR Display:";
	displayOptionsInfo.hdr.generic.flags		= QMF_PULSEIFFOCUS|QMF_SMALLFONT;
	displayOptionsInfo.hdr.generic.callback		= UI_DisplayOptionsMenu_Event;
	displayOptionsInfo.hdr.generic.id			= ID_HDR;
	displayOptionsInfo.hdr.generic.x			= 400;
	displayOptionsInfo.hdr.generic.y			= y;
	y += BIGCHAR_HEIGHT+2;

	displayOptionsInfo.bloom.generic.type		= MTYPE_RADIOBUTTON;
	displayOptionsInfo.bloom.generic.name		= "Bloom:";
	displayOptionsInfo.bloom.generic.flags		= QMF_PULSEIFFOCUS|QMF_SMALLFONT;
	displayOptionsInfo.bloom.generic.callback	= UI_DisplayOptionsMenu_Event;
	displayOptionsInfo.bloom.generic.id			= ID_BLOOM;
	displayOptionsInfo.bloom.generic.x			= 400;
	displayOptionsInfo.bloom.generic.y			= y;
	y += BIGCHAR_HEIGHT+2;

	displayOptionsInfo.flares.generic.type		= MTYPE_RADIOBUTTON;
	displayOptionsInfo.flares.generic.name		= "Flares:";
	displayOptionsInfo.flares.generic.flags		= QMF_PULSEIFFOCUS|QMF_SMALLFONT;
	displayOptionsInfo.flares.generic.callback	= UI_DisplayOptionsMenu_Event;
	displayOptionsInfo.flares.generic.id		= ID_FLARES;
	displayOptionsInfo.flares.generic.x			= 400;
	displayOptionsInfo.flares.generic.y			= y;
	y += BIGCHAR_HEIGHT+2;

	// Screen Size is a flatscreen-only control; the runtime owns the view under VR
	if( !displayOptions_vr ) {
		displayOptionsInfo.screensize.generic.type		= MTYPE_SLIDER;
		displayOptionsInfo.screensize.generic.name		= "Screen Size:";
		displayOptionsInfo.screensize.generic.flags		= QMF_PULSEIFFOCUS|QMF_SMALLFONT;
		displayOptionsInfo.screensize.generic.callback	= UI_DisplayOptionsMenu_Event;
		displayOptionsInfo.screensize.generic.id		= ID_SCREENSIZE;
		displayOptionsInfo.screensize.generic.x			= 400;
		displayOptionsInfo.screensize.generic.y			= y;
		displayOptionsInfo.screensize.minvalue			= 3;
		displayOptionsInfo.screensize.maxvalue			= 10;
		y += BIGCHAR_HEIGHT+2;
	}

	y += BIGCHAR_HEIGHT+2;	// extra gap to separate the link from the rows
	displayOptionsInfo.hdrcalib.generic.type		= MTYPE_PTEXT;
	displayOptionsInfo.hdrcalib.generic.flags		= QMF_CENTER_JUSTIFY|QMF_PULSEIFFOCUS|QMF_SMALLFONT;
	displayOptionsInfo.hdrcalib.generic.id			= ID_HDRCALIB;
	displayOptionsInfo.hdrcalib.generic.callback	= UI_DisplayOptionsMenu_Event;
	displayOptionsInfo.hdrcalib.generic.x			= 400;	// center under the slider column
	displayOptionsInfo.hdrcalib.generic.y			= y;
	displayOptionsInfo.hdrcalib.string				= "HDR Calibration";
	displayOptionsInfo.hdrcalib.style				= UI_CENTER|UI_SMALLFONT;
	displayOptionsInfo.hdrcalib.color				= color_red;

	displayOptionsInfo.apply.generic.type		= MTYPE_BITMAP;
	displayOptionsInfo.apply.generic.name		= ART_APPLY0;
	displayOptionsInfo.apply.generic.flags		= QMF_RIGHT_JUSTIFY|QMF_PULSEIFFOCUS|QMF_HIDDEN|QMF_INACTIVE;
	displayOptionsInfo.apply.generic.x			= 640;
	displayOptionsInfo.apply.generic.y			= 416;
	displayOptionsInfo.apply.generic.id			= ID_APPLY;
	displayOptionsInfo.apply.generic.callback	= UI_DisplayOptionsMenu_Event;
	displayOptionsInfo.apply.width				= 128;
	displayOptionsInfo.apply.height				= 64;
	displayOptionsInfo.apply.focuspic			= ART_APPLY1;

	displayOptionsInfo.back.generic.type		= MTYPE_BITMAP;
	displayOptionsInfo.back.generic.name		= ART_BACK0;
	displayOptionsInfo.back.generic.flags		= QMF_LEFT_JUSTIFY|QMF_PULSEIFFOCUS;
	displayOptionsInfo.back.generic.callback	= UI_DisplayOptionsMenu_Event;
	displayOptionsInfo.back.generic.id			= ID_BACK;
	displayOptionsInfo.back.generic.x			= 0;
	displayOptionsInfo.back.generic.y			= 480-64;
	displayOptionsInfo.back.width				= 128;
	displayOptionsInfo.back.height				= 64;
	displayOptionsInfo.back.focuspic			= ART_BACK1;

	Menu_AddItem( &displayOptionsInfo.menu, ( void * ) &displayOptionsInfo.banner );
	Menu_AddItem( &displayOptionsInfo.menu, ( void * ) &displayOptionsInfo.framel );
	Menu_AddItem( &displayOptionsInfo.menu, ( void * ) &displayOptionsInfo.framer );
	Menu_AddItem( &displayOptionsInfo.menu, ( void * ) &displayOptionsInfo.graphics );
	Menu_AddItem( &displayOptionsInfo.menu, ( void * ) &displayOptionsInfo.display );
	Menu_AddItem( &displayOptionsInfo.menu, ( void * ) &displayOptionsInfo.sound );
	Menu_AddItem( &displayOptionsInfo.menu, ( void * ) &displayOptionsInfo.network );
	if (UI_VR_CanSwitchMode()) {
		Menu_AddItem(&displayOptionsInfo.menu, &displayOptionsInfo.mode);
	}
	Menu_AddItem( &displayOptionsInfo.menu, ( void * ) &displayOptionsInfo.brightness );
	Menu_AddItem( &displayOptionsInfo.menu, ( void * ) &displayOptionsInfo.fbo );
	Menu_AddItem( &displayOptionsInfo.menu, ( void * ) &displayOptionsInfo.hdr );
	Menu_AddItem( &displayOptionsInfo.menu, ( void * ) &displayOptionsInfo.bloom );
	Menu_AddItem( &displayOptionsInfo.menu, ( void * ) &displayOptionsInfo.flares );
	if( !displayOptions_vr ) {
		Menu_AddItem( &displayOptionsInfo.menu, ( void * ) &displayOptionsInfo.screensize );
	}
	Menu_AddItem( &displayOptionsInfo.menu, ( void * ) &displayOptionsInfo.hdrcalib );
	Menu_AddItem( &displayOptionsInfo.menu, ( void * ) &displayOptionsInfo.apply );
	Menu_AddItem( &displayOptionsInfo.menu, ( void * ) &displayOptionsInfo.back );

	displayOptionsInfo.brightness.curvalue  = trap_Cvar_VariableValue("r_gamma") * 10;
	if( !displayOptions_vr ) {
		displayOptionsInfo.screensize.curvalue  = trap_Cvar_VariableValue( "cg_viewsize")/10;
	}

	// an HDR request without the frame buffer never takes effect, so switch it off rather than show it on
	if ( UI_HDR_FBOOff() && trap_Cvar_VariableValue( "r_hdrDisplay" ) != 0 ) {
		trap_Cvar_Set( "r_hdrDisplay", "0" );
	}
	displayOptionsInfo.fbo.curvalue		= !UI_HDR_FBOOff();
	// shown off where nothing shown can do HDR, keeping the choice for an output that can
	displayOptionsInfo.hdr.curvalue		= UI_HDR_Available() && trap_Cvar_VariableValue( "r_hdrDisplay" ) != 0;
	displayOptionsInfo.bloom.curvalue	= trap_Cvar_VariableValue( "r_bloom" ) != 0;
	displayOptionsInfo.flares.curvalue	= trap_Cvar_VariableValue( "r_flares" ) != 0;
	displayOptionsInfo.initialFbo		= displayOptionsInfo.fbo.curvalue;
	displayOptionsInfo.initialHdr		= displayOptionsInfo.hdr.curvalue;
	displayOptionsInfo.initialBloom		= displayOptionsInfo.bloom.curvalue;

	UI_DisplayOptionsMenu_UpdateItems();
}


/*
===============
UI_DisplayOptionsMenu_Cache
===============
*/
void UI_DisplayOptionsMenu_Cache( void ) {
	trap_R_RegisterShaderNoMip(ART_APPLY0);
	trap_R_RegisterShaderNoMip(ART_APPLY1);
	trap_R_RegisterShaderNoMip( ART_FRAMEL );
	trap_R_RegisterShaderNoMip( ART_FRAMER );
	trap_R_RegisterShaderNoMip( ART_BACK0 );
	trap_R_RegisterShaderNoMip( ART_BACK1 );
}


/*
===============
UI_DisplayOptionsMenu
===============
*/
void UI_DisplayOptionsMenu( void ) {
	UI_DisplayOptionsMenu_Init();
	UI_PushMenu( &displayOptionsInfo.menu );
	Menu_SetCursorToItem( &displayOptionsInfo.menu, &displayOptionsInfo.display );
}
