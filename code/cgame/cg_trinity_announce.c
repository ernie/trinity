// Client-side Trinity announcement playback.
// Receives "tann <j|w> <clientNum>" server command, derives the .wav path
// per Q3 case quirk, and plays on CHAN_ANNOUNCER. Silent fallback on
// missing files.

#include "cg_local.h"

static qboolean HasPathTraversal( const char *name ) {
	const char *p;
	for ( p = name; *p; p++ ) {
		if ( *p == '/' || *p == '\\' || *p == ':' ) {
			return qtrue;
		}
		if ( p[0] == '.' && p[1] == '.' ) {
			return qtrue;
		}
	}
	return qfalse;
}

void CG_TrinityAnnounce_Play( char subtype, int clientNum ) {
	clientInfo_t	*ci;
	char			clean[MAX_QPATH];
	char			lower[MAX_QPATH];
	char			path[MAX_QPATH];
	sfxHandle_t		h;

	if ( cg_trinityAnnounce.integer == 0 ) {
		return;
	}

	if ( clientNum < 0 || clientNum >= MAX_CLIENTS ) {
		return;
	}

	ci = &cgs.clientinfo[clientNum];
	if ( !ci->infoValid ) {
		return;
	}

	Q_strncpyz( clean, ci->name, sizeof( clean ) );
	Q_CleanStr( clean );

	if ( clean[0] == '\0' ) {
		return;
	}

	if ( HasPathTraversal( clean ) ) {
		return;
	}

	switch ( subtype ) {
		case 'j':
			Com_sprintf( path, sizeof( path ),
				"sound/player/announce/%s.wav", clean );
			break;

		case 'w':
			Q_strncpyz( lower, clean, sizeof( lower ) );
			Q_strlwr( lower );
			Com_sprintf( path, sizeof( path ),
				"sound/player/announce/%s_wins.wav", lower );
			break;

		default:
			return;
	}

	h = trap_S_RegisterSound( path, qfalse );
	if ( !h ) {
		return;
	}

	trap_S_StartLocalSound( h, CHAN_ANNOUNCER );
}
