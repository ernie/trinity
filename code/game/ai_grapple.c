// code/game/ai_grapple.c
//
// See ai_grapple.h. The grapple maneuvers a bot chooses for itself from live
// game state, never from a route.

#include "g_local.h"
#include "bg_grapple_model.h"
#include "bg_mode.h"
#include "botlib.h"
#include "be_aas.h"
#include "be_ea.h"
#include "be_ai_char.h"
#include "be_ai_chat.h"
#include "be_ai_gen.h"
#include "be_ai_goal.h"
#include "be_ai_move.h"
#include "be_ai_weap.h"
#include "ai_main.h"
#include "ai_dmq3.h"
#include "ai_chat.h"
#include "ai_cmd.h"
#include "ai_dmnet.h"
#include "ai_team.h"
#include "ai_grapple.h"
#include "chars.h"
#include "inv.h"
#include "syn.h"

#define GRAPPLE_MODE_NONE			0
#define GRAPPLE_MODE_YANK			1
#define GRAPPLE_MODE_RIDE			2
#define GRAPPLE_MODE_SAVE			3
#define GRAPPLE_MODE_SPEED			4
#define GRAPPLE_SPEED_MINDIST		600
#define GRAPPLE_SPEED_MAXDIST		1200
#define GRAPPLE_SPEED_PITCH			30		//degrees above the leg the anchor is sought
#define GRAPPLE_SPEED_HOLD			0.5		//seconds of pull before letting go
#define GRAPPLE_SPEED_MAXHOLD		2.0
#define GRAPPLE_SPEED_RELEASE		500		//horizontal speed worth keeping
#define GRAPPLE_ALIGN_SPEED			6.0
#define GRAPPLE_YANK_MINDIST		400
#define GRAPPLE_YANK_MAXDIST		1200
#define GRAPPLE_YANK_CLOSEDIST		200
#define GRAPPLE_YANK_FIREWINDOW		2.0		//draw + align before giving up
#define GRAPPLE_YANK_FLIGHTWINDOW	1.0		//post-launch attach window
#define GRAPPLE_YANK_HOLDTIME		3.5
#define GRAPPLE_RIDE_MINDIST		100
#define GRAPPLE_RIDE_MAXDIST		1600
#define GRAPPLE_RIDE_HOLDTIME		4.0
#define GRAPPLE_SAVE_RANGE			1200
#define GRAPPLE_SAVE_YAWS			8
#define GRAPPLE_SAVE_PITCHES		5
#define GRAPPLE_SAVE_LIPS			12		//nav-derived deck-edge candidates
#define GRAPPLE_SAVE_AREAS			160		//a void map's box is mostly air columns, and the
											//grounded areas must survive the enumeration cap
#define GRAPPLE_SAVE_CANDIDATES		(GRAPPLE_SAVE_YAWS * GRAPPLE_SAVE_PITCHES + GRAPPLE_SAVE_LIPS)
#define GRAPPLE_FALL_FRAMES			25		//2.5s at 0.1s steps outfalls any real drop
#define GRAPPLE_BURST_DAMAGE		25
#define GRAPPLE_COOLDOWN			8.0
#define GRAPPLE_COOLDOWN_MIN		1.5		//what an appetite of 1.0 buys
#define GRAPPLE_SNOOZE				2.0
#define GRAPPLE_WHIFF_SLACK			150
#define GRAPPLE_ALIGN_YANK			8.0		//a strafing shooter and a moving target rarely converge tighter
#define GRAPPLE_ALIGN_RIDE			4.0		//slop here anchors the side instead of the top
#define GRAPPLE_ALIGN_SAVE			10.0	//the bot slews the turn for real; the firing snap only closes this
											//sliver, and a falling body's ideal drifts too fast for a tighter one
#define GRAPPLE_UNSAFE_GRACE		1.5
#define GRAPPLE_MAXHOLD				12.0	//only a lethal release keeps the hold past its grace
#define GRAPPLE_MANTLE_SETTLE		0.2		//one more think than the hang needs to stop moving
#define GRAPPLE_MANTLE_BUDGET		1.0
#define GRAPPLE_MANTLE_PITCH_MAX	85		//the flattened forward vector stops pressing near 90
#define GRAPPLE_MANTLE_CLIP			(MASK_PLAYERSOLID|CONTENTS_BOTCLIP)

//how far under a prediction's unresolved end a floor must lie for the fall
//to count as going somewhere; past this the trajectory is descending onto
//nothing, which is the void
#define GRAPPLE_VOID_FLOOR_REACH	2048

#define GRAPPLE_ARC_LETHAL			0		//void, lava or slime
#define GRAPPLE_ARC_PAINFUL			1		//lands, costs health
#define GRAPPLE_ARC_SAFE			2		//lands free
#define GRAPPLE_ARC_UNSCORED		3		//pad or teleporter: survivable, but the landing area means nothing
#define GRAPPLE_ARC_UNRESOLVED		4		//never landed inside the horizon and not provably void: readers keep their current state

//steering assumed through a flight; a release decision uses 0 instead
#define GRAPPLE_AIRCONTROL			400

/*
==================
BotGrappleAvailable

The admin switch AND actual possession: g_grapple decides who the server hands
a hook to, but a bot that has one for another reason is still entitled to use
it, and one that lacks it can do nothing with a grapple reachability but shoot
the wall.

Entry gates use this; the live-state drivers deliberately do not, since a tow
already running rides out through its own cleanup.
==================
*/
qboolean BotGrappleAvailable(bot_state_t *bs) {
	return (bot_grapple.integer && bs->inventory[INVENTORY_GRAPPLINGHOOK]);
}

/*
==================
BotTacticalGrappleActive
==================
*/
int BotTacticalGrappleActive(bot_state_t *bs) {
	return bs->grapplemode != GRAPPLE_MODE_NONE;
}

/*
==================
BotGrappleRouteAvailable

Whether the router may plan grapple travel. Not while a maneuver owns the
hook: BotTravel_Grapple would read the tactical pull as its own tow and arm
a route release window on it.
==================
*/
qboolean BotGrappleRouteAvailable(bot_state_t *bs) {
	return BotGrappleAvailable(bs) && !BotTacticalGrappleActive(bs);
}

/*
==================
BotGrappleCooldown

Appetite reads as frequency; the dice roll it replaces skipped good chances
for no visible reason. The jitter keeps the old 8-12s span at appetite 0.
==================
*/
static float BotGrappleCooldown(bot_state_t *bs) {
	float user;

	user = trap_Characteristic_BFloat(bs->character, CHARACTERISTIC_GRAPPLE_USER, 0, 1);
	if (user < 0) user = 0;
	if (user > 1) user = 1;
	return (GRAPPLE_COOLDOWN - user * (GRAPPLE_COOLDOWN - GRAPPLE_COOLDOWN_MIN))
			* (1.0f + 0.5f * random());
}

static qboolean BotGrappleMantleUp(bot_state_t *bs, playerState_t *ps);
static void BotGrappleMantleSpend(bot_state_t *bs, qboolean ok);
static void BotGrappleMantleRefuse(bot_state_t *bs, const char *why, int detail);

/*
==================
BotTacticalGrappleBegin
==================
*/
static void BotTacticalGrappleBegin(bot_state_t *bs, int mode) {
	bs->grapplemode = mode;
}

/*
==================
BotTacticalGrappleEnd
==================
*/
static void BotTacticalGrappleEnd(bot_state_t *bs, qboolean whiff) {
	//a mantle still running when the mode ends ended with it, whatever ended it
	if (bs->grapplemantle_time > 0) BotGrappleMantleSpend(bs, BotGrappleMantleUp(bs, &bs->cur_ps));
	bs->grapplemode = GRAPPLE_MODE_NONE;
	bs->grapplehookent = 0;
	bs->grapplepull_time = 0;
	bs->grapplemantle_time = 0;
	bs->grapplemantle_deckz = 0;
	bs->grapplesettle_time = 0;
	bs->grapplevault_time = 0;
	bs->grappleretarget_time = 0;
	//a whiff never engaged anything, so let the bot try again soon
	bs->grapplenext_time = FloatTime() + (whiff ? GRAPPLE_SNOOZE : BotGrappleCooldown(bs));
}

/*
==================
BotGrapplePredictArc

The shared prediction: same AAS call, stop mask, frame count and timestep
for any origin/velocity pair, so two calls stay directly comparable.

aircontrol is how hard the bot is assumed to steer through the flight, on the
EA 0-400 stick scale. A release DECISION passes 0: it commits, and a landing
promised by steering the bot may not actually do costs more than the ride it
replaced.
==================
*/
static int BotGrapplePredictArc(bot_state_t *bs, vec3_t origin, vec3_t velocity,
								float aircontrol, aas_clientmove_t *move) {
	vec3_t cmdmove;

	VectorCopy(velocity, cmdmove);
	cmdmove[2] = 0;
	VectorNormalize(cmdmove);
	VectorScale(cmdmove, aircontrol, cmdmove);
	trap_AAS_PredictClientMovement(move, bs->entitynum, origin,
			PRESENCE_NORMAL, qfalse, velocity, cmdmove, GRAPPLE_FALL_FRAMES,
			GRAPPLE_FALL_FRAMES, 0.1f,
			SE_HITGROUND|SE_HITGROUNDDAMAGE|SE_ENTERWATER|SE_ENTERSLIME
				|SE_ENTERLAVA|SE_TOUCHJUMPPAD|SE_TOUCHTELEPORTER,
			0, qfalse);
	if (move->stopevent & (SE_ENTERSLIME|SE_ENTERLAVA)) return GRAPPLE_ARC_LETHAL;
	//prediction can't carry a pad's boost, so the landing it reports is not the real one
	if (move->stopevent & (SE_TOUCHJUMPPAD|SE_TOUCHTELEPORTER)) return GRAPPLE_ARC_UNSCORED;
	if (move->stopevent & (SE_HITGROUNDDAMAGE|SE_HITGROUND|SE_ENTERWATER)) {
		aas_areainfo_t landinfo;
		int landarea;

		//a landing return preempts the prediction's own contents check, and
		//the kill volume over a pit floor is lava in the nav data: a body
		//that stops inside it has not landed, it has died
		landarea = trap_AAS_PointAreaNum(move->endpos);
		if (landarea) {
			trap_AAS_AreaInfo(landarea, &landinfo);
			if (landinfo.contents & (AREACONTENTS_LAVA|AREACONTENTS_SLIME)) {
				if (bot_grapple.integer >= 2) {
					G_Printf("GRAPPLE-ARC c%d lethal-floor area %d\n", bs->client, landarea);
				}
				return GRAPPLE_ARC_LETHAL;
			}
		}
		if (move->stopevent & SE_HITGROUNDDAMAGE) return GRAPPLE_ARC_PAINFUL;
		return GRAPPLE_ARC_SAFE;
	}
	//nothing reached within the horizon. The kill volume under a void is an
	//entity the prediction cannot see, so the void is read off what lies
	//under the trajectory's end: descending onto nothing, or onto sky, is
	//the void; the rest is UNRESOLVED, which every reader answers by
	//keeping the state it is in
	if (move->velocity[2] < 0) {
		bsp_trace_t downtr;
		vec3_t below;

		VectorCopy(move->endpos, below);
		below[2] -= GRAPPLE_VOID_FLOOR_REACH;
		BotAI_Trace(&downtr, move->endpos, NULL, NULL, below, bs->entitynum, MASK_PLAYERSOLID);
		if (downtr.fraction >= 1.0f || (downtr.surface.flags & SURF_SKY)) {
			return GRAPPLE_ARC_LETHAL;
		}
	}
	return GRAPPLE_ARC_UNRESOLVED;
}

/*
==================
BotGrappleArc

Where the arc actually ends if we let go now. One prediction answers
survivability, landing cost and destination together.

Ballistic, with no steering assumed. Over a void, 400 units of assumed air
control walk the prediction up to 320ups sideways for the whole 2.5s horizon
and find it a ledge, so a drop that ends in nothing reads as a landing.
==================
*/
static int BotGrappleArc(bot_state_t *bs, aas_clientmove_t *move) {
	return BotGrapplePredictArc(bs, bs->origin, bs->cur_ps.velocity, 0, move);
}

/*
==================
BotGrappleArcBreaksLOS

Getting shot is not a reason to stop going somewhere; reaching cover is.
==================
*/
static qboolean BotGrappleArcBreaksLOS(bot_state_t *bs, aas_clientmove_t *move) {
	bsp_trace_t bsptr;
	vec3_t from, to;
	int attacker;

	attacker = g_entities[bs->client].client->lasthurt_client;
	if (attacker == ENTITYNUM_NONE) return qfalse;
	if (attacker < 0 || attacker >= MAX_CLIENTS || attacker == bs->client) return qfalse;
	if (!g_entities[attacker].inuse || !g_entities[attacker].client) return qfalse;
	VectorCopy(move->endpos, from);
	from[2] += bs->cur_ps.viewheight;
	VectorCopy(g_entities[attacker].client->ps.origin, to);
	to[2] += g_entities[attacker].client->ps.viewheight;
	BotAI_Trace(&bsptr, from, NULL, NULL, to, bs->entitynum, MASK_SHOT);
	return (bsptr.fraction < 1 && bsptr.ent != attacker);
}

/*
==================
BotGrappleArcSeesEnemy

Whether the enemy is still in view from where the arc lands.
==================
*/
static qboolean BotGrappleArcSeesEnemy(bot_state_t *bs, aas_clientmove_t *move, int enemy) {
	bsp_trace_t bsptr;
	vec3_t from, to;

	if (enemy < 0 || enemy >= MAX_CLIENTS) return qfalse;
	if (!g_entities[enemy].inuse || !g_entities[enemy].client) return qfalse;
	VectorCopy(move->endpos, from);
	from[2] += bs->cur_ps.viewheight;
	VectorCopy(g_entities[enemy].client->ps.origin, to);
	to[2] += g_entities[enemy].client->ps.viewheight;
	BotAI_Trace(&bsptr, from, NULL, NULL, to, bs->entitynum, MASK_SHOT);
	return (bsptr.fraction >= 1 || bsptr.ent == enemy);
}

/*
==================
BotGrappleEnemyFallDoomed

Whether the enemy's own fall ends somewhere lethal. A body headed into the
void is no anchor and no target: a yank at it pulls the shooter after it.
==================
*/
static qboolean BotGrappleEnemyFallDoomed(bot_state_t *bs, int enemy) {
	aas_clientmove_t move;
	gentity_t *e;

	if (enemy < 0 || enemy >= MAX_CLIENTS) return qfalse;
	e = &g_entities[enemy];
	if (!e->inuse || !e->client) return qfalse;
	if (e->client->ps.groundEntityNum != ENTITYNUM_NONE) return qfalse;
	return BotGrapplePredictArc(bs, e->client->ps.origin, e->client->ps.velocity, 0, &move)
			== GRAPPLE_ARC_LETHAL;
}

/*
==================
BotGrappleFallWouldKill

A painful landing is a fixed 10 damage bite armor cannot soak, so it only
matters to a body that cannot pay it.
==================
*/
static qboolean BotGrappleFallWouldKill(bot_state_t *bs) {
	return bs->inventory[INVENTORY_HEALTH] <= 10;
}

/*
==================
BotGrappleReleaseSafe

Voluntary releases only happen where landing can't kill: fall damage is
capped, so only void, lava, or slime are lethal.
==================
*/
static qboolean BotGrappleReleaseSafe(bot_state_t *bs) {
	aas_clientmove_t move;

	int arc;

	if (bs->cur_ps.groundEntityNum != ENTITYNUM_NONE) return qtrue;
	arc = BotGrappleArc(bs, &move);
	//this function's word is a promise, and an UNRESOLVED read makes none:
	//a holder asking whether letting go is fine keeps holding
	if (arc == GRAPPLE_ARC_LETHAL || arc == GRAPPLE_ARC_UNRESOLVED) return qfalse;
	//a save holds the bar its entry set: a landing that would finish the bot
	//is not the danger passing; standing down on it hands the fall back and
	//the entry retakes it next think
	if (arc == GRAPPLE_ARC_PAINFUL && bs->grapplemode == GRAPPLE_MODE_SAVE
			&& BotGrappleFallWouldKill(bs)) return qfalse;
	return qtrue;
}

/*
==================
BotGrappleAimDir
==================
*/
static void BotGrappleAimDir(bot_state_t *bs, vec3_t aimpoint) {
	vec3_t dir;
	int i;
	float aim_accuracy;

	VectorSubtract(aimpoint, bs->eye, dir);
	VectorNormalize(dir);
	aim_accuracy = trap_Characteristic_BFloat(bs->character, CHARACTERISTIC_AIM_ACCURACY, 0, 1);
	if (aim_accuracy < 0.8f) {
		for (i = 0; i < 3; i++) dir[i] += 0.3f * crandom() * (1 - aim_accuracy);
	}
	vectoangles(dir, bs->ideal_viewangles);
}

/*
==================
BotPredictEnemyPoint

Where the enemy will be when a projectile arrives. A tethered enemy is easy:
the tow overwrites velocity every frame, so the path is straight, gravity-free.
==================
*/
static void BotPredictEnemyPoint(bot_state_t *bs, aas_entityinfo_t *entinfo,
								float projspeed, float aim_skill, vec3_t aimpoint) {
	aas_clientmove_t move;
	vec3_t dir, cmdmove, origin;
	float dist, t;

	VectorCopy(entinfo->origin, aimpoint);
	if (projspeed <= 0 || entinfo->update_time <= 0) return;
	//under the rocket's own threshold the shot goes unled, same as any projectile
	if (aim_skill <= 0.4) return;
	VectorSubtract(entinfo->origin, bs->eye, dir);
	dist = VectorLength(dir);
	t = dist / projspeed;
	VectorSubtract(entinfo->origin, entinfo->lastvisorigin, dir);
	VectorScale(dir, 1.0f / entinfo->update_time, dir);
	//a tethered enemy flies straight and the vertical is the whole point, so
	//neither the gravity-bound predictor nor the flattened lead applies
	if (entinfo->number >= 0 && entinfo->number < MAX_CLIENTS
			&& g_entities[entinfo->number].client
			&& (g_entities[entinfo->number].client->ps.pm_flags & PMF_GRAPPLE_PULL)) {
		VectorMA(aimpoint, t, dir, aimpoint);
		return;
	}
	if (aim_skill > 0.8) {
		VectorCopy(entinfo->origin, origin);
		origin[2] += 1;
		VectorClear(cmdmove);
		trap_AAS_PredictClientMovement(&move, entinfo->number, origin,
									PRESENCE_CROUCH, qfalse, dir, cmdmove, 0,
									(int)(t * 10), 0.1f, 0, 0, qfalse);
		VectorCopy(move.endpos, aimpoint);
		return;
	}
	//the linear lead is flattened: vertical velocity reverses under gravity
	//well inside a flight time
	dir[2] = 0;
	VectorMA(aimpoint, t, dir, aimpoint);
}

/*
==================
BotGrappleEnemyAimPoint

Split from the aiming so a decision can test the shot without committing the
view to it.
==================
*/
static void BotGrappleEnemyAimPoint(bot_state_t *bs, aas_entityinfo_t *entinfo, vec3_t aimpoint) {
	float aim_skill;

	//the hook has no per-weapon aim characteristic, so the generic one governs
	aim_skill = trap_Characteristic_BFloat(bs->character, CHARACTERISTIC_AIM_SKILL, 0, 1);
	BotPredictEnemyPoint(bs, entinfo, GRAPPLE_MODEL_FIRE_SPEED, aim_skill, aimpoint);
}

/*
==================
BotGrappleAimAtEnemy
==================
*/
static void BotGrappleAimAtEnemy(bot_state_t *bs, aas_entityinfo_t *entinfo, vec3_t aimpoint) {
	BotGrappleEnemyAimPoint(bs, entinfo, aimpoint);
	BotGrappleAimDir(bs, aimpoint);
}

static int BotWeaponDPSAt(const modeConfig_t *cfg, int w, float dist);

/*
==================
BotYankMatchup

Pulling yourself onto a better-armed opponent is a gift. Yank when the
close-range trade favors the bot, or its health and armor cover the trip.
==================
*/
static qboolean BotYankMatchup(bot_state_t *bs, aas_entityinfo_t *entinfo) {
	const modeConfig_t *cfg;
	gentity_t *e;
	int w, mine, theirs, dps;

	cfg = Mode_GetConfig(g_mode.integer);
	mine = 0;
	for (w = WP_GAUNTLET; w < WP_NUM_WEAPONS; w++) {
		if (w == WP_GRAPPLING_HOOK) continue;
		if (!(bs->cur_ps.stats[STAT_WEAPONS] & (1 << w))) continue;
		if (bs->cur_ps.ammo[w] == 0) continue;
		dps = BotWeaponDPSAt(cfg, w, GRAPPLE_YANK_CLOSEDIST);
		if (dps > mine) mine = dps;
	}
	theirs = (entinfo->weapon > WP_NONE && entinfo->weapon < WP_NUM_WEAPONS)
		? BotWeaponDPSAt(cfg, entinfo->weapon, GRAPPLE_YANK_CLOSEDIST) : 0;
	if (mine >= theirs) return qtrue;
	//outgunned, so only a lead the trip cannot erase pays for the pull
	if (entinfo->number < 0 || entinfo->number >= MAX_CLIENTS) return qfalse;
	e = &g_entities[entinfo->number];
	if (!e->inuse || !e->client) return qfalse;
	return bs->inventory[INVENTORY_HEALTH] + bs->inventory[INVENTORY_ARMOR]
		>= e->health + e->client->ps.stats[STAT_ARMOR] + 50;
}

/*
==================
BotCheckTacticalGrapple

Yank consideration, fight node only.
==================
*/
void BotCheckTacticalGrapple(bot_state_t *bs, aas_entityinfo_t *entinfo) {
	bsp_trace_t bsptr;
	vec3_t dir, aimpoint;
	float dist;

	if (!BotGrappleAvailable(bs)) return;
	if (bs->grapplemode != GRAPPLE_MODE_NONE) return;
	if (bs->settings.skill < 3) return;
	if (bs->enemy < 0 || bs->enemy >= MAX_CLIENTS) return;
	if (bs->grapplenext_time > FloatTime()) return;
	if (g_entities[bs->client].client->hook) return;
	if (bs->cur_ps.pm_flags & PMF_GRAPPLE_PULL) return;
	//a yank is a reposition taken from footing: fired mid-air it hijacks
	//whatever flight the body is finishing
	if (bs->cur_ps.groundEntityNum == ENTITYNUM_NONE) return;
	VectorSubtract(entinfo->origin, bs->origin, dir);
	dist = VectorLength(dir);
	if (dist < GRAPPLE_YANK_MINDIST || dist > GRAPPLE_YANK_MAXDIST) return;
	if (!BotYankMatchup(bs, entinfo)) return;
	//a target already falling to its death is no anchor: the pull follows it
	if (BotGrappleEnemyFallDoomed(bs, bs->enemy)) return;
	//test against the aim point itself, not the enemy. Skill decides how much
	//it leads, and a visible enemy can still sit behind where that point lands
	BotGrappleEnemyAimPoint(bs, entinfo, aimpoint);
	BotAI_Trace(&bsptr, bs->eye, NULL, NULL, aimpoint, bs->entitynum, MASK_SHOT);
	if (bsptr.fraction < 1 && bsptr.ent != bs->enemy) {
		bs->grapplenext_time = FloatTime() + GRAPPLE_SNOOZE;
		return;
	}
	//a yank pulls the SHOOTER, and a pull that ends early (the enemy dies,
	//the hook comes loose) drops the bot mid-crossing. The early break is
	//the dangerous one, so read the drop from a point in each half of the
	//flight at tow speed; ground on both is worth a fight
	{
		static const float yankbreak[2] = { 0.35f, 0.75f };
		aas_clientmove_t yankmove;
		vec3_t at, yankdir, yankvel;
		int yi;

		VectorSubtract(aimpoint, bs->origin, yankdir);
		VectorNormalize(yankdir);
		VectorScale(yankdir, GRAPPLE_MODEL_TOWSPEED, yankvel);
		for (yi = 0; yi < 2; yi++) {
			int yankarc;

			VectorMA(bs->origin, yankbreak[yi] * dist, yankdir, at);
			yankarc = BotGrapplePredictArc(bs, at, yankvel, 0, &yankmove);
			if (yankarc == GRAPPLE_ARC_LETHAL || yankarc == GRAPPLE_ARC_UNRESOLVED) {
				bs->grapplenext_time = FloatTime() + GRAPPLE_SNOOZE;
				return;
			}
		}
	}
	BotTacticalGrappleBegin(bs, GRAPPLE_MODE_YANK);
	//the gate closes here but BotTacticalGrappleFrame doesn't run until after the node,
	//so claim the slot now or the node's own consumers read whatever it held
	bs->weaponnum = WP_GRAPPLING_HOOK;
	bs->grappleent = bs->enemy;
	bs->grapplehookent = 0;
	bs->grapplestart_time = FloatTime();
	bs->grapplepull_time = 0;
}

/*
==================
BotGrappleMoverAimPoint

Anchors on the top face only: an underside hook is a hang, not a ride. The
face test also decides when a ride is possible at all, since the bot can only
see a top it is already above.
==================
*/
static qboolean BotGrappleMoverAimPoint(bot_state_t *bs, int entnum, vec3_t aimpoint) {
	gentity_t *e;
	trace_t tr;
	vec3_t top, pred, delta;
	float dist, t;

	if (entnum < MAX_CLIENTS || entnum >= level.num_entities) return qfalse;
	e = &g_entities[entnum];
	if (!e->inuse || e->s.eType != ET_MOVER) return qfalse;
	if (e->s.pos.trType == TR_STATIONARY && e->s.apos.trType == TR_STATIONARY) return qfalse;
	//just under the top face, so a descending ray terminates on the surface to stand on
	top[0] = (e->r.absmin[0] + e->r.absmax[0]) * 0.5f;
	top[1] = (e->r.absmin[1] + e->r.absmax[1]) * 0.5f;
	top[2] = e->r.absmax[2] - 1;
	//validate against the pose the trace can see, then lead the shot from that point
	trap_Trace(&tr, bs->eye, NULL, NULL, top, bs->entitynum, MASK_SHOT);
	if (tr.startsolid || tr.allsolid) return qfalse;
	if (tr.entityNum != entnum) return qfalse;
	if (tr.plane.normal[2] < 0.7f) return qfalse;
	dist = Distance(bs->eye, top);
	t = dist / GRAPPLE_MODEL_FIRE_SPEED;
	BG_EvaluateTrajectory(&e->s.pos, level.time + (int)(t * 1000), pred);
	VectorSubtract(pred, e->r.currentOrigin, delta);
	VectorAdd(top, delta, aimpoint);
	return qtrue;
}

/*
==================
BotFindGrappleMover

The route already picked the mover; this just resolves it to an entity, so
nearest-with-a-standable-top is enough.
==================
*/
static int BotFindGrappleMover(bot_state_t *bs) {
	int i, best;
	float dist, bestdist;
	vec3_t center, aimpoint;
	gentity_t *e;

	best = -1;
	bestdist = GRAPPLE_RIDE_MAXDIST + 1;
	for (i = MAX_CLIENTS; i < level.num_entities; i++) {
		e = &g_entities[i];
		if (!e->inuse || e->s.eType != ET_MOVER) continue;
		if (e->s.pos.trType == TR_STATIONARY && e->s.apos.trType == TR_STATIONARY) continue;
		VectorAdd(e->r.absmin, e->r.absmax, center);
		VectorScale(center, 0.5f, center);
		dist = Distance(bs->eye, center);
		if (dist < GRAPPLE_RIDE_MINDIST || dist > GRAPPLE_RIDE_MAXDIST) continue;
		if (dist >= bestdist) continue;
		if (!BotGrappleMoverAimPoint(bs, i, aimpoint)) continue;
		bestdist = dist;
		best = i;
	}
	return best;
}

/*
==================
BotCheckGrappleRide

Boards a mover the route already runs through, instead of standing at the
boarding point waiting for it. Riding what botlib chose is goal-directed for
free, and lands the bot where the reachability expects it.
==================
*/
void BotCheckGrappleRide(bot_state_t *bs, bot_moveresult_t *moveresult) {
	int mover;

	if (!BotGrappleAvailable(bs)) return;
	if (bs->grapplemode != GRAPPLE_MODE_NONE) return;
	if (bs->settings.skill < 3) return;
	if (bs->grapplenext_time > FloatTime()) return;
	if (g_entities[bs->client].client->hook) return;
	if (bs->cur_ps.pm_flags & PMF_GRAPPLE_PULL) return;
	//the one state worth shortcutting: parked at the boarding point, mover away
	if (!(moveresult->flags & MOVERESULT_WAITING)) return;
	if (moveresult->type != RESULTTYPE_WAITFORFUNCBOBBING) return;
	//don't rescan every think
	bs->grapplenext_time = FloatTime() + GRAPPLE_SNOOZE;
	mover = BotFindGrappleMover(bs);
	if (mover < 0) return;
	BotTacticalGrappleBegin(bs, GRAPPLE_MODE_RIDE);
	//claim the weapon slot before the node's consumers read it
	bs->weaponnum = WP_GRAPPLING_HOOK;
	bs->grappleent = mover;
	bs->grapplehookent = 0;
	bs->grapplestart_time = FloatTime();
	bs->grapplepull_time = 0;
}

/*
==================
BotCheckGrappleSpeed

A player on a long straight run hooks something ahead and above and lets go
with the speed. Seek_LTG only: the leg has to be the route's, not a fight's.
==================
*/
void BotCheckGrappleSpeed(bot_state_t *bs, bot_moveresult_t *moveresult) {
	static const vec3_t up = { 0, 0, 1 };
	vec3_t dir, end, hang, vel;
	bsp_trace_t bsptr;
	aas_clientmove_t move;
	float dist;

	//a leg is only a leg while it is being walked: a gap in the calls means
	//the bot was doing something else, and the heading it left proves nothing
	if (FloatTime() > bs->legcheck_time + 0.25f) bs->legstart_time = 0;
	bs->legcheck_time = FloatTime();
	//track the run: a direction held for half a second is a leg
	if (VectorLengthSquared(moveresult->movedir) < 0.01f) {
		bs->legstart_time = 0;
		return;
	}
	VectorCopy(moveresult->movedir, dir);
	dir[2] = 0;
	VectorNormalize(dir);
	if (!bs->legstart_time || DotProduct(dir, bs->legdir) < 0.94f) {	//20 degrees
		VectorCopy(dir, bs->legdir);
		bs->legstart_time = FloatTime();
	}
	if (!BotGrappleAvailable(bs)) return;
	if (bs->grapplemode != GRAPPLE_MODE_NONE) return;
	if (bs->settings.skill < 4) return;
	if (bs->grapplenext_time > FloatTime()) return;
	if (g_entities[bs->client].client->hook) return;
	if (bs->cur_ps.pm_flags & PMF_GRAPPLE_PULL) return;
	if (bs->cur_ps.groundEntityNum == ENTITYNUM_NONE) return;
	if (FloatTime() < bs->legstart_time + 0.5f) return;
	//an anchor ahead and above the leg
	VectorScale(bs->legdir, cos(DEG2RAD(GRAPPLE_SPEED_PITCH)), dir);
	VectorMA(dir, sin(DEG2RAD(GRAPPLE_SPEED_PITCH)), up, dir);
	VectorMA(bs->eye, GRAPPLE_SPEED_MAXDIST, dir, end);
	BotAI_Trace(&bsptr, bs->eye, NULL, NULL, end, bs->entitynum, MASK_SHOT);
	if (bsptr.fraction >= 1 || (bsptr.surface.flags & SURF_SKY)) return;
	dist = GRAPPLE_SPEED_MAXDIST * bsptr.fraction;
	if (dist < GRAPPLE_SPEED_MINDIST) return;
	//the swing has to end somewhere safe: from the hang under the anchor,
	//carrying the leg's speed
	VectorMA(bsptr.endpos, -48, dir, hang);
	VectorScale(bs->legdir, GRAPPLE_SPEED_RELEASE, vel);
	if (BotGrapplePredictArc(bs, hang, vel, GRAPPLE_AIRCONTROL, &move) != GRAPPLE_ARC_SAFE) return;
	BotTacticalGrappleBegin(bs, GRAPPLE_MODE_SPEED);
	bs->weaponnum = WP_GRAPPLING_HOOK;
	VectorCopy(bsptr.endpos, bs->grapplesavepoint);
	bs->grappleent = ENTITYNUM_WORLD;
	bs->grapplehookent = 0;
	bs->grapplestart_time = FloatTime();
	bs->grapplepull_time = 0;
}

/*
==================
BotGrappleOutDamagesSwap

Is the tether killing faster than whatever the bot would raise instead?

An attached hook re-ticks into a body it is already stuck in, so its damage is
a rate the target cannot dodge, spread cannot thin, and travel time cannot
delay. A swap answers with a rate that has to be earned, after paying the
mode's drop and raise time. Both sides are read from the mode's own weapon
table, so a balance change moves this decision with it.

Blind to victim health, comparing NOMINAL rates. That is the conservative
direction: where it is wrong it lets go of a tether that was winning.
==================
*/
static int BotWeaponDPSAt(const modeConfig_t *cfg, int w, float dist) {
	int dps, n;
	float s, fx, fy;

	if (cfg->weapons[w].fireTime <= 0) return 0;
	//count is pellets/burst where it applies: the shotgun's damage is ONE
	//pellet, and reading it as the shot understates that weapon elevenfold
	n = cfg->weapons[w].count > 0 ? cfg->weapons[w].count : 1;
	dps = cfg->weapons[w].damage * n * 1000 / cfg->weapons[w].fireTime;
	if (cfg->weapons[w].spread <= 0) return dps;
	//a spread weapon only lands its whole volley up close. The pattern is a
	//lateral offset of spread*16 thrown at 8192*16 units, so at range it
	//covers a disc of spread*dist/8192, and what hits is the share of that
	//disc the body covers (15 out from the middle across, 28 tall). Without
	//this the nailgun reads 300/s at any range and the shotgun 110, which is
	//only true with the muzzle against them
	s = (float) cfg->weapons[w].spread * dist / 8192.0f;
	fx = (s > 15.0f) ? 15.0f / s : 1.0f;
	fy = (s > 28.0f) ? 28.0f / s : 1.0f;
	return (int) (dps * fx * fy);
}

static qboolean BotGrappleOutDamagesSwap(bot_state_t *bs, float dist) {
	const modeConfig_t *cfg;
	int w, dps, best, grappledps;

	if (G_GrappleDamage() <= 0) return qfalse;	//a tether that does nothing is just a rope
	cfg = Mode_GetConfig(g_mode.integer);
	grappledps = G_GrappleDamage() * 1000 / GRAPPLE_MODEL_TICK_MS;
	best = 0;
	//WP_NUM_WEAPONS is itself MISSIONPACK-gated, so this walks the nailgun,
	//prox launcher and chaingun in that build and stops before them in baseq3
	for (w = WP_GAUNTLET; w < WP_NUM_WEAPONS; w++) {
		if (w == WP_GRAPPLING_HOOK) continue;
		if (!(bs->cur_ps.stats[STAT_WEAPONS] & (1 << w))) continue;
		if (bs->cur_ps.ammo[w] == 0) continue;	//-1 is infinite, 0 is empty
		dps = BotWeaponDPSAt(cfg, w, dist);
		if (dps > best) best = dps;
	}
	return (grappledps >= best) ? qtrue : qfalse;
}

/*
==================
BotWeaponReach

How far a weapon is worth firing: the mode's own hitscan range where it has
one, a volley weapon only up close, everything else by class.
==================
*/
static float BotWeaponReach(const modeConfig_t *cfg, int w) {
	if (w == WP_GAUNTLET) return 64;
	if (cfg->weapons[w].range > 0) return cfg->weapons[w].range;
	//a volley spends the whole pull at once, so it has to be fired close;
	//BotWeaponDPSAt already thins what lands of it with distance
	if (cfg->weapons[w].count > 0) return 384;
	//a hitscan stream reaches as far as the room does
	if (cfg->weapons[w].speed <= 0) return 1024;
	return 800;
}

/*
==================
BotWantsEngagementRelease

A player being towed into a fight lets go to shoot. True when the bot has a
weapon that reaches the enemy it can see and the drop is survivable.

Judged from where the drop LANDS, not from where the bot hangs: a fall that
keeps the bot alive but puts a ledge between it and the target is not an
engagement.
==================
*/
qboolean BotWantsEngagementRelease(bot_state_t *bs) {
	aas_entityinfo_t entinfo;
	aas_clientmove_t move;
	const modeConfig_t *cfg;
	float dist;
	int w, best, bestdps, dps;

	if (bs->enemy < 0 || bs->enemy >= MAX_CLIENTS) return qfalse;
	//the first half second of a tow is the part the route was priced on
	if (FloatTime() < bs->grapplebite_time + 0.5f) return qfalse;
	BotEntityInfo(bs->enemy, &entinfo);
	if (!entinfo.valid || EntityIsDead(&entinfo)) return qfalse;
	if (!BotEntityVisible(bs->entitynum, bs->eye, bs->viewangles, 360, bs->enemy)) return qfalse;
	{
		int arc = BotGrappleArc(bs, &move);

		if (arc == GRAPPLE_ARC_LETHAL || arc == GRAPPLE_ARC_UNRESOLVED) return qfalse;
	}
	if (!BotGrappleArcSeesEnemy(bs, &move, bs->enemy)) return qfalse;
	dist = Distance(move.endpos, entinfo.origin);
	cfg = Mode_GetConfig(g_mode.integer);
	best = WP_NONE;
	bestdps = 0;
	for (w = WP_GAUNTLET; w < WP_NUM_WEAPONS; w++) {
		if (w == WP_GRAPPLING_HOOK) continue;
		if (!(bs->cur_ps.stats[STAT_WEAPONS] & (1 << w))) continue;
		if (bs->cur_ps.ammo[w] == 0) continue;
		dps = BotWeaponDPSAt(cfg, w, dist);
		if (dps > bestdps) { bestdps = dps; best = w; }
	}
	if (best == WP_NONE) return qfalse;
	if (dist > BotWeaponReach(cfg, best)) return qfalse;
	//the arc above IS the survivability read, on the same state: predicting
	//the same 25 frames a second time can only agree with it
	return qtrue;
}

/*
==================
BotGrappleHookAboutToBite

Whether a flying hook is within a blink of its wall; one already anchored
bit long ago.
==================
*/
qboolean BotGrappleHookAboutToBite(gentity_t *hook) {
	bsp_trace_t bsptr;
	vec3_t dir, end;
	float speed;

	if (hook->s.eType == ET_GRAPPLE) return qtrue;
	VectorCopy(hook->s.pos.trDelta, dir);
	speed = VectorNormalize(dir);
	if (speed < 1) return qtrue;
	VectorMA(hook->r.currentOrigin, speed * 0.3f, dir, end);
	BotAI_Trace(&bsptr, hook->r.currentOrigin, NULL, NULL, end, hook->s.number, MASK_SHOT);
	return bsptr.fraction < 1.0f;
}

/*
==================
BotGrappleSnapsAim

A shot at the world or a mover is exact on the firing command; a yank at a
player fires from the slewed view and can miss.
==================
*/
qboolean BotGrappleSnapsAim(bot_state_t *bs) {
	return bs->grapplemode != GRAPPLE_MODE_YANK;
}

/*
==================
BotGrappleMantleUp

Standing, with the feet at the deck the mantle went for.
==================
*/
static qboolean BotGrappleMantleUp(bot_state_t *bs, playerState_t *ps) {
	if (ps->groundEntityNum == ENTITYNUM_NONE) return qfalse;
	return (ps->origin[2] + MINS_Z >= bs->grapplemantle_deckz - 4);
}

/*
==================
BotGrappleMantleSpend

One attempt per pull, whatever the outcome.
==================
*/
static void BotGrappleMantleSpend(bot_state_t *bs, qboolean ok) {
	if (bot_grapple.integer >= 2) {
		G_Printf("GRAPPLE-MANTLE end c%i %s dt %i\n", bs->client, ok ? "ok" : "fail",
				(int)((FloatTime() - bs->grapplemantle_time) * 1000));
	}
	bs->grapplemantle_time = -1;
}

/*
==================
BotGrappleFrameRelease

The frame half of the maneuvers. EA input only changes on a think, so a
release decided there trails the moment by up to a whole think: a mantle's
tow drags the body back off the lip the step just cleared, and a vault's
knee bleeds the speed the climb was going to spend. Deciding on the think
and acting on the frame is how the move state runs the tow itself.
==================
*/
qboolean BotGrappleFrameRelease(int clientNum) {
	bot_state_t *bs;
	playerState_t *ps;

	bs = BotStateForClient(clientNum);
	if (!bs) return qfalse;
	if (!g_entities[clientNum].client) return qfalse;
	ps = &g_entities[clientNum].client->ps;
	//the mantle's step: let go the frame the ground appears
	if (bs->grapplemantle_time > 0 && BotGrappleMantleUp(bs, ps)) return qtrue;
	//the vault: a lip anchor releases just before the knee's deceleration
	//bleeds the tow, while the rise left in the climb still clears the lip.
	//A save has no published release window, so the knee plus a frame of travel
	//is the gate; an undershoot settles into a hang and the mantle runs
	if (bs->grapplemode == GRAPPLE_MODE_SAVE && bs->grapplesavelip) {
		//once let go, stay let go: the resent command still holds attack,
		//and a single restored frame refires the hook into the face it just
		//cleared. The think ends the save when it finds the hook gone
		if (bs->grapplevault_time) return qtrue;
		if ((ps->pm_flags & PMF_GRAPPLE_PULL)
				&& ps->velocity[2] > 0
				&& Distance(ps->origin, ps->grapplePoint) < GRAPPLE_MODEL_DECEL_KNEE + 20
				&& ps->velocity[2] * ps->velocity[2] / 1600.0f
					> ps->grapplePoint[2] - ps->origin[2] + 30) {
			bs->grapplevault_time = FloatTime();
			bs->grapplevault_deckz = ps->grapplePoint[2] + 6;
			return qtrue;
		}
	}
	return qfalse;
}

/*
==================
BotGrappleMantleFace

The face is asked from the wall itself. The claws' embed angles carry the
SHOT's pitch, not the surface's tilt (a save fired steeply up at a vertical
wall would read as a ceiling by them), but that axis crossed the surface at
the anchor by construction, so the trace rides it through the impact point
(through, because an impact backs off the wall and a trace ENDING there
reads clear) and the tilt comes off the plane it hits. A floor or a ceiling
has no lip to climb onto, and a normal with no horizontal left leaves the
maneuver no direction to work in.
==================
*/
static qboolean BotGrappleMantleFace(bot_state_t *bs, gentity_t *hook, float *nz, vec3_t hn) {
	trace_t tr;
	vec3_t embed, start, end, normal;

	AngleVectors(hook->s.angles, embed, NULL, NULL);
	VectorMA(hook->r.currentOrigin, -8, embed, start);
	VectorMA(hook->r.currentOrigin, 8, embed, end);
	trap_Trace(&tr, start, NULL, NULL, end, bs->entitynum, MASK_SOLID);
	if (tr.startsolid || tr.fraction >= 1.0f) {
		BotGrappleMantleRefuse(bs, "probe", tr.startsolid ? -1 : 1);
		return qfalse;
	}
	VectorCopy(tr.plane.normal, normal);
	if (fabs(normal[2]) > 0.4f) {
		BotGrappleMantleRefuse(bs, "tilt", (int) (normal[2] * 100));
		return qfalse;
	}
	VectorSet(hn, normal[0], normal[1], 0);
	if (VectorLength(hn) < 0.1f) return qfalse;
	VectorNormalize(hn);
	*nz = normal[2];
	return qtrue;
}

/*
==================
BotGrappleMantlePitchSin

The view IS the maneuver: PM_GrappleMove insets the tow target 16 units back
along it, so aiming down and into the face lifts the target by 16*sin(pitch)
and swings it out from the face by 16*cos(pitch): up and clear of the wall,
not into it. This is the sine that parks the feet mid-step below a deck dz
above the anchor (the settled body hangs 2 under the target and its feet 24
under that), held short of the pitch where the press dies.
==================
*/
static float BotGrappleMantlePitchSin(float dz) {
	float s, top;

	s = (dz + 17) / 16.0f;
	//shallower than this and the target clears the wall instead of riding up it
	if (s < 0.42f) s = 0.42f;
	top = sin(DEG2RAD(GRAPPLE_MANTLE_PITCH_MAX));
	if (s > top) s = top;
	return s;
}

/*
==================
BotGrappleMantleRefuse

Why a settled, unsafe hang did NOT mantle: the refusals are where the hangs
that ride out the full hold come from, so each names its gate.
==================
*/
static void BotGrappleMantleRefuse(bot_state_t *bs, const char *why, int detail) {
	if (bot_grapple.integer < 2) return;
	if (FloatTime() < bs->grapplemantlelog_time + 1) return;
	bs->grapplemantlelog_time = FloatTime();
	G_Printf("GRAPPLE-MANTLE refuse c%d %s %d\n", bs->client, why, detail);
}

/*
==================
BotGrappleMantleWanted

Whether a settled hang is one step below a deck it could stand on. The 16-unit
inset caps the lift, so the whole reach is the step: a deck outside it is not
a mantle, it is a climb the tether cannot make.
==================
*/
static qboolean BotGrappleMantleWanted(bot_state_t *bs, gentity_t *hook, float *deckz,
									float *sinp, float *nz, vec3_t hn) {
	vec3_t anchor, start, end, over, raised;
	trace_t tr;
	float dz, gap, hout;

	if (!(bs->cur_ps.pm_flags & PMF_GRAPPLE_PULL)) return qfalse;
	if (hook->s.eType != ET_GRAPPLE) return qfalse;
	//a yank's anchor is a moving player, and its view is not snapped: the
	//commanded pitch would still be slewing when the budget ran out
	if (bs->grapplemode == GRAPPLE_MODE_YANK) return qfalse;
	VectorCopy(bs->cur_ps.grapplePoint, anchor);
	//settled, inside the same arrive window a route tow is judged by, so the
	//two agree on when a hang has stopped going anywhere. The velocity is
	//COMMANDED (the tow writes 10x the remaining distance every frame), so
	//a body pinned against a lip reads fast while going nowhere: what it
	//MOVED since last think is the truth
	if (Distance(bs->origin, anchor) > 48
			|| (VectorLength(bs->cur_ps.velocity) > 40
				&& Distance(bs->origin, bs->grapplehang_org) > 8)) {
		VectorCopy(bs->origin, bs->grapplehang_org);
		bs->grapplesettle_time = 0;
		return qfalse;
	}
	VectorCopy(bs->origin, bs->grapplehang_org);
	if (!bs->grapplesettle_time) bs->grapplesettle_time = FloatTime();
	if (FloatTime() < bs->grapplesettle_time + GRAPPLE_MANTLE_SETTLE) return qfalse;
	//the same predicate that pins the hang in the first place: where letting go
	//is already fine there is nothing to salvage
	if (BotGrappleReleaseSafe(bs)) return qfalse;
	if (FloatTime() > bs->grapplepull_time + GRAPPLE_MAXHOLD) return qfalse;
	if (!BotGrappleMantleFace(bs, hook, nz, hn)) return qfalse;
	//the air over the lip has to be open, far enough past the face that the
	//standing spot survives the tow's last tug back toward the anchor. Traced
	//the way a bot's own pmove traces: a botclip brush capping the ledge is
	//nothing to MASK_PLAYERSOLID and would then refuse the step
	VectorMA(anchor, 2, hn, start);
	start[2] += 10;
	VectorMA(start, -32, hn, end);
	trap_Trace(&tr, start, NULL, NULL, end, bs->entitynum, GRAPPLE_MANTLE_CLIP);
	if (tr.startsolid || tr.fraction < 1.0f) {
		BotGrappleMantleRefuse(bs, "lip", tr.startsolid ? -1 : (int) (tr.fraction * 100));
		return qfalse;
	}
	//and there has to be a standable floor under it
	VectorCopy(tr.endpos, over);
	VectorCopy(over, end);
	end[2] -= 42;
	trap_Trace(&tr, over, NULL, NULL, end, bs->entitynum, GRAPPLE_MANTLE_CLIP);
	if (tr.startsolid || tr.fraction >= 1.0f || tr.plane.normal[2] < 0.7f) {
		BotGrappleMantleRefuse(bs, "floor", 0);
		return qfalse;
	}
	*deckz = tr.endpos[2];
	dz = *deckz - anchor[2];
	//the pitch this deck asks for, and what it leaves for the step
	*sinp = BotGrappleMantlePitchSin(dz);
	gap = dz + 26 - 16 * (*sinp);
	//the step lifts 18 and needs somewhere to lift from: outside that the deck
	//is either out of reach or already underfoot
	if (gap < 2 || gap > 18) {
		BotGrappleMantleRefuse(bs, "gap", (int) gap);
		return qfalse;
	}
	//the settled body hangs 16*cos(pitch) out from the face, the same inset
	//PM_GrappleMove leaves the tow target at; floored so the trace never
	//starts inside the face near the 85-degree pitch cap, where that thins
	//to under 1.4 units
	hout = 16 * sqrt(1 - (*sinp) * (*sinp));
	if (hout < 2) hout = 2;
	//the body's own path: flush against the face, raised by the step, across to
	//the standing spot. This is the slide the step itself will attempt
	VectorMA(anchor, hout, hn, raised);
	raised[2] = anchor[2] + 16 * (*sinp) - 2 + 18;
	VectorCopy(over, end);
	end[2] = raised[2];
	trap_Trace(&tr, raised, playerMins, playerMaxs, end, bs->entitynum, GRAPPLE_MANTLE_CLIP);
	if (tr.startsolid || tr.fraction < 1.0f) {
		BotGrappleMantleRefuse(bs, "path", 0);
		return qfalse;
	}
	return qtrue;
}

/*
==================
BotGrappleMantleInputs

Built, not solved: the direction comes straight from the sine the geometry
picked. vectoangles rather than the aim helper, which jitters the view for a
low-accuracy character, and the view is exact on the next command because the
grapple is in hand.
==================
*/
static void BotGrappleMantleInputs(bot_state_t *bs, float sinp, vec3_t hn) {
	vec3_t want, into;

	VectorScale(hn, -sqrt(1 - sinp * sinp), want);
	want[2] = -sinp;
	vectoangles(want, bs->ideal_viewangles);
	//a single think without the trigger, or with another weapon, frees the hook
	bs->weaponnum = WP_GRAPPLING_HOOK;
	trap_EA_Attack(bs->client);
	//the tow is what climbs the face; this only matters once the step has put
	//ground under the feet
	VectorNegate(hn, into);
	trap_EA_Move(bs->client, into, 400);
}

/*
==================
BotTacticalGrappleFrame

Runs after the AI node every think, so a node transition can never drop a
live hook unguarded. Releases happen by not pressing attack for a frame.
==================
*/
void BotTacticalGrappleFrame(bot_state_t *bs) {
	gentity_t *hook;
	gentity_t *mover;
	aas_entityinfo_t entinfo;
	bsp_trace_t bsptr;
	vec3_t aimpoint, dir, center;
	qboolean wantrelease, whiff, samecomover;
	float hookdist, targetdist, hold, align;
	int pulled;

	if (bs->grapplemode == GRAPPLE_MODE_NONE) return;
	if (BotIsDead(bs) || BotIsObserver(bs) || BotIntermission(bs) || !bot_grapple.integer) {
		BotTacticalGrappleEnd(bs, qfalse);
		return;
	}
	hook = g_entities[bs->client].client->hook;
	pulled = bs->cur_ps.pm_flags & PMF_GRAPPLE_PULL;
	//one hook per press, so a launched hook that is gone was freed: done,
	//unless a retargeting save freed it on purpose, in which case the mode
	//keeps running and the pre-launch flow takes the new point from here
	if (hook) {
		if (!bs->grapplehookent) bs->grapplehookent = hook->s.number;
	}
	else if (bs->grapplehookent) {
		if (bs->grapplemode == GRAPPLE_MODE_SAVE && bs->grappleretarget_time
				&& FloatTime() < bs->grappleretarget_time + 1.0f) {
			bs->grapplehookent = 0;
			bs->grapplepull_time = 0;
			bs->grapplesettle_time = 0;
			bs->grapplestart_time = FloatTime();
		}
		else {
			BotTacticalGrappleEnd(bs, qfalse);
			return;
		}
	}
	//pre-launch: aim, fire when lined up and clear
	if (!bs->grapplehookent) {
		if (FloatTime() > bs->grapplestart_time + GRAPPLE_YANK_FIREWINDOW) {
			BotTacticalGrappleEnd(bs, qfalse);
			return;
		}
		if (bs->grapplemode == GRAPPLE_MODE_YANK) {
			BotEntityInfo(bs->grappleent, &entinfo);
			//a target that starts falling to its death while the aim settles
			//cancels the shot the same as one that died
			if (!entinfo.valid || EntityIsDead(&entinfo)
					|| BotGrappleEnemyFallDoomed(bs, bs->grappleent)) {
				BotTacticalGrappleEnd(bs, qfalse);
				return;
			}
			bs->weaponnum = WP_GRAPPLING_HOOK;
			BotGrappleAimAtEnemy(bs, &entinfo, aimpoint);
		}
		else if (bs->grapplemode == GRAPPLE_MODE_RIDE) {
			if (!BotGrappleMoverAimPoint(bs, bs->grappleent, aimpoint)) {
				BotTacticalGrappleEnd(bs, qfalse);
				return;
			}
			bs->weaponnum = WP_GRAPPLING_HOOK;
			BotGrappleAimDir(bs, aimpoint);
		}
		else {
			//a speed hook's anchor is a world point too, and the run it was
			//picked from is still under the bot
			bs->weaponnum = WP_GRAPPLING_HOOK;
			VectorCopy(bs->grapplesavepoint, aimpoint);
			BotGrappleAimDir(bs, aimpoint);
		}
		if (bs->grapplemode == GRAPPLE_MODE_SAVE) {
			align = GRAPPLE_ALIGN_SAVE;
			//at flight speed anchors cross the window faster than the slew
			//chases them, and a bite at any speed is clean (the tow
			//overwrites velocity), so the snap earns a wider sliver
			if (bs->cur_ps.velocity[0] * bs->cur_ps.velocity[0]
					+ bs->cur_ps.velocity[1] * bs->cur_ps.velocity[1] > 600 * 600) {
				align *= 2;
			}
		}
		else if (bs->grapplemode == GRAPPLE_MODE_YANK) align = GRAPPLE_ALIGN_YANK;
		else if (bs->grapplemode == GRAPPLE_MODE_SPEED) align = GRAPPLE_ALIGN_SPEED;
		else align = GRAPPLE_ALIGN_RIDE;
		if (fabs(AngleDifference(bs->viewangles[YAW], bs->ideal_viewangles[YAW])) < align
				&& fabs(AngleDifference(bs->viewangles[PITCH], bs->ideal_viewangles[PITCH])) < align) {
			BotAI_Trace(&bsptr, bs->eye, NULL, NULL, aimpoint, bs->entitynum, MASK_SHOT);
			//a world anchor cannot be told from a world occluder by the hit
			//entity: only a trace ENDING at the point is a clear line. A save
			//keeps falling while it slews, and the deck it falls past eats
			//the line to an anchor approved a few thinks ago. Fired anyway,
			//the hook bites the deck's underside and buys a hang over the
			//same fall
			if (bs->grappleent == ENTITYNUM_WORLD
					? Distance(bsptr.endpos, aimpoint) < 32
					: (bsptr.fraction >= 1 || bsptr.ent == bs->grappleent)) {
				trap_EA_Attack(bs->client);
			}
		}
		return;
	}
	//in flight: keep the button down, give up if it never bites
	if (!pulled) {
		//a static point can't be flown past: skip pass-detection for a save
		if (bs->grapplemode != GRAPPLE_MODE_SAVE && bs->grapplemode != GRAPPLE_MODE_SPEED) {
			//a pad that's already flown past its target missed; don't wait around
			hookdist = Distance(bs->origin, hook->r.currentOrigin);
			if (bs->grapplemode == GRAPPLE_MODE_YANK) {
				BotEntityInfo(bs->grappleent, &entinfo);
				if (!entinfo.valid || EntityIsDead(&entinfo)) {
					BotTacticalGrappleEnd(bs, qtrue);
					return;
				}
				targetdist = Distance(bs->origin, entinfo.origin);
			}
			else {
				mover = &g_entities[bs->grappleent];
				if (!mover->inuse || mover->s.eType != ET_MOVER) {
					BotTacticalGrappleEnd(bs, qtrue);
					return;
				}
				VectorAdd(mover->r.absmin, mover->r.absmax, center);
				VectorScale(center, 0.5f, center);
				targetdist = Distance(bs->origin, center);
			}
			if (hookdist > targetdist + GRAPPLE_WHIFF_SLACK) {
				BotTacticalGrappleEnd(bs, qtrue);
				return;
			}
		}
		//backstop: a shot that never got anywhere near is also a failed launch
		if (FloatTime() > bs->grapplestart_time + GRAPPLE_YANK_FIREWINDOW + GRAPPLE_YANK_FLIGHTWINDOW) {
			BotTacticalGrappleEnd(bs, qtrue);
			return;
		}
		bs->weaponnum = WP_GRAPPLING_HOOK;
		trap_EA_Attack(bs->client);
		return;
	}
	//anchored: reel/ride
	if (!bs->grapplepull_time) {
		bs->grapplepull_time = FloatTime();
		bs->grapplepull_health = bs->inventory[INVENTORY_HEALTH];
		bs->grapplemantle_time = 0;
		bs->grapplemantle_deckz = 0;
		bs->grapplesettle_time = 0;
	}
	//lip anchors release frame-side (BotGrappleFrameRelease); a frame that
	//already let go ends the save through the hook-gone path above
	//a hang settled under a lip it could climb is salvageable: aim down so the
	//tow target swings over the deck and let the step finish it. This sits ahead
	//of every mode's ending so the attempt gets its budget whole, and it is a
	//salvage, not a plan: one try per pull and the ending runs unchanged after
	if (bs->grapplemantle_time >= 0) {
		float deckz, sinp, nz;
		vec3_t hn;

		if (bs->grapplemantle_time > 0) {
			if (BotGrappleMantleUp(bs, &bs->cur_ps)) {
				BotTacticalGrappleEnd(bs, qfalse);
				return;
			}
			if (FloatTime() < bs->grapplemantle_time + GRAPPLE_MANTLE_BUDGET
					&& FloatTime() <= bs->grapplepull_time + GRAPPLE_MAXHOLD
					&& hook->s.eType == ET_GRAPPLE
					&& BotGrappleMantleFace(bs, hook, &nz, hn)) {
				//the deck was measured once; the aim it asks for follows from it
				sinp = BotGrappleMantlePitchSin(bs->grapplemantle_deckz - bs->cur_ps.grapplePoint[2]);
				BotGrappleMantleInputs(bs, sinp, hn);
				return;
			}
			BotGrappleMantleSpend(bs, qfalse);
		}
		else if (BotGrappleMantleWanted(bs, hook, &deckz, &sinp, &nz, hn)) {
			bs->grapplemantle_time = FloatTime();
			bs->grapplemantle_deckz = deckz;
			if (bot_grapple.integer >= 2) {
				G_Printf("GRAPPLE-MANTLE start c%i m%i rn %i dz %i gap %i pitch %i\n",
						bs->client, bs->grapplemode, (int)(nz * 100),
						(int)(deckz - bs->cur_ps.grapplePoint[2]),
						(int)(deckz - bs->cur_ps.grapplePoint[2] + 26 - 16 * sinp),
						(int)RAD2DEG(atan2(sinp, sqrt(1 - sinp * sinp))));
			}
			BotGrappleMantleInputs(bs, sinp, hn);
			return;
		}
	}
	wantrelease = qfalse;
	whiff = qfalse;
	//attach identity: latching onto the wrong target/mover is a whiff, not a hit
	if (bs->grapplemode == GRAPPLE_MODE_YANK) {
		if (hook->enemy != &g_entities[bs->grappleent]) {
			wantrelease = qtrue;
			whiff = qtrue;
		}
	}
	else if (bs->grapplemode == GRAPPLE_MODE_RIDE) {
		//wiring lands seconds after impact (CAPTURE), so also accept a
		//trajectory match for the window before the co-mover set exists
		samecomover = G_GrappleCoMoverMember(hook, bs->grappleent)
			|| (hook->target_ent && hook->target_ent->s.eType == ET_MOVER
				&& BG_MoverCoMoves(&hook->target_ent->s, &g_entities[bs->grappleent].s));
		if (!samecomover) {
			wantrelease = qtrue;
			whiff = qtrue;
		}
	}
	else {
		float hspeed = sqrt(bs->cur_ps.velocity[0] * bs->cur_ps.velocity[0]
			+ bs->cur_ps.velocity[1] * bs->cur_ps.velocity[1]);
		aas_clientmove_t speedmove;

		//the pull is only worth what it adds: a short one, or the moment the
		//swing is already fast and the drop lands
		if (FloatTime() > bs->grapplepull_time + GRAPPLE_SPEED_HOLD) wantrelease = qtrue;
		if (hspeed >= GRAPPLE_SPEED_RELEASE && BotGrappleArc(bs, &speedmove) == GRAPPLE_ARC_SAFE) wantrelease = qtrue;
	}
	//dropping into the same field of fire is worse than riding out of it
	if (bs->inventory[INVENTORY_HEALTH] < bs->grapplepull_health - GRAPPLE_BURST_DAMAGE) {
		aas_clientmove_t hurtmove;

		int hurtarc = BotGrappleArc(bs, &hurtmove);

		if (hurtarc != GRAPPLE_ARC_LETHAL && hurtarc != GRAPPLE_ARC_UNRESOLVED
				&& BotGrappleArcBreaksLOS(bs, &hurtmove)) {
			wantrelease = qtrue;
		}
	}
	if (bs->grapplemode == GRAPPLE_MODE_YANK) {
		if (FloatTime() > bs->grapplepull_time + GRAPPLE_YANK_HOLDTIME) wantrelease = qtrue;
		BotEntityInfo(bs->grappleent, &entinfo);
		if (entinfo.valid && !EntityIsDead(&entinfo)) {
			VectorSubtract(entinfo.origin, bs->origin, dir);
			if (VectorLengthSquared(dir) < Square(GRAPPLE_YANK_CLOSEDIST)
					&& !BotGrappleOutDamagesSwap(bs, VectorLength(dir))) wantrelease = qtrue;
		}
	}
	else if (bs->grapplemode == GRAPPLE_MODE_RIDE) {
		//arrived: standing on what we hooked, or captured and carried level
		//with its top (a captured body has no ground, the mover is unlinked
		//around its pmove)
		if (bs->cur_ps.groundEntityNum != ENTITYNUM_NONE
				&& (bs->cur_ps.groundEntityNum == bs->grappleent
					|| G_GrappleCoMoverMember(hook, bs->cur_ps.groundEntityNum))) wantrelease = qtrue;
		if (hook->s.generic1
				&& bs->origin[2] >= g_entities[bs->grappleent].r.absmax[2] - 8) wantrelease = qtrue;
		if (FloatTime() > bs->grapplepull_time + GRAPPLE_RIDE_HOLDTIME) wantrelease = qtrue;
	}
	hold = (bs->grapplemode == GRAPPLE_MODE_RIDE) ? GRAPPLE_RIDE_HOLDTIME
		: (bs->grapplemode == GRAPPLE_MODE_SPEED) ? GRAPPLE_SPEED_MAXHOLD
		: GRAPPLE_YANK_HOLDTIME;
	if (FloatTime() > bs->grapplepull_time + hold + GRAPPLE_UNSAFE_GRACE) wantrelease = qtrue;
	//a yank's anchor is the enemy's body, and one falling to its death drags
	//the tether after it: holding on is the one guaranteed ending, so let go
	//now, while the fall is still shallow
	if (bs->grapplemode == GRAPPLE_MODE_YANK
			&& BotGrappleEnemyFallDoomed(bs, bs->grappleent)) {
		BotTacticalGrappleEnd(bs, qfalse);
		return;
	}
	//letting go over a void or lava is certain death, so ride it out and hope the anchor
	//carries us somewhere survivable; the cap stops that becoming a permanent hang
	if (wantrelease && (BotGrappleReleaseSafe(bs)
			|| FloatTime() > bs->grapplepull_time + GRAPPLE_MAXHOLD)) {
		BotTacticalGrappleEnd(bs, whiff);
		return;
	}
	bs->weaponnum = WP_GRAPPLING_HOOK;
	trap_EA_Attack(bs->client);
	//the view is the tow's mechanism: hold it on the anchor
	VectorCopy(bs->cur_ps.grapplePoint, aimpoint);
	BotGrappleAimDir(bs, aimpoint);
}
