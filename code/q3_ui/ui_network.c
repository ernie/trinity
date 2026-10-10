// Copyright (C) 1999-2000 Id Software, Inc.
//
/*
=======================================================================

NETWORK OPTIONS MENU

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
#define ID_PACKETDUP		14
#define ID_BACK				15


// cl_packetdup 1-3; rate and cl_maxpackets stay at the engine defaults
static const char *packetdup_items[] = {
	"Low",
	"Medium",
	"High",
	NULL
};

typedef struct {
	menuframework_s	menu;

	menutext_s		banner;
	menubitmap_s	framel;
	menubitmap_s	framer;

	menutext_s		graphics;
	menutext_s		display;
	menutext_s		sound;
	menutext_s		network;

	menulist_s		packetdup;

	menubitmap_s	back;
} networkOptionsInfo_t;

static networkOptionsInfo_t	networkOptionsInfo;


static void UI_NetworkOptionsMenu_PacketDupStatusBar( void *self ) {
	UI_DrawString( SCREEN_WIDTH * 0.50, 424, "Send duplicate input packets", UI_SMALLFONT|UI_CENTER, colorWhite );
	UI_DrawString( SCREEN_WIDTH * 0.50, 424 + SMALLCHAR_HEIGHT, "(raise if on unreliable network)", UI_SMALLFONT|UI_CENTER, colorWhite );
}


/*
=================
UI_NetworkOptionsMenu_Event
=================
*/
static void UI_NetworkOptionsMenu_Event( void* ptr, int event ) {
	if( event != QM_ACTIVATED ) {
		return;
	}

	switch( ((menucommon_s*)ptr)->id ) {
	case ID_GRAPHICS:
		UI_PopMenu();
		UI_GraphicsOptionsMenu();
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
		break;

	case ID_PACKETDUP:
		trap_Cvar_SetValue( "cl_packetdup", networkOptionsInfo.packetdup.curvalue + 1 );
		break;

	case ID_BACK:
		UI_PopMenu();
		break;
	}
}


/*
===============
UI_NetworkOptionsMenu_Init
===============
*/
static void UI_NetworkOptionsMenu_Init( void ) {
	int			y;
	int			packetdup;

	memset( &networkOptionsInfo, 0, sizeof(networkOptionsInfo) );

	UI_NetworkOptionsMenu_Cache();

	networkOptionsInfo.menu.wrapAround = qtrue;
	networkOptionsInfo.menu.fullscreen = qtrue;

	networkOptionsInfo.banner.generic.type		= MTYPE_BTEXT;
	networkOptionsInfo.banner.generic.flags		= QMF_CENTER_JUSTIFY;
	networkOptionsInfo.banner.generic.x			= 320;
	networkOptionsInfo.banner.generic.y			= 16;
	networkOptionsInfo.banner.string			= "SYSTEM SETUP";
	networkOptionsInfo.banner.color				= color_white;
	networkOptionsInfo.banner.style				= UI_CENTER;

	networkOptionsInfo.framel.generic.type		= MTYPE_BITMAP;
	networkOptionsInfo.framel.generic.name		= ART_FRAMEL;
	networkOptionsInfo.framel.generic.flags		= QMF_INACTIVE;
	networkOptionsInfo.framel.generic.x			= 0;  
	networkOptionsInfo.framel.generic.y			= 78;
	networkOptionsInfo.framel.width				= 256;
	networkOptionsInfo.framel.height			= 329;

	networkOptionsInfo.framer.generic.type		= MTYPE_BITMAP;
	networkOptionsInfo.framer.generic.name		= ART_FRAMER;
	networkOptionsInfo.framer.generic.flags		= QMF_INACTIVE;
	networkOptionsInfo.framer.generic.x			= 376;
	networkOptionsInfo.framer.generic.y			= 76;
	networkOptionsInfo.framer.width				= 256;
	networkOptionsInfo.framer.height			= 334;

	networkOptionsInfo.graphics.generic.type		= MTYPE_PTEXT;
	networkOptionsInfo.graphics.generic.flags		= QMF_RIGHT_JUSTIFY|QMF_PULSEIFFOCUS;
	networkOptionsInfo.graphics.generic.id			= ID_GRAPHICS;
	networkOptionsInfo.graphics.generic.callback	= UI_NetworkOptionsMenu_Event;
	networkOptionsInfo.graphics.generic.x			= 216;
	networkOptionsInfo.graphics.generic.y			= 240 - 2 * PROP_HEIGHT;
	networkOptionsInfo.graphics.string				= "GRAPHICS";
	networkOptionsInfo.graphics.style				= UI_RIGHT;
	networkOptionsInfo.graphics.color				= color_red;

	networkOptionsInfo.display.generic.type			= MTYPE_PTEXT;
	networkOptionsInfo.display.generic.flags		= QMF_RIGHT_JUSTIFY|QMF_PULSEIFFOCUS;
	networkOptionsInfo.display.generic.id			= ID_DISPLAY;
	networkOptionsInfo.display.generic.callback		= UI_NetworkOptionsMenu_Event;
	networkOptionsInfo.display.generic.x			= 216;
	networkOptionsInfo.display.generic.y			= 240 - PROP_HEIGHT;
	networkOptionsInfo.display.string				= "DISPLAY";
	networkOptionsInfo.display.style				= UI_RIGHT;
	networkOptionsInfo.display.color				= color_red;

	networkOptionsInfo.sound.generic.type			= MTYPE_PTEXT;
	networkOptionsInfo.sound.generic.flags			= QMF_RIGHT_JUSTIFY|QMF_PULSEIFFOCUS;
	networkOptionsInfo.sound.generic.id				= ID_SOUND;
	networkOptionsInfo.sound.generic.callback		= UI_NetworkOptionsMenu_Event;
	networkOptionsInfo.sound.generic.x				= 216;
	networkOptionsInfo.sound.generic.y				= 240;
	networkOptionsInfo.sound.string					= "SOUND";
	networkOptionsInfo.sound.style					= UI_RIGHT;
	networkOptionsInfo.sound.color					= color_red;

	networkOptionsInfo.network.generic.type			= MTYPE_PTEXT;
	networkOptionsInfo.network.generic.flags		= QMF_RIGHT_JUSTIFY;
	networkOptionsInfo.network.generic.id			= ID_NETWORK;
	networkOptionsInfo.network.generic.callback		= UI_NetworkOptionsMenu_Event;
	networkOptionsInfo.network.generic.x			= 216;
	networkOptionsInfo.network.generic.y			= 240 + PROP_HEIGHT;
	networkOptionsInfo.network.string				= "NETWORK";
	networkOptionsInfo.network.style				= UI_RIGHT;
	networkOptionsInfo.network.color				= color_red;

	// content column (1 row) centered on the frame
	y = 242 - ( 1 * (BIGCHAR_HEIGHT+2) ) / 2;
	networkOptionsInfo.packetdup.generic.type		= MTYPE_SPINCONTROL;
	networkOptionsInfo.packetdup.generic.name		= "Packet Recovery:";
	networkOptionsInfo.packetdup.generic.flags		= QMF_PULSEIFFOCUS|QMF_SMALLFONT;
	networkOptionsInfo.packetdup.generic.callback	= UI_NetworkOptionsMenu_Event;
	networkOptionsInfo.packetdup.generic.statusbar	= UI_NetworkOptionsMenu_PacketDupStatusBar;
	networkOptionsInfo.packetdup.generic.id			= ID_PACKETDUP;
	networkOptionsInfo.packetdup.generic.x			= 400;
	networkOptionsInfo.packetdup.generic.y			= y;
	networkOptionsInfo.packetdup.itemnames			= packetdup_items;

	networkOptionsInfo.back.generic.type		= MTYPE_BITMAP;
	networkOptionsInfo.back.generic.name		= ART_BACK0;
	networkOptionsInfo.back.generic.flags		= QMF_LEFT_JUSTIFY|QMF_PULSEIFFOCUS;
	networkOptionsInfo.back.generic.callback	= UI_NetworkOptionsMenu_Event;
	networkOptionsInfo.back.generic.id			= ID_BACK;
	networkOptionsInfo.back.generic.x			= 0;
	networkOptionsInfo.back.generic.y			= 480-64;
	networkOptionsInfo.back.width				= 128;
	networkOptionsInfo.back.height				= 64;
	networkOptionsInfo.back.focuspic			= ART_BACK1;

	Menu_AddItem( &networkOptionsInfo.menu, ( void * ) &networkOptionsInfo.banner );
	Menu_AddItem( &networkOptionsInfo.menu, ( void * ) &networkOptionsInfo.framel );
	Menu_AddItem( &networkOptionsInfo.menu, ( void * ) &networkOptionsInfo.framer );
	Menu_AddItem( &networkOptionsInfo.menu, ( void * ) &networkOptionsInfo.graphics );
	Menu_AddItem( &networkOptionsInfo.menu, ( void * ) &networkOptionsInfo.display );
	Menu_AddItem( &networkOptionsInfo.menu, ( void * ) &networkOptionsInfo.sound );
	Menu_AddItem( &networkOptionsInfo.menu, ( void * ) &networkOptionsInfo.network );
	Menu_AddItem( &networkOptionsInfo.menu, ( void * ) &networkOptionsInfo.packetdup );
	Menu_AddItem( &networkOptionsInfo.menu, ( void * ) &networkOptionsInfo.back );

	packetdup = trap_Cvar_VariableValue( "cl_packetdup" );
	networkOptionsInfo.packetdup.curvalue = packetdup <= 1 ? 0 : packetdup >= 3 ? 2 : 1;
}


/*
===============
UI_NetworkOptionsMenu_Cache
===============
*/
void UI_NetworkOptionsMenu_Cache( void ) {
	trap_R_RegisterShaderNoMip( ART_FRAMEL );
	trap_R_RegisterShaderNoMip( ART_FRAMER );
	trap_R_RegisterShaderNoMip( ART_BACK0 );
	trap_R_RegisterShaderNoMip( ART_BACK1 );
}


/*
===============
UI_NetworkOptionsMenu
===============
*/
void UI_NetworkOptionsMenu( void ) {
	UI_NetworkOptionsMenu_Init();
	UI_PushMenu( &networkOptionsInfo.menu );
	Menu_SetCursorToItem( &networkOptionsInfo.menu, &networkOptionsInfo.network );
}
