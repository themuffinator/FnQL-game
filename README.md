# FnQL-game

Clean-room game modules for [FnQL](https://github.com/themuffinator/FnQL)
(Fappin' Quake Live). FnQL-game builds replacements for the retail Quake Live
`qagame`, `cgame` and `ui` modules. They interoperate with retail Quake Live
in both directions: FnQL servers running FnQL-game accept retail clients, and
FnQL clients running FnQL-game join retail servers. Nothing on the wire
differs from retail unless both peers negotiated it.

- **Baseline:** [baseq3a](https://github.com/ec-/baseq3a), an improved
  Quake III 1.32 game code. See [`docs/BASELINE.md`](docs/BASELINE.md).
- **Quake Live behavior** is rewritten from retail evidence, with the QLSRP
  reconstruction as a behavioral reference. Nothing is ported line by line.
- **Retail assets** come from your legitimate Steam install of Quake Live.
  This repository ships none.

## Status

FnQL releases build, bundle and load these modules by default. Each
milestone of [`docs/PLAN.md`](docs/PLAN.md) closes on gates that compare
FnQL-game with the retail modules: the lockstep harness runs the retail
qagame and FnQL-game's side by side and requires zero divergence, and the
cgame and UI gates compare frames and menus with retail captures pixel by
pixel.

| Milestone | State | What it covers |
|---|---|---|
| M1 Foundation | complete | The retail native module ABI; boots retail maps in an FnQL dedicated server |
| M2 Wire contract | complete | Item table, enums, configstrings, published cvars and spawn table match the retail binaries |
| M3 Movement | complete | `Pmove` and weapon timing bit for bit with retail; the retail cgame predicts FnQL-game's server, VQL and PQL, with no error |
| M4 Combat and items | complete | Every weapon, damage and knockback tunables, loadouts, item timers and the QL spawn system, on every stock map |
| M5 Match flow | complete | FFA, Duel, TDM, CTF and One Flag CTF; warmup, ready-up, timeouts, overtime, votes, scoreboards, awards and factories |
| M6 Round-based and special modes | complete | Clan Arena, Freeze Tag, Red Rover, Attack & Defend, Race, Domination, Harvester and Overload, with their instagib, quad hog and infected variants |
| M7 cgame | exit gates pass | Zero prediction misses; gameplay frames and scoreboards match retail in FFA, TDM, CA and CTF. A few cvars remain open ([`docs/CGAME.md`](docs/CGAME.md)) |
| M8 ui | complete | The [native UI](docs/UI.md): every menu, tab page and scripted interaction, and the cvar and binding tables, match retail |
| M9 Bots | exit gates pass | Bots hear, hunt, aim, chat and play every mode as retail's do. Open: some modes part from retail after 20 to 40 seconds, and the training nodes |
| M10 Engine integration | complete | FnQL selects all three modules, a pure server offers the client package, and FnQL-game peers negotiate extensions retail peers never see |
| M11 Architectures | Linux range complete | x86_64 on Windows and Linux; aarch64, armv7l, ppc64le and ppc64 on Linux. macOS, the BSDs and Windows on ARM64 are not enabled |
| M12 Release | complete | FnQL's CI builds the pinned commit and ships the modules in its x86 and x86_64 packages |
| M13 Server extensions | planned | Default-off server extension interface, spectator evidence and cgame multiview for FnQL's dedicated-server plan |

The same modules also run as drop-ins in the retail engine. See
[server command compatibility](docs/SERVER_COMMANDS.md) for command-level
coverage.

## Using it

### With an FnQL release

Every FnQL release carries the modules in `baseq3/`: `fnql_qagamex86.dll`,
`fnql_cgamex86.dll` and `fnql_uix86.dll` on Windows, the `i386.so` set on
Linux, and the `x86_64` sets in the 64-bit packages. It also carries
`fnql_game_client.pk3`, the client package (below). Extract the package into
your Quake Live folder as FnQL's own README describes, and FnQL uses
FnQL-game with no further setup:

```bash
fnql.ded.exe +map campgrounds ffa
```

hosts FnQL-game's qagame, and `fnql.exe` runs its cgame and UI. Run `vminfo`
in the console to see which modules are active.

FnQL picks each module with a provider cvar: `vm_gameProvider` for the
server, `vm_cgameProvider` and `vm_uiProvider` for a player. Each takes one of:

| Value | Module |
|---|---|
| `auto` | FnQL-game's, if it declares retail wire parity, as released builds do; otherwise retail's |
| `fnql` | FnQL-game's, whatever it declares |
| `retail` | the retail module, exactly as retail Quake Live loads it |

`vm_gameProvider` defaults to `auto` everywhere. The cgame and UI providers
default to `auto` on Win32 x86 and to `fnql` on every other target, where no
retail client modules exist.

If an FnQL-game module is missing or invalid, FnQL falls back to the retail
module and says so in the console. To go back to retail on x86, set the
provider:

```bash
fnql.ded.exe +set vm_gameProvider retail +map campgrounds ffa
```

```bash
fnql.exe +set vm_cgameProvider retail +set vm_uiProvider retail
```

A configuration written by an FnQL release from before milestone 12 may still
hold `vm_gameProvider "retail"`, because those releases saved the old default.
Run `reset vm_gameProvider` once to take the new default.

### Pure servers and the client package

Loose modules serve unpure servers, local play and demos. On a pure server
the client keeps FnQL-game's cgame and UI only if the server has loaded the
same client package, `fnql_game_client.pk3`, and lists it in
`sv_fnqlClientPaks`. A pure FnQL server from a release does that on its own.
On any other pure server, retail ones included, the client uses the retail
cgame and UI, and reports what it ran. A server operator can stop offering
the package with `sv_fnqlClientModules 0`.

### Playing with retail Quake Live

- Retail clients join FnQL servers running FnQL-game and need nothing extra:
  no package, no download, no difference on the wire.
- FnQL clients join retail servers. Retail servers require a Steam ticket,
  so the FnQL client must be able to reach Steam. A pure retail server always
  gets the retail cgame and UI.
- FnQL-game peers negotiate extensions only with each other, never with a
  retail peer. See [`docs/COMPATIBILITY.md`](docs/COMPATIBILITY.md).
- The retail cgame and UI, the Steam provider and the WebUI exist only for
  32-bit x86. On every other target FnQL-game is the game (FnQL on Linux
  x86_64 can also host retail's `qagamex64.so`). The wire stays the same, so
  x86 and non-x86 peers play together.

### With your own build

Build as [`BUILD.md`](BUILD.md) describes. Then put the modules, or the
`fnql_game_client.pk3` the build makes beside them, in your FnQL home
`baseq3` directory. FnQL-game modules carry the `fnql_` prefix and the
engine's architecture suffix, so they never shadow a retail module; the
providers above select them. The build also makes `fnql_game_server.pk3`,
the qagame in a package. Servers never reference it to clients, so it is
optional.

FnQL releases do not package the Linux aarch64, armv7l, ppc64le and ppc64
targets yet. Their modules cross-build from x86_64 Linux with the committed
`meson/linux-<arch>.ini` cross files, for FnQL's dedicated server on those
targets.

## Documents

- [`docs/PLAN.md`](docs/PLAN.md): milestones, evidence rules and validation
  gates.
- [`docs/COMPATIBILITY.md`](docs/COMPATIBILITY.md): the interoperability
  contract with retail peers and the FnQL engine.
- [`docs/SERVER_COMMANDS.md`](docs/SERVER_COMMANDS.md): server command
  coverage against retail.
- [`docs/CGAME.md`](docs/CGAME.md): cgame parity work and what remains open.
- [`docs/UI.md`](docs/UI.md): the native UI and its validation.
- [`docs/BASELINE.md`](docs/BASELINE.md): what was imported from baseq3a, and
  what was left out.
- [`docs/LICENSING.md`](docs/LICENSING.md): the two licenses the code and
  the released modules are under, and one open question.
- [`BUILD.md`](BUILD.md): building and running the gates.
- [`CREDITS.md`](CREDITS.md): lineage and acknowledgements.
