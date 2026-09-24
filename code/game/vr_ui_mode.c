#include "q_shared.h"

void trap_Cvar_VariableStringBuffer( const char *name, char *buffer, int size );
float trap_Cvar_VariableValue( const char *name );
void trap_Cmd_ExecuteText( int when, const char *text );

// a module cannot read a latched value; this choice lives until the restart that applies it reloads the module
static int vrModeChoice = -1;

qboolean UI_VR_CanSwitchMode( void ) {
	char modes[128], *cursor, *token;
	qboolean flat = qfalse, headset = qfalse;

	trap_Cvar_VariableStringBuffer( "vr_availableModes", modes, sizeof( modes ) );
	cursor = modes;
	while ( *(token = COM_Parse( &cursor )) ) {
		if ( !strcmp( token, "flat" ) ) {
			flat = qtrue;
		}
		if ( !strcmp( token, "vr" ) ) {
			headset = qtrue;
		}
	}
	return flat && headset;
}

qboolean UI_VR_ModeChoice( void ) {
	if ( vrModeChoice < 0 ) {
		return trap_Cvar_VariableValue( "vr_enabled" ) != 0;
	}
	return vrModeChoice;
}

void UI_VR_ChooseMode( qboolean vr ) {
	vrModeChoice = vr ? 1 : 0;
	// console set keeps CVAR_LATCH; a module's own cvar set would force the value
	trap_Cmd_ExecuteText( EXEC_APPEND, vr ? "set vr_enabled 1\n" : "set vr_enabled 0\n" );
}

qboolean UI_VR_ModePending( void ) {
	return UI_VR_CanSwitchMode() && UI_VR_ModeChoice() != ( trap_Cvar_VariableValue( "vr_enabled" ) != 0 );
}
