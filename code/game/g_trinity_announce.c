// Trinity-verified player announcement broadcast.
// Server emits a "tann <j|w> <clientNum>" reliable server command at the
// moment an announcement should play. Client (cg_trinity_announce.c)
// resolves the filename and plays the sound.

#include "g_local.h"

void G_TrinityMaybeAnnounceJoin( gentity_t *ent ) {
	if ( !ent || !ent->client ) {
		return;
	}
	if ( !ent->client->sess.trinityVerified ) {
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
	if ( !ent->client->sess.trinityVerified ) {
		return;
	}
	if ( ent->r.svFlags & SVF_BOT ) {
		return;
	}

	G_BroadcastServerCommand( -1, va("tann w %d", clientNum) );
}
