// VR API conformance probe: registers the vr_shared_t mirror and renders
// live engine state + a sync round-trip check. Gated by cg_vrApiProbe.
#include "cg_local.h"
#include "../game/vr_bg.h"
#include "../game/vr_shared.h"
#include "../game/vr_trap.h"

const char vr_api_sentinel[] = VR_API_SENTINEL;

vr_shared_t vr_state;
vr_shared_t *vr = &vr_state;
qboolean vrActive = qfalse;
static int probeFrame = 0;
static int lastEchoSent = 0;
static int echoFailures = 0;

#ifdef Q3_VM
void	(*trap_R_BeginPostBloom2D)( void );
void	(*trap_R_EndPostBloom2D)( void );
void	(*trap_R_HUDBufferStart)( qboolean clear );
void	(*trap_R_HUDBufferEnd)( void );
void	(*trap_HapticEvent)( const char *description, int position, int channel, int intensity, float yaw, float height );
#else
int dll_trap_R_BeginPostBloom2D;
int dll_trap_R_EndPostBloom2D;
int dll_trap_R_HUDBufferStart;
int dll_trap_R_HUDBufferEnd;
int dll_trap_HapticEvent;
#endif

void CG_VR_Init( void ) {
	char ext[64];

	// keep the sentinel referenced so the toolchain retains it in the data segment
	if ( vr_api_sentinel[0] != 'T' )
		return;

	trap_Cvar_VariableStringBuffer( "//trap_GetValue", ext, sizeof( ext ) );
	if ( !ext[0] )
		return;		// flatscreen engine: dormant

#ifdef Q3_VM
	trap_GetValue = (void*)~atoi( ext );
#else
	dll_com_trapGetValue = atoi( ext );
#endif

	// trap_VR_RegisterState is the VR handshake: a flatscreen engine may expose
	// trap_GetValue for non-VR extensions but won't answer this, so its absence
	// means "not a VR engine" - stay dormant.
	if ( !VR_RESOLVE( trap_VR_RegisterState, ext ) )
		return;

	vr_state.structSize = sizeof( vr_state );
	vr_state.apiVersion = VR_API_MAJOR;
	trap_VR_RegisterState( &vr_state, sizeof( vr_state ), VR_API_MAJOR, VR_API_MINOR );
	vrActive = qtrue;

	// The rest of the VR trap set is part of the v1 contract, so a registered
	// engine provides all of it - bind unconditionally.
	VR_RESOLVE( trap_R_BeginPostBloom2D, ext );
	VR_RESOLVE( trap_R_EndPostBloom2D, ext );
	VR_RESOLVE( trap_R_HUDBufferStart, ext );
	VR_RESOLVE( trap_R_HUDBufferEnd, ext );
	VR_RESOLVE( trap_HapticEvent, ext );

	if ( vrActive ) {
		char buf[16];

#ifdef MISSIONPACK
		trap_Cvar_VariableStringBuffer( "ui_singlePlayerActive", buf, sizeof( buf ) );
		vr->single_player = ( atof( buf ) != 0.0f );
#else
		trap_Cvar_VariableStringBuffer( "g_gametype", buf, sizeof( buf ) );
		vr->single_player = ( atof( buf ) == GT_SINGLE_PLAYER );
#endif

		// engine recomputes use_6dof per input frame; this write seeds the local mirror only
		trap_Cvar_VariableStringBuffer( "vr_6dof", buf, sizeof( buf ) );
		vr->use_6dof = vr->single_player && ( atof( buf ) != 0.0f );
	}
}

void CG_VR_Frame( void ) {
	if ( vrActive && vr->scoreboardCursorActive ) {
		cgs.cursorX = vr->scoreboardCursorX;
		cgs.cursorY = vr->scoreboardCursorY;
	}

	if ( vrActive ) {
		char buf[32];

		trap_Cvar_VariableStringBuffer( "vr_worldscale", buf, sizeof( buf ) );
		cg.worldscale = atof( buf );
	}

	if ( vrActive ) {
		char buf[32];
		float hudDrawStatus;
		float hudDepth;

		trap_Cvar_VariableStringBuffer( "vr_hudDrawStatus", buf, sizeof( buf ) );
		hudDrawStatus = atof( buf );
		trap_Cvar_VariableStringBuffer( "vr_hudDepth", buf, sizeof( buf ) );
		hudDepth = atof( buf );

		//HACK!! - should change this to a renderer function call
		//Indicate to renderer whether we are in deathcam mode, We don't want sky in death cam mode
		trap_Cvar_Set( "vr_thirdPersonSpectator", (CG_VR_IsDeathCam() ||
	                                             cg.demoPlayback ||
	                                             CG_VR_IsThirdPersonFollow(VRFM_QUERY) ? "1" : "0" ));
		// SP intermission: ALWAYS force mode 1 for world-locked podium HUD
		// This must come FIRST so user can always see and interact with UI to proceed
		if (cg.snap && cg.snap->ps.pm_type == PM_INTERMISSION &&
		    cgs.gametype == GT_SINGLE_PLAYER) {
			trap_Cvar_Set( "vr_currentHudDrawStatus", "1" );
		} else if (hudDrawStatus == 0) {
			// If user disabled HUD, respect that in all modes (except SP intermission above)
			trap_Cvar_Set( "vr_currentHudDrawStatus", "0" );
		} else if (vr->first_person_following) {
			// draw mode 1 won't work with virtual screen at the moment
			trap_Cvar_Set( "vr_currentHudDrawStatus", "2" );
		} else if (CG_VR_IsDeathCam() ||
			((cg.snap && (cg.snap->ps.pm_flags & PMF_FOLLOW) || cg.demoPlayback) && (!vr->first_person_following)))
		{
			trap_Cvar_Set( "vr_currentHudDrawStatus", "1" );
			trap_Cvar_Set( "vr_currentHudDepth", "1" );
		} else {
			trap_Cvar_Set( "vr_currentHudDrawStatus", va( "%i", (int)hudDrawStatus ) );
			trap_Cvar_Set( "vr_currentHudDepth", va( "%i", (int)hudDepth ) );
		}
	}
}

void CG_VRHaptic( const char *description, int position, int channel, int intensity, float yaw, float height ) {
	if ( !vrActive )
		return;
	trap_HapticEvent( description, position, channel, intensity, yaw, height );
}

static void ProbeLine( int *y, const char *text ) {
	CG_DrawStringExt( 8, *y, text, colorWhite, qtrue, qfalse, 6, 10, 0 );
	*y += 10;
}

void CG_VRProbe_Draw( void ) {
	char buf[128];
	int y = 40;

	if ( !cg_vrApiProbe.integer )
		return;

	probeFrame++;

	Com_sprintf( buf, sizeof( buf ), "VR API %s  frame %d", vrActive ? "ACTIVE" : "ABSENT", probeFrame );
	ProbeLine( &y, buf );
	if ( !vrActive )
		return;

	// sync round-trip: engine must reflect last frame's probeEcho into
	// probeEchoBack at the next sync-in
	if ( lastEchoSent && vr_state.probeEchoBack != lastEchoSent )
		echoFailures++;
	Com_sprintf( buf, sizeof( buf ), "echo sent %d back %d failures %d  [%s]",
		lastEchoSent, vr_state.probeEchoBack, echoFailures, echoFailures ? "FAIL" : "PASS" );
	ProbeLine( &y, buf );
	vr_state.probeEcho = probeFrame;
	lastEchoSent = probeFrame;

	Com_sprintf( buf, sizeof( buf ), "hmd pos %.2f %.2f %.2f  orient %.1f %.1f %.1f",
		vr_state.hmdposition[0], vr_state.hmdposition[1], vr_state.hmdposition[2],
		vr_state.hmdorientation[0], vr_state.hmdorientation[1], vr_state.hmdorientation[2] );
	ProbeLine( &y, buf );
	Com_sprintf( buf, sizeof( buf ), "weapon ang %.1f %.1f %.1f  pos %.2f %.2f %.2f",
		vr_state.weaponangles[0], vr_state.weaponangles[1], vr_state.weaponangles[2],
		vr_state.weaponposition[0], vr_state.weaponposition[1], vr_state.weaponposition[2] );
	ProbeLine( &y, buf );
	Com_sprintf( buf, sizeof( buf ), "offhand ang %.1f %.1f %.1f",
		vr_state.offhandangles[0], vr_state.offhandangles[1], vr_state.offhandangles[2] );
	ProbeLine( &y, buf );
	Com_sprintf( buf, sizeof( buf ), "fov %.1f x %.1f  eyeL %.3f/%.3f eyeR %.3f/%.3f",
		vr_state.fov_x, vr_state.fov_y,
		vr_state.eye_fov_angle_left[0], vr_state.eye_fov_angle_right[0],
		vr_state.eye_fov_angle_left[1], vr_state.eye_fov_angle_right[1] );
	ProbeLine( &y, buf );
	Com_sprintf( buf, sizeof( buf ), "sticks L %.2f %.2f  R %.2f %.2f",
		vr_state.thumbstick_location[0][0], vr_state.thumbstick_location[0][1],
		vr_state.thumbstick_location[1][0], vr_state.thumbstick_location[1][1] );
	ProbeLine( &y, buf );
	Com_sprintf( buf, sizeof( buf ), "flags vs=%d fpf=%d 6dof=%d rh=%d fm=%d stab=%d zoom=%d",
		vr_state.virtual_screen, vr_state.first_person_following, vr_state.use_6dof,
		vr_state.right_handed, vr_state.follow_mode, vr_state.weapon_stabilised,
		vr_state.weapon_zoomed );
	ProbeLine( &y, buf );
	Com_sprintf( buf, sizeof( buf ), "cursor menu %d,%d sb %d,%d  client %d  cvyd %.2f",
		vr_state.menuCursorX, vr_state.menuCursorY,
		vr_state.scoreboardCursorX, vr_state.scoreboardCursorY,
		vr_state.clientNum, vr_state.clientview_yaw_delta );
	ProbeLine( &y, buf );
}

void CG_VR_RegisterMedia( void ) {
	if ( vrActive ) {
		int i;

		for ( i = 1 ; i < WP_NUM_WEAPONS ; i++ ) {
			if ( i == WP_GRAPPLING_HOOK ) {
				continue;
			}
			CG_RegisterWeapon( i );
		}

		cgs.media.reticleShader = trap_R_RegisterShader( "gfx/weapon/scope" );

		//HUD
		cgs.media.hudShader = trap_R_RegisterShader( "sprites/vr/hud" );
	}
}

void CG_VR_Shutdown( void ) {
	if ( vrActive ) {
		// an active scrub must fully unwind here: restore menuYaw and release
		// the lock, or the engine keeps suppressing input in the next session
		if ( cgs.tvScrubActive ) {
			vr->menuYawLocked = qfalse;
			vr->menuYaw = cgs.tvScrubSavedMenuYaw;
		}
		vr->scoreboardCursorActive = qfalse;
	}
}

// cg_view.c per-frame view-pipeline hooks: FOV override, forced third-person
// camera modes, view-kick suppression, and damage view-punch scale. Each is
// a verbatim gate-for-hook swap at its original call site.

/*
============
CG_VR_Fov

Engine-supplied per-eye FOV override. No-op when the mirror is dormant.
NOTE (spec C2, pinned): this override runs AFTER the underwater fov warp
and intentionally discards it - pre-existing behavior, do not "fix".
============
*/
void CG_VR_Fov( float *fov_x, float *fov_y ) {
	if ( !vrActive ) {
		return;
	}
	// VR: engine-supplied per-eye FOV. In virtual screen mode use
	// symmetric FOV: the 4:3 virtual screen crops content to 4:3 by
	// calculating height = 3/4 width; otherwise content stretches.
	*fov_x = vr->fov_x;
	if ( vr->virtual_screen ) {
		*fov_y = vr->fov_x;
	} else {
		*fov_y = vr->fov_y;
	}
}

/*
============
CG_VR_ForceThirdPerson

qtrue when a VR camera mode requires third-person rendering (spectator,
demo playback outside first-person follow, or an active follow-camera mode).
============
*/
qboolean CG_VR_ForceThirdPerson( void ) {
	return vrActive && ( cg.predictedPlayerState.pm_type == PM_SPECTATOR ||
			(cg.demoPlayback && !vr->first_person_following) || CG_VR_IsThirdPersonFollow(VRFM_QUERY) );
}

/*
============
CG_VR_OwnsViewKick

VR owns view-kick presentation: kick angles/origin fight the HMD-tracked
camera, so their application is suppressed.
============
*/
qboolean CG_VR_OwnsViewKick( void ) {
	return vrActive;
}

/*
============
CG_VR_DamageRollScale

VR: scale damage view-punch by the engine's comfort cvar (default 0 =
suppressed); flatscreen keeps full punch via coefficient 1.
============
*/
float CG_VR_DamageRollScale( void ) {
	float hitRollCoeff;

	hitRollCoeff = 1.0f;
	if ( vrActive ) {
		char rollBuf[16];
		trap_Cvar_VariableStringBuffer( "vr_rollWhenHit", rollBuf, sizeof( rollBuf ) );
		hitRollCoeff = atof( rollBuf );
	}
	return hitRollCoeff;
}

/*
===============
CG_VR_IsThirdPersonFollow

Returns qtrue when following a player in the given VR follow mode.
VRFM_QUERY matches either third-person mode (spectator/demo UI query).
===============
*/
qboolean CG_VR_IsThirdPersonFollow( VR_FollowMode followMode )
{
	qboolean isFollowing = (cg.snap->ps.pm_flags & PMF_FOLLOW) || cg.demoPlayback || cgs.tvPlayback;

	if (followMode == VRFM_QUERY)
	{
		return isFollowing &&
			   (vr->follow_mode == VRFM_THIRDPERSON_1 || vr->follow_mode == VRFM_THIRDPERSON_2);
	}

	return 	isFollowing && vr->follow_mode == followMode;
}

/*
===============
CG_VR_IsDeathCam

Returns qtrue while the local player is dead and not at intermission.
===============
*/
qboolean CG_VR_IsDeathCam( void )
{
	return 	(( cg.predictedPlayerState.stats[STAT_HEALTH] <= 0 ) &&
               ( cg.predictedPlayerState.pm_type != PM_INTERMISSION ));
}

// VR view-pipeline camera statics: CG_CalcViewValues's hmdYaw and the
// follow/intermission helpers it called. hmdYaw moves from function-local
// static to file-static (see CG_VR_OffsetView below for the write site).
static float vr_hmdYaw = 0;

/*
===============
CG_InitSmoothFollow

Initialize or recenter smooth follow camera behind the player
===============
*/
static void CG_InitSmoothFollow( void ) {
	cg.smoothFollow_distance = 200.0f;
	cg.smoothFollow_distanceTarget = 200.0f;
	cg.smoothFollow_yaw = cg.snap->ps.viewangles[YAW] + 180.0f;
	cg.smoothFollow_pitch = 0.0f;
	cg.smoothFollow_hmdYawOffset = vr->hmdorientation[YAW];
	cg.smoothFollow_initialized = qtrue;
	cg.followLastClientNum = cg.snap->ps.clientNum;
}

/*
===============
CG_OffsetVRThirdPersonView

===============
*/
static void CG_OffsetVRThirdPersonView( void ) {
	float scale = 1.0f;
	vec3_t position;

	cg.refdef.vieworg[2] += cg.predictedPlayerState.viewheight;

	// Detect when we switch to a different followed player (any follow mode, not death cam)
	if (CG_VR_IsThirdPersonFollow(VRFM_QUERY))
	{
		int currentClient = cg.snap->ps.clientNum;
		if (cg.followLastClientNum != currentClient)
		{
			vr->recenter_follow_camera = qtrue;
		}
		cg.followLastClientNum = currentClient;
	}

	//Follow mode 1
 	if ( CG_VR_IsThirdPersonFollow(VRFM_THIRDPERSON_1) )
	{
		scale *= SPECTATOR_WORLDSCALE_MULTIPLIER;

		if (cg_smoothFollow.integer)
		{
			int primaryThumb;
			float yawInput, pitchInput;
			int secondaryThumb;
			float distanceInput;
			int ft;
			float dt;
			vec3_t orbitAngles, forward, right, up;
			vec3_t playerEye;
			vec3_t camPos;
			vec3_t lookDir;

			// Handle camera recentering when B button is pressed or followed player changes
			if (vr->recenter_follow_camera)
			{
				CG_InitSmoothFollow();
				vr->recenter_follow_camera = qfalse;
			}

			// Initialize on first use
			if (!cg.smoothFollow_initialized) {
				CG_InitSmoothFollow();
			}

			// Primary thumbstick: controls camera rotation and height
			primaryThumb = vr->right_handed ? THUMB_RIGHT : THUMB_LEFT;
			yawInput = vr->virtual_screen ? 0.0f : vr->thumbstick_location[primaryThumb][0];
			pitchInput = vr->virtual_screen ? 0.0f : vr->thumbstick_location[primaryThumb][1];

			// Secondary thumbstick: controls camera distance
			secondaryThumb = vr->right_handed ? THUMB_LEFT : THUMB_RIGHT;
			distanceInput = vr->virtual_screen ? 0.0f : vr->thumbstick_location[secondaryThumb][1];

			// Framerate-independent input scaling
			// Use real time when paused so camera still works at timescale 0
			{
				static int lastRealTime;
				int realTime = trap_Milliseconds();

				if ( cg.frametime > 0 ) {
					ft = cg.frametime;
				} else {
					ft = realTime - lastRealTime;
					if ( ft < 0 || ft > 100 ) ft = 16;
				}
				lastRealTime = realTime;
			}
			dt = ft / 1000.0f;

			// Yaw: rotate around player (left/right on primary stick)
			cg.smoothFollow_yaw += yawInput * 180.0f * dt;

			// Pitch: move up/down on sphere surface (up/down on primary stick)
			cg.smoothFollow_pitch += pitchInput * 120.0f * dt;
			// FIXME: It would be great to allow near-vertical rotation, but
			// doing so makes it obvious that moving forward/backward with the
			// HMD isn't getting closer to the player, but moving along the game's
			// Y axis. Which is kinda nauseating, no matter how good your VR legs are.
			if (cg.smoothFollow_pitch > 45.0f) cg.smoothFollow_pitch = 45.0f;
			if (cg.smoothFollow_pitch < 0.0f) cg.smoothFollow_pitch = 0.0f;

			// Distance: adjust target (up = closer, down = farther)
			cg.smoothFollow_distanceTarget -= distanceInput * 120.0f * dt;
			if (cg.smoothFollow_distanceTarget < 40.0f) cg.smoothFollow_distanceTarget = 40.0f;
			if (cg.smoothFollow_distanceTarget > 512.0f) cg.smoothFollow_distanceTarget = 512.0f;

			// Compute camera direction
			// Negate pitch because AngleVectors uses forward[2] = -sin(pitch),
			// but our pitch convention is positive = camera above player
			VectorSet(orbitAngles, -cg.smoothFollow_pitch, cg.smoothFollow_yaw, 0.0f);
			AngleVectors(orbitAngles, forward, right, up);

			VectorCopy(cg.refdef.vieworg, playerEye);

			// Trace at the target distance to find max safe distance
			{
				vec3_t testPos;
				trace_t trace;
				static vec3_t mins = { -4, -4, -4 };
				static vec3_t maxs = { 4, 4, 4 };
				float safeDistance;
				float effectiveTarget;
				float factor;

				VectorMA(playerEye, cg.smoothFollow_distanceTarget, forward, testPos);
				CG_Trace(&trace, playerEye, mins, maxs, testPos,
					cg.predictedPlayerState.clientNum, MASK_SOLID);

				safeDistance = trace.fraction * cg.smoothFollow_distanceTarget;
				effectiveTarget = (safeDistance < cg.smoothFollow_distanceTarget)
					? safeDistance : cg.smoothFollow_distanceTarget;

				// Lerp distance - faster when pulling in for walls
				if (cg.smoothFollow_distance > effectiveTarget) {
					factor = 16.0f * dt;  // pull in quickly to avoid clipping
				} else {
					factor = 8.0f * dt;   // push out gently
				}
				if (factor > 1.0f) factor = 1.0f;
				cg.smoothFollow_distance += (effectiveTarget - cg.smoothFollow_distance) * factor;
			}

			// Final camera position at the lerped distance
			VectorMA(playerEye, cg.smoothFollow_distance, forward, camPos);

			VectorCopy(camPos, cg.vr_vieworigin);

			// Point camera at player
			VectorSubtract(playerEye, cg.vr_vieworigin, lookDir);
			vectoangles(lookDir, vr->clientviewangles);
			vr->clientviewangles[YAW] -= cg.smoothFollow_hmdYawOffset;
		}
		else
		{
			// Handle camera recentering when B button is pressed or followed player changes
			qboolean needsRecenter = qfalse;
			vec3_t current;

			if (vr->recenter_follow_camera)
			{
				needsRecenter = qtrue;
				vr->recenter_follow_camera = qfalse;
			}

			//Check to see if the followed player has moved far enough away to mean we should update our location
			VectorSubtract(cg.refdef.vieworg, cg.vr_vieworigin, current);
			current[2] = 0;
			if (needsRecenter || VectorLength(current) > 400)
			{
				vec3_t angles;
				vec3_t		forward, right, up;
				vec3_t lookDir, targetAngles;
				float currentWorldYaw;
				float snapTurn;

				VectorCopy(cg.refdef.vieworg, cg.vr_vieworigin);

				//Move behind the player
				VectorCopy(cg.snap->ps.viewangles, angles);
				angles[PITCH] = 0;
				AngleVectors( angles, forward, right, up );
				VectorMA( cg.vr_vieworigin, -60, forward, cg.vr_vieworigin );

				// Rotate view to face player
				VectorSubtract(cg.refdef.vieworg, cg.vr_vieworigin, lookDir);
				vectoangles(lookDir, targetAngles);

				// Current world yaw
				currentWorldYaw = vr->clientviewangles[YAW] - vr->hmdorientation[YAW];

				// Calculate snap turn amount
				snapTurn = AngleSubtract(currentWorldYaw, targetAngles[YAW]);

				// Request the client to apply this rotation
				vr->snapTurnYaw = snapTurn;
			}
		}
	}
	//Death or follow mode 2
	else if (CG_VR_IsDeathCam() || CG_VR_IsThirdPersonFollow(VRFM_THIRDPERSON_2))
	{
		scale *= SPECTATOR2_WORLDSCALE_MULTIPLIER;

		// Start grace period on first frame of death cam
		if (CG_VR_IsDeathCam() && cg.deathCamTime == -1) {
			cg.deathCamTime = cg.time;
		}

		// Handle camera recentering when B button is pressed or followed player changes
		if (vr->recenter_follow_camera && CG_VR_IsThirdPersonFollow(VRFM_THIRDPERSON_2))
		{
			vec3_t angles;
			vec3_t forward, right, up;
			vec3_t lookDir, targetAngles;
			float currentWorldYaw;
			float snapTurn;

			// Reset camera to followed player's position
			VectorCopy(cg.refdef.vieworg, cg.vr_vieworigin);

			// Move camera behind and slightly above the player
			VectorCopy(cg.snap->ps.viewangles, angles);
			angles[PITCH] = 0;
			AngleVectors(angles, forward, right, up);
			VectorMA(cg.vr_vieworigin, -180, forward, cg.vr_vieworigin);

			// Calculate rotation needed to point at player
			VectorSubtract(cg.refdef.vieworg, cg.vr_vieworigin, lookDir);
			vectoangles(lookDir, targetAngles);

			// Current world yaw
			currentWorldYaw = vr->clientviewangles[YAW] - vr->hmdorientation[YAW];

			// Calculate snap turn amount (like thumbstick snap turn)
			// CL_SnapTurn does: cl.viewangles[YAW] -= dxYaw
			// We want to rotate from currentWorldYaw to targetAngles[YAW]
			snapTurn = AngleSubtract(currentWorldYaw, targetAngles[YAW]);

			// Request the client to apply this rotation
			vr->snapTurnYaw = snapTurn;

			// Clear the flag
			vr->recenter_follow_camera = qfalse;
		}

		if (!vr->virtual_screen && (!CG_VR_IsDeathCam() || cg.time - cg.deathCamTime >= 500))
		{
			//Move camera if the user is pushing thumbstick
			vec3_t angles, forward, right, up;
			float deltaYaw;

			VectorCopy(vr->offhandangles, angles);
			deltaYaw = (cg.demoPlayback || (cg.snap->ps.pm_flags & PMF_FOLLOW)) ? 0.0f : SHORT2ANGLE(cg.predictedPlayerState.delta_angles[YAW]);
			angles[YAW] += deltaYaw + (vr->clientviewangles[YAW] - vr->hmdorientation[YAW]);
			AngleVectors(angles, forward, right, up);
			VectorMA(cg.vr_vieworigin, vr->thumbstick_location[THUMB_LEFT][1] * 5.0f, forward, cg.vr_vieworigin);
			VectorMA(cg.vr_vieworigin, vr->thumbstick_location[THUMB_LEFT][0] * 5.0f, right, cg.vr_vieworigin);
		}
	}

	VectorCopy(vr->hmdposition, position);
	CG_ConvertFromVR(position, NULL, position);
	position[2] -= PLAYER_HEIGHT;
	VectorScale(position, scale, position);
	VectorAdd(cg.vr_vieworigin, position, cg.refdef.vieworg);
}

/*
=================
CG_CalculateSPIntermissionHUD

Calculates HUD sprite position for single-player intermission.
Called during intermission when pm_type == PM_INTERMISSION.
Positions the HUD at the podium location so it's world-locked.
=================
*/
static void CG_CalculateSPIntermissionHUD( void )
{
	char buf[32];
	float podiumDist;
	float podiumDrop;
	vec3_t forward;
	vec3_t podiumOrigin;
	vec3_t toHud;

	if ( cgs.gametype != GT_SINGLE_PLAYER ) {
		return;
	}

	// Get podium placement cvars (defaults from g_main.c)
	trap_Cvar_VariableStringBuffer( "g_podiumDist", buf, sizeof( buf ) );
	podiumDist = atof( buf );
	trap_Cvar_VariableStringBuffer( "g_podiumDrop", buf, sizeof( buf ) );
	podiumDrop = atof( buf );
	if ( podiumDist == 0 ) podiumDist = 80.0f;
	if ( podiumDrop == 0 ) podiumDrop = 70.0f;

	// Calculate podium position from server's intended camera origin
	AngleVectors( cg.snap->ps.viewangles, forward, NULL, NULL );

	VectorMA( cg.snap->ps.origin, podiumDist, forward, podiumOrigin );
	podiumOrigin[2] -= podiumDrop;

	// Position HUD slightly in front of podium, at eye level
	VectorMA( podiumOrigin, -10.0f, forward, vr->sp_intermission_hud_origin );
	vr->sp_intermission_hud_origin[2] += podiumDrop;

	// Fixed radius based on distance from actual camera to HUD
	VectorSubtract( vr->sp_intermission_hud_origin, cg.refdef.vieworg, toHud );
	vr->sp_intermission_hud_radius = VectorLength( toHud ) * 0.8f;
}

/*
============
CG_VR_MenuViewFreeze

qtrue: caller recomputes vrect+fov only, refdef keeps last frame (menu
freeze). Prevents an odd/nauseating situation where both the screen in
the "virtual theater" and the in-game view projected on the screen are
both rotating based on HMD movement.
============
*/
qboolean CG_VR_MenuViewFreeze( void ) {
	// When the player is in a menu, freeze the camera position/angle
	// This prevents odd/nauseating situation where both the screen in
	// the "virtual theater" and in-game view projected on the screen are
	// both rotating based on the HMD movement.
	return vrActive && vr->virtual_screen && !vr->first_person_following;
}

/*
============
CG_VR_IntermissionView

qtrue: VR intermission camera (trace-back + SP HUD + HMD tracking)
written; qfalse: caller runs stock.
============
*/
qboolean CG_VR_IntermissionView( void ) {
	if ( !vrActive ) {
		return qfalse;
	}
	{
		playerState_t *ps = &cg.predictedPlayerState;
		static vec3_t	mins = { -1, -1, -1 };
		static vec3_t	maxs = { 1, 1, 1 };
		trace_t		trace;
		vec3_t forward;
		vec3_t end;
		vec3_t hmdPos;

		VectorCopy( ps->origin, cg.refdef.vieworg );

		// Trace backward to avoid camera clipping into walls
		AngleVectors(ps->viewangles, forward, NULL, NULL);
		VectorMA(ps->origin, -80, forward, end);
		CG_Trace( &trace, ps->origin, mins, maxs, end, cg.predictedPlayerState.clientNum, MASK_SOLID );
		VectorCopy(trace.endpos, cg.refdef.vieworg);

		// Calculate HUD sprite position for SP intermission
		// This must be done AFTER the trace so we know the actual camera position
		CG_CalculateSPIntermissionHUD();

		VectorCopy(vr->hmdorientation, cg.refdefViewAngles);
		cg.refdefViewAngles[YAW] += (ps->viewangles[YAW] - vr_hmdYaw);
		AnglesToAxis( cg.refdefViewAngles, cg.refdef.viewaxis );

		// Apply HMD positional tracking so the view moves with head movement
		// This prevents the world from appearing frozen while HMD moves
		// Note: Don't apply worldscale - intermission uses 1:1 scale, not spectator god-view
		VectorCopy(vr->hmdposition, hmdPos);
		CG_ConvertFromVR(hmdPos, NULL, hmdPos);
		hmdPos[2] -= PLAYER_HEIGHT;
		VectorAdd(cg.refdef.vieworg, hmdPos, cg.refdef.vieworg);
	}
	return qtrue;
}

/*
============
CG_VR_OffsetView

qtrue: VR camera offset stage fully handled (deathcam/follow/
first-person + vr_vieworigin bookkeeping).
============
*/
qboolean CG_VR_OffsetView( void ) {
	if ( !vrActive ) {
		return qfalse;
	}

	vr_hmdYaw = vr->hmdorientation[YAW];

	if ((CG_VR_IsDeathCam() && !vr->first_person_following) || (cg.demoPlayback && !vr->first_person_following) || CG_VR_IsThirdPersonFollow(VRFM_QUERY))
	{
		//If dead, or spectating, view the map from above
		CG_OffsetVRThirdPersonView();
	}
	else
	{
		if (cg.renderingThirdPerson)
		{
// This offset is verry annoying in VR, let's not do it now...
#if 0
			// back away from character
			CG_OffsetThirdPersonView();
#endif
		}
		else
		{
			// offset for local bobbing and kicks
			CG_OffsetFirstPersonView();
		}

		//Reset this in case we die or follow
		VectorCopy(cg.refdef.vieworg, cg.vr_vieworigin);
	}

	return qtrue;
}

/*
============
CG_VR_ComputeWeaponAngles

Fake-6DOF vr->calculated_weaponangles; no-op dormant/6DOF/virtual-screen.
============
*/
void CG_VR_ComputeWeaponAngles( void ) {
	playerState_t *ps;

	if ( !vrActive ) {
		return;
	}

	// NOTE (undisclosed-by-brief glue, same pattern as CG_VR_IntermissionView's
	// ps rebinding): the fork body below reads ps->origin; ps was the enclosing
	// CG_CalcViewValues's local, not in scope here. Binding is identical to
	// what the fork assigned: ps = &cg.predictedPlayerState.
	ps = &cg.predictedPlayerState;

	if (!vr->use_6dof && !vr->virtual_screen)
	{
		vec3_t weaponorigin, weaponangles;
		vec3_t forward, end, dir;
		trace_t trace;

		if (cg.predictedPlayerState.pm_type == PM_SPECTATOR && !(cg.snap->ps.pm_flags & PMF_FOLLOW)) {
			// For free cam spectator, use offhand's angles to compute fly direction (mainly up/down)
  			CG_CalculateVROffHandPosition(weaponorigin, weaponangles);
		} else {
			CG_CalculateVRWeaponPosition(weaponorigin, weaponangles);
		}

		AngleVectors(weaponangles, forward, NULL, NULL);
		VectorMA(weaponorigin, 4096, forward, end);
		CG_Trace(&trace, weaponorigin, NULL, NULL, end, cg.predictedPlayerState.clientNum, MASK_SOLID);

		if (cg_debugWeaponAiming.integer)
		{
			byte colour[4];
			colour[0] = 0xff;
			colour[1] = 0x00;
			colour[2] = 0x00;
			colour[3] = 0x40;

			CG_LaserSight(weaponorigin, trace.endpos, colour, 1.0f);
		}

		if (cg.predictedPlayerState.pm_type == PM_SPECTATOR && (cg.snap->ps.pm_flags & PMF_FOLLOW))
		{
			//If spectating, just take the weapon angles directly
			VectorCopy(weaponangles, vr->calculated_weaponangles);
		}
		else
		{
			vec3_t origin;
			int timeDelta;
			vec3_t forward2, end2;
			trace_t trace2;
			float deltaYaw;

			VectorSubtract(trace.endpos, cg.refdef.vieworg, dir);
			vectoangles(dir, vr->calculated_weaponangles);

			VectorCopy(ps->origin, origin);
			origin[2] += cg.predictedPlayerState.viewheight;
			timeDelta = cg.time - cg.duckTime;
			if ( timeDelta < DUCK_TIME) {
				origin[2] -= cg.duckChange * (DUCK_TIME - timeDelta) / DUCK_TIME;
			}

			AngleVectors(vr->calculated_weaponangles, forward2, NULL, NULL);
			VectorMA(origin, 4096, forward2, end2);

			CG_Trace(&trace2, cg.refdef.vieworg, NULL, NULL, end2, cg.predictedPlayerState.clientNum, MASK_SOLID);

			if (cg_debugWeaponAiming.integer)
			{
				byte colour[4];
				colour[0] = 0x00;
				colour[1] = 0xff;
				colour[2] = 0x00;
				colour[3] = 0x40;

				CG_LaserSight(cg.refdef.vieworg, trace2.endpos, colour, 1.0f);
			}

			//convert to real-world angles - should be very close to real weapon angles
			deltaYaw = SHORT2ANGLE(cg.predictedPlayerState.delta_angles[YAW]);
			vr->calculated_weaponangles[YAW] -= (deltaYaw + (vr->clientviewangles[YAW] - vr->hmdorientation[YAW]));
		}
	}
}

/*
============
CG_VR_ViewAxis

qtrue: cg.refdef.viewaxis set (zoom/follow/fake-6DOF trees); qfalse:
caller runs stock AnglesToAxis.
============
*/
qboolean CG_VR_ViewAxis( void ) {
	if ( !vrActive ) {
		return qfalse;
	}
	// position eye relative to origin
	if (!vr->use_6dof && !vr->virtual_screen)
	{
		if (vr->weapon_zoomed) {
			//If we are zoomed, then we use the refdefViewANgles (which are the weapon angles)
			vec3_t angles;
			VectorCopy(cg.refdefViewAngles, angles);
			angles[ROLL] = vr->hmdorientation[ROLL];
			AnglesToAxis( angles, cg.refdef.viewaxis );
		}
		else if ( (cg.demoPlayback && !vr->first_person_following) || CG_VR_IsThirdPersonFollow(VRFM_QUERY))
		{
			//If we're following someone in third person,
			vec3_t angles;
			VectorCopy(vr->hmdorientation, angles);
			angles[PITCH] += vr->clientviewangles[PITCH];
			angles[YAW] += vr->clientviewangles[YAW];
			AnglesToAxis(angles, cg.refdef.viewaxis);
		}
		else if (!vr->first_person_following)
		{
			//We are using fake 6DoF, so make the appropriate adjustment to the view
			//angles as we send orientation to the server that includes the weapon angles
			vec3_t angles;
			float deltaYaw;
			VectorCopy(vr->hmdorientation, angles);
			deltaYaw = SHORT2ANGLE(cg.predictedPlayerState.delta_angles[YAW]);
			angles[YAW] = deltaYaw + vr->clientviewangles[YAW];
			AnglesToAxis(angles, cg.refdef.viewaxis);
		}
		else
		{
			AnglesToAxis(cg.refdefViewAngles, cg.refdef.viewaxis);
		}
	} else {
		if (vr->weapon_zoomed) {
			vec3_t angles;
			angles[ROLL] = vr->hmdorientation[ROLL];
			angles[PITCH] = vr->weaponangles[PITCH];
			angles[YAW] = (cg.refdefViewAngles[YAW] - vr->hmdorientation[YAW]) + vr->weaponangles[YAW];
			AnglesToAxis(angles, cg.refdef.viewaxis);
		}
		else if ( (cg.demoPlayback && !vr->first_person_following) || CG_VR_IsThirdPersonFollow(VRFM_QUERY))
		{
			//If we're following someone in third person,
			vec3_t angles;
			VectorCopy(vr->hmdorientation, angles);
			angles[PITCH] += vr->clientviewangles[PITCH];
			angles[YAW] += vr->clientviewangles[YAW];
			AnglesToAxis(angles, cg.refdef.viewaxis);
		}
		else
		{
			AnglesToAxis(cg.refdefViewAngles, cg.refdef.viewaxis);
		}
	}
	return qtrue;
}

// VR-protocol head rendering: the follow predicate, predicted head-stat
// interpolation, and the first-person follow head view EMA (cg_predict.c/
// cg_view.c/cg_draw.c call sites). EF-gated or self-gating, NOT
// vrActive-gated: flatscreen hosts render VR players' heads too.

/*
===============
CG_VR_IsVRFollow

Returns qtrue when following a VR player (first or third person).
===============
*/
qboolean CG_VR_IsVRFollow( void ) {
	if ( !cg.demoPlayback && !(cg.snap->ps.pm_flags & PMF_FOLLOW) ) {
		return qfalse;
	}
	if ( !(cg.predictedPlayerState.eFlags & EF_VR_PLAYER) ) {
		return qfalse;
	}
	return qtrue;
}

/*
===============
CG_VR_InterpolateHeadStats

CG_InterpolatePlayerState's head-stat lerp between snapshots.
===============
*/
void CG_VR_InterpolateHeadStats( playerState_t *out, const playerState_t *prev, const playerState_t *next, float f ) {
	// Interpolate VR head angle stats (same treatment as viewangles)
	out->stats[STAT_VR_HEAD_PITCH] = (int)( LerpAngle(
		(float)prev->stats[STAT_VR_HEAD_PITCH] / 182.04f,
		(float)next->stats[STAT_VR_HEAD_PITCH] / 182.04f, f ) * 182.04f );
	out->stats[STAT_VR_HEAD_YAW_OFFSET] = (int)( LerpAngle(
		(float)prev->stats[STAT_VR_HEAD_YAW_OFFSET] / 182.04f,
		(float)next->stats[STAT_VR_HEAD_YAW_OFFSET] / 182.04f, f ) * 182.04f );
}

// VR first-person head view smoothing state (formerly cg.vrView*)
static float	vr_followViewPitch;
static float	vr_followViewYaw;
static qboolean	vr_followViewInitialized;

/*
===============
CG_VR_FollowHeadView

CG_CalcViewValues' first-person-follow head view; ps is the caller's
&cg.predictedPlayerState.
===============
*/
void CG_VR_FollowHeadView( const playerState_t *ps ) {
	// VR first-person follow: look through the player's head, not weapon.
	// EMA smooths the ~1.4 degree steps from 7-bit usercmd packing.
	if ( !cg.renderingThirdPerson
			&& (cg.demoPlayback || (cg.snap->ps.pm_flags & PMF_FOLLOW))
			&& (ps->eFlags & EF_VR_PLAYER) ) {
		float	targetPitch, targetYaw, alpha;

		targetPitch = (float)ps->stats[STAT_VR_HEAD_PITCH] / 182.04f;
		targetYaw   = ps->viewangles[YAW]
			+ (float)ps->stats[STAT_VR_HEAD_YAW_OFFSET] / 182.04f;

		if ( !vr_followViewInitialized || cg.nextFrameTeleport ) {
			vr_followViewPitch = targetPitch;
			vr_followViewYaw   = targetYaw;
			vr_followViewInitialized = qtrue;
		} else {
			// tau ~30ms: smooths quantization steps without perceptible lag
			alpha = (float)cg.frametime / ( (float)cg.frametime + 30.0f );
			vr_followViewPitch += alpha * AngleSubtract( targetPitch, vr_followViewPitch );
			vr_followViewYaw   += alpha * AngleSubtract( targetYaw,   vr_followViewYaw );
		}

		cg.refdefViewAngles[PITCH] = vr_followViewPitch;
		cg.refdefViewAngles[YAW]   = vr_followViewYaw;
	} else {
		vr_followViewInitialized = qfalse;
	}
}

/*
===============
CG_VR_FollowHeadViewReset

Re-seeds the follow head view EMA (tv_seek_sync timeline jumps).
===============
*/
void CG_VR_FollowHeadViewReset( void ) {
	vr_followViewInitialized = qfalse;
}

// cg_event.c EV_USE_ITEM0..15 pre-switch haptic. The local-predicted gate
// stayed at the call site (it wraps the "which event range" check too);
// this is just the haptic that used to fire inside it.
void CG_VR_OnUseItem( void ) {
	CG_VRHaptic("pickup_shield", 0, 0, 100, 0, 0);
}

// cg_event.c EV_FALL_SHORT/MEDIUM/FAR. Each call site also updates
// cg.landChange/cg.landTime under the same gate, so the gate itself stays
// at the call site; severity carries the 40/60/100 intensity.
void CG_VR_OnFall( int severity ) {
	CG_VRHaptic("jump_landing", 0, 0, severity, 0, 0);
}

// cg_event.c EV_JUMP_PAD (pad=qtrue, intensity 100) / EV_JUMP (pad=qfalse,
// intensity 50).
void CG_VR_OnJump( qboolean pad ) {
	if ( pad ) {
		CG_VRHaptic("jump_start", 0, 0, 100, 0, 0);
	} else {
		CG_VRHaptic("jump_start", 0, 0, 50, 0, 0);
	}
}

// cg_event.c EV_ITEM_PICKUP. The three item-kind branches all shared the
// same local-predicted gate, so that gate stays (once) at the call site;
// the giType branching that picks which haptic fires moves here verbatim.
void CG_VR_OnItemPickup( const gitem_t *item ) {
	if ( item->giType == IT_POWERUP || item->giType == IT_TEAM) {
		CG_VRHaptic("pickup_weapon", 0, 0, 80, 0, 0);
	} else if (item->giType == IT_PERSISTANT_POWERUP) {
		CG_VRHaptic("pickup_weapon", 0, 0, 50, 0, 0);
	} else {
		CG_VRHaptic("RTCWQuest:pickup_item", 0, 0, 100, 0, 0);
	}
}

// cg_event.c EV_CHANGE_WEAPON. The gate wrapped only the haptic, so it
// stays at the call site verbatim.
void CG_VR_OnWeaponSwitch( void ) {
	CG_VRHaptic("weapon_switch", 0, 0, 100, 0, 0);
}

// cg_event.c EV_PLAYER_TELEPORT_IN. The whole gated block (vrActive +
// local-predicted compare, the fake-6DoF realign, and the haptic) moves
// here verbatim; the call site is unconditional and just forwards
// clientNum.
void CG_VR_OnTeleport( int clientNum ) {
	if (vrActive && clientNum == cg.predictedPlayerState.clientNum) {
		vr->realign = 3; // Initiate position reset for fake 6DoF
		CG_VRHaptic("spark", 0, 0, 80, 0, 0);
	}
}

// cg_event.c EV_DEATH1/2/3. The gate wrapped only the haptic, so it stays
// at the call site verbatim.
void CG_VR_OnDeath( void ) {
	CG_VRHaptic("fireball", 0, 0, 100, 0, 0);
}

// cg_event.c EV_POWERUP_QUAD/BATTLESUIT/REGEN. Each of the three call
// sites keeps its own (pre-existing, not-quite-identical) local-predicted
// gate; this is just the shared haptic.
void CG_VR_OnPowerup( void ) {
	CG_VRHaptic("decontaminate", 0, 0, 100, 0, 0);
}

// cg_event.c EV_GIB_PLAYER. The gate wrapped only the haptic, so it stays
// at the call site verbatim.
void CG_VR_OnGibbed( void ) {
	CG_VRHaptic("shield_break", 0, 0, 100, 0, 0);
}

// cg_weapons.c CG_MissileHitPlayer. entityNum is passed straight through,
// so the vrActive + local-client gate moves here verbatim and the call
// site is unconditional.
void CG_VR_OnHitByMissile( int entityNum ) {
	if ( vrActive && entityNum == vr->clientNum )
	{
		CG_VRHaptic("fireball", 0, 0, 80, 0, 0);
	}
}

// cg_playerstate.c CG_DamageFeedback. damage/yaw are passed straight
// through, so the whole dispatch moves here verbatim and the call site is
// unconditional.
void CG_VR_OnDamageTaken( int damage, float yaw ) {
	if (damage > 30)
	{
		CG_VRHaptic("shotgun", 0, 0, 100, yaw, 0);
	} else {
		CG_VRHaptic("bullet", 0, 0, 100, yaw, 0);
	}
}

// cg_weapons.c CG_FireWeapon's entire per-weapon haptic dispatch, moved
// wholesale (position compute, haptic_skip rate-limit static, all weapon
// cases incl. MISSIONPACK). The local-player gate (cent->currentState.number
// == cg.predictedPlayerState.clientNum) needed `cent`, which isn't
// available here, so it stays at the call site; weapon is ent->weapon.
void CG_VR_OnWeaponFired( int weapon ) {
	int position = vr->weapon_stabilised ? 4 : (vr->right_handed ? 1 : 2);

	static int haptic_skip = 0;
	// This is to adjust the excessive fire rate of the plasma-gun/machine-gun, everything else fires slower (or has haptics that compensate)
	// so this will just fire every other time for the affected weapons
	++haptic_skip;

	//Haptics
	switch (weapon) {
		case WP_GAUNTLET:
			CG_VRHaptic("chainsaw_fire", position, 0, 50, 0, 0);
			break;
		case WP_MACHINEGUN:
			if (haptic_skip & 1) {
				CG_VRHaptic("machinegun_fire", position, 0, 100, 0, 0);
			}
			break;
		case WP_SHOTGUN:
			CG_VRHaptic("shotgun_fire", position, 0, 100, 0, 0);
			break;
		case WP_GRENADE_LAUNCHER:
			CG_VRHaptic("handgrenade_fire", position, 0, 80, 0, 0);
			break;
		case WP_ROCKET_LAUNCHER:
			CG_VRHaptic("rocket_fire", position, 0, 100, 0, 0);
			break;
		case WP_LIGHTNING:
			//Haptics handled in the CG_LightningBolt code
			break;
		case WP_RAILGUN:
			CG_VRHaptic("railgun_fire", position, 0, 100, 0, 0);
			break;
		case WP_PLASMAGUN:
			if (haptic_skip & 1) {
				CG_VRHaptic("plasmagun_fire", position, 0, 100, 0, 0);
			}
			break;
		case WP_BFG:
			CG_VRHaptic("bfg_fire", position, 0, 100, 0, 0);
			break;
		case WP_GRAPPLING_HOOK:
			//No Haptics
			break;
#ifdef MISSIONPACK
		case WP_NAILGUN:
			CG_VRHaptic("shotgun_fire", position, 0, 100, 0, 0);
			break;
		case WP_PROX_LAUNCHER:
			CG_VRHaptic("handgrenade_fire", position, 0, 100, 0, 0);
			break;
		case WP_CHAINGUN:
			if (haptic_skip & 1) {
				CG_VRHaptic("chaingun_fire", position, 0, 100, 0, 0);
			}
			break;
#endif
	}
}

// cg_weapons.c CG_LightningBolt's per-frame haptic only (the muzzle/
// AngleVectors code that surrounds it stays put). weapon is unused here -
// the original code hardcoded "tesla_fire" regardless, so this preserves
// that verbatim; the call site still only reaches this hook from inside
// its `vrActive && directView` branch.
void CG_VR_OnWeaponFiring( int weapon ) {
	// refresh haptics every frame while the bolt renders, not just on fire
	int position = vr->weapon_stabilised ? 4 : (vr->right_handed ? 1 : 2);
	CG_VRHaptic( "tesla_fire", position, 0, 100, 0, 0 );
}

/*
================
CG_VR_OnMenuMove

Menu hover feedback for cgame-driven menu surfaces (the scoreboard cursor).
================
*/
void CG_VR_OnMenuMove( void ) {
	CG_VRHaptic( "menu_move", 0, 0, 30, 0, 0 );
}

/*
==============================================================================

VIEW WEAPON

cg_weapons.c's matrix/position helpers, weapon selector, and hand-pose hooks.
CG_AddViewWeaponVR itself is gone; CG_AddViewWeapon absorbs it via the four
hooks below.

==============================================================================
*/

#define M_PI2		(float)6.28318530717958647692

/*
=================
SinCos
=================
*/
static void SinCos( float radians, float *sine, float *cosine )
{
#if _MSC_VER == 1200
    _asm
	{
		fld	dword ptr [radians]
		fsincos

		mov edx, dword ptr [cosine]
		mov eax, dword ptr [sine]

		fstp dword ptr [edx]
		fstp dword ptr [eax]
	}
#else
  // I think, better use math.h function, instead of ^
    *sine = sin(radians);
    *cosine = cos(radians);
#endif
}

static void Matrix4x4_Concat (matrix4x4 out, matrix4x4 in1, matrix4x4 in2)
{
    out[0][0] = in1[0][0] * in2[0][0] + in1[0][1] * in2[1][0] + in1[0][2] * in2[2][0] + in1[0][3] * in2[3][0];
    out[0][1] = in1[0][0] * in2[0][1] + in1[0][1] * in2[1][1] + in1[0][2] * in2[2][1] + in1[0][3] * in2[3][1];
    out[0][2] = in1[0][0] * in2[0][2] + in1[0][1] * in2[1][2] + in1[0][2] * in2[2][2] + in1[0][3] * in2[3][2];
    out[0][3] = in1[0][0] * in2[0][3] + in1[0][1] * in2[1][3] + in1[0][2] * in2[2][3] + in1[0][3] * in2[3][3];
    out[1][0] = in1[1][0] * in2[0][0] + in1[1][1] * in2[1][0] + in1[1][2] * in2[2][0] + in1[1][3] * in2[3][0];
    out[1][1] = in1[1][0] * in2[0][1] + in1[1][1] * in2[1][1] + in1[1][2] * in2[2][1] + in1[1][3] * in2[3][1];
    out[1][2] = in1[1][0] * in2[0][2] + in1[1][1] * in2[1][2] + in1[1][2] * in2[2][2] + in1[1][3] * in2[3][2];
    out[1][3] = in1[1][0] * in2[0][3] + in1[1][1] * in2[1][3] + in1[1][2] * in2[2][3] + in1[1][3] * in2[3][3];
    out[2][0] = in1[2][0] * in2[0][0] + in1[2][1] * in2[1][0] + in1[2][2] * in2[2][0] + in1[2][3] * in2[3][0];
    out[2][1] = in1[2][0] * in2[0][1] + in1[2][1] * in2[1][1] + in1[2][2] * in2[2][1] + in1[2][3] * in2[3][1];
    out[2][2] = in1[2][0] * in2[0][2] + in1[2][1] * in2[1][2] + in1[2][2] * in2[2][2] + in1[2][3] * in2[3][2];
    out[2][3] = in1[2][0] * in2[0][3] + in1[2][1] * in2[1][3] + in1[2][2] * in2[2][3] + in1[2][3] * in2[3][3];
    out[3][0] = in1[3][0] * in2[0][0] + in1[3][1] * in2[1][0] + in1[3][2] * in2[2][0] + in1[3][3] * in2[3][0];
    out[3][1] = in1[3][0] * in2[0][1] + in1[3][1] * in2[1][1] + in1[3][2] * in2[2][1] + in1[3][3] * in2[3][1];
    out[3][2] = in1[3][0] * in2[0][2] + in1[3][1] * in2[1][2] + in1[3][2] * in2[2][2] + in1[3][3] * in2[3][2];
    out[3][3] = in1[3][0] * in2[0][3] + in1[3][1] * in2[1][3] + in1[3][2] * in2[2][3] + in1[3][3] * in2[3][3];
}

static void Matrix4x4_CreateFromEntity( matrix4x4 out, const vec3_t angles, const vec3_t origin, float scale )
{
    float	angle, sr, sp, sy, cr, cp, cy;

    if( angles[ROLL] )
    {
#ifdef XASH_VECTORIZE_SINCOS
        SinCosFastVector3( DEG2RAD(angles[YAW]), DEG2RAD(angles[PITCH]), DEG2RAD(angles[ROLL]),
			&sy, &sp, &sr,
			&cy, &cp, &cr);
#else
        angle = angles[YAW] * (M_PI2 / 360.0f);
        SinCos( angle, &sy, &cy );
        angle = angles[PITCH] * (M_PI2 / 360.0f);
        SinCos( angle, &sp, &cp );
        angle = angles[ROLL] * (M_PI2 / 360.0f);
        SinCos( angle, &sr, &cr );
#endif

        out[0][0] = (cp*cy) * scale;
        out[0][1] = (sr*sp*cy+cr*-sy) * scale;
        out[0][2] = (cr*sp*cy+-sr*-sy) * scale;
        out[0][3] = origin[0];
        out[1][0] = (cp*sy) * scale;
        out[1][1] = (sr*sp*sy+cr*cy) * scale;
        out[1][2] = (cr*sp*sy+-sr*cy) * scale;
        out[1][3] = origin[1];
        out[2][0] = (-sp) * scale;
        out[2][1] = (sr*cp) * scale;
        out[2][2] = (cr*cp) * scale;
        out[2][3] = origin[2];
        out[3][0] = 0.0f;
        out[3][1] = 0.0f;
        out[3][2] = 0.0f;
        out[3][3] = 1.0f;
    }
    else if( angles[PITCH] )
    {
#ifdef XASH_VECTORIZE_SINCOS
        SinCosFastVector2( DEG2RAD(angles[YAW]), DEG2RAD(angles[PITCH]),
						  &sy, &sp,
						  &cy, &cp);
#else
        angle = angles[YAW] * (M_PI2 / 360.0f);
        SinCos( angle, &sy, &cy );
        angle = angles[PITCH] * (M_PI2 / 360.0f);
        SinCos( angle, &sp, &cp );
#endif

        out[0][0] = (cp*cy) * scale;
        out[0][1] = (-sy) * scale;
        out[0][2] = (sp*cy) * scale;
        out[0][3] = origin[0];
        out[1][0] = (cp*sy) * scale;
        out[1][1] = (cy) * scale;
        out[1][2] = (sp*sy) * scale;
        out[1][3] = origin[1];
        out[2][0] = (-sp) * scale;
        out[2][1] = 0.0f;
        out[2][2] = (cp) * scale;
        out[2][3] = origin[2];
        out[3][0] = 0.0f;
        out[3][1] = 0.0f;
        out[3][2] = 0.0f;
        out[3][3] = 1.0f;
    }
    else if( angles[YAW] )
    {
        angle = angles[YAW] * (M_PI2 / 360.0f);
        SinCos( angle, &sy, &cy );

        out[0][0] = (cy) * scale;
        out[0][1] = (-sy) * scale;
        out[0][2] = 0.0f;
        out[0][3] = origin[0];
        out[1][0] = (sy) * scale;
        out[1][1] = (cy) * scale;
        out[1][2] = 0.0f;
        out[1][3] = origin[1];
        out[2][0] = 0.0f;
        out[2][1] = 0.0f;
        out[2][2] = scale;
        out[2][3] = origin[2];
        out[3][0] = 0.0f;
        out[3][1] = 0.0f;
        out[3][2] = 0.0f;
        out[3][3] = 1.0f;
    }
    else
    {
        out[0][0] = scale;
        out[0][1] = 0.0f;
        out[0][2] = 0.0f;
        out[0][3] = origin[0];
        out[1][0] = 0.0f;
        out[1][1] = scale;
        out[1][2] = 0.0f;
        out[1][3] = origin[1];
        out[2][0] = 0.0f;
        out[2][1] = 0.0f;
        out[2][2] = scale;
        out[2][3] = origin[2];
        out[3][0] = 0.0f;
        out[3][1] = 0.0f;
        out[3][2] = 0.0f;
        out[3][3] = 1.0f;
    }
}

static void Matrix4x4_ConvertToEntity( vec4_t *in, vec3_t angles, vec3_t origin )
{
  float xyDist = sqrt( in[0][0] * in[0][0] + in[1][0] * in[1][0] );

  // enough here to get angles?
  if( xyDist > 0.001f )
  {
    angles[0] = RAD2DEG( atan2( -in[2][0], xyDist ));
    angles[1] = RAD2DEG( atan2( in[1][0], in[0][0] ));
    angles[2] = RAD2DEG( atan2( in[2][1], in[2][2] ));
  }
  else	// forward is mostly Z, gimbal lock
  {
    angles[0] = RAD2DEG( atan2( -in[2][0], xyDist ));
    angles[1] = RAD2DEG( atan2( -in[0][1], in[1][1] ));
    angles[2] = 0.0f;
  }

  origin[0] = in[0][3];
  origin[1] = in[1][3];
  origin[2] = in[2][3];
}

void rotateAboutOrigin(float x, float y, float rotation, vec2_t out)
{
	out[0] = cos(DEG2RAD(-rotation)) * x  +  sin(DEG2RAD(-rotation)) * y;
	out[1] = cos(DEG2RAD(-rotation)) * y  -  sin(DEG2RAD(-rotation)) * x;
}

void CG_ConvertFromVR(vec3_t in, vec3_t offset, vec3_t out)
{
	vec3_t vrSpace;
	vec2_t r;
	vec3_t temp;

	VectorSet(vrSpace, in[2], in[0], in[1] );

	if ( cg.predictedPlayerState.pm_type == PM_INTERMISSION ) {
		// Match the position offset to the frozen reference the orientation uses,
		// else clientviewangles drift orbits the camera in MP (use_6dof is false).
		float angleYaw = cg.refdefViewAngles[YAW] - vr->hmdorientation[YAW];
		rotateAboutOrigin(vrSpace[0], vrSpace[1], angleYaw, r);
	} else if (!vr->use_6dof)
	{
		//We are not using true 6DoF, so make the appropriate adjustment to the view
		//angles as we send orientation to the server that includes the weapon angles
		float deltaYaw = SHORT2ANGLE(cg.predictedPlayerState.delta_angles[YAW]);
		float angleYaw;
		if (cg.demoPlayback || (cg.snap->ps.pm_flags & PMF_FOLLOW))
		{
			// Don't include delta if following another player, or playing back a demo
			// In these cases, we're a floating camera, not the player.
			deltaYaw = 0.0f;
		}
		angleYaw = deltaYaw + (vr->clientviewangles[YAW] - vr->hmdorientation[YAW]);
		rotateAboutOrigin(vrSpace[0], vrSpace[1], angleYaw, r);
	} else {
		float angleYaw;
		if (cg.demoPlayback || (cg.snap->ps.pm_flags & PMF_FOLLOW))
		{
			// Don't rotate HMD position when spectating/demo - use camera's own orientation
			angleYaw = vr->clientviewangles[YAW] - vr->hmdorientation[YAW];
		}
		else
		{
			angleYaw = cg.refdefViewAngles[YAW] - vr->hmdorientation[YAW];
		}
		rotateAboutOrigin(vrSpace[0], vrSpace[1], angleYaw, r);
	}

	vrSpace[0] = -r[0];
	vrSpace[1] = -r[1];

	VectorScale(vrSpace, cg.worldscale, temp);

	if (offset) {
		VectorAdd(temp, offset, out);
	} else {
		VectorCopy(temp, out);
	}
}

static void CG_CalculateVRPositionInWorld( vec3_t in_position,  vec3_t in_offset, vec3_t in_orientation, vec3_t origin, vec3_t angles )
{
	if (!vr->use_6dof)
	{
		//Use absolute position for faked 6DoF
		vec3_t offset;
		VectorSubtract(in_position, vr->hmdorigin, offset);
		offset[1] = 0; // up/down is index 1 in this case
		CG_ConvertFromVR(offset, cg.refdef.vieworg, origin);
		origin[2] -= PLAYER_HEIGHT;
		origin[2] += in_position[1] * cg.worldscale;
	}
	else
	{
		//Singleplayer - true 6DoF offset from HMD
		vec3_t offset;
		VectorCopy(in_offset, offset);
		offset[1] = 0; // up/down is index 1 in this case
		CG_ConvertFromVR(offset, cg.refdef.vieworg, origin);
		origin[2] -= PLAYER_HEIGHT;
		origin[2] += in_position[1] * cg.worldscale;
	}

	VectorCopy(in_orientation, angles);
	if ( !vr->use_6dof )
	{
		//Calculate the offhand angles from "first principles"
		float deltaYaw = SHORT2ANGLE(cg.predictedPlayerState.delta_angles[YAW]);
		angles[YAW] = deltaYaw + (vr->clientviewangles[YAW] - vr->hmdorientation[YAW]) + in_orientation[YAW];
	}
	else
	{
		angles[YAW] += (cg.refdefViewAngles[YAW] - vr->hmdorientation[YAW]);
	}
}

void CG_CalculateVROffHandPosition( vec3_t origin, vec3_t angles )
{
	CG_CalculateVRPositionInWorld(vr->offhandposition, vr->offhandoffset, vr->offhandangles, origin, angles);
}

void CG_CalculateVRWeaponPosition( vec3_t origin, vec3_t angles )
{
	if ( cg.demoPlayback || (cg.snap->ps.pm_flags & PMF_FOLLOW))
	{
		CG_CalculateWeaponPosition(origin, angles);
		return;
	}

	CG_CalculateVRPositionInWorld(vr->weaponposition, vr->weaponoffset, vr->weaponangles, origin, angles);
}

//Selects the currently selected weapon (if one _is_ selected)
void CG_WeaponSelectorSelect_f( void )
{
	cg.weaponSelectorTime = 0;

	if (cg.weaponSelectorSelection == WP_NONE || cg.weaponSelect == cg.weaponSelectorSelection)
	{
		return;
	}

	cg.weaponSelectTime = cg.time;
	cg.weaponSelect = cg.weaponSelectorSelection;
	cg.weaponSelectorSelection = WP_NONE;
}

static float length(float x, float y)
{
	return sqrt(x * x + y * y);
}

void CG_DrawWeaponSelector( void )
{
	int selectorMode;
	float dist;
	float radius;
	float scale;
	float frac;
	vec3_t controllerOrigin, controllerAngles, controllerOffset, selectorOrigin;
	vec3_t wheelAngles, wheelOrigin, beamOrigin, wheelForward, wheelRight, wheelUp;
	int switchThumbsticks;
	int thumb;
	float thumbstickAxisX;
	float thumbstickAxisY;
	float a;
	float thumbstickValue;
	float x;
	float y;
#ifdef MISSIONPACK
	float wheelIconAngles[WP_NUM_WEAPONS] = {0.0f, 45.0f, 90.0f, 120.0f, 150.0f, 180.0f, 210.0f, 240.0f, 270.0f, 300.0f, 330.0f, 360.0f, 390.0f};
#else
	float wheelIconAngles[WP_NUM_WEAPONS] = {0.0f, 45.0f, 90.0f, 135.0f, 180.0f, 225.0f, 270.0f, 315.0f, 360.0f, 390.0f};
#endif
	qboolean selected;
	int angleIndex;
	int weaponId;
	char cvarBuf[16];

	if (cg.weaponSelectorTime == 0)
	{
		cg.weaponSelectorTime = cg.time;
		VectorCopy(vr->weaponangles, cg.weaponSelectorAngles);
		VectorCopy(vr->weaponposition, cg.weaponSelectorOrigin);
		VectorCopy(vr->weaponoffset, cg.weaponSelectorOffset);
	}

	// trinity cgame has no trap_Cvar_VariableValue; read via string buffer
	trap_Cvar_VariableStringBuffer( "vr_weaponSelectorMode", cvarBuf, sizeof( cvarBuf ) );
	selectorMode = (int)atof( cvarBuf );
	dist = 10.0f;
	radius = 4.0f;
	scale = 0.05f;

	if (selectorMode == WS_HMD) // HMD locked
	{
		VectorCopy(vr->hmdorientation, cg.weaponSelectorAngles);
		VectorCopy(vr->hmdposition, cg.weaponSelectorOrigin);
		VectorClear(cg.weaponSelectorOffset);
		trap_Cvar_VariableStringBuffer( "vr_currentHudDepth", cvarBuf, sizeof( cvarBuf ) );
		// Distance formula: depth 0-5 maps to 9-54 units (matches HUD distance)
		dist = (atof( cvarBuf ) + 1) * 9;
		radius = dist / 3.0f;
		// Scale formula: depth 0-5 maps to 0.05-0.20 (preserves old range)
		scale = 0.05f + 0.03f * atof( cvarBuf );
	}

	frac = (cg.time - cg.weaponSelectorTime) / 100.0f;
	if (frac > 1.0f)
	{
		frac = 1.0f;
	}

	CG_CalculateVRWeaponPosition(controllerOrigin, controllerAngles);
	VectorSubtract(vr->weaponposition, cg.weaponSelectorOrigin, controllerOffset);

	CG_CalculateVRPositionInWorld(cg.weaponSelectorOrigin, cg.weaponSelectorOffset, cg.weaponSelectorAngles, wheelOrigin, wheelAngles);

	// Zero out roll so "up" on the wheel is always world-relative, not controller-roll-relative
	wheelAngles[ROLL] = 0;
	AngleVectors(wheelAngles, wheelForward, wheelRight, wheelUp);

	if (selectorMode == WS_CONTROLLER)
	{
		VectorCopy(controllerOrigin, wheelOrigin);
	}
	else
	{
		// Do not shift weapon wheel down in order to fit inside comfort vignette
		//VectorMA(wheelOrigin, -3.0f, wheelUp, wheelOrigin);
	}

	VectorCopy(wheelOrigin, beamOrigin);
	VectorMA(wheelOrigin, (dist * ((selectorMode == WS_CONTROLLER) ? frac : 1.0f)), wheelForward, wheelOrigin);
	VectorCopy(wheelOrigin, selectorOrigin);

	trap_Cvar_VariableStringBuffer( "vr_switchThumbsticks", cvarBuf, sizeof( cvarBuf ) );
	switchThumbsticks = (int)atof( cvarBuf );
	thumb = switchThumbsticks !=0 ? THUMB_LEFT : THUMB_RIGHT;

	thumbstickAxisX = 0.0f;
	thumbstickAxisY = 0.0f;
	a = atan2(vr->thumbstick_location[thumb][0], vr->thumbstick_location[thumb][1]);
	thumbstickValue = length(vr->thumbstick_location[thumb][0], vr->thumbstick_location[thumb][1]);
	if (thumbstickValue > 0.95f)
	{
		thumbstickAxisX = sin(a) * 0.95f;
		thumbstickAxisY = cos(a) * 0.95f;
	}

	x = 0.0f;
	y = 0.0f;
	if (selectorMode == WS_CONTROLLER)
	{
		float len;

		x = (sin(DEG2RAD(wheelAngles[YAW] - controllerAngles[YAW])) / sin(DEG2RAD(22.5f)));
		y = ((wheelAngles[PITCH] - controllerAngles[PITCH]) / 22.5f);

		len = length(x, y);
		if (len > 1.0f)
		{
			x *= (1.0f / len);
			y *= (1.0f / len);
		}
	}
	else //selectorMode == WS_HMD
	{
		x = thumbstickAxisX;
		y = thumbstickAxisY;
	}

	VectorMA(selectorOrigin, radius * x, wheelRight, selectorOrigin);
	VectorMA(selectorOrigin, radius * y, wheelUp, selectorOrigin);

	{
		refEntity_t		blob;
		memset( &blob, 0, sizeof( blob ) );
		VectorCopy( selectorOrigin, blob.origin );
		AnglesToAxis(vec3_origin, blob.axis);
		VectorScale( blob.axis[0], scale - 0.01f, blob.axis[0] );
		VectorScale( blob.axis[1], scale - 0.01f, blob.axis[1] );
		VectorScale( blob.axis[2], scale - 0.01f, blob.axis[2] );
		blob.nonNormalizedAxes = qtrue;
		blob.renderfx = RF_FIRST_PERSON;
		blob.hModel = cgs.media.smallSphereModel;
		trap_R_AddRefEntityToScene( &blob );

		if (selectorMode == WS_CONTROLLER)
		{
			byte colour[4];
			colour[0] = 0x00;
			colour[1] = 0x00;
			colour[2] = 0xff;
			colour[3] = 0x40;
			CG_LaserSight(beamOrigin, selectorOrigin, colour, 0.1f);
		}
	}

	selected = qfalse;
	angleIndex = 0;
	for (weaponId = 1; weaponId < WP_NUM_WEAPONS; ++weaponId)
	{
		if (weaponId == WP_GRAPPLING_HOOK || weaponId == WP_GAUNTLET)
		{
			continue;
		}

		//increment now we know we aren't looking at an invalid weapon id
		++angleIndex;

		CG_RegisterWeapon(weaponId);

		{
			qboolean selectable = CG_WeaponSelectable(weaponId) && cg.snap->ps.ammo[weaponId];
			vec3_t angles, iconOrigin,iconBackground,iconForeground;
			vec3_t forward, up;

			//first calculate wheel slot position
			VectorClear(angles);
			angles[YAW] = wheelAngles[YAW];
			angles[PITCH] = wheelAngles[PITCH];
			angles[ROLL] = wheelIconAngles[angleIndex];
			AngleVectors(angles, forward, NULL, up);

			VectorMA(wheelOrigin, (radius*frac), up, iconOrigin);
			VectorMA(iconOrigin, 0.2f, forward, iconBackground);
			VectorMA(iconOrigin, -0.2f, forward, iconForeground);

			if (selectorMode == WS_CONTROLLER)
			{
				vec3_t diff;
				float length;

				VectorSubtract(selectorOrigin, iconOrigin, diff);
				length = VectorLength(diff);
				if (length <= 1.4f && frac == 1.0f && selectable)
				{
					if (cg.weaponSelectorSelection != weaponId)
					{
						cg.weaponSelectorSelection = weaponId;
						CG_VRHaptic("selector_icon", 0, 0, 100, 0, 0);
					}

					selected = qtrue;
				}
			}
			else
			{
				//For HMD selector, the weapon can be selected before the selector has finished
				//its opening animation, use angles to identify the selected weapon, rather than
				//the position of the selector pointer
				float angle = AngleNormalize360(RAD2DEG(a));
				float angle360 = angle + 360; // HACK - Account for the icon at the top

				float low = ((wheelIconAngles[angleIndex-1]+wheelIconAngles[angleIndex])/2.0f);
				float high = ((wheelIconAngles[angleIndex]+wheelIconAngles[angleIndex+1])/2.0f);

				if (((angle > low && angle <= high) || (angle360 > low && angle360 <= high)) &&
				    (length(vr->thumbstick_location[thumb][0], vr->thumbstick_location[thumb][1]) > 0.5f) &&
				    selectable)
				{
					if (cg.weaponSelectorSelection != weaponId)
					{
						cg.weaponSelectorSelection = weaponId;
						CG_VRHaptic("selector_icon", 0, 0, 100, 0, 0);
					}

					selected = qtrue;
				}
			}

			if (cg.weaponSelectorSelection == weaponId)
			{
				refEntity_t		sprite;
				memset( &sprite, 0, sizeof( sprite ) );
				VectorCopy( iconOrigin, sprite.origin );
				sprite.origin[2] += 2.5f + (0.5f * sin(DEG2RAD(AngleNormalize360((cg.time - cg.weaponSelectorTime)/4))));
				sprite.reType = RT_SPRITE;
				sprite.renderfx = RF_FIRST_PERSON;
				sprite.customShader = cgs.media.friendShader;
				sprite.radius = 0.5f;
				sprite.shaderRGBA.rgba[0] = 255;
				sprite.shaderRGBA.rgba[1] = 255;
				sprite.shaderRGBA.rgba[2] = 255;
				sprite.shaderRGBA.rgba[3] = 255;
				trap_R_AddRefEntityToScene( &sprite );
			}

			if (!cg_weaponSelectorSimple2DIcons.integer)
			{
				refEntity_t ent;
				vec3_t iconAngles;
				float weaponScale;

				memset(&ent, 0, sizeof(ent));
				VectorCopy(iconOrigin, ent.origin);

				//Shift model a bit
				VectorMA(ent.origin, 0.3f, wheelForward, ent.origin);
				VectorMA(ent.origin, -0.2f, wheelRight, ent.origin);
				VectorMA(ent.origin, 0.1f, wheelUp, ent.origin);

				// Use wheel orientation so icons face consistently relative to the wheel
				iconAngles[PITCH] = 10;
				iconAngles[YAW] = wheelAngles[YAW] - 145.0f;
				iconAngles[ROLL] = 0;
				if (weaponId == WP_GAUNTLET)
				{
					iconAngles[ROLL] = -90.0f;
				}

				weaponScale = ((scale+0.02f)*frac) + (cg.weaponSelectorSelection == weaponId ? 0.04f : 0);

				AnglesToAxis(iconAngles, ent.axis);
				VectorScale(ent.axis[0], weaponScale, ent.axis[0]);
				VectorScale(ent.axis[1], weaponScale, ent.axis[1]);
				VectorScale(ent.axis[2], weaponScale, ent.axis[2]);
				ent.nonNormalizedAxes = qtrue;

				if( weaponId == WP_RAILGUN ) {
					clientInfo_t *ci = &cgs.clientinfo[cg.predictedPlayerState.clientNum];
					if( cg_entities[cg.predictedPlayerState.clientNum].pe.railFireTime + 1500 > cg.time ) {
						int colorScale = 255 * ( cg.time - cg_entities[cg.predictedPlayerState.clientNum].pe.railFireTime ) / 1500;
						ent.shaderRGBA.rgba[0] = ( ci->c1RGBA[0] * colorScale ) >> 8;
						ent.shaderRGBA.rgba[1] = ( ci->c1RGBA[1] * colorScale ) >> 8;
						ent.shaderRGBA.rgba[2] = ( ci->c1RGBA[2] * colorScale ) >> 8;
						ent.shaderRGBA.rgba[3] = 255;
					}
					else {
						Byte4Copy( ci->c1RGBA, ent.shaderRGBA.rgba );
					}
				}

				ent.hModel = cg_weapons[weaponId].weaponModel;
				ent.renderfx = RF_OVERBRIGHT | RF_FIRST_PERSON;
				if (!selectable)
				{
					ent.customShader = cgs.media.invisShader;
				}
				trap_R_AddRefEntityToScene(&ent);

				if ( cg_weapons[weaponId].barrelModel )
				{
				refEntity_t barrel;
				vec3_t barrelAngles;

				memset(&barrel, 0, sizeof(barrel));
				barrel.hModel = cg_weapons[weaponId].barrelModel;
				barrel.renderfx = RF_OVERBRIGHT | RF_FIRST_PERSON;
				VectorClear(barrelAngles);
				barrelAngles[ROLL] = AngleNormalize360((cg.time - cg.weaponSelectorTime) * 0.9f);
				AnglesToAxis(barrelAngles, barrel.axis);
				CG_PositionRotatedEntityOnTag(&barrel, &ent, cg_weapons[weaponId].weaponModel, "tag_barrel");
				if (!selectable)
				{
					barrel.customShader = cgs.media.invisShader;
				}
				trap_R_AddRefEntityToScene(&barrel);
				}
			}
			else
			{
				refEntity_t		sprite;
				// Scale icon radius proportionally to distance so apparent size stays consistent
				float sRadius = dist * 0.08f;

				memset( &sprite, 0, sizeof( sprite ) );

				VectorCopy(iconOrigin, sprite.origin);
				sprite.reType = RT_SPRITE;
				sprite.renderfx = RF_FIRST_PERSON;
				sprite.customShader = cg_weapons[weaponId].weaponIcon;
				sprite.radius = sRadius * 0.9f * (cg.weaponSelectorSelection == weaponId ? 1.1f : 1.0);
				sprite.shaderRGBA.rgba[0] = 255;
				sprite.shaderRGBA.rgba[1] = 255;
				sprite.shaderRGBA.rgba[2] = 255;
				sprite.shaderRGBA.rgba[3] = 255;
				trap_R_AddRefEntityToScene(&sprite);

				//And now the selection background
				memset( &sprite, 0, sizeof( sprite ) );
				VectorCopy( iconBackground, sprite.origin );
				sprite.reType = RT_SPRITE;
				sprite.renderfx = RF_FIRST_PERSON;
				sprite.customShader = cgs.media.selectShader;
				sprite.radius = sRadius * (cg.weaponSelectorSelection == weaponId ? 1.1f : 1.0);
				sprite.shaderRGBA.rgba[0] = 255;
				sprite.shaderRGBA.rgba[1] = 255;
				sprite.shaderRGBA.rgba[2] = 255;
				sprite.shaderRGBA.rgba[3] = 255;
				trap_R_AddRefEntityToScene( &sprite );

				if (!selectable)
				{
					memset(&sprite, 0, sizeof(sprite));
					VectorCopy(iconForeground, sprite.origin);
					sprite.reType = RT_SPRITE;
					sprite.renderfx = RF_FIRST_PERSON;
					sprite.customShader = cgs.media.noammoShader;
					sprite.radius = sRadius;
					sprite.shaderRGBA.rgba[0] = 255;
					sprite.shaderRGBA.rgba[1] = 255;
					sprite.shaderRGBA.rgba[2] = 255;
					sprite.shaderRGBA.rgba[3] = 255;
					trap_R_AddRefEntityToScene(&sprite);
				}
			}
		}
	}

	//Only reset selection if using controller pointer
	if (!selected && selectorMode == WS_CONTROLLER)
	{
		cg.weaponSelectorSelection = WP_NONE;
	}

	// In case was invoked by thumbstick axis and thumbstick is centered
	// select weapon (if any selected) and close the selector
	if (vr->weapon_select_autoclose && frac > 0.25f) {
		if (thumbstickValue < 0.1f) {
			if (selected) {
				cg.weaponSelect = cg.weaponSelectorSelection;
			}
			vr->weapon_select = qfalse;
			vr->weapon_select_autoclose = qfalse;
			vr->weapon_select_using_thumbstick = qfalse;
		}
	}
}


qboolean CG_VR_WeaponWheel( void ) {
	if ( vrActive && vr->weapon_select && !vr->weapon_adjust )
	{
		CG_DrawWeaponSelector();
		return qtrue;
	}
	return qfalse;
}

qboolean CG_VR_HideViewWeapon( void ) {
	// do not draw weapon model with enabled weapon scope or when in menu
	return vrActive && ( vr->weapon_zoomed || (vr->virtual_screen && !(vr->first_person_following)) );
}

qboolean CG_VR_WeaponHandPose( vec3_t origin, vec3_t angles, float *scale ) {
	vec3_t		forward, end;
	trace_t		trace;
	byte		colour[4];
	char		lasersightBuf[16];

	if ( !vrActive ) {
		return qfalse;
	}

	// set up gun position
	CG_CalculateVRWeaponPosition( origin, angles );

	// trinity cgame has no trap_Cvar_VariableValue; read via string buffer
	trap_Cvar_VariableStringBuffer( "vr_lasersight", lasersightBuf, sizeof( lasersightBuf ) );
	if (atof( lasersightBuf ) != 0.0f && !vr->no_crosshair &&
		// The laser sight looks terrible in first-person follow mode, and shouldn't render
		// in deathcam either.
		!(CG_VR_IsDeathCam() ||
			(cg.snap->ps.pm_flags & PMF_FOLLOW && vr->follow_mode == VRFM_FIRSTPERSON)))
	{
		AngleVectors(angles, forward, NULL, NULL);
		VectorMA(origin, 4096, forward, end);
		CG_Trace(&trace, origin, NULL, NULL, end, cg.predictedPlayerState.clientNum, MASK_SOLID);

		colour[0] = 0xff;
		colour[1] = 0x00;
		colour[2] = 0x00;
		colour[3] = 0x40;

		CG_LaserSight(origin, trace.endpos, colour, 1.0f);
	}

		//Scale / Move gun etc
	*scale = 1.0f;
	if (!(((cg.snap->ps.pm_flags & PMF_FOLLOW) || cg.demoPlayback) && vr->follow_mode == VRFM_FIRSTPERSON))
	{
		char cvar_name[64];
		char weapon_adjustment[256];

		Com_sprintf(cvar_name, sizeof(cvar_name), "vr_weapon_adjustment_%i", cg.predictedPlayerState.weapon);

		trap_Cvar_VariableStringBuffer(cvar_name, weapon_adjustment, 256);

		if (strlen(weapon_adjustment) > 0) {
			vec3_t offset;
			vec3_t adjust;
			vec3_t temp_offset;
			matrix4x4 m1, m2, m3;
			vec3_t zero;
			vec3_t forward, right, up;
			VectorClear(temp_offset);
			VectorClear(adjust);

			Q_sscanf(weapon_adjustment, "%f,%f,%f,%f,%f,%f,%f", scale,
				   &(temp_offset[0]), &(temp_offset[1]), &(temp_offset[2]),
				   &(adjust[PITCH]), &(adjust[YAW]), &(adjust[ROLL]));
			VectorScale(temp_offset, *scale, offset);

			if (!vr->right_handed)
			{
				//yaw needs to go in the other direction as left handed model is reversed
				adjust[YAW] *= -1.0f;
			}

			//Adjust angles for weapon models that aren't aligned very well
			VectorClear(zero);
			Matrix4x4_CreateFromEntity(m1, angles, zero, 1.0);
			Matrix4x4_CreateFromEntity(m2, adjust, zero, 1.0);
			Matrix4x4_Concat(m3, m1, m2);
			Matrix4x4_ConvertToEntity(m3, angles, zero);

			//Now move weapon closer to proper origin
			AngleVectors( angles, forward, right, up );
			VectorMA( origin, offset[2], forward, origin );
			VectorMA( origin, offset[1], up, origin );
			if (vr->right_handed) {
				VectorMA(origin, offset[0], right, origin);
			} else {
				VectorMA(origin, -offset[0], right, origin);
			}
		}
	}

	//Move gun a bit
	if ( CG_VR_IsVRFollow() ) {
		vec3_t	weaponAxis[3];
		AnglesToAxis( cg.predictedPlayerState.viewangles, weaponAxis );
		VectorMA( origin, cg_gun_x.value, weaponAxis[0], origin );
		VectorMA( origin, cg_gun_y.value, weaponAxis[1], origin );
		VectorMA( origin, cg_gun_z.value, weaponAxis[2], origin );
	} else {
		VectorMA( origin, cg_gun_x.value, cg.refdef.viewaxis[0], origin );
		VectorMA( origin, cg_gun_y.value, cg.refdef.viewaxis[1], origin );
		VectorMA( origin, cg_gun_z.value, cg.refdef.viewaxis[2], origin );
	}

	return qtrue;
}

void CG_VR_WeaponHandFinish( refEntity_t *hand, float scale ) {
	int i;

	hand->renderfx = /*RF_DEPTHHACK |*/ RF_FIRST_PERSON | RF_MINLIGHT;

	//scale the whole model
	for ( i = 0; i < 3; i++ ) {
		VectorScale( hand->axis[i], vr->right_handed || i != 1 ? scale : -scale, hand->axis[i] );
	}
}

/*
=====================
CG_VR_DrawFrame

VR draw tail: renders the 3D scene, then draws 2D overlays and the HUD
inside the engine's post-bloom / HUD-buffer brackets. Optional engine
traps are used only when the engine advertised them (has* presence flags);
otherwise the bracketed op is skipped and content still draws to screen.
=====================
*/
qboolean CG_VR_DrawFrame( stereoFrame_t stereoView ) {
	vec3_t baseOrg;
	float  heightOffset = 0.0f;
	float  worldscale = cg.worldscale;

	if ( !vrActive ) {
		return qfalse;
	}

	if ( !vr->weapon_zoomed && (!vr->virtual_screen || vr->first_person_following) ) {
		CG_DrawCrosshair3D();
	}

	// Fold the camera anchor from Q3 viewheight down to the floor-relative HMD
	// frame the weapon math assumes, then save the pre-fold origin so it can be
	// restored after the scene is submitted.
	VectorCopy( cg.refdef.vieworg, baseOrg );

	if ( CG_VR_IsThirdPersonFollow(VRFM_THIRDPERSON_1) )
	{
		worldscale *= SPECTATOR_WORLDSCALE_MULTIPLIER;
		trap_Cvar_Set( "vr_worldscaleScaler", va( "%i", SPECTATOR_WORLDSCALE_MULTIPLIER ) );
		//Just move camera down about 20cm
		heightOffset = -0.2f;
	}
	else if (CG_VR_IsDeathCam() || CG_VR_IsThirdPersonFollow(VRFM_THIRDPERSON_2))
	{
		worldscale *= SPECTATOR2_WORLDSCALE_MULTIPLIER;
		trap_Cvar_Set( "vr_worldscaleScaler", va( "%i", SPECTATOR2_WORLDSCALE_MULTIPLIER ) );
		//Just move camera down about 50cm
		heightOffset = -0.5f;
	}
	else
	{
		float zoomCoeff =  ((2.5f-vr->weapon_zoomLevel)/1.5f); // normally 1.0
		trap_Cvar_Set( "vr_worldscaleScaler", va( "%f", zoomCoeff ) );
	}

	if (vr->virtual_screen || CG_VR_IsDeathCam() || CG_VR_IsThirdPersonFollow(VRFM_QUERY))
	{
		// Do nothing to view height if we are viewing the virtual screen or in a camera mode
		// view is already positioned correctly
	}
	else
	{
		cg.refdef.vieworg[2] -= PLAYER_HEIGHT;
		cg.refdef.vieworg[2] += (vr->hmdposition[1] + heightOffset) * worldscale;
	}

	if ( !vr->use_6dof && !vr->virtual_screen )
	{
		// If not using true 6DoF, allow some amount of faked positional tracking
		if ( cg.snap->ps.stats[STAT_HEALTH] > 0 &&
		     // Intermission folds absolute HMD position into vieworg itself; this
		     // hmdorigin-relative path would fight it and snap back on realign.
		     cg.snap->ps.pm_type != PM_INTERMISSION &&
		     // Don't use fake positional if following another player - this is
		     // handled in the VR third person code
		     !( cg.demoPlayback || CG_VR_IsThirdPersonFollow(VRFM_QUERY) ) )
		{
			vec3_t  pos, hmdposition, vieworg;
			float   angleYaw;
			trace_t trace;
			vec3_t  mins = {-8, -8, -8};
			vec3_t  maxs = {8, 8, 8};

			VectorClear( pos );
			VectorSubtract( vr->hmdposition, vr->hmdorigin, hmdposition );

			angleYaw = SHORT2ANGLE( cg.predictedPlayerState.delta_angles[YAW] ) + ( vr->clientviewangles[YAW] - vr->hmdorientation[YAW] );
			rotateAboutOrigin( hmdposition[2], hmdposition[0], angleYaw, pos );
			VectorScale( pos, worldscale, pos );
			VectorSubtract( cg.refdef.vieworg, pos, vieworg );

			// Prevent player clipping through solid objects
			CG_Trace( &trace, cg.refdef.vieworg, mins, maxs, vieworg, cg.snap->ps.clientNum, CONTENTS_SOLID|CONTENTS_BODY );

			VectorCopy( trace.endpos, cg.refdef.vieworg );
		}
	}

	// Draw the HUD sprite in the world (HUD mode 1)
	// Also used for SP intermission UI (world-locked at podium position)
	{
		char      cvarBuf[16];
		qboolean  isSPIntermission;
		qboolean  drawHUDSprite;

		isSPIntermission = (cg.snap->ps.pm_type == PM_INTERMISSION) &&
		                   (cgs.gametype == GT_SINGLE_PLAYER);
		// trinity cgame has no trap_Cvar_VariableValue; read via string buffer
		trap_Cvar_VariableStringBuffer( "vr_currentHudDrawStatus", cvarBuf, sizeof( cvarBuf ) );
		drawHUDSprite = ((atof( cvarBuf ) != 2.0f &&
		                  !vr->weapon_zoomed && !vr->virtual_screen) || isSPIntermission);

		if (drawHUDSprite)
		{
			refEntity_t ent;
			vec3_t endpos, angles;
			vec3_t forward, right, up;
			vec3_t spriteAxis[3];  // For world-oriented sprites
			qboolean worldOrientedSprite = qfalse;
			float scale;
			float dist;
			float radius;

			trap_Cvar_VariableStringBuffer( "vr_worldscaleScaler", cvarBuf, sizeof( cvarBuf ) );
			scale = atof( cvarBuf );
			// Distance formula: depth 0-5 maps to 9-54 units (before worldscale)
			trap_Cvar_VariableStringBuffer( "vr_currentHudDepth", cvarBuf, sizeof( cvarBuf ) );
			dist = (atof( cvarBuf ) + 1) * 9 * scale;
			trap_Cvar_VariableStringBuffer( "vr_hudScale", cvarBuf, sizeof( cvarBuf ) );
			radius = (dist / 3.0f) * atof( cvarBuf );

			if (isSPIntermission)
			{
				// SP intermission: position HUD sprite at podium (world-locked)
				// Use the absolute world position calculated in CG_CalculateSPIntermissionHUD
				VectorCopy(vr->sp_intermission_hud_origin, endpos);

				// Use pre-calculated fixed radius (computed once in CG_CalculateSPIntermissionHUD)
				// This ensures the HUD doesn't resize when leaning forward/backward
				radius = vr->sp_intermission_hud_radius;

				// Orient sprite to face the viewer
				angles[YAW] = cg.snap->ps.viewangles[YAW];
				angles[PITCH] = 0;
				angles[ROLL] = 0;
				AngleVectors(angles, forward, right, up);

				// Store orientation for world-anchored rendering (fixed orientation, no billboarding)
				worldOrientedSprite = qtrue;
				VectorCopy(forward, spriteAxis[0]);
				VectorNegate(right, spriteAxis[1]);  // Negate: sprite expects "left", not "right"
				VectorCopy(up, spriteAxis[2]);
			}
			else if (cg.snap->ps.stats[STAT_HEALTH] > 0 &&
			         cg.snap->ps.pm_type != PM_INTERMISSION &&
			         !(cg.demoPlayback || (cg.snap->ps.pm_flags & PMF_FOLLOW)))
			{
				// Normal gameplay: account for the yaw of the player vs worldspace
				static float hmd_yaw_x = 0.0f;
				static float hmd_yaw_y = 1.0f;
				static float prevPitch = 0.0f;

				// Smooth only the HMD orientation
				hmd_yaw_x = 0.95f * hmd_yaw_x + 0.05f * cos(DEG2RAD(vr->hmdorientation[YAW]));
				hmd_yaw_y = 0.95f * hmd_yaw_y + 0.05f * sin(DEG2RAD(vr->hmdorientation[YAW]));

				if (!vr->use_6dof)
				{
					// Fake 6DoF: use clientviewangles logic
					float viewYaw = SHORT2ANGLE(cg.predictedPlayerState.delta_angles[YAW]) +
					    (vr->clientviewangles[YAW] - vr->hmdorientation[YAW]);
					angles[YAW] = viewYaw + RAD2DEG(atan2(hmd_yaw_y, hmd_yaw_x));
				}
				else
				{
					// Single player: use refdefViewAngles - HMD offset + smoothed HMD
					angles[YAW] = cg.refdefViewAngles[YAW] - vr->hmdorientation[YAW] + RAD2DEG(atan2(hmd_yaw_y, hmd_yaw_x));
				}

				angles[PITCH] = 0.95f * prevPitch + 0.05f * vr->hmdorientation[PITCH];
				prevPitch = angles[PITCH];
				angles[ROLL] = 0;
				AngleVectors(angles, forward, right, up);

				VectorMA(cg.refdef.vieworg, dist, forward, endpos);
				trap_Cvar_VariableStringBuffer( "vr_hudYOffset", cvarBuf, sizeof( cvarBuf ) );
				VectorMA(endpos, atof( cvarBuf ) / 20, up, endpos);
			}
			else
			{
				// Lock to face
				VectorMA(cg.refdef.vieworg, dist, cg.refdef.viewaxis[0], endpos);
			}

			memset(&ent, 0, sizeof(ent));
			ent.reType = RT_SPRITE;
			ent.renderfx = RF_DEPTHHACK | RF_FIRST_PERSON;

			if (worldOrientedSprite) {
				// Use world-oriented rendering (fixed orientation, no billboarding)
				ent.renderfx |= RF_WORLD_ORIENTED;
				AxisCopy(spriteAxis, ent.axis);
			}

			VectorCopy(endpos, ent.origin);

			ent.radius = radius;
			ent.invert = qtrue;
			ent.customShader = cgs.media.hudShader;

			trap_R_AddRefEntityToScene(&ent);
		}
	}

	// draw 3D view
	trap_R_RenderScene( &cg.refdef );

	// Restore the un-folded origin now that the scene has been submitted.
	VectorCopy( baseOrg, cg.refdef.vieworg );

	// Apply bloom now, BEFORE 2D drawing begins. This ensures bloom only
	// affects the 3D scene, not UI elements.
	trap_R_BeginPostBloom2D();

	{
		char  hudBuf[16];
		float hudStatus;
		qboolean selectorHidesHud;

		// trinity cgame has no trap_Cvar_VariableValue; read via string buffer
		trap_Cvar_VariableStringBuffer( "vr_currentHudDrawStatus", hudBuf, sizeof( hudBuf ) );
		hudStatus = atof( hudBuf );

		// If the weapon selector wheel is open, vr_weaponSelectorWithHud decides
		// whether the HUD content draw is skipped while it's up.
		// Match native: the selector may only suppress the HUD content draw
		// while the player is alive, not spectating, and the scoreboard is not
		// being shown (native CG_DrawHUD2D gate: outer PERS_TEAM != SPECTATOR,
		// inner !cg.showScores && stats[STAT_HEALTH] > 0).
		selectorHidesHud = qfalse;
		if ( cg.weaponSelectorTime != 0 &&
			cg.snap->ps.persistant[PERS_TEAM] != TEAM_SPECTATOR &&
			cg.snap->ps.stats[STAT_HEALTH] > 0 &&
			!cg.showScores ) {
			char selectorBuf[16];
			trap_Cvar_VariableStringBuffer( "vr_weaponSelectorWithHud", selectorBuf, sizeof( selectorBuf ) );
			selectorHidesHud = ( atof( selectorBuf ) == 0.0f );
		}

		// Draw screen 2D overlays directly to the XR swapchain
		CG_DrawScreen2D();
		CG_VRProbe_Draw();

		if (vr->weapon_zoomed)
		{
			// Weapon zoomed: render minimal HUD with scaled coordinates
			cg.drawingHUD = qtrue;
			cg.drawingZoomedHUD = qtrue;
			CG_WarmupEvents();
			CG_PushHUDAnchors();
			CG_Draw2DMinimal( STEREO_CENTER );
			CG_PopHUDAnchors();
			cg.drawingZoomedHUD = qfalse;
			cg.drawingHUD = qfalse;
		}
		else if (hudStatus == 2 && !vr->virtual_screen)
		{
			// HUD mode 2: render directly to main swapchain with stereo parallax
			cg.drawingHUD = qtrue;
			trap_R_HUDBufferStart( qfalse );
			CG_WarmupEvents();
			if ( !selectorHidesHud ) {
				CG_PushHUDAnchors();
				CG_Draw2D( STEREO_CENTER );
				CG_PopHUDAnchors();
			}
			trap_R_HUDBufferEnd();
			cg.drawingHUD = qfalse;
		}

		if (!vr->weapon_zoomed && (!vr->virtual_screen || vr->first_person_following))
		{
			if (hudStatus != 0)
			{
				cg.drawingHUD = qtrue;

				if (hudStatus == 2 && vr->first_person_following)
				{
					// HUD mode 2 in first person following: direct-to-screen with stereo offset
					trap_R_HUDBufferStart( qfalse );
					CG_WarmupEvents();
					if ( !selectorHidesHud ) {
						CG_PushHUDAnchors();
						CG_Draw2D( STEREO_CENTER );
						CG_PopHUDAnchors();
					}
					trap_R_HUDBufferEnd();
				}
				else if (hudStatus == 1)
				{
					// HUD mode 1: use existing HUD buffer (floating in-world); buffer
					// discipline is unchanged by the selector gate below - it still
					// clears even when the content draw is skipped.
					trap_R_HUDBufferStart( qtrue );
					CG_WarmupEvents();
					if ( !selectorHidesHud ) {
						CG_PushHUDAnchors();
						CG_Draw2D( STEREO_CENTER );
						CG_PopHUDAnchors();
					}
					trap_R_HUDBufferEnd();
				}

				cg.drawingHUD = qfalse;
			}
			else
			{
				// HUD disabled - just clear the HUD buffer to remove any stale content
				trap_R_HUDBufferStart( qtrue );
				trap_R_HUDBufferEnd();
			}
		}
	}

	trap_R_EndPostBloom2D();

	return qtrue;
}

/*
==============================================================================

SUPPRESSION PREDICATES / HUD VISIBILITY

cg_draw.c/cg_view.c/cg_weapons.c/cg_newdraw.c/cg_scoreboard.c 2D-HUD and
suppression gates. Each predicate answers "does VR own this decision" (or,
for CG_VR_HudVisible, "should HUD content draw at all") so call sites become
a single gate-for-predicate swap.

==============================================================================
*/
qboolean CG_VR_OwnsHudVisibility( void ) {	// A9: flatscreen HUD toggles don't apply in VR
	return vrActive;
}
qboolean CG_VR_HudVisible( void ) {
	char hudBuf[16];
	if ( !vrActive ) {
		return qtrue;
	}
	trap_Cvar_VariableStringBuffer( "vr_currentHudDrawStatus", hudBuf, sizeof( hudBuf ) );
	return atof( hudBuf ) != 0.0f;
}
qboolean CG_VR_Owns2DCrosshair( void ) { return vrActive; }		// 3D crosshair replaces it
qboolean CG_VR_OwnsWeaponSelect( void ) { return vrActive; }	// the weapon wheel replaces the 2D bar
qboolean CG_VR_OwnsDamageBorder( void ) { return vrActive; }	// drawn from the VR tail's screen-2D pass instead
qboolean CG_VR_OwnsViewBob( void ) { return vrActive; }			// bob fights the HMD-tracked camera
qboolean CG_VR_OwnsHiddenGunMuzzle( void ) { return vrActive; }	// head-derived muzzle would clobber the controller muzzle
qboolean CG_VR_SuppressDeadScoreboard( void ) {
	return CG_VR_IsThirdPersonFollow( VRFM_THIRDPERSON_2 );
}
qboolean CG_VR_OwnsHoldableIcon( void ) {
	char showBuf[16];
	char twoHandedBuf[16];
	qboolean showInHandEnabled;
	qboolean twoHandedEnabled;
	if ( !vrActive ) {
		return qfalse;
	}
	trap_Cvar_VariableStringBuffer( "vr_showItemInHand", showBuf, sizeof( showBuf ) );
	trap_Cvar_VariableStringBuffer( "vr_twoHandedWeapons", twoHandedBuf, sizeof( twoHandedBuf ) );
	showInHandEnabled = ( atof( showBuf ) != 0.0f );
	twoHandedEnabled = ( atof( twoHandedBuf ) != 0.0f );
	// the icon is owned (hidden) when the item renders in the off-hand instead
	return showInHandEnabled && !( twoHandedEnabled && vr->weapon_stabilised );
}

/*
==============================================================================

WEAPON ADJUSTMENT MODE

==============================================================================
*/

#define WEAPADJUST_NUM_PARAMS 7

static const char *weaponAdjustParamNames[WEAPADJUST_NUM_PARAMS] = {
	"scale", "right", "up", "fwd", "pitch", "yaw", "roll"
};

// Defaults matching vr_cvars.c registration
static const char *weaponAdjustDefaultStrings[] = {
	"",                                     // 0: WP_NONE
	"1,-4.0,7,-10,-20,-15,0",              // 1: Gauntlet
	"0.8,-3.0,5.5,0,0,0,0",               // 2: Machinegun
	"0.8,-3.3,8,3.7,0,0,0",               // 3: Shotgun
	"0.75,-5.4,6.5,-4,0,0,0",             // 4: Grenade Launcher
	"0.8,-5.2,6,7.5,0,0,0",               // 5: Rocket Launcher
	"0.8,-3.3,6,7,0,0,0",                 // 6: Lightning Gun
	"0.8,-5.5,6,0,0,0,0",                 // 7: Railgun
	"0.8,-4.5,6,1.5,0,0,0",               // 8: Plasma Gun
	"0.8,-5.5,6,0,0,0,0",                 // 9: BFG
	"",                                     // 10: Grappling Hook
	"0.8,-5.5,6,0,0,0,0",                 // 11: Nailgun (TA)
	"0.8,-5.5,6,0,0,0,0",                 // 12: Prox Launcher (TA)
	"0.8,-5.5,6,0,0,0,0",                 // 13: Chaingun (TA)
};
#define WEAPADJUST_NUM_DEFAULTS (sizeof(weaponAdjustDefaultStrings) / sizeof(weaponAdjustDefaultStrings[0]))

// Adjustment state (file-scoped statics — reset on level load since cgame reloads)
static int weaponAdjustParam = 0;         // 0-6: which parameter is selected
static float weaponAdjustValues[WEAPADJUST_NUM_PARAMS];
static float weaponAdjustDefaults[WEAPADJUST_NUM_PARAMS];
static int weaponAdjustWeaponId = 0;      // weapon we're currently adjusting
static qboolean weaponAdjustParamCycled = qfalse; // debounce for thumbstick param cycling
static int weaponAdjustResetHoldStart = 0; // time A button was first pressed (for hold-to-reset-all)

static void CG_WeaponAdjust_ParseValues( const char *str, float *out ) {
	out[0] = out[1] = out[2] = out[3] = out[4] = out[5] = out[6] = 0.0f;
	if ( str && strlen(str) > 0 ) {
		Q_sscanf( str, "%f,%f,%f,%f,%f,%f,%f",
			&out[0], &out[1], &out[2], &out[3],
			&out[4], &out[5], &out[6] );
	}
}

static void CG_WeaponAdjust_WriteCvar( int weaponId, float *vals ) {
	char cvar_name[64];
	char value[256];
	Com_sprintf( cvar_name, sizeof(cvar_name), "vr_weapon_adjustment_%i", weaponId );
	Com_sprintf( value, sizeof(value), "%.3f,%.3f,%.3f,%.3f,%.3f,%.3f,%.3f",
		vals[0], vals[1], vals[2], vals[3], vals[4], vals[5], vals[6] );
	trap_Cvar_Set( cvar_name, value );
}

static void CG_WeaponAdjust_LoadWeapon( int weaponId ) {
	char cvar_name[64];
	char weapon_adjustment[256];

	weaponAdjustWeaponId = weaponId;

	// Load current values from cvar
	Com_sprintf( cvar_name, sizeof(cvar_name), "vr_weapon_adjustment_%i", weaponId );
	trap_Cvar_VariableStringBuffer( cvar_name, weapon_adjustment, sizeof(weapon_adjustment) );
	CG_WeaponAdjust_ParseValues( weapon_adjustment, weaponAdjustValues );

	// Load defaults
	if ( weaponId >= 0 && weaponId < (int)WEAPADJUST_NUM_DEFAULTS ) {
		CG_WeaponAdjust_ParseValues( weaponAdjustDefaultStrings[weaponId], weaponAdjustDefaults );
	} else {
		memset( weaponAdjustDefaults, 0, sizeof(weaponAdjustDefaults) );
	}
}

void CG_WeaponAdjust_Enter( void ) {
	playerState_t *ps = &cg.snap->ps;

	// Dismiss weapon wheel and weapon stabilisation from the grip hold that got us here
	vr->weapon_select = qfalse;
	vr->weapon_stabilised = qfalse;
	cg.weaponSelectorTime = 0;

	vr->weapon_adjust = qtrue;
	weaponAdjustParam = 0;
	weaponAdjustParamCycled = qfalse;
	weaponAdjustResetHoldStart = 0;

	CG_WeaponAdjust_LoadWeapon( ps->weapon );
	CG_Printf( "Weapon adjustment mode: ON\n" );
}

void CG_WeaponAdjust_Exit( void ) {
	vr->weapon_adjust = qfalse;
	weaponAdjustResetHoldStart = 0;
	CG_Printf( "Weapon adjustment mode: OFF\n" );
}

void CG_WeaponAdjust_f( void ) {
	char enabledBuf[16];

	if ( vr->weapon_adjust ) {
		CG_WeaponAdjust_Exit();
	} else {
		trap_Cvar_VariableStringBuffer( "vr_weaponAdjust", enabledBuf, sizeof(enabledBuf) );
		if ( !vrActive || atof( enabledBuf ) == 0 ) {
			return;
		}
		if ( !cg.snap ) return;
		// Don't enter if in virtual screen, zoomed, spectator, or dead
		if ( vr->virtual_screen || vr->weapon_zoomed ||
			 cg.snap->ps.stats[STAT_HEALTH] <= 0 ||
			 cg.snap->ps.persistant[PERS_TEAM] == TEAM_SPECTATOR ) {
			CG_Printf( "Cannot enter weapon adjustment mode right now.\n" );
			return;
		}
		CG_WeaponAdjust_Enter();
	}
}

void CG_WeaponAdjustReset_f( void ) {
	if ( !vr->weapon_adjust ) return;
	weaponAdjustValues[weaponAdjustParam] = weaponAdjustDefaults[weaponAdjustParam];
	CG_WeaponAdjust_WriteCvar( weaponAdjustWeaponId, weaponAdjustValues );
}

void CG_WeaponAdjustResetAll_f( void ) {
	int i;
	const char *weaponName = "Unknown";

	if ( !vr->weapon_adjust ) return;
	for ( i = 0; i < WEAPADJUST_NUM_PARAMS; i++ ) {
		weaponAdjustValues[i] = weaponAdjustDefaults[i];
	}
	CG_WeaponAdjust_WriteCvar( weaponAdjustWeaponId, weaponAdjustValues );
	if ( cg_weapons[weaponAdjustWeaponId].item ) {
		weaponName = cg_weapons[weaponAdjustWeaponId].item->pickup_name;
	}
	CG_Printf( "Reset %s adjustments to defaults.\n", weaponName );
}

void CG_WeaponAdjustFrame( void ) {
	playerState_t *ps;
	int primaryThumb;
	int offhandThumb;
	float offhandX;
	float primaryY;
	float deadzone;
	float absY;

	if ( !vr->weapon_adjust || !cg.snap ) return;

	ps = &cg.snap->ps;

	// Auto-exit conditions
	if ( vr->virtual_screen || vr->weapon_zoomed ||
		 ps->stats[STAT_HEALTH] <= 0 ||
		 ps->persistant[PERS_TEAM] == TEAM_SPECTATOR ||
		 (ps->pm_flags & PMF_FOLLOW) ) {
		CG_WeaponAdjust_Exit();
		return;
	}

	// If weapon changed, reload values for new weapon
	if ( ps->weapon != weaponAdjustWeaponId ) {
		CG_WeaponAdjust_LoadWeapon( ps->weapon );
	}

	// Off-hand thumbstick left/right cycles params, weapon-hand thumbstick up/down adjusts value
	primaryThumb = vr->right_handed ? THUMB_RIGHT : THUMB_LEFT;
	offhandThumb = vr->right_handed ? THUMB_LEFT : THUMB_RIGHT;

	// --- Parameter cycling (off-hand thumbstick left/right) ---
	offhandX = vr->thumbstick_location[offhandThumb][0];
	if ( offhandX > 0.5f && !weaponAdjustParamCycled ) {
		weaponAdjustParam = ( weaponAdjustParam + 1 ) % WEAPADJUST_NUM_PARAMS;
		weaponAdjustParamCycled = qtrue;
	} else if ( offhandX < -0.5f && !weaponAdjustParamCycled ) {
		weaponAdjustParam = ( weaponAdjustParam + WEAPADJUST_NUM_PARAMS - 1 ) % WEAPADJUST_NUM_PARAMS;
		weaponAdjustParamCycled = qtrue;
	} else if ( offhandX > -0.3f && offhandX < 0.3f ) {
		weaponAdjustParamCycled = qfalse;
	}

	// --- Value adjustment (weapon-hand thumbstick up/down) ---
	primaryY = vr->thumbstick_location[primaryThumb][1];
	deadzone = 0.15f;
	absY = fabs( primaryY );

	if ( absY > deadzone ) {
		float normalized = ( absY - deadzone ) / ( 1.0f - deadzone );
		// Cubic curve for fine control at small deflections
		float curved = normalized * normalized * normalized;
		float direction = primaryY > 0 ? 1.0f : -1.0f;

		// Per-parameter scale factors (per second, at full deflection)
		float paramScale;
		float deltaTime;
		float delta;

		switch ( weaponAdjustParam ) {
			case 0: paramScale = 0.25f; break;  // scale
			case 1: paramScale = 5.0f; break;   // right
			case 2: paramScale = 5.0f; break;   // up
			case 3: paramScale = 5.0f; break;   // forward
			case 4: paramScale = 22.0f; break;  // pitch (degrees)
			case 5: paramScale = 22.0f; break;  // yaw (degrees)
			case 6: paramScale = 22.0f; break;  // roll (degrees)
			default: paramScale = 1.0f; break;
		}

		deltaTime = cg.frametime / 1000.0f;
		if ( deltaTime > 0.1f ) deltaTime = 0.1f; // clamp to prevent huge jumps

		delta = direction * curved * paramScale * deltaTime;
		weaponAdjustValues[weaponAdjustParam] += delta;

		// Write updated values to cvar immediately
		CG_WeaponAdjust_WriteCvar( weaponAdjustWeaponId, weaponAdjustValues );
	}
}

void CG_WeaponAdjustDraw( void ) {
	// Colors
	vec4_t bgColor        = { 0.0f, 0.0f, 0.0f, 0.65f };
	vec4_t titleColor     = { 1.0f, 1.0f, 1.0f, 1.0f };
	vec4_t activeColor    = { 1.0f, 1.0f, 0.0f, 1.0f };
	vec4_t normalColor    = { 0.7f, 0.7f, 0.7f, 1.0f };
	vec4_t changedColor   = { 0.0f, 1.0f, 1.0f, 1.0f };
	vec4_t helpColor      = { 0.5f, 0.5f, 0.5f, 1.0f };
	vec4_t activeBgColor  = { 1.0f, 1.0f, 0.0f, 0.15f };

	int charW = TINYCHAR_WIDTH;
	int charH = TINYCHAR_HEIGHT;
	int lineH = charH + 1;

	// Horizontal layout across the top — keeps the bottom clear for weapon view
	// Rows: title | arrow-up | name | value | arrow-down
	int colChars = 6;                          // each column is 6 characters wide
	int colW = colChars * charW;               // pixel width per column
	int colPad = 2;                            // pixels between columns
	float boxW = (float)(colW * WEAPADJUST_NUM_PARAMS + colPad * (WEAPADJUST_NUM_PARAMS - 1) + 8);
	float boxX = (640 - boxW) / 2;            // center horizontally
	float boxY = 110;
	float boxH = (float)(lineH * 4 + 14);     // title + arrow + name + value + arrow
	int textY = (int)boxY + 4;

	const char *weaponName = "Unknown";
	int paramStartY;
	int nameY;
	int valY;
	int i;

	if ( !vr->weapon_adjust ) return;

	// Background
	CG_FillRect( boxX, boxY, boxW, boxH, bgColor );

	// Title: weapon name
	if ( weaponAdjustWeaponId > 0 && weaponAdjustWeaponId < WP_NUM_WEAPONS ) {
		CG_RegisterWeapon( weaponAdjustWeaponId );
		if ( cg_weapons[weaponAdjustWeaponId].item ) {
			weaponName = cg_weapons[weaponAdjustWeaponId].item->pickup_name;
		}
	}
	CG_DrawStringExt( (int)boxX + 4, textY, va("ADJUST: %s", weaponName), titleColor,
		qtrue, qfalse, charW, charH, 0 );

	// Help text on the right side of the title row
	CG_DrawStringExt( (int)(boxX + boxW) - 18 * charW, textY, "A:Accept B:Default",
		helpColor, qtrue, qfalse, charW, charH, 0 );
	textY += lineH + 2;

	// Parameter rows: up-arrow | name | value | down-arrow
	paramStartY = textY;
	nameY = paramStartY + lineH;               // name row below up-arrow
	valY = nameY + lineH;                      // value row below name

	for ( i = 0; i < WEAPADJUST_NUM_PARAMS; i++ ) {
		qboolean isActive;
		qboolean isChanged;
		vec4_t *color;
		int colX;
		int arrowX;
		int nameLen;
		int nameX;
		const char *valStr;
		int valLen;
		int valX;

		isActive = ( weaponAdjustParam == i );
		isChanged = ( weaponAdjustValues[i] != weaponAdjustDefaults[i] );
		color = isActive ? &activeColor : ( isChanged ? &changedColor : &normalColor );
		colX = (int)boxX + 4 + i * ( colW + colPad );

		// Highlight background for active parameter — spans from arrow row to bottom of box
		if ( isActive ) {
			float highlightTop = (float)paramStartY - 1;
			float highlightBot = boxY + boxH;
			CG_FillRect( (float)colX - 1, highlightTop,
				(float)colW + 2, highlightBot - highlightTop, activeBgColor );
		}

		// Up/down triangle arrows on active column (charset row 8: col 7=▲, col 6=▼)
		if ( isActive ) {
			arrowX = colX + ( colW - charW ) / 2;
			trap_R_SetColor( activeColor );
			CG_DrawChar( arrowX, paramStartY, charW, charH, 135 );
			CG_DrawChar( arrowX, valY + lineH, charW, charH, 134 );
			trap_R_SetColor( NULL );
		}

		// Parameter name — right-align within column
		nameLen = (int)strlen( weaponAdjustParamNames[i] );
		nameX = colX + colW - nameLen * charW;
		CG_DrawStringExt( nameX, nameY, weaponAdjustParamNames[i],
			*color, qtrue, qfalse, charW, charH, 0 );

		// Parameter value — right-align within column
		valStr = va( "%.2f", weaponAdjustValues[i] );
		valLen = (int)strlen( valStr );
		valX = colX + colW - valLen * charW;
		CG_DrawStringExt( valX, valY, valStr,
			*color, qtrue, qfalse, charW, charH, 0 );
	}
}

/*
==============================================================================

TRANSFORMS / QUERIES

cg_drawtools.c's CG_AdjustFrom640 keystone (2D screen-space scaling), plus
the crosshair aim ray, rail muzzle origin, chat paint offsets, portrait head
angles, and standardized cvar accessors.

==============================================================================
*/

/*
=================
CG_VR_AdjustFrom640

The entire vrActive body of CG_AdjustFrom640, moved verbatim (internal bare
returns become return qtrue). The one sanctioned edit: `vr && vr->virtual_screen`
drops the vestigial `vr &&` (A4-retired idiom; vr is never NULL).
=================
*/
qboolean CG_VR_AdjustFrom640( float *x, float *y, float *w, float *h )
{
	if ( !vrActive ) {
		return qfalse;
	}
	{
		char hudBuf[16];
		int  hudDrawStatus;
		float vrXScale = cgs.glconfig.vidWidth * (1.0/640.0);
		float vrYScale = cgs.glconfig.vidHeight * (1.0/480.0);

		trap_Cvar_VariableStringBuffer( "vr_currentHudDrawStatus", hudBuf, sizeof( hudBuf ) );
		hudDrawStatus = (int)atof( hudBuf );

		//If using floating HUD and we are drawing it, double coordinates since the HUD
		//buffer is now 1280x960 instead of 640x480
		if ( hudDrawStatus == 1 && cg.drawingHUD && !cg.drawingZoomedHUD)
		{
			*x *= 2.0f;
			*y *= 2.0f;
			*w *= 2.0f;
			*h *= 2.0f;
			return qtrue;
		}

		if (cg.drawingZoomedHUD)
		{
			// Weapon zoomed HUD: scaled down and centered, similar to HUD mode 2
			// Use 1:1 square layout (640x640) to push top/bottom elements away from center
			float screenXScale = vrXScale / 2.25f;
			float screenYScale = vrXScale / 2.25f;
			float effectiveWidth = cg.refdef.width;
			float effectiveHeight = cg.refdef.height;

			// Remap Y from 480 range to 640 range (stretch vertically to fill square)
			// This maps input y=0 to output y=0, and input y=480 to output y=640
			*y = (*y / 480.0f) * 640.0f;

			// Cyclopean scope renders to the geometric framebuffer center; HUD too.

			*x *= screenXScale;
			*y *= screenYScale;
			*w *= screenXScale;
			*h *= screenYScale;

			*x += (effectiveWidth - (640 * screenXScale)) / 2.0f;
			*y += (effectiveHeight - (640 * screenYScale)) / 2.0f;
		}
		else if (!cg.drawingHUD)
		{
			float screenXScale = vrXScale;
			float screenYScale = vrYScale;
			float xOffset = 0.0f;
			float yOffset = 0.0f;

			// In virtual screen mode, scale to fit 4:3 aspect within the framebuffer
			// For ultra-wide headsets, we may be height-limited rather than width-limited
			// This ensures 2D elements are positioned correctly for the 4:3 crop
			if (vr->virtual_screen) {
				float viewableWidth, viewableHeight;
				float projCenterY;
				float opticalOffsetVirtual;
				CG_GetViewable4x3Dimensions(&viewableWidth, &viewableHeight);
				screenXScale = viewableWidth / 640.0f;
				screenYScale = viewableHeight / 480.0f;

				// Use optical center offset instead of geometric center to account for
				// asymmetric FOV (VR headsets have more down-look than up-look)
				CG_GetProjectionCenter(NULL, &projCenterY);
				opticalOffsetVirtual = projCenterY - 240.0f;
				xOffset = (cgs.glconfig.vidWidth - viewableWidth) / 2.0f;
				yOffset = (cgs.glconfig.vidHeight - viewableHeight) / 2.0f + opticalOffsetVirtual * screenYScale;
			}

			*x *= screenXScale;
			*x += xOffset;
			*y *= screenYScale;
			*y += yOffset;
			*w *= screenXScale;
			*h *= screenYScale;
		}
		else  // scale to clearly visible portion of VR screen
		{
			char  yOffBuf[16];
			float hudYOffset;
			float screenXScale, screenYScale;
			float xOffset = 0.0f;
			float yOffset = 0.0f;
			float effectiveWidth = cg.refdef.width;
			float effectiveHeight = cg.refdef.height;

			if (vr->virtual_screen) {
				screenXScale = vrXScale;
				screenYScale = vrYScale;

				// For VRFM_FIRSTPERSON gameplay, we're rendering to full framebuffer
				// but only displaying the centered 4:3 portion, so adjust scale and offset
				if (vr->first_person_following) {
					// Calculate the 4:3 safe area
					float safeWidth, safeHeight;
					float projCenterY;
					float opticalOffsetVirtual;
					CG_GetViewable4x3Dimensions(&safeWidth, &safeHeight);

					// Recalculate scale based on the visible 4:3 area, not full dimensions
					screenXScale = safeWidth / 640.0f;
					screenYScale = safeHeight / 480.0f;

					// Use safe dimensions for centering calculation
					effectiveWidth = safeWidth;
					effectiveHeight = safeHeight;

					// Use optical center offset instead of geometric center to account for
					// asymmetric FOV (VR headsets have more down-look than up-look)
					CG_GetProjectionCenter(NULL, &projCenterY);
					opticalOffsetVirtual = projCenterY - 240.0f;
					xOffset = (cgs.glconfig.vidWidth - safeWidth) / 2.0f;
					yOffset = (cgs.glconfig.vidHeight - safeHeight) / 2.0f + opticalOffsetVirtual * screenYScale;
				}
			} else {
				// HUD mode 2: scaled down for in-world display
				float projCenterY;
				float opticalOffset;
				screenXScale = vrXScale / 2.25f;
				screenYScale = (vrXScale / 2.25f);

				// Apply optical centering for HUD mode 2 (asymmetric FOV compensation)
				CG_GetProjectionCenter(NULL, &projCenterY);
				opticalOffset = projCenterY - 240.0f;
				yOffset = opticalOffset * screenYScale;
			}

			trap_Cvar_VariableStringBuffer( "vr_hudYOffset", yOffBuf, sizeof( yOffBuf ) );
			hudYOffset = atof( yOffBuf );

			*x *= screenXScale;
			*y *= screenYScale;
			*w *= screenXScale;
			*h *= screenYScale;

			*x += (effectiveWidth - (640 * screenXScale)) / 2.0f + xOffset;
			*y += (effectiveHeight - (480 * screenYScale)) / 2.0f - hudYOffset + yOffset;
		}
		return qtrue;
	}
}

/*
=================
CG_VR_CrosshairScanRay

cg_draw.c CG_ScanForCrosshairEntity's vrActive branch, verbatim: aims the
scan ray down the weapon axis instead of the view axis.
=================
*/
qboolean CG_VR_CrosshairScanRay( vec3_t start, vec3_t end ) {
	if ( !vrActive ) {
		return qfalse;
	}
	{
		vec3_t viewaxis[3];
		vec3_t weaponangles;
		CG_CalculateVRWeaponPosition(start, weaponangles);
		AnglesToAxis(weaponangles, viewaxis);
		VectorMA(start, 131072, viewaxis[0], end);
	}
	return qtrue;
}

/*
=================
CG_VR_WeaponMuzzleOrigin

cg_event.c EV_RAILTRAIL's vrActive branch, verbatim.
=================
*/
qboolean CG_VR_WeaponMuzzleOrigin( vec3_t origin, vec3_t angles ) {
	if ( !vrActive ) {
		return qfalse;
	}
	CG_CalculateVRWeaponPosition( origin, angles );
	return qtrue;
}

/*
=================
CG_VR_ChatOffsetX
CG_VR_ChatOffsetY

cg_newdraw.c chat paint offsets. Bodies are the pre-existing expressions
verbatim; both fold to 0 when the mirror is dormant (vrActive false).
=================
*/
int CG_VR_ChatOffsetX( void ) {
	return vrActive ? 200 : 0;
}

int CG_VR_ChatOffsetY( void ) {
	return (-5 * (vrActive ? vr->hmdorientation[PITCH] : 0));
}

/*
=================
CG_VR_PortraitHeadAngles

Fills 'angles' with the viewed player's real VR head orientation for the HUD
portrait, EMA-smoothing pitch and yaw to hide the ~1.42 degree steps from the
7-bit usercmd packing. Yaw is weapon-relative (added to the viewer-facing 180);
pitch is absolute world look pitch; roll comes straight from viewangles (full
precision, already interpolated). Returns qtrue when VR head data is present
(current player, followed player, or demo), qfalse otherwise so callers fall
back to the legacy idle-bob.

NOTE: EF-gated inside, deliberately NOT vrActive-gated - flatscreen hosts
must still render a VR player's real head orientation when spectating/demo
playback.
=================
*/
qboolean CG_VR_PortraitHeadAngles( vec3_t angles ) {
	playerState_t	*ps = &cg.snap->ps;
	float			targetPitch, targetYaw, alpha;

	if ( !( ps->eFlags & EF_VR_PLAYER ) ) {
		cg.vrPortraitInitialized = qfalse;
		return qfalse;
	}

	targetYaw   = 180.0f + (float)ps->stats[STAT_VR_HEAD_YAW_OFFSET] / 182.04f;
	targetPitch = (float)ps->stats[STAT_VR_HEAD_PITCH] / 182.04f;

	if ( !cg.vrPortraitInitialized || cg.nextFrameTeleport ) {
		cg.vrPortraitYaw   = targetYaw;
		cg.vrPortraitPitch = targetPitch;
		cg.vrPortraitInitialized = qtrue;
	} else {
		// tau ~30ms: smooths quantization steps without perceptible lag
		alpha = (float)cg.frametime / ( (float)cg.frametime + 30.0f );
		cg.vrPortraitYaw   += alpha * AngleSubtract( targetYaw,   cg.vrPortraitYaw );
		cg.vrPortraitPitch += alpha * AngleSubtract( targetPitch, cg.vrPortraitPitch );
	}

	angles[PITCH] = cg.vrPortraitPitch;
	angles[YAW]   = cg.vrPortraitYaw;
	angles[ROLL]  = ps->viewangles[ROLL];	// full precision, applied directly
	return qtrue;
}

/*
=================
CG_VR_CvarValue
CG_VR_CvarSet

Standardized single-value cvar read/write accessors.
=================
*/
float CG_VR_CvarValue( const char *name ) {
	char buf[16];
	trap_Cvar_VariableStringBuffer( name, buf, sizeof( buf ) );
	return atof( buf );
}

void CG_VR_CvarSet( const char *name, float value ) {
	trap_Cvar_Set( name, va( "%f", value ) );
}

/*
=================
CG_VR_FirstPersonBody
CG_VR_ShowFirstPersonBody
CG_VR_FirstPersonBodyAxes

CG_Player's first-person body. FirstPersonBody gates the pose (whether the
legs/torso axes get the HMD-yaw rotation and scale below) on
cg_firstPersonBodyScale.value > 0.0f; ShowFirstPersonBody gates the renderfx
RF_THIRD_PERSON decision on plain != 0. These are deliberately different
conditions: a negative scale must still draw the (unscaled/unrotated) normal
body today, so a single shared predicate would change behavior. FirstPerson-
BodyAxes is CG_Player's verbatim rotation block, run only when the pose
predicate is true.
=================
*/
qboolean CG_VR_FirstPersonBody( centity_t *cent ) {
	return vrActive &&
			( cent->currentState.number == cg.snap->ps.clientNum) &&
			(!cg.renderingThirdPerson) &&
			(cg_firstPersonBodyScale.value > 0.0f);
}

qboolean CG_VR_ShowFirstPersonBody( void ) {
	return vrActive && cg_firstPersonBodyScale.value != 0;
}

void CG_VR_FirstPersonBodyAxes( refEntity_t *legs, refEntity_t *torso ) {
	vec3_t bodyAngles;
	VectorClear(bodyAngles);
	bodyAngles[YAW] = cg.refdefViewAngles[YAW] + vr->hmdorientation[YAW] - vr->weaponangles[YAW];
	AnglesToAxis(bodyAngles, legs->axis);
	VectorScale( legs->axis[0], cg_firstPersonBodyScale.value, legs->axis[0] );
	VectorScale( legs->axis[1], cg_firstPersonBodyScale.value, legs->axis[1] );
	VectorScale( legs->axis[2], cg_firstPersonBodyScale.value, legs->axis[2] );
	AnglesToAxis(vec3_origin, torso->axis);
	VectorScale( torso->axis[0], cg_firstPersonBodyScale.value, torso->axis[0] );
	VectorScale( torso->axis[1], cg_firstPersonBodyScale.value, torso->axis[1] );
	VectorScale( torso->axis[2], cg_firstPersonBodyScale.value, torso->axis[2] );
	//Don't care about head
}

/*
=================
CG_VR_IsLocalClientEntity

True when cent is the local VR player's own entity, by entity number. Used
by CG_PlayerShadow/CG_PlayerSplash/CG_Player to pick the VR-specific cvar
(cg_playerShadow vs cg_shadows). NOT the clientNum-based comparison used by
CG_VR_OffHandCarriesFlag/CG_VR_SuppressOffHandItem/CG_VR_OffHandItemPose,
which route off-hand item/flag rendering and have different semantics.
=================
*/
qboolean CG_VR_IsLocalClientEntity( const centity_t *cent ) {
	return vrActive && cent->currentState.number == cg.snap->ps.clientNum;
}

/*
=================
CG_VR_SuppressOffHandItem
CG_VR_OffHandItemPose

CG_TrailItem's VR branch, split in two. SuppressOffHandItem covers the two
early-return guards (no off-hand draw during demo playback/follow mode; no
draw when the off-hand display cvar is off or the item is hidden by a
stabilised two-handed grip). OffHandItemPose fills in the off-hand-relative
ent transform when not suppressed. Both gate on the item entity being the
local VR player's own (by clientNum, matching the stock branch condition).
=================
*/
qboolean CG_VR_SuppressOffHandItem( centity_t *cent ) {
	char		showBuf[16];
	char		twoHandedBuf[16];
	qboolean	show_in_hand_enabled;
	qboolean	two_handed_enabled;

	if ( !( vrActive && cent->currentState.clientNum == vr->clientNum ) ) {
		return qfalse;
	}

	// Don't draw offhand item in demo playback or follow mode
	if (cg.demoPlayback || (cg.snap->ps.pm_flags & PMF_FOLLOW))
	{
		return qtrue;
	}

	trap_Cvar_VariableStringBuffer( "vr_showItemInHand", showBuf, sizeof( showBuf ) );
	trap_Cvar_VariableStringBuffer( "vr_twoHandedWeapons", twoHandedBuf, sizeof( twoHandedBuf ) );
	show_in_hand_enabled = ( atof( showBuf ) != 0.0f );
	two_handed_enabled = ( atof( twoHandedBuf ) != 0.0f );
	if (!show_in_hand_enabled || (two_handed_enabled && vr->weapon_stabilised))
	{
		return qtrue;
	}

	return qfalse;
}

qboolean CG_VR_OffHandItemPose( centity_t *cent, refEntity_t *ent, vec3_t offset, float scale, float horizontalScale ) {
	vec3_t angles;
	vec3_t forward, right, up;

	if ( !( vrActive && cent->currentState.clientNum == vr->clientNum ) ) {
		return qfalse;
	}

	CG_CalculateVROffHandPosition(ent->origin, angles);
	AnglesToAxis(angles, ent->axis);
	VectorScale( ent->axis[0], scale, ent->axis[0] );
	VectorScale( ent->axis[1], horizontalScale, ent->axis[1] );
	VectorScale( ent->axis[2], scale, ent->axis[2] );

	AngleVectors( angles, forward, right, up );
	VectorMA( ent->origin, offset[0], right, ent->origin );
	VectorMA( ent->origin, offset[1], forward, ent->origin );
	VectorMA( ent->origin, offset[2], up, ent->origin );
	ent->nonNormalizedAxes = qtrue;

	return qtrue;
}

/*
=================
CG_VR_DrawOffHandHoldable
CG_VR_OffHandCarriesFlag

CG_VR_DrawOffHandHoldable is CG_PlayerPowerups' former inline block, moved
wholesale: draws the active held item in the local VR player's off hand
instead of its stock world position. CG_VR_OffHandCarriesFlag says whether
cent is the local VR player carrying a CTF flag - true routes the flag
through the off-hand CG_TrailItem path instead of the shoulder pole model.
=================
*/
void CG_VR_DrawOffHandHoldable( centity_t *cent ) {
	//Player held items should render in the off-hand
	if ( vrActive && cent->currentState.number == cg.snap->ps.clientNum && !cg.renderingThirdPerson &&
	     !cg.demoPlayback && !(cg.snap->ps.pm_flags & PMF_FOLLOW) )
	{
		int		value;
		char	showBuf[16];

		value = cg.snap->ps.stats[STAT_HOLDABLE_ITEM];
		trap_Cvar_VariableStringBuffer( "vr_showItemInHand", showBuf, sizeof( showBuf ) );

		if ( value && atof( showBuf ) != 0.0f ) {
			vec3_t offset;
			CG_RegisterItemVisuals( value );
			VectorSet(offset, 0, 0, -4);
			CG_TrailItem( cent, cg_items[ value ].models[0], offset, 0.35f );
		}
	}
}

qboolean CG_VR_OffHandCarriesFlag( centity_t *cent ) {
	return vrActive && cent->currentState.clientNum == vr->clientNum;
}

/*
=================
CG_VR_FollowedPlayerSprite

CG_PlayerSprites' followed-player gate: puts a friend sprite over the head
of the player entity the local VR player is currently third-person
following, verbatim 4-term condition.
=================
*/
qboolean CG_VR_FollowedPlayerSprite( centity_t *cent ) {
	return vrActive && cent->currentState.number == cg.snap->ps.clientNum &&
			cg.renderingThirdPerson &&
			CG_VR_IsThirdPersonFollow(VRFM_QUERY);
}

/*
=================
CG_VR_PlayerHeadLerp
CG_VR_PlayerTorsoYaw
CG_VR_PlayerTorsoPitch
CG_VR_PlayerSaveAbsoluteTorso
CG_VR_PlayerHeadAngles
CG_VR_PlayerHeadReset

CG_PlayerAngles' EF_VR_PLAYER regions, moved verbatim. EF-gated inside, NOT
vrActive-gated: flatscreen hosts render VR players' heads too. PlayerHeadLerp
caches the interpolated head pitch/yaw offset from angles2 in per-entity
module storage (formerly playerEntity_t.vrHeadPitch/vrHeadYawOffset);
PlayerHeadAngles consumes it after the torso hierarchy is unwound, remapping
the world-space head pose into torso-local angles. TorsoYaw/TorsoPitch return
qtrue when the VR arm ran (torso follows weapon aim 1:1), qfalse to send the
caller down the stock swing path. SaveAbsoluteTorso snapshots torsoAngles
before AnglesSubtract makes them legs-relative - PlayerHeadAngles needs the
absolute value.

cg.predictedPlayerEntity lives outside cg_entities[] but shares its entity
number with cg_entities[clientNum], so it gets a dedicated storage slot
(index MAX_GENTITIES) - the old per-struct fields were likewise two
independent copies. PlayerHeadReset zeroes the storage; cg_snapshot.c's TV
backward-seek entity wipe calls it where its memsets used to zero the fields
with the rest of the entity state.
=================
*/
/*
================
AxisTranspose - Transpose a 3x3 rotation matrix (for orthonormal matrices, transpose = inverse)
================
*/
static void AxisTranspose(vec3_t in[3], vec3_t out[3]) {
	out[0][0] = in[0][0]; out[0][1] = in[1][0]; out[0][2] = in[2][0];
	out[1][0] = in[0][1]; out[1][1] = in[1][1]; out[1][2] = in[2][1];
	out[2][0] = in[0][2]; out[2][1] = in[1][2]; out[2][2] = in[2][2];
}

/*
================
AxisToAngles - Extract Euler angles from a rotation matrix (inverse of AnglesToAxis)
================
*/
static void AxisToAngles(vec3_t axis[3], vec3_t angles) {
	float sp = -axis[0][2];

	if (sp >= 1.0f) {
		angles[PITCH] = 90.0f;
	} else if (sp <= -1.0f) {
		angles[PITCH] = -90.0f;
	} else {
		angles[PITCH] = RAD2DEG(atan2(sp, sqrt(1.0 - sp * sp)));
	}

	if (fabs(sp) < 0.999f) {
		angles[YAW] = RAD2DEG(atan2(axis[0][1], axis[0][0]));
		angles[ROLL] = RAD2DEG(atan2(axis[1][2], axis[2][2]));
	} else {
		angles[YAW] = RAD2DEG(atan2(-axis[1][0], axis[1][1]));
		angles[ROLL] = 0.0f;
	}
}

static vrHeadOrient_t vr_headOrient[MAX_GENTITIES + 1];

static vrHeadOrient_t *CG_VR_HeadOrientFor( const centity_t *cent ) {
	if ( cent == &cg.predictedPlayerEntity ) {
		return &vr_headOrient[MAX_GENTITIES];
	}
	return &vr_headOrient[cent->currentState.number];
}

void CG_VR_PlayerHeadLerp( centity_t *cent ) {
	if ((cent->currentState.eFlags & EF_VR_PLAYER) && !(cent->currentState.eFlags & EF_DEAD)) {
		float headPitch, headYawOffset;

		if (cg.nextSnap && cent->interpolate) {
			headPitch = LerpAngle(cent->currentState.angles2[PITCH],
			                      cent->nextState.angles2[PITCH], cg.frameInterpolation);
			headYawOffset = LerpAngle(cent->currentState.angles2[ROLL],
			                          cent->nextState.angles2[ROLL], cg.frameInterpolation);
		} else {
			headPitch = cent->currentState.angles2[PITCH];
			headYawOffset = cent->currentState.angles2[ROLL];
		}

		CG_VR_HeadOrientFor( cent )->pitch = headPitch;
		CG_VR_HeadOrientFor( cent )->yawOffset = headYawOffset;
	}
}

qboolean CG_VR_PlayerTorsoYaw( centity_t *cent, const vec3_t headAngles, vec3_t torsoAngles ) {
	if ( !( cent->currentState.eFlags & EF_VR_PLAYER ) ) {
		return qfalse;
	}

	torsoAngles[YAW] = headAngles[YAW];
	cent->pe.torso.yawAngle = torsoAngles[YAW];
	cent->pe.torso.yawing = qfalse;
	return qtrue;
}

qboolean CG_VR_PlayerTorsoPitch( centity_t *cent, float dest ) {
	if ( !( cent->currentState.eFlags & EF_VR_PLAYER ) ) {
		return qfalse;
	}

	cent->pe.torso.pitchAngle = dest;
	cent->pe.torso.pitching = qfalse;
	return qtrue;
}

void CG_VR_PlayerSaveAbsoluteTorso( const centity_t *cent, const vec3_t torsoAngles, vec3_t absoluteTorsoAngles ) {
	if (cent->currentState.eFlags & EF_VR_PLAYER) {
		VectorCopy(torsoAngles, absoluteTorsoAngles);
	}
}

void CG_VR_PlayerHeadAngles( centity_t *cent, const vec3_t absoluteTorsoAngles, vec3_t headAngles ) {
	vec3_t headWorldAngles, headLocalAngles;
	vec3_t headWorldAxis[3], torsoWorldAxis[3], torsoInverseAxis[3], headLocalAxis[3];

	if (cent->currentState.eFlags & EF_VR_PLAYER) {
		headWorldAngles[PITCH] = CG_VR_HeadOrientFor( cent )->pitch;
		headWorldAngles[YAW] = cent->lerpAngles[YAW] + CG_VR_HeadOrientFor( cent )->yawOffset;
		headWorldAngles[ROLL] = cent->lerpAngles[ROLL];

		AnglesToAxis(headWorldAngles, headWorldAxis);
		AnglesToAxis(absoluteTorsoAngles, torsoWorldAxis);
		AxisTranspose(torsoWorldAxis, torsoInverseAxis);

		MatrixMultiply(headWorldAxis, torsoInverseAxis, headLocalAxis);

		AxisToAngles(headLocalAxis, headLocalAngles);

		// Apply biological limits
		headAngles[PITCH] = Com_Clamp(-80.0f, 80.0f, headLocalAngles[PITCH]);
		headAngles[YAW] = Com_Clamp(-80.0f, 80.0f, headLocalAngles[YAW]);
		headAngles[ROLL] = Com_Clamp(-60.0f, 60.0f, headLocalAngles[ROLL]);
	}
}

void CG_VR_PlayerHeadReset( void ) {
	memset( vr_headOrient, 0, sizeof( vr_headOrient ) );
}

/*
=================
CG_VR_PredictedPlayerHead

Feeds the local VR player's HMD orientation into the predicted-player
entity state so mirror/third-person-follow rendering shows the real head
pose, skipping demo playback (would replace recorded angles) and the
virtual screen (no head movement during menus). The read side is
CG_PlayerAngles' CG_VR_PlayerHeadLerp/CG_VR_PlayerHeadAngles hooks.
=================
*/
void CG_VR_PredictedPlayerHead( void ) {
	if ( !( vrActive && !cg.demoPlayback && !vr->virtual_screen ) ) {
		return;
	}

	cg.predictedPlayerEntity.currentState.angles2[PITCH] = vr->hmdorientation[PITCH];
	cg.predictedPlayerEntity.currentState.angles2[ROLL] = AngleSubtract(vr->hmdorientation[YAW], vr->weaponangles[YAW]);
	cg.predictedPlayerEntity.currentState.eFlags |= EF_VR_PLAYER;
}

/*
=================
CG_VR_DamagePlumAxis

World-oriented damage-plum digit basis for VR: axis[0] faces into the scene
to keep the basis right-handed (a mirrored/left-handed basis inverts face
culling and hides the one-sided digit shader). Normalizes dir and fills
vec/re->axis; the caller's post-block code (VectorMA(origin, pos, vec,
re->origin) and the digit render loop) only reads vec afterward, which both
this branch and the stock non-VR branch fill identically in shape - dir
itself is scratch and unread past this call.
=================
*/
qboolean CG_VR_DamagePlumAxis( refEntity_t *re, vec3_t dir, vec3_t up, vec3_t vec ) {
	if ( !vrActive ) {
		return qfalse;
	}

	VectorNormalize(dir);

	// World-oriented so digits don't roll with head tilt. axis[0] faces into the
	// scene to keep the basis right-handed; a mirrored (left-handed) basis gets
	// face culling inverted, hiding the one-sided digit shader.
	re->renderfx |= RF_WORLD_ORIENTED;
	VectorNegate(dir, re->axis[0]);
	CrossProduct(dir, up, vec);
	VectorNormalize(vec);
	VectorCopy(vec, re->axis[1]);
	CrossProduct(vec, dir, re->axis[2]);  // up, from forward and left

	return qtrue;
}

/*
=================
CG_VR_MenuYaw
CG_VR_LockMenuYaw
CG_VR_UnlockMenuYaw
CG_VR_SetScoreboardCursor
CG_VR_ScoreboardCursor
CG_VR_SetVoteActive
CG_VR_VoteHolding
CG_VR_MenuPointerYaw

Trinity-feature VR state shared by the TV-scrub pointer, the spectator
scoreboard cursor, and hold-to-vote: cg_consolecmds.c's CG_TVScrubDown_f/
CG_TVScrubUp_f/CG_TVScrubCancel_f, cg_main.c's CG_SetScoreCatcher/
CG_KeyEvent/CG_MouseEvent, cg_newdraw.c's CG_KeyEvent/CG_MouseEvent, and
cg_draw.c's scoreboard/TV-timeline/vote draws. CG_VR_MenuPointerYaw is
CG_TVScrubDown_f's former inline hand-selection + controller-yaw read,
moved here verbatim.
=================
*/
float CG_VR_MenuYaw( void ) {
	return vrActive ? vr->menuYaw : 0.0f;
}

void CG_VR_LockMenuYaw( float yaw ) {
	if ( !vrActive ) {
		return;
	}
	vr->menuYaw = yaw;
	vr->menuYawLocked = qtrue;
}

void CG_VR_UnlockMenuYaw( float yaw ) {
	if ( !vrActive ) {
		return;
	}
	vr->menuYawLocked = qfalse;
	vr->menuYaw = yaw;
}

void CG_VR_SetScoreboardCursor( qboolean active ) {
	if ( !vrActive ) {
		return;
	}
	vr->scoreboardCursorActive = active;
}

qboolean CG_VR_ScoreboardCursor( float *x, float *y ) {
	if ( !vrActive || !vr->scoreboardCursorActive ) {
		return qfalse;
	}
	*x = vr->scoreboardCursorX;
	*y = vr->scoreboardCursorY;
	return qtrue;
}

void CG_VR_SetVoteActive( qboolean active ) {
	if ( !vrActive ) {
		return;
	}
	vr->vote_active = active;
}

int CG_VR_VoteHolding( void ) {
	return vrActive ? vr->vote_holding : 0;
}

float CG_VR_MenuPointerYaw( void ) {
	float controllerYaw;

	if ( !vrActive ) {
		return 0.0f;
	}

	if ( vr->menuLeftHanded ) {
		controllerYaw = vr->right_handed ? vr->offhandangles[YAW] : vr->weaponangles[YAW];
	} else {
		controllerYaw = vr->right_handed ? vr->weaponangles[YAW] : vr->offhandangles[YAW];
	}

	return controllerYaw;
}
