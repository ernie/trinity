// Copyright (C) 1999-2000 Id Software, Inc.
//
// Shared color-swatch translation tables for the UI color picker.
//
// Single source of truth for all three UI surfaces (baseq3 preferences,
// baseq3 player settings, missionpack main UI). Previously each UI source
// file kept its own copy of these arrays, which let them drift.
//
// Maps color1 / cg_crosshairColor cvar value (1-7) to UI slider position (0-6).
// Cvar is parsed by CG_ColorFromString (bit-pattern), producing:
//   1=blue, 2=green, 3=cyan, 4=red, 5=magenta, 6=yellow, 7=white
// UI slot order is the visible spectrum: red, yellow, green, cyan, blue,
// magenta, white. See docs/COLOR_SCHEMES.md.

#ifndef __UI_SWATCHES_H
#define __UI_SWATCHES_H

static const int gamecodetoui[] = {4,2,3,0,5,1,6};
static const int uitogamecode[] = {4,6,2,3,1,5,7};

#endif // __UI_SWATCHES_H
