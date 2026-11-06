# Baseline Import

Native cgame bring-up adds a typed retail ABI adapter, repairs the dormant
Team Arena HUD build, consumes shared QL movement settings and retail menu
constants, and handles retail PNG icons/native text/numbered crosshairs.
Legacy Q3-only configstrings and enums no longer enter this build. See
[CGAME.md](CGAME.md) for clean-room evidence, gates and remaining M7 gaps.

The cgame QL event adapter now handles surface footsteps, drown, armor regen,
thaw ticks, dropped-weapon selection, spectator item timers, lightning
discharge, awards and two announcer events. Award queuing is shared with the
inherited playerstate path and uses retail's 16-slot capacity. Native event
tests cover routing, bounds and effect parameters. Damage plums now have
bounded world-text storage, weapon filtering, native projection and color
modes; shotgun kill events produce blood and optional impact sparks from
the retail event fields. Distance-dependent plum sizing remains open.
No QLSRP implementation was incorporated.

Cgame now handles overtime, generic match results, explicit team wins and
red/blue round wins using retail event evidence. Result handling clears
pending rewards/announcements, preserves a future sound deadline, chooses
music/voice by the retail rank/team rules and suppresses later queued
announcements until map restart. `cg_buzzerSound` defaults on. Windowed
event-driven probes exercise complete matches and overtime; they leave the
command buffer free for warmup's queued restart and finish through loopback
RCON. Audio playback and full end-screen parity remain unproven.

Race events now select the snapshot runner, update private run/split/record
state, play the selected checkpoint beep and report finish deltas. Bounded
`race_info` and `race_init` handling, timeout compensation and the installed
race status/time ownerdraws are independently implemented from retail
evidence. Serverinfo now publishes retail's read-only `cg_gametype` HUD
selector. The gate links the real VM formatter, including its supported
signed-decimal format, and gravity-only runtime probes cover both providers.
Race points now select their active/following/finish model, floor projector,
bob and rotation from retail evidence. Projected navigation icons use the
retail colors, offset, world sizing and minimum size. The respawn hint reads
native key-name import 98 without changing bindings and retires after the
forwarded kill command. Private state, bounds and fallback behavior are
covered by focused gates; exact text/font parity remains open.

Weapon-list presentation now lives in `cg_weaponbar.c`, with retail persistent
left/right/center lists, snapshot ammo, full-list/follow policies and the timed
strip. Native ammo/name text, low/empty warnings and installed selection/infinite
images follow shipping binary evidence. Default `cg_drawWeaponSelect 1` now
uses `cg_weaponBar`; zero still hides and explicit alternate baseq3a layouts
remain available. Production tests and retail-module comparison captures cover
these paths. `cg_weaponselection.c` now implements shipping snapshot-based
cycle/direct/toggle policy, empty switching, frame-observed history, spawn
preferences, queued primary loadouts and weapon config dispatch. The native
adapter forwards the primary instead of zero. Pickup autoswitch defaults to
off and includes machinegun pickups when enabled, matching retail. Production
tests bound invalid indices/numbers; input-free hosting captures exercise
spawn preference and loadout/config behavior with both qagame providers.
See CGAME.md for evidence and deliberate hardening of retail's slot bounds.

Match/round clocks, round labels, overtime and local date/time now have
retail-backed handlers in `cg_matchclocks.c`. Private configstring state
handles round warmup precedence and timeout adjustments without changing
shared structures. Production tests cover clock boundaries and malformed
input; paired retail-module captures verify active round clocks, overtime
and clocks held during a pause. The input-free hosting harness has new round
and timeout gates for both providers and a finish-script marker that avoids
retry starvation during long capture waits. Pause/time-in banners and server
center notices (`cp`/`pcp`) now use retail text placement, lifetime and
console behavior. Timeout audio uses existing voice selection and native
volume import 40; the zero-count round-mode message handles AD sides and
RR infection. Production tests and three-stage time-in captures cover the
new path. Normal-match warmup presentation now lives in `cg_pregame.c`:
waiting/player-count/team-balance messages, ready/unready hints, mode/duel
titles and fixed-size countdowns use retail text and placement. Private
configstring state supplies readiness, ready deadlines, round state and mode
overrides. Initial preparation and per-second announcer cues replace the
inherited duplicate sound path; round start and warmup-free map restart share
the timeout path's retail match-start handler. Production tests and matched
FFA/CA captures cover the normal-match path; training/tutorial presentation
and broader menu-capture behavior remain outside this slice.

The top-left standings now have private configstring inputs and retail
mode icons, win limits, player/team rows, local-rank selection and
remaining-player/owned-point counters. Team HUD overrides use the shipping
`cg_redTeamName`/`cg_blueTeamName` cvars separately from inherited team names.
Clipped names use native import 123; retail reserves ellipsis space but its
local boundary copy leaves the dots hidden. Numeric ordering, malformed
updates, native clipping and mode gates have production coverage. Input-free
FFA/CA probes exercise long player names and custom team labels with both
qagame providers. See CGAME.md for observations and validation limits.

Status HUD values now use retail text anchors/ascent and the installed
infinite-ammo icon. New production handlers supply primary/excess health
and armor bars with retail cropped texture geometry, including the 22/32
bonus fill height, and team-tinted HUD images. The existing team background
and world-effect palettes are preserved. Tests cover these dispatch paths,
invalid snapshots/indices and screen transforms; a hand-authored factory
provides bonus-health/armor and finite-ammo runtime captures. See CGAME.md
for shipping-binary evidence and QLSRP reconstruction discrepancies.

Native HUD text now lives in `cg_text.c`. Painting and measurement use the
same retail pixel size (`scale * vidHeight / 8`); five-float visual bounds
and ascent are converted back to the existing HUD canvas. Native font
selection, colors, glyph limits, shadows and glyph quads are retained.
Production tests cover resolution transforms, invalid values and the bounds
packet; damage-plum packed measurement remains unchanged. Custom extra
spacing/cursor behavior and complete HUD visual parity remain open.

Cgame view projection and zoom now live in `cg_fov.c`, independently written
from retail binary evidence. The default uses retail's aspect presets,
4:3 command FOV, local/followed/demo selection, 150 ms zoom transitions,
sensitivity, toggle and death/intermission policy. Native range registration
supplies the retail cvar defaults/bounds. User-command settings are published
after the current view is calculated. The inherited explicit viewport-aspect
option remains available, with its original attribution. Production tests
cover these paths and invalid input; broader camera parity remains open.

Round draws, last-standing voice and Red Rover winner/survivor messages now
consume retail configstring and recipient evidence. Decimal lists and output
buffers are bounded, with generic fallback for invalid lists. Draws clear
pending announcements without ending the match. Paused-bot round probes
exercise CA draws and infected RR survivors; unit tests cover local/followed
team selection and the remaining winner-message branches.

The QL effect adapter now handles thaw fragments and damage-through missile
impacts. Thawing uses seven fragments with ice trails and one ice mark per
fragment; the existing fragment collision path stops their trails without
blood or bounce sounds. Through-surface impacts consume the retail distance
configstring, add a backside burn/debris effect when collision permits it,
and retain the ordinary explosion. Native values and trail catch-up work
are bounded. Focused effect tests cover both effects and fallback paths;
matched retail visual parity remains open.

Player animation, powerup passes and overhead-sprite selection are isolated
in `cg_playerpresentation.c` for direct regression tests. Frozen animation
uses zero speed before haste; `generic1` thaw-progress bits choose the three
installed ice materials after the existing powerup passes. Invisible players
retain their single invisibility pass. Retail's default-off `cg_drawSpriteSelf`
also prevents the inherited local friend sprite from obscuring third-person
views. Teammate marker presentation remains unfinished.

The cgame prediction cache now compares QL jump/crouch timing and additional
playerstate fields before reuse, preventing server corrections from being
replaced by stale cached state. A production-predictor harness reproduces
the original failure and compares delayed-acknowledgement replay with fresh
Pmove state, while bounding collision work. Missing command reads preserve
the last valid prediction and discard the cache. Disconnect display includes
retail's three-second level-start grace period. Full G4 remains open.

Cgame now parses the retail score command family with bounded, transactional
updates and supplies the main per-mode scoreboard rows. It loads the installed
ingame/end scoreboard menus alongside the HUD and reloads them together.
The shared menu reader accepts scoreboard colors, per-item widescreen anchors,
secondary visibility flags and up to 32 list columns; excess accepted column
definitions are consumed without desynchronizing the parser. Cgame's menu
allocation pool is enlarged to fit these assets, with allocation-failure guards.
Team score/name ownerdraws now use retail text anchors. Detailed end tables and
remaining scoreboard ownerdraws are still open; see [CGAME.md](CGAME.md).

The team-info reader now accepts retail's client-index list without erasing
vitals, while preserving exact legacy records. Current snapshots supply HUD
health, armor, location, weapon and powerup data. Bounded `acc`/`pstats` replies
feed a working `+acc` panel with periodic requests, per-weapon icons/accuracy
and the selected player's overall accuracy. This uses installed menu assets
and adds no input capture or shared ABI changes.

Ammo warnings now classify the selected snapshot weapon using the shared
retail ammo-capacity table and a floating percentage threshold. Unlimited
ammo is silent, transition sounds use the installed low/empty-ammo assets,
and both retail warning-text sizes are available. This replaces the inherited
Q3 loadout-wide estimate that falsely warned in Race. Production tests cover
all weapons, fractional thresholds, transitions, invalid input and drawing.

Server settings-mask bit 25 now selects the installed no-player-clipping
material for remote bodies, missiles and weapons, before ordinary powerup
passes. The body and weapon paths retain retail's different local/viewed
client comparisons. `cg_drawGun 3` selects the installed local ghost-weapon
material. Existing animation, powerup and freeze passes remain regression
tested. Missile client lookup also replaces an incorrect bit mask with a
bounded index, avoiding wrong team colors and a possible out-of-bounds read.

| | |
|---|---|
| Upstream | https://github.com/ec-/baseq3a |
| Commit | `a3085f7aa6a9242383795b2806b0124401d8a409` (2026-10-10, "Merge pull request #100 from WofWca/fix-scan-crosshair-entity-conservative") |
| Imported | 2026-10-29 |

baseq3a is ec-'s unofficial Quake III Arena game code patch on the 1.32
lineage. It provides security fixes, the float fix, a fixed vote and spawn
system, unlagged weapons, improved prediction, a map rotation system, and
single-line cvar, event and means-of-death declarations. Its upstream README
is kept at [`baseline/baseq3a-README.md`](baseline/baseq3a-README.md).

## What was imported

| Upstream path | Here |
|---|---|
| `code/` (game, cgame, ui, q3_ui) | `code/` |
| `docs/` (client, server and rotation notes) | `docs/baseline/` |
| `README.md` | `docs/baseline/baseq3a-README.md` |
| `QIIIA Game Source License.txt` | `docs/baseline/QIIIA-Game-Source-License.txt` |

## What was left out, and why

- `assets/`: Quake III artwork (icons, fonts, menu art) and other binaries.
  Retail Quake Live supplies its own assets from the user's install, and
  FnQL-game ships no game art.
- `build/`: the MSVC project files, Makefiles and q3lcc/q3asm toolchain.
  This includes prebuilt Windows and Linux tool executables and a `7za.exe`.
  Meson replaces all of it. Retail Quake Live uses native modules, so the QVM
  toolchain has no role.
- `QIIIA Game Source License.doc`: a binary duplicate of the `.txt` copy.

## Changes made on top of the baseline in milestone 1

- `code/game/g_syscalls.c`, `g_syscalls.asm`, `game.def` (the Quake III
  syscall ABI) were removed. `code/game/g_native.[ch]` replace them with the
  retail Quake Live native ABI.
- `vmMain` is no longer exported. It stays as the internal dispatcher.
- `q_shared.h`: `playerState_t`, `entityState_t`, `usercmd_t`, `trajectory_t`
  (`gravity`, `TR_QL_ACCEL`) and `vmCvar_t` (`flags`) now use the Quake Live
  layout. `g_public.h`: the `entityShared_t` pad is named `s`, as in FnQL.
  `scripts/check_engine_abi.py` holds these against the engine.
- `g_main.c` registers the `fnql_game` serverinfo identity cvar.

## Changes made in milestone 2 (wire contract)

- qagame builds with `MISSIONPACK`: Quake Live is Team Arena–derived.
- `bg_public.h`, `bg_events.h`, `bg_mods.h`: the retail gametype, powerup,
  holdable, persistant powerup, tier, key, item type, weapon, stat, persistant,
  event, means-of-death and configstring values (`docs/contract/`).
- `bg_misc.c`: the retail 60-row `bg_itemlist` and `TR_QL_ACCEL`; powerup
  lookups no longer match persistant powerups.
- `inv.h`: the retail bot item indices.
- `g_ql_entities.c` (new) and `g_spawn.c`: the eight retail-only spawn
  functions, in retail table order.
- Persistant powerups store their tag in `STAT_PERSISTANT_POWERUP`
  (`ai_dmq3.c`, `bg_misc.c`, `bg_pmove.c`, `g_active.c`, `g_combat.c`,
  `g_items.c`, `g_weapon.c`).
- `g_client.c`: the retail `CS_PLAYERS` keys and the SteamID from import 200.
  `g_main.c`, `g_spawn.c`: the retail `CS_WARMUP` infostring.
- Quake III writes into configstring slots that Quake Live uses for other
  things are gone: bot debug info, shader remaps and team votes (`ai_main.c`,
  `g_cmds.c`, `g_main.c`, `g_utils.c`).
- Quake III single player is unreachable (`G_IsSinglePlayer`); slot 2 is Race.
- Step events are replaced by `pmove_t.stepUp` (`bg_slidemove.c`,
  `ai_dmq3.c`).
- `ai_dmq3.c`, `g_weapon.c`: fixes for two Team Arena blocks the baseline
  never compiled.
- `ai_*.c`, `g_cmds.c`: voice chat names now come from `bg_voicechat.h`
  (from the GPL Quake III `menudef.h`).
- `g_bot.c`: arena and bot script buffers sized for the retail files.
- `g_ql_cvars.h` (new), `g_cvar.h`, `q_shared.h`: the 59 retail cvars that
  the engine publishes to clients, and the Quake Live cvar flag bits.
- `g_main.c`: retail first/second place configstrings.
- `g_spawn.c`: the retail spawn filters replace Quake III's.

## Changes made in milestone 3 (movement)

- `bg_pmove.c`, `bg_slidemove.c`, `bg_local.h` are rewritten to the retail
  qagame, function by function. Expressions are evaluated in double, in the
  retail order, using retail forms of the math helpers (`PM_Dot3`,
  `PM_VectorNormalize`, `PM_AngleVectors`, `PM_CrossProduct`). The
  rewrite adds:
  - the per-frame VQL/PQL movement profile;
  - QL jumps, air control, crouch slide, ladders, flight, the grapple,
    weapons, footsteps and landings;
  - `pmove_t` fields `noFootsteps`, `xyspeed`, `stepTime` and
    `grappleSlow`.
- `bg_pmove_settings.c`, `bg_public.h`: the retail filler's clamps and
  boolean storage; a float `pmove_velocity_gh`; per-weapon reload times from
  `weapon_reload_*`.
- `g_main.c`: the server fills movement and reload settings after the level
  reset, which had zeroed them.
- `g_client.c`: the spawn-time `pm_flags` options. `g_ql_cvars.h`:
  `pmove_AirControl`, `pmove_CrouchSlide`, `pmove_DoubleJump`.
- `g_active.c`, `g_cvar.h`, `bg_public.h`: `pmove_fixed` and `pmove_msec`
  are removed, since retail movement has no fixed-msec mode.
- `surfaceflags.h`: the QL snow and wood footstep surfaces.

To see the exact drift, diff a checkout of the commit above against `code/`.

## Changes made in milestone 4 (combat, items, spawning; in progress)

Retail behaviour replaces these baseq3a behaviours (evidence in the code
comments and docs/PLAN.md):

- `SnapVectorTowards` is Quake III's original form again (retail
  0x1006cc20), which truncates negative coordinates toward zero; baseq3a had
  fixed that.
- `G_RadiusDamage` no longer reorders the entity list to damage the attacker
  first; retail uses the engine's order.
- Hitscan weapons trace from the one retail muzzle point instead of
  baseq3a's eye-level origin; Quake III's 14-unit muzzle is gone.
- Lightning gun damage under `MISSIONPACK` (baseq3a never called `G_Damage`
  there), the shotgun under `MISSIONPACK`, and the HMG fire case.
- Joining: `g_autoJoin` is gone; everyone starts as a spectator as in
  retail (`g_teamAutoJoin` places players in team games). The "entered the
  game" print is gone, and team-change broadcasts follow retail's wording and
  five-second quiet period.
- `gamename`, `gamedate`, and the serverinfo flags on `g_redteam`,
  `g_blueteam`, `g_maxGameClients`, `g_unlagged`, `g_enableBreath` and
  `g_synchronousClients` are gone; retail publishes none of them.
- `ClearRegisteredItems` no longer registers the machinegun and gauntlet;
  spawning does, after `CS_ITEMS` is written, as in retail.
- `bg_lib.c`'s `rand()` is the Microsoft C runtime's generator on every
  platform (baseq3a used a 69069 generator), the generator the retail
  modules draw from.
- `trigger_push` aims every frame through Quake Live's per-frame think
  (`frameThink`), and the push velocity is in whole units.
- Items: only spawned items are registered; `g_spawnItem*`, `g_ammoPack`
  and `g_ammoPackHack` filter them; they rest one unit up on whole units;
  baseq3a's `time2` pickup-prediction value is gone; powerups and keys wait
  for the delays drawn at worldspawn.
- Spawn points come from the retail picker; baseq3a's furthest-spot
  selection and the `FL_NO_BOTS`/`FL_NO_HUMANS` retries are gone.
- Death follows retail `player_die`: no `LookAtKiller`/`STAT_DEAD_YAW` (the
  view turns to face the killer, and the stat slot is retail's battle suit
  frag count), the death event carries no killer, a suicide always gibs and
  waits `g_allowKill` longer, and the respawn delay is
  `g_respawn_delay_min` instead of 1700 ms. baseq3a's `g_blood` and nodrop
  gib exceptions are gone, `r.maxs[2] = -8` is back (baseq3a had removed it
  for the shotgun gib bug; retail sets it), and a gibbed player's invisible
  body enters the body queue again.
- Dropped items keep no `s.time` scale-down stamp, flags drop as tall as
  placed flags, and `TossClientItems` drops powerups (in every mode, under
  `g_dropPowerups`) before the weapon. `TossClientPersistantPowerups` is no
  longer called on death.
- The FFA scoreboard is retail's `scores_ffa`, and `smscores` replaces an
  oversized one; the other modes still send Quake III's `scores` until their
  retail grammars land.
- Rename notifications use a single server broadcast, including spectator
  recipients, as retail qagamex86 0x1003a7e1 does. The connected/name-change
  guards and message payload remain unchanged. Bullet flesh impacts leave
  `clientNum` at zero (0x1006cecf-0x1006cefe); `eventParm` identifies the
  victim and `otherEntityNum` identifies the shooter for both flesh and
  wall impacts. MG, HMG and chaingun use this shared path.
- Server player names now use `g_names.c`, independently normalized from
  retail 0x1003a0b0-0x1003a212. Nonwhite names receive a trailing `^7`;
  explicit white needs no extra reset and black codes do not change the
  current color. Normalization precedes the 39-byte stored-name copy
  (0x1003a771), including retail's clipping at a color-token boundary.
  Empty/color-only input retains the `UnnamedPlayer` fallback. Control
  characters remain excluded from protocol text. Other `BG_CleanName`
  callers retain their existing behavior.
- Items follow retail `Touch_Item` and the `Pickup_*` functions: respawn
  delays are seconds from `g_weaponRespawn`/`g_ammoRespawn`/
  `g_powerupRespawn` and the retail constants (baseq3a's `SpawnTime` is
  gone with its last user, the warmup-end respawn); a weapon gives its full ammo (Quake III's "only add a
  single shot" rule is gone); the Team Arena runes no longer change health
  or armor on pickup; powerup pickups are never predicted.
- `TeleportPlayer` throws the player out at a fixed 400 units a second
  (baseq3a scaled `g_speed`), sets no `clientNum` on its events, frees a
  grapple, and skips the kill box under `pmove_noPlayerClip`.
- A spawn that finds no spot of its mode no longer stops the server with
  `G_Error`; it is deferred as in retail. Harvester's items are found by
  QL's "Red Skull"/"Blue Skull" names. Respawning follows retail
  (`g_respawn_delay_max`, not `g_forcerespawn`), the team overlay is
  retail's list of teammates, and Team Arena's team-leader announcement is
  gone.

## Changes made in milestone 5 (match flow; in progress)

- The match starts by `map_restart 0` with `g_restarted` set, as in retail;
  baseq3a's in-place `G_WarmupEnd` reset is gone.
- Exit rules run once a frame from `G_RunFrame`; baseq3a's
  `CheckExitRulesLater` after every command is gone. The intermission
  follows 200 msec after the exit instead of a second, `LogExit` no longer
  sets `CS_INTERMISSION` (BeginIntermission does), and the intermission
  lasts until retail's map vote ends rather than until players are ready.
- `ExitLevel` runs the retail map vote's result; baseq3a's map rotation
  file (`g_rotation.c`'s `ParseMapRotation`) is no longer called from it.
- At the intermission and the 200 msec before it, commands other than
  chat, `score`, `readyup` and `vote` are ignored instead of being chatted.
- SetTeam starts a spectator on client 0 (baseq3a used the caller's own
  number), and ClientBegin no longer keeps the spawn count or tracks
  `inGame`.
- The map vote configstrings put each new key first, like the retail
  qagame's `Info_SetValueForKey` (FnQL-game's shared one appends).
- `target_location` names stop at 63, as in retail.
- `AddTeamScore` takes no origin and follows retail's calls; the Team
  Arena "team scored" call in other modes is gone.
- Items first think two server frames after spawning instead of 200 msec.
- Capture the flag's touch rules, prints, scores and sounds are retail's
  (one flag CTF still baseq3a's).
- A `map_restart` reconnect no longer repeats the team announcement, and
  training servers greet no one.
- Manual `dropweapon`, `dropflag`, `droppowerup` and `droprune` commands now
  share a bounded placement/policy implementation in `g_drop.c`, independently
  written from retail qagamex86 0x10044910-0x10045630 and 0x100513b0.
  Weapon drops transfer ammo and publish `EV_DROP_WEAPON`; flags pass or
  throw and return after ten seconds. Powerups can decay on the ground and
  retain their remaining duration through a timeout. Death-drop behavior
  remains separate. Rune release translates PW_* slots to PP_* item tags;
  rune lifecycle and effects now have their own G3 coverage below.
- Dropped pickups no longer emit the map-pickup announcement (the guard
  at retail 0x1004dcbc covers the whole announcement). Dynamically spawned
  timed items now notify spectators before their first think, matching
  G_SpawnItem's call at 0x10050ba1. Both initial and pickup notifications
  carry a zero groundEntityNum marker, as retail's callers specify.
- Optional `g_flagPhysics` attaches a per-frame callback to manual flag
  drops at creation, following retail 0x10050f80 and 0x100515f9. It handles
  jump pads, teleporters and capture zones; death drops keep their own path.
  `trigger_capturezone` now resolves the opposing carrier (or thrown flag's
  owner) and uses the zone's `count` for the team score (0x1006bda0).
- Portal cameras transmit raw roll in `angles2` and surfaces copy it, as
  retail 0x1005a77f and 0x1005a919 do; Quake III's byte in `clientNum` is gone.
  Intermission fallback uses the retail spawn picker and its nine-unit offset
  (0x10054f4b), preserving the legacy picker for its other callers.
- Sine trajectories retain precision through the angle multiplication
  (0x1002d297 / 0x1002d42b), avoiding early float rounding that moved items
  differently on bobbing platforms. Both position and velocity have a
  direct retail comparison in the existing x86 lockstep harness.
- Rune initialization and relocation are independently implemented in
  `g_runes.c`: four registered items, reserved eligible spawn points,
  alternating team starts, and random relocation after a manual drop
  (ten seconds), death drop (thirty seconds), or entry into a no-drop
  volume. Invalid spawn layouts warn and stop instead of looping.
- Scout uses a 1.25 speed multiplier, with Haste taking precedence, and
  drains armor toward 50 every 500 ms outside Clan Arena. Guard no longer
  regenerates health or doubles the handicap on a userinfo update. Armor
  Regen restores ten armor per second, observes tiered armor, and never
  replenishes ammo. Rune timers pause during a timeout.
- Rune combat tests also corrected pain-event health bands and damage
  sequencing (0x100338dd-0x10033923), plus knockback normalization precision
  (0x10048d9d). Damage/Quad precedence, Guard absorption, Armor Regen
  absorption and Scout self-damage immunity retain their retail behavior.
- Disconnects use the regular powerup-drop policy for runes, as retail
  0x1003c9f7 does. The inherited Team Arena pickup-relink path is removed;
  it bypassed `g_dropPowerups 0` and could expose a freed entity.
- Factory lookup now calls the host's retail import 206, including custom
  definitions, without a module-local factory catalog. The map-vote contract
  is checked against retail qagamex86 0x100428bf-0x10042a1b and the strict
  `factory-contract` G3 suite; the engine remains responsible for applying
  and restoring factory settings.
- Factory gameplay replaces inherited map-item and spawn-weapon behavior:
  Instagib keeps flag objectives and its rail/gauntlet kit, while CA/RR,
  Domination and ordinary Race map pickups are suppressed. Race's explicit
  gametype inclusion and the `give` cheat retain retail's force-spawn bypass
  (0x100507e0). Handicap now scales factory starting health (0x1003a878).
- `g_quadhog.c` independently implements the world-quad lifecycle and carrier
  locator from retail 0x1004d290-0x1004d500, 0x10034725 and 0x1004f21a.
  Spawn selection preserves retail's random stream; missing spawn points
  terminate safely. Idle deadlines and locator accumulation use bounded
  arithmetic. The native objective query identifies Quad Hog's world item.
- Factory rule groups publish retail's HUD settings mask at initialization
  and update it with `g_customSettings` after live rule changes
  (0x10053400-0x10054915). The `factory-gameplay` suite compares the resulting
  gameplay and network state against the user's retail x86 module.

- Public voting is independently implemented in `g_vote.c` from retail
  0x100423a0/0x10043920/0x10044270/0x100588f0/0x10058ab0/0x100591b0.
  Q3's multi-command grammar, fixed three-call limit, EF_VOTED bookkeeping
  and non-retail vote options are replaced by QL policy, private ballot
  states, moderator overrides and the retail action allowlist. Map/factory
  queries remain at the host boundary. The end-of-match map vote retains
  its existing owner. Inputs that would truncate a token or overflow an
  integer are rejected. Execution strings now have retail's 1,024-byte
  storage rather than the inherited 256-byte cvar buffer.
- A disconnect's teleport-out event keeps its default zero clientNum,
  matching retail 0x1003c9ef; the disconnect-voting differential covers the
  correction, including disconnects after a passed clientkick.
- Bot movement no longer scales the largest projected component to full
  speed. The independently rewritten projection follows retail qagamex86
  0x10021d0a-0x10021d76: float storage for scaled input speed, wide dot
  products before command-byte truncation, and integer-absolute vertical
  projection. The existing strict `ffa_bot_restart` G3 gate now matches
  all 400 frames, removing its previously documented 803 field-frames
  of drift without changing the bot decision logic or player movement.

- Server command completion uses independently written `g_admin.c` and
  `g_access.c` for the shared retail privilege table and account policy.
  `g_commandextras.c` adds bot chat and spectator selectors; `g_race.c`
  implements bounded checkpoint editing. Mute/privilege session persistence,
  SteamID ban admission, team locks, shuffle suppression and first-human
  leader selection connect the commands to the existing game lifecycle.
- Chat packets now carry the retail two-digit sender prefix and location
  color reset. Legacy voice commands retain dispatch/logging/bookkeeping but
  emit no Q3 voice packets, matching both retail HLIL and runtime probes.
  Unknown server-console text is no longer broadcast as player chat. The
  full inventory, evidence and regression gates are in [Server command compatibility](SERVER_COMMANDS.md).

- Race now maintains map-local personal best times and checkpoint splits,
  including retail timing events, spectator progress, ascending score order,
  tied-time adjustment and reset versus respawn semantics. Checkpoint chains
  and editor storage are bounded. Evidence: 0x100634f0, 0x10063580,
  0x1003c3fe, 0x10041238, 0x1002cc40; production unit tests and strict G3
  editor/record fixtures exercise the implementation.
- Red Rover shuffle respawns unchanged-team players and uses ClientSpawn for
  changed teams, preserving match records (0x1004066c/0x1006a47f). Equal-team
  joins consume retail's RNG draw and join blue (0x1003a07a/0x1003a099), and the mode has its
  nineteen-field scores_rr serializer (0x1003f260).

- `g_rounds.c` independently implements Clan Arena, Freeze Tag and Red
  Rover round controllers, including Infected. Private qagame state drives
  the retail round/countdown configstrings, result events, scoring and
  awards without changing the engine-shared ABI. Evidence and strict
  `round-modes` scenarios are listed under M6 in [the plan](PLAN.md).
- Round spawns now distinguish warmup, live players, frozen players and
  eliminated team spectators. Freeze Tag owns its collision shells,
  helper/automatic thaw, environmental respawn and native thaw-tick filter.
  RR conversion preserves round scores across respawns, sorts tied match
  scores by round score and publishes independently sorted round winners.
- Retail probes exposed shared corrections: kit weapons register for
  warmup ammo, CA/RR pickups use ordinary ammo caps, dmflags suppresses
  footsteps, and enemy timed-powerup carrier deaths announce the powerup.
  None of these additions copies implementation text from the reference.

- Attack & Defend extends the private round controller with alternating
  turns, role-aware flag scoring, round awards and rolling score history.
  Domination now owns capture points, contention, periodic scoring, defense
  and owned-point respawns in `g_domination.c`. Factory regeneration is
  connected to damage and spawn lifecycle hooks. Harvester skull counts,
  tosses, pickup events and captures match the six-bit retail contract;
  Overload retains its verified baseline controller with arithmetic and
  non-player-attacker guards. No engine-shared layouts change. Retail HLIL
  evidence and the 30 strict `objective-modes` scenarios are documented in
  [the plan](PLAN.md); map fixtures are independently authored.
- Stock-map rotation parity: door travel distance retains double arithmetic
  until after lip subtraction (retail 0x1005fbe5-0x1005fbf5), avoiding an early
  rounding error on Brimstone Abbey's 360-degree doors. `target_push` no longer
  registers the Quake III default wind sound; retail only registers the
  flagged bounce sound (0x1006b900-0x1006b912). Extended objective limit gates
  cover the subsequent stock map; `stock-maps` also checks direct empty-server
  startup and both push-sound cases. No external implementation was copied.

- Spawn registration retains authored origins, matching retail 0x10055000.
  The inherited floor-lowering pass was removed because it displaced the
  missing-intermission spectator fallback on Devilish (0x10054f4b). The
  stock-map gates now compare two spectators as well as map entities.

- Damaged-health drops are independently implemented from retail 0x1004f8c0:
  health actually lost becomes 25/5 pickups, with the retail toss and random
  consumption. Dynamic precaching and ten-second ordinary-drop expiry follow
  0x1004ffd0 and 0x1004f6ec. Missile updates run in entity order (0x100597d4)
  with retail's current-position collision (legacy rewind remains opt-in);
  this permits health items
  spawned by impacts to run in the same frame. No QLSRP implementation was
  incorporated.
- Grapple lifecycle, player pull/friction, obstruction reset and damage ticks
  follow retail 0x1005bf3d, 0x1006e330, 0x1006e400 and 0x10035190. Projectile
  collision uses the selected box/capsule trace path (0x1005c991). Lightning
  discharge uses a separate wet-target radius query and its retail ammo/effect
  contract (0x1004a7a0, 0x1006eb1d); LG pickups replenish the infinite-ammo
  sentinel after a discharge (0x1004dac1). All are independent implementations.
- Sessions now write the retail thirteen-field order (0x10065880), preserve
  the reserved tail, and read older FnQL-game records with explicit bounds.
  ClientThink persists the replicated primary weapon as the loadout choice
  before pmove (0x10034e20). Team-kill complaint prompts, vote responses,
  privilege exemptions and temporary bans are independently implemented in
  `g_complaint.c` from 0x1004756e/0x10043ab0/0x1003c943. The unreachable
  nonfatal-damage branch (0x10049363..6f) remains inactive, as verified by
  retail disassembly and the strict dedicated-server scenario.
- Rocket penetration is independently implemented from 0x1005c395 and
  0x1004a3e0: reverse solid probes, event 86, reduced-radius visibility and
  far-side knockback. Normal splash retains the original path. Warmup's
  inherited forced/read-only cvar override is removed to match the retail
  registration at 0x1008e0b8; countdown joins preserve the CA/FT firing lock.

- One Flag CTF now uses retail flag status values, neutral carrier snapshot
  visibility and team pings (0x10069eaf/0x10052dc0/0x10034777). Death-drop
  pickup bonuses follow 0x10069f51. Seven strict pairs cover captures, drops,
  expiry, disconnect, pause, restart and capturelimit. The latter also
  verifies that map_restart retains CS_INTERMISSION, as retail G_InitGame does.
- `g_laghax.c` independently implements retail's bounded hitscan rewind
  (0x10051660..0x10051add), including newer-sample stance bounds. FnQL's
  harness supplies delayed usercmds; the prior DLL differs by 1,744 fields/
  frames in the new moving-target fixture, while the rewrite has zero.
  Legacy full/frame compensation remains selectable. Moving trains now
  collide with hooks and use retail's crush handler (0x1005e395/0x10060a56).
- Live infinite-ammo changes update normal connected players immediately
  (0x100520e0/0x10059cb0). Jump-pad contact consumes the double-jump allowance,
  emits event parm zero and accepts frozen players (0x1002d540); a player
  with a hook ignores trigger_push (0x1006b5d0). The ordinary-movement
  stock-map fixture reproduces the former jump-pad mismatch and now passes.

- Rail headshot classification is supplied by FnQL's point-trace path from
  retail engine evidence (0x4c6bd0/0x4c6d6b/0x4c6da0), independently of the
  preserved movement-capsule profile. `ffa_headshot` compares standing and
  ducked bonuses, ordinary torso damage, box traces and restart. Retail
  qagame's bonus/death-type branch is exercised with actual 210-point hits.
- Duplicate player names use retail's numeric-color/case comparison and
  bounded prefix selection (0x1003a350/0x1003a62a), followed by the exact
  bad-userinfo predicate (0x1003aa03). The strict userinfo pair covers both
  enforcement settings, valid colors, collisions, truncation and restart.

- Native UI: replaced the unused vmMain/syscall entrypoint with a typed retail
  97-import/14-export boundary. The old `code/q3_ui/` tree is removed; native
  menus, settings, bindings, voting, player actions and demos
  live only in `code/ui/`. QL widgets, numeric crosshairs, model/skin pairs,
  native text and bounded menu parsing are independent adaptations of the
  baseline architecture. [UI.md](UI.md) records observations and gates.
