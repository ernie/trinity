// VR bindings: per-tab VR controller actions, bound by pressing the button.
#include "ui_local.h"
#include "../game/vr_bindmenu.h"

#define ART_BACK0	"menu/art/back_0"
#define ART_BACK1	"menu/art/back_1"
#define ART_FRAMEL	"menu/art/frame2_l"
#define ART_FRAMER	"menu/art/frame1_r"
#define ART_RESET0	"menu/art/reset_0"
#define ART_RESET1	"menu/art/reset_1"

#define ID_TAB		10	// + tab
#define ID_ALT		20
#define ID_RESET	21
#define ID_BACK		22
#define ID_ROW		100	// + row index

#define VRB_ITEMS		12
// Inside the frame art, as the stock Controls rows are: the right oval's inner edge sits near x 570.
#define VRB_X			312
#define VRB_LEFT		160
#define VRB_GLYPH_X		540
#define VRB_RIGHT		552
#define VRB_SPACING		18

typedef struct {
	menuframework_s	menu;
	menutext_s		banner;
	menubitmap_s	framel;
	menubitmap_s	framer;
	menutext_s		tabs[VRBM_TABS];
	menulist_s		alt;
	menuaction_s	rows[VRB_ITEMS];
	menubitmap_s	reset;
	menubitmap_s	back;
	int				tab;
} vrbindings_t;

static vrbindings_t s_vrb;

// Five letters at most, as the stock Controls tabs, so they stay inside the frame art.
static const char *vrbTabNames[VRBM_TABS] = {"MOVE", "SHOOT", "WATCH", "MISC"};
static const char *vrbAltNames[] = {"No", "Yes", NULL};

static qboolean VRBindings_OnGlyph( menucommon_s *item ) {
	return !UI_VR_StickNavActive() && uis.cursorx >= VRB_GLYPH_X - 4 && uis.cursory >= item->top &&
		   uis.cursory < item->bottom;
}

static void VRBindings_Update( void ) {
	int i, n, y;

	n = 0;
	for ( i = 0; i < VRB_ITEMS; i++ ) {
		s_vrb.rows[i].generic.flags |= QMF_HIDDEN | QMF_INACTIVE;
	}
	for ( i = 0; i < VRBM_ROW_COUNT && n < VRB_ITEMS; i++ ) {
		if ( vrbmRows[i].tab != s_vrb.tab ) {
			continue;
		}
		s_vrb.rows[n].generic.id = ID_ROW + i;
		s_vrb.rows[n].generic.flags &= ~( QMF_HIDDEN | QMF_INACTIVE );
		// Headers, and rows this view can't edit, show but are never selected.
		if ( ( vrbmRows[i].flags & VRBM_HEADER ) || !VRBM_Editable( &vrbmRows[i], vrbm.altView ) ) {
			s_vrb.rows[n].generic.flags |= QMF_INACTIVE;
		}
		n++;
	}

	// Rows sit below the "Alt held" switch, which every tab shows.
	y = ( SCREEN_HEIGHT - n * VRB_SPACING ) / 2 + 12;
	// The spin control's click bounds are set once when added, so they move with it.
	s_vrb.alt.generic.y = y - 26;
	s_vrb.alt.generic.top = y - 26;
	s_vrb.alt.generic.bottom = y - 26 + SMALLCHAR_HEIGHT;
	for ( i = 0; i < n; i++, y += VRB_SPACING ) {
		s_vrb.rows[i].generic.x = VRB_X;
		s_vrb.rows[i].generic.y = y;
		s_vrb.rows[i].generic.left = VRB_LEFT;
		s_vrb.rows[i].generic.right = VRB_RIGHT;
		s_vrb.rows[i].generic.top = y;
		s_vrb.rows[i].generic.bottom = y + SMALLCHAR_HEIGHT;
	}

	for ( i = 0; i < VRBM_TABS; i++ ) {
		s_vrb.tabs[i].generic.flags &= ~( QMF_HIGHLIGHT | QMF_HIGHLIGHT_IF_FOCUS );
		s_vrb.tabs[i].generic.flags |= QMF_PULSEIFFOCUS;
	}
	s_vrb.tabs[s_vrb.tab].generic.flags &= ~QMF_PULSEIFFOCUS;
	s_vrb.tabs[s_vrb.tab].generic.flags |= QMF_HIGHLIGHT | QMF_HIGHLIGHT_IF_FOCUS;
}

static void VRBindings_DrawRow( void *self ) {
	menuaction_s *a = (menuaction_s *)self;
	const int index = a->generic.id - ID_ROW;
	const vrbmRow_t *row = &vrbmRows[index];
	const qboolean waiting = ( vrbm.waiting == index );
	qboolean focused;
	char names[128];
	float *color;
	int style;

	VRBM_Tick( uis.realtime );
	if ( row->flags & VRBM_HEADER ) {
		UI_DrawString( VRB_X, a->generic.y, row->label, UI_CENTER | UI_SMALLFONT, color_yellow );
		return;
	}
	if ( !VRBM_Editable( row, vrbm.altView ) ) {
		VRBM_Names( row, 0, names, sizeof( names ) );
		UI_DrawString( VRB_X - SMALLCHAR_WIDTH, a->generic.y, row->label, UI_RIGHT | UI_SMALLFONT, text_color_disabled );
		UI_DrawString( VRB_X + SMALLCHAR_WIDTH, a->generic.y, names, UI_LEFT | UI_SMALLFONT, text_color_disabled );
		return;
	}
	focused = ( Menu_ItemAtCursor( a->generic.parent ) == a );
	color = focused || waiting ? text_color_highlight : text_color_normal;
	if ( focused || waiting ) {
		UI_FillRect( a->generic.left, a->generic.top, a->generic.right - a->generic.left + 1,
					 a->generic.bottom - a->generic.top + 1, listbar_color );
	}
	VRBM_Names( row, vrbm.altView, names, sizeof( names ) );
	UI_DrawString( VRB_X - SMALLCHAR_WIDTH, a->generic.y, row->label, UI_RIGHT | UI_SMALLFONT, color );
	style = UI_LEFT | UI_SMALLFONT;
	if ( waiting ) {
		style |= UI_PULSE;
	}
	UI_DrawString( VRB_X + SMALLCHAR_WIDTH, a->generic.y, waiting ? "..." : names, style, color );
	UI_DrawChar( VRB_GLYPH_X, a->generic.y, 'X', UI_LEFT | UI_SMALLFONT,
				 focused && VRBindings_OnGlyph( &a->generic ) ? color_red : color );
}

static void VRBindings_RowEvent( void *ptr, int event ) {
	menucommon_s *item = (menucommon_s *)ptr;
	const int index = item->id - ID_ROW;

	if ( event != QM_ACTIVATED || VRBM_Waiting() ) {
		return;
	}
	if ( VRBindings_OnGlyph( item ) ) {
		VRBM_Clear( &vrbmRows[index], vrbm.altView );
	} else {
		VRBM_Start( index );
	}
}

static void VRBindings_ResetAction( qboolean result ) {
	if ( result ) {
		trap_Cmd_ExecuteText( EXEC_APPEND, "vr_bindreset\n" );
	}
}

static void VRBindings_ResetDraw( void ) {
	UI_DrawProportionalString( SCREEN_WIDTH / 2, 356 + PROP_HEIGHT * 0, "All VR buttons will get",
							   UI_CENTER | UI_SMALLFONT, color_yellow );
	UI_DrawProportionalString( SCREEN_WIDTH / 2, 356 + PROP_HEIGHT * 1, "this controller's defaults.",
							   UI_CENTER | UI_SMALLFONT, color_yellow );
}

static void VRBindings_Event( void *ptr, int event ) {
	const int id = ( (menucommon_s *)ptr )->id;

	if ( event != QM_ACTIVATED ) {
		return;
	}
	if ( id >= ID_TAB && id < ID_TAB + VRBM_TABS ) {
		s_vrb.tab = id - ID_TAB;
		VRBindings_Update();
	} else if ( id == ID_ALT ) {
		vrbm.altView = s_vrb.alt.curvalue;
		VRBindings_Update();
	} else if ( id == ID_RESET ) {
		UI_ConfirmMenu( "RESET BINDINGS?", VRBindings_ResetDraw, VRBindings_ResetAction );
	} else if ( id == ID_BACK ) {
		VRBM_Cancel();
		UI_PopMenu();
	}
}

static sfxHandle_t VRBindings_Key( int key ) {
	if ( VRBM_Waiting() ) {
		const int vrKey = UI_VR_KeyIndex( key );
		if ( key == K_ESCAPE ) {
			VRBM_Cancel();
			return menu_out_sound;
		}
		if ( vrKey >= 0 ) {
			return VRBM_Capture( vrKey ) == VRBM_REFUSED ? menu_buzz_sound : menu_move_sound;
		}
		VRBM_OtherKey();
		return menu_null_sound;
	}
	if ( key == K_ESCAPE || key == K_MOUSE2 ) {
		VRBM_Cancel();
	}
	return Menu_DefaultKey( &s_vrb.menu, key );
}

static void VRBindings_Draw( void ) {
	char status[256];
	int style;

	Menu_Draw( &s_vrb.menu );
	VRBM_Status( status, sizeof( status ) );
	style = UI_SMALLFONT | UI_CENTER;
	if ( VRBM_Waiting() ) {
		style |= UI_PULSE;
	}
	UI_DrawString( SCREEN_WIDTH / 2, SCREEN_HEIGHT * 0.80, status, style, colorWhite );
}

static void VRBindings_Cache( void ) {
	trap_R_RegisterShaderNoMip( ART_BACK0 );
	trap_R_RegisterShaderNoMip( ART_BACK1 );
	trap_R_RegisterShaderNoMip( ART_FRAMEL );
	trap_R_RegisterShaderNoMip( ART_FRAMER );
	trap_R_RegisterShaderNoMip( ART_RESET0 );
	trap_R_RegisterShaderNoMip( ART_RESET1 );
}

static void VRBindings_MenuInit( void ) {
	int i;

	memset( &s_vrb, 0, sizeof( s_vrb ) );
	VRBM_Cancel();
	VRBindings_Cache();

	s_vrb.menu.key = VRBindings_Key;
	s_vrb.menu.draw = VRBindings_Draw;
	s_vrb.menu.wrapAround = qtrue;
	s_vrb.menu.fullscreen = qtrue;

	s_vrb.banner.generic.type = MTYPE_BTEXT;
	s_vrb.banner.generic.flags = QMF_CENTER_JUSTIFY;
	s_vrb.banner.generic.x = 320;
	s_vrb.banner.generic.y = 16;
	s_vrb.banner.string = "VR BINDINGS";
	s_vrb.banner.color = color_white;
	s_vrb.banner.style = UI_CENTER;

	s_vrb.framel.generic.type = MTYPE_BITMAP;
	s_vrb.framel.generic.name = ART_FRAMEL;
	s_vrb.framel.generic.flags = QMF_LEFT_JUSTIFY | QMF_INACTIVE;
	s_vrb.framel.generic.x = 0;
	s_vrb.framel.generic.y = 78;
	s_vrb.framel.width = 256;
	s_vrb.framel.height = 329;

	s_vrb.framer.generic.type = MTYPE_BITMAP;
	s_vrb.framer.generic.name = ART_FRAMER;
	s_vrb.framer.generic.flags = QMF_LEFT_JUSTIFY | QMF_INACTIVE;
	s_vrb.framer.generic.x = 376;
	s_vrb.framer.generic.y = 76;
	s_vrb.framer.width = 256;
	s_vrb.framer.height = 334;

	for ( i = 0; i < VRBM_TABS; i++ ) {
		s_vrb.tabs[i].generic.type = MTYPE_PTEXT;
		s_vrb.tabs[i].generic.flags = QMF_RIGHT_JUSTIFY | QMF_PULSEIFFOCUS;
		s_vrb.tabs[i].generic.id = ID_TAB + i;
		s_vrb.tabs[i].generic.callback = VRBindings_Event;
		s_vrb.tabs[i].generic.x = 152;
		s_vrb.tabs[i].generic.y = 240 + ( i - 2 ) * PROP_HEIGHT;
		s_vrb.tabs[i].string = (char *)vrbTabNames[i];
		s_vrb.tabs[i].style = UI_RIGHT;
		s_vrb.tabs[i].color = color_red;
	}

	s_vrb.alt.generic.type = MTYPE_SPINCONTROL;
	s_vrb.alt.generic.flags = QMF_PULSEIFFOCUS | QMF_SMALLFONT;
	s_vrb.alt.generic.id = ID_ALT;
	s_vrb.alt.generic.callback = VRBindings_Event;
	s_vrb.alt.generic.name = "Alt held:";
	s_vrb.alt.generic.x = VRB_X;
	s_vrb.alt.itemnames = vrbAltNames;
	s_vrb.alt.curvalue = vrbm.altView;

	for ( i = 0; i < VRB_ITEMS; i++ ) {
		s_vrb.rows[i].generic.type = MTYPE_ACTION;
		s_vrb.rows[i].generic.flags = QMF_LEFT_JUSTIFY | QMF_PULSEIFFOCUS | QMF_HIDDEN | QMF_INACTIVE;
		s_vrb.rows[i].generic.callback = VRBindings_RowEvent;
		s_vrb.rows[i].generic.ownerdraw = VRBindings_DrawRow;
		s_vrb.rows[i].generic.id = ID_ROW;
	}

	s_vrb.back.generic.type = MTYPE_BITMAP;
	s_vrb.back.generic.name = ART_BACK0;
	s_vrb.back.generic.flags = QMF_LEFT_JUSTIFY | QMF_PULSEIFFOCUS;
	s_vrb.back.generic.id = ID_BACK;
	s_vrb.back.generic.callback = VRBindings_Event;
	s_vrb.back.generic.x = 0;
	s_vrb.back.generic.y = 480 - 64;
	s_vrb.back.width = 128;
	s_vrb.back.height = 64;
	s_vrb.back.focuspic = ART_BACK1;

	s_vrb.reset.generic.type = MTYPE_BITMAP;
	s_vrb.reset.generic.name = ART_RESET0;
	s_vrb.reset.generic.flags = QMF_RIGHT_JUSTIFY | QMF_PULSEIFFOCUS;
	s_vrb.reset.generic.id = ID_RESET;
	s_vrb.reset.generic.callback = VRBindings_Event;
	s_vrb.reset.generic.x = 640;
	s_vrb.reset.generic.y = 480 - 64;
	s_vrb.reset.width = 128;
	s_vrb.reset.height = 64;
	s_vrb.reset.focuspic = ART_RESET1;

	Menu_AddItem( &s_vrb.menu, &s_vrb.banner );
	Menu_AddItem( &s_vrb.menu, &s_vrb.framel );
	Menu_AddItem( &s_vrb.menu, &s_vrb.framer );
	for ( i = 0; i < VRBM_TABS; i++ ) {
		Menu_AddItem( &s_vrb.menu, &s_vrb.tabs[i] );
	}
	Menu_AddItem( &s_vrb.menu, &s_vrb.alt );
	for ( i = 0; i < VRB_ITEMS; i++ ) {
		Menu_AddItem( &s_vrb.menu, &s_vrb.rows[i] );
	}
	Menu_AddItem( &s_vrb.menu, &s_vrb.back );
	Menu_AddItem( &s_vrb.menu, &s_vrb.reset );

	VRBindings_Update();
}

void UI_VRBindingsMenu( void ) {
	VRBindings_MenuInit();
	UI_PushMenu( &s_vrb.menu );
}
