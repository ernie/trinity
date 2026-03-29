// Copyright (C) 1999-2000 Id Software, Inc.
//
//
// ui_login.c -- Trinity tracker login menu
//

#include "ui_local.h"


#define LOGIN_FRAME		"menu/art/cut_frame"
#define ART_BACK0		"menu/art/back_0"
#define ART_BACK1		"menu/art/back_1"

#define ID_NAME			100
#define ID_NAME_BOX		101
#define ID_PASSWORD		102
#define ID_PASSWORD_BOX	103
#define ID_LOGIN		104
#define ID_BACK		105
#define ID_LOGOUT		106

#define LOGIN_STATE_IDLE		0
#define LOGIN_STATE_WAITING		1


typedef struct
{
	menuframework_s	menu;
	menubitmap_s	frame;
	menutext_s		title;
	menutext_s		name;
	menufield_s		name_box;
	menutext_s		password;
	menufield_s		password_box;
	menutext_s		login;
	menubitmap_s	back;
	menutext_s		logout;
	menutext_s		loggedInAs;
	menutext_s		loggedInName;
	menutext_s		statusText;
	char			loggedInLabel[80];
	qboolean		isLoggedIn;
	int				loginState;
} login_t;

static login_t	s_login;

static vec4_t s_login_color_prompt  = {1.00f, 0.43f, 0.00f, 1.00f};
static vec4_t s_login_color_status  = {1.00f, 1.00f, 0.00f, 1.00f};
static vec4_t s_login_color_success = {0.00f, 1.00f, 0.00f, 1.00f};
static vec4_t s_login_color_fail    = {1.00f, 0.00f, 0.00f, 1.00f};

/*
===============
Login_DrawPassword

Custom ownerdraw for password field — masks characters with '*'.
===============
*/
static void Login_DrawPassword( void *self ) {
	menufield_s		*f;
	char			saved[MAX_EDIT_LINE];
	int				len;
	int				i;

	f = (menufield_s *)self;

	len = strlen( f->field.buffer );
	if ( f->field.cursor > len ) {
		f->field.cursor = len;
	}

	// save, mask, draw, restore
	Q_strncpyz( saved, f->field.buffer, sizeof( saved ) );
	for ( i = 0; i < len; i++ ) {
		f->field.buffer[i] = '*';
	}
	MenuField_Draw( f );
	Q_strncpyz( f->field.buffer, saved, sizeof( f->field.buffer ) );
}

/*
===============
Login_MenuDraw
===============
*/
static void Login_MenuDraw( void ) {
	// Poll login status while waiting
	if ( s_login.loginState == LOGIN_STATE_WAITING ) {
		char status[32];
		trap_Cvar_VariableStringBuffer( "cl_trinityLoginStatus", status, sizeof( status ) );

		if ( !Q_stricmp( status, "success" ) ) {
			s_login.statusText.string = "Login successful!";
			s_login.statusText.color = s_login_color_success;
			s_login.loginState = LOGIN_STATE_IDLE;
			// Dismiss after brief display — re-init will show logged-in view
			UI_PopMenu();
			UI_LoginMenu();
			return;
		} else if ( !Q_stricmp( status, "failed" ) ) {
			s_login.statusText.string = "Login failed.";
			s_login.statusText.color = s_login_color_fail;
			s_login.loginState = LOGIN_STATE_IDLE;
			// Re-enable login button
			s_login.login.generic.flags &= ~QMF_GRAYED;
		} else {
			s_login.statusText.string = "Logging in...";
			s_login.statusText.color = s_login_color_status;
		}
	}

	Menu_Draw( &s_login.menu );
}

/*
===============
Login_MenuEvent
===============
*/
static void Login_MenuEvent( void* ptr, int event ) {
	if( event != QM_ACTIVATED ) {
		return;
	}

	switch( ((menucommon_s*)ptr)->id ) {
	case ID_LOGIN:
		if ( s_login.name_box.field.buffer[0] == '\0' ||
		     s_login.password_box.field.buffer[0] == '\0' ) {
			s_login.statusText.string = "Enter name and password.";
			s_login.statusText.color = s_login_color_fail;
			return;
		}
		// Gray out login button while waiting
		s_login.login.generic.flags |= QMF_GRAYED;
		s_login.loginState = LOGIN_STATE_WAITING;
		s_login.statusText.string = "Logging in...";
		s_login.statusText.color = s_login_color_status;
		// Pass credentials via cvars to avoid command injection, then trigger login
		trap_Cvar_Set( "cl_trinityLoginUser", s_login.name_box.field.buffer );
		trap_Cvar_Set( "cl_trinityLoginPass", s_login.password_box.field.buffer );
		trap_Cmd_ExecuteText( EXEC_APPEND, "trinity_login\n" );
		break;

	case ID_LOGOUT:
		trap_Cmd_ExecuteText( EXEC_NOW, "trinity_logout\n" );
		// Re-init to show login view
		UI_PopMenu();
		UI_LoginMenu();
		break;

	case ID_BACK:
		UI_PopMenu();
		break;
	}
}


/*
===============
Login_MenuInit

Layout is designed to fit inside the oval frame.
The oval is 480x330, centered at (320,240).
Safe interior for content is roughly 320 wide, 200 tall,
centered at (320,240), so y range ~[140..340].
===============
*/
void Login_MenuInit( void ) {
	int				y;
	char			trinityUser[64];
	int				frameW = 480;
	int				frameH = 330;
	int				frameX = 320 - frameW / 2;
	int				frameY = 240 - frameH / 2;
	// label right edge and field left edge — centered pair
	int				labelX = 255;
	int				fieldX = 275;

	memset( &s_login, 0, sizeof(s_login) );

	Login_Cache();

	trap_Cvar_VariableStringBuffer( "cl_trinityUser", trinityUser, sizeof( trinityUser ) );
	s_login.isLoggedIn = ( trinityUser[0] != '\0' );

	s_login.menu.wrapAround = qtrue;
	s_login.menu.fullscreen = qtrue;
	s_login.menu.draw = Login_MenuDraw;

	s_login.frame.generic.type			= MTYPE_BITMAP;
	s_login.frame.generic.flags			= QMF_INACTIVE;
	s_login.frame.generic.name			= LOGIN_FRAME;
	s_login.frame.generic.x			= frameX;
	s_login.frame.generic.y			= frameY;
	s_login.frame.width					= frameW;
	s_login.frame.height				= frameH;

	y = 165;	// centered vertically within the oval

	s_login.title.generic.type			= MTYPE_PTEXT;
	s_login.title.generic.flags			= QMF_CENTER_JUSTIFY|QMF_INACTIVE;
	s_login.title.generic.x			= 320;
	s_login.title.generic.y			= y;
	s_login.title.string				= "TRINITY ACCOUNT";
	s_login.title.style					= UI_CENTER|UI_SMALLFONT;
	s_login.title.color					= color_white;

	Menu_AddItem( &s_login.menu, (void*) &s_login.frame );
	Menu_AddItem( &s_login.menu, (void*) &s_login.title );

	if ( s_login.isLoggedIn ) {
		y += 35;

		s_login.loggedInAs.generic.type		= MTYPE_PTEXT;
		s_login.loggedInAs.generic.flags	= QMF_CENTER_JUSTIFY|QMF_INACTIVE;
		s_login.loggedInAs.generic.x		= 320;
		s_login.loggedInAs.generic.y		= y;
		s_login.loggedInAs.string			= "Logged in as";
		s_login.loggedInAs.style			= UI_CENTER|UI_SMALLFONT;
		s_login.loggedInAs.color			= color_white;
		y += 20;

		s_login.loggedInName.generic.type	= MTYPE_PTEXT;
		s_login.loggedInName.generic.flags	= QMF_CENTER_JUSTIFY|QMF_INACTIVE;
		s_login.loggedInName.generic.x		= 320;
		s_login.loggedInName.generic.y		= y;
		Q_strncpyz( s_login.loggedInLabel, trinityUser, sizeof( s_login.loggedInLabel ) );
		s_login.loggedInName.string			= s_login.loggedInLabel;
		s_login.loggedInName.style			= UI_CENTER|UI_SMALLFONT;
		s_login.loggedInName.color			= s_login_color_prompt;
		y += 35;

		s_login.logout.generic.type			= MTYPE_PTEXT;
		s_login.logout.generic.flags		= QMF_CENTER_JUSTIFY|QMF_PULSEIFFOCUS;
		s_login.logout.generic.id			= ID_LOGOUT;
		s_login.logout.generic.callback		= Login_MenuEvent;
		s_login.logout.generic.x			= 320;
		s_login.logout.generic.y			= y;
		s_login.logout.string				= "LOGOUT";
		s_login.logout.style				= UI_CENTER|UI_SMALLFONT;
		s_login.logout.color				= colorRed;
		y += 30;

		Menu_AddItem( &s_login.menu, (void*) &s_login.loggedInAs );
		Menu_AddItem( &s_login.menu, (void*) &s_login.loggedInName );
		Menu_AddItem( &s_login.menu, (void*) &s_login.logout );
	} else {
		y += 40;

		s_login.name.generic.type			= MTYPE_PTEXT;
		s_login.name.generic.flags			= QMF_RIGHT_JUSTIFY|QMF_INACTIVE;
		s_login.name.generic.id				= ID_NAME;
		s_login.name.generic.x				= labelX;
		s_login.name.generic.y				= y;
		s_login.name.string					= "NAME";
		s_login.name.style					= UI_RIGHT|UI_SMALLFONT;
		s_login.name.color					= s_login_color_prompt;

		s_login.name_box.generic.type		= MTYPE_FIELD;
		s_login.name_box.generic.ownerdraw	= NULL;
		s_login.name_box.generic.name		= "";
		s_login.name_box.generic.flags		= 0;
		s_login.name_box.generic.x			= fieldX;
		s_login.name_box.generic.y			= y + 1;
		s_login.name_box.field.widthInChars	= 14;
		s_login.name_box.field.maxchars		= 32;
		y += 26;

		s_login.password.generic.type		= MTYPE_PTEXT;
		s_login.password.generic.flags		= QMF_RIGHT_JUSTIFY|QMF_INACTIVE;
		s_login.password.generic.id			= ID_PASSWORD;
		s_login.password.generic.x			= labelX;
		s_login.password.generic.y			= y;
		s_login.password.string				= "PASSWORD";
		s_login.password.style				= UI_RIGHT|UI_SMALLFONT;
		s_login.password.color				= s_login_color_prompt;

		s_login.password_box.generic.type		= MTYPE_FIELD;
		s_login.password_box.generic.ownerdraw	= Login_DrawPassword;
		s_login.password_box.generic.name		= "";
		s_login.password_box.generic.flags		= 0;
		s_login.password_box.generic.x			= fieldX;
		s_login.password_box.generic.y			= y + 1;
		s_login.password_box.field.widthInChars	= 14;
		s_login.password_box.field.maxchars		= 32;
		y += 32;

		// Status text (shown during/after login attempts)
		s_login.statusText.generic.type		= MTYPE_PTEXT;
		s_login.statusText.generic.flags	= QMF_CENTER_JUSTIFY|QMF_INACTIVE;
		s_login.statusText.generic.x		= 320;
		s_login.statusText.generic.y		= y;
		s_login.statusText.string			= "";
		s_login.statusText.style			= UI_CENTER|UI_SMALLFONT;
		s_login.statusText.color			= s_login_color_status;
		y += 32;

		s_login.login.generic.type				= MTYPE_PTEXT;
		s_login.login.generic.flags				= QMF_CENTER_JUSTIFY|QMF_PULSEIFFOCUS;
		s_login.login.generic.id				= ID_LOGIN;
		s_login.login.generic.callback			= Login_MenuEvent;
		s_login.login.generic.x					= 320;
		s_login.login.generic.y					= y;
		s_login.login.string					= "LOGIN";
		s_login.login.style						= UI_CENTER|UI_SMALLFONT;
		s_login.login.color						= colorRed;

		Menu_AddItem( &s_login.menu, (void*) &s_login.name );
		Menu_AddItem( &s_login.menu, (void*) &s_login.name_box );
		Menu_AddItem( &s_login.menu, (void*) &s_login.password );
		Menu_AddItem( &s_login.menu, (void*) &s_login.password_box );
		Menu_AddItem( &s_login.menu, (void*) &s_login.statusText );
		Menu_AddItem( &s_login.menu, (void*) &s_login.login );
	}

	// Back button — shared by both views, bottom-left corner
	s_login.back.generic.type		= MTYPE_BITMAP;
	s_login.back.generic.name		= ART_BACK0;
	s_login.back.generic.flags		= QMF_LEFT_JUSTIFY|QMF_PULSEIFFOCUS;
	s_login.back.generic.callback	= Login_MenuEvent;
	s_login.back.generic.id			= ID_BACK;
	s_login.back.generic.x			= 0;
	s_login.back.generic.y			= 480 - 64;
	s_login.back.width				= 128;
	s_login.back.height				= 64;
	s_login.back.focuspic			= ART_BACK1;
	Menu_AddItem( &s_login.menu, (void*) &s_login.back );
}


/*
===============
Login_Cache
===============
*/
void Login_Cache( void ) {
	trap_R_RegisterShaderNoMip( LOGIN_FRAME );
	trap_R_RegisterShaderNoMip( ART_BACK0 );
	trap_R_RegisterShaderNoMip( ART_BACK1 );
}


/*
===============
UI_LoginMenu
===============
*/
void UI_LoginMenu( void ) {
	Login_MenuInit();
	UI_PushMenu ( &s_login.menu );
}
