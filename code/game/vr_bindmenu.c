#include "vr_bindmenu.h"

#define M VRBM_TAB_MOVE
#define A VRBM_TAB_ACTIONS
#define F VRBM_TAB_FOLLOW
#define S VRBM_TAB_SYSTEM

const vrbmRow_t vrbmRows[VRBM_ROW_COUNT] = {
	{M, "Jump", "+moveup", "gameplay", 0, 0, 0},
	{M, "Crouch", "+movedown", "gameplay", 0, 0, 0},
	{M, "Snap turn left", "turnleft", "gameplay", 0, 0, 0},
	{M, "Snap turn right", "turnright", "gameplay", 0, 0, 0},
	{M, "U-turn", "uturn", "gameplay", 0, 0, 0},
	{A, "Fire", "+attack", "gameplay", 0, 0, 0},
	{A, "Weapon wheel", "+weapon_select", "gameplay", 0, 0, 0},
	{A, "Stabilize weapon", "+weapon_stabilise", "gameplay", 0, 0, 0},
	{A, "Next weapon", "weapnext", "gameplay", 0, 0, 0},
	{A, "Previous weapon", "weapprev", "gameplay", 0, 0, 0},
	{A, "Use item", "+button2", "gameplay", 0, 0, 0},
	{A, "Gesture", "+button3", "gameplay", 0, 0, 0},
	{A, "Scores", "+scores", "gameplay", 0, 0, 0},
	{F, "Next player", "follownext", "follow", 0, 0, 0},
	{F, "Previous player", "followprev", "follow", 0, 0, 0},
	{F, "Stop following", "follow", "follow", 0, 0, 0},
	{F, "Camera", "followcam", "follow", 0, 0, 0},
	{F, "Recenter camera", "followrecenter", "follow", 0, 0, 0},
	{F, "Pause", "demopause", "follow", 0, 0, 0},
	{F, "Scrub", "+tv_scrub", "follow", 0, 0, 0},
	{F, "Cancel scrub", "tv_scrub_cancel", "scrub", 0, 0, 0},
	{S, "Menu", "+key ESCAPE", "global", VRBM_REQUIRED, 0, 0},
	{S, "Alt button", "+alt", "global", VRBM_NOALT, 0, 0},
	{S, "Console", "toggleconsole", "global", 0, 0, 0},
	{S, "Recenter screen", "vr_recenter", "menu", 0, 0, 0},
	{S, "Weapon adjust", "weapon_adjust", "gameplay", 0, 0, 0},
	{S, "In-Game Dialogs", 0, 0, VRBM_HEADER, 0, 0},
	{S, "Yes / Accept", "+vote_yes", "vote", 0, "weapon_adjust", "adjust"},
	{S, "No / Reset", "+vote_no", "vote", 0, "+adjust_reset", "adjust"},
	{S, "Voice", 0, 0, VRBM_HEADER, 0, 0},
	{S, "VoIP PTT / Toggle", "+voiprecord", "gameplay", 0, 0, 0},
	{S, "Voice target", "voiptarget", "gameplay", 0, 0, 0},
};

#undef M
#undef A
#undef F
#undef S

vrbmState_t vrbm = {-1, 0, -1};

#define VRBM_NAMES_MS 500
static struct {
	char text[128];
	int valid;
} names[VRBM_ROW_COUNT][2];
static int namesTime;

// Every layer, global first; the Alt names sit at the same index.
static const char *const layerNames[] = {"global", "menu", "adjust", "scrub", "scoreboard", "vote", "wheel",
										 "follow", "gameplay"};
static const char *const layerAltNames[] = {"global+alt", "menu+alt", "adjust+alt", "scrub+alt", "scoreboard+alt",
											 "vote+alt", "wheel+alt", "follow+alt", "gameplay+alt"};
#define VRBM_LAYERS ( (int)( sizeof( layerNames ) / sizeof( layerNames[0] ) ) )
static vrbmIO_t io;

static int VRBM_Lower( int c ) {
	return c >= 'A' && c <= 'Z' ? c - 'A' + 'a' : c;
}

// Bindings compare as the engine's own lookups do, ignoring case.
static int VRBM_Same( const char *a, const char *b ) {
	while ( *a && VRBM_Lower( *a ) == VRBM_Lower( *b ) ) {
		a++;
		b++;
	}
	return VRBM_Lower( *a ) == VRBM_Lower( *b );
}

static int VRBM_Equals( const char *a, const char *b ) {
	while ( *a && *a == *b ) {
		a++;
		b++;
	}
	return *a == *b;
}

static void VRBM_Append( char *buf, int size, const char *text ) {
	int n = 0;
	while ( n < size - 1 && buf[n] )
		n++;
	while ( n < size - 1 && *text )
		buf[n++] = *text++;
	buf[n] = '\0';
}

void VRBM_SetIO( const vrbmIO_t *table ) {
	io = *table;
}

void VRBM_Forget( void ) {
	int i;
	for ( i = 0; i < VRBM_ROW_COUNT; i++ )
		names[i][0].valid = names[i][1].valid = 0;
}

void VRBM_Tick( int timeMs ) {
	if ( timeMs - namesTime >= VRBM_NAMES_MS || timeMs < namesTime ) {
		namesTime = timeMs;
		VRBM_Forget();
	}
}

static const char *VRBM_Set( const char *context, int alt ) {
	int i;
	for ( i = 0; i < VRBM_LAYERS; i++ )
		if ( VRBM_Equals( context, layerNames[i] ) )
			return alt ? layerAltNames[i] : layerNames[i];
	return context;
}

const char *VRBM_Context( const vrbmRow_t *row, int altView ) {
	return VRBM_Set( row->context, altView );
}

int VRBM_Editable( const vrbmRow_t *row, int altView ) {
	return !(altView && (row->flags & VRBM_NOALT));
}

int VRBM_Keys( const vrbmRow_t *row, int altView, int keys[2] ) {
	char binding[VRBM_BINDING];
	int k, count = 0;
	if ( row->flags & VRBM_HEADER )
		return 0;
	for ( k = 0; k < VRBM_KEYS && count < 2; k++ ) {
		io.read( VRBM_Context( row, altView ), k, binding, sizeof( binding ) );
		if ( binding[0] && VRBM_Same( binding, row->command ) )
			keys[count++] = k;
	}
	return count;
}

// A row's (layer, command) pairs; the count.
static int VRBM_Pairs( const vrbmRow_t *row, const char *contexts[2], const char *commands[2] ) {
	contexts[0] = row->context;
	commands[0] = row->command;
	if ( !row->command2 )
		return 1;
	contexts[1] = row->context2;
	commands[1] = row->command2;
	return 2;
}

// Unbinds key in set when it runs command.
static void VRBM_Drop( const char *set, int key, const char *command ) {
	char binding[VRBM_BINDING];
	io.read( set, key, binding, sizeof( binding ) );
	if ( binding[0] && VRBM_Same( binding, command ) ) {
		io.unbind( set, key );
		VRBM_Forget();
	}
}

// Binds key in the layer's set, clearing what it would shadow or be shadowed by within the same kind of set.
static void VRBM_Place( const char *context, int alt, int key, const char *command ) {
	char binding[VRBM_BINDING];
	int i;
	if ( VRBM_Equals( context, "global" ) ) {
		for ( i = 1; i < VRBM_LAYERS; i++ ) {
			io.read( VRBM_Set( layerNames[i], alt ), key, binding, sizeof( binding ) );
			if ( binding[0] )
				io.unbind( VRBM_Set( layerNames[i], alt ), key );
		}
	} else {
		io.read( VRBM_Set( "global", alt ), key, binding, sizeof( binding ) );
		if ( binding[0] )
			io.unbind( VRBM_Set( "global", alt ), key );
	}
	io.bind( VRBM_Set( context, alt ), key, command );
	VRBM_Forget();
}

vrbmResult_t VRBM_Bind( const vrbmRow_t *row, int altView, int key ) {
	const char *contexts[2], *commands[2];
	char binding[VRBM_BINDING];
	int keys[2], count, pairs, i, j;
	if ( (row->flags & VRBM_HEADER) || key < 0 || key >= VRBM_KEYS )
		return VRBM_UNCHANGED;
	count = VRBM_Keys( row, altView, keys );
	for ( i = 0; i < count; i++ )
		if ( keys[i] == key )
			return VRBM_UNCHANGED;
	pairs = VRBM_Pairs( row, contexts, commands );
	// An Alt binding leaves the plain one working, so only plain bindings are refused.
	if ( !altView ) {
		// Global and menu bindings would take the menu's own clicks and navigation.
		for ( j = 0; j < pairs; j++ ) {
			if ( VRBM_Equals( contexts[j], "global" ) || VRBM_Equals( contexts[j], "menu" ) ) {
				io.read( "menu", key, binding, sizeof( binding ) );
				if ( binding[0] )
					return VRBM_REFUSED;
			}
		}
		io.read( "global", key, binding, sizeof( binding ) );
		if ( VRBM_Same( binding, "+key ESCAPE" ) )
			return VRBM_REFUSED;
	}
	// the new key goes on before the old ones come off, so a required row always has one
	for ( j = 0; j < pairs; j++ )
		VRBM_Place( contexts[j], altView, key, commands[j] );
	if ( count == 2 )
		for ( j = 0; j < pairs; j++ )
			for ( i = 0; i < 2; i++ )
				VRBM_Drop( VRBM_Set( contexts[j], altView ), keys[i], commands[j] );
	return VRBM_BOUND;
}

// A required row goes back to this controller's default keys, which it takes from whatever holds them now.
static void VRBM_Restore( const vrbmRow_t *row ) {
	int defaults[2], count, i, k;
	count = io.defaults ? io.defaults( row->context, row->command, defaults ) : 0;
	if ( count <= 0 )
		return;
	for ( i = 0; i < count; i++ )
		VRBM_Place( row->context, 0, defaults[i], row->command );
	for ( k = 0; k < VRBM_KEYS; k++ )
		if ( k != defaults[0] && !(count > 1 && k == defaults[1]) )
			VRBM_Drop( row->context, k, row->command );
}

void VRBM_Clear( const vrbmRow_t *row, int altView ) {
	const char *contexts[2], *commands[2];
	int pairs, j, k;
	if ( row->flags & VRBM_HEADER )
		return;
	if ( (row->flags & VRBM_REQUIRED) && !altView ) {
		VRBM_Restore( row );
		return;
	}
	pairs = VRBM_Pairs( row, contexts, commands );
	for ( j = 0; j < pairs; j++ )
		for ( k = 0; k < VRBM_KEYS; k++ )
			VRBM_Drop( VRBM_Set( contexts[j], altView ), k, commands[j] );
}

static void VRBM_BuildNames( const vrbmRow_t *row, int altView, char *buf, int size ) {
	char name[64];
	int keys[2], count, i;
	buf[0] = '\0';
	count = VRBM_Keys( row, altView, keys );
	if ( !count ) {
		VRBM_Append( buf, size, "???" );
		return;
	}
	for ( i = 0; i < count; i++ ) {
		if ( i )
			VRBM_Append( buf, size, ", " );
		io.keyName( keys[i], name, sizeof( name ) );
		VRBM_Append( buf, size, name );
	}
}

void VRBM_Names( const vrbmRow_t *row, int altView, char *buf, int size ) {
	const int index = (int)( row - vrbmRows );
	if ( index < 0 || index >= VRBM_ROW_COUNT ) {
		VRBM_BuildNames( row, altView, buf, size );
		return;
	}
	if ( !names[index][altView ? 1 : 0].valid ) {
		VRBM_BuildNames( row, altView, names[index][altView ? 1 : 0].text, sizeof( names[index][altView ? 1 : 0].text ) );
		names[index][altView ? 1 : 0].valid = 1;
	}
	buf[0] = '\0';
	VRBM_Append( buf, size, names[index][altView ? 1 : 0].text );
}

int VRBM_Waiting( void ) {
	return vrbm.waiting >= 0;
}

void VRBM_Start( int row ) {
	if ( row < 0 || row >= VRBM_ROW_COUNT || (vrbmRows[row].flags & VRBM_HEADER) )
		return;
	vrbm.waiting = row;
	vrbm.refused = -1;
	io.capture();
}

void VRBM_Cancel( void ) {
	vrbm.waiting = -1;
	vrbm.refused = -1;
}

vrbmResult_t VRBM_Capture( int key ) {
	vrbmResult_t result;
	if ( vrbm.waiting < 0 )
		return VRBM_UNCHANGED;
	result = VRBM_Bind( &vrbmRows[vrbm.waiting], vrbm.altView, key );
	if ( result == VRBM_REFUSED ) {
		vrbm.refused = key;
		io.capture();
		return result;
	}
	VRBM_Cancel();
	return result;
}

void VRBM_OtherKey( void ) {
	if ( vrbm.waiting >= 0 )
		io.capture();
}

void VRBM_Status( char *buf, int size ) {
	char name[64];
	buf[0] = '\0';
	if ( vrbm.waiting < 0 ) {
		VRBM_Append( buf, size, "Select an action to bind it. X clears it." );
		return;
	}
	if ( vrbm.refused >= 0 ) {
		io.keyName( vrbm.refused, name, sizeof( name ) );
		VRBM_Append( buf, size, name );
		VRBM_Append( buf, size, " is reserved for menu navigation" );
		return;
	}
	VRBM_Append( buf, size, "Press and release an input. " );
	io.cancelName( name, sizeof( name ) );
	VRBM_Append( buf, size, name );
	VRBM_Append( buf, size, " cancels." );
}
