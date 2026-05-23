// Trinity-verified player announcement broadcast.
// Server emits a "tann <j|w> <clientNum>" reliable server command at the
// moment an announcement should play. Client (cg_trinity_announce.c)
// resolves the filename and plays the sound.

#include "g_local.h"

void G_TrinityMaybeAnnounceJoin( gentity_t *ent ) {
	if ( !ent || !ent->client ) {
		return;
	}
	// Verification gate: humans on a handshake-enabled server must be
	// verified. Bots can't run the handshake, so they're announced on
	// name alone (their personality names are stable). With handshake
	// disabled, all clients fall back to name-only announcements.
	if ( g_trinityHandshake.integer &&
	     !ent->client->sess.trinityVerified &&
	     !( ent->r.svFlags & SVF_BOT ) ) {
		return;
	}
	if ( ent->client->sess.announcedJoin ) {
		return;
	}
	if ( ent->client->sess.sessionTeam == TEAM_SPECTATOR ) {
		return;
	}
	// In single-player, g_bot.c's PlayerIntroSound() already plays the bot
	// intro via the legacy `play` console command from G_CheckBotSpawn.
	// Skip our broadcast for SP bots to avoid double-announcing.
	if ( g_gametype.integer == GT_SINGLE_PLAYER &&
	     ( ent->r.svFlags & SVF_BOT ) ) {
		return;
	}

	G_BroadcastServerCommand( -1, va("tann j %d", (int)(ent - g_entities)) );
	ent->client->sess.announcedJoin = qtrue;
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
