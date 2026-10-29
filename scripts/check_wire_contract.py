#!/usr/bin/env python3
"""Gate G2: FnQL-game's shared tables must match the retail wire contract.

A Quake Live client interprets numbers it receives from the server through
its own copy of these tables: an entity's modelindex is a bg_itemlist row,
an obituary parm is a means of death, a powerup bit is a powerup_t, and a
configstring index selects the meaning of its payload.  Any drift shows up on
retail clients as wrong models, wrong sounds or wrong HUD state.

This gate builds a probe against FnQL-game's own bg_misc.c and headers and
compares what it prints with:

  docs/contract/retail_tables.json  tables extracted from the retail binaries
                                    (scripts/extract_retail_contract.py)
  docs/contract/retail_enums.json   curated enum values with retail evidence

It also checks the spawn table (code/game/g_spawn.c) and the bot item
indices (code/game/inv.h) against the extracted retail ones.
"""

from __future__ import annotations

import argparse
import json
import re
import shlex
import subprocess
import sys
import tempfile
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
GAME = ROOT / "code" / "game"
TABLES = ROOT / "docs" / "contract" / "retail_tables.json"
ENUMS = ROOT / "docs" / "contract" / "retail_enums.json"

PROBE_STUBS = r"""
#include "q_shared.h"
#include "bg_public.h"
#include <stdio.h>
#include <stdlib.h>
void QDECL Com_Error( int level, const char *fmt, ... ) { (void)level; fprintf( stderr, "%s\n", fmt ); exit( 2 ); }
void QDECL Com_Printf( const char *fmt, ... ) { (void)fmt; }
void trap_Cvar_VariableStringBuffer( const char *name, char *buffer, int size ) { (void)name; if ( size > 0 ) buffer[0] = 0; }
"""


def c_string(value: str | None) -> str:
    return "NULL" if value is None else '"' + value.replace("\\", "\\\\").replace('"', '\\"') + '"'


def probe_source(enums: dict) -> str:
    lines = [
        '#include "q_shared.h"',
        '#include "bg_public.h"',
        "#include <stdio.h>",
        "static const char *modNames[] = {",
        "#define MOD_STRINGS",
        '#include "bg_mods.h"',
        "#undef MOD_STRINGS",
        "};",
        "static const char *eventNames[] = {",
        "#define EVENT_STRINGS",
        '#include "bg_events.h"',
        "#undef EVENT_STRINGS",
        "};",
        "static void str( const char *s ) { printf( \"\\t%s\", s ? s : \"(null)\" ); }",
        "int main( void ) {",
        "\tint i, m;",
        '\tprintf( "numItems %d\\n", bg_numItems );',
        "\tfor ( i = 0; i <= bg_numItems; i++ ) {",
        "\t\tconst gitem_t *it = &bg_itemlist[i];",
        '\t\tprintf( "item %d", i );',
        "\t\tstr( it->classname ); str( it->pickup_sound );",
        "\t\tfor ( m = 0; m < MAX_ITEM_MODELS; m++ ) str( it->world_model[m] );",
        "\t\tstr( it->icon ); str( it->pickup_name );",
        '\t\tprintf( "\\t%d\\t%d\\t%d\\t%d\\t%d\\t%d\\n", it->quantity, (int)it->giType, it->giTag,'
        " it->timer, it->retail[0], it->retail[1] );",
        "\t}",
        '\tfor ( i = 0; i < (int)ARRAY_LEN( modNames ); i++ ) printf( "mod %d\\t%s\\n", i, modNames[i] );',
        '\tfor ( i = 0; i < (int)ARRAY_LEN( eventNames ); i++ ) printf( "event %d\\t%s\\n", i, eventNames[i] );',
        '\tprintf( "string GAME_VERSION\\t%s\\n", GAME_VERSION );',
    ]
    for group in enums["groups"].values():
        for name in group["values"]:
            lines.append(f'\tprintf( "enum {name}\\t%d\\n", (int)({name}) );')
    lines += ["\treturn 0;", "}", ""]
    return "\n".join(lines)


def run_probe(cc: list[str], enums: dict) -> list[str]:
    with tempfile.TemporaryDirectory(prefix="fnql-game-wire-") as temporary:
        work = Path(temporary)
        (work / "probe.c").write_text(probe_source(enums), encoding="utf-8")
        (work / "stubs.c").write_text(PROBE_STUBS, encoding="utf-8")
        sources = [str(work / "probe.c"), str(work / "stubs.c"),
                   str(GAME / "bg_misc.c"), str(GAME / "bg_lib.c"), str(GAME / "q_shared.c"),
                   str(GAME / "q_math.c")]
        exe = work / "probe.exe"
        msvc = Path(cc[0]).stem.lower() in ("cl", "clang-cl")
        if msvc:
            command = cc + ["/nologo", "/TC", "/W0", "/DWIN32", "/D_CRT_SECURE_NO_WARNINGS",
                            "/DQAGAME", "/DMISSIONPACK", f"/I{GAME}", *sources, f"/Fe:{exe}", f"/Fo:{work}\\"]
        else:
            command = cc + ["-w", "-DQAGAME", "-DMISSIONPACK", f"-I{GAME}", *sources, "-o", str(exe), "-lm"]
        built = subprocess.run(command, cwd=work, capture_output=True, text=True, errors="replace")
        if built.returncode != 0:
            print(f"wire-contract probe failed to build:\n{built.stdout}{built.stderr}", file=sys.stderr)
            raise SystemExit(1)
        ran = subprocess.run([str(exe)], capture_output=True, text=True, check=True)
        return ran.stdout.splitlines()


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__.splitlines()[0])
    parser.add_argument("--cc", default="cl", help="C compiler command for a 32-bit x86 target")
    args = parser.parse_args()

    tables = json.loads(TABLES.read_text(encoding="utf-8"))
    enums = json.loads(ENUMS.read_text(encoding="utf-8"))
    out = run_probe(shlex.split(args.cc, posix=False), enums)
    failures: list[str] = []

    # bg_itemlist, row by row
    rows = {}
    for line in out:
        if line.startswith("item "):
            fields = line.split("\t")
            rows[int(fields[0].split()[1])] = fields[1:]
    num_items = int(next(l for l in out if l.startswith("numItems")).split()[1])
    if num_items != len(tables["items"]):
        failures.append(f"bg_numItems {num_items} != retail {len(tables['items'])}")
    for item in tables["items"]:
        expected = [item["classname"], item["pickupSound"], *item["models"], item["icon"], item["pickupName"]]
        expected = ["(null)" if value is None else value for value in expected]
        expected += [str(v) for v in (item["quantity"], item["giType"], item["giTag"], *item["trailing"])]
        if item["index"] == 0:
            expected = ["(null)"] * 12 + ["0"] * 6
        got = rows.get(item["index"])
        if got != expected:
            failures.append(f"bg_itemlist[{item['index']}] {item['classname']}: {got} != {expected}")

    # name tables
    mods = [line.split("\t")[1] for line in out if line.startswith("mod ")]
    if mods != tables["meansOfDeath"]:
        failures.append(f"means of death differ: {mods} != {tables['meansOfDeath']}")
    events = {line.split("\t")[1]: int(line.split()[1]) for line in out if line.startswith("event ")}
    values = {line.split("\t")[0][5:]: int(line.split("\t")[1]) for line in out if line.startswith("enum ")}
    for index, name in enumerate(tables["pmTypes"]):
        if values.get(name) != index:
            failures.append(f"{name} = {values.get(name)} but retail pm type {index}")
    if values.get("GT_MAX_GAME_TYPE") != len(tables["gametypes"]):
        failures.append(f"GT_MAX_GAME_TYPE {values.get('GT_MAX_GAME_TYPE')} != {len(tables['gametypes'])} retail gametypes")

    # curated enums
    for group_name, group in enums["groups"].items():
        for name, expected in group["values"].items():
            if values.get(name) != expected:
                failures.append(f"{group_name}: {name} = {values.get(name)}, retail {expected}")
            if name.startswith("EV_") and name != "EV_MAX" and events.get(name) != expected:
                failures.append(f"event name table: {name} at {events.get(name)}, retail {expected}")
    for name, spec in enums["strings"].items():
        got = next((l.split("\t")[1] for l in out if l.startswith(f"string {name}\t")), None)
        if got != spec["value"]:
            failures.append(f"{name} = {got!r}, retail {spec['value']!r}")

    # spawn table and bot indices, from source
    spawns = re.findall(r'\{\s*"(\w+)",\s*SP_\w+\s*\}', (GAME / "g_spawn.c").read_text(encoding="latin-1"))
    if spawns != tables["spawnTable"]:
        failures.append(f"spawn table differs:\n    ours   {spawns}\n    retail {tables['spawnTable']}")
    inv = dict((n, int(v)) for n, v in re.findall(r"#define\s+(MODELINDEX_\w+)\s+(\d+)",
                                                  (GAME / "inv.h").read_text(encoding="latin-1")))
    for name, expected in tables["botItemIndices"].items():
        if inv.get(name) != expected:
            failures.append(f"inv.h {name} = {inv.get(name)}, retail botfiles/inv.h {expected}")

    # configstring payload grammars, from the format strings in the source
    def c_formats(path: Path, prefix: str) -> list[str]:
        source = path.read_text(encoding="latin-1")
        found = []
        for call in re.finditer(r'va\(\s*((?:"(?:[^"\\]|\\.)*"\s*)+),', source):
            literal = "".join(re.findall(r'"((?:[^"\\]|\\.)*)"', call.group(1)))
            literal = literal.encode("latin-1").decode("unicode_escape")
            if literal.startswith(prefix):
                found.append(literal)
        return found

    def format_keys(fmt: str) -> list[str]:
        parts = fmt.split("\\")
        first = 1 if fmt.startswith("\\") else 0
        return [parts[k] for k in range(first, len(parts), 2)]

    player_formats = c_formats(GAME / "g_client.c", "n\\%s\\t\\%i\\model\\")
    ours = {"bot" if "\\skill\\" in f else "human": format_keys(f) for f in player_formats}
    for kind, keys in tables["playerKeys"].items():
        if ours.get(kind) != keys:
            failures.append(f"CS_PLAYERS {kind} keys {ours.get(kind)} != retail {keys}")
    warmup = set(re.findall(r'CS_WARMUP,\s*va\(\s*"((?:[^"\\]|\\.)*)"',
                            (GAME / "g_main.c").read_text(encoding="latin-1")
                            + (GAME / "g_spawn.c").read_text(encoding="latin-1")))
    warmup = {w.encode("latin-1").decode("unicode_escape") for w in warmup}
    if warmup != {tables["warmupFormat"]}:
        failures.append(f"CS_WARMUP formats {sorted(warmup)} != retail {tables['warmupFormat']!r}")

    if failures:
        print(f"wire contract FAILED ({len(failures)}):")
        for failure in failures:
            print(f"  - {failure}")
        return 1
    checks = len(tables["items"]) + len(mods) + sum(len(g["values"]) for g in enums["groups"].values()) \
        + len(spawns) + len(tables["botItemIndices"])         + sum(len(keys) for keys in tables["playerKeys"].values()) + 1
    print(f"wire contract OK: {checks} retail values match")
    return 0


if __name__ == "__main__":
    sys.exit(main())
