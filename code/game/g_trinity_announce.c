// Trinity-verified player announcement broadcast.
// Server emits a "tann <j|w> <clientNum>" reliable server command at the
// moment an announcement should play. Client (cg_trinity_announce.c)
// resolves the filename and plays the sound.

#include "g_local.h"

void G_TrinityMaybeAnnounceJoin( gentity_t *ent ) {
	if ( !ent || !ent->client ) {
		return;
	}
	// When handshake is enabled, only verified players announce. When it
	// is disabled server-wide there's no way to verify anyone, so fall
	// back to announcing based on name alone for any non-bot player.
	if ( g_trinityHandshake.integer && !ent->client->sess.trinityVerified ) {
		return;
	}
	if ( ent->client->sess.announcedJoin ) {
		return;
	}
	if ( ent->r.svFlags & SVF_BOT ) {
		return;
	}
	if ( ent->client->sess.sessionTeam == TEAM_SPECTATOR ) {
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
	// Same fallback policy as G_TrinityMaybeAnnounceJoin: verified-only when
	// the server runs handshake, name-only otherwise.
	if ( g_trinityHandshake.integer && !ent->client->sess.trinityVerified ) {
		return;
	}
	if ( ent->r.svFlags & SVF_BOT ) {
		return;
	}

	G_BroadcastServerCommand( -1, va("tann w %d", clientNum) );
}
