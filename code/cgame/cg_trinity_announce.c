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

// Remove "[VR]" tags (case-insensitive) from anywhere in the name, then
// trim leading/trailing whitespace. q3vr's default name is
// "[VR] Player #NNNN", and players often add the tag manually before or
// after their chosen name. The announcement should match the player's
// underlying identity, not the VR decoration.
static void StripVRTag( char *s ) {
	char	*p;
	int		n;

	for ( p = s; *p; p++ ) {
		if ( p[0] == '[' &&
		     ( p[1] == 'v' || p[1] == 'V' ) &&
		     ( p[2] == 'r' || p[2] == 'R' ) &&
		     p[3] == ']' ) {
			memmove( p, p + 4, strlen( p + 4 ) + 1 );
			p--;  // re-scan from the same position
		}
	}

	// Trim leading whitespace
	p = s;
	while ( *p == ' ' || *p == '\t' ) {
		p++;
	}
	if ( p != s ) {
		memmove( s, p, strlen( p ) + 1 );
	}

	// Trim trailing whitespace
	for ( n = (int)strlen( s ) - 1; n >= 0 && ( s[n] == ' ' || s[n] == '\t' ); n-- ) {
		s[n] = '\0';
	}
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
	StripVRTag( clean );

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
