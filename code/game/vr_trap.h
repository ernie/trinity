// Name-based VR trap negotiation shared by the QVM and native modules.
#ifndef __VR_TRAP_H
#define __VR_TRAP_H

#ifdef Q3_VM
#define VR_RESOLVE( trap, ext ) \
	( trap_GetValue( (ext), sizeof( ext ), #trap ) ? ( (trap) = (void *)~atoi( ext ), qtrue ) : qfalse )
#else
#define VR_RESOLVE( trap, ext ) \
	( trap_GetValue( (ext), sizeof( ext ), #trap ) ? ( dll_##trap = atoi( ext ), qtrue ) : qfalse )
#endif

// Registration during INIT establishes VR activity for this module's lifetime.
static qboolean VR_RegisterMirror( vr_shared_t *state ) {
	char ext[64];
	if ( !VR_RESOLVE( trap_VR_RegisterState, ext ) ) {
		return qfalse;
	}
	state->structSize = sizeof( *state );
	state->apiVersion = VR_API_MAJOR;
	trap_VR_RegisterState( state, sizeof( *state ), VR_API_MAJOR, VR_API_MINOR );
	return qtrue;
}

#endif
