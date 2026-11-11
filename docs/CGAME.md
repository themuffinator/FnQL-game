# Native cgame bring-up

The x86 `fnql_cgamex86.dll` builds and runs through active play in FnQL with
retail or FnQL-game qagame. This establishes hosting, not M7 completion.
Prediction, events, scoreboards and visual parity remain unfinished. The
module advertises capability bits **zero**.

## Contract and evidence

The retail cgame HLIL corpus in QuakeLive-SRP is behavioral evidence only;
no reconstructed implementation was incorporated.

| Surface | Evidence and implementation |
| --- | --- |
| Native entry | Retail `dllEntry` 0x10020a70, table 0x100769a8: API 8, 21 exports, slot 16 NULL. Typed imports follow FnQL's 128-slot retail contract. No exported `vmMain`. |
| Snapshot | FnQL retail bridge: 384 entities at offset 0x280, total size 0x16488. Explicit padding preserves the packet without changing shared playerState. Invalid counts/client/entity indices are rejected. |
| Renderer config | Retail-facing FnQL glconfig: `multitextureAvailable` before viewport, no inherited `smpActive` tail; size 0x2c44. |
| Chat | Retail 0x100209e0–0x10020a32: event mode 5 uses y=455, width=300, 30 characters; normal uses 413, 640, 73. Y/width return floats. |
| Identity | Retail 0x10020910: 96-byte output, client/transport/identity halves/two 40-byte names. Decimal `st` rejects overflow. Transport semantics remain open. |
| Speaking | Retail 0x10020a40: retain state and update time with bounds checks; HUD indicator remains open. |
| Text | Slots 123/124, including the five-float bounds/ascent output and the preserved packed float width/height helper. FnQL `docs/fnql/FONT_COMPATIBILITY.md` documents negative unlimited glyph limits; legacy menu zero translates to -1. Font handles 0/1/2 select normal/sans/mono. |
| Menus | Constants from installed `pak00.pk3/ui/menudef.h`, regenerated/checked by `extract_retail_contract.py`. No menu assets redistributed. |
| Assets | Retail pak00 contains PNG player icons and numbered `gfx/2d/crosshair1` through `crosshair29`, not lettered crosshairs. |
| Settings | Shared parsers consume CS_PMOVE_SETTINGS/CS_WEAPON_SETTINGS and invalidate prediction caches. Item reach/respawn and tiered armor use server configuration. CS_WARMUP uses the extracted `\\time\\%i` infostring contract. |

Dormant Team Arena code needed duplicate-member, callback, obsolete-enum and
shader-time union build repairs. Retail `font` and `widescreen` keywords
parse, left/right anchoring works, and text uses native font imports. Full
stretching and ownerdraw behavior remain open. Missing legacy bitmap-font
configs have a charset fallback. Shared menu callbacks remain optional.

## Validation

On 2026-11-05, after the flag-status, teammate-marker, electro-gib,
lightning, damage-plum, `+pstats`, speaking/muted, crosshair-name and cursor
slices, a clean rebuild passed the unit, ABI, contract, parity and every
optional cgame/UI hosting suite **94/94** in one run
(`.tmp/cgame/poi/final-all.log`). An earlier run of the same set
(`hud-final.log`) had one FnQL-game frozen-player timeout at 90.38 s; the
unchanged probe passed on rerun (`.tmp/cgame/poi/frozen-recheck/`), as
before. The objective-marker follow-up is in `.tmp/cgame/poi/objectives.log`.

On 2026-11-05, after the world-ping/objective-sound slice (before its
lead-voice and cursor follow-ups), the same suite set plus the two new POI
hosting probes passed **88/88** in one clean run, with no reruns. The run
included the race-marker hosting gates that exercise the shared projection
refactor. Log: `.tmp/cgame/poi/regression.log`. The follow-up rerun after
the lead-voice and cursor changes rebuilt the whole tree and passed
**68/68** (unit, ABI, contract, parity, cgame-hosting, scoreboard and
effects/POI hosting): `.tmp/cgame/poi/final.log`.

On 2026-11-05, `meson test -C build/win32-x86 --suite unit --suite abi
--suite contract --suite parity --suite cgame-hosting --suite ui-hosting
--suite cgame-scoreboard --suite cgame-stats --suite cgame-match
--suite cgame-effects-hosting --suite cgame-race-hosting`
passed **81/83** after the pregame slice. Its new unit gate and all four new
hosting gates passed. The unchanged UI native unit executable crashed with
`0xc0000005` (Windows reported an unknown module at offset zero); twenty
logged direct rechecks and the parallel **35/35** unit/ABI/contract/parity
recheck passed. No root cause has been established. The FnQL-game frozen
probe timed out at 90.37 seconds without the required local-player obituary;
the log shows continued bot activity. The unchanged frozen probe passed on
rerun in 43.37 seconds. Retail frozen passed in 19.33 seconds, and
damage-through passed with both providers (15.02 / 23.54 seconds).
Evidence is retained in `.tmp/cgame/pregame/regression.log`,
`unit-recheck.log`, `ui-recheck.log`, `ui-failure-event.txt` and
`frozen-recheck.log`. Every check now has a passing result, but this is not a
clean 83/83 single-run result; the autonomous-bot fixture remains timing-sensitive.

The previous run passed **78/78** after the standings slice, including its production unit
gate and all four new individual/team hosting gates. The retained full log
is `.tmp/cgame/standings/regression.log`. Both frozen-player gates passed
(20.34 / 34.73 seconds), without a rerun. The x86 module build, probe syntax
check and `git diff --check` also passed. Matched captures and the native
clipping correction are described below.

The previous run passed **73/73** after the pause/server-notice slice, including both new
time-in gates. The retained full log is
`.tmp/cgame/matchnotices/regression.log`. Both damage-through checks passed
(12.36 / 16.50 seconds), as did both frozen-player checks (50.31 / 29.93
seconds).
After the final multiline-bounds hardening and reuse of the existing
announcement-reset helper, all five focused native/event/clock and time-in
hosting checks passed again in `.tmp/cgame/matchnotices/final-focused.log`.

The previous match-clock run passed **70/71**. All five new clock checks
passed. The FnQL-game frozen-player probe timed out at 90.33 seconds without
the required local-player obituary; the retained output shows continued bot
combat. The retail frozen-player probe passed in 18.33 seconds. The full log
is `.tmp/cgame/matchclocks/regression.log`; focused clock results are in
`focused.log` in the same directory. The unchanged frozen-player gate passed
on rerun in 74.27 seconds, retained as `frozen-recheck.log` there. All 71
checks therefore have passing results, but this is not a clean 71/71
single-run result; the autonomous-bot fixture remains timing-sensitive.

The previous weapon-selection run passed **66/66**, including both new
spawn/loadout hosting gates. The retained log is
`.tmp/cgame/weaponselection/regression.log`; the focused four-test run is
`focused.log` in the same directory. Both damage-through gates passed on
this run (9.14 / 20.70 seconds), as did both frozen-player gates
(30.02 / 41.23 seconds).

The earlier weapon-bar run passed 62 of 63 checks. The FnQL-game
`damage-through` probe timed out at 90.30 seconds without its required
`EV_MISSILE_MISS_DMGTHROUGH`; the retained output shows continuing combat,
not a module error. The full log is `.tmp/cgame/weaponbar-tests.log`.
The unchanged damage-through gate passed on rerun in 9.45 seconds, retained
as `.tmp/cgame/weaponbar-damage-recheck.log`. This is not a clean 63/63
single-run result. Both frozen-player gates passed (38.57 / 45.02 seconds). The earlier
status-HUD and native-font runs passed 62/62 and 59/59, retained respectively
as `.tmp/cgame/statushud-tests.log` and `.tmp/cgame/font-tests.log`.
Autonomous-bot event fixtures remain timing-sensitive: the earlier
ammo/material run required an unchanged frozen-player rerun, recorded in
`.tmp/cgame/tests.log` and `.tmp/cgame/frozen-recheck.log`. The native adapter
test covers exports, float returns, loop clearing, user-command arguments,
text limits/results, projection slots, snapshot bounds and identity. Shared movement lockstep
still passes; this is **not** client prediction G4.
After the final race icon-anchor correction, all seven focused native/race
unit and hosting checks passed again. That log is retained separately as
`.tmp/cgame/race-presentation-tests.log`.
After the view slice's final malformed-projection guard, its two focused
FOV/native checks passed again in `.tmp/cgame/fov-tests.log`.

The earlier concurrent `game_quadhog.exe` link failure for
`g_neutralFlagPingRate` is resolved in this full-tree run. The additional
`cgame-scoreboard` suite passes four FFA/TDM probes, covering both qagame
providers and eight consecutive HUD reloads in each process.
The reload probes also verify that a missing custom HUD loads the default
without dropping the connection; the inherited fatal-error call prevented
that advertised fallback before this fix.

`scripts/cgame_probe.py` runs windowless (`com_headless 2`) with mouse/joystick disabled and no
input injection, mounts legitimate retail assets, joins FFA, verifies module
identity and active state, captures with engine `screenshotJPEG`, and exits.
Both qagame providers passed on Campgrounds. Captures confirmed world,
viewweapon and readable health/ammo/armor text, while exposing incomplete HUD
presentation. Meson's optional `cgame-hosting` suite runs both combinations.

Until provider/purity work lands, the probe copies the DLL as `cgamex86.dll`
only inside its disposable, explicitly unpure home. Retail files are untouched.

## Native HUD text metrics

Retail cgame 0x100082b0/0x10008440 computes font pixels as
`scale * vidHeight / 768 * 96`. Its import-124 call at 0x10008327 supplies
five floats: left, top, right, bottom and ascent. Width is right minus left
converted to HUD units; height uses ascent, not the visual bounding-box
height. The shared bridge in `cg_text.c` now independently applies this
policy for both measurement and painting. At 960x600, scale 1 requests 75
pixels instead of the previous 60-pixel paint and 48-pixel measurement.
Measuring at the actual rendered size also preserves font rasterization and
rounding before converting the result to logical HUD units.

The existing centered/anchored HUD canvas is preserved, including tall-screen
letterboxing. Shadow styles 3 and 6 preserve alpha and offset by one/two
logical units on each axis. Invalid size/coordinates are rejected before
native integer conversion. Font selection, unlimited-limit translation,
whole-string native color/UTF-8 handling and the legacy glyph-quad helper
remain available. The packed return helper used by damage plums is unchanged.
FnQL already packs visual width and ascent; using the bounds output aligns
with retail cgame's observed call contract rather than changing those two
quantities on FnQL.

A separate engine robustness issue was found during review:
`code/renderercommon/ql_font_text.h:QL_FontScaleTenths` casts the requested
size to integer tenths before clamping to its short-sized glyph-cache range.
Extreme finite inputs can overflow that conversion. This pre-existing host
issue is not exercised by ordinary HUD sizes and remains to be repaired at
the engine boundary for all native callers.

The production `cgame-text` gate covers physical sizing, negative left
bearings, ascent versus visual height, logical width/height conversion,
font handles, glyph limits, shadows/alpha, wide/tall/unequal-axis canvases,
legacy glyph quads and ASCII extra spacing, and invalid values. The native
adapter test verifies exactly five initialized output floats with adjacent
canaries, while retaining coverage of the packed-return consumer.

Windowed 960x600 gravity-only captures with the same engine, race fixture and
retail qagame compare FnQL-game cgame against installed retail cgame:
`.tmp/cgame/fonts/{fnql,retail}/{qconsole.log,capture.jpg}`. Both probes
passed and removed their disposable profiles. The race label and respawn
hint now show matching font size and placement. The elapsed digits and
checkpoint animation phases differ, so these are visual comparisons, not
pixel-identical frames. At that stage, differences remained in top-left player rows, weapon/ammo
presentation, health/armor alignment and bars; the following status-HUD
slice addresses the bottom-HUD values, bars and tinted images. Complete HUD
parity remains open.

## Status values, bars and tinted HUD images

`cg_statushud.c` supplies the three player values, four health/armor bar
ownerdraws, and `CG_TEAM_COLORIZED`. The installed HUD previously reached
missing bar/image handlers and the values still used Quake III's centered
rectangle layout. The independent implementation follows shipping cgame:

- Armor 0x1002e500, ammo 0x1002e7c0 and health 0x1002fdf0 use the item origin
  as a left/center/right anchor, with the text baseline at origin plus font
  ascent. A caller-supplied value image retains its original rectangle/color.
- Ammo selects the viewed snapshot client's current entity weapon, excludes
  gauntlet/grapple, and treats -1 as infinite. The square infinite icon uses
  **rectangle height**, with integer anchor truncation before and after
  alignment (0x1002e90d/928/963). QLSRP's width-based reconstruction differs
  from the shipping binary. The installed asset is `icons/infinite.tga`,
  registered by retail at 0x100232b1. Other negative ammo suppresses drawing.
- Primary health fills from the left relative to `STAT_MAX_HEALTH`; primary
  armor fills from the right relative to 100. Excess segments fill from the
  bottom over **22/32 of the texture height**, with matching cropped UVs
  (0x1002ed50, 0x1002ee50, 0x1002efb0, 0x1002f0c0). Shipping x87 operations
  retain the maximum on the floating-point stack: the HLIL loses that value,
  and QLSRP omits the 0.6875 factor. Health excess is capped at its maximum;
  armor above 200 retains retail's uncapped bonus segment.
- `CG_TEAM_COLORIZED` (0x1002f5d0, dispatch 0x1003b5ca) tints the supplied
  image using the retail free/red/blue/spectator palette and caller alpha.
  Bars use the same palette at full alpha. The inherited `CG_TEAM_COLOR`
  background path and world/team effect colors remain separate.

Bounds checks prevent malformed client/weapon indices from indexing ammo
arrays, including the shared menu color-range query. Invalid rectangles are
ignored, arithmetic uses a wide intermediate before subtraction, and missing
bar/colorized images fall back to the existing white shader. Invalid maximum
health falls back to 100; nonpositive fills are skipped. These are explicit
robustness policies for malformed inputs, not inferred retail behavior.
The focused gate covers routing, anchors, image overrides, infinite ammo,
all weapon classes, bar UV/geometry, bonus caps, team colors/alpha, screen
transforms, malformed indices and missing snapshots.

`--status-hud` loads the hand-authored `status_hud.factories` fixture for
bonus health/armor and finite ammo, verifies its cvars, and captures without
movement/input injection. The warmup loadout supplies 150 machinegun ammo
in the retail-qagame captures despite the fixture's configured starting ammo
of 23. No claim is made that the capture exercises a 23-ammo player state.
Evidence is `.tmp/cgame/status-hud/{fnql,retail}/{qconsole.log,capture.jpg}`;
both show 149 health/armor at capture and visually matching bottom-HUD
values, bonus bars, colors and icons. Spawn positions differ. The companion
`fnql-race` capture shows unlimited ammo with the corrected icons/outlines;
compare the earlier `.tmp/cgame/fonts/retail` race reference. These establish
the affected presentation, not complete HUD parity. At that stage the
weapon list was still missing; the following slice supplies it. Top-left
rows, timer and warmup messages still visibly differ.

## Weapon-list presentation

`cg_weaponbar.c` now supplies retail's persistent left/right/center lists
and timed strip. The default fading baseq3a selector hid the list between
weapon changes, leaving all persistent-list modes absent. Evidence is the
shipping wrapper at 0x10053960 and list renderer 0x10053100:

- `cg_weaponBar` defaults to 1, range 0..4; `cg_drawFullWeaponBar` defaults
  to 0, range 0..1; `cg_lowAmmoWeaponBarWarning` defaults to 2, range 0..2
  (tables 0x10078248, 0x100770f0, 0x10077858). Mode 0 hides, modes 1/2/3
  anchor left/right/center, and mode 4 fades over 1400 ms after selection.
  The outer gate checks predicted health, not the selection timestamp;
  QLSRP's additional timestamp gate does not match the shipping wrapper.
- Persistent lists use 16-unit icons, 22-unit vertical spacing or 55-unit
  horizontal spacing, at y=94 or y=413. They exclude the gauntlet, use
  snapshot ownership/ammo, and register owned weapon assets. The full-list
  option includes already available map icons without registering unowned
  weapons. Practice flag bit 0 hides unowned icons while retaining spacing.
- Local selection highlights the pending selection. Follow/demo views use
  the snapshot weapon; follow/spectator lists move to y=452, center, ignore
  the full-list option and suppress themselves while the scoreboard shows.
  Follow text alpha distinguishes current versus other weapons.
- Ammo uses scale .24 and a 13-unit baseline offset; the right list measures
  and right-aligns it. Every negative ammo value draws the installed infinite
  icon, matching 0x10053505. Empty-ammo red and low-ammo yellow follow retail
  mode tests. Unlike the main warning, the low-ammo list threshold truncates
  the capacity/percentage product at 0x100536e6; yellow requires float mode
  exactly 2 at 0x1005375b.
- The timed strip includes the gauntlet, uses 32-unit icons, a 40-unit
  selection frame, empty-ammo overlays and the selected weapon's name. Native
  .25 text uses integer centering and baseline 374 (0x10053b4b/0x100128b0).
  Only this strip clears item-pickup text; persistent lists do not.

The inherited `cg_drawWeaponSelect` remains a master hide switch at 0.
Its default value 1 delegates to `cg_weaponBar`; other nonzero values retain
baseq3a's counter/vertical/always-on layouts. This intentionally changes the
old default to the retail persistent list while preserving explicit alternate
layouts. Unsafe weapon indices, extreme timer differences and `abs(INT_MIN)`
are bounded. Missing icons are skipped; a missing list-selection image falls
back to the existing selection shader. Numeric output has room for a complete
signed integer instead of retail's four-byte buffer.

The production `cgame-weaponbar` gate exercises all four layouts, mode 0,
legacy overrides, full-list registration/spacing, practice filtering,
follow/demo selection and alpha, empty/low/infinite ammo, fractional settings,
wide-screen anchors, strip fade/name/pickup behavior and malformed values.
Windowed 960x600 captures in `.tmp/cgame/weaponbar` compare `fnql-left`
against the earlier `.tmp/cgame/status-hud/retail` reference, plus paired
`{fnql,retail}-{right,center,strip}` captures. These show matching list and
strip presentation; spawn positions differ. The paired `*-full` Race
captures confirm infinite-ammo rendering with the full-list option enabled,
but contain no unowned registered weapons and do not prove that case alone.
The additional paired `*-unowned` captures use `--status-hud
--starting-loadout --full-weapon-bar` and the hand-authored
`weaponbar.factories`. Loading `g_loadout 1` at map start retains a limited
kit while map icons remain registered. Both modules show machinegun ammo 23
in yellow, unowned icons with no counts, and selected heavy-machinegun ammo
50 in the same order and layout. The earlier attempt to change training mode
after spawn did not change the inventory; retail also reset training mode
when supplied by the factory, so that attempt is not validation evidence.
The `fnql-provider` capture repeats the limited-loadout fixture with
FnQL-game qagame and cgame together; the probe passes and shows the same
weapon-list inventory, counts and highlighting.
All capture profiles are disposable; each evidence directory retains only
`qconsole.log` and the engine-rendered `capture.jpg`.

## Weapon commands, spawn preferences and loadouts

`cg_weaponselection.c` replaces the inherited command policy using shipping
code at 0x10053ba0–0x10053d8e. Cycling and direct selection now use snapshot
inventory/ammo instead of predicted inventory. Cycling skips the gauntlet,
empty weapons and reserved slots, preserves selection when no candidate
exists, and updates the selector timestamp only on success. Direct selection
updates the timestamp before ownership/ammo rejection. The commands reject
follow mode; retail has no separate demo-playback exclusion. Every nonzero
ammo value, including negative values, passes the shipping ammo test.

`cg_switchToEmpty` and `cg_switchOnEmpty` both default to 1, range 0..1
(tables 0x10077f00 and 0x10077ee8). The former governs direct selection,
not cycling or the automatic highest-loaded-weapon fallback. Disabling the
latter plays the installed no-ammo sound on `CHAN_AUTO` instead of changing
selection (event dispatch 0x1001a6bc–0x1001a6ec). Snapshot-client events still
apply when following or playing a demo. Pickup autoswitch now includes the
machinegun (0x10019c8a); its default is 0, matching table 0x10076b08, rather
than baseq3a's enabled default. Explicit enabled settings continue to work.

`weapon toggle` uses the previous selection observed by a rendered frame,
not the immediately preceding command. Shipping 0x1004e756 updates that
history after packet entities; multiple selections in one frame do not
invent intermediate history entries. The command's case-insensitive match
uses the first four characters (0x10053c93). With empty switching disabled,
zero previous ammo falls back to `cg_weaponPrimary`; missing ownership with
nonzero ammo does not. If that empty previous weapon already is the primary,
retail reads a second fallback initialized to zero at 0x10029a3a. Only that
initialization and the fallback read were found in the inspected executable;
the rewrite therefore uses zero, rather than QLSRP's third history entry.
This is the current static-evidence interpretation.

Spawn preferences accept at most fourteen short-name or numeric tokens from
`cg_preferredStartingWeapons`, choosing the first owned weapon even if empty,
then falling back to the server's selected weapon (0x10043300). The short
names are the shipping descriptor tokens at 0x10079094 with 48-byte stride.
`cg_weaponPrimaryQueued` accepts its first named token only (0x10001df0).
That queued primary is now passed through native user-command import 91;
the old adapter always sent zero. Respawn publishes the preceding frame's
queued primary to read-only `cg_weaponPrimary` (0x100433d0). The native
adapter bounds the primary without changing any shared ABI structure.

Per-weapon `cg_weaponConfig_<token>` commands, falling back to
`cg_weaponConfig`, run once per observed selection change, with a trailing
newline (0x1004e67e–0x1004e75c; tables 0x10078278–0x100783d0). All default
to empty. User command text remains intact, including intentional command
separators. Frame history also observes pickup, drop and spawn selection.

The rewrite intentionally bounds malformed indices instead of reproducing
retail's reverse-cycle slot-16 read or accepting reserved slot 15 as a
weapon. Decimal prefix parsing preserves valid `atoi` syntax while rejecting
overflow. QLSRP's empty-weapon cycling, eager history update and additional
fallback rules were not adopted.

The production `cgame-weaponselection` gate exercises contradictory snapshot
and predicted inventories, both cycle directions/wrap, empty/infinite ammo,
toggle/frame history, primary fallback, follow/demo policy, timestamps,
no-ammo sound/fallback, pickup autoswitch, spawn token order/limits, primary
parsing, config dispatch and invalid indices/numbers. `cgame-native` verifies
nonzero primary forwarding and its invalid-value fallback alongside existing
sensitivity/FOV checks.

`cgame_probe.py --weapon-selection` sets a rocket primary loadout and a
gauntlet spawn preference through configuration, then checks the gauntlet's
config marker and `cg_weaponPrimary 5`. It uses no movement, weapon or button
input commands. The two new `cgame-hosting` gates cover both qagame providers.
Retained windowed captures/logs are in
`.tmp/cgame/weaponselection/{fnql,retail,fnql-provider}`. All three probes
pass. The engine captures show the selected gauntlet, machinegun ammo 23
and rocket ammo 5, confirming that the queued loadout reached the server;
the same fixture without that queued primary previously spawned with HMG.
Different spawn positions and remaining HUD gaps prevent a full visual-parity
claim. Live cycle/toggle input comparisons remain unperformed; their current
evidence is shipping disassembly and production-code tests.

## Match clocks and round labels

`cg_matchclocks.c` supplies the previously missing `CG_LEVELTIMER`,
`CG_ROUND`, `CG_ROUNDTIMER`, `CG_OVERTIME` and `CG_LOCALTIME` ownerdraws.
The level clock follows shipping 0x10029770/0x10030c00: elapsed time without
a limit, otherwise countdown by default (`cg_levelTimerDirection 1`, table
0x100777b0). Direction 0 counts up until the limit; both directions then
count elapsed overtime from zero. Match time freezes at the timeout-start
timestamp. Ordinary warmup shows `0:00`; a round break after round zero keeps
the match clock running. Times before the level start clamp to zero instead
of displaying retail's malformed negative clock digits.

Private clock state consumes initial and changed configstrings, and resets
on map restart. `CS_WARMUP` carries `time` and resets the round number;
`CS_MATCH_STATE` carries `time`/`round` and takes precedence at initialization
for CA, Freeze, Attack & Defend and Red Rover. Attack & Defend adds one to
the published round number (0x10048c80/0x10048d10/0x100497df–0x100497ff).
The remaining inputs are level start (13), round start (0x296), timeout start
(0x29d), timeout end (0x29e), and serverinfo `timelimit`, `roundtimelimit`,
`g_overtime` (0x10048a27/0x10048a4f/0x10048b03 and 0x10049a85–0x10049cd8).
This sidecar does not replace the inherited warmup-message/announcer logic;
those remaining paths need their own compatibility work.

The round label is `Warmup` for nonpositive round numbers, otherwise
`Round N` (0x100336e0). It and the round countdown are restricted to the four
round modes above. The countdown shows red whole seconds only during the
last 1..29999 milliseconds, including a final `0`, with no round warmup
(0x10033b10). The private round-start timestamp follows retail's timeout-HUD
adjustment after painting: add frame time during an indefinite pause, stop
adding when a time-in deadline is supplied, and apply the final correction
when timeout start clears (0x10011173/0x1000ef30). Server round-start updates
replace the local adjustment. Duplicate stereo draws do not double-advance
it, and clock rewinds do not add negative frame time.

Overtime is derived from complete seconds beyond the configured match limit,
not the warning-sound flags or an event-maintained counter. A positive limit
and overtime duration are required; round modes and warmup suppress it.
The label appears after the first complete second and gains ` xN` at exact
duration multiples (0x10030d20/0x1002a5f0). Constant-time division replaces
retail's per-second counting loop. QLSRP's warning-flag gate and different
round-timeout gate were not adopted.

Text uses the item origin as alignment anchor and its y coordinate as the
baseline, with the supplied scale, color and style. Local time follows
0x10033d80's `HH:MM (Mon DD, YYYY)` format through the native real-time import;
invalid calendar fields are hidden. Clock arithmetic uses 64-bit
intermediates, bounded formatting, and checked signed-decimal config values.
Malformed updates retain prior state; absent fields retain retail's zero
default. The legacy `Com_sprintf` formatter does not support 64-bit formats,
so this native-only handler uses bounded C-library `snprintf`.

`cgame-matchclocks` tests production parsing and painting, both directions,
limit/overtime boundaries, all game-mode gates, warmup/round transitions,
timeout/time-in adjustments, authoritative replacement, stereo/rewind behavior,
alignment, calendar bounds and extreme numeric input. The windowed
`--clock-hud round` and `--clock-hud timeout` fixtures have hosting gates for
both qagame providers. They use a paused bot and no movement/button input.
The timeout fixture takes two engine screenshots 250 frames apart and
requires the server's timeout-start configstring. The finish-script retry
loop now stops when an explicit start marker appears, preventing repeated
insertion from starving waits in longer capture scripts.

Matched retail-cgame captures/logs live in
`.tmp/cgame/matchclocks/{fnql,retail}-{round,overtime,timeout}`. Round captures
show `4:55` and red `21`; overtime captures show `0:08` and `Overtime`.
Both timeout captures retain `4:55`/`21` across the two screenshots.
`fnql-provider` repeats the timeout capture with FnQL-game qagame. These
probes pass, while different spawn positions and remaining HUD differences
preclude a full visual-parity claim. The subsequent notice slice below adds
time-in comparisons. Local wall-clock comparisons remain unperformed.

## Normal-match warmup and ready-up presentation

`cg_pregame.c` replaces the inherited Q3 warmup painter and its separate
frame-based countdown audio. Retail 0x1000f090 uses two sans-font lines at
y=88/108, scale 0.35, for waiting messages. It counts valid clientinfo team
members, checks per-team minimums/balance, handles Duel overpopulation and
Infected RR's combined population, and distinguishes waiting for players
from waiting for readiness. CS_READY_UP (0x2c4, read at 0x10049a6c) adds
the singular/plural deadline message. `cg_drawPregameMessages` controls those
two lines; it does not hide the ready prompt or countdown.

The ready hint at y=124, normal font scale 0.25, uses the local clientinfo
team and `rp` configstring field, not the snapshot EF_READY bit. It reads the
existing `readyup` key name through native imports without setting bindings
or acquiring input. Not-ready text is red; ready text is green (retail color
records 0x100761b4/0x100761c4). Spectators and insufficient populations have
no hint. Scores and the cgame menu catcher suppress presentation while the
countdown state continues to advance. Specialized training/tutorial screens
and the complete retail menu-capture state machine remain open.

Countdown titles follow 0x1000ec00: Duel names, the 13 mode names, Quad Hog,
Infected, AD offense/defense, Freeze with zero round delay, and later-round
or sudden-death labels. Titles use normal font scale 0.6 at y=90; counts
use a fixed 0.45 at y=125. Round-mode later rounds show a bare count; initial
countdowns use `Starts in: N`. Retail's Race vertical offset is **10**
(float 0x10078fbc), unlike QLSRP's reconstructed waiting offset of 30.
Retail's serverinfo minimum is **g_teamSizeMin** (0x10048993), not the
reconstruction's `teamsize` alias. Counts include the exact timestamp boundary
using `(warmupTime - time + 1000) / 1000`; wide arithmetic avoids overflow.

CS_WARMUP's optional `g_gametype` changes the title and initial preparation
choice; missing values mean the current mode. CS_MATCH_STATE supplies the
shared warmup time, round, turn and state, with AD's one-based presentation
round. Parsing rejects malformed/overflowing updates without partially
replacing state. Player indices and team counts are bounded independently
of untrusted serverinfo limits. Initial preparation follows 0x10048d70:
`prepare_to_fight` versus `prepare_your_team`. Later-round five-second cues
use `round_begins_in`, with Freeze delay rules; three/two/one are emitted once
per observed count. Round zero and non-round warmup-free map restart share
0x1000ea40's match-start handler. The old Q3 prepare/count/fight path is
removed, retaining the unrelated client-change reset helper. Timeout
behavior keeps its existing production tests.

The `cgame-pregame` gate covers all mode titles, player/team shortages,
balance, Duel excess players, Infected counts, ready/deadline states,
color/font/baselines, override parsing, countdown boundaries and rewind,
round/Freeze/AD/sudden-death variants, audio dispatch, restart, visibility and
invalid inputs. The four `--pregame-hud ffa|ca` hosting gates use a bot and
server-side `allready`, requiring positive CS_WARMUP, three engine captures
and normal exit. Bot pausing happens **after** it joins; the first attempted
fixture paused too early and left it spectating. That failed fixture log is
retained as `.tmp/cgame/pregame/fixture-failure.log`. Captures are now ordered
by write time because same-second engine filename suffixes sort incorrectly.

Matched retail/FnQL cgame captures with retail qagame are retained under
`.tmp/cgame/pregame/{retail-ffa,fnql-ffa,retail-ca,fnql-ca}/` as
`capture-waiting.jpg`, `capture-ready.jpg` and `capture.jpg` (countdown).
Waiting lines, ready hints, titles and the seven-second countdown match in
the reviewed frames. The retail FFA frame additionally shows a join notice;
console-overlay and world/spawn differences remain. These captures establish
the scoped presentation, not full-frame parity or actual retail-engine
hosting. Audio calls are unit-checked; no recorded-audio parity is claimed.

## Top-left standings, mode icons and counters

`cg_standings.c` supplies the installed HUD's first/second-place rows, mode
icon, win limit and remaining-player/owned-point counters. Retail functions
0x10032520/0x100329a0 select team standings for TDM through AD, excluding
Race and Red Rover. Red wins a team tie. Individual rows use the server's
first/second names (CS 0x293/0x294), but the second row shows the snapshot
player's rank/name/score unless that player is uniquely first or a spectator.
Race uses the client's recorded time, including retail's distinct zero-time
rules for the first and second rows. Missing-score sentinels and tied ranks
retain their shipping behavior. Invalid local client/rank indices hide the
row instead of reading beyond its arrays.

Team labels reserve six leading spaces and remain unclipped; team scores
use at most four characters. HUD overrides are `cg_redTeamName` and
`cg_blueTeamName`, both defaulting to empty with flags zero (retail cvar
records 0x10077ab0/0x10076b68). They are separate from inherited `g_redteam`
and `g_blueteam`. The absent-score branch reverses the normal custom-name
test; production tests preserve that observable shipping branch.

Individual names use native clipped text (import 123, helper 0x100087c0),
normal font and no shadow, with separately painted rank and score. The score
uses alpha one without mutating the menu's input color. Retail reserves the
ellipsis width even when the name fits (0x10032852/0x10032dcf), but clips
through a **local** copy of the boundary. The caller's unchanged boundary
makes the equality branches 0x100328f1/0x10032e77 hide the dots. Disassembly
and matched captures confirm this; the initial approximate name-clipping
implementation incorrectly showed dots and was replaced with the native
path. The native adapter preserves UTF-8/color strings and lets the renderer
perform glyph clipping. Its optional returned boundary is transformed back
to the HUD canvas and covered independently by text/native tests.

The mode icon follows 0x10034840/0x100232cb; Overload uses the FFA image.
The limit follows 0x10032260: capture modes use capturelimit, CA/FT/RR use
roundlimit, DOM/AD use scorelimit, and other modes use fraglimit. Race uses
CS_RACE_RECORDS minus passed points, retaining negative results; subtraction
uses 64 bits to avoid overflow on malformed/extreme inputs. Unlike QLSRP's
reconstructed formatting, retail uses `%i`, without a two-character minimum.
Remaining-player counters use CS 0x297/0x298 and round modes 4/9/11/12;
owned-point counters reuse CS 0x2bc/0x2bd in DOM/AD. Counter origins are text
baselines, not the bottom of the menu rectangle. Round-counter, Domination,
local-first/not-first and red/blue-leading visibility bits are now handled;
broader combinations with inherited visibility flags remain to be audited.

The `cgame-standings` production gate covers every mode, icons and limits,
transactional serverinfo, rejected numeric updates, score ties/sentinels,
local/spectator/follow selection, Race times, long/color/UTF-8 names passed
to clipping, caller-color preservation, counters and invalid geometry.
`cgame-native` and `cgame-text` cover the clipped import and physical/logical
boundary conversion. Four `--standings-hud individual|team` hosting gates
exercise a paused bot with both qagame providers and require successful
configuration, an active match, an engine screenshot and normal exit.

Matched retail/FnQL cgame captures with retail qagame are retained under
`.tmp/cgame/standings/`: `retail-team`/`fnql-team` for default CA labels,
`retail-custom-team`/`fnql-custom-team` for colored overrides, and
`retail-individual`/`fnql-individual` for FFA tied rows and long-name clipping.
The affected top-left fields match in these captures. Player spawn positions
and other HUD differences remain, so these are scoped presentation checks,
not full-frame parity or proof of hosting inside the retail engine.

## Pause, time-in and server center notices

`cg_matchclocks.c` now handles `pcp` and corrects the existing `cp` path.
Shipping dispatch 0x1004af3d–0x1004b035 places `pcp` at y=90 and echoes its
argument verbatim to the console; `cp` uses y=144 during warmup and 90
otherwise. Both use scale 0.4. The native center painter
(0x1000be10/0x1000beb0) adds ten pixels in Race, centers each explicit line,
uses native font ascent and six-pixel line gaps, retains at most 50 bytes
per line, and fades over the final 200 milliseconds of `cg_centertime`.
New server notices replace inherited center text, and later local center
messages replace server notices. Initialization/restart clears private
state; malformed lifetimes and clock rewinds expire it safely. No shared
layout changes are required.

The pause banner follows 0x10011173/0x1000ef30: centered at (320,128),
scale 0.5, normal font and shadow style 3. An indefinite pause displays
`Match Paused`; a timed pause displays `Match resuming in ^5N^7 seconds`
for every nonzero whole-second value. This intentionally differs from
QLSRP's final-five-seconds-only reconstruction. The zero-second interval
has no banner, and clearing timeout start hides it immediately. The banner
is independent of following and scoreboard visibility; center notices keep
the existing scoreboard suppression.

The same retail routine plays `sound/world/klaxon1.ogg` once at pause start
and samples the countdown once per second. Shipping 0x1000eb30 plays
prepare-to-fight at five, then three/two/one, and 0x1000ea40 starts a round-mode
resume notice at zero: `FIGHT!`, the AD attack/defend instruction selected
from snapshot team and match-state turn, or infected RR's `BITE!`.
RR retains its round-start effect. Pending awards/announcement queues are
cleared while a future sound deadline is retained (0x1004e180).
The existing stock/evil/female voice resolver supplies the clips, and typed
native import 40 applies `s_announcerVolume` (shipping table 0x100784d0,
default 1, range 0..2). The adapter bounds invalid volume and has a normal
local-sound fallback if that optional pointer is absent. Audio timing and
selection are unit-tested; no captured-audio parity claim is made.

Production tests cover pause/resume text, zero-second suppression, negative
and long deadlines, center alignment, multiline/truncated text, fading,
replacement/reset, all game-mode gates, AD sides/turns, RR infection, sound
deduplication and queue clearing. The native gate exercises slot 40, float
volume and fallback. `--clock-hud timein` adds two hosting gates and three
engine captures: pause, countdown, and resumed play. The server may clear
timeout start with an empty string (retail) or `0`; the gate accepts both.
Matched captures/logs are under `.tmp/cgame/matchnotices/{fnql,retail}`.
They show the same caller notice, pause banner, cyan `3` countdown and
post-time-in `FIGHT!`, with matching `4:55`/`21`, `4:55`/`20`, then
`4:54`/`20` clocks. Spawn positions and other incomplete HUD rows differ.
The final build also passed the same captures with FnQL-game qagame:
`fnql-provider` and `retail-fnql-provider` in that directory show matching
notices and clocks at all three stages (that server reports `The server`
as the caller). All images were inspected from engine screenshots.
Warmup's inherited local messages/announcer path still needs its own rewrite.

## Overhead names

Observed in retail cgame 0x1000cb60, called from CG_Draw2D (0x10010ea3)
after the world markers:

- Free-flying spectators (predicted `PM_SPECTATOR` without `PMF_FOLLOW`)
  draw nothing unless `cg_specNames` (2, 0..2) is set. For each entity up
  to `sv_maxclients` that is valid, not the snapshot client, `ET_PLAYER`,
  not `EF_DEAD`, in front of the view and has client info, the point 40
  units above `lerpOrigin` is located by its screen fractions. Retail
  engine 0x4514c0 returns them in `window[2]` (across) and `window[3]`
  (up); FnQL's renderers do the same, and FnQL-game derives them from the
  normalized coordinates so either source agrees. Retail also traces from
  the view to the player and never reads the result; FnQL-game skips it.
- Teammates (team modes, the snapshot's team) follow `cg_teammateNames`,
  spectators `cg_specNames`: mode 2 always names the player, mode 1 only
  inside 0.4..0.6 across and 0.4..0.7 up. Any nonzero mode adds the bars.
  A frozen player gets no bars; flight is never counted.
- Names are color-stripped (0x100574c0 drops `^0`..`^7` and control
  bytes), fitted in font 0 at scale 0.12 to `cg_overheadNamesWidth` (75,
  50..100) by dropping characters, then cut one character more with
  `...`. The box is the font 1 width, at least 11 units, at
  `(0,0,0,0.5)` with a one-unit pad (six units taller with bars); the name
  is shadowed one unit and colored by the viewer's team (0x10012b50:
  free white, red `1 .5 .5`, blue `.5 .75 1`, spectator `.85`, other
  `1 .8 .2`), or by the player's team for spectators in team modes.
- Bars: `gfx/2d/health_segment.tga` squares (2 units, 3 apart) for any
  health and past 25, 50 and 75% of `STAT_MAX_HEALTH`, tinted by
  0x1000ca90 against the handicapped `g_startingHealth` maximum; an armor
  strip `(0,.625,1,1)` 11 units wide at full armor, scaled below it.
  Below them come the carried powerup icons (8 units, 2-unit gaps,
  centered; the runes use their persistent items and invulnerability its
  holdable, as 0x100011c0 does), or for a teammate without powerups the
  friend marker, red `friend_hit` within `cg_drawHitFriendTime`.
- `g_startingHealth` comes from serverinfo (retail 10048a77); the
  scoreboard handicap column now hides values at or above it, as
  0x100262d9 does, instead of a fixed 100.

`cgame-overheadnames` covers the gates, band edges, placement, fitting,
bars, icon centering, the friend marker and the health tint.
`cgame-scores` checks the handicap column. With the overlay built in,
unit, abi, contract and parity (40) plus the cgame-hosting, match,
effects-hosting, scoreboard, stats and race-hosting suites (48) pass on
x86 against the FnQL no-SDL client and the retail install.

## HUD feeds and the ownerdraw inventory

Retail's ownerdraw dispatcher (0x1003b0f0) indexes a byte table at
0x1003c260 by `ownerDraw - 1` (0x225 entries). 328 ownerdraws have a
handler; the FnQL-game cases cover all but about 250. The retail menus
(`ui/*.menu` in the install) use these missing ones:

- Every in-game HUD (`hud.menu`, `comp_hud.menu`, `min_hud.menu`, and
  others): `CG_PLAYER_OBIT` (88), `CG_AREA_NEW_CHAT` (89),
  `CG_ARMORTIERED_COLORIZED` (43) and `CG_PLAYER_HASKEY` (59), all now done.
- Spectator HUDs (`spectator*.menu`, `comp_spectator*.menu`):
  `CG_SPEC_MESSAGES` (35), `CG_FOLLOW_PLAYER_NAME`/`_EX` (94/95),
  `CG_FLAG_STATUS` (337), `CG_HEALTH_COLORIZED` (338, done) and the duel slots
  `CG_1ST_PLYR`/`CG_2ND_PLYR` with scores. CG_Draw2D's spectator branch
  (0x1000dad0) paints `comp_spechud_menu`, or `comp_specfollowhud_menu`
  while following, plus the four `comp_spec{red,blue}_{top,bottom}` panels,
  instead of Quake III's SPECTATOR text.
- End-of-match scoreboards: map name (18), match details/end condition
  (8/9), winner (82), your finish line (90) and match state (339) are done;
  the winner model (85) remains; the team statistics (287-336) are
  done. The vote ownerdraws (19-34),
  best weapon, awards and player counts are done. The duel 1ST/2ND player
  statistics (103-280) are done.

Kill feed (`cg_obituary.c`). Retail's obituary (0x10019130) replaces the
inherited Quake III handler:

- Names are the player string's `n` cut to 28 characters plus `^7`. Weapon
  icons follow the means of death (`cg_weapons[].weaponIcon`); other deaths
  use `icons/icon_frag`, thaws `icons/thaw` (registered from Freeze Tag up).
- Single-party and self kills print `"%s %s."` (self kills are gendered,
  including retail's "found it's prox mine"), kills print
  `"%s %s %s%s"` with retail's message table (for example "almost dodged
  ...'s rocket", "was ripped up by ...'s HMG", "tried to invade ...'s
  personal space"), a missing attacker prints `"%s died."`.
- The killer's center print, gated by `cg_drawFragMessages` (1) and
  `cg_cameraOrbit`, uses y 90 and scale 0.3 through the native center
  painter: "You fragged %s." (team modes, Race, Red Rover), the teammate
  forms with "Watch your fire!" (not for telefrags), Freeze Tag's froze and
  thawed forms, and "You fragged %s\n%s place with %i." elsewhere. Retail's
  g_training tutorial variant is not hosted.
- A victim keeps its killer's name and the weapon icon; a following
  spectator (`EF_SPECTATOR`, `PMF_FOLLOW`) with `cg_followKiller` moves to
  the killer 400 ms later. FnQL-game skips that for world deaths, where
  retail would request `follow 1022`.
- The feed (producer 0x10018f60, drawer 0x1002e9b0) keeps the newest
  `cg_obituaryRowSize` (5, 0..5) rows for two seconds, fading over the last
  200 ms. Each row starts at the rect's truncated top: attacker in its team
  color, the icon one row-height square half a height later, then the target
  in its team color, all font 0 unshadowed; rows advance by the target's
  height plus two. Team modes strip the names' color codes. Map restarts
  clear it (0x1004a3a1).

Chat area (`cg_chatarea.c`, producer 0x10006910, drawer 0x10006a10):

- `print`, `chat`, `tchat` and `bchat` lines become the current message for
  two seconds (plus bchat's seconds), keeping at most 254 characters (72 at
  intermission) without the trailing newline. A replaced or expired line
  joins a 24-line history.
- The current line is at the rect's bottom edge; while `+chat` or
  `togglechathistory` is on, or the scoreboard is up, `cg_chatHistoryLength`
  (6, 0..24) history lines go upwards 13 units apart (2 with the
  scoreboard, 4 at intermission). Font 1, white, scale clamped to
  0.1..0.22, limit 256, shadow style 3, unshadowed while more than ten
  lines remain to be drawn. `clearChat` and CG_Init clear it.
- `chat`/`tchat` (0x1004b11b, 0x1004b4f7) carry `"%02d ..."` as retail and
  FnQL-game qagame send it: the client number is parsed, out-of-range and
  muted senders are dropped, text starts three bytes in (two for team chat),
  is cut to 149 characters and printed. The beeps are opt-in
  (`cg_chatbeep`, `cg_teamChatBeep`, both 0); `cg_teamChatsOnly` drops public
  chat. Previously FnQL-game printed the client-number prefix. `bchat`
  always beeps.

End-of-match text (`cg_endsummary.c`):

- `CG_MATCH_STATE` (0x10034360): "MATCH SUMMARY" at intermission, else
  "MATCH WARMUP" or "MATCH IN PROGRESS". `CG_MAP_NAME` (0x100343d0): the
  `CS_MESSAGE` title. `CG_MATCH_DETAILS` (0x10034420): "state - tag - map"
  with retail's short tags (FFA, DUEL, RACE, TDM, CA, CTF, 1F, OB, HAR, FT,
  DOM, AD, RR, else ERR).
- `CG_MATCH_END_CONDITION` (0x10034280): Race "Fastest race time within
  the time limit"; past the time limit CTF "Most flag captures ...", CA "Most
  rounds won ...", others "Highest score within the time limit"; otherwise
  CTF capture or mercy limit, CA round limit, Domination/A&D score limit,
  else "Highest score at the end of the game".
- `CG_MATCH_WINNER` (0x10033f00): individual leaders by the first-place
  name (the second when the first forfeited), cut under 192 units with
  "..." past 200; team modes say "^1Red Team"/"^4Blue Team". At
  intermission "WINS by forfeit", "WINS by a score of %d to %d" (teams) or
  "WINS"; before it "leads with a score of %d" (Race: "-m:ss.mmm", none
  without a time), or "Teams are tied with a score of %d". Center alignment
  centers on the origin. Both scores absent draw nothing.
- `CG_PLYR_END_GAME_SCORE` (0x100346e0): nothing for Race or spectators;
  team modes report captures ("You captured %d skull%s." in Harvester),
  assists, defends, or the score; others "You finished %s with a score of
  %d" with an uncolored place and the tie flag cleared first, or the
  forfeit line.
- `CG_GAME_TYPE` (0x10034c20) now uses retail's long-name table ("Unknown
  Gametype" beyond it) at the rect's top in font 0, centered on its origin
  for center alignment; it was drawn at the bottom edge.
- Race times use the C runtime formatter: FnQL-game's bg_lib behind
  Com_sprintf truncates fractions where retail rounds.

The FFA match-end capture now matches retail's header, winner, end
condition and finish line. The winner model, best weapon and the vote
panel remain.

Status pictures (`cg_statusicons.c`): `CG_ARMORTIERED_COLORIZED`
(0x1002f780) tints the item picture by `stats[STAT_ARMOR_TIER]` (0 green, 1
yellow, 2 red) at the item alpha; retail leaves other tiers' tint unset and
FnQL-game keeps the item color. `CG_HEALTH_COLORIZED` (0x1002f6b0) tints by
the followed player's health (0 while its entity is dead) through the
handicapped tint 0x1000ca90. `CG_PLAYER_HASKEY` (0x10031f90) draws the held
silver, gold and master key icons half a width apart. The default in-game
HUD capture now matches retail's, including the tier-tinted armor icon.

Spectator HUD (`cg_spectatorhud.c`). CG_Draw2D's spectator branch
(0x10010fd9) is taken while the predicted state has `EF_SPECTATOR` or
`PM_SPECTATOR`, following included. It paints `comp_specfollowhud_menu`
while following, otherwise `comp_spechud_menu` (neither under the
scoreboard), then the four `comp_spec{red,blue}_{top,bottom}` panels
(retail ships these inside a comment, so they do not load), then the chat
area at (5, 287, 634, 120). A followed view adds the crosshair, crosshair
names, weapon list and rewards (0x1000ac30, inferred). Quake III's
"SPECTATOR ... JOIN menu" text and the "following <name>" banner are gone;
retail has neither. Retail loads 34 built-in menu files (0x100767e0) before
the HUD set; FnQL-game loads the two comp_spectator files beside the
current mode's scoreboards, since the full list exceeds its 64-menu limit
and memory pool. `CG_FOLLOW_PLAYER_NAME` (94) and `_EX` (95) share
0x10033bf0: the followed name, "Following - " first for 94, in the team
color in team modes, aligned by its width. `CG_RED_NAME`/`CG_BLUE_NAME`
(0x10030090) use `cg_redTeamName`/`cg_blueTeamName`, else "Red Team" and
"Blue Team", in font 0; Quake III's `g_redteam` default "Stroggs" no longer
shows.

`CG_FLAG_STATUS` (337, `cg_flagtrack.c`, retail 0x10030240) is the
spectator flag track, drawn for `EF_SPECTATOR` in CTF, Overload, Harvester,
Domination and Attack & Defend (not One Flag or Freeze Tag). The bases come
from `CS_RED/BLUE_FLAG_ORIGIN` ("x", "y", "z"); each flag starts at its last
seen objective position and moves to a valid client below `sv_maxclients`
carrying it (entity powerups, lerped origin; a later client wins). With the
bases apart and a flag away, a half-alpha red fill sits left of the rect x
and a blue one right of it (w / 2 wide, int(h / 8) high). An away red flag
sits at x + w/2 - h/2 - w * d / D (d to the blue base, clamped to the base
distance D), an away blue flag at x - w/2 - h/2 + w * d / D (d to the red
base); each gets `track_pointer` (h/2 square at +h/4 on the bar) and its
flag icon (h square, h/4 under the bar), stolen while taken and dropped
otherwise. `track_pointer` is registered beside the flag-status icons
(0x10023b66). `cgame_probe.py --flag-track` spectates a CTF bot match on
spidercrossings (campgrounds has no CTF flags) until EV_GLOBAL_TEAM_SOUND;
both cgames draw the same bar, pointers and stolen-flag icons under the
score header (the captured flag positions differ with the bots' play).

`CG_SPEC_MESSAGES` (35, 0x10034d70; placed by `ui/spectator.menu` and
`spectator_follow.menu`, which FnQL-game does not load by default) draws,
with `cg_drawSpecMessages` set: in Clan Arena and Attack & Defend, for a
`PM_SPECTATOR` player still on a team, only "Round In Progress" centred at
y 60 (font 0, 0.35, shadowed); otherwise, with any predicted eFlags,
"SPECTATOR MODE" (0.22, white) at the item rect and the mouse-button hint
(0.18, 0.73 grey at 0.7) 12 below, unstyled; then, unless the follow HUD
menu exists and the view follows a player, the left-placed join hints at
0.28, shadowed: "waiting to play" (20, 461) in Duel while client 0's `so`
is clear (retail reads client 0, not the local client), or "press ESC and
use the JOIN buttons" (20, 453) and "to enter the game" (20, 470) in team
games. Retail looks up the `+moveup` key after drawing and never uses it.
`CG_NewClientInfo` now keeps `so` and `pq` (retail clientInfo +0x714 and
+0x718, read at 0x1003e9f2/0x1003ea0b).

Message and notice flags: retail export slots 10 and 11 (0x10029ff0,
0x1002a060) are the engine's new-message and new-notice calls. Each sets
its own timer to time + 3000 (0x10a5f2f8, 0x10a5f2fc) and feeds
`cg_lastmsg` to the HUD chat area (0x10006910) with no extra lifetime; the
2D frame clears a lapsed timer (0x10010de3). `CG_SHOW_IF_MSG_PRESENT` and
`CG_SHOW_IF_NOTICE_PRESENT` show an item while its timer runs, blinking on
bit 0x200 of the client time (0x10031938..0x1003195a). FnQL-game used to
centre-print `cg_lastmsg` for both slots.

Item timers (`cg_itemtimers.c`): EV_ITEM_PICKUP_SPEC records (0x10019d90)
now count down by the frame time beside the frame-time update (0x1004e80b
calling 0x1001ca90), clamping a non-positive remainder to zero, and hold
while `CS_TIMEOUT_START_TIME` is set; the filled records are then sorted
(0x1001ca10): outside CTF by origin z, y, x descending; in CTF by side
descending, flag distance ascending, then group ascending when both are
grouped (else z descending) and entity descending. Map restart clears them
(0x1001c8d0). The painter (0x1000f700) needs `cg_specItemTimers` nonzero,
the scoreboard hidden, a record and `EF_SPECTATOR`; the cvar bits select
powerups (1), mega health (2), red armor (4) and yellow armor (8).
`cg_specItemTimersSize` 0.24 (x100 for the icon), `cg_specItemTimersX` 10
and `cg_specItemTimersY` 200 place the rows: red and blue side rows get a
2.25-wide quarter-alpha fill, grouped icons draw at half size stepping a
quarter, and the whole seconds left draw at (x + size + 3, y + size - 5),
shadowed, in font 0. Retail writes the quarter alpha into its shared red
and blue colors; FnQL-game uses private colors. Retail sets a placement
mode (0x10aa6938) around this painter: its 640-to-screen adjust
(0x100122b0) then drops the wide-screen bias (mode 1, left), keeps it
(2) or doubles it (3, right). `CG_SetScreenPlacement` carries modes 1-3
into `CG_AdjustFrom640` and the text painters; mode 0 keeps FnQL-game's
centred layout. `cgame_probe.py --item-timers` spectates a TDM bot match
with `cg_specItemTimers 15` until EV_ITEM_PICKUP_SPEC arrives; both
cgames then draw the timer column flush left at the same size and text
offset.

Team vitals (`cg_teamvitals.c`, retail 0x1000fab0, after the timers at
0x100110c0): with `cg_specTeamVitals` nonzero, the scoreboard hidden and a
team game, each client entity below `sv_maxclients` whose eType has a bit
of 0xb and whose client info is valid gets a row, spectators excluded. Red
rows stack at x 5 in the left placement, blue rows at 640 - width - 23 in
the right placement, both from `cg_specTeamVitalsY` (85). Health and armor
come from the entity state, or from the snapshot for the followed client,
which also gets `ui/assets/voiceWindow` (18 square at x - 1, y - 17,
registered as in retail 0x100244fa); a dead player's health is zero and
the maximum is the handicapped `g_startingHealth`. A faint client number
(font 1, 0.12, quarter-alpha white) sits at x - 5; the first carried
powerup's icon (flight and frozen skipped), else `friend_dead` when dead,
else the weapon icon, draws 16 square at (x, y - 16). The name drops ^0..^7
pairs and control characters (0x100574c0) and, when wider in font 0 at
0.18 than `cg_specTeamVitalsWidth` (less 21 in mode 2), is cut to one
character under the fitting count plus "...". Default bars: a pure team
color quarter-alpha fill and the health bar (team color at 0.75, or the
bright team color at 0.5 above the maximum) at (x + 18, int(y - 16)), text
height + 2 high; the white name at y - text height; a 6-high armor bar in
the bright team color at y - 6; rows advance 21. With
`cg_specTeamVitalsHealthColor` 1 the bar is 4 high at y - 5 over a black
quarter fill, tinted as the overhead names' health (0x1000ca90), the name
takes the overhead team color, the armor line is 1-high cyan and rows
advance 22. Mode 2 adds the health number (font 1) at x + 19 and moves the
name 21 right. Retail writes these alphas into shared color tables;
FnQL-game uses private copies. The `--item-timers` captures show the same
red and blue panels, rows, bars, names and icons for both cgames.

End-of-match vote (`cg_endvote.c`, `ui/endgamevote.menu`):
`CS_ROTATION_TITLES` (0x2c1) holds `map_N`, `title_N` and `gt_N`;
`CS_ROTATION_VOTES` (0x2c2) holds counts under "0".."2". The game type
(0x100356f0) is drawn eight units above its item, map and title
(0x100355d0, 0x10035660) at it, all font 0 and aligned by their width
(0x1002e050). Shots (0x10035790) use `levelshots/preview/<map>` or
`levelshots/preview/default`. Counts (0x10035820) read "Votes: %s" in the
item font. The timer (0x10035920) counts 20 seconds from `CS_VOTE_TIME`:
"Voting ends in %i second(s).", then "Voting has ended.". FnQL-game's
qagame does not publish these configstrings yet; retail servers do. With the
retail qagame the FFA match-end capture's vote panel (shots, game types,
titles, counts and countdown) matches retail's. The Freeze Tag effects
probes now allow 150 s for bots to kill the local player; the 90 s
default timed out on some runs. The frozen probe now also issues a console
`kill` after about 20 s, so the local player freezes even when no bot finds
it; the capture still shows the ice shell, and both providers pass in
about 27 s.

Connection notice (0x1000aef0/0x10012790): "Connection Interrupted" in
white, scale 0.25, font 0, shadow style 3, centered on x 320 with its
baseline at 116, and the net icon blinking at (592, 432), replacing Quake
III's big-character text and screen-corner icon.

More end-of-match and scoreboard ownerdraws (`cg_endsummary.c`):
`CG_PLYR_BEST_WEAPON_NAME` (0x100345f0) names the selected row's best
weapon (grapple and unknown values draw nothing). The award pictures
75-80 (0x1003a0d0) show the model icon of the client in
`CS_AWARD_OFFENSE`, `_DEFENSE`, `_VALUABLE`, `_ITEM_CONTROL`, `_ACCURACY`
or `_DAMAGE`; retail prefers the Steam avatar with `cg_drawProfileImages`,
and FnQL-game has no avatar import wired (slot 127). `CG_PLAYER_COUNTS`
(0x10032f30) reads "%d/%d Players": clients with info over
`sv_maxclients`, or over serverinfo `teamsize` (doubled in team modes,
capped) outside Duel. The winner's 3D model (85, 0x10008c40) is not
implemented.

Duel slots (`cg_duelhud.c`, 1ST/2ND_PLYR 103-190 and 193-280):
`scores_duel` (0x10045440) fills two persistent slots in row order,
cleared during warmup; at intermission a single new row goes to slot two
behind the departed player. Retail's score row layout (0x268 bytes:
score +0x88, ping +0xa4, accuracy +0xac, medals +0xb0..+0xb8, kills +0xc8,
deaths +0xcc, best weapon +0xdc, damage +0xec, pickups from +0xf4, weapon
arrays at +0x128/+0x168/+0x1a8/+0x1e8/+0x228) maps onto FnQL-game's parsed
fields. Names and scores come from `CS_DUEL_NAME1/2` and
`CS_DUEL_SCORE1/2`; names past 140 units are cut under 132 with "...".
Counters use font 0 and skip a slot without client info except at
intermission; ping is green to 40, yellow to 80, red beyond (alpha 0.8);
wins read "%d/%d". The ready item draws the `{1st,2nd}_plyr_{leads,tied,
trails,ready,notready}` banner and its label at scale 0.16 (slot one at
x+16, slot two right-aligned to x+164), LEADS/TIED/TRAILS outside warmup.
Per-weapon frags, hits, shots, damage and accuracy follow retail's order
(G MG SG GL RL LG RG PG BFG CG NG PL HMG); accuracy above the opponent's
turns white at 0.8 alpha. FLAG draws `ui/assets/flags/none.tga` (no
country is carried) and AVATAR the model icon. Pickups (0x10036eb0): the
PICKUPS row draws each armor/mega item with pickups as its icon, then the
count one row lower (15 right for slot one) and the average wait 15
beyond, each aligned by its font 0 width; single counts (180-183) print
"%d", average waits (184-187, 0x10037730) "%3.2f" through the C runtime
or "-" without pickups. Health/armor bars (116/206, 0x10033040, gated by
`cg_specDuelHealthArmor` 1): the CS_DUEL_CLIENT player's health over twice
its handicapped maximum fills an 8-unit bar 2 below the rect, anchored
right for slot one and left for slot two, orange-yellow or the health tint
with `cg_specDuelHealthColor`; for two seconds after a hit the lost health
trails in red; armor fills a cyan 2-unit strip at +10 scaled by the
handicap. The followed player uses its snapshot stats and damage time.
The second ownerdraw flag word now follows 0x10031a40..0x10031cba: duel
player one/two (that client, or a predicted spectator), `cg_loadout`
(read-only, 0) enabled/disabled, the first/second duel player being
followed, and each weapon either duel player fired. A warmup duel
scoreboard capture now matches retail's, PICKUPS label and "1/8 Players"
included.

Team statistics (`cg_teamstats.c`, 287-336): the `scores_tdm`/`scores_ctf`
team headers (retail 0x10aa6864 red, 0x10aa68a8 blue) are, in order, red,
yellow and green armor, mega, quad, battle suit, regen, haste, invis, flag
and medkit pickups, then quad, battle suit, regen, haste, invis and flag
held seconds, matching `cg.scoreTeamStats`. Pickup counts (0x10037f80)
print "%d" in the item font; held times (0x10039ae0) "m:ss" (0x10057470)
or "-" in font 0. Average ping (0x10039e70) averages the team's score rows,
"(%d)", tinted green to 40, yellow to 80, red beyond. Player counts
(0x100333c0) read "(%d)"/"(%d/%d)" in TDM, CA, CTF, One Flag, Harvester,
Domination and A&D, otherwise "%d Player(s)" or "%d/%d Players" when
`teamsize` fits twice in `sv_maxclients`. Timeouts (0x10033530) read "TO:
%d" from `CS_RED/BLUE_TIMEOUTS_LEFT`, empty meaning serverinfo
`g_timeoutCount`. The map pickup strips (292/319) draw, from the rect's
truncated origin, the red/yellow/green armor, mega and medkit icons
(`pickup_*`) with counts at (+14, +15) (+11 for yellow), advancing 25,
then the flag (the opponent's, or the neutral one in One Flag), quad,
battle suit, regen, haste and invis icons 7 higher with counts at (+14, +8)
and held times at (+2, +15), advancing 28; items without pickups are
skipped. A TDM match-end capture is now available: the probe sets a one-minute
time limit, places one red and two blue bots, issues `allready`, and
finishes on "hit the fraglimit" (team modes log no client-side
EV_GAMEOVER). It matches retail's summary, scores, awards, pickup strips,
vote panel and per-row ready arrows. Both retail list painters (uix86
0x1001c31e..0x1001c47f, cgame 0x1005f51e..0x1005f64e) skip the vertical
up arrow, bar, down arrow and thumb when the list box was parsed with
`notselectable` (0x1001e0a0 / 0x10061260); every retail scoreboard list
is `notselectable`, so retail shows no scrollbar there. `Item_ListBox_Paint`
now does the same; the horizontal branch still draws its scrollbar, as in
retail. Column icons sit at `y + elementHeight * 0.5 - 2` for every list
(cgame 0x1005fa5c, uix86 0x1001c85c), which moved the scoreboard head icons
down to retail's position. The timeout counts (`CG_RED/BLUE_TIMEOUT_COUNT`)
draw only when serverinfo `g_timeoutCount` (0x10049457) is above zero
(dispatcher 0x1003b99d/0x1003b9d4); before, FnQL-game showed "TO: 0" on
servers without timeouts. Scoreboard
column 3 (0x1002631a) is the ready arrow (`ui/assets/score/arrowg.tga` or
`arrowr.tga` from `STAT_CLIENTS_READY`) in warmup and at intermission, and
during play the first carried powerup's icon (0x100261b4); column 4 is the
country flag, which FnQL-game does not carry. The first ownerdraw flag word now also follows
retail for Duel, Clan Arena (answered by mode alone), intermission and
not-intermission (the end screen's `UI_ADVERT` default logo now shows),
warmup and not-warmup, chat visible, and the player being on red or blue;
the message/notice-present flags follow retail's last-message timers
(see the spectator HUD notes).

`cgame-obituary`, `cgame-chatarea`, `cgame-endsummary`,
`cgame-statusicons`, `cgame-endvote`, `cgame-duelhud` and `cgame-teamstats`
cover these. Live check: FnQL-game
now prints retail-format obituary lines in an FFA bot match
("Visor^7 cratered.", "UnnamedPlayer^7 was gunned down by Sarge^7").

## Remaining work

- Scoreboard ownerdraws and visual parity. Main score, team and accuracy
  layouts parse, primary player rows and the detailed end-of-match team
  tables display, and `+acc` works; see the bring-up evidence below. `+pstats`/`-pstats` (retail
  10006dd0/10006e20) request `pstats` at most once a second, sharing the
  `+acc` timer and skipping free spectators by predicted state; retail's
  held flag has no other reader, so there is no separate panel. Race state, checkpoint models/navigation icons and the
  respawn-binding hint work; exact race text/font parity remains open.
- Every retail-handled event now has a handler: EV_POI pings and the
  domination, survivor and lead-change team sounds landed last; see the
  world-ping section below. Round draws, Red Rover survivors/winners and
  last-standing voice have handlers; see the round-result evidence below.
  Damage plums now follow the retail drawer: they have no distance
  sizing. Reserved unused events keep the unknown-event error, as retail
  does (0x1001c5f2 calls the error import). All retail team-sound subcases
  now follow 0x1001b349; see the team announcer section below.
- The gore-DLC paths (`cg_gore.c`) are implemented from retail: media
  register only when `FS_GetFileList("dlc_gibs")` finds files (cgame import
  19, now named `trap_FS_GetFileList`); gibbing launches the fifteen DLC
  models (skull half the time) at (crandom, crandom, crandom + 1) * 250 as
  blood fragments (5-8 s, bounce 0.6); EV_GIB_PLAYER plays `gibsplt1`;
  blood fragments sound `gibimp1-3` on every bounce, mark with
  `bloodSpray3/4` (radius 16-47) and trail a random-spray puff every 50 ms;
  bleeding is a random-spray puff (radius 32, 500 ms). EV_JUICED shows the
  DLC juicer, and without the DLC gibs with electro gibs as retail does
  (FnQL-game used to run the Team Arena juicer with assets the retail paks
  lack). Without `dlc_gibs` every path keeps the electro/no-blood behaviour.
  This install has no DLC; `cgame_probe.py --gore-standin` stages a
  throwaway `dlc_gibs` folder of retail sphere-gib copies in the probe home,
  and the running cgame registers all eleven DLC model names (the retail
  paks' own `bloodSpray`/`viewBloodBlend` shaders resolve and point into
  `dlc_gibs/`). `--gib-watch` (bots with rocket launchers and 20 starting
  health, FnQL-game qagame) runs until a client-observed EV_GIB_PLAYER: with
  the stand-ins the gib, splat and blood paths run live without errors (only
  the stand-in sprays' missing textures warn). `cgame-gore` covers the
  launch values, sounds, marks and trails.
- Prediction step smoothing, item quantities/timing, powerup behavior and
  broader replay/cache handling. Live G4 against both server providers now
  passes (see the prediction entry below). The
  QL cache-state correction and offline production-prediction gate below
  cover one part of this work.
- Remaining HUD ownerdraws, visibility and warmup presentation. The
  message/notice flags, item timers, team vitals and spectator messages are
  done; see the spectator HUD notes. Flag status,
  scoreboard speaking/muted icons, crosshair names, teammate markers,
  objective markers, the overhead name overlay, the kill feed and the chat
  area are done; see the flag-status, overhead-name and HUD feed sections.
  The ownerdraw inventory in the HUD feed section lists what is still
  missing: spectator HUD menus, end-of-match summary and vote ownerdraws,
  duel and team statistics, and the flag-word visibility tests. Status values/bars and
  team-tinted HUD images and weapon-list presentation now have retail-backed
  handlers. Live follow/demo list captures remain to be added. Tracked exports currently show only
  last-message text.
- Full text/HUD parity beyond the native metrics bridge. Retail's painter
  (0x10008440) never reads its `adjust` argument (stack slot 7; it reads the
  limit and style slots only), so `CG_Text_Paint` now ignores it too and
  always draws the whole string; the inherited byte-at-a-time spacing path
  is gone. Retail cgame's display context (0x10029210) assigns the slots on
  both sides of 0x10a25684 but never that one, which by the Quake III layout
  is `drawTextWithCursor` (inferred from `setCVar` at the preceding slot), so
  retail cgame has no cursor painter; FnQL-game's fallback paints the field
  text without a cursor. See the font slice below.
- Camera/view parity. The FOV projection, command value and zoom controls
  follow retail evidence with production tests; see the view section below.
  `cgame_probe.py --zoom` pins the player with `setviewpos` on a devmap and
  holds `+zoom`: both cgames show the same zoomed framing, hidden weapon and
  crosshair. Followed views are compared by `--overhead-names`.
  `--record-demo FILE` records a short bot match (the FnQL client writes
  `.dm_68`, an engine-side naming question) and `--play-demo FILE` replays
  it with the chosen cgame. Fixed from that comparison:
  - The lagometer is off by default (`cg_lagometer` 0, range 0-2) and is
    not drawn on a local server or in a demo (0x1000b037); the connection
    warning is drawn on its own after it and skipped in demos
    (0x100110f0). FnQL-game showed "Connection Interrupted", the lagometer
    and the net icon over demos.
  - Demo controls (`cg_demohud.c`, 0x10010450, `cg_drawDemoHUD` 1): a
    half-black box (210, 355, 220, 60), the clock "[-]m:[0]s.mmm" at
    (288, 375) in font 2 at 0.3, and centred labels (font 1, 0.25) over
    yellow key hints (0.15): Play/Pause [SPACE] at 235; while
    `cl_freezeDemo` is set and three frames after, Advance [RIGHT] at 320;
    otherwise - [LEFT] at 275, the timescale "%0.1fx" from x 290,
    + [RIGHT] at 330 and 1.0x [DOWN] at 370; Hide [DEL] at 410.
  - The clock counts from the scoreboard's last close. CG_Init raises the
    scoreboard (0x10029fb2); the first 2D frame closes it for a
    non-spectator (0x1000e9c0), and a spectator keeps it until -scores or
    `team`, which is now a cgame command that forwards "team <args>" and
    closes it (0x10007880). Retail takes the cgame key catcher when the menu
    activates (0x1000ea05) and drops it on closing (0x1000e8f0); FnQL-game
    keeps it for the spectator through `CG_SetScoreCatcher` and releases it
    on close.
  Both cgames now show the same demo frame and clock (retail 0:04.891,
  FnQL-game 0:04.911, one frame apart; 0:16.559 before the
  initial-scoreboard change).
- Crosshair (`cg_crosshair.c`, retail 0x1000c2a0): crosshair n % 30 (0 and
  multiples of 30 draw nothing, nor does third person); colour from
  `cg_crosshairHealth` (health/armor tint) or the 26-entry
  `cg_crosshairColor` table (default 25, white), times
  `cg_crosshairBrightness`; size `cg_crosshairSize` (default 32, not Quake
  III's 24), scaled by vidWidth / 640 on both axes and centred in the
  refdef with `cg_crosshairX/Y`. A pickup newer than the last hit pulses it
  (`cg_crosshairPulse`, up to t / 200 for 200 ms); otherwise
  `cg_crosshairHitStyle` 1-8 (0x1000c0a0) tints it by hit level (generic1 /
  64 + 1, recorded when PERS_HITS rises, 0x10043c2c: light blue, yellow,
  orange, red) or `cg_crosshairHitColor`, and grows it by f * level or f
  over `cg_crosshairHitTime`. The zoom captures now show the same crosshair
  for both cgames; before, FnQL-game's was 30 px against retail's 48 px.
- Prediction (G4, live part): suite `cgame-prediction` runs
  `prediction_probe.py --cgame` with FnQL-game's cgame against the retail
  and FnQL-game servers on campgrounds and bloodrun, VQL and PQL (air
  control, crouch slide, double jump): zero prediction misses in all eight.
  The recorded-demo corpus does not apply to prediction (demos replay
  snapshots). Under load (deep in a full 112-test run) the campgrounds PQL
  runs twice showed 2 misses in the turn-run segment, for both server
  providers; 14 standalone runs and 15 back-to-back meson runs of the
  FnQL-game and retail-cgame PQL gates were clean. This matches the
  load-dependent G4a PQL misses already recorded for the retail cgame
  (docs/PLAN.md); it needs a controlled-frame-timing reproduction, not a
  cgame change.
- The plain `--combat` gate now finishes on the first client-observed
  EV_OBITUARY (probe timeout 150 s) instead of a fixed 43 s window, which
  campgrounds bots sometimes passed without a kill.
- Matched visual parity for frozen players. Teammate indicators (friend,
  hit, frozen, flag-carrier) now follow retail; see the teammate marker
  section below.
- Server player-appearance settings (`CS_APPEARANCE_SETTINGS`, 0x2ac, parsed
  by retail 0x10048e40). `g_playerheadScale`, `g_playerheadScaleOffset` and
  `g_playerModelScale` are now read; outside demos (0x10042a83, gated on the
  DrawActiveFrame demo flag) the head's axes are scaled by the head scale
  with non-normalized axes and the head is lowered by
  24 - 24 * offset, so the server defaults (1.0, 1.0) change nothing.
  A missing key keeps the server default instead of retail's atof("") = 0.
  In client model selection, a non-empty `g_playermodelOverride`
  (`g_playerheadmodelOverride`) turns off the client's own forced body
  (head) models (0x1003e133, 0x1003e347); the override reaches clients
  through their configstrings. Without `g_allowCustomHeadmodels` the head
  model comes from the "model" key instead of "hmodel" (0x1003eb38). A
  change re-reads every client (0x1003e440). FnQL-game keeps its inherited
  `cg_forceModel`/`cg_enemyModel`/`cg_teamModel` scheme under these rules;
  retail's own forced-model cvars are not mirrored.
- The grapple's hook (`grapple_ammo.md3`, registered only for weapon 10 at
  0x10050da6) now sits on `tag_ammo` of the world item (0x1001737a, placed
  like the barrel), the held weapon (0x1005282d, hidden while the player
  fires and the predicted weapon is the grapple) and the leader model.
- Leader model (`cg_leadermodel.c`): the per-model height scale (0x1003d260,
  56 / (legs tag_torso z + 24 + torso tag_head z + head maxs z) *
  `g_playerModelScale`, 1.0 for Orbb) is computed after a model loads,
  copied with the model and refreshed with the appearance settings.
  `CG_1STPLACE_PLYR_MODEL` (85) and `CG_PLAYERMODEL` (37) now draw
  `scores[0].client` as 0x10008c40 does: fov_x int(width / 640 * 90), the
  model 1.1 * (s + 24) / tan(fov_x / 2) ahead at (s - 24) * -0.5 (s = 32, or
  32 / (scale * 0.85)), legs at the LEGS_IDLE first frame, torso and head on
  their tags, colour 0 128 0 255, the weapon on `tag_weapon` with a barrel
  for the gauntlet, machinegun and BFG, a white and a red light, no world.
  85 uses that row's best weapon (TORSO_ATTACK2 for none or the gauntlet,
  yaw 160); 37 uses TORSO_ATTACK, yaw 210 and the selected weapon, or the
  snapshot primary with `cg_loadout`. The FFA match-end capture
  (`--combat --match-end`) shows the winner standing in the summary's left
  panel at the same size, pose and lighting for both cgames.
- Obsolete Q3 asset/effect/sound/model paths; runtime still reports missing
  optional/inherited assets.
- Linux i386: an i686 cross build in WSL Ubuntu (gcc 13 `-m32`, a cross file
  with `needs_exe_wrapper = false`) builds `fnql_cgamei386.so`,
  `fnql_uii386.so` and `fnql_qagamei386.so`, and all 50 unit tests plus the
  ABI-parity and wire-contract gates pass there. Fixed on the way: `offsetof`
  without `<stddef.h>` (q_shared.h now includes it; MSVC pulled it in through
  stdlib.h), `CG_EventHandling` defined with `int` against its
  `cgame_event_t` prototype, and two game-test stubs typed `int` where the
  headers use `team_t`. Runtime: the FnQL Linux i686 client (BUILD.md's
  Meson X11 path, `-Dsdl=disabled`, built in WSL Ubuntu and shown through
  WSLg) hosts FnQL-game's `cgamei386.so`, `uii386.so` (`cgame_probe.py --ui`;
  the retail paks carry no Linux UI) and `fnql_qagamei386.so` with the retail
  assets: the `--scoreboard`, `--combat`, `--item-timers` and
  `--combat --match-end` probes pass and the scoreboard capture matches the
  Windows one. The probe now names copied modules by suffix
  (`cgamex86.dll`/`cgamei386.so`). Linux runs used to end with the engine
  segfaulting inside `XCloseDisplay` (debugoptimized builds; plain debug and
  ASan builds did not show it): an FnQL engine shutdown bug, not a module
  one. Cause: a failed DGA probe (no DGA on XWayland) still registered
  libXxf86dga's `XESetCloseDisplay` hook on the open Display, and the
  engine unloaded the library before closing the Display. FnQL now keeps
  the RandR, VidMode and DGA client libraries loaded until after
  `XCloseDisplay` (`GLW_ReleaseDisplayExtensionLibrary`, gated by
  `tests/platform_port_safety_tests.py`); both debugoptimized clients
  (renderer dlopen and static) now quit with exit 0.
- Retail-engine hosting of FnQL-game's cgame, without altering the retail
  install (the project owner's condition, 2026-11-06). Observed in
  `quakelive_steam.exe`: a module is looked up in the paks first (0x4d2750),
  newest search path first, only in paks named `bin` (0x4d1770), and on a
  pure server only in paks on the pure list; the hit is extracted to
  `<fs_homepath>/baseq3/cgamex86.dll` and loaded with only a `dllEntry`
  check (0x4eceb0). The home path never mounts paks (directory-only flag,
  0x4d32d8 / 0x4d2f38), but `fs_basepath` is CVAR_INIT (0x4d31c6). So
  `retail_client_probe.py --cgame` gives the retail client a scratch base
  path: `pak00.pk3` and `web.pak` hard-linked (or copied) from the install,
  plus a `bin.pk3` rebuilt from the retail one with only `cgamex86.dll`
  replaced; the FnQL server's home carries the same pak so it is on the
  pure list. Gates `retail-client-cgame` and `retail-client-cgame-unpure`
  (suite `retail-client`) pass: the retail client joins a pure or unpure
  FnQL-game server, stays 30 s with a bot, and its process has
  `cgamex86.dll` loaded from the scratch home with FnQL-game's SHA-256. The
  run writes nothing to the install beyond what any retail launch writes
  (`awesomium.log` and `profile.pid`, which the probe restores, and Steam's
  own `steam_autocloud.vdf`).
- `retail-client-join-unpure` "client never entered" (also in UI.md) was
  the probe, not the server: the retail client writes its PID to
  `<steamid>/baseq3/profile.pid` and rewrites it to `0` on a clean exit; a
  leftover PID sets `com_crashed` and, without `com_ignoreCrash`, blocks on
  an "Enable Safe Mode?" message box (0x4c93e0, 0x4cc1f9). The probe stops
  the client, so every retail launch after the first waited on that box
  and never sent a packet (the server logged none). The probe now launches
  with `com_ignoreCrash 1` and restores `profile.pid`; all four
  retail-client gates pass back to back.
- FnQL provider, purity and capability negotiation (M10).

Do not enable automatic selection or retail-wire parity until these gates pass.

## Event bring-up evidence (2026-11-05)

`cg_ql_events.c` adds eighteen event types and all twenty-three retail global-team-sound subcases, using independently written handlers
and the existing effects/reward infrastructure. `cgame-events` tests the
production handlers without graphics or input devices.

- Snow/wood footsteps: retail cgame 0x1001c007/054 selects four body-channel
  variants, gated by `cg_footsteps`; registration is 0x1002236c/394.
- Drown: 0x1001be15 routes the player's custom drown sound to voice channel.
- Armor regeneration: 0x1001bf1b uses powerup 14 and item channel;
  0x10022e5d registers `sound/misc/ar1_pkup.ogg`.
- Thaw tick: 0x1001c209, body channel, `sound/misc/tim_pump.ogg`
  (0x10022ed7).
- Thaw fragments: event 0x1001c1e8 and helper 0x100147d0 create seven
  alternating `models/gibs/brain.md3` / `abdomen.md3` fragments at the
  interpolated origin. They use gravity, horizontal velocity within 250,
  upward velocity 0..500, 5..8-second lifetime and 0.6 bounce. The existing
  fragment system now supports ice marks (0x1001e76d: alpha fade, radius
  16..47) and 150 ms ice-trail samples (0x1001d790: radius 20, lifetime
  2000 ms, falling/scaling type). Trails stop at the first collision;
  there is no blood trail or bounce sound. Long-frame catch-up is bounded
  to the visible lifetime. Shader registrations follow 0x100231af and
  0x10024af0; both models and all shaders were checked in installed paks.
- Damage-through missile impacts: 0x1001c1aa decodes the direction and
  passes the event weapon. 0x10054db0 traces from the configured distance
  beyond the impact back to its origin, skipping backside effects for a
  solid start or no hit. CS 0x2a5 carries the distance (0x10049ebd), despite
  its inherited `CS_DOM_CAPTURE_TIME` name. A valid backside gets a
  radius-64 burn mark and ten `surfacePuff` particles, radius 24, color
  (.8,.8,.7), speed 100..400 along the normal plus 0..100 per axis, lifetime
  240..360 ms. The ordinary explosion is retained for both trace outcomes.
  `cg_ql_effects.c` uses existing trace, mark, puff and fragment services;
  malformed native coordinates/directions and unsafe distance strings are
  bounded rather than passed to collision/rendering. `cgame-effects` tests
  fragments, sampled trails, marks, debris and ordinary-explosion fallback.
- Dropped weapon: 0x1001a714 and 0x10053d90 select the highest owned,
  nonempty weapon other than the dropped one and reserved slot 15. Grapple
  (slot 10) remains eligible; infinite ammo (-1) is nonempty. The recipient
  is the local client rather than the followed player's snapshot client.
- Spectator item pickup: 0x10019d90 updates ten records keyed by item entity,
  retaining side, item, timer, distance, group flags and origin. Full tables
  preserve existing records; malformed native indices are rejected. Display
  of these records remains HUD work.
- Lightning discharge: 0x10013310 creates an electric sprite with integer
  radius `(cells*10+48)/16` and duration `cells+300` ms. Retail local-entity
  type 8 draws through 0x1001f4e0's scale/fade path; FnQL-game uses its named
  `LE_SCALE_FADE` equivalent. Invalid non-byte parameters are rejected.
- Awards: 0x1001c410 maps event IDs 0–9 to combo, rampage, midair, revenge,
  perforated, headshot, accuracy, quad god, first frag and perfect. Count is
  `modelindex2`, except first frag forces one. The queue limit is 16 entries
  (0x10043570). Only the snapshot client receives the award. Voice paths and
  three-way variants follow registration at 0x10021efa; stock/evil/female
  selection follows 0x10020dd0. Missing alternate sounds fall back to stock.
- Infected/new high score: 0x1001c5a8/5c5 uses the buffered announcer queue.
- Overtime: 0x1001b2ac starts the local player's klaxon and centers the
  configured `g_overtime` seconds. The first-second test in 0x1002a5f0
  controls the buffered overtime voice; subsequent extensions keep the
  klaxon/message without repeating that voice. Wide arithmetic replaces
  the reference's iterative overtime count. Invalid serverinfo durations
  become zero without division or overflow.
- Match end: 0x1001c0be clears reward/voice queues, preserves a future
  announcement deadline (0x1004e180), optionally plays `cg_buzzerSound`,
  then chooses result music/voice. Individual wins require rank exactly
  zero, including Red Rover; tied-first is not a win. Team score ties favor
  red (0x1002a500). Generic gameover gives spectators win music, whereas
  explicit team wins give spectators no music. Team win subcases 14/15
  use the local client's team, not a followed player's team
  (0x1001b80c/898). Subcases 16/17 center the round winner and queue its
  voice without ending the match (0x1001b927/94f). New announcements stop
  after a match result (0x1004e110), and map restart resets that guard
  (0x1004a32e). The nine sound/music paths were verified in installed paks;
  unit tests check routing, queue reset, rank/team/spectator cases and
  overtime timing. Runtime probes disable audio, so they do not prove
  audible mixing or voice timing parity.
- Damage plums: 0x1001c3cc uses the snapshot client, `time` damage and
  `generic1` weapon. 0x10001e50 parses at most sixteen weapon tokens;
  0x10013660 establishes the three color modes, strict 25/50/75 thresholds,
  one-second lifetime, random scatter/velocity and 600 ms fade delay.
  The weapon-color constants are regenerated from the installed module by
  `extract_retail_contract.py`. `cg_damageplums.c` owns 32 bounded records,
  handles clock wrap/seeks, and uses native projection slots 120/121 plus
  text slots 123/124. The retail drawer (0x10011680, text branch
  0x100118c4) leaves the plum's world width at zero, so the marker size is
  zero: text is painted at the projected point by the HUD text painter
  0x10008440 at constant scale 0.15 (`0.15 * vidHeight / 8` pixels),
  without centering. Offsets are `vx*t` and `vy*t + 150*t*t` in the
  640x480 canvas (x stretched by vidWidth/640); the inherited 75 halved
  retail's 150. There is no distance-dependent sizing. Unit tests verify
  actual draw calls, size, offsets, lifetime, recipient, filters, all
  threshold boundaries, queue overflow and malformed indices. Native
  formatting safely handles INT_MIN; the inherited shared VM formatter
  still has a signed-negation bug for that value outside this path.
- Shotgun kill: 0x10055aa0 uses `generic1 / 5` bursts, two blood effects per
  burst, `pos.trBase` plus integer scatter and `otherEntityNum2` as victim.
  Optional tracer sparks use the four `cg_impactSparks*` cvars (retail table
  0x100776a8–0x100776f0). This differs from the QLSRP reconstruction's event
  fields and burst cap; the implementation follows retail observations.
  Native input outside the eight-bit wire range is rejected. Spark size and
  lifetime are bounded against pathological local settings.

`cgame_probe.py --combat` adds four autonomous bots without injecting user
input. Retail qagame on Campgrounds reached active combat, then produced
`Unknown event: 96`, then `Unknown event: 89` after implementing damage plums.
Both dispatch failures are now covered by unit tests. The original failure
logs remain in `.tmp/cgame/probe-nodwsd7r/baseq3/qconsole.log` and
`.tmp/cgame/probe-qy1s870v/baseq3/qconsole.log`. Short bot runs subsequently
exited normally with both qagame providers, but did not prove a kill occurred.
The combat probe now uses private `devmap` to enable cheat-protected event
diagnostics, waits longer and requires a client-observed `EV_OBITUARY`, as
well as an engine screenshot and normal shutdown. The retail game log wrote
timestamps without message bodies in this runtime, so it is not used as the
kill oracle. Meson's optional `cgame-combat` suite runs both server providers.
Both strengthened probes passed on 2026-11-05. Retail evidence is retained
in `.tmp/cgame/probe-f7sz08sr/` (one obituary, six damage plums, one shotgun
kill); FnQL-game evidence is in `.tmp/cgame/probe-r6308xn7/` (two obituaries,
six damage plums). Those are handled event counts, not proof that every plum
was visible to the stationary local player. Both contain engine screenshots.
None of these checks establishes prediction or visual parity. A capture also
shows the inherited connection-interrupted overlay during local play; that
diagnostic/prediction path needs investigation. The later prediction slice
adds retail's three-second level-start grace period and failed-read handling;
it does not prove the cause of every earlier overlay.

The match-end probe revealed a harness limitation: the older long queued
`wait` followed by `quit` prevented qagame's appended `map_restart` from
running at the end of warmup. The earlier combat gates therefore prove
warmup combat, not a completed match. `--match-end` and `--overtime` now
leave the command queue free and use authenticated loopback RCON to capture
and exit after the expected client event. Overtime pauses the bot only
after it has joined; pausing before that caused a forfeit, followed by an
AAS initialization failure on the next map. No user input is injected.

Full FFA matches now reach `EV_GAMEOVER`, intermission and normal exit with
both providers: `.tmp/cgame/probe-_x34ogq6/` (retail) and
`.tmp/cgame/probe-ohyum_hg/` (FnQL-game). Both engine captures show populated
end-scoreboard rows. Detailed end ownerdraws and next-map voting cards are
still incomplete. Overtime evidence is in `.tmp/cgame/probe-mshlskb1/`
(retail) and `.tmp/cgame/probe-mbxq120k/` (FnQL-game); the retail engine capture shows
"Overtime! 120 seconds added" after the real server event. The optional
`cgame-match` suite covers match end and overtime with both providers.
An unrelated engine botlib warning, "weapon number out of range", remains
in bot-match logs (`code/botlib/be_ai_weap.cpp` in FnQL).

Thaw and damage-through probes consume real bot-generated server events,
capture with the engine and exit normally. Retained thaw evidence is in
`.tmp/cgame/probe-pbt4hjoq/` (retail, Campgrounds) and
`.tmp/cgame/probe-ts845yu7/` (FnQL-game, Thunderstruck). The effect suite uses
explicit red/blue bot teams and Thunderstruck for thawing: two FnQL-game
Campgrounds runs produced no client-observed thaw, including one with many
kills and thaw obituaries. Passing on the smaller map supports a visibility
explanation but does not prove the reason for every missed event. The first
run also exposed automatic bot team assignment placing all bots on blue;
that server-side behavior remains outside this cgame change.

Damage-through evidence is in `.tmp/cgame/probe-_5qugcz_/` (retail) and
`.tmp/cgame/probe-igpwu8r6/` (FnQL-game). These private devmap probes force
surface penetration eligibility and supply rockets; they require the
actual `EV_MISSILE_MISS_DMGTHROUGH`, not a kill. The ordinary combat and
match-end gates retain their obituary requirement. The optional
`cgame-effects-hosting` suite runs both effects with both providers.
The captures establish continued rendering after dispatch; effects can be
outside the stationary camera's view, so these are not visual-parity gates.

## Flag status, teammate markers, beams and gibs (2026-11-05)

A scan of every cgame model, shader and skin registration against the
installed paks found 41 unresolved paths. It now finds 26, mostly in
compiled-out Quake III/Team Arena branches or assets retail also lacks.

**Flag status ownerdraws.** Retail dispatches 283/288 (red flag/base
status) and 310/315 (blue) to 0x10030a80 and 54 (One Flag) to 0x10030ff0
(jump table 0x1003c260, index = ownerdraw - 1). The base-status variants
were unhandled and the others drew Quake III statusbar icons absent from
the paks. `cg_flagstatus.c` registers `gfx/2d/flag_status/` neutral, red
and blue at-base/taken/dropped/stolen icons (0x10023a47..0x10023b51):
flag status shows at base, taken or dropped; base status at base or taken;
One Flag shows the stolen icon, neutral while that team carries the flag.
Freeze Tag and individual modes draw nothing, Harvester and Domination keep
the menu color. Ownerdraw 54 nudges a carried neutral flag 9 units down
while the carrying team is not ahead (or has no score) and up otherwise.
The defend medal uses `medal_defense` (0x100246ee). `cgame-flagstatus`
covers every state, gametype and offset.

**Teammate markers.** Retail 0x100411a0 (gated by `cg_drawSprites`) picks
the first of: connection; One Flag neutral carrier; flag carrier (hit
variant for 1.5 s after pain); frozen teammate (Freeze Tag); talk; the six
medals; the Red Rover bite; teammate. Infected enemies hide their talk
balloon and first three medals. Retail 0x10040cd0 draws a teammate's
sprite as a one-frame screen marker through walls, 48 units up, sized by
`cg_teammatePOIsMaxWidth` (24, 16..32) and `cg_teammatePOIsMinWidth`
(4, 2..16), tinted red-to-yellow below the snapshot's `STAT_MAX_HEALTH` and
yellow-to-white up to twice it. Red Rover survivors see infected players
as the bite marker with alpha `(minSpeed - speed) / minSpeed`, where
minSpeed is CS 0x2c5 (`g_rrInfectedSurvivorMinSpeed`, despite its
`CS_RR_PING_RATE` name). Others keep the 48-unit radius-10 world sprite.
New cvars replace Quake III `cg_drawFriend`: `cg_teammatePOIs` (1),
`cg_teammateNames` (1, which suppresses the friend/carrier icons),
`cg_drawHitFriendTime` (100 ms), `cg_drawSprites` (1). Registrations:
`sprites/friend`, `friend_hit`, `frozen` from TDM up; `flagcarrier` and
`_hit` for CTF, One Flag and A&D. The hit time is FnQL-game's pain time
(inference for retail centity 0x288). The `cg_teammateNames` name overlay
is described in the overhead-name section. A teammate's death (obituary 0x10019246; team
modes except Red Rover and Freeze Tag, not the local client) leaves a red
`sprites/friend_dead` marker 16 units below the event origin, drawn 64 up
with the teammate widths for `cg_drawDeadFriendTime` ms (3000, 0..5000).
`cgame-playerpresentation` and `cgame-poi` cover selection, tint, hit
windows, gates, the bite alpha, capacity and placement.

**Objective markers.** Retail 0x10040870 runs for the snapshot player's
own entity (from the player drawer, 0x10042bc3) and queues one-frame
markers 64 units above recorded bases, sized by the `cg_poi` widths. Bases
are the last drawn positions of team flag items (by tag, 0x100160a7) and
team bases (by modelindex, 0x1001891b). In Harvester a skull carrier sees
the enemy base in the enemy color (`gfx/2d/har/poi_capture`, Harvester
only). A flag carrier, gated by `cg_flagPOIs` (new, default 1), sees
`gfx/2d/ad/poi_capture`: in CTF the own base, only while the own flag is
home; in One Flag the enemy base in the own team's color; with the neutral
flag in other modes the own base. Retail would mark the world origin for a
base it never drew; FnQL-game waits until the base is seen. `cgame-poi`
covers every branch, the gates and the unseen-base case.

**Lightning.** `lightningBoltNew` does not exist in the retail paks.
`cg_lightningStyle` (1..5, default 1) selects `lightningBolt1..5`
(0x10051f11); the grapple keeps `lightningBolt1` (its retail beam is
unverified). The select cursor uses `ui/assets/3_cursor3.tga` (0x10024d13).

**Electro gibs.** Quake III gore gibs and blood shaders are not in the
paks, so gibs were invisible and blood sprites drew the renderer's default
image. Retail without the DLC (0x10014c80) launches ten
`models/gibs/sphere.md3` fragments in a ring around a point eight units
along the event's fixed (0, 0, -1) direction (0x10019ef0): velocity is the
ring offset plus 20 along it, scaled per axis by 21 +/- 10.5, with
trajectory type 6, gravity 425, bounce 0.35, start jitter +/-250 ms and a
375..1125 ms life. The first bounce plays `electrogib_bounce_01`/`_04` and
one 4..11-unit burn mark is left (bounce and mark type 3, 0x1001ec7a /
0x1001e316). The gib sound is `electrogib_01` or `_04`: retail's
`rand() & 4` never selects the other two. A 500 ms orange light (400) holds
for half its life, then fades, and is removed with a viewer inside 50 units
(0x1001f730). Retail also queues a deathEffect sprite and 36 tracer
particles whose life rate is never set, so they draw with zero color
(inference); they are not reproduced. Retail does not gate electro gibs on
`com_blood`. Bleeding draws nothing without the DLC, as at 0x10013da0.
`cgame-effects` covers geometry bounds and signs, trajectory, timing, sound
choice, single bounce, marks and the light curve.

**Crosshair names.** Retail has no `cg_drawCrosshairNames` (0x1000c670).
`cg_drawCrosshair` (retail default 2, 0..29; the cgame registered 4)
gates names, never in third person. Teammates (team modes, the snapshot's
team) are named only with `cg_teammateCrosshairNames` (0) and without
`cg_teammateNames`; everyone else only with `cg_enemyCrosshairNames` (1).
The fade is scaled by the matching `...Opacity` (0.75). The name is
centered at y 190, scale 0.26, shadowed. With `cg_drawCrosshairTeamHealth`
(2) a teammate's `health / armor` follows at y `198 + 16 * size`, scale
`cg_drawCrosshairTeamHealthSize` (0.12, 0.10..0.26): health right-aligned
at 320, `/` at 320, armor at 325, tinted by the snapshot maximum in mode 2
and white in mode 1. Values come from FnQL-game's team info (retail reads
a per-client table at 0x10abbac0; inference). Widths use the integer
measure. `cgame-crosshairnames` covers the gates, opacities, placement,
tints and invalid input. Retail never draws its registered
`menu/art/3_cursor2`; the engine shows the system pointer while the cgame
catcher owns input, so the spectator scoreboard catcher no longer draws a
sprite cursor (reported by the UI session).

**End-of-match team tables.** Feeders 17/18 (red/blue stats) were empty.
Retail's dispatcher (0x10028830) picks a table by gametype: TDM and Freeze
Tag (0x10028300), Clan Arena (0x10027720), and CTF, One Flag, Harvester,
Domination and A&D (0x10026cf0); Overload and Red Rover have none. All
start with the name (names 100 units or wider at scale 0.16 are cut below
92 and given `...`), `^3score` (`^3-` for the -9999/-999 sentinels), kills
and deaths less team kills and suicides. TDM adds suicides, team kills,
team killed, `kills - deaths - team kills + team killed`, damage
given/taken, accuracy, the four armor/mega pickups, quad and battle suit,
then time (`%4i`). Clan Arena adds damage given/taken, accuracy, gauntlet
kills and `^3kills ^7accuracy%` for machinegun through plasma and the heavy
machinegun, then time. The flag modes add suicides, kills minus deaths,
damage given/taken, accuracy, the four item and five powerup pickups, then
time. The retail row offsets were matched to the `scores_tdm`,
`tdmstats`, `ctfstats` and `castats` parsers (0x10045f20, 0x10046610,
0x10047400, 0x10046b00). `cgame-scores` covers every table, sentinels,
truncation and unsupported modes.

**Speaking and muted players.** Export 20 (0x10020a40) stores each
client's speaking state; every retail player feeder (0x10026294,
0x10026a08, 0x10027398, 0x10027cf4, 0x10027f8c) shows column 1 as
`ui/assets/score/muted` when import 125 (`IsClientMuted`, the player's
Steam identity halves) says so, otherwise `ui/assets/score/speaking` while
speaking. FnQL implements that import; a missing import, an invalid client
or an unparsable identity is "not muted". `cgame-native` and
`cgame-scores` cover the import call and the icon choice.

## Local announcer and feedback sounds (2026-11-05)

Before this slice, 62 sound paths registered by the cgame were Quake III
files absent from the installed paks; a rescan now finds 9 (gibs,
invulnerability and obelisk sounds), all listed under remaining work. `cg_localsounds.c` replaces the inherited local-sound
logic with retail 0x100435b0, called from the player-state transition for
non-spectators outside intermission (0x10043b60):

- Hits within one life (spawn count unchanged): when the kill counter also
  rises and `cg_killBeep` is 1..8 (new; retail 10077750, default 7), play
  `impact1..6`, `bell_01` or `chaching` (values out of range use impact1) at
  `s_killBeepVolume` (new; 100784b8, default 1, 0..4). Otherwise
  `cg_hitBeep` (new; 10077648, default 2, 0..3) selects one tone (1), the
  hit tier from `generic1` bits 6-7 that qagame writes (2), or the inverted
  tier (3), from `sound/feedback/hit0..3.ogg`. Fewer hits play
  `hit_teammate` for any nonzero mode (registered from TDM up).
- Medals in retail order (captures, assists, defenses, impressive,
  excellent, gauntlet) for counters that changed to nonzero values.
  Captures use the flag-capture effect (flag/objective modes only),
  assists and defenses one voice each, the others `first_*` for the first
  award and then one of three variants at random.
- Player events: denied, a random humiliation variant, or holy shit; the
  first changed bit wins. "You have the flag" moved here unchanged.
- FFA lead changes outside warmup, skipped on a first-frag frame:
  `lead_taken`/`lead_lost` with `cg_announcerLeadsVO`, `lead_tied` with
  `cg_announcerTiesVO`.
- Frag warnings `1_frag`/`2_frags`/`3_frags` read only CS_SCORES1 and stop
  at TDM; the inherited code used the higher team score and included clan
  arena.

`CG_MatchClockWarnings` (retail 0x1004395d) uses the match-clock state:
outside warmup and timeouts it plays `5_minute`, `1_minute` and
`sudden_death`. Sudden death is skipped with `g_overtime` outside round
modes. The round modes add `30_second_warning`, re-armed by every
CS_MATCH_STATE update (0x10048cb6), when no match warning fired that frame.
Retail announces a moment only if the local player joined before it,
comparing the time its join menu last closed (0x1000e880). FnQL-game has no
such menu and uses the last spectator-to-team snapshot transition
(inference).

Votes queue `vote_now` only for a non-empty vote string (0x10049c5b) and
`vote_passed`/`vote_failed` from the server print (0x1004b09d). Rune
pickups play the item's own sound and queue `scout`, `guard`, `damage` or
`armor_regen` (0x1001a557), replacing missing Team Arena `cl_*.wav` files.
The single-player win/lose commands use `you_win`/`you_lose`. The obsolete
Quake III registrations and the dead team-sound switch were removed.

`cgame-localsounds` covers registration by gametype, every beep mode,
tier inversion and malformed tiers, kill-beep selection and volume, the
spawn-count guard, medal order, first-award voices, all three variants,
silent resets, player-event priority, the flag voice gates, lead changes
and their cvars, frag warnings and the intermission split.
`cgame-matchclocks` covers the time warnings: strict boundaries, flag
progression, the overtime and round-mode rules, the join guard, limits of
five minutes or less, warmup, timeouts, the round warning and its re-arm.
`cgame-events` covers the rune voices.

The full unit, ABI, contract, parity and every optional cgame/UI hosting
suite passed **90/90** in one run (`.tmp/cgame/poi/localsounds.log`).
Runtime probes disable audio, so these checks prove dispatch and
selection, not audibility.

## Team announcer (2026-11-05)

The inherited Quake III handlers for global team sounds 0..13 played
`sound/teamplay/voc_*.wav` and `sound/feedback/*leads.wav`/`teamstied.wav`
voices. None of these exist in the installed paks, so every team voice was
silent. Only the flag capture/return effects resolved, through `.ogg`
fallback. A scan of all `cg_main.c` registrations against the installed
paks found 62 missing paths. `CG_QLTeamSound` now handles these subcases
from retail switch 0x1001b349, using the local client's team:

- 0/1 captures: `flagcapture_yourteam`/`_opponent` effect only, on the
  local-sound channel; spectators get the opponent effect.
- 2/3 returns: `GTS_RED_RETURN` returns the blue flag. Its owners hear the
  `flagreturn_opponent` effect and "your flag returned", the other team
  `flagreturn_yourteam` and "enemy flag returned", and anyone else
  "blue/red flag returned".
- 4/5 takes: `GTS_RED_TAKEN` means red took the blue flag. The victims hear
  "your flag taken", the takers "enemy flag taken" and spectators
  "blue/red flag taken". One Flag players hear the return effects instead.
  The snapshot player carrying that flag or the neutral flag hears nothing
  here, since the local pickup voice covers it. Both cases also call the
  auto-follow helper.
- 6/7 obelisk under attack: "base attack" for the owning team.
- 8/9 scores: Freeze Tag center-prints "Red/Blue Scores" (y 90, scale 0.5).
  Outside A&D the scoring team hears "your team scores", the other team
  "enemy scores" and anyone else "red/blue scores".
- 12 teams tied: "teams tied" unless in A&D or `cg_announcerTiesVO 0`
  (new; retail 10076ac0, default 1).
- 13 kamikaze: `sound/items/kam_explode_far.ogg` on the local-sound
  channel.

Registration gates mirror retail, and a voice retail would not register
stays silent. Flag returns and team voices register from TDM up
(0x1002112b). Flag captures register for CTF, One Flag, Overload,
Harvester, Domination and A&D (0x100213de). The flag-taken/returned voices
register for CTF and A&D (0x10021646), "base attack" for Overload, and the
far kamikaze sound always. Voices resolve through the announcer folder at
event time, as elsewhere in this module.

Retail EV_GLOBAL_ITEM_PICKUP (0x1001a5ab) also queues "quad damage",
"battlesuit", "haste", "invisibility" or "regeneration" for item tags 4..8
unless Quad Hog is on. Its powerups call 0x10019d20: with
`cg_followPowerup` (new; retail 10077438, default 0, range 0..2), a free
spectator sends `follow <client>`, and mode 2 appends ` pw` for powerups
but not flags. FnQL-game qagame already accepts `pw`. The local flag-pickup
voice (0x10043893) is now the queued QL "you have flag". It is checked in
CTF, One Flag, Overload, Harvester and A&D but registered only for CTF, One
Flag and A&D (0x10021790), so the gate covers those three. Retail errors on
pickup indices outside 1..59, while FnQL-game keeps its existing silent
rejection.

`cgame-events` covers every branch, including local versus followed team,
the snapshot carrier exclusion, the One Flag substitution, gametype
registration gates, both cvars, follow eligibility and suffixes, the
Quad Hog suppression and the dispatcher fallback for unused subcases. Runtime
probes disable audio. A kept FnQL-game Freeze Tag thaw probe consumed a
real broadcast return subcase without errors
(`.tmp/cgame/poi/thaw-teamsound/`), but nothing checks audibility. The
unit, ABI, contract, parity, cgame-hosting, match and effects hosting
suites passed **72/72** in one run (`.tmp/cgame/poi/teamsounds.log`).

## World pings and objective announcements (2026-11-05)

Before this slice the cgame had no EV_POI (90) handler. The event fell
through to the unknown-event error, which drops the client. Both qagame
providers send it: Quad Hog carriers (`g_quadHog`, FFA) on pickup and every
`g_quadHogPingRate` ms, and neutral flag carriers. This failure path comes
from the code; it was not reproduced with an older build. Retail cgame
evidence:

- Event 0x1001c228 reads the event origin, lifetime `powerups` (ms), fade
  delay `powerups / 4` (signed, truncating) and `generic1`. The settings
  word from CS 0x2c0 (0x10a3ff28) selects the kind. Bit 18 (Quad Hog) gives
  48 units up, color (1, .75, 0) and `gfx/2d/quad_hog/quadhog`, and
  `generic1` is the carrier client. With neither bit, the record is a
  neutral flag ping: 64 units up, white, `sprites/neutralflagcarrier`,
  `cg_poiMaxWidth`/`cg_poiMinWidth` scaled by 0.75, and `generic1` is the
  team. Bit 26 alone (Red Rover infection) gives 48 units up, white,
  `gfx/2d/infected/bite`, team `generic1`. Retail's centity has eight bytes
  before `currentState`, so its `arg2[0x31]`/`arg2[0x38]` are `powerups`
  and `generic1`; the qagame emitters agree.
- Icons are registered only where used (0x100237cf, 0x100240dc,
  0x100241f4): Quad Hog in FFA, the bite in Red Rover, and the neutral flag
  in CTF, One Flag and Attack & Defend. All three exist in installed
  `pak00.pk3` (two PNGs and a `scripts/gfx.shader` entry).
- The 32-record allocator 0x1002a0d0 refuses race free spectators and
  drops new markers when full. 0x1002a190 runs at frame start (from
  0x1004e4e0) and keeps a record while `time < start + lifetime`. After
  the delay it sets alpha to `1 - (elapsed - delay) / (lifetime - delay)`,
  clamped to 0..1; color channels do not fade.
- Drawer 0x10011680 hides a record whose team equals the snapshot
  `PERS_TEAM` in gametypes from TDM up. With Quad Hog on, it also hides the
  record whose client is the snapshot client. It skips points on or
  behind the view plane (dot with the forward axis <= 0), then projects as
  for race icons.

`cg_poi.c` implements this. `cg_worldmarkers.c` now holds the shared
0x10011680 projection and the cvar limits, used unchanged by race icons
and by pings. Widths are captured per ping, as retail stores the cvar
values in the record. Divergences, all inferences: points of interest
have their own 32 records, instead of sharing retail's queue with damage
plums and other markers. A clock running backwards expires a ping instead
of leaving it on screen. Timing uses integer milliseconds where retail
uses single-precision time.

Global team sounds 23 and 24 were silently ignored:

- 23 (0x1001bce3) is a domination point change. `modelindex2` is the
  capturing team and `powerups` the point. A capture center-prints
  "%s captured point %s!" ("Red" for team 1, otherwise "Blue"; points 1..5
  are A..E, otherwise "?") at y 90, scale 0.5, as round results do. With
  `cg_announcerLeadsVO` (new; retail table 10076a90, default 1), the local
  client's team hears `<x>_captured`, other players hear `<x>_lost`, and
  spectators hear nothing. Warmup captures (point 0) use
  `flagcapture_yourteam`/`flagcapture_opponent` and do not print. A point
  turning neutral (team 0) plays `flagreturn_opponent` on the local-sound
  channel. Registration (0x100214a3) is domination only. Retail indexes
  six-entry tables directly; other point numbers stay silent here.
- 24 (0x1001bb46) plays `sound/feedback/survivor_01.ogg` on the
  local-sound channel to the team in `modelindex2` (registered for CA,
  Freeze, A&D and Red Rover, 0x1002138b). FnQL-game qagame sends it for Red
  Rover infection survivor bonuses.
- 10/11 (0x1001b750/7b3), taking the lead, now use the QL voices instead of
  Quake III's `sound/feedback/redleads.wav`, which the installed paks lack:
  `lead_taken`/`lead_lost` for players and `red_leads`/`blue_leads`
  otherwise. Attack & Defend and `cg_announcerLeadsVO 0` are silent.

The `cgame-poi` unit gate covers registration for all thirteen gametypes,
the three ping kinds, the per-ping width capture and cvar limits, fade at
exact points, team/own-client/behind-view filters, zero, negative, tiny and
`INT_MAX` lifetimes, clock rewind, queue saturation and reuse, race
spectators and on-screen placement. `cgame-events` adds the domination,
survivor and lead branches, their registrations, and EV_POI dispatch.
`cgame_probe.py --poi` starts a Quad Hog FFA bot match without input
injection and requires a client-observed EV_POI, the pickup notice, no
unknown-event error and a normal exit. Both providers passed:
`.tmp/cgame/poi/retail/` and `.tmp/cgame/poi/fnql/`. The retail-provider
capture shows the gold ping icon at the carrier's position through a wall.
The optional `cgame-effects-hosting` suite now runs both. Domination,
survivor and lead sounds have unit coverage only; runtime probes disable
audio.

## Cursor input (2026-11-05)

FnQL's `CL_MouseAbsoluteEvent` sends native modules absolute positions,
projected into the module's glconfig space. The inherited Team Arena
handler added them as deltas, so the cgame cursor ran to the 640x480
corner. Retail export slot 8 (0x100208f0) saves the previous cursor and
tail-calls 0x1003c4f0, which maps the pixel position onto the canvas. With
a positive widescreen bias (0x10a3ff04, which equals `cgs.screenXBias`), x
becomes `(x - bias) / (vidHeight / 3 * 4 / 640)`, the divisor rounded to
single precision; otherwise `x * 640 / vidWidth`. y becomes
`y * 480 / vidHeight`. It truncates both (`_ftol2`), does not clamp, and
calls `Display_MouseMove(NULL, x, y)` (0x10063830: ebx = x, eax = y), with
no cursor-type or captured-item handling. `cg_native.c` now exports that
mapping, and `CG_MouseMoveTo` applies it. FnQL-game keeps its inherited
release of the cgame key catcher during ordinary play. Nonpositive video
sizes are rejected instead of divided by. The previous-cursor copy has no
consumer found yet and is not kept. The legacy `vmMain` delta path is
unchanged. `cgame-native` checks 4:3 and pillarboxed 16:9 mappings,
truncation, unclamped edges, extreme inputs and zero sizes. Runtime probes
inject no input, so the cursor has unit coverage only.

## Frozen-player presentation (2026-11-05)

Retail 0x1003f057 gives frozen players animation speed zero before checking
haste. This uses the selected animation's initial frame through the existing
lerp evaluator, rather than retaining an arbitrary mid-animation pose. The
ordinary animation resumes after thaw. Retail 0x1004198c adds an ice pass
after existing powerup passes: `generic1 & 2` selects `powerups/ice1`, else
`generic1 & 1` selects `powerups/ice2`, otherwise `powerups/ice3`. Invisibility
takes precedence under ordinary player-clipping settings. Registrations are
at 0x10023567/57c/594; all three
materials were verified in installed `scripts/gfx.shader`. No special weapon
hiding was added: retail 0x10042b8b still calls the player weapon path while
frozen (its exception there is for infected Red Rover).

These functions and the existing sprite selector now live in
`cg_playerpresentation.c`, retaining baseline provenance. The new unit gate
checks actual submitted render passes, all thaw-progress bit combinations,
missing-material fallback, preserved render properties, invisibility,
quad/regen/battlesuit combinations, animation freeze/thaw and haste.

The capture comparison also exposed the inherited self friend-sprite default:
its unavailable asset could cover most of the view when the camera was near
a wall. Retail 0x10041205 suppresses local-client sprites unless
`cg_drawSpriteSelf` is enabled (table 0x10077288, default `0`). FnQL-game now
uses that rule, including third-person views, and tests self/other-player and
connection-sprite behavior. Other-player teammate/frozen markers still need
the retail selection, placement and scaling work; this does not claim full
player-marker parity.

`cgame_probe.py --frozen --map thunderstruck` requires a local-player obituary
after `IN_PROGRESS`, then turns the third-person capture toward the player
and hides 2D UI. It never controls input devices. The corrected capture in
`.tmp/cgame/probe-ugfr68nb/` visibly shows the ice body without the placeholder
sprite. The installed retail-cgame reference in `.tmp/cgame/probe-08ydg_qj/`
was captured using `--retail-cgame`, which leaves the installed module and
assets in place. Body treatment is qualitatively consistent; differing
combat timing, lighting and camera positions prevent a pixel-parity claim.
Both provider variants are included in `cgame-effects-hosting`.

## Ammo warnings and no-player-clipping materials (2026-11-05)

Retail cgame 0x10042e50 evaluates only the selected **snapshot** weapon,
using the capacity at `0x10079078 + weapon * 0x30`. These fifteen capacities
were checked against the existing `bg_weaponMaxAmmo` table. The comparison
uses a floating threshold: with capacity 25 and percentile 0.22, five rounds
is low because the threshold is 5.5. The reconstructed source truncates this
threshold; the shipping disassembly explicitly stores and compares a float.
Ammo -1 is unlimited. Zero is empty; positive ammo below threshold is low.
Gauntlet and grapple have capacity -1 and never generate normal ammo warnings.
Unselected ammo and weapon-ownership bits do not enter the decision.

`cg_ammo.c` implements that policy separately from playerstate transitions.
The native range table registers `cg_lowAmmoWarningPercentile=0.20`, range
0.01..1.00, and `cg_lowAmmoWarningSound=1`, range 0..2 (0x10077828/40).
Entering empty plays `sound/weapons/noammo.ogg` regardless of the low-ammo
sound option. Entering low plays `sound/weapons/lowammo.ogg` for mode 1,
the empty sound for mode 2, and nothing otherwise. Retail compares the mode
as a float, so fractional settings are silent. Unchanged states and recovery
are silent. Asset registrations follow 0x10021b8b/0x10021ba5. Missing sounds
do not issue a zero-handle playback request.

`cg_drawAmmoWarning` now defaults to 2 (0x10077018). Retail drawing at
0x1000e390 centers normal-font white text at x=320 with shadow style 6;
mode 1 uses scale 0.35/y=55, mode 2 uses scale 0.24/y=52. Zero disables it.
The text still uses the shared font bridge, whose wider visual parity work
remains open. Defensive handling clears warnings for missing snapshots,
invalid weapon indices and corrupt negative ammo; non-finite percentiles
and values outside the registered range cannot overflow or misindex.

Retail 0x10041878 tests **CS_SETTINGS_MASK bit 25**, not Race gametype or
the independently parsed movement flag. For an entity owned by another
client, it submits a single `noPlayerClipShader` pass ahead of invisibility,
quad, regen, battlesuit and frozen materials. The ordinary local body retains
those existing passes. Registration is 0x10023654. The installed
`scripts/gfx.shader` supplies an additive environment material with turbulence;
no asset or alpha approximation is shipped. The same helper also serves
missiles, preserving that ownership policy. Missile team lookup now validates
`clientNum`; the previous `clientNum & MAX_CLIENTS` selected the wrong client
or indexed past the array.

Weapon/barrel rendering at 0x10052350 also prioritizes the no-clipping
material, but compares ownership to **snapshot playerstate clientNum**, while
bodies compare the persistent local client. This matters while following.
After invisibility, local `cg_drawGun 3` selects `ghostWeaponShader`
(registration 0x1002366c). Ordinary weapons retain their base, battlesuit and
quad passes. Missing optional materials fall back to available existing
rendering. Models, skins, render flags, frames, colors and placement are
preserved. The settings bit is cached at initialization and refreshed on its
reliable configstring update; malformed/overflowing masks safely disable it.

`cgame-ammo` links the production classifier, drawing and shared capacity
table, covering all weapon boundaries, fractional thresholds, unlimited
ammo, silent recovery, sound modes, invalid inputs and both text layouts.
`cgame-playerpresentation` now also checks settings changes and malformed
masks, local/remote/followed bodies and weapons, ghost guns, material
precedence/fallback and unchanged submitted entity fields. Existing
freeze/animation/sprite tests remain in the same gate.

Four additional x86 runtime captures passed with retail qagame and installed
retail assets: FnQL/retail cgame side views using `--race-markers
--third-person`, and first-person views using `--race-markers --ghost-weapon`.
The no-clipping views show an opaque local body with a translucent remote
body and weapon; the ghost captures show the local translucent weapon.
The false gauntlet warning is absent. Materials are qualitatively consistent
with retail. Spawn choice and animation phase differ, so these are not
pixel-parity comparisons. HUD font/layout differences remain visible.
Logs and engine screenshots are retained in `.tmp/cgame/ammo-materials/`
under `fnql`, `retail`, `fnql-ghost` and `retail-ghost`. The new optional
`--evidence-dir` exports those two artifacts before the probe's normal
cleanup of its fresh private profile; all four fresh profiles were removed.
Previously blocked older profiles were not retried.

## Round-result presentation (2026-11-05)

Retail global-team-sound cases 18, 19 and 20 are now handled independently
in the QL event adapter. Round draw (0x1001b971) clears pending rewards and
announcements, preserving a future voice deadline, then centers "Round Draw"
and queues `round_draw`. It does not set the match-ended guard. Last standing
(0x1001bafb) queues `last_standing` only for the local client's team from
`modelindex2`; following another player does not change the recipient.
`cg_announcerLastStandingVO` defaults to 1 (table 0x10076a78).

For Red Rover, case 20 (0x1001bb6e) reads winner client indices from
CS_MATCH_AWARDS (0x2bf) and the infected flag, bit 26 of CS_SETTINGS_MASK
(0x2c0). Their update paths are 0x1004a05a/077. Non-infected winners get
"Round Winner(s)" and `round_over`. Infected draws list surviving names;
any listed local survivor gets `you_win`. Infected case 20 instead compares
the final listed client to the local client, matching 0x1001bc1f: that client
gets "Wins!" and `you_win`, while others get "Last Survivor(s)" and
`round_over`. Disconnected client names are omitted without changing the
retail token-count plural rule. All three new stock voice paths were checked
inside the installed retail paks (registrations 0x10021358/378/3cb).

The parser bounds decimal conversion, client indices, record count and
message buffers. Invalid/duplicate lists fall back to a generic round result
without partially displaying winners; an invalid settings mask becomes zero.
These are intentional hardening choices. Tests cover queue reset without
match termination, local-versus-followed recipients, disabled voice, all
winner/survivor branches, colors, disconnected clients, maximum client index,
malformed strings and numeric overflow.

`cgame_probe.py --round-result draw|survivor --map thunderstruck` pauses bots
after joining, uses a three-second round limit and waits for a positive
round-start configstring followed by its -1 end marker and the global-team
sound event. It captures through the engine and exits normally. Retail-qagame
captures in `.tmp/cgame/probe-p3bq751d/` and `.tmp/cgame/probe-7tloqn3h/`
visibly show "Round Draw" and "Last Survivor: Sarge". FnQL-qagame survivor
evidence is in `.tmp/cgame/probe-62f0b0ui/`. The `cgame-match` suite includes
both variants with both providers. These probes do not prove audible mixing;
last-standing routing and non-infected winner variants have focused unit
coverage. The inherited out-of-ammo warning overlaps the survivor message
in the infected capture; general HUD placement remains open.

## Prediction cache and command history (2026-11-05)

The inherited cache comparison omitted QL playerstate fields. The new
`cgame-prediction` gate first reproduced loss of a server correction to
`jumpTime` (playerstate offset 460), despite the acknowledged command time
matching a cached prediction. Retail 0x100448ea–0x10044929 starts each replay
from the complete acknowledged state. FnQL-game retains incremental replay,
but now invalidates cached predictions when jump/double-jump/crouch/slide
timing, primary weapon, FOV, location or armor-tier state differs. The test
independently changes each field and verifies the resulting full playerstate
against fresh production Pmove replay.

Command reads are checked before replay changes state or emits events.
Unavailable oldest, newest or intermediate commands preserve the last valid
prediction and invalidate its cache; recovery starts with fresh replay.
Non-predicting interpolation falls back to snapshot angles if the latest
command is unavailable. The disconnect predicate follows retail
0x1000af31–0x1000af4f, including its three-second level-start grace period;
failed reads, future commands and clock rewinds cannot establish a warning.
This does not silence valid exhausted-history warnings.

The offline harness links the actual predictor, shared movement/settings,
trajectory and math code. Its deterministic floor and synthetic command
records stay inside the test process; it does not control input devices.
A 399-command course runs with both default and PQL movement-profile flags,
covering running, strafing, jumping, crouching, turning,
more than 100 delayed snapshot acknowledgements and repeated cache wrap.
Every resulting playerstate is compared with sequential production Pmove.
A collision-call bound protects incremental reuse from degenerating into
full replay each frame. Additional cases cover missing-history recovery,
initial failure, demo/follow interpolation, unavailable local angles,
teleport exclusion and disconnect timing boundaries. This is not G4: both
sides use FnQL-game Pmove, and stock-map/live/retail-server corpora remain
required.

The later combined 56-check regression log is retained in
`.tmp/cgame/tests.log`, including the two-profile prediction gate.

`scripts/prediction_probe.py` now accepts `--cgame` and `--retail-qagame` for
the native client/server matrix, verifies module identity and normal active
completion, rejects mismatched requested settings and can retain evidence
with `--keep`. It runs windowless (`com_headless 2`) with mouse/joystick
disabled, so it never shows a window or takes focus. Its console movement course still requires the user's specific
input-control permission under the project rules; it was prepared but not
run as part of the offline gate.


## Race state and HUD (2026-11-05)

`cg_race.c` consumes events 93/94/95 and reliable `race_init`/`race_info`.
Retail cgame 0x1001aba4, 0x1001acf8 and 0x1001aeb9 establish the event
fields and recipient: the snapshot client, including a followed runner.
The six reliable fields come from 0x10048820. Reset at 0x1002a4a0 preserves
records after a finish, clears them on initialization, and requests
`raceinit`; playback stays local. Parsers reject truncated, overflowing and
out-of-range records without partial updates. Private sidecars do not
change the engine/shared ABI.

Start/checkpoint sounds follow the eight-choice table registered at
0x10021c4d..0x10021cf9. The installed default is `cg_raceBeep 7` (bell);
other out-of-range choices are silent. Checkpoint messages show signed
best-run deltas and, by default, at most three remaining points.
0x10032240 supplies the count calculation from CS_RACE_RECORDS. Finish
always uses `sound/world/klaxon1.ogg`, with last/best updates only for valid
completions. Paths were checked against the installed paks; these probes
disable sound and do not establish audible parity.

HUD ownerdraws 52/53 show CURRENT RUN or LAST TIME and the corresponding
`m:ss.mmm` time. Retail 0x1002f1f0 uses timeout-start CS 0x29d while paused;
0x10049b91 shifts the run origin when level-start time changes. Labels are
yellow and numbers white (retail data 0x100761e4/0x10076214). Publishing
`cg_gametype` from serverinfo, as retail does at 0x10048967, fixes the
installed menu's previously hidden race panel. Integer decomposition
deliberately preserves exact milliseconds at long durations instead of
retail's float rounding. The inherited formatter supports `%i`, not `%u`;
linking it into the unit gate reproduced the initially blank timer and
guards the correction. The engine-rendered capture confirms a visible
LAST TIME and `0:00.765` for the corresponding server completion.

`cgame-race` covers local/followed recipients, sound selection, malformed
commands, signed splits, aborted finishes, record preservation, pause and
restart clocks, HUD alignment and colors using the production formatter.
`cgame-events` separately checks dispatch of all three race events.
The optional `cgame-race-hosting` suite uses a hand-authored entity override
on the installed Campgrounds BSP. Both possible spawns fall through three
points above its platform and below player clip; a paused bot supplies the
two-player minimum. No movement input is injected. The gate requires all
three events, the local runner's finish and cgame's subsequent reset request.
An earlier unusable course also failed with retail cgame; it was corrected
without changing the shared snapshot/event suppression behavior.

`cg_racepresentation.c` also adapts general race entities using retail
0x100155d0. The next checkpoint changes to the blue model, a finish uses
the finish model, and later inactive/finish flags are hidden during a run
except for the following point. Floor projectors retain the retail skin
selection and z-16 offset even when the later flag is hidden; floating
spawnflag one omits them. Item-effect bits select the observed 1024 ms bob
and shared slow/fast rotation. Missing replacement models retain the
existing valid model. Non-race/general entities keep their previous path,
with a bounds guard before indexing the model table.

Navigation icons select start/checkpoint/finish assets and colors from
0x10015822 and 0x1002409b. They project the unbobbed origin plus 64 units,
with a camera-up half-width probe and the configured minimum pixel size
(0x10011680). Icons are horizontally centered and bottom-anchored at that
projected point (0x100119e3/0x10011a42); matched-position captures exposed
and corrected an initial half-icon vertical displacement.
`cg_poiMaxWidth` controls world size, not a pixel maximum;
the retail 16..32 / 2..16 cvar bounds are applied, including safe handling
of non-finite input. Behind-camera points and invalid snapshot/model
references are skipped. Free race spectators receive no icons; following
spectators do. The frame-local pass is bounded to retail's 32-marker
capacity without retaining pointers or adding a shared engine structure.

The respawn hint follows 0x1002f260..0x1002f400: it reads the existing `kill`
binding through native import 98 and uses half-opacity white. Bound-key
reminders stop after the user invokes `kill`; an unbound key still prompts
binding. Demo playback and spectators suppress the hint. The command
continues to forward `kill` to the server (0x10007e70), and race resets
preserve the session's used-hint flag. No bindings or input devices are
changed. The native adapter handles a missing key-name import and forces
bounded termination; its ABI gate checks the slot and behavior.

`cgame-racepresentation` covers model progression, free/follow spectators,
projectors, floating points, item effects, missing models, minimum/world
icon sizes and invalid input. The race-state test covers hint selection,
alignment, suppression and reset lifetime. `--race-markers` adds a
gravity-only course that stops after a start with separated upcoming
points in view; both qagame providers run it in `cgame-race-hosting`.
The initial retail/FnQL cgame captures used the same requested FOV and enabled
FnQL's inherited widescreen adjustment for comparison. The later FOV slice
removes that probe override and uses the default retail aspect path.
Live timer values and
rotating flag phases are not synchronized between processes.
The final reference/FnQL captures both report view position `(-625 320 308)`
and yaw zero. Visual inspection confirms the corrected checkpoint icon
location and size; `reference-markers.jpg` and `fnql-markers.jpg` plus their
logs are retained under `.tmp/cgame/race/`.

This is functional race presentation, not full visual parity. Exact HUD
and center-message font/scale parity remains open. The initial captures
showed overlapping widgets, a false low-ammo warning for the gauntlet-only
fixture, and opaque race players. The later ammo and no-player-clipping
slice addresses the latter two; HUD overlap remains.
Selected runtime logs and the verified engine HUD capture are retained in
`.tmp/cgame/race/`; the complete regression log is `.tmp/cgame/tests.log`.

## View FOV and zoom (2026-11-05)

`cg_fov.c` separates projection/zoom policy from the inherited camera-position
code. Retail cgame disassembly at 0x1004d640 and aspect helper 0x1004c5f0
are authoritative here; the reconstructed source differs in several branches.
The projection starts with a 640:480 view. `r_aspectRatio` zero keeps that
horizontal FOV, 1 selects 16:9, 2 selects 16:10, and other nonzero values
select 5:4. FnQL publishes the preset from its renderer configuration.
The inherited explicit `cg_fovAdjust` option still selects actual viewport
aspect and retains its upstream attribution; zero now selects retail policy.

The cvar table at 0x10077540 and 0x10078410 gives `cg_fov=100` and
`cg_zoomfov=30`, each bounded to 10..130 through native import 5.
`cg_specFov`, `cg_zoomScaling`, `cg_zoomOutOnDeath` and `cg_waterWarp`
default to one, `cg_zoomToggle` to zero, and `cg_zoomSensitivity` to one
(range 0..10). Range registration refreshes the mirrored value after the
engine applies its clamp. A missing range import falls back to ordinary
registration; the view calculation also has defensive bounds.

Retail 0x1004d731 selects the nonzero snapshot FOV when `cg_specFov` is on
and either `PMF_FOLLOW` or demo playback applies. This bypasses local zoom;
intermission takes precedence and uses 90. Otherwise zoom transitions blend
over 150 ms, or immediately select their target when scaling is disabled.
Disassembly at 0x1004d7de–0x1004d837 shows that **both transition directions
publish the zoom target in user commands**, including the frame at 150 ms,
while the rendered view blends. Outside the transition, commands use the
selected local/zoom/recorded FOV. Projection and water warp never alter it.
`trap_SetUserCmdValue` now publishes this private integer after calculating
the current view, matching 0x1004e597–0x1004e5c6; the former adapter sent the
previous frame's projected horizontal FOV. No shared ABI layout changes.

Zoom commands follow 0x1004d5b0/0x1004d610: hold or toggle behavior, with
zoom-down disabled while dead/frozen. Retail 0x10011520 clears the zoom flag
after view calculation on death/freeze when enabled, and unconditionally at
intermission. It does not restart the transition. Map restart resets only
the transition timestamp (0x1004a351). Water/slime/lava still report the
underwater state when warping is disabled. Enabled warp is one degree at
0.4 Hz, applied before the retail aspect projection; the disassembly doubles
the phase, not the sine amplitude. Unzoomed sensitivity is one. Zoomed
sensitivity is vertical FOV divided by 75 when its setting is zero, or the
ratio to the local unzoomed viewport vertical FOV times that setting.

Explicit hardening differs from retail's unchecked inputs: non-finite local
FOVs are bounded, invalid viewport sizes use 4:3 for the optional projection
and sensitivity denominator, malformed recorded FOVs stay in 1..179, and
clock rewinds settle to the selected zoom state rather than extrapolating.
Elapsed-time subtraction uses a wider type to avoid signed overflow.
Warped angles remain strictly between zero and 180 degrees even for malformed
recordings; overflowing aspect-preset strings use the nonzero fallback.

`cgame-fov` exercises production calculations for every aspect preset,
resized/invalid viewports, the inherited override, both transition endpoints
and midpoint, command/render separation, spectator/demo/intermission
precedence, water toggles, sensitivity, hold/toggle controls, death policies,
clock rewind/overflow and invalid numbers. The native gate checks import 5,
its fallback and mirror refresh, and distinct command/rendered FOV values.
Runtime zoom/follow/demo capture parity and broader camera behavior remain
open; these unit checks do not substitute for that corpus.

The final x86 DLL also passes `cgame_probe.py --race-markers` with retail
qagame. Its engine capture in `.tmp/cgame/probe-gkf75xuo/baseq3/screenshots/`
(`shot-20261105-112339.jpg`) and `qconsole.log` record position
`(-625 320 308)`, yaw zero and requested FOV 90. World framing and checkpoint
marker placement match `.tmp/cgame/race/reference-markers.jpg` without
forcing `cg_fovAdjust`. Timer/animation phases are not synchronized.
An earlier valid hosting capture in `probe-_9jpgujg` selected the other
spawn at x=-575 and was excluded from the visual comparison. Cleanup of
these two temporary profiles was rejected by automatic approval review
("blocked by policy"), so they remain. HUD font/layout and the false
gauntlet low-ammo warning remain visible and are outside this FOV slice.

## Score bring-up evidence (2026-11-05)

`cg_scores.c` replaces the inherited Q3-only score parser with independently
written field maps and a bounded reader. The main command contracts are:

| Command | Header and row shape | Retail cgame evidence |
| --- | --- | --- |
| `scores_ffa` | count, two team scores, 18 integers per player | 0x10045090 |
| `scores_duel` | count (at most two), 13 base fields, four pickup/wait pairs, fourteen five-field weapon records | 0x10045440 |
| `scores_race` | playing count, four integers per client; connected spectators may follow | 0x10045db0 |
| `scores_tdm` / `scores_ft` | 28 team statistics, count, two scores, 15 / 17 integers per player | 0x10045f20 / 0x100476a0 |
| `scores_ca` | count, two scores, 16 integers per player | 0x100467e0 |
| `scores_ctf` | 34 team statistics, count, two scores, 17 integers per player | 0x10046bf0 |
| `scores_rr` | count, two scores, 19 integers per player | 0x10047de0 |
| `scores_ad` / `adscores` | twenty round entries and two team scores, preserving player rows | 0x100475f0 |

The retail server corroborates these shapes at qagame 0x1003d090 (FFA),
0x1003d2b0 (duel), 0x1003dbb0 (race), 0x1003dd20 (TDM), 0x1003e300 (CA),
0x1003e6d0 (CTF), 0x1003eda0 (FT), and 0x1003f260 (RR). The generic
`scores` command accepts retail's eighteen-field layout (0x1003cec0), while
retaining the inherited fourteen-field shape when its exact length identifies
it. `smscores` accepts eight fields per row (0x1003cd70).

Auxiliary `tdmstats`, `castats` and `ctfstats` address scoreboard **row indices**,
not client numbers (qagame 0x1003e1b0, 0x1003e510, 0x1003ec40). They retain
unmodified fields. CA includes the reserved weapon slot 15 in its detail reply.
Race validates appended spectator records but displays the playing count,
matching retail's client parser. Duel wait values are finite, nonnegative
floats; weapon records cover slots 1 through 14.

Malformed commands are rejected as a whole, preserving the prior scoreboard:
exact field counts, signed integer overflow, client/team/weapon bounds,
duplicate clients and invalid duel waits are checked before commit. This is
intentional hardening of retail's unchecked conversions. `cgame-scores` tests
production parsing and cell formatting with distinct field values, boundary
clients, truncated/extra/overflowing input and unchanged-state assertions.

`cg_scorecells.c` supplies primary free/team/race player rows and bounded team
filtering. Retail cell dispatch is 0x10028830; FFA is 0x10026160, CTF
0x100268b0, CA 0x10027240, Race 0x10027c40, and TDM/FT 0x10027e30.
Retail, rather than the reconstruction's labels, determines the columns:
FFA/CA best-weapon accuracy differs from overall accuracy; the last team
column is ping; Freeze's extra column is assists/thaws. Team score/name
ownerdraw anchors follow 0x1002ff50/0x10030090, placing text at the rectangle
origin with the requested alignment. Four secondary team/spectator visibility
bits follow 0x10031a84–0x10031ae5; other secondary bits remain open.

The installed `ui/hud.txt` does not load the mode-specific scoreboard menus.
Cgame now explicitly loads both `ui/ingame_scoreboard_<mode>.menu` and
`ui/end_scoreboard_<mode>.menu`, selects their mode-specific names, and reloads
them with the HUD. The shared menu reader accepts their colors, item anchors,
secondary visibility flags and longer column lists. Allocation failures are
checked; the cgame pool is 1 MB instead of the insufficient inherited 128 KB.
No menu assets are shipped. `defaultContent` uses the local background shader
fallback; `cellId` is consumed without invoking an external advert service.

`cgame_probe.py --scoreboard` requests scores through the engine console and
requires a score reply without rejection, active play, engine capture and
normal shutdown. `--reload-hud` reloads the menus eight times first and
exercises a missing-custom-HUD fallback. Meson's
`cgame-scoreboard` suite covers FFA and TDM with both qagame providers.
Additional retail probes passed Duel, Race and CA on Campgrounds and CTF on
Ironworks (Campgrounds correctly rejects the CTF factory). Engine captures
show populated primary rows and corrected team score/name placement.
Evidence is in `.tmp/cgame/probe-27vg0q2t/` (FFA),
`.tmp/cgame/probe-w_glcq_3/` (TDM) and `.tmp/cgame/probe-v622y1y3/`
(CTF after the anchor fix and repeated reloads).

These checks establish parser and menu bring-up, not visual parity. Detailed
end-table feeders, duel summaries, team pickup/selected-player ownerdraws,
friend/country/readiness icons, some visibility bits and Overload's fallback
remain incomplete. The waiting-for-players overlap seen in these older
captures is addressed by the normal-match pregame handler described above.

## Team information and accuracy panel (2026-11-05)

Retail `tinfo` (cgame 0x100487b0, qagame 0x10068490) is a count followed by
client indices. The old six-field reader overwrote health/armor with missing
arguments and misidentified later teammates. The new reader accepts the
compact form without changing vitals; the exact six-field inherited form is
retained for legacy senders. Counts are bounded to the existing overlay array,
and invalid/duplicate clients, incomplete records, invalid location/weapon
indices and overflowing integers reject the whole update. Retail's sender
currently lists at most eight players; the local receiver safely supports its
existing 32-entry overlay capacity.

Retail overlay reads at 0x1000a173–0x1000a1bb obtain health, armor and weapon
from entity state, with the current player's own playerstate taking precedence.
FnQL-game adapts that data to the inherited HUD's clientInfo fields after both
initial and subsequent snapshot transitions. Entity number must match the
client, and the entity must be a player or invisible player; corpse copies
cannot overwrite live vitals. Location and weapon indices have safe fallbacks.
Powerups are copied from entity state, or reconstructed from the local
playerstate slots. These sidecars do not change any shared ABI layout.

Both `acc` and `pstats` use fifteen integers, including weapon slot zero
(retail cgame 0x100481a0; qagame 0x1003f4d0/0x1003f660). Parsing is
transactional and retains percentages above 100. The server sends a score
update with `acc`, which supplies the selected player's overall accuracy;
it is not an average of weapon percentages (cgame 0x10034590).

`+acc`/`-acc` now show/hide the installed `ui/ingamestats.menu` without
changing input capture. Retail 0x10006d60/0x10006db0 and 0x1000e320 govern
the free-spectator exclusion, following-player eligibility and one-second
refresh interval. Demo playback displays cached data without sending network
requests; clock rewinds reset the request timer. `CG_WP_VERTICAL` and
`CG_ACC_VERTICAL` follow 0x10035a10/0x10035b10: registered weapon icons
from machinegun through heavy machinegun, excluding grapple, use the menu's
row spacing. The overall field paints at its origin, as 0x10034590 does.

The expanded `cgame-scores` test covers compact/legacy team replies,
unchanged state after rejection, snapshot vitals and local-player precedence,
corpse/invalid entities, weapon accuracy payloads, visibility, request
throttling/clock rewind/demo playback, and actual text/icon draw calls with
distinct nonzero values. The optional `cgame-stats` suite checks FFA and TDM
with both providers, requiring repeated `acc` replies, `pstats`, a team reply
in TDM, no command rejection, an engine screenshot and normal exit.

All four statistics probes pass. Engine captures show the populated accuracy
panel with retail qagame in `.tmp/cgame/probe-hmclfovf/` and FnQL-game qagame
in `.tmp/cgame/probe-9fgyt68r/`; the latter also verifies the corrected overall
text position. The stationary runtime player has zero accuracy; nonzero and
over-100 values are checked through production drawing tests. Actual retail
engine hosting, full HUD visual parity, and the separate `+pstats` panel remain
unproven. Existing health/armor HUD placement and waiting-text overlap remain
visible in these captures.

## Level loading screen (2026-11-06)

The Quake III loading screen (proportional font, item and player icons,
hostname, pure/cheat lines and limits) is replaced by retail's
(`code/cgame/cg_info.c`). Observed in the retail cgamex86:

- `CG_LoadingString` (0x1001cb80) copies the text unmasked into
  `cg.infoScreenText`, then calls `trap_UpdateScreen` and the advertisement
  bridge's `UpdateLoadingViewParameters` (import 77) and `SetFrameTime(1000)`
  (import 64). Both slots are now named in `cg_native.h`; the FnQL engine
  already implements them.
- Items (0x100249d0) and clients (0x1001cbe0) still register their icons (38
  items, 16 clients) without drawing them. A client's text is its `n` key cut
  to 63 characters, minus `^0`-`^7` codes and control bytes (0x100574c0); an
  empty name stays empty. The graphics stage shows `CS_MESSAGE` before the
  world loads (0x10022f80).
- CG_Init (0x10029c58) counts finished stages after the collision map,
  sounds, graphics and clients and once more before the final empty loading
  string; the counter lives for the module.
- `CG_DrawInformation` (0x1001ce10) rereads `CS_SETTINGS_MASK` and draws
  either `backscreen_smoke` with "Awaiting gamestate..." (no loading text but
  a snapshot) or the levelshot (`menu/art/unknownmap` when missing) with: a
  0.75 dark-red bar, `logo_background` and `ql_logo` on the left; a 0.75
  black bar at y 400 with `CS_MESSAGE` (font 0, 0.8) and `CS_MAP_AUTHOR` /
  `CS_MAP_AUTHOR2` (font 1, 0.25) at x 10; `gt_background` and the 13-entry
  gametype name right-aligned to x 620 on the right; and, centred, "Loading
  %s" or "Awaiting Snapshot" (font 1, 0.3, shadowed) at y 370 over four 8x8
  progress squares, opaque dark red once their stage is done. Each text's y
  is its measured height plus 400, 447, 462 or 10. Full-screen images keep
  their 16:9 centre: the texture window is cropped by
  (1920 - 1080 / height * width) / 3840 on each side. Text styles were read
  from the call sites' pushes (style 3 for the shadowed lines), since the
  HLIL drops the ninth argument of the painter 0x10008440.
- The five loading shaders register early in CG_Init in retail order,
  including the unused `levelshots/loadingback.jpg`.

Gates: `cgame-loading` (unit) covers the strings, names, icon limits, crop,
gametype table, every layout element, the progress squares and both awaiting
states. `fnql-cgame-loading-960` (campgrounds, 960x600) and
`fnql-cgame-loading-1024` (aerowalk with two authors, 1024x768) host the
retail cgame and FnQL-game's on the FnQL client through the test-only
`tests/cgame_snapshot_hold_proxy.c`, which reports no snapshot to the wrapped
module until `pxholdsnap 0`, so each draws its "Awaiting Snapshot" loading
screen during ordinary frames; the engine screenshots match within the
menu parity threshold (no pixel differs by more than 16 levels). Earlier
desktop captures of the retail engine during loading showed the same layout
for both modules, but other windows covered parts of them, so the engine
captures are the evidence.
