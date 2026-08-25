// code/game/ai_grapple.h
//
// The grapple's expressive half: combat yanks, fall saves, mover rides. None
// of it is load-bearing for navigation: a maneuver here may fail and no route
// was depending on it, which is why it may be chosen live from game state a
// route cannot see. Everything not declared here is static by design.

#ifndef AI_GRAPPLE_H
#define AI_GRAPPLE_H

void		BotCheckTacticalGrapple(bot_state_t *bs, aas_entityinfo_t *entinfo);
void		BotCheckGrappleRide(bot_state_t *bs, bot_moveresult_t *moveresult);
void		BotCheckGrappleSave(bot_state_t *bs);
void		BotCheckGrappleSpeed(bot_state_t *bs, bot_moveresult_t *moveresult);
void		BotTacticalGrappleFrame(bot_state_t *bs);
int			BotTacticalGrappleActive(bot_state_t *bs);
qboolean	BotGrappleAvailable(bot_state_t *bs);
qboolean	BotGrappleRouteAvailable(bot_state_t *bs);
qboolean	BotWantsEngagementRelease(bot_state_t *bs);
qboolean	BotGrappleSnapsAim(bot_state_t *bs);
qboolean	BotGrappleHookAboutToBite(gentity_t *hook);
//also declared in g_local.h: the frame side of the maneuvers runs from
//ClientThink_real, which cannot see a bot state
qboolean	BotGrappleFrameRelease(int clientNum);

#endif
