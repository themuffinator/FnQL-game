# Compatibility Contract

This is the game half of the contract between FnQL-game and the FnQL engine.
The engine half is FnQL's `docs/fnql/GAME_MODULES.md`. Together they say how
FnQL-game modules can replace the retail Quake Live modules without any peer
noticing.

## Who talks to whom

The engine owns the wire: the protocol 91 field tables, delta compression,
netchan and the snapshot builder. A game module owns what the fields mean:
which number is which item, what a configstring says, what a server command
looks like, and where pmove puts a player. FnQL-game can therefore break
compatibility only through meaning, never through framing, and every
milestone gate checks meaning.

| Server | Client | Expectation | Gate |
|---|---|---|---|
| retail engine + retail qagame | FnQL + retail cgame | unchanged | `retail-server-fnql-client` (pure) |
| retail engine + retail qagame | FnQL + FnQL-game cgame | unpure only; a retail pure server gets the retail cgame | `retail-server-fnql-client-unpure` |
| retail engine + FnQL-game qagame | retail host, FnQL clients | the qagame is a drop-in | `retail-server-dropin-qagame`, `-unpure`, `retail-server-fnql-client-dropin` |
| FnQL + FnQL-game qagame | retail Quake Live | retail profile; the core requirement (M2–M6); never asked for the client package | `retail-client-join`, `retail-client-package`, `-unpure`, `retail-client-mode-*` |
| FnQL + FnQL-game qagame | retail Quake Live + FnQL-game cgame and ui | the client modules are drop-ins; scratch base path, install untouched; the handshake runs through the retail engine | `retail-client-cgame`, `retail-client-dropin`, `-unpure` |
| FnQL + FnQL-game qagame | FnQL + retail cgame | same as the retail client row | `fnql-client-pure-retail-client`, `-unpure-retail-client`, `interop*` |
| FnQL + FnQL-game qagame | FnQL + FnQL-game cgame | retail profile plus negotiated extensions; pure only with the client package | `fnql-client-pure-package`, `-pure-auto`, `-unpure-*`; refusals in `-pure-no-package`, `-pure-refused`, `-pure-loose` |
| FnQL + retail qagame | FnQL + FnQL-game cgame | as on a retail server, but a pure FnQL server may accept the package; no extensions | `fnql-client-pure-retail-qagame`, `-unpure-retail-qagame` |

The `fnql-client-*` rows are the `interop-fnql` suite. The `retail-*` rows
are the opt-in `retail-client` suite (`FNQL_ALLOW_RETAIL_CLIENT=1`, Steam
running). The `retail-server-fnql-client*` rows need an FnQL client that can
get a Steam ticket: the retail engine refuses one without ("No Steam auth
token."), and those rows then skip with that answer.

## Client modules on pure servers

A pure server checks the pak a client reports for its cgame. Retail clients
report the pak that holds `cgamex86.dll`. An FnQL client running
FnQL-game's cgame reports the FnQL-game **client package**,
`fnql_game_client.pk3`, which holds `fnql_cgame*` and `fnql_ui*` and nothing
else (`scripts/make_package.py` builds it deterministically). It may do so
only on a server that lists the package's checksum in `sv_fnqlClientPaks`:
an FnQL server publishes that key while it has the package loaded. On any
other pure server the client loads the retail cgame and ui. The **server
package**, `fnql_game_server.pk3`, holds the qagame. Neither package is ever
referenced to clients, so no retail client downloads or needs one. FnQL's
`docs/fnql/GAME_MODULES.md` has the engine rules.

## Extension channels

`code/game/bg_fnql_ext.h` holds the protocol both modules share:

1. The qagame publishes `fnql_game <version>` in serverinfo.
2. The cgame answers only that key: it sets the temporary userinfo cvar
   `fnql_cg` to its extension protocol (1) at init, sets it empty on any
   other server, and clears it on shutdown. A retail server therefore never
   receives it, not even in the next connect packet.
3. The qagame enables extensions for that client alone
   (`clientPersistant_t.fnqlExtensions`). Bots, free slots and clients
   without the key get nothing. Extension commands travel only on that
   client's own reliable command stream (`G_FnQLExtSend`, and
   `G_FnQLExtBroadcast` loops over negotiated clients), never to -1.
4. Extension 1, the handshake: when a negotiated client enters the game, or
   answers while already in it, the server sends
   `fnql_hello <protocol> <mask> <version>`. The cgame records the enabled
   set, prints `FnQL-game server <version>: extensions <mask>` and answers
   `fnql_ack <protocol> <mask>`. The server then logs the channel as
   confirmed. Each level and restart repeats it for the new cgame. The
   client commands are claimed only for negotiated clients; anyone else's
   `fnql_ack` takes the retail path, an unknown command.

Later extensions take the next mask bits and travel the same way. Each must
stay out of every retail peer's sight; `tests/game_fnql_ext.c` gates the
channel rules.

## Rules

1. **One ABI.** Every module uses the retail structured native ABI and only
   the retail import slots. The same binary is a drop-in for the retail
   engine. FnQL-only services are reached through the `//trap_GetValue`
   handshake, and only after the engine has answered it
   (`G_NativeHasEngineExtensions`).
2. **Shared memory layouts match the engine.** `scripts/check_engine_abi.py`
   enforces this (gate G0).
3. **Retail profile by default.** With no negotiation, the module's behavior
   on the wire matches retail: numbering, table order, configstring indices
   and payloads, server command grammar, prediction. Retail HLIL decides.
4. **Negotiate, never assume.** FnQL-game qagame announces itself with a
   `fnql_game` serverinfo key. FnQL-game cgame answers with a userinfo key.
   Extensions are enabled per client, only when both sides announced, and
   travel only on per-client channels. Retail peers never receive an unknown
   server command, an unknown configstring payload, or a field whose meaning
   changed.
5. **Identify honestly.** `FnQL_GetModuleInfo` states the product, slot,
   version and capabilities. `FNQL_MODULE_CAP_RETAIL_WIRE` is claimed only
   once the module's parity gates pass. Since M10 the qagame and cgame claim
   it and `FNQL_MODULE_CAP_EXTENSIONS`, and the ui claims it.
6. **Stay out of the way of purity.** Server modules are server-only content.
   Client modules are optional packages that a retail client never needs, and
   the engine never reports a retail checksum for them.
7. **No synthesized success.** Authorization, purity and capability answers
   reflect what actually ran.

## Module names

FnQL-game modules carry an `fnql_` prefix (`fnql_qagamex86.dll`,
`fnql_cgamex86.dll`, `fnql_uix86.dll`; Linux `fnql_*i386.so`). They never
shadow the retail modules, and the engine selects them by provider
(`vm_gameProvider`, `vm_cgameProvider`, `vm_uiProvider` set to `fnql` or
`auto`) rather than by search order.
