# Trinity Color Schemes

Trinity has **two distinct number-to-color mappings**, plus a handful of
consumers and presentation layers built on top of them. Several inline
comments in the source historically described one mapping while the code
beneath them implemented another, so this document is the authoritative
reference.

The two underlying mappings:

1. **Chat `^N` palette** — id's original, defined in `g_color_table`. Used
   when text rendering sees a `^N` escape. The engine extends this palette
   past `^7` with additional colors (orange, light purple, etc.) that the
   mod's QVMs cannot access — see §8.
2. **`CG_ColorFromString` bit-pattern** — a separate mapping where digit `1`
   produces blue, `4` produces red, etc. Used by `color1` / `color2`
   userinfo, and by `cg_enemyColors` / `cg_teamColors` (a multi-character
   string fed to the same parser, with an optional `'?'` placeholder
   substitution for team-aware configs — see §5).

The UI slider is a **presentation layer** on top of mapping #2: it shows
the player seven swatches in spectrum order (red, yellow, green, cyan,
blue, magenta, white) and uses translation arrays to map the chosen slot
back to a `color1` digit — see §6.

For completeness, `CG_TeamColor()` returns a small fixed set of pastel
tints for HUD/scoreboard text. It takes a team enum, not a digit, so it
isn't on any numbering scheme — but it's a visual you'll see in code and
is worth a section (§7).

---

## 1. What you see in player settings

This is the most human-facing view. The settings menu (baseq3 `Q3_UI` and
missionpack `UI`) shows seven color swatches the player clicks through, in
**rainbow spectrum order**:

| Slot | Visible swatch | Stored `color1` value | Renders as |
|---|---|---|---|
| 0 | **red**     | 4 | red                   |
| 1 | **yellow**  | 6 | yellow                |
| 2 | **green**   | 2 | green                 |
| 3 | **cyan**    | 3 | cyan (0,1,1)          |
| 4 | **blue**    | 1 | blue                  |
| 5 | **magenta** | 5 | magenta (1,0,1)       |
| 6 | **white**   | 7 | white                 |

The stored values (4, 6, 2, 3, 1, 5, 7) look out-of-order because they're the
bit-pattern encoding used by `CG_ColorFromString` (see §4); the menu is what a
player actually thinks in, and the storage is an implementation detail.

---

## 2. The numbering mappings at a glance

For digit `N`, what does each subsystem produce?

| `N` | Chat `^N` (cgame/q3_ui) | Chat `^N` (engine path) | `color1` / `cg_*Colors` |
|---|---|---|---|
| 0 | black            | black                          | white (fallback) |
| 1 | **red**          | red                            | **blue**         |
| 2 | green            | green                          | green            |
| 3 | yellow           | yellow                         | cyan             |
| 4 | **blue**         | blue                           | **red**          |
| 5 | cyan             | cyan                           | magenta          |
| 6 | magenta          | magenta                        | yellow           |
| 7 | white            | white                          | white            |
| 8 | black (wraps)    | _engine-specific_              | white (fallback) |
| 9 | red (wraps)      | _engine-specific_              | white (fallback) |

Only `2` (green) and `7` (white) mean the same color in both chat and
`color1`. The chat palette and the `color1` palette are mirror-inverted in
the R↔B bits (e.g. `1` swaps with `4`, `3` swaps with `5` under an R↔B bit
flip) because the bit-pattern parser reads `bit 0 = Blue` while the chat
palette puts red at index 1.

The UI slider is documented separately in §6 — the player only ever sees
seven swatches in spectrum order (red, yellow, green, cyan, blue, magenta,
white) and the `uitogamecode` / `gamecodetoui` tables translate that to
`color1` values behind the scenes.

---

## 3. Chat / console `^N` escapes

**Definition:** [code/game/q_math.c:23-33](../code/game/q_math.c#L23-L33)

```c
vec4_t g_color_table[8] = {
    {0.0, 0.0, 0.0, 1.0},   // ^0 black
    {1.0, 0.0, 0.0, 1.0},   // ^1 red
    {0.0, 1.0, 0.0, 1.0},   // ^2 green
    {1.0, 1.0, 0.0, 1.0},   // ^3 yellow
    {0.0, 0.0, 1.0, 1.0},   // ^4 blue
    {0.0, 1.0, 1.0, 1.0},   // ^5 cyan
    {1.0, 0.0, 1.0, 1.0},   // ^6 magenta
    {1.0, 1.0, 1.0, 1.0},   // ^7 white
};
```

**Indexing:** [code/game/q_shared.h:334](../code/game/q_shared.h#L334)

```c
#define ColorIndex(c)  ( ( (c) - '0' ) & 7 )
```

The 3-bit mask means `^8` wraps to `^0`, `^9` wraps to `^1`, and so on —
*inside this repository's code*. Consumers in the mod that render colored text
are:

- HUD and frag feed: [code/cgame/cg_drawtools.c:193](../code/cgame/cg_drawtools.c#L193), [code/cgame/cg_draw.c:140](../code/cgame/cg_draw.c#L140)
- New high-res font path: [code/cgame/cg_newdraw.c:1210](../code/cgame/cg_newdraw.c#L1210)
- Menus and server browser (q3_ui): [code/q3_ui/ui_atoms.c:654](../code/q3_ui/ui_atoms.c#L654)
- Menus (missionpack UI): [code/ui/ui_main.c:363](../code/ui/ui_main.c#L363), [code/ui/ui_main.c:433](../code/ui/ui_main.c#L433), [code/ui/ui_main.c:528](../code/ui/ui_main.c#L528)

The convenience macros `COLOR_RED`…`COLOR_WHITE` and `S_COLOR_RED`…`S_COLOR_WHITE`
([code/game/q_shared.h:326-343](../code/game/q_shared.h#L326-L343)) name the
seven characters so source code doesn't have to embed magic digits.

---

## 4. Player `color1` / `color2` userinfo

**Parser:** [code/cgame/cg_players.c:680-706](../code/cgame/cg_players.c#L680-L706)

This is not a palette lookup — it's a bit-pattern decode:

```c
val = v[0] - '0';
if ( val < 1 || val > 7 ) { VectorSet( color, 1.0f, 1.0f, 1.0f ); return; }
if ( val & 1 ) color[2] = 1.0f;   // bit 0 -> Blue
if ( val & 2 ) color[1] = 1.0f;   // bit 1 -> Green
if ( val & 4 ) color[0] = 1.0f;   // bit 2 -> Red
```

| digit | bit pattern | RGB     | Name    |
|---|---|---|---|
| `'0'` | —    | (1,1,1) | **white** (fallback, NOT black) |
| `'1'` | 001  | (0,0,1) | blue    |
| `'2'` | 010  | (0,1,0) | green   |
| `'3'` | 011  | (0,1,1) | cyan    |
| `'4'` | 100  | (1,0,0) | red     |
| `'5'` | 101  | (1,0,1) | magenta |
| `'6'` | 110  | (1,1,0) | yellow  |
| `'7'` | 111  | (1,1,1) | white   |
| `'8'`+ | —   | (1,1,1) | white (fallback) |

Why the digits look "out of order" relative to the menu spectrum: read each
digit as a three-bit number `bit2 bit1 bit0` = `R G B`. Setting any bit lights
that channel. So `'4'` = `100` = red-channel-only = red, and `'1'` = `001` =
blue-channel-only = blue. The menu sorts by visible spectrum; the parser sorts
by bit weight.

**There is no `cg_color1` cvar** — only the lowercase `color1` userinfo, set
by the UI ([§6](#6-ui-slider-translation-tables)) and broadcast unchanged by
the server ([code/game/g_client.c:716](../code/game/g_client.c#L716)).

**Where the colors render:** rail-core color (`color1`) and rail-spiral color
(`color2`) — [code/cgame/cg_weapons.c:196-290](../code/cgame/cg_weapons.c#L196-L290).
The lightning gun does not use these. Scoreboard and other HUD elements use
[`CG_TeamColor()`](#7-pastel-team-tint) instead.

---

## 5. `cg_enemyColors` / `cg_teamColors`

**Cvars:** [code/cgame/cg_cvar.h:128,130](../code/cgame/cg_cvar.h#L128). Both
default to the empty string.

**Parser:** [`CG_SetColorInfo` cg_players.c:709-735](../code/cgame/cg_players.c#L709-L735)
walks up to five characters of the cvar string and feeds each one to
`CG_ColorFromString` ([§4](#4-player-color1--color2-userinfo)). String layout:

| Position | Field on `clientInfo_t` | Effect |
|---|---|---|
| `[0]` | `headColor`  | tint applied to head model |
| `[1]` | `bodyColor`  | tint applied to torso model |
| `[2]` | `legsColor`  | tint applied to legs model |
| `[3]` | `color1`     | overrides userinfo `color1` (rail core) |
| `[4]` | `color2`     | overrides userinfo `color2` (rail spiral) |

Parsing stops at the first `'\0'`, but the two halves of the string handle
missing positions differently:

- Positions `[0]` / `[1]` / `[2]` (head / body / legs): the parser **resets
  these to white at the start of every call** ([cg_players.c:711-713](../code/cgame/cg_players.c#L711-L713)),
  so any missing position falls back to white.
- Positions `[3]` / `[4]` (color1 / color2): these are **not reset** — see
  the literal `// override color1/color2 if specified` comment at
  [cg_players.c:727](../code/cgame/cg_players.c#L727). If the string is too
  short to reach them, the player's existing `color1` / `color2` (set
  earlier from their userinfo `c1` / `c2` in `CG_NewClientInfo`) are kept.

So `cg_enemyColors "1234"` sets head=blue, body=green, legs=cyan, color1=red,
**color2 stays at the enemy's own userinfo color2**. And `cg_enemyColors "1"`
sets head=blue, body=white, legs=white, with the enemy's own color1/color2
both preserved unchanged.

**Gating:** these cvars are applied only inside the `cg_enemyModel`,
`cg_teamModel`, `pm` (pmod skin) and `fb` (fullbright skin) branches of
[`CG_SetSkinAndModel` cg_players.c:1071-1184](../code/cgame/cg_players.c#L1071-L1184),
and re-evaluated for color1/color2 in teamplay at
[`CG_NewClientInfo` cg_players.c:1343-1351](../code/cgame/cg_players.c#L1343-L1351).
Players without a model override see other players' normal `color1` /
`color2`.

### The `'?'` placeholder

[`CG_GetTeamColors` cg_players.c:754](../code/cgame/cg_players.c#L754)
substitutes `'?'` characters in the cvar string with the bit-pattern digit
for the player's team color before parsing:

```c
case TEAM_RED:  replace1( '?', '4', str ); break;
case TEAM_BLUE: replace1( '?', '1', str ); break;
case TEAM_FREE: replace1( '?', '7', str ); break;
```

Substituted digits feed `CG_ColorFromString` ([§4](#4-player-color1--color2-userinfo)),
which uses the same bit-pattern semantics — so `'?'` renders as the player's own
team color:

- `TEAM_RED  → '?' = '4' → renders red`
- `TEAM_BLUE → '?' = '1' → renders blue`
- `TEAM_FREE → '?' = '7' → renders white`

So `cg_teamColors "?????"` paints every teammate's head/body/legs/color1/color2
in their team color, and `cg_enemyColors "1????"` always paints enemy heads
blue while letting the rest of the body and the rail colors follow whichever
team the enemy happens to be on.

---

## 6. UI slider translation tables

The settings menus don't expose game codes directly. A pair of translation
arrays sits between the slider position the user sees and the `color1` /
`cg_crosshairColor` cvar value the engine stores. The arrays live in a
single shared header used by all three UI surfaces (baseq3 preferences,
baseq3 player settings, missionpack main UI):

[code/game/ui_swatches.h](../code/game/ui_swatches.h):

```c
static const int gamecodetoui[] = {4,2,3,0,5,1,6};
static const int uitogamecode[] = {4,6,2,3,1,5,7};
```

`uitogamecode[ui_slot]` → the cvar value to write when the player picks that
slot. `gamecodetoui[cvar_value - 1]` → the slider position to show for that
cvar value. Decoded against `CG_ColorFromString` semantics:

| Slot (visible swatch) | `uitogamecode[slot]` | `CG_ColorFromString` renders |
|---|---|---|
| 0 (red)     | 4 | red     |
| 1 (yellow)  | 6 | yellow  |
| 2 (green)   | 2 | green   |
| 3 (cyan)    | 3 | cyan    |
| 4 (blue)    | 1 | blue    |
| 5 (magenta) | 5 | magenta |
| 6 (white)   | 7 | white   |

Every entry is consistent with §4. The UI was built for the bit-pattern
parser, and the spectrum order it presents is a deliberate UX choice — the
internal cvar storage just doesn't share that ordering.

---

## 7. Pastel team tint

[`CG_TeamColor()` cg_drawtools.c:871-887](../code/cgame/cg_drawtools.c#L871-L887)
returns a small set of fixed pastel tints, with no numeric-character input:

| Team | RGB |
|---|---|
| `TEAM_RED`       | (1.0, 0.2, 0.2)   |
| `TEAM_BLUE`      | (0.2, 0.2, 1.0)   |
| `TEAM_SPECTATOR` | (0.7, 0.7, 0.7)   |
| default          | (1.0, 1.0, 1.0)   |

Used for HUD/scoreboard team text and team-marker UI elements. This is
separate from any of the four numbering schemes — it's just a hard-coded
visual identity for the red/blue teams that doesn't interact with `color1`,
chat escapes, or model tinting.

---

## 8. Engine vs. mod split-brain on `^8+`

Trinity is a pure mod. The `code/` tree contains only the QVMs (`cgame`,
`game`, `q3_ui`, `ui`); the engine is a separate binary, typically
[trinity-engine](https://github.com/ernie/trinity-engine) (a Quake3e
derivative). The engine ships its own copy of `g_color_table` with more than
eight entries, so its renderer can produce colors for `^8`, `^9`, and beyond.

This creates an asymmetry inside Trinity:

- **Engine-rendered text** (console scrollback, server browser, native pause
  menu, connect screen): uses the engine's extended palette.
- **cgame-rendered text** (in-game HUD, scoreboard, chat overlay, frag feed,
  player names above heads): parses the escape itself, then indexes into this
  repo's 8-entry `g_color_table` with `ColorIndex(c) & 7`. So `^8` wraps to
  black and `^9` wraps to red, regardless of what the engine could otherwise
  show.
- **q3_ui- and ui-rendered text** (menu screens, server list rows): same
  8-entry path as cgame, same wrap behavior.

A practical test: send `say "^8orange ^9purple"` in chat, then open the
console with `~`. The scrollback (engine path) shows the extended colors; the
in-game chat overlay (cgame path) shows the wrapped colors. Both surfaces are
rendering the exact same string.

Original id Quake 3 did not have this asymmetry — its engine also used an
8-entry table with `& 7` wrap, matching the mod. The split-brain in Trinity
is downstream from the Quake3e/CNQ3 lineage adding extended palettes without
coordinating with mod-side rendering. A fix is tracked separately in
[trinity-engine](https://github.com/ernie/trinity-engine).

### Portability

Historically, many Quake 3 engines (Quake3e, ioquake3 forks, CNQ3, ETe, etc.)
extended `^N` past 7 to varying degrees, primarily for CPMA compatibility.
Trinity-engine honors Quake3e's CPMA-compatible 32-color mapping for `^8`+.
Other engines extend differently or not at all, and the chat strings travel
over the network as raw bytes — so a player on a different engine reading the
same chat sees different colors, or no colors past 7. **Anything that needs
to render the same way everywhere should stick to `^0`-`^7`.**
