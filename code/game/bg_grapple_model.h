// code/game/bg_grapple_model.h
//
// The numbers that define the tow, in one place.

#ifndef BG_GRAPPLE_MODEL_H
#define BG_GRAPPLE_MODEL_H

// speed the tether pulls at beyond the deceleration knee below
#define GRAPPLE_MODEL_TOWSPEED			800

// PM_GrappleMove measures its distance to the target from this many units back
// along the wielder's view, not from the anchor itself
#define GRAPPLE_MODEL_TOW_TARGET_INSET	16

// inside the knee the tether decelerates, pulling at DECEL_FACTOR times the
// distance that remains rather than the flat towspeed: 1000 at the knee itself,
// 500 at 50u, zero at the target. The knee is where the tow is FASTEST
#define GRAPPLE_MODEL_DECEL_KNEE		100
#define GRAPPLE_MODEL_DECEL_FACTOR		10

// how often an attached hook re-ticks its damage into the body it is stuck in
#define GRAPPLE_MODEL_TICK_MS			125

// speed the hook itself flies at, which is what decides how far a bot has to
// lead a moving target
#define GRAPPLE_MODEL_FIRE_SPEED		1800

#endif
