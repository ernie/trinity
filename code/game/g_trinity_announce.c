// Trinity-verified player announcement broadcast.
// Server emits a "tann <j|w> <clientNum>" reliable server command at the
// moment an announcement should play. Client (cg_trinity_announce.c)
// resolves the filename and plays the sound.

#include "g_local.h"

// Eligibility check shared by the queue-on-enter path and the per-frame drain.
// Returns qtrue if this client should be announced (subject to the
// announcedJoin one-shot, which is checked outside).
static qboolean ShouldAnnounceJoin( gentity_t *ent ) {
	if ( !ent || !ent->client ) {
		return qfalse;
	}
	// Verification gate: humans on a handshake-enabled server must be
	// verified. Bots can't run the handshake, so they're announced on
	// name alone. With handshake disabled, everyone falls back to
	// name-only.
	if ( g_trinityHandshake.integer &&
	     !ent->client->sess.trinityVerified &&
	     !( ent->r.svFlags & SVF_BOT ) ) {
		return qfalse;
	}
	if ( ent->client->sess.sessionTeam == TEAM_SPECTATOR ) {
		return qfalse;
	}
	// In single-player, g_bot.c's PlayerIntroSound() already plays the bot
	// intro via the legacy `play` console command from G_CheckBotSpawn.
	// Skip our broadcast for SP bots to avoid double-announcing.
	if ( g_gametype.integer == GT_SINGLE_PLAYER &&
	     ( ent->r.svFlags & SVF_BOT ) ) {
		return qfalse;
	}
	return qtrue;
}

// Queue an announcement. We DON'T emit inline because ClientBegin runs
// during the new client's connection handshake — on a listen server
// (and likely on remote connects too), the local cgame isn't yet
// subscribed to commands at that moment, so an inline broadcast goes
// out to nobody. G_TrinityProcessAnnouncements drains the queue on the
// next G_RunFrame tick, by which time every connected client is fully
// alive and processing reliable commands. This mirrors the existing
// handshake-challenge pattern (see g_client.c:975 and g_main.c).
void G_TrinityMaybeAnnounceJoin( gentity_t *ent ) {
	if ( !ent || !ent->client ) {
		return;
	}
	if ( ent->client->sess.announcedJoin ) {
		return;
	}
	if ( !ShouldAnnounceJoin( ent ) ) {
		return;
	}
	ent->client->sess.pendingAnnounceJoin = qtrue;
}

// Drain the queue. Called from G_RunFrame.
void G_TrinityProcessAnnouncements( void ) {
	int i;
	for ( i = 0; i < level.maxclients; i++ ) {
		gentity_t *ent = &g_entities[i];
		if ( !ent->client ) {
			continue;
		}
		if ( ent->client->pers.connected != CON_CONNECTED ) {
			continue;
		}
		if ( !ent->client->sess.pendingAnnounceJoin ) {
			continue;
		}
		// Clear pending unconditionally; if eligibility lapsed (player
		// moved to spectator, lost verification, etc.) we just drop.
		ent->client->sess.pendingAnnounceJoin = qfalse;
		if ( ent->client->sess.announcedJoin ) {
			continue;
		}
		if ( !ShouldAnnounceJoin( ent ) ) {
			continue;
		}
		G_BroadcastServerCommand( -1, va("tann j %d", i) );
		ent->client->sess.announcedJoin = qtrue;
	}
}

void G_TrinityAnnounceWinner( int clientNum ) {
	gentity_t *ent;

	if ( clientNum < 0 || clientNum >= MAX_CLIENTS ) {
		return;
	}

	ent = &g_entities[clientNum];
	if ( !ent->client ) {
		return;
	}
	if ( ent->client->pers.connected != CON_CONNECTED ) {
		return;
	}
	// Same gate as G_TrinityMaybeAnnounceJoin: humans on a handshake
	// server must be verified; bots are always eligible (no handshake);
	// name-only fallback when handshake is disabled.
	if ( g_trinityHandshake.integer &&
	     !ent->client->sess.trinityVerified &&
	     !( ent->r.svFlags & SVF_BOT ) ) {
		return;
	}

	G_BroadcastServerCommand( -1, va("tann w %d", clientNum) );
}
