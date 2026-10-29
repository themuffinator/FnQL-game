#!/usr/bin/env python3
"""Extract wire-contract tables from the retail Quake Live game modules.

Reads qagamex86.dll and cgamex86.dll out of the user's legitimate retail
baseq3/bin.pk3 (read-only; nothing is copied or redistributed) and recovers
the tables whose order is part of the network contract:

  items          bg_itemlist rows (entity modelindex / EV_ITEM_PICKUP parms)
  meansOfDeath   modNames[] (obituary parms)
  gametypes      gametype display names (g_gametype values)
  pmTypes        pmove type names (pm_type values)
  spawnTable     spawn function classnames, in table order
  botItemIndices MODELINDEX_* values from baseq3/pak00.pk3 botfiles/inv.h
  playerKeys     CS_PLAYERS infostring keys, for bots and for humans
  warmupFormat   CS_WARMUP payload format

  --write   regenerate docs/contract/retail_tables.json
  --check   compare a fresh extraction with the committed file (default)

The tables are located structurally: a string table is found through the
pointer to its first entry, and bg_itemlist through the pointer to the first
real item's classname, so the tool does not depend on fixed addresses.
"""

from __future__ import annotations

import argparse
import json
import os
import re
import struct
import sys
import zipfile
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
TABLES = ROOT / "docs" / "contract" / "retail_tables.json"

# Retail gitem_t is 72 bytes: classname, pickup sound, eight model slots,
# icon, pickup name, quantity, type, tag and three trailing integers.
ITEM_STRIDE = 72
ITEM_FIELDS = 18


class PeImage:
    def __init__(self, data: bytes) -> None:
        self.data = data
        pe = struct.unpack_from("<I", data, 0x3C)[0]
        if data[pe:pe + 4] != b"PE\0\0" or struct.unpack_from("<H", data, pe + 4)[0] != 0x14C:
            raise SystemExit("not an x86 PE image")
        count = struct.unpack_from("<H", data, pe + 6)[0]
        optional = struct.unpack_from("<H", data, pe + 20)[0]
        self.base = struct.unpack_from("<I", data, pe + 24 + 28)[0]
        self.sections = [struct.unpack_from("<8sIIII", data, pe + 24 + optional + 40 * i)
                         for i in range(count)]

    def va(self, offset: int) -> int | None:
        for _, _, rva, size, raw in self.sections:
            if raw <= offset < raw + size:
                return self.base + rva + offset - raw
        return None

    def offset(self, va: int) -> int | None:
        rva = va - self.base
        for _, _, section_rva, size, raw in self.sections:
            if section_rva <= rva < section_rva + size:
                return raw + rva - section_rva
        return None

    def dword(self, va: int) -> int:
        return struct.unpack_from("<I", self.data, self.offset(va))[0]

    def string(self, va: int) -> str | None:
        offset = self.offset(va) if va else None
        if offset is None:
            return None
        end = self.data.find(b"\0", offset)
        raw = self.data[offset:end]
        if not raw or not all(32 <= c < 127 for c in raw):
            return None
        return raw.decode("ascii")

    def string_va(self, text: str) -> int:
        needle = b"\0" + text.encode("ascii") + b"\0"
        offset = self.data.find(needle)
        if offset < 0:
            raise SystemExit(f"string {text!r} not found")
        return self.va(offset + 1)

    def references(self, va: int) -> list[int]:
        packed = struct.pack("<I", va)
        found, start = [], 0
        while (index := self.data.find(packed, start)) >= 0:
            if index % 4 == 0:
                found.append(self.va(index))
            start = index + 1
        return [ref for ref in found if ref is not None]

    def string_table(self, first: str) -> list[str]:
        for ref in self.references(self.string_va(first)):
            names, cursor = [], ref
            while (name := self.string(self.dword(cursor))) is not None:
                names.append(name)
                cursor += 4
            if len(names) > 3:
                return names
        raise SystemExit(f"no string table starts with {first!r}")


def extract_items(image: PeImage) -> list[dict]:
    refs = image.references(image.string_va("item_armor_shard"))
    if len(refs) != 1:
        raise SystemExit(f"expected one bg_itemlist reference to item_armor_shard, found {len(refs)}")
    row_va = refs[0] - ITEM_STRIDE  # row 0 is the empty sentinel
    items = []
    while True:
        values = [image.dword(row_va + 4 * i) for i in range(ITEM_FIELDS)]
        if items and values[0] == 0:
            break
        text = [image.string(v) for v in values]
        items.append({
            "index": len(items),
            "classname": text[0],
            "pickupSound": text[1],
            "models": [text[2 + i] for i in range(8)],
            "icon": text[10],
            "pickupName": text[11],
            "quantity": values[12],
            "giType": values[13],
            "giTag": values[14],
            "trailing": values[15:18],
        })
        row_va += ITEM_STRIDE
    return items


def extract_spawn_table(image: PeImage) -> list[str]:
    """Classnames of the {name, function} spawn table, in order."""
    for ref in image.references(image.string_va("info_player_start")):
        start = ref
        while image.string(image.dword(start - 8)) is not None and image.dword(start - 4) > image.base:
            start -= 8
        names, cursor = [], start
        while (name := image.string(image.dword(cursor))) is not None:
            names.append(name)
            cursor += 8
        if len(names) > 20:
            return names
    raise SystemExit("spawn table not found")


def format_keys(fmt: str) -> list[str]:
    """Keys of an infostring format such as n\%s\t\%i (every other field)."""
    parts = fmt.split("\\")
    first = 1 if fmt.startswith("\\") else 0
    return [parts[k] for k in range(first, len(parts), 2)]


def extract_player_keys(image: PeImage) -> dict[str, list[str]]:
    formats = []
    needle = b"n\\%s\\t\\%i\\model\\"
    start = 0
    while (index := image.data.find(needle, start)) >= 0:
        end = image.data.find(b"\x00", index)
        formats.append(image.data[index:end].decode("ascii"))
        start = end
    bot = [f for f in formats if "\\skill\\" in f]
    human = [f for f in formats if "\\skill\\" not in f]
    if len(bot) != 1 or len(human) != 1:
        raise SystemExit(f"expected one bot and one human CS_PLAYERS format, found {formats}")
    return {"bot": format_keys(bot[0]), "human": format_keys(human[0])}


def extract_bot_item_indices(retail: Path) -> dict[str, int]:
    with zipfile.ZipFile(retail / "baseq3" / "pak00.pk3") as package:
        text = package.read("botfiles/inv.h").decode("latin-1")
    return {name: int(value) for name, value in
            re.findall(r"#define\s+(MODELINDEX_\w+)\s+(\d+)", text)}


def extract(retail: Path) -> dict:
    with zipfile.ZipFile(retail / "baseq3" / "bin.pk3") as package:
        qagame = PeImage(package.read("qagamex86.dll"))
        cgame = PeImage(package.read("cgamex86.dll"))
    mods = qagame.string_table("MOD_UNKNOWN")
    pm_types = qagame.string_table("PM_NORMAL")
    return {
        "source": "retail Quake Live baseq3/bin.pk3 (qagamex86.dll, cgamex86.dll)",
        "items": extract_items(qagame),
        "meansOfDeath": mods[:mods.index("MOD_RAILGUN_HEADSHOT") + 1],
        "gametypes": cgame.string_table("Free For All"),
        "pmTypes": pm_types[:pm_types.index("PM_TUTORIAL") + 1],
        "spawnTable": extract_spawn_table(qagame),
        "botItemIndices": extract_bot_item_indices(retail),
        "playerKeys": extract_player_keys(qagame),
        "warmupFormat": "\\time\\%i" if qagame.data.find(b"\\time\\%i\x00") >= 0 else None,
    }


def find_retail(explicit: str | None) -> Path:
    candidates = [explicit, os.environ.get("FNQL_RETAIL_QL_PATH")]
    program_files_x86 = os.environ.get("ProgramFiles(x86)", r"C:\Program Files (x86)")
    candidates.append(str(Path(program_files_x86) / "Steam" / "steamapps" / "common" / "Quake Live"))
    for candidate in candidates:
        if candidate and (Path(candidate) / "baseq3" / "bin.pk3").is_file():
            return Path(candidate)
    raise SystemExit(77)


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__.splitlines()[0])
    parser.add_argument("--retail", help="Quake Live install (default: $FNQL_RETAIL_QL_PATH or Steam)")
    parser.add_argument("--write", action="store_true", help="rewrite the committed tables")
    args = parser.parse_args()

    fresh = extract(find_retail(args.retail))
    if args.write:
        TABLES.parent.mkdir(parents=True, exist_ok=True)
        TABLES.write_text(json.dumps(fresh, indent=1) + "\n", encoding="utf-8")
        print(f"wrote {TABLES.relative_to(ROOT)}: {len(fresh['items'])} items, "
              f"{len(fresh['meansOfDeath'])} MODs, {len(fresh['gametypes'])} gametypes")
        return 0

    committed = json.loads(TABLES.read_text(encoding="utf-8"))
    if committed != fresh:
        print("retail tables differ from docs/contract/retail_tables.json; "
              "re-run with --write after reviewing the change")
        return 1
    print("committed retail tables match the retail install")
    return 0


if __name__ == "__main__":
    sys.exit(main())
