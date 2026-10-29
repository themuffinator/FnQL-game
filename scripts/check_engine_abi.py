#!/usr/bin/env python3
"""Compare the structures FnQL-game shares with the FnQL engine.

The engine and the game module exchange raw pointers to these structures, so
a single shifted field silently corrupts state (a vmCvar_t without the Quake
Live flags word turns every cvar string into garbage).  This gate compiles one
layout probe twice -- against FnQL-game's headers and against the engine's --
and requires identical sizes, field offsets, field sizes and constants.

Field lists are parsed from the engine headers, so a field the engine adds is
covered automatically and fails the game-side compile until it is mirrored.

Exit status: 0 on parity, 1 on mismatch, 77 (Meson "skip") when the FnQL
checkout is absent.
"""

from __future__ import annotations

import argparse
import re
import shlex
import subprocess
import sys
import tempfile
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]

# Structures the engine and qagame share by pointer, with the engine header
# that defines each one.
SHARED_STRUCTS = {
    "vmCvar_t": "code/qcommon/q_shared.h",
    "cplane_t": "code/qcommon/q_shared.h",
    "trace_t": "code/qcommon/q_shared.h",
    "trajectory_t": "code/qcommon/q_shared.h",
    "entityState_t": "code/qcommon/q_shared.h",
    "playerState_t": "code/qcommon/q_shared.h",
    "usercmd_t": "code/qcommon/q_shared.h",
    "qtime_t": "code/qcommon/q_shared.h",
    "entityShared_t": "code/game/g_public.h",
    "sharedEntity_t": "code/game/g_public.h",
    "fnqlModuleInfo_t": "code/qcommon/fnql_module.h",
}

SHARED_CONSTANTS = [
    "MAX_CLIENTS", "GENTITYNUM_BITS", "MAX_GENTITIES", "ENTITYNUM_NONE", "ENTITYNUM_WORLD",
    "MAX_CONFIGSTRINGS", "MAX_STATS", "MAX_PERSISTANT", "MAX_POWERUPS", "MAX_WEAPONS",
    "MAX_PS_EVENTS", "MAX_STRING_CHARS", "MAX_INFO_STRING", "MAX_QPATH", "MAX_CVAR_VALUE_STRING",
    "SVF_NOCLIENT", "SVF_CLIENTMASK", "SVF_BOT", "SVF_BROADCAST", "SVF_PORTAL",
    "SVF_USE_CURRENT_ORIGIN", "SVF_SINGLECLIENT", "SVF_NOSERVERINFO", "SVF_CAPSULE",
    "SVF_NOTSINGLECLIENT",
    # SVF_SELF_PORTAL2 is deliberately absent: it is an engine extension the
    # game discovers at runtime (GetValue "SVF_SELF_PORTAL2_Q3E"), because the
    # retail engine gives no meaning to that bit.
    "CONTENTS_SOLID", "CONTENTS_LAVA", "CONTENTS_SLIME", "CONTENTS_WATER", "CONTENTS_FOG",
    "CONTENTS_PLAYERCLIP", "CONTENTS_MONSTERCLIP", "CONTENTS_BODY", "CONTENTS_CORPSE",
    "CONTENTS_TRIGGER", "CONTENTS_NODROP",
    "CVAR_QL_PMOVE_SETTINGS", "CVAR_QL_WEAPON_RELOAD", "CVAR_QL_PLAYER_APPEARANCE",
    "CVAR_QL_ARMOR_TIERED", "CVAR_GAMERULE",
    "GAME_NATIVE_API_VERSION",
    "FNQL_MODULE_INFO_VERSION", "FNQL_MODULE_CAP_RETAIL_WIRE", "FNQL_MODULE_CAP_EXTENSIONS",
]

# trType_t is shared by value inside trajectory_t.
SHARED_ENUM_VALUES = [
    "TR_STATIONARY", "TR_INTERPOLATE", "TR_LINEAR", "TR_LINEAR_STOP", "TR_SINE", "TR_GRAVITY",
    "TR_QL_ACCEL",
]

GAME_PRELUDE = """\
#include "q_shared.h"
#include "g_public.h"
#include "g_native.h"
#include "fnql_module.h"
"""

ENGINE_PRELUDE = """\
#include "q_shared.h"
#include "qcommon.h"
#include "../game/g_public.h"
#include "fnql_module.h"
"""


def strip_comments(text: str) -> str:
    text = re.sub(r"/\*.*?\*/", " ", text, flags=re.S)
    return re.sub(r"//[^\n]*", " ", text)


def struct_fields(header: Path, type_name: str) -> list[str]:
    source = strip_comments(header.read_text(encoding="utf-8", errors="replace"))
    match = re.search(
        r"typedef\s+struct\s*\w*\s*\{(?P<body>[^{}]*)\}\s*" + re.escape(type_name) + r"\s*;",
        source,
    )
    if not match:
        raise SystemExit(f"cannot find struct {type_name} in {header}")
    fields: list[str] = []
    for declaration in match.group("body").split(";"):
        declaration = " ".join(declaration.split())
        if not declaration or declaration.startswith("#"):
            continue
        # "type a, b[2], *c" -> declarators after the first type token(s)
        head, *rest = declaration.split(",")
        names = [re.search(r"(\w+)\s*(?:\[[^\]]*\])*\s*$", head).group(1)]
        names += [re.search(r"\**\s*(\w+)", part).group(1) for part in rest]
        fields.extend(names)
    return fields


def probe_source(prelude: str, layout: dict[str, list[str]]) -> str:
    lines = [prelude, "#include <stdio.h>", "#include <stddef.h>", "int main( void ) {"]
    for type_name, fields in layout.items():
        lines.append(f'\tprintf( "sizeof {type_name} %u\\n", (unsigned)sizeof( {type_name} ) );')
        for field in fields:
            lines.append(
                f'\tprintf( "{type_name}.{field} %u %u\\n", (unsigned)offsetof( {type_name}, {field} ), '
                f"(unsigned)sizeof( (({type_name} *)0)->{field} ) );"
            )
    for constant in SHARED_CONSTANTS:
        lines += [
            f"#ifdef {constant}",
            f'\tprintf( "const {constant} %lld\\n", (long long)( {constant} ) );',
            "#else",
            f'\tprintf( "const {constant} <undefined>\\n" );',
            "#endif",
        ]
    for value in SHARED_ENUM_VALUES:
        lines.append(f'\tprintf( "enum {value} %d\\n", (int){value} );')
    lines += ["\treturn 0;", "}", ""]
    return "\n".join(lines)


def compile_and_run(cc: list[str], source: str, include_dir: Path, work: Path, tag: str) -> list[str]:
    source_path = work / f"abi_probe_{tag}.c"
    exe_path = work / f"abi_probe_{tag}.exe"
    source_path.write_text(source, encoding="utf-8")
    is_msvc = Path(cc[0]).stem.lower() in ("cl", "clang-cl")
    if is_msvc:
        command = cc + ["/nologo", "/TC", "/W0", "/DWIN32", "/D_CRT_SECURE_NO_WARNINGS", f"/I{include_dir}",
                        str(source_path), f"/Fe:{exe_path}", f"/Fo:{work}\\"]
    else:
        command = cc + ["-w", f"-I{include_dir}", str(source_path), "-o", str(exe_path), "-lm"]
    built = subprocess.run(command, cwd=work, capture_output=True, text=True, errors="replace")
    if built.returncode != 0:
        print(f"[{tag}] layout probe failed to compile:\n{built.stdout}{built.stderr}", file=sys.stderr)
        raise SystemExit(1)
    ran = subprocess.run([str(exe_path)], capture_output=True, text=True, check=True)
    return ran.stdout.splitlines()


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__.splitlines()[0])
    parser.add_argument("--fnql", type=Path, default=ROOT.parent / "FnQL",
                        help="FnQL engine checkout (default: ../FnQL)")
    parser.add_argument("--cc", default="cl", help="C compiler command for a 32-bit x86 target")
    args = parser.parse_args()

    fnql = args.fnql.resolve()
    if not (fnql / "code" / "qcommon" / "q_shared.h").is_file():
        print(f"FnQL checkout not found at {fnql}; skipping engine ABI parity")
        return 77

    layout = {name: struct_fields(fnql / header, name) for name, header in SHARED_STRUCTS.items()}
    cc = shlex.split(args.cc, posix=False)

    with tempfile.TemporaryDirectory(prefix="fnql-game-abi-") as temporary:
        work = Path(temporary)
        game = compile_and_run(cc, probe_source(GAME_PRELUDE, layout), ROOT / "code" / "game", work, "game")
        engine = compile_and_run(cc, probe_source(ENGINE_PRELUDE, layout), fnql / "code" / "qcommon",
                                 work, "engine")

    mismatches = [(g, e) for g, e in zip(game, engine) if g != e]
    if len(game) != len(engine):
        mismatches.append((f"{len(game)} game lines", f"{len(engine)} engine lines"))
    if mismatches:
        print("FnQL-game / FnQL engine ABI mismatch (game | engine):")
        for game_line, engine_line in mismatches:
            print(f"  {game_line:55s} | {engine_line}")
        return 1

    print(f"engine ABI parity OK: {len(game)} checks against {fnql}")
    return 0


if __name__ == "__main__":
    sys.exit(main())
