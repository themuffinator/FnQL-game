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

| Server | Client | Expectation |
|---|---|---|
| retail engine + retail qagame | FnQL + retail cgame | FnQL today; unchanged |
| retail engine + retail qagame | FnQL + FnQL-game cgame | unpure servers only; retail profile (M7, M10) |
| FnQL + FnQL-game qagame | retail Quake Live | retail profile; the core requirement (M2–M6) |
| FnQL + FnQL-game qagame | FnQL + retail cgame | same as the retail client row |
| FnQL + FnQL-game qagame | FnQL + FnQL-game cgame | retail profile plus negotiated extensions (M10) |
| FnQL + retail qagame | FnQL + FnQL-game cgame | same as the retail server row |

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
   once the module's parity gates pass.
6. **Stay out of the way of purity.** Server modules are server-only content.
   Client modules are optional packages that a retail client never needs, and
   the engine never reports a retail checksum for them.
7. **No synthesized success.** Authorization, purity and capability answers
   reflect what actually ran.

## Module names

FnQL-game modules carry an `fnql_` prefix (`fnql_qagamex86.dll`,
`fnql_cgamex86.dll`, `fnql_uix86.dll`; Linux `fnql_*i386.so`). They never
shadow the retail modules, and the engine selects them by provider
(`vm_gameProvider fnql`) rather than by search order.
