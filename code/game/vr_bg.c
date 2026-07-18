// Vendored VR API - shared game/cgame hooks (see vr_bg.h).
#include "q_shared.h"
#include "bg_public.h"
#include "vr_bg.h"
#include "vr_shared.h"

// VR shared-state mirror, resolved per link unit: vr_cgame.c owns it in both
// cgames (baseq3 and missionpack), vr_game.c in both qagames.
extern vr_shared_t *vr;

/*
============
BG_VR_DeadViewLocked

The stock "dead players get no view changes" gate with the VR exception:
VR players keep looking around while dead.
============
*/
qboolean BG_VR_DeadViewLocked( const playerState_t *ps ) {
	if ( ps->pm_type != PM_SPECTATOR && ps->stats[STAT_HEALTH] <= 0
			&& !( ps->eFlags & EF_VR_PLAYER ) ) {
		return qtrue;
	}
	return qfalse;
}

/*
============
BG_VR_UpdateViewAngles

6DOF: the engine feeds absolute HMD-relative angles, so only YAW keeps the
delta_angles compensation; pitch/roll come raw from the command. Returns qtrue
after applying the full circular clamp for the 6DOF client; qfalse leaves the
stock loop to run (dormant mirror: use_6dof reads 0, so this is qfalse on
flatscreen engines and for every other client).
============
*/
qboolean BG_VR_UpdateViewAngles( playerState_t *ps, const usercmd_t *cmd ) {
	short		temp;
	int			i;

	if ( !( vr->clientNum == ps->clientNum && vr->use_6dof ) ) {
		return qfalse;
	}

	// circularly clamp the angles with deltas
	for (i=0 ; i<3 ; i++) {
		//Client is the VR player with 6DoF enabled
		temp = cmd->angles[i] + (i == YAW ? ps->delta_angles[i] : 0);

		if ( i == PITCH ) {
			// don't let the player look up or down more than 90 degrees
			if ( temp > 16000 ) {
				ps->delta_angles[i] = (16000 - cmd->angles[i]) & 0xFFFF;
				temp = 16000;
			} else if ( temp < -16000 ) {
				ps->delta_angles[i] = (-16000 - cmd->angles[i]) & 0xFFFF;
				temp = -16000;
			}
		}
		ps->viewangles[i] = SHORT2ANGLE(temp);
	}

	return qtrue;
}

/*
============
BG_VR_PmoveFriction / BG_VR_PmoveAccelerate

6DoF (single-player only): the body must track the HMD instantly, so ground
friction and acceleration are overridden while every other movement number
stays identical to the server's. Value transforms over the host's own
movement config - the drop needs no knowledge of how the host stores it.
Recomputed every tick on purpose: use_6dof can toggle mid-session (e.g. the
SP intermission podium), so these must never be cached. Identity transforms
when the mirror is dormant or the ps isn't the VR client.
============
*/
static qboolean BG_VR_Is6DOFClient( const playerState_t *ps ) {
	return ( vr->clientNum == ps->clientNum && vr->use_6dof ) ? qtrue : qfalse;
}

float BG_VR_PmoveFriction( const playerState_t *ps, float stockFriction ) {
	return BG_VR_Is6DOFClient( ps ) ? 10.0f : stockFriction;
}

float BG_VR_PmoveAccelerate( const playerState_t *ps, float stockAccelerate ) {
	return BG_VR_Is6DOFClient( ps ) ? 1000.0f : stockAccelerate;
}

/*
============
BG_VR_DriftIdle

Whether the footsteps idle clause widens for this player: analog sticks and
roomscale drift produce tiny velocities that must not advance the walk cycle.
Keyed on EF_VR_PLAYER so the server, prediction, and every client agree per
player; the flag is never set on flatscreen engines, so stock behavior there
is untouched.
============
*/
qboolean BG_VR_DriftIdle( const playerState_t *ps ) {
	return ( ps->eFlags & EF_VR_PLAYER ) ? qtrue : qfalse;
}

/*
============
BG_VR_HeadToEntityState

Unpack VR head angles from stats for demo playback of the local player.
No-op for non-VR players (EF_VR_PLAYER clear).
============
*/
void BG_VR_HeadToEntityState( const playerState_t *ps, entityState_t *s ) {
	if (ps->eFlags & EF_VR_PLAYER) {
		s->angles2[PITCH] = (float)ps->stats[STAT_VR_HEAD_PITCH] / 182.04f;
		s->angles2[ROLL] = (float)ps->stats[STAT_VR_HEAD_YAW_OFFSET] / 182.04f;
	}
}
