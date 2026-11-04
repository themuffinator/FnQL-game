# Server command compatibility

The command audit covers FnQL-game's `ClientCommand` and `ConsoleCommand`,
including the shared privilege table. Public `vote`, `callvote` and `cv`
belong to the separate voting implementation. Engine-owned commands such
as `map`, `status`, Steam authentication and RCON transport remain in FnQL.

## Implemented command surfaces

| Surface | Commands | Implementation / evidence |
| --- | --- | --- |
| Public match controls | `?`, `players`, `timeout`, `timein` | `g_admin.c`; retail 0x10080750 table, 0x10062e20 dispatch |
| Moderator match controls | `allready`, `pause`, `unpause`, `lock`, `unlock`, `put`, `abort`, `addscore`, `addteamscore`, `setmatchtime` | Shared player/console handlers; retail 0x10061550–0x10062330, 0x10062ce0 |
| Moderation / access | `mute`, `unmute`, `tempban`, `ban`, `listaccess`, `unban`, `opsay`, `addadmin`, `addmod`, `demote`, `reload_access` | `g_admin.c`, `g_access.c`; retail 0x10062470–0x10062c60, 0x10032460–0x10032b5b |
| Chat | `say`, `say_team`, `tell`, `botSay` | `g_cmds.c`, `g_commandextras.c`; retail 0x100417a0–0x10041e3d |
| Legacy voice commands | `vsay`, `vsay_team`, `vtell`, `vosay`, `vosay_team`, `votell`, `vtaunt` | Retail 0x10041e40/0x10041e60 consumes these without emitting Q3 voice packets. Tell logging and taunt bookkeeping remain. |
| Spectating | `follow`, `follownext`, `followprev`, `spec`, `specresp` | Existing follow/item-timer paths plus retail 0x10045770 selector cycling (`fc`, `red`/`r`, `blue`/`b`, `pu`) |
| Player / game | `score`, `acc`, `pstats`, `readyup`, `ragequit`, `give`, `god`, `notarget`, `noclip`, `kill`, `teamtask`, `levelshot`, `team`, `where`, `gc`, `setviewpos`, `forfeit` | Existing handlers retained; `gc` rejects the first out-of-range order instead of indexing past its table. Retail dispatcher 0x10045dec. |
| Item drops | `dropflag`, `droppowerup`, `droprune`, `dropweapon` | Existing `g_drop.c` owner and drop gates retained |
| Race editor | `racepoint`, `raceinit` | `g_race.c`; retail 0x10063ec0, 0x10063e50; mode/cheat/alive checks, bounded marker creation, chaining, dump, clear and active-run reset |
| Operator commands | `entitylist`, `forceteam`, `game_memory`, `addbot`, `botlist`, `game_crash`, `forceshuffle`, `dumpvars`, dedicated `say`/`tell` | `g_svcmds.c`; retail 0x10066b90. Cleaned-name lookup, partial lookup for dumpvars, shared admin routing. |
| Retail diagnostic no-ops | `markstate`, `diffstate`, `dumpentities`, `printentitystates` | Accepted without invented output, matching retail 0x10066d88–0x10066e02 |

The `rcon` row has no game handler in retail and falls through; it does not
grant a client arbitrary console execution. Existing FnQL-game extensions
(`stats`, IP filters, `rotate`, `abort_podium`) remain available. Unknown
console commands no longer become unsolicited chat broadcasts.

## Access and state lifetime

`g_accessFile` defaults to `access.txt`. Each permanent entry is an engine
SteamID followed by `|ban`, `|mod` or `|admin`. Identity comes from the host's
existing SteamID import, never from client-supplied identity text. The
retail localhost exception is tier 3; normal players are tier 0, moderators
1 and administrators 2. Promotion/demotion sends `priv` to the affected
client. Ban enforcement occurs before a non-bot client is admitted.

The implementation uses a bounded, sorted array (16,384 entries) instead
of retail's tree. Input files are capped at 1 MiB. Decimal IDs reject
overflow, signs and trailing junk. Failed opens/allocations retain the
active policy. Permanent changes save on shutdown; temporary bans and
zero-privilege records are not serialized. Failed save opens are reported.
The retail FS import has no write-result return, so mid-write failures
cannot be diagnosed through this ABI.

Mute and privilege state survive module restarts through two appended
session fields. The existing ten-field FnQL-game prefix remains readable;
this is not a claim that the entire session format now equals retail's.
Team locks and editor ownership reset with the level. Shuffle uses the
project's existing deterministic CRT-compatible sort, suppresses team-change
obituaries/score/chat, and preserves retail's team-leader selection.

Malformed numeric targets and overflowing score/time adjustments fail
closed. `game_crash` uses the engine's fatal-error path when developer mode
is enabled instead of deliberately dereferencing null. The Race editor
bounds both live markers and names at 64 and reuses level-owned name storage.

## Validation and remaining mode dependencies

`game-admin` executes the production admin/access/spectator/bot-chat/editor
code against fake host imports. It checks privilege boundaries, bans,
access persistence and failed reload/save handling, malformed/overflowing
IDs, score/time bounds, locks, forced moves, pauses, abort, ready state,
spectator selection, bot-only chat and bounded checkpoint editing.

Five strict G3 scenarios have retail-twice determinism checks and full
retail-versus-FnQL comparisons: `tdm_admin` (1,250 frames), `tdm_access`
(70), `tdm_commands` (70), and `tdm_shuffle` (100), and `tdm_shuffle_live` (1,100, five players in a live match). They compare player and
entity states, configstrings and reliable commands with zero divergence.
The access scenario includes help filtering, mute visibility, tells,
promotion/demotion, list paging, temporary/permanent bans and unban.
The commands scenario covers selector aliases/case, powerup selection,
Race's wrong-mode refusal, team chat and all six legacy voice forms.

Race records and checkpoint scoring now have strict retail comparisons:
`race_editor` (40 frames) and `race_records` (280 frames, a hand-authored
three-marker course). They cover initial untimed scores, ordered starts,
split deltas, personal-best replacement, ties, slower finishes after death,
spectator `race_info`, `raceinit`, and record reset on team rejoin.
`game-race` also checks malformed target chains, repeated checkpoints, and
bounded editor/checkpoint storage using the production code.

Retail 0x100634f0 initializes each player's map-local best to `INT_MAX`;
0x10063580 handles timing and events. Lower times rank first. A finish tied
with an existing time advances by one millisecond. Personal records and
best splits survive respawns, while ClientBegin resets them. Despite its
historical name, `CS_RACE_RECORDS` contains the registered marker count,
not a leaderboard. This implementation does not invent disk or online
record persistence. Unrepresentable durations saturate instead of wrapping
into a new record. Event payloads follow the retail integer entity-state
offsets, and time formatting follows 0x1002cc40.

Red Rover shuffle (`rr_shuffle` and `rr_shuffle_odd`, 100 frames each)
respawns every playing client, including clients whose team stays unchanged
or who were dead, selects the starting weapon,
and preserves match records. Changed teams use ClientSpawn rather than
ClientBegin (0x1004066c/0x1006a47f). A shuffle retains the old spatial link
through KillBox, including retail's zero-damage self contact when selecting
the same spot. Equal team counts and scores consume retail's RNG draw but
join blue:
0x1003a099 compares for equality to an unreachable half-integer draw,
rather than testing its sign. The `scores_rr` packet follows 0x1003f260.
The additional `rr_shuffle_live` fixture covers five-player shuffles during
both the round countdown and active play, with locked-team admission bypass
and last-standing notifications. `rr_join` (40 frames) separately checks
two-player joins and shuffles. The six fixture pairs cover 2,160 frames per
provider with strict zero-divergence requirements. The odd-player fixture
uses seed 7.

These observations use the legitimate Windows x86 retail 1069 module hosted
by FnQL. They do not establish live Steam-client or Linux runtime parity.
The initially observed `ffa_bot_restart` divergence was fixed separately in
the concurrent bot-input work; the combined checkout passes that gate.
