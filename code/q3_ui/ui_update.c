// ui_update.c -- engine self-update menu

#include "ui_local.h"

#define ART_BACK0		"menu/art/back_0"
#define ART_BACK1		"menu/art/back_1"
#define ART_FRAMEL		"menu/art/frame2_l"
#define ART_FRAMER		"menu/art/frame1_r"

#define ID_DOWNLOAD		10
#define ID_CANCEL		11
#define ID_RESTART		12
#define ID_BACK			20

#define PROGRESS_X		160
#define PROGRESS_Y		278
#define PROGRESS_W		320
#define PROGRESS_H		16

typedef struct {
	menuframework_s	menu;

	menutext_s		banner;
	menubitmap_s	framel;
	menubitmap_s	framer;

	menutext_s		currentLabel;
	menutext_s		currentValue;
	menutext_s		newLabel;
	menutext_s		newValue;
	menutext_s		sizeLabel;
	menutext_s		statusText;

	menutext_s		download;
	menutext_s		cancel;
	menutext_s		restart;

	menubitmap_s	back;
} updateMenuInfo_t;

static updateMenuInfo_t	s_update;

static char	currentVerBuf[64];
static char	newVerBuf[64];
static char	sizeBuf[64];
static char	statusBuf[128];
static int	lastBuiltState = -1;


/*
=================
UpdateMenu_GetState
=================
*/
static int UpdateMenu_GetState( void ) {
	return (int)trap_Cvar_VariableValue( "update_state" );
}


/*
=================
UpdateMenu_RefreshStatus
=================
*/
static void UpdateMenu_RefreshStatus( void ) {
	int state = UpdateMenu_GetState();
	int progress;

	trap_Cvar_VariableStringBuffer( "update_current", currentVerBuf, sizeof( currentVerBuf ) );
	trap_Cvar_VariableStringBuffer( "update_version", newVerBuf, sizeof( newVerBuf ) );

	{
		int bytes = (int)trap_Cvar_VariableValue( "update_size" );
		if ( bytes >= 1024 * 1024 )
			Com_sprintf( sizeBuf, sizeof( sizeBuf ), "%i.%iMB",
				bytes / (1024*1024), (bytes / (1024*1024/10)) % 10 );
		else if ( bytes > 0 )
			Com_sprintf( sizeBuf, sizeof( sizeBuf ), "%iKB", bytes / 1024 );
		else
			Q_strncpyz( sizeBuf, "", sizeof( sizeBuf ) );
	}

	switch ( state ) {
	case 0: // IDLE
		Q_strncpyz( statusBuf, "Up to date", sizeof( statusBuf ) );
		break;
	case 1: // CHECKING
		Q_strncpyz( statusBuf, "Checking for updates...", sizeof( statusBuf ) );
		break;
	case 2: // AVAILABLE
		Q_strncpyz( statusBuf, "Update available", sizeof( statusBuf ) );
		break;
	case 3: // DOWNLOADING
		progress = (int)trap_Cvar_VariableValue( "update_progress" );
		Com_sprintf( statusBuf, sizeof( statusBuf ), "Downloading... %i%%", progress );
		break;
	case 4: // EXTRACTING
		Q_strncpyz( statusBuf, "Extracting...", sizeof( statusBuf ) );
		break;
	case 5: // STAGED
		Q_strncpyz( statusBuf, "Update ready! Restart to apply.", sizeof( statusBuf ) );
		break;
	case 6: // ERROR
		trap_Cvar_VariableStringBuffer( "update_error", statusBuf, sizeof( statusBuf ) );
		break;
	}

}


/*
=================
UpdateMenu_Draw
=================
*/
static void UpdateMenu_Draw( void ) {
	int state;
	vec4_t barBg = { 0.2f, 0.2f, 0.2f, 0.8f };
	vec4_t barFg = { 0.0f, 0.6f, 0.0f, 0.9f };
	vec4_t barBorder = { 0.5f, 0.5f, 0.5f, 1.0f };

	state = UpdateMenu_GetState();

	// rebuild menu when state changes so buttons become clickable
	if ( state != lastBuiltState ) {
		UI_PopMenu();
		UI_UpdateMenu();
		return;
	}

	UpdateMenu_RefreshStatus();

	Menu_Draw( &s_update.menu );

	// draw progress bar during download
	state = UpdateMenu_GetState();
	if ( state == 3 ) {
		int progress = (int)trap_Cvar_VariableValue( "update_progress" );
		float fillW;

		if ( progress < 0 ) progress = 0;
		if ( progress > 100 ) progress = 100;
		fillW = ( PROGRESS_W * progress ) / 100.0f;

		// background
		UI_FillRect( PROGRESS_X, PROGRESS_Y, PROGRESS_W, PROGRESS_H, barBg );
		// fill
		if ( fillW > 0 )
			UI_FillRect( PROGRESS_X, PROGRESS_Y, fillW, PROGRESS_H, barFg );
		// border
		UI_DrawRect( PROGRESS_X, PROGRESS_Y, PROGRESS_W, PROGRESS_H, barBorder );
	}
}


/*
=================
UI_UpdateMenu_Event
=================
*/
static void UI_UpdateMenu_Event( void *ptr, int event ) {
	if ( event != QM_ACTIVATED )
		return;

	switch ( ((menucommon_s *)ptr)->id ) {
	case ID_DOWNLOAD:
		trap_Cmd_ExecuteText( EXEC_APPEND, "updatedownload\n" );
		break;
	case ID_CANCEL:
		trap_Cmd_ExecuteText( EXEC_APPEND, "updatecancel\n" );
		break;
	case ID_RESTART:
		trap_Cmd_ExecuteText( EXEC_NOW, "updaterestart\n" );
		break;
	case ID_BACK:
		UI_PopMenu();
		break;
	}
}


/*
=================
UI_UpdateMenu_Init
=================
*/
static void UI_UpdateMenu_Init( void ) {
	int y;
	int style = UI_CENTER | UI_DROPSHADOW;

	UI_UpdateMenu_Cache();

	lastBuiltState = UpdateMenu_GetState();

	memset( &s_update, 0, sizeof( s_update ) );
	s_update.menu.wrapAround = qtrue;
	s_update.menu.fullscreen = qtrue;
	s_update.menu.draw = UpdateMenu_Draw;

	// banner
	s_update.banner.generic.type	= MTYPE_BTEXT;
	s_update.banner.generic.x		= 320;
	s_update.banner.generic.y		= 16;
	s_update.banner.string			= "ENGINE UPDATE";
	s_update.banner.color			= color_white;
	s_update.banner.style			= UI_CENTER;

	// frames
	s_update.framel.generic.type	= MTYPE_BITMAP;
	s_update.framel.generic.name	= ART_FRAMEL;
	s_update.framel.generic.flags	= QMF_INACTIVE;
	s_update.framel.generic.x		= 0;
	s_update.framel.generic.y		= 78;
	s_update.framel.width			= 256;
	s_update.framel.height			= 329;

	s_update.framer.generic.type	= MTYPE_BITMAP;
	s_update.framer.generic.name	= ART_FRAMER;
	s_update.framer.generic.flags	= QMF_INACTIVE;
	s_update.framer.generic.x		= 376;
	s_update.framer.generic.y		= 76;
	s_update.framer.width			= 256;
	s_update.framer.height			= 334;

	// version info
	y = 180;

	s_update.currentLabel.generic.type	= MTYPE_TEXT;
	s_update.currentLabel.generic.x		= 240;
	s_update.currentLabel.generic.y		= y;
	s_update.currentLabel.string		= "Current:";
	s_update.currentLabel.color			= color_white;
	s_update.currentLabel.style			= UI_RIGHT | UI_SMALLFONT;

	s_update.currentValue.generic.type	= MTYPE_TEXT;
	s_update.currentValue.generic.x		= 250;
	s_update.currentValue.generic.y		= y;
	s_update.currentValue.string		= currentVerBuf;
	s_update.currentValue.color			= color_white;
	s_update.currentValue.style			= UI_LEFT | UI_SMALLFONT;

	y += SMALLCHAR_HEIGHT + 4;

	s_update.newLabel.generic.type		= MTYPE_TEXT;
	s_update.newLabel.generic.x			= 240;
	s_update.newLabel.generic.y			= y;
	s_update.newLabel.string			= "Available:";
	s_update.newLabel.color				= color_white;
	s_update.newLabel.style				= UI_RIGHT | UI_SMALLFONT;

	s_update.newValue.generic.type		= MTYPE_TEXT;
	s_update.newValue.generic.x			= 250;
	s_update.newValue.generic.y			= y;
	s_update.newValue.string			= newVerBuf;
	s_update.newValue.color				= color_yellow;
	s_update.newValue.style				= UI_LEFT | UI_SMALLFONT;

	y += SMALLCHAR_HEIGHT + 2;

	s_update.sizeLabel.generic.type		= MTYPE_TEXT;
	s_update.sizeLabel.generic.x		= 320;
	s_update.sizeLabel.generic.y		= y;
	s_update.sizeLabel.string			= sizeBuf;
	s_update.sizeLabel.color			= color_white;
	s_update.sizeLabel.style			= UI_CENTER | UI_SMALLFONT;

	y += SMALLCHAR_HEIGHT + 20;

	// status text
	s_update.statusText.generic.type	= MTYPE_TEXT;
	s_update.statusText.generic.x		= 320;
	s_update.statusText.generic.y		= y;
	s_update.statusText.string			= statusBuf;
	s_update.statusText.color			= color_white;
	s_update.statusText.style			= UI_CENTER | UI_SMALLFONT;

	// action buttons: progress bar is at 278, 16px tall, so buttons at 302
	y = 302;

	s_update.download.generic.type		= MTYPE_PTEXT;
	s_update.download.generic.flags		= QMF_CENTER_JUSTIFY | QMF_PULSEIFFOCUS;
	s_update.download.generic.x			= 320;
	s_update.download.generic.y			= y;
	s_update.download.generic.id		= ID_DOWNLOAD;
	s_update.download.generic.callback	= UI_UpdateMenu_Event;
	s_update.download.string			= "DOWNLOAD";
	s_update.download.color				= color_red;
	s_update.download.style				= style;

	s_update.cancel.generic.type		= MTYPE_PTEXT;
	s_update.cancel.generic.flags		= QMF_CENTER_JUSTIFY | QMF_PULSEIFFOCUS;
	s_update.cancel.generic.x			= 320;
	s_update.cancel.generic.y			= y;
	s_update.cancel.generic.id			= ID_CANCEL;
	s_update.cancel.generic.callback	= UI_UpdateMenu_Event;
	s_update.cancel.string				= "CANCEL";
	s_update.cancel.color				= color_red;
	s_update.cancel.style				= style;

	s_update.restart.generic.type		= MTYPE_PTEXT;
	s_update.restart.generic.flags		= QMF_CENTER_JUSTIFY | QMF_PULSEIFFOCUS;
	s_update.restart.generic.x			= 320;
	s_update.restart.generic.y			= y;
	s_update.restart.generic.id			= ID_RESTART;
	s_update.restart.generic.callback	= UI_UpdateMenu_Event;
	s_update.restart.string				= "RESTART NOW";
	s_update.restart.color				= color_red;
	s_update.restart.style				= style;

	// back button
	s_update.back.generic.type			= MTYPE_BITMAP;
	s_update.back.generic.name			= ART_BACK0;
	s_update.back.generic.flags			= QMF_LEFT_JUSTIFY | QMF_PULSEIFFOCUS;
	s_update.back.generic.id			= ID_BACK;
	s_update.back.generic.callback		= UI_UpdateMenu_Event;
	s_update.back.generic.x			= 0;
	s_update.back.generic.y			= 480 - 64;
	s_update.back.width					= 128;
	s_update.back.height				= 64;
	s_update.back.focuspic				= ART_BACK1;

	Menu_AddItem( &s_update.menu, &s_update.banner );
	Menu_AddItem( &s_update.menu, &s_update.framel );
	Menu_AddItem( &s_update.menu, &s_update.framer );
	Menu_AddItem( &s_update.menu, &s_update.currentLabel );
	Menu_AddItem( &s_update.menu, &s_update.currentValue );
	Menu_AddItem( &s_update.menu, &s_update.newLabel );
	Menu_AddItem( &s_update.menu, &s_update.newValue );
	Menu_AddItem( &s_update.menu, &s_update.sizeLabel );
	Menu_AddItem( &s_update.menu, &s_update.statusText );

	// only add the action button relevant to the current state
	if ( lastBuiltState == 2 ) {
		Menu_AddItem( &s_update.menu, &s_update.download );
	} else if ( lastBuiltState == 3 ) {
		Menu_AddItem( &s_update.menu, &s_update.cancel );
	} else if ( lastBuiltState == 5 ) {
		Menu_AddItem( &s_update.menu, &s_update.restart );
	}

	Menu_AddItem( &s_update.menu, &s_update.back );

	UpdateMenu_RefreshStatus();
}


/*
=================
UI_UpdateMenu_Cache
=================
*/
void UI_UpdateMenu_Cache( void ) {
	trap_R_RegisterShaderNoMip( ART_BACK0 );
	trap_R_RegisterShaderNoMip( ART_BACK1 );
	trap_R_RegisterShaderNoMip( ART_FRAMEL );
	trap_R_RegisterShaderNoMip( ART_FRAMER );
}


/*
=================
UI_UpdateMenu
=================
*/
void UI_UpdateMenu( void ) {
	UI_UpdateMenu_Init();
	UI_PushMenu( &s_update.menu );
}
