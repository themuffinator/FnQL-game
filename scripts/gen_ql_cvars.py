#!/usr/bin/env python3
"""Print G_CVAR registrations for retail Quake Live cvars.

The names, default strings and flags come from the retail qagame cvar table in
docs/contract/retail_tables.json, so registrations added to
code/game/g_ql_cvars.h match retail exactly (gate G2 checks them).  Pass cvar
names or shell-style patterns:

    python scripts/gen_ql_cvars.py g_max_knockback "g_knockback_*"
"""

from __future__ import annotations

import argparse
import fnmatch
import json
import sys
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]

FLAG_NAMES = [
    (0x1, "CVAR_ARCHIVE"), (0x2, "CVAR_USERINFO"), (0x4, "CVAR_SERVERINFO"),
    (0x8, "CVAR_SYSTEMINFO"), (0x10, "CVAR_INIT"), (0x20, "CVAR_LATCH"),
    (0x40, "CVAR_ROM"), (0x80, "CVAR_USER_CREATED"), (0x100, "CVAR_TEMP"),
    (0x200, "CVAR_CHEAT"), (0x400, "CVAR_NORESTART"), (0x4000, "CVAR_QL_PMOVE_SETTINGS"),
    (0x8000, "CVAR_QL_WEAPON_RELOAD"), (0x10000, "CVAR_QL_PLAYER_APPEARANCE"),
    (0x20000, "CVAR_QL_ARMOR_TIERED"), (0x40000, "CVAR_QL_FLAG_40000"), (0x100000, "CVAR_GAMERULE"),
]


def flag_expression(flags: int) -> str:
    parts = []
    for bit, name in reversed(FLAG_NAMES):
        if flags & bit:
            parts.append(name)
            flags &= ~bit
    if flags:
        parts.append(f"0x{flags:x}")
    return " | ".join(parts) if parts else "0"


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__.splitlines()[0])
    parser.add_argument("names", nargs="+")
    args = parser.parse_args()

    table = json.loads((ROOT / "docs" / "contract" / "retail_tables.json").read_text(encoding="utf-8"))
    cvars = table["cvars"]
    missing = []
    for pattern in args.names:
        rows = [c for c in cvars if fnmatch.fnmatchcase(c["name"], pattern)]
        if not rows:
            missing.append(pattern)
        for c in rows:
            default = c["default"].replace("\\", "\\\\").replace('"', '\\"')
            print(f'G_CVAR( {c["name"]}, "{c["name"]}", "{default}", {flag_expression(c["flags"])}, 0, qfalse, qfalse )')
    if missing:
        print("not in the retail table: " + ", ".join(missing), file=sys.stderr)
        return 1
    return 0


if __name__ == "__main__":
    sys.exit(main())
