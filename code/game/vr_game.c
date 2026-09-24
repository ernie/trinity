// Vendored VR API - game-module implementation (see vr_game.h).
#include "g_local.h"
#include "vr_bg.h"
#include "vr_game.h"

// VR shared-state mirror. The engine's VR-aware QVM ladder scans each
// module's .qvm for this sentinel, so the game module carries its own copy.
const char vr_api_sentinel[] = VR_API_SENTINEL;

vr_shared_t vr_state;
vr_shared_t *vr = &vr_state;
qboolean g_vrActive = qfalse;

#ifdef Q3_VM
void (*trap_VR_RegisterState)( void *state, int stateSize, int apiMajor, int apiMinor );
#else
int dll_trap_VR_RegisterState;
#endif

#include "vr_trap.h" // its VR_RegisterMirror uses the trap pointer above

/*
============
G_VR_Init

Bootstrap the extension interface and register our VR state mirror by name.
The engine syncs it in immediately, so vr-> reads below see live values. If
the engine lacks trap_GetValue the mirror stays zeroed (dormant), the sane
flatscreen-engine mode. Also seeds the module-owned config block
(no_crosshair / local_server / single_player) - writes into the local mirror
are harmless when dormant.
============
*/
void G_VR_Init( void ) {
	char ext[64];
	g_vrActive = qfalse;
	memset( &vr_state, 0, sizeof( vr_state ) );

	// keep the sentinel referenced so the toolchain retains it in the data segment
	if ( vr_api_sentinel[0] != 'T' )
		return;

	trap_Cvar_VariableStringBuffer( "//trap_GetValue", ext, sizeof( ext ) );
	if ( ext[0] ) {
#ifdef Q3_VM
		trap_GetValue = (void*)~atoi( ext );
#else
		dll_com_trapGetValue = atoi( ext );
#endif

		g_vrActive = VR_RegisterMirror( &vr_state );
	}

	// module-owned config block (engine sync-out publishes it after this call)
	{
		char serverinfo[MAX_INFO_STRING];
		trap_GetServerinfo( serverinfo, sizeof( serverinfo ) );
		vr->no_crosshair = (Q_stristr(serverinfo, "nocrosshair") != NULL || Q_stristr(serverinfo, "no crosshair") != NULL);
		vr->local_server = qtrue;
#ifdef MISSIONPACK
		vr->single_player = trap_Cvar_VariableValue("ui_singlePlayerActive");
#else
		vr->single_player = trap_Cvar_VariableValue( "g_gametype" ) == GT_SINGLE_PLAYER;
#endif
	}
}

/*
============
G_VR_Active

Mirror-registered predicate - the module's dormancy signal. Replaces the
retired `vr != NULL` idiom (vr statically points at the local mirror and is
never NULL; registration is the real signal).
============
*/
qboolean G_VR_Active( void ) {
	return g_vrActive;
}

// Zeroed only at module load: every read is EF_VR_PLAYER-gated and connect/spawn clear that flag.
typedef struct {
	qboolean		vrClient;	// `vr 1` in the client's userinfo
	vrHeadOrient_t	head;		// decoded from the head bits
} vrClient_t;

static vrClient_t vr_clients[MAX_CLIENTS];

/*
============
G_VR_ClientThink

Unpack VR head orientation from buttons bits 12-25; the userinfo vr flag gates
the decode, so a flatscreen client's stray bits never mark it as a VR player.
============
*/
void G_VR_ClientThink( struct gclient_s *client, const usercmd_t *ucmd ) {
	vrClient_t *vrc = &vr_clients[client - level.clients];

	if (vrc->vrClient && (ucmd->buttons & 0x03FFF000)) {
		int pitchPacked = (ucmd->buttons >> 12) & 0x7F;
		int yawPacked = (ucmd->buttons >> 19) & 0x7F;

		vrc->head.pitch = (pitchPacked * 180.0f / 127.0f) - 90.0f;
		vrc->head.yawOffset = (yawPacked * 180.0f / 127.0f) - 90.0f;
		client->ps.eFlags |= EF_VR_PLAYER;
	} else {
		client->ps.eFlags &= ~EF_VR_PLAYER;
	}
}

/*
============
G_VR_ClientEndFrame

Copy VR head orientation data to entityState_t for network transmission.
angles2[PITCH] = head pitch, angles2[ROLL] = head yaw offset (repurposed).
Roll is already in the player's viewangles via standard networking. Also
packs the angles into playerState stats for demo playback of the local
player: range [-180, 180] -> [-32768, 32767] (182.04 = 32767/180).
============
*/
void G_VR_ClientEndFrame( struct gclient_s *client, struct gentity_s *ent ) {
	if (client->ps.eFlags & EF_VR_PLAYER) {
		const vrHeadOrient_t *head = &vr_clients[client - level.clients].head;
		ent->s.angles2[PITCH] = head->pitch;
		ent->s.angles2[ROLL] = head->yawOffset;
		client->ps.stats[STAT_VR_HEAD_PITCH] = (short)(head->pitch * 182.04f);
		client->ps.stats[STAT_VR_HEAD_YAW_OFFSET] = (short)(head->yawOffset * 182.04f);
	}
}

/*
============
G_VR_ClientUserinfoChanged

Value-gate for the `vr` userinfo key (presence is NOT the signal - flatscreen
trinity-engine clients send vr\0). Feeds the head-bit decode gate and the
CS_PLAYERS `vr\` field.
============
*/
qboolean G_VR_ClientUserinfoChanged( struct gclient_s *client, const char *userinfo ) {
	vrClient_t *vrc = &vr_clients[client - level.clients];

	vrc->vrClient = atoi( Info_ValueForKey( userinfo, "vr" ) ) == 1;
	return vrc->vrClient;
}

static void rotateAboutOrigin(float x, float y, float rotation, vec2_t out)
{
	out[0] = cos(DEG2RAD(-rotation)) * x  +  sin(DEG2RAD(-rotation)) * y;
	out[1] = cos(DEG2RAD(-rotation)) * y  -  sin(DEG2RAD(-rotation)) * x;
}

static void convertFromVR(gentity_t *ent, vec3_t in, vec3_t offset, vec3_t out)
{
	vec3_t vrSpace;
	vec2_t r;
	float worldscale;
	vec3_t temp;

	VectorSet(vrSpace, in[2], in[0], in[1] );

	rotateAboutOrigin(vrSpace[0], vrSpace[1], ent->client->ps.viewangles[YAW] - vr->hmdorientation[YAW], r);
	vrSpace[0] = -r[0];
	vrSpace[1] = -r[1];

	worldscale = trap_Cvar_VariableValue("vr_worldscale");
	VectorScale(vrSpace, worldscale, temp);

	if (offset) {
		VectorAdd(temp, offset, out);
	} else {
		VectorCopy(temp, out);
	}
}

/*
============
G_VR_AimAngles

6DOF weapon aim: controller angles plus the yaw delta between the server
view and the HMD. qfalse (leave stock viewangles aim) for bots, other
clients, non-6DOF play, and dormant mirrors - the mirror only carries the
one local VR client's pose, so it can't be used for anyone else.
============
*/
qboolean G_VR_AimAngles( struct gentity_s *ent, vec3_t angles ) {
	if ( ( ent->r.svFlags & SVF_BOT ) ||
	    ent->client->ps.clientNum != vr->clientNum ||
	    !vr->use_6dof ) {
		return qfalse;
	}

	VectorCopy(vr->weaponangles, angles);
	angles[YAW] += ent->client->ps.viewangles[YAW] - vr->hmdorientation[YAW];
	return qtrue;
}

/*
============
G_VR_MuzzlePoint

6DOF muzzle from the controller pose. This is trinity's merged muzzle
calculation: it also produces the hitscan trace origin, so the VR-derived
muzzle must feed both outputs. qfalse = caller runs the stock eye-forward
muzzle math.
============
*/
qboolean G_VR_MuzzlePoint( struct gentity_s *ent, const vec3_t forward, const vec3_t right, const vec3_t up, vec3_t origin, vec3_t muzzlePoint ) {
	float worldscale;

	if ( ( ent->r.svFlags & SVF_BOT ) ||
	    ent->client->ps.clientNum != vr->clientNum ||
	    !vr->use_6dof ) {
		return qfalse;
	}

	worldscale = trap_Cvar_VariableValue("vr_worldscale");
	convertFromVR(ent, vr->weaponoffset, ent->r.currentOrigin, muzzlePoint);
	muzzlePoint[2] -= ent->client->ps.viewheight;
	muzzlePoint[2] += vr->hmdposition[1] * worldscale;
	VectorCopy( muzzlePoint, origin );
	return qtrue;
}
