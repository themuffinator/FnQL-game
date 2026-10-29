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

To see the exact drift, diff a checkout of the commit above against `code/`.
