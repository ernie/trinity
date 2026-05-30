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

// Resolve the clan-specific team callout sound/teamplay/voc_<clan>_<verb>.wav,
// or 0 when the announcer is disabled, the team name is empty/unsafe, or the
// per-clan file isn't installed. The clan name is whatever g_redteam/g_blueteam
// hold — "Stroggs"/"Pagans" in missionpack, empty in baseq3 (which short-
// circuits to 0 here, so baseq3 always uses the caller's color fallback).
// Filenames follow the teamplay grammar (see the plan
// ~/.claude/plans/i-d-like-to-find-sharded-thunder.md): a clan is a collective
// plural, so it takes the singular verb — voc_<clan>_win.wav, _score.wav,
// _lead.wav. This resolves ONLY the clan form; callers supply their own generic
// red/blue color fallback, because it differs per callout: the win/score
// generics are voc_ teamplay files, while the lead generic is the base-Q3
// feedback voice already loaded as cgs.media.red/blueLeadsSound (there is no
// voc_*_leads.wav).
static sfxHandle_t CG_TrinityResolveClanSound( int team, const char *verb ) {
	const char	*teamName;
	char		clean[MAX_QPATH];
	char		lower[MAX_QPATH];
	char		path[MAX_QPATH];

	if ( cg_trinityAnnounce.integer == 0 ) {
		return 0;
	}

	if ( team == TEAM_RED ) {
		teamName = cgs.redTeam;
	} else if ( team == TEAM_BLUE ) {
		teamName = cgs.blueTeam;
	} else {
		return 0;
	}

	Q_NormalizeAnnounceName( clean, teamName, sizeof( clean ) );
	Q_strncpyz( lower, clean, sizeof( lower ) );
	Q_strlwr( lower );
	if ( !lower[0] || HasPathTraversal( lower ) ) {
		return 0;
	}

	Com_sprintf( path, sizeof( path ),
		"sound/teamplay/voc_%s_%s.wav", lower, verb );
	return trap_S_RegisterSound( path, qfalse );
}

// End-of-game team-winner callout. Entirely a Trinity feature (base Q3 has
// no team-winner voice), so it's gated wholesale on cg_trinityAnnounce and
// played through the announcer queue. Falls back from the clan file to the
// generic voc_red_wins.wav / voc_blue_wins.wav ("Red"/"Blue" are singular
// nouns, hence the +s). That generic isn't pre-registered anywhere, so we
// build the path directly.
void CG_TrinityAnnounce_PlayTeam( int team ) {
	sfxHandle_t	h;

	if ( cg_trinityAnnounce.integer == 0 ) {
		return;
	}

	h = CG_TrinityResolveClanSound( team, "win" );
	if ( !h ) {
		char path[MAX_QPATH];
		Com_sprintf( path, sizeof( path ),
			"sound/teamplay/voc_%s_wins.wav",
			team == TEAM_RED ? "red" : "blue" );
		h = trap_S_RegisterSound( path, qfalse );
	}
	if ( !h ) {
		return;
	}

	{
		int next = ( cg.trinityAnnounceIn + 1 ) % MAX_TRINITY_ANNOUNCE_QUEUE;
		if ( next == cg.trinityAnnounceOut ) {
			cg.trinityAnnounceOut = ( cg.trinityAnnounceOut + 1 ) % MAX_TRINITY_ANNOUNCE_QUEUE;
		}
		cg.trinityAnnounceQueue[cg.trinityAnnounceIn] = h;
		cg.trinityAnnounceIn = next;
	}
}

// Team scoring callout (cg_event.c, GTS_*TEAM_SCORED). The caller routes the
// result through the buffered-sound queue, keeping the existing CTF/score
// pacing. Falls back to cgs.media.red/blueScoredSound, which is the generic
// voc_red_scores.wav / voc_blue_scores.wav — so base behavior is preserved
// when the announcer is off or no clan file is installed. Returns 0 if the
// fallback handle itself failed to register (non-team game / asset missing).
sfxHandle_t CG_TrinityAnnounce_TeamScoreSound( int team ) {
	sfxHandle_t h = CG_TrinityResolveClanSound( team, "score" );
	if ( h ) {
		return h;
	}
	return ( team == TEAM_RED ) ? cgs.media.redScoredSound : cgs.media.blueScoredSound;
}

// Team-took-the-lead callout (cg_event.c, GTS_*TEAM_TOOK_LEAD). AddTeamScore
// (g_team.c) emits TOOK_LEAD *instead of* SCORED when a score changes who's
// ahead, so this is the lead-changing sibling of the score callout and shares
// the buffered-sound queue. Falls back to cgs.media.red/blueLeadsSound — the
// base-Q3 "Red/Blue leads" voice — so the long-standing lead callout keeps
// working even before any clan voc_<clan>_lead.wav audio exists.
sfxHandle_t CG_TrinityAnnounce_TeamLeadSound( int team ) {
	sfxHandle_t h = CG_TrinityResolveClanSound( team, "lead" );
	if ( h ) {
		return h;
	}
	return ( team == TEAM_RED ) ? cgs.media.redLeadsSound : cgs.media.blueLeadsSound;
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

	Q_NormalizeAnnounceName( clean, ci->name, sizeof( clean ) );

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

// Shortened reward defer during intermission. REWARD_TIME=3000 is the
// visual fade window; the actual medal *audio* runs ~1.5s. During
// intermission the medal display isn't drawing anyway (CG_Draw2D early-
// returns), and the intermission-exit minimum is 5s — so we cap at the
// audio duration to leave room for the team callout + a margin.
#define TRINITY_ANNOUNCE_INTERMISSION_REWARD_MS	1500

// Called once per frame from CG_DrawActiveFrame (next to CG_PlayBufferedSounds).
void CG_TrinityAnnounce_Tick( void ) {
	sfxHandle_t h;

	if ( cg.trinityAnnounceIn == cg.trinityAnnounceOut ) {
		return;  // queue empty
	}

	// Pace our own consecutive plays. trinityAnnounceTime == 0 means "never
	// played" (cg is memset to 0 at init), not "played at time 0" — without
	// this guard, a fresh-server join (level.time < SPACING_MS) would defer
	// the first announcement spuriously. Same convention as CG_FadeColor.
	if ( cg.trinityAnnounceTime > 0 &&
	     cg.time - cg.trinityAnnounceTime < TRINITY_ANNOUNCE_SPACING_MS ) {
		return;
	}

	// Defer to the reward fade window (medal sounds run ~1.5s, plus 3s
	// visual fade). Time-based and drains naturally; keeps the callout
	// from stepping on a last-kill medal in FFA/1v1 and gives a natural
	// beat after intermission triggers. During intermission, cap at the
	// medal *audio* duration so we don't run out the 5s intermission-exit
	// minimum and get clipped by a map change.
	{
		int rewardWindow = ( cg.snap && cg.snap->ps.pm_type == PM_INTERMISSION )
			? TRINITY_ANNOUNCE_INTERMISSION_REWARD_MS
			: REWARD_TIME;
		if ( cg.rewardTime > 0 && cg.time - cg.rewardTime < rewardWindow ) {
			return;
		}
	}
	// Defer to stacked rewards (queued behind a currently-fading one).
	// Only outside intermission: CG_DrawReward is gated behind CG_Draw2D,
	// which early-returns on PM_INTERMISSION, so the stack can't drain
	// once intermission starts and we'd hang forever.
	if ( cg.rewardStack > 0 &&
	     ( !cg.snap || cg.snap->ps.pm_type != PM_INTERMISSION ) ) {
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
