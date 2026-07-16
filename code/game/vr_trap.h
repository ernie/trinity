// Mod-side helper for binding the VR integration traps. trap_GetValue is the
// engine's GENERAL extension-discovery interface - it also answers for non-VR
// extensions like voip and trap_R_ProjectDecal - and VR is just one client of
// it. VR_RESOLVE looks a VR trap up by its own name and binds it to the
// module's callable, returning whether the engine answered; VR_HAVE tests an
// already-bound trap. Both take the trap by its C identifier, which is also its
// lookup key. trap_GetValue itself bootstraps by hand: it cannot discover
// itself, and its DLL number lives in dll_com_trapGetValue.
#ifndef __VR_TRAP_H
#define __VR_TRAP_H

#ifdef Q3_VM
#define VR_RESOLVE( trap, ext ) \
	( trap_GetValue( (ext), sizeof( ext ), #trap ) ? ( (trap) = (void *)~atoi( ext ), qtrue ) : qfalse )
#define VR_HAVE( trap )  ( (trap) != NULL )
#else
#define VR_RESOLVE( trap, ext ) \
	( trap_GetValue( (ext), sizeof( ext ), #trap ) ? ( dll_##trap = atoi( ext ), qtrue ) : qfalse )
#define VR_HAVE( trap )  ( dll_##trap != 0 )
#endif

#endif
