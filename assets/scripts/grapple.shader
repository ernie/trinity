// grappling hook weapon: launcher, anchor pad, and the tether between them

// Projected energy tether.  CG_GrappleTrail draws three crossed planes; these
// maps carry the cross-section (V) and the traveling charge (U), so the cable
// costs the same at any length.  S counts texture repeats, one per
// TETHER_PULSE_WAVELENGTH of world, which is what stops the wave stretching on
// a long shot.  cull disable: the planes get sighted from either face.
grapplingTether
{
	nopicmip
	cull disable
	sort additive
	{
		map models/weapons2/grapple/tether_halo.tga
		blendfunc GL_SRC_ALPHA GL_ONE
		rgbGen vertex
		alphaGen vertex
		tcMod scroll -2.5 0
	}
	{
		map models/weapons2/grapple/tether_core.tga
		blendfunc GL_SRC_ALPHA GL_ONE
		alphaGen vertex
		tcMod scroll -2.5 0
	}
}

// Electrical arcs, on the launcher's beam span and along the tether.  They
// write their cross-section into the vertices, so they take no map, and must
// not take the tether's scroll.
// grapple.shaderx carries grappleArcFP, this same body plus `depthhack`
// for the view weapon; keep the two in lockstep.
grappleArc
{
	nopicmip
	cull disable
	sort additive
	{
		map $whiteimage
		blendfunc GL_SRC_ALPHA GL_ONE
		rgbGen vertex
		alphaGen vertex
	}
}

// Muzzle glow while the tether is out.  rgbGen entity so it wears the
// owner's effects color the way the railgun blast does; the image is
// white-core on black, so any tint runs it at full brightness.
models/weapons2/grapple/flash
{
	nopicmip
	cull disable
	sort additive
	{
		map models/weapons2/grapple/flashglow.tga
		blendfunc add
		rgbGen entity
	}
}

// Both hardware models are plain diffuse under an emission system.  Emission
// stages run rgbGen entity (owner's color; cgame sets a FLAT per-state level,
// since any brightness pulse strobes the traveling wave, see
// CG_GrappleEnvelope):
//   bed:    the glow sheet, dimmed so the traveling waves above read at full
//           contrast.
//   energy: one filament pattern baked at three phase offsets.  The stages'
//           alphaGen waves share a frequency at 120-degree spacing, a rotating
//           phasor: the pattern travels continuously at render rate, the three
//           sinusoids sum flat, and zero UV motion means energy never leaves
//           the glass.
// States pair cgame's flat shaderRGBA levels with direction/speed variants
// picked via refEntity customShader:
//   base:  idle, forward (toward the muzzle / rings outward), idle pace
//   _fly:  hook away, forward, faster
//   _pull: hook anchored, owner reeling, REVERSED, faster
// SIGN NOTE: dominance rotates BACKWARD through the alpha phases, so forward
// is phase order 0/.667/.333 and reverse is 0/.333/.667.
// The depthFunc equal alternation keeps the vk stage-collapser from bundling
// the wave stages.  Every path drawing these models must set shaderRGBA or
// the emission renders black.
// surfaceparm nodlight: the vk per-pixel dlight pass modulates its light by
// ONE stage's texture, chosen by heuristic; here that lands on nrg3, stamping
// a static copy of the filament pattern over the traveling wave whenever a
// dynamic light touches the model.
// grapple.shaderx overrides all six shaders wherever .shaderx is scanned,
// swapping nodlight for a `dlight` keyword on the diffuse stage.  These
// copies keep nodlight as the fallback for engines that never scan .shaderx.
// EDIT BOTH FILES IN LOCKSTEP: same six shaders, differing only by that swap.

// ---- anchor pad ---------------------------------------------------------

// docked in the bore (only drawn at idle): rings outward at the idle pace
models/weapons2/grapple/pad
{
	nopicmip
	surfaceparm nodlight
	{
		map models/weapons2/grapple/pad.tga
		rgbGen lightingDiffuse
	}
	{
		map models/weapons2/grapple/pad_glow.tga
		blendFunc GL_SRC_ALPHA GL_ONE
		rgbGen entity
		alphaGen const 0.4
		depthFunc equal
	}
	{
		map models/weapons2/grapple/pad_nrg1.tga
		blendFunc GL_SRC_ALPHA GL_ONE
		rgbGen entity
		alphaGen wave sin 0.2 0.2 0 0.41667
	}
	{
		map models/weapons2/grapple/pad_nrg2.tga
		blendFunc GL_SRC_ALPHA GL_ONE
		rgbGen entity
		alphaGen wave sin 0.2 0.2 0.667 0.41667
		depthFunc equal
	}
	{
		map models/weapons2/grapple/pad_nrg3.tga
		blendFunc GL_SRC_ALPHA GL_ONE
		rgbGen entity
		alphaGen wave sin 0.2 0.2 0.333 0.41667
	}
}

// in flight: rings outward, faster
models/weapons2/grapple/pad_fly
{
	nopicmip
	surfaceparm nodlight
	{
		map models/weapons2/grapple/pad.tga
		rgbGen lightingDiffuse
	}
	{
		map models/weapons2/grapple/pad_glow.tga
		blendFunc GL_SRC_ALPHA GL_ONE
		rgbGen entity
		alphaGen const 0.4
		depthFunc equal
	}
	{
		map models/weapons2/grapple/pad_nrg1.tga
		blendFunc GL_SRC_ALPHA GL_ONE
		rgbGen entity
		alphaGen wave sin 0.2 0.2 0 1.25
	}
	{
		map models/weapons2/grapple/pad_nrg2.tga
		blendFunc GL_SRC_ALPHA GL_ONE
		rgbGen entity
		alphaGen wave sin 0.2 0.2 0.667 1.25
		depthFunc equal
	}
	{
		map models/weapons2/grapple/pad_nrg3.tga
		blendFunc GL_SRC_ALPHA GL_ONE
		rgbGen entity
		alphaGen wave sin 0.2 0.2 0.333 1.25
	}
}

// anchored, owner reeling: rings inward, faster
models/weapons2/grapple/pad_pull
{
	nopicmip
	surfaceparm nodlight
	{
		map models/weapons2/grapple/pad.tga
		rgbGen lightingDiffuse
	}
	{
		map models/weapons2/grapple/pad_glow.tga
		blendFunc GL_SRC_ALPHA GL_ONE
		rgbGen entity
		alphaGen const 0.4
		depthFunc equal
	}
	{
		map models/weapons2/grapple/pad_nrg1.tga
		blendFunc GL_SRC_ALPHA GL_ONE
		rgbGen entity
		alphaGen wave sin 0.2 0.2 0 1.25
	}
	{
		map models/weapons2/grapple/pad_nrg2.tga
		blendFunc GL_SRC_ALPHA GL_ONE
		rgbGen entity
		alphaGen wave sin 0.2 0.2 0.333 1.25
		depthFunc equal
	}
	{
		map models/weapons2/grapple/pad_nrg3.tga
		blendFunc GL_SRC_ALPHA GL_ONE
		rgbGen entity
		alphaGen wave sin 0.2 0.2 0.667 1.25
	}
}

// materializing: pad.tga's alpha is an arrival order, not an opacity, so
// sweeping alphaGen entity travels a cut across the surface. The test is on
// the product of the two, so the ramp is packed into 128..255.
//
// No depth prepass: it would write depth for the texels the cut removes, and
// the additive stages behind depthFunc equal would light up holes.
//
// Those stages run hotter here than at idle, and their strength is their own
// alphaGen rather than the entity's: rgbGen entity cannot flare a color that
// already has a channel at 255, so cgame has no way to push them.
models/weapons2/grapple/pad_fade
{
	nopicmip
	surfaceparm nodlight
	{
		map models/weapons2/grapple/pad.tga
		alphaFunc GE128
		alphaGen entity
		rgbGen lightingDiffuse
		depthwrite
	}
	{
		// the leading edge: pad_ramp.tga is the same ramp scaled down, so it
		// trips later than the diffuse and LT128 keeps only the strip between
		// the two thresholds -- a band that travels with the cut
		map models/weapons2/grapple/pad_ramp.tga
		blendFunc GL_ONE GL_ONE
		alphaFunc LT128
		alphaGen entity
		rgbGen entity
		depthFunc equal
	}
	{
		map models/weapons2/grapple/pad_glow.tga
		blendFunc GL_SRC_ALPHA GL_ONE
		rgbGen entity
		alphaGen const 0.4
		depthFunc equal
	}
	{
		map models/weapons2/grapple/pad_nrg1.tga
		blendFunc GL_SRC_ALPHA GL_ONE
		rgbGen entity
		alphaGen wave sin 0.2 0.2 0 0.41667
		depthFunc equal
	}
	{
		map models/weapons2/grapple/pad_nrg2.tga
		blendFunc GL_SRC_ALPHA GL_ONE
		rgbGen entity
		alphaGen wave sin 0.2 0.2 0.667 0.41667
		depthFunc equal
	}
	{
		map models/weapons2/grapple/pad_nrg3.tga
		blendFunc GL_SRC_ALPHA GL_ONE
		rgbGen entity
		alphaGen wave sin 0.2 0.2 0.333 0.41667
		depthFunc equal
	}
}

// coming apart: the same dissolve with the test inverted, so the ramp's high
// ground -- the claw tips and the rim -- clears first and the pad collapses
// onto its hub. One ramp cannot run both ways off the same test.
models/weapons2/grapple/pad_unform
{
	nopicmip
	surfaceparm nodlight
	{
		map models/weapons2/grapple/pad.tga
		alphaFunc LT128
		alphaGen entity
		rgbGen lightingDiffuse
		depthwrite
	}
	{
		// the trailing edge: what survives is below the sweep, so this needs
		// the ramp scaled up instead, tripping just before the diffuse -- the
		// band sits inside the vanishing boundary
		map models/weapons2/grapple/pad_ramp_hi.tga
		blendFunc GL_ONE GL_ONE
		alphaFunc GE128
		alphaGen entity
		rgbGen entity
		depthFunc equal
	}
	{
		map models/weapons2/grapple/pad_glow.tga
		blendFunc GL_SRC_ALPHA GL_ONE
		rgbGen entity
		alphaGen const 0.4
		depthFunc equal
	}
	{
		map models/weapons2/grapple/pad_nrg1.tga
		blendFunc GL_SRC_ALPHA GL_ONE
		rgbGen entity
		alphaGen wave sin 0.2 0.2 0 0.41667
		depthFunc equal
	}
	{
		map models/weapons2/grapple/pad_nrg2.tga
		blendFunc GL_SRC_ALPHA GL_ONE
		rgbGen entity
		alphaGen wave sin 0.2 0.2 0.667 0.41667
		depthFunc equal
	}
	{
		map models/weapons2/grapple/pad_nrg3.tga
		blendFunc GL_SRC_ALPHA GL_ONE
		rgbGen entity
		alphaGen wave sin 0.2 0.2 0.333 0.41667
		depthFunc equal
	}
}


// ---- launcher -----------------------------------------------------------

// idle: stream toward the muzzle at the idle pace
models/weapons2/grapple/gun
{
	nopicmip
	surfaceparm nodlight
	{
		map models/weapons2/grapple/gun.tga
		rgbGen lightingDiffuse
	}
	{
		map models/weapons2/grapple/gun_glow.tga
		blendFunc GL_SRC_ALPHA GL_ONE
		rgbGen entity
		alphaGen const 0.4
		depthFunc equal
	}
	{
		map models/weapons2/grapple/gun_nrg1.tga
		blendFunc GL_SRC_ALPHA GL_ONE
		rgbGen entity
		alphaGen wave sin 0.2 0.2 0 0.41667
	}
	{
		map models/weapons2/grapple/gun_nrg2.tga
		blendFunc GL_SRC_ALPHA GL_ONE
		rgbGen entity
		alphaGen wave sin 0.2 0.2 0.667 0.41667
		depthFunc equal
	}
	{
		map models/weapons2/grapple/gun_nrg3.tga
		blendFunc GL_SRC_ALPHA GL_ONE
		rgbGen entity
		alphaGen wave sin 0.2 0.2 0.333 0.41667
	}
}

// hook away: toward the muzzle, faster
models/weapons2/grapple/gun_fly
{
	nopicmip
	surfaceparm nodlight
	{
		map models/weapons2/grapple/gun.tga
		rgbGen lightingDiffuse
	}
	{
		map models/weapons2/grapple/gun_glow.tga
		blendFunc GL_SRC_ALPHA GL_ONE
		rgbGen entity
		alphaGen const 0.4
		depthFunc equal
	}
	{
		map models/weapons2/grapple/gun_nrg1.tga
		blendFunc GL_SRC_ALPHA GL_ONE
		rgbGen entity
		alphaGen wave sin 0.2 0.2 0 1.25
	}
	{
		map models/weapons2/grapple/gun_nrg2.tga
		blendFunc GL_SRC_ALPHA GL_ONE
		rgbGen entity
		alphaGen wave sin 0.2 0.2 0.667 1.25
		depthFunc equal
	}
	{
		map models/weapons2/grapple/gun_nrg3.tga
		blendFunc GL_SRC_ALPHA GL_ONE
		rgbGen entity
		alphaGen wave sin 0.2 0.2 0.333 1.25
	}
}

// reeling: drawn back down the weapon, faster
models/weapons2/grapple/gun_pull
{
	nopicmip
	surfaceparm nodlight
	{
		map models/weapons2/grapple/gun.tga
		rgbGen lightingDiffuse
	}
	{
		map models/weapons2/grapple/gun_glow.tga
		blendFunc GL_SRC_ALPHA GL_ONE
		rgbGen entity
		alphaGen const 0.4
		depthFunc equal
	}
	{
		map models/weapons2/grapple/gun_nrg1.tga
		blendFunc GL_SRC_ALPHA GL_ONE
		rgbGen entity
		alphaGen wave sin 0.2 0.2 0 1.25
	}
	{
		map models/weapons2/grapple/gun_nrg2.tga
		blendFunc GL_SRC_ALPHA GL_ONE
		rgbGen entity
		alphaGen wave sin 0.2 0.2 0.333 1.25
		depthFunc equal
	}
	{
		map models/weapons2/grapple/gun_nrg3.tga
		blendFunc GL_SRC_ALPHA GL_ONE
		rgbGen entity
		alphaGen wave sin 0.2 0.2 0.667 1.25
	}
}

// Anchor bite scars, stamped with the mark orientation solved onto the pad's
// rolled axes (the claw layout is not 3-fold symmetric). Same darken-by-alpha
// recipe as the stock gfx/damage marks. Three of them because a rotation
// would walk the gouges off the claws; cgame picks from the hook's stamp.
gfx/damage/pad_mrk
{
	polygonOffset
	{
		map gfx/damage/pad_mrk.tga
		blendFunc GL_ZERO GL_ONE_MINUS_SRC_ALPHA
		rgbGen exactVertex
		// this blend reads only alpha: the release fade must ride vertex
		// alpha (stock bloodMark recipe) or the mark fade is inert
		alphaGen vertex
	}
}

gfx/damage/pad_mrk2
{
	polygonOffset
	{
		map gfx/damage/pad_mrk2.tga
		blendFunc GL_ZERO GL_ONE_MINUS_SRC_ALPHA
		rgbGen exactVertex
		alphaGen vertex
	}
}

gfx/damage/pad_mrk3
{
	polygonOffset
	{
		map gfx/damage/pad_mrk3.tga
		blendFunc GL_ZERO GL_ONE_MINUS_SRC_ALPHA
		rgbGen exactVertex
		alphaGen vertex
	}
}
