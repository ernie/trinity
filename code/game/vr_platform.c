// Vendored VR API - shared platform query (see vr_platform.h).
#include "q_shared.h"
#include "vr_platform.h"

// Resolved per link unit; every module compiling this file provides it.
void trap_Cvar_VariableStringBuffer( const char *var_name, char *buffer, int bufsize );

/*
============
VR_Platform

cl_platform names the device on every build, but it counts as a VR platform
only while the caller's VR handshake is live.
============
*/
vrPlatform_t VR_Platform( qboolean vrActive ) {
	char buf[16];

	if ( !vrActive ) {
		return VRP_NONE;
	}
	trap_Cvar_VariableStringBuffer( "cl_platform", buf, sizeof( buf ) );
	if ( !Q_stricmp( buf, "windows" ) || !Q_stricmp( buf, "macos" ) || !Q_stricmp( buf, "linux" ) ) {
		return VRP_PC;
	}
	if ( !Q_stricmp( buf, "frame" ) || !Q_stricmp( buf, "pico" ) || !Q_stricmp( buf, "quest" ) ) {
		return VRP_STANDALONE;
	}
	return VRP_NONE;
}
