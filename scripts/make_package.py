#!/usr/bin/env python3
"""Build an FnQL-game module package (a pk3) deterministically.

The client package holds the FnQL-game cgame and ui, and nothing else; the
server package holds the qagame.  FnQL treats both as optional, never
referenced content (FnQL docs/fnql/GAME_MODULES.md):

  - a pure FnQL server that has the client package loaded lists it in its
    sv_fnqlClientPaks systeminfo key, and an FnQL client with the same
    package then runs its cgame and ui and reports the package's checksum;
  - reading either package's modules never marks the pak referenced, so no
    server asks a retail client to download or hold it.

Modules sit at the archive root, where the engine looks them up.  Entries
are sorted, stamped 1980-01-01 and deflated at a fixed level, so the same
inputs give the same bytes and the same pure checksum on every build.
"""

from __future__ import annotations

import argparse
import sys
import zipfile
from pathlib import Path

FIXED_TIME = (1980, 1, 1, 0, 0, 0)


def build(output: Path, members: list[Path]) -> None:
    names = [member.name for member in members]
    if len(set(name.lower() for name in names)) != len(names):
        raise SystemExit(f"duplicate member names: {names}")
    for member in members:
        if not member.is_file():
            raise SystemExit(f"missing module {member}")
        if not member.name.lower().startswith("fnql_"):
            raise SystemExit(f"{member.name}: FnQL-game packages hold only fnql_ modules")
    output.parent.mkdir(parents=True, exist_ok=True)
    temporary = output.with_name(output.name + ".tmp")
    with zipfile.ZipFile(temporary, "w") as archive:
        for member in sorted(members, key=lambda path: path.name.lower()):
            info = zipfile.ZipInfo(member.name, date_time=FIXED_TIME)
            info.compress_type = zipfile.ZIP_DEFLATED
            info.external_attr = 0o644 << 16
            info.create_system = 0
            archive.writestr(info, member.read_bytes(), compresslevel=9)
    temporary.replace(output)


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__.splitlines()[0])
    parser.add_argument("output", type=Path)
    parser.add_argument("modules", type=Path, nargs="+")
    args = parser.parse_args()
    build(args.output, args.modules)
    return 0


if __name__ == "__main__":
    sys.exit(main())
