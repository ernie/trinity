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

	// Replace interior whitespace with underscores so multi-word names
	// resolve to a filename a pk3 author can ship: "Nil Class" looks up
	// "Nil_Class.wav" (and "nil_class_wins.wav" for the win subtype).
	{
		char *p;
		for ( p = clean; *p; p++ ) {
			if ( *p == ' ' || *p == '\t' ) {
				*p = '_';
			}
		}
	}

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

	// Enqueue. CG_TrinityAnnounce_Tick drains the queue with pacing that
	// defers to the reward stack (Excellent/Impressive/etc.) and the
	// buffered-sound queue (CTF/score/lead callouts).
	{
		int next = ( cg.trinityAnnounceIn + 1 ) % MAX_TRINITY_ANNOUNCE_QUEUE;
		if ( next == cg.trinityAnnounceOut ) {
			// Buffer full: drop the oldest, keep the newest (matches the
			// existing soundBuffer drop policy in CG_AddBufferedSound).
			cg.trinityAnnounceOut = ( cg.trinityAnnounceOut + 1 ) % MAX_TRINITY_ANNOUNCE_QUEUE;
		}
		cg.trinityAnnounceQueue[cg.trinityAnnounceIn] = h;
		cg.trinityAnnounceIn = next;
	}
}

// How long to hold the channel between consecutive Trinity announcements.
// Larger than CG_PlayBufferedSounds's 750ms gate because name .wav files
// typically run 1-1.5s; a 2s spacing keeps the tail of one announcement
// from being clipped by the next one starting.
#define TRINITY_ANNOUNCE_SPACING_MS	2000

// Called once per frame from CG_DrawActiveFrame (next to CG_PlayBufferedSounds).
void CG_TrinityAnnounce_Tick( void ) {
	sfxHandle_t h;

	if ( cg.trinityAnnounceIn == cg.trinityAnnounceOut ) {
		return;  // queue empty
	}

	// Pace our own consecutive plays.
	if ( cg.time - cg.trinityAnnounceTime < TRINITY_ANNOUNCE_SPACING_MS ) {
		return;
	}

	// Defer to the reward stack (medal sounds run ~1.5s, plus 3s visual fade).
	if ( cg.rewardStack > 0 || cg.time - cg.rewardTime < REWARD_TIME ) {
		return;
	}

	// Defer to the buffered-sound queue (CTF/score/lead callouts) — both
	// any pending entries and the 750ms recovery window after a recent play.
	if ( cg.soundBufferIn != cg.soundBufferOut ) {
		return;
	}
	if ( cg.time < cg.soundTime ) {
		return;
	}

	h = cg.trinityAnnounceQueue[cg.trinityAnnounceOut];
	cg.trinityAnnounceOut = ( cg.trinityAnnounceOut + 1 ) % MAX_TRINITY_ANNOUNCE_QUEUE;

	trap_S_StartLocalSound( h, CHAN_ANNOUNCER );
	cg.trinityAnnounceTime = cg.time;

	// Bump soundTime so the buffered-sound queue defers to us for the
	// expected duration of this announcement. Bidirectional defer keeps
	// flag/score callouts from clipping our tail.
	cg.soundTime = cg.time + TRINITY_ANNOUNCE_SPACING_MS;
}
