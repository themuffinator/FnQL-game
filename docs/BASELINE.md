# Baseline Import

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
