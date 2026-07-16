// Vendored VR API - shared platform query (see vr_platform.h).
#include "q_shared.h"
#include "vr_platform.h"

// Resolved per link unit; every module compiling this file provides it.
void trap_Cvar_VariableStringBuffer( const char *var_name, char *buffer, int bufsize );

/*
============
VR_Platform

Two-gate rule: vr_platform is believed only when the caller's VR handshake is
live. The flatscreen engine never registers the cvar, so a stale value in a
user config must not impersonate a headset.
============
*/
vrPlatform_t VR_Platform( qboolean vrActive ) {
	char buf[16];

	if ( !vrActive ) {
		return VRP_NONE;
	}
	trap_Cvar_VariableStringBuffer( "vr_platform", buf, sizeof( buf ) );
	if ( !Q_stricmp( buf, "pc" ) ) {
		return VRP_PC;
	}
	if ( !Q_stricmp( buf, "quest" ) ) {
		return VRP_QUEST;
	}
	return VRP_NONE;
}
