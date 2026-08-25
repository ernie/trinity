// code/game/ai_grapple.h
//
// The grapple's expressive half: combat yanks, fall saves, mover rides. None
// of it is load-bearing for navigation: a maneuver here may fail and no route
// was depending on it, which is why it may be chosen live from game state a
// route cannot see. Everything not declared here is static by design.

#ifndef AI_GRAPPLE_H
#define AI_GRAPPLE_H

void		BotCheckTacticalGrapple(bot_state_t *bs, aas_entityinfo_t *entinfo);
void		BotTacticalGrappleFrame(bot_state_t *bs);
int			BotTacticalGrappleActive(bot_state_t *bs);
qboolean	BotGrappleAvailable(bot_state_t *bs);
qboolean	BotWantsEngagementRelease(bot_state_t *bs);
qboolean	BotGrappleSnapsAim(bot_state_t *bs);
qboolean	BotGrappleHookAboutToBite(gentity_t *hook);

#endif
