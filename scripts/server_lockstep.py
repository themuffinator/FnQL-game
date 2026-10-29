#!/usr/bin/env python3
"""Gate G3: run the full-server lockstep differential and report divergences.

FnQL's dedicated server runs twice in its lockstep mode (sv_lockstep, see
FnQL's docs/fnql/SERVER_LOCKSTEP.md) with identical settings: the same map,
factory, seed, frame count and scenario script of scripted (non-bot,
networkless) clients.  The first run hosts the retail qagame, the second
FnQL-game's.  Both recordings are decoded and diffed frame by frame:

  - each scripted client's playerState_t, field by field;
  - every client-visible entity: presence, svFlags and entityState_t fields;
  - configstrings (infostrings key by key);
  - the game's reliable server commands and their targets.

Both runs must also have executed the same inputs and timeline commands;
anything else means the rig, not the module, diverged.

--self-check hosts the retail qagame in both runs.  Every divergence there is
nondeterminism in the rig, so it must be zero.

Exit status: 0 when the divergence count is within --max-divergent (a
self-check requires zero), 1 when it is not, 2 when a run or a recording is
broken, 77 (Meson "skip") without a retail install.  Scratch homes live under
.tmp/ and are removed unless --keep is given.
"""

from __future__ import annotations

import argparse
import json
import os
import re
import shutil
import struct
import subprocess
import sys
import tempfile
import time
from collections import Counter
from dataclasses import dataclass, field
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
sys.path.insert(0, str(ROOT / "scripts"))
from smoke_dedicated import MODULE_NAME, find_retail_install  # noqa: E402

SKIP = 77
HARNESS_ERROR = 2
GAME_HEADER = ROOT / "code" / "game" / "q_shared.h"
CONTRACT_ENUMS = ROOT / "docs" / "contract" / "retail_enums.json"

# ---------------------------------------------------------------------------
# Shared layouts: the one place the Python side describes them.
#
# These are the Quake Live structures FnQL-game shares with the engine by
# pointer (code/game/q_shared.h; gate G0 holds them equal to FnQL's).  Each
# entry is (field, kind, count); kind is "i" int32, "f" float32, "b" int8,
# "B" uint8, or the name of another table.  check_layouts() compares these
# tables with the C header, and the recording header's sizes are checked
# against them before anything is decoded.
# ---------------------------------------------------------------------------

TRAJECTORY_T = (
    ("trType", "i", 1),
    ("trTime", "i", 1),
    ("trDuration", "i", 1),
    ("trBase", "f", 3),
    ("trDelta", "f", 3),
    ("gravity", "f", 1),
)

PLAYER_STATE_T = (
    ("commandTime", "i", 1),
    ("pm_type", "i", 1),
    ("bobCycle", "i", 1),
    ("pm_flags", "i", 1),
    ("pm_time", "i", 1),
    ("origin", "f", 3),
    ("velocity", "f", 3),
    ("weaponTime", "i", 1),
    ("gravity", "i", 1),
    ("speed", "i", 1),
    ("delta_angles", "i", 3),
    ("groundEntityNum", "i", 1),
    ("legsTimer", "i", 1),
    ("legsAnim", "i", 1),
    ("torsoTimer", "i", 1),
    ("torsoAnim", "i", 1),
    ("movementDir", "i", 1),
    ("grapplePoint", "f", 3),
    ("eFlags", "i", 1),
    ("eventSequence", "i", 1),
    ("events", "i", "MAX_PS_EVENTS"),
    ("eventParms", "i", "MAX_PS_EVENTS"),
    ("externalEvent", "i", 1),
    ("externalEventParm", "i", 1),
    ("clientNum", "i", 1),
    ("location", "i", 1),
    ("weapon", "i", 1),
    ("weaponPrimary", "i", 1),
    ("weaponstate", "i", 1),
    ("fov", "i", 1),
    ("viewangles", "f", 3),
    ("viewheight", "i", 1),
    ("damageEvent", "i", 1),
    ("damageYaw", "i", 1),
    ("damagePitch", "i", 1),
    ("damageCount", "i", 1),
    ("stats", "i", "MAX_STATS"),
    ("persistant", "i", "MAX_PERSISTANT"),
    ("powerups", "i", "MAX_POWERUPS"),
    ("ammo", "i", "MAX_WEAPONS"),
    ("generic1", "i", 1),
    ("loopSound", "i", 1),
    ("jumppad_ent", "i", 1),
    ("jumpTime", "i", 1),
    ("doubleJumped", "i", 1),
    ("crouchTime", "i", 1),
    ("crouchSlideTime", "i", 1),
    ("forwardmove", "b", 1),
    ("rightmove", "b", 1),
    ("upmove", "b", 1),
    ("commandMirrorPad", "b", 1),
    ("ping", "i", 1),
    ("pmove_framecount", "i", 1),
    ("jumppad_frame", "i", 1),
    ("entityEventSequence", "i", 1),
    ("externalEventTime", "i", 1),
    ("armorTier", "i", 1),
)

# "source-only local sidecars appended after the retail replicated prefix"
# (q_shared.h).  With the retail qagame these bytes belong to whatever retail
# gclient_t stores after its playerState, so they are not compared unless
# --compare-sidecars is given.
PLAYER_STATE_SIDECARS = ("externalEventTime", "armorTier")

ENTITY_STATE_T = (
    ("number", "i", 1),
    ("eType", "i", 1),
    ("eFlags", "i", 1),
    ("pos", "trajectory_t", 1),
    ("apos", "trajectory_t", 1),
    ("time", "i", 1),
    ("time2", "i", 1),
    ("origin", "f", 3),
    ("origin2", "f", 3),
    ("angles", "f", 3),
    ("angles2", "f", 3),
    ("otherEntityNum", "i", 1),
    ("otherEntityNum2", "i", 1),
    ("groundEntityNum", "i", 1),
    ("constantLight", "i", 1),
    ("loopSound", "i", 1),
    ("modelindex", "i", 1),
    ("modelindex2", "i", 1),
    ("clientNum", "i", 1),
    ("frame", "i", 1),
    ("solid", "i", 1),
    ("event", "i", 1),
    ("eventParm", "i", 1),
    ("powerups", "i", 1),
    ("health", "i", 1),
    ("armor", "i", 1),
    ("weapon", "i", 1),
    ("location", "i", 1),
    ("legsAnim", "i", 1),
    ("torsoAnim", "i", 1),
    ("generic1", "i", 1),
    ("jumpTime", "i", 1),
    ("doubleJumped", "i", 1),
)

USERCMD_T = (
    ("serverTime", "i", 1),
    ("angles", "i", 3),
    ("buttons", "i", 1),
    ("weapon", "B", 1),
    ("weaponPrimary", "B", 1),
    ("fov", "B", 1),
    ("forwardmove", "b", 1),
    ("rightmove", "b", 1),
    ("upmove", "b", 1),
)

LAYOUTS = {
    "trajectory_t": TRAJECTORY_T,
    "playerState_t": PLAYER_STATE_T,
    "entityState_t": ENTITY_STATE_T,
    "usercmd_t": USERCMD_T,
}
# struct tag (or typedef name) of each table in q_shared.h
C_STRUCT_NAMES = {
    "trajectory_t": "trajectory_t",
    "playerState_t": "playerState_t",
    "entityState_t": "entityState_t",
    "usercmd_t": "usercmd_t",
}
ARRAY_CONSTANTS = ("MAX_PS_EVENTS", "MAX_STATS", "MAX_PERSISTANT", "MAX_POWERUPS", "MAX_WEAPONS")
SCALAR_SIZES = {"i": 4, "f": 4, "b": 1, "B": 1}
C_TYPES = {
    "int": ("i", 1),
    "float": ("f", 1),
    "vec3_t": ("f", 3),
    "signed char": ("b", 1),
    "char": ("b", 1),
    "byte": ("B", 1),
    "unsigned char": ("B", 1),
    "trType_t": ("i", 1),
    "trajectory_t": ("trajectory_t", 1),
}

# Fields whose values read best in hex.
HEX_FIELDS = {"pm_flags", "eFlags", "svFlags", "powerups", "buttons", "constantLight", "solid"}


@dataclass(frozen=True)
class Field:
    label: str      # e.g. "pos.trBase[2]"
    key: str        # what divergences are counted under: vectors collapse ("pos.trBase"), arrays do not
    name: str       # top-level field name, e.g. "pos"
    offset: int
    kind: str       # struct format character


def header_constants(source: str) -> dict[str, int]:
    return {name: int(value) for name, value in re.findall(r"#define\s+(\w+)\s+(\d+)\b", source)}


def resolve_count(count, constants: dict[str, int]) -> int:
    if isinstance(count, int):
        return count
    if count not in constants:
        raise SystemExit(f"{GAME_HEADER}: no #define for {count}")
    return constants[count]


def layout_size(name: str, constants: dict[str, int]) -> int:
    offset = 0
    for _, kind, count in LAYOUTS[name]:
        size, align = (layout_size(kind, constants), 4) if kind in LAYOUTS else (SCALAR_SIZES[kind], SCALAR_SIZES[kind])
        offset = (offset + align - 1) // align * align + size * resolve_count(count, constants)
    return (offset + 3) // 4 * 4


def flatten(name: str, constants: dict[str, int], base: int = 0, prefix: str = "") -> list[Field]:
    fields: list[Field] = []
    offset = 0
    for field_name, kind, count in LAYOUTS[name]:
        n = resolve_count(count, constants)
        if kind in LAYOUTS:
            size, align = layout_size(kind, constants), 4
        else:
            size = align = SCALAR_SIZES[kind]
        offset = (offset + align - 1) // align * align
        top = prefix.split(".")[0] if prefix else field_name
        # a literal count of 3 is a vector (vec3_t, delta_angles, angles); a
        # symbolic count (MAX_STATS, ...) is an indexed array
        vector = count == 3
        for i in range(n):
            label = f"{prefix}{field_name}" + (f"[{i}]" if n > 1 else "")
            if kind in LAYOUTS:
                fields += flatten(kind, constants, base + offset + i * size, label + ".")
            else:
                key = f"{prefix}{field_name}" if vector else label
                fields.append(Field(label, key, top, base + offset + i * size, kind))
        offset += size * n
    return fields


def parse_c_struct(source: str, type_name: str) -> list[tuple[str, str, int]]:
    text = re.sub(r"/\*.*?\*/", " ", source, flags=re.S)
    text = re.sub(r"//[^\n]*", " ", text)
    match = re.search(r"typedef\s+struct\s*\w*\s*\{(?P<body>[^{}]*)\}\s*" + re.escape(type_name) + r"\s*;", text)
    if not match:
        raise SystemExit(f"{GAME_HEADER}: cannot find struct {type_name}")
    constants = header_constants(source)
    result = []
    for declaration in match.group("body").split(";"):
        declaration = " ".join(declaration.split())
        if not declaration or declaration.startswith("#"):
            continue
        parsed = re.fullmatch(r"(?P<type>(?:signed |unsigned )?\w+)\s+(?P<names>.+)", declaration)
        if not parsed or parsed.group("type") not in C_TYPES:
            raise SystemExit(f"{GAME_HEADER}: {type_name}: unsupported declaration '{declaration}'")
        kind, width = C_TYPES[parsed.group("type")]
        for declarator in parsed.group("names").split(","):
            item = re.fullmatch(r"\s*(\w+)\s*(?:\[\s*(\w+)\s*\])?\s*", declarator)
            if not item:
                raise SystemExit(f"{GAME_HEADER}: {type_name}: unsupported declarator '{declarator}'")
            dim = item.group(2)
            count = 1 if dim is None else (int(dim) if dim.isdigit() else constants.get(dim, -1))
            result.append((item.group(1), kind, width * count))
    return result


def check_layouts() -> dict[str, int]:
    """Hold the tables to FnQL-game's C header; return the header constants."""
    source = GAME_HEADER.read_text(encoding="utf-8", errors="replace")
    constants = header_constants(source)
    missing = [name for name in ARRAY_CONSTANTS if name not in constants]
    if missing:
        raise SystemExit(f"{GAME_HEADER}: missing #define {', '.join(missing)}")
    problems = []
    for name, table in LAYOUTS.items():
        # vec3_t parses as ("x", "f", 3), the way the tables write it
        expected = [(f, k, resolve_count(c, constants)) for f, k, c in table]
        actual = parse_c_struct(source, C_STRUCT_NAMES[name])
        if expected != actual:
            problems.append(f"{name}: Python table {expected} != header {actual}")
    if problems:
        raise SystemExit("layout tables disagree with code/game/q_shared.h:\n  " + "\n  ".join(problems))
    return constants


# ---------------------------------------------------------------------------
# Names from the retail wire contract (docs/contract/retail_enums.json)
# ---------------------------------------------------------------------------

@dataclass
class Names:
    stats: dict[int, str] = field(default_factory=dict)
    persistant: dict[int, str] = field(default_factory=dict)
    powerups: dict[int, str] = field(default_factory=dict)
    weapons: dict[int, str] = field(default_factory=dict)
    entity_types: dict[int, str] = field(default_factory=dict)
    events: dict[int, str] = field(default_factory=dict)
    configstrings: dict[int, str] = field(default_factory=lambda: {0: "CS_SERVERINFO", 1: "CS_SYSTEMINFO"})

    @staticmethod
    def load() -> "Names":
        names = Names()
        try:
            groups = json.loads(CONTRACT_ENUMS.read_text(encoding="utf-8"))["groups"]
        except (OSError, ValueError, KeyError):
            return names

        def inverse(group: str) -> dict[int, str]:
            values = groups.get(group, {}).get("values", {})
            result: dict[int, str] = {}
            for key, value in values.items():
                if isinstance(value, int) and value not in result and "NUM_" not in key and not key.endswith("_MAX_GAME_TYPE"):
                    result[value] = key
            return result

        names.stats = inverse("statIndex_t")
        names.persistant = inverse("persEnum_t")
        names.powerups = inverse("powerup_t")
        names.weapons = inverse("weapon_t")
        names.entity_types = inverse("entityType_t")
        names.events = inverse("entity_event_t")
        # literal CS_* indices from bg_public.h (held to retail by gate G2),
        # then the contract's, which also covers the computed ranges
        try:
            header = (ROOT / "code" / "game" / "bg_public.h").read_text(encoding="utf-8", errors="replace")
            for name, value in re.findall(r"#define\s+(CS_\w+)\s+(0x[0-9A-Fa-f]+|\d+)\b", header):
                names.configstrings.setdefault(int(value, 0), name)
        except OSError:
            pass
        names.configstrings.update(inverse("configstrings"))
        return names

    def ps_label(self, label: str) -> str:
        match = re.fullmatch(r"(stats|persistant|powerups|ammo)\[(\d+)\]", label)
        if not match:
            return label
        table = {"stats": self.stats, "persistant": self.persistant, "powerups": self.powerups,
                 "ammo": self.weapons}[match.group(1)]
        index = int(match.group(2))
        return f"{match.group(1)}[{table[index]}]" if index in table else label

    def configstring(self, index: int) -> str:
        if index in self.configstrings:
            return self.configstrings[index]
        lower = [i for i in self.configstrings if i < index]
        if not lower:
            return str(index)
        base = max(lower)
        return f"{self.configstrings[base]}+{index - base}"


# ---------------------------------------------------------------------------
# Recording decoder (format: FnQL docs/fnql/SERVER_LOCKSTEP.md)
# ---------------------------------------------------------------------------

RECORD_MAGIC = b"FNQLLSTP"
RECORD_VERSION = 1
END_REASONS = {0: "frame limit", 1: "server shutdown"}
COMMAND_KINDS = {0: "cmd", 1: "clientcmd", 2: "userinfo"}


class RecordError(Exception):
    pass


@dataclass
class Frame:
    number: int
    time: int
    msec: int
    commands: list = field(default_factory=list)
    inputs: dict = field(default_factory=dict)
    players: dict = field(default_factory=dict)
    entities: list = field(default_factory=list)
    removed: list = field(default_factory=list)
    configstrings: list = field(default_factory=list)
    server_commands: list = field(default_factory=list)
    dropped_commands: int = 0


@dataclass
class Recording:
    path: Path
    ps_size: int = 0
    es_size: int = 0
    cmd_size: int = 0
    max_configstrings: int = 0
    max_entities: int = 0
    maxclients: int = 0
    fps: int = 0
    seed: int = 0
    frame_limit: int = 0
    map: str = ""
    provider: str = ""
    script: str = ""
    clients: list = field(default_factory=list)
    frames: list = field(default_factory=list)
    end: tuple | None = None


class Reader:
    def __init__(self, data: bytes):
        self.data = data
        self.pos = 0

    def take(self, count: int) -> bytes:
        if count < 0 or self.pos + count > len(self.data):
            raise RecordError(f"truncated at byte {self.pos} (wanted {count} more)")
        chunk = self.data[self.pos:self.pos + count]
        self.pos += count
        return chunk

    def u8(self) -> int:
        return self.take(1)[0]

    def u32(self) -> int:
        return struct.unpack("<I", self.take(4))[0]

    def i32(self) -> int:
        return struct.unpack("<i", self.take(4))[0]

    def string(self, limit: int = 1 << 20) -> str:
        length = self.u32()
        if length > limit:
            raise RecordError(f"string of {length} bytes at byte {self.pos}")
        return self.take(length).decode("latin-1")


def read_recording(path: Path) -> Recording:
    data = path.read_bytes()
    r = Reader(data)
    rec = Recording(path)
    if r.take(8) != RECORD_MAGIC:
        raise RecordError(f"{path}: not an FnQL lockstep recording")
    version = r.u32()
    if version != RECORD_VERSION:
        raise RecordError(f"{path}: recording version {version}, this decoder reads {RECORD_VERSION}")
    rec.ps_size, rec.es_size, rec.cmd_size = r.u32(), r.u32(), r.u32()
    rec.max_configstrings, rec.max_entities, rec.maxclients = r.u32(), r.u32(), r.u32()
    rec.fps, rec.seed, rec.frame_limit = r.u32(), r.u32(), r.u32()
    rec.map, rec.provider, rec.script = r.string(), r.string(), r.string()
    for _ in range(r.u32()):
        slot = r.i32()
        rec.clients.append((slot, r.string()))

    frame: Frame | None = None
    while r.pos < len(data):
        tag = chr(r.u8())
        if tag == "F":
            if frame is not None:
                raise RecordError(f"{path}: frame {frame.number} has no end record")
            frame = Frame(r.u32(), r.i32(), r.i32())
            continue
        if tag == "Q":
            if frame is not None:
                raise RecordError(f"{path}: end record inside frame {frame.number}")
            rec.end = (r.u32(), r.u32())
            continue
        if frame is None:
            raise RecordError(f"{path}: record '{tag}' outside a frame at byte {r.pos - 1}")
        if tag == "K":
            kind = r.u8()
            frame.commands.append((COMMAND_KINDS.get(kind, str(kind)), r.i32(), r.string()))
        elif tag == "U":
            slot = r.i32()
            frame.inputs[slot] = r.take(rec.cmd_size)
        elif tag == "P":
            slot = r.i32()
            frame.players[slot] = r.take(rec.ps_size)
        elif tag == "E":
            number, flags = r.i32(), r.i32()
            frame.entities.append((number, flags, r.take(rec.es_size)))
        elif tag == "R":
            frame.removed.append(r.i32())
        elif tag == "C":
            index = r.i32()
            frame.configstrings.append((index, r.string()))
        elif tag == "S":
            target = r.i32()
            frame.server_commands.append((target, r.string()))
        elif tag == "O":
            frame.dropped_commands += r.u32()
        elif tag == "X":
            if r.u32() != frame.number:
                raise RecordError(f"{path}: frame {frame.number} closed with a different number")
            rec.frames.append(frame)
            frame = None
        else:
            raise RecordError(f"{path}: unknown record '{tag}' at byte {r.pos - 1}")
    if frame is not None:
        raise RecordError(f"{path}: truncated inside frame {frame.number}")
    return rec


# ---------------------------------------------------------------------------
# Differ
# ---------------------------------------------------------------------------

def parse_info(text: str) -> dict[str, str] | None:
    """Split an infostring ("\\k\\v..." or "k\\v...") into keys, or None."""
    if "\\" not in text:
        return None
    parts = text[1:].split("\\") if text.startswith("\\") else text.split("\\")
    if len(parts) % 2:
        return None
    return dict(zip(parts[0::2], parts[1::2]))


# Infostring keys whose value is wall-clock time that no engine can pin, so
# only their shape is compared.  (configstring index, key) -> (pattern, why)
WALL_CLOCK_INFO_KEYS = {
    (0, "g_levelStartTime"): (r"\d+", "retail G_InitGame stores the CRT's _time64() (qagamex86 0x100551b6), "
                                      "not trap_RealTime"),
}


def show(value: str, limit: int = 160) -> str:
    return repr(value if len(value) <= limit else value[:limit] + "...")


class Differ:
    def __init__(self, a: Recording, b: Recording, constants: dict[str, int], names: Names, *,
                 detail_limit: int, compare_sidecars: bool, ignore_info_keys: set[str]):
        self.a, self.b = a, b
        self.names = names
        self.detail_limit = detail_limit
        self.ignore_info_keys = ignore_info_keys
        self.counts: Counter[str] = Counter()
        self.first_frame: dict[str, int] = {}
        self.first_detail: dict[str, str] = {}
        self.frames_with_divergence: set[int] = set()
        self.details: list[str] = []
        self.ignored: Counter[str] = Counter()
        self.wall_clock: dict[str, str] = {}
        self.ps_fields = [f for f in flatten("playerState_t", constants)
                          if compare_sidecars or f.name not in PLAYER_STATE_SIDECARS]
        self.es_fields = flatten("entityState_t", constants)
        self.cmd_fields = flatten("usercmd_t", constants)
        self.etype_offset = next(f.offset for f in self.es_fields if f.label == "eType")

    def add(self, frame: Frame, key: str, detail: str) -> None:
        self.counts[key] += 1
        line = f"frame {frame.number} (sv.time {frame.time}): {detail}"
        if key not in self.first_frame:
            self.first_frame[key] = frame.number
            self.first_detail[key] = line
        self.frames_with_divergence.add(frame.number)
        if len(self.details) < self.detail_limit:
            self.details.append(line)

    @staticmethod
    def value(raw: bytes, f: Field):
        return struct.unpack_from("<" + f.kind, raw, f.offset)[0]

    def render(self, raw: bytes, f: Field) -> str:
        value = self.value(raw, f)
        if f.kind == "f":
            bits = struct.unpack_from("<I", raw, f.offset)[0]
            return f"{value!r} (0x{bits:08x})"
        if f.name in HEX_FIELDS or f.label.split("[")[0] in HEX_FIELDS:
            return f"{value} (0x{value & 0xFFFFFFFF:x})"
        return str(value)

    def fields_differ(self, fields: list[Field], ra: bytes, rb: bytes) -> list[Field]:
        size = struct.calcsize
        return [f for f in fields if ra[f.offset:f.offset + size(f.kind)] != rb[f.offset:f.offset + size(f.kind)]]

    def entity_name(self, raw: bytes) -> str:
        etype = struct.unpack_from("<i", raw, self.etype_offset)[0]
        events = next((v for v, n in self.names.entity_types.items() if n == "ET_EVENTS"), None)
        if events is not None and etype > events:
            event = etype - events
            return f"ET_EVENTS+{self.names.events.get(event, event)}"
        return self.names.entity_types.get(etype, f"eType {etype}")

    def compare_header(self) -> list[str]:
        a, b = self.a, self.b
        problems = []
        for attr in ("ps_size", "es_size", "cmd_size", "max_configstrings", "max_entities", "maxclients",
                     "fps", "seed", "frame_limit", "map", "script", "clients"):
            if getattr(a, attr) != getattr(b, attr):
                problems.append(f"header {attr}: {getattr(a, attr)!r} != {getattr(b, attr)!r}")
        return problems

    def run(self) -> None:
        ents = ({}, {})
        css = ({}, {})
        for fa, fb in zip(self.a.frames, self.b.frames):
            if fa.number != fb.number:
                raise RecordError(f"frame numbering differs: {fa.number} != {fb.number}")
            if (fa.time, fa.msec) != (fb.time, fb.msec):
                self.add(fa, "frame.time", f"sv.time/msec {fa.time}/{fa.msec} != {fb.time}/{fb.msec}")
            if fa.commands != fb.commands:
                self.add(fa, "rig.commands", f"executed timeline commands differ: {fa.commands} != {fb.commands}")
            self.compare_inputs(fa, fb)
            self.compare_players(fa, fb)
            for state, frame in zip(ents, (fa, fb)):
                for number in frame.removed:
                    state.pop(number, None)
                for number, flags, raw in frame.entities:
                    state[number] = (flags, raw)
            self.compare_entities(fa, ents[0], ents[1])
            for state, frame in zip(css, (fa, fb)):
                for index, text in frame.configstrings:
                    if text:
                        state[index] = text
                    else:
                        state.pop(index, None)
            self.compare_configstrings(fa, css[0], css[1])
            self.compare_server_commands(fa, fb)
        if len(self.a.frames) != len(self.b.frames):
            last = self.a.frames[-1] if self.a.frames else Frame(0, 0, 0)
            self.add(last, "frame.count", f"{len(self.a.frames)} recorded frames != {len(self.b.frames)}")

    def compare_inputs(self, fa: Frame, fb: Frame) -> None:
        for slot in sorted(set(fa.inputs) | set(fb.inputs)):
            ra, rb = fa.inputs.get(slot), fb.inputs.get(slot)
            if ra != rb:
                which = "missing on one side" if ra is None or rb is None else ", ".join(
                    f"{f.label} {self.value(ra, f)} != {self.value(rb, f)}"
                    for f in self.fields_differ(self.cmd_fields, ra, rb))
                self.add(fa, "rig.usercmd", f"client {slot} usercmd differs: {which}")

    def compare_players(self, fa: Frame, fb: Frame) -> None:
        for slot in sorted(set(fa.players) | set(fb.players)):
            ra, rb = fa.players.get(slot), fb.players.get(slot)
            if ra is None or rb is None:
                self.add(fa, "ps.presence", f"client {slot} playerState only in "
                         f"{'first' if rb is None else 'second'} run")
                continue
            if ra == rb:
                continue
            for f in self.fields_differ(self.ps_fields, ra, rb):
                label = self.names.ps_label(f.label)
                self.add(fa, f"ps.{self.names.ps_label(f.key)}",
                         f"client {slot} ps.{label}: {self.render(ra, f)} != {self.render(rb, f)}")

    def compare_entities(self, frame: Frame, ea: dict, eb: dict) -> None:
        for number in sorted(set(ea) | set(eb)):
            a, b = ea.get(number), eb.get(number)
            if a == b:
                continue
            if a is None or b is None:
                raw = (a or b)[1]
                side = "first" if b is None else "second"
                self.add(frame, "entity.presence", f"entity {number} ({self.entity_name(raw)}) visible only in {side} run")
                continue
            kind = self.entity_name(a[1])
            if a[0] != b[0]:
                self.add(frame, "es.svFlags", f"entity {number} ({kind}) svFlags 0x{a[0]:x} != 0x{b[0]:x}")
            for f in self.fields_differ(self.es_fields, a[1], b[1]):
                self.add(frame, f"es.{f.key}",
                         f"entity {number} ({kind}) es.{f.label}: {self.render(a[1], f)} != {self.render(b[1], f)}")

    def compare_configstrings(self, frame: Frame, ca: dict, cb: dict) -> None:
        if ca == cb:
            return
        for index in sorted(set(ca) | set(cb)):
            ta, tb = ca.get(index, ""), cb.get(index, "")
            if ta == tb:
                continue
            name = self.names.configstring(index)
            ia, ib = parse_info(ta), parse_info(tb)
            if ia is not None and ib is not None:
                for key in sorted(set(ia) | set(ib)):
                    va, vb = ia.get(key), ib.get(key)
                    if va == vb:
                        continue
                    if index == 0 and key in self.ignore_info_keys:
                        self.ignored[f"cs[{name}].{key}"] += 1
                        continue
                    wall_clock = WALL_CLOCK_INFO_KEYS.get((index, key))
                    if wall_clock and va is not None and vb is not None and \
                            re.fullmatch(wall_clock[0], va) and re.fullmatch(wall_clock[0], vb):
                        self.wall_clock[f"cs[{name}].{key}"] = wall_clock[1]
                        continue
                    self.add(frame, f"cs[{name}].{key}",
                             f"configstring {index} ({name}) key {key!r}: "
                             f"{'<absent>' if va is None else show(va)} != {'<absent>' if vb is None else show(vb)}")
            else:
                self.add(frame, f"cs[{name}]", f"configstring {index} ({name}): {show(ta)} != {show(tb)}")

    def compare_server_commands(self, fa: Frame, fb: Frame) -> None:
        if fa.dropped_commands or fb.dropped_commands:
            self.add(fa, "rig.commandOverflow", f"server commands dropped by the recorder: "
                     f"{fa.dropped_commands} / {fb.dropped_commands}")
        if fa.server_commands == fb.server_commands:
            return
        only_a = Counter(fa.server_commands) - Counter(fb.server_commands)
        only_b = Counter(fb.server_commands) - Counter(fa.server_commands)
        if not only_a and not only_b:
            self.add(fa, "cmd.order", f"same server commands in a different order: {fa.server_commands}")
            return
        for side, extra in (("first", only_a), ("second", only_b)):
            for (target, text), count in sorted(extra.items()):
                verb = text.split(" ", 1)[0] if text else "<empty>"
                where = "broadcast" if target == -1 else f"client {target}"
                for _ in range(count):
                    self.add(fa, f"cmd.{verb}", f"server command only in {side} run ({where}): {show(text)}")

    def total(self) -> int:
        return sum(self.counts.values())


# ---------------------------------------------------------------------------
# Runner
# ---------------------------------------------------------------------------

@dataclass
class RunResult:
    label: str
    provider: str
    home: Path
    returncode: int | None
    seconds: float
    log: str
    recording: Recording | None = None
    error: str = ""


# Com_Error banner and message, the shutdown a fatal error causes (a headless
# Windows dedicated server then exits 0), and FnQL-game module rejection
LOG_ERRORS = ("********************", "ERROR:", "Server fatal crashed", "Server crashed", "Rejected DLL",
              "is not a usable FnQL-game qagame module", "loading the retail module instead")
FATAL_LOG_MARKERS = ("Server fatal crashed", "Server crashed", "********************")


def run_server(args, label: str, provider: str, scenario: Path) -> RunResult:
    scratch_root = ROOT / ".tmp"
    scratch_root.mkdir(exist_ok=True)
    home = Path(tempfile.mkdtemp(prefix=f"server-lockstep-{label}-", dir=scratch_root))
    (home / "baseq3" / "lockstep").mkdir(parents=True)
    script_qpath = f"lockstep/{scenario.name}"
    shutil.copy2(scenario, home / "baseq3" / "lockstep" / scenario.name)
    if provider == "fnql":
        shutil.copy2(args.module, home / "baseq3" / MODULE_NAME)
    command = [
        str(args.engine),
        "+set", "fs_basepath", str(args.retail),
        "+set", "fs_homepath", str(home),
        "+set", "logfile", "2",
        "+set", "net_port", str(args.port),
        "+set", "sv_pure", "0",
        "+set", "com_steamIntegration", "0",
        "+set", "sv_floodProtect", "0",
        "+set", "vm_gameProvider", provider,
        "+set", "sv_lockstep", "1",
        "+set", "sv_lockstepSeed", str(args.seed),
        "+set", "sv_lockstepScript", script_qpath,
        "+set", "sv_lockstepRecord", "lockstep.rec",
        "+set", "sv_lockstepFrames", str(args.frames),
    ]
    for name, value in args.set:
        command += ["+set", name, value]
    # --cheats starts the map with devmap, so a scenario may use cheat commands
    # such as setviewpos to put a player where the test needs it
    map_command = "devmap" if args.cheats else "map"
    command += ["+set", "serverstartup", f"{map_command} {args.map} {args.factory}"]

    log_path = home / "baseq3" / "qconsole.log"

    def read_log() -> str:
        try:
            return log_path.read_text(encoding="utf-8", errors="replace")
        except OSError:
            return ""

    started = time.monotonic()
    process = subprocess.Popen(command, cwd=args.engine.parent, stdin=subprocess.DEVNULL,
                               stdout=subprocess.DEVNULL, stderr=subprocess.DEVNULL,
                               creationflags=getattr(subprocess, "CREATE_NO_WINDOW", 0))
    returncode = None
    stopped = ""
    try:
        while True:
            try:
                returncode = process.wait(timeout=0.25)
                break
            except subprocess.TimeoutExpired:
                pass
            # A fatal error leaves the Windows dedicated console waiting for a
            # user to close it, so stop the process as soon as the log says so.
            if any(marker in read_log() for marker in FATAL_LOG_MARKERS):
                stopped = "stopped after a fatal error"
                break
            if time.monotonic() - started > args.timeout:
                stopped = f"timed out after {args.timeout:.0f}s"
                break
    finally:
        if process.poll() is None:
            process.kill()
            process.wait(timeout=30)
    seconds = time.monotonic() - started
    log = read_log()
    result = RunResult(label, provider, home, returncode, seconds, log)

    if stopped:
        fatal = [line for line in log.splitlines() if any(marker in line for marker in LOG_ERRORS)]
        result.error = stopped + (": " + fatal[0].strip("- ") if fatal else "")
        return result
    if returncode != 0:
        result.error = f"exited with code {returncode}"
    fatal = [line for line in log.splitlines() if any(marker in line for marker in LOG_ERRORS)]
    if fatal and not result.error:
        result.error = "engine error in the log: " + next((l for l in fatal if "ERROR" in l), fatal[0])
    record_path = home / "lockstep.rec"
    if not record_path.is_file():
        result.error = result.error or "no recording was written"
        return result
    try:
        result.recording = read_recording(record_path)
    except RecordError as error:
        result.error = result.error or f"recording unreadable: {error}"
        return result
    rec = result.recording
    if rec.end is None:
        result.error = result.error or "recording has no end record (the server did not shut down cleanly)"
    elif rec.end[1] != 0 or rec.end[0] != args.frames or len(rec.frames) != args.frames:
        result.error = result.error or (f"recording ended after {rec.end[0]} frame(s) by "
                                        f"{END_REASONS.get(rec.end[1], rec.end[1])}, wanted {args.frames}")
    if not result.error and (rec.provider != provider or (provider == "fnql") != ("VM_LoadDll(qagame): FnQL-game " in log)):
        result.error = f"vm_gameProvider {provider} did not host the expected module (recorded {rec.provider!r})"
    return result


def report(differ: Differ, title: str, runs: list[str]) -> None:
    a = differ.a
    print(title)
    for line in runs:
        print(f"  {line}")
    print(f"  scripted clients: {', '.join(f'{name} (slot {slot})' for slot, name in a.clients) or 'none'}")
    total = differ.total()
    if total == 0:
        print("no divergence")
    else:
        first = min(differ.first_frame.values())
        print(f"{total} divergence(s) (field x frame) in {len(differ.counts)} field(s) over "
              f"{len(differ.frames_with_divergence)} of {len(a.frames)} frame(s); first at frame {first}")
        print(f"first divergences ({len(differ.details)} shown, --details N for more):")
        for line in differ.details:
            print(f"  {line}")
        print("per field: divergent frames, first frame, and its first divergence:")
        width = max(len(key) for key in differ.counts)
        for key, count in sorted(differ.counts.items(), key=lambda kv: (differ.first_frame[kv[0]], kv[0])):
            detail = differ.first_detail[key].split(": ", 1)[1]
            print(f"  {key:<{width}}  {count:6d}  @{differ.first_frame[key]:<5d} {detail[:150]}")
    for key, why in sorted(differ.wall_clock.items()):
        print(f"compared by shape only: {key} is wall-clock time ({why})")
    for key, count in sorted(differ.ignored.items()):
        print(f"ignored by policy: {key} differed on {count} frame(s) "
              f"(identity announcement, docs/COMPATIBILITY.md rule 4; --strict-identity compares it)")


def diff_recordings(args, constants: dict[str, int], a: Recording, b: Recording, title: str,
                    runs: list[str]) -> int:
    sizes = tuple(layout_size(name, constants) for name in ("playerState_t", "entityState_t", "usercmd_t"))
    for rec in (a, b):
        if (rec.ps_size, rec.es_size, rec.cmd_size) != sizes:
            print(f"{rec.path}: engine structure sizes {rec.ps_size}/{rec.es_size}/{rec.cmd_size} do not match "
                  f"the layout tables {sizes}; rebuild both sides (gate G0)")
            return HARNESS_ERROR
    differ = Differ(a, b, constants, Names.load(), detail_limit=args.details,
                    compare_sidecars=args.compare_sidecars,
                    ignore_info_keys=set() if args.strict_identity else {"fnql_game"})
    header_problems = differ.compare_header()
    if header_problems:
        print("the two runs were not set up identically:\n  " + "\n  ".join(header_problems))
        return HARNESS_ERROR
    try:
        differ.run()
    except RecordError as error:
        print(f"recordings cannot be compared: {error}")
        return HARNESS_ERROR
    report(differ, title, runs)

    rig = sum(count for key, count in differ.counts.items() if key.startswith("rig."))
    if rig:
        print(f"FAILED: the two runs did not execute the same inputs ({rig} rig divergence(s))")
        return HARNESS_ERROR
    limit = 0 if args.self_check else args.max_divergent
    if differ.total() > limit:
        print(f"FAILED: {differ.total()} divergence(s), allowed {limit}")
        return 1
    print("server lockstep " + ("self-check OK: the rig is deterministic" if args.self_check
                                else f"differential OK: {differ.total()} divergence(s), allowed {limit}"))
    return 0


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__.splitlines()[0])
    parser.add_argument("--engine", type=Path, default=os.environ.get("FNQL_DEDICATED_EXE"),
                        help="x86 FnQL dedicated server (default: $FNQL_DEDICATED_EXE)")
    parser.add_argument("--retail", help="Quake Live install (default: $FNQL_RETAIL_QL_PATH or Steam)")
    parser.add_argument("--module", type=Path, default=ROOT / "build" / "win32-x86" / MODULE_NAME)
    parser.add_argument("--scenario", type=Path, default=ROOT / "tests" / "lockstep" / "server" / "ffa_two_players.txt",
                        help="scenario script (FnQL docs/fnql/SERVER_LOCKSTEP.md grammar)")
    parser.add_argument("--map", default="campgrounds")
    parser.add_argument("--factory", default="ffa")
    parser.add_argument("--cheats", action="store_true",
                        help="start the map with devmap (sv_cheats 1) in both runs")
    parser.add_argument("--seed", type=int, default=1, help="sv_lockstepSeed for both runs")
    parser.add_argument("--frames", type=int, default=400, help="recorded server frames per run")
    parser.add_argument("--max-divergent", type=int, default=0,
                        help="pass when the divergence count (field x frame) is at most this")
    parser.add_argument("--self-check", action="store_true",
                        help="host the retail qagame in both runs; requires zero divergence")
    parser.add_argument("--details", type=int, default=25, help="first divergences printed in detail")
    parser.add_argument("--compare-sidecars", action="store_true",
                        help="also compare the playerState_t local sidecars (externalEventTime, armorTier)")
    parser.add_argument("--strict-identity", action="store_true",
                        help="also compare the fnql_game serverinfo identity key")
    parser.add_argument("--set", nargs=2, action="append", default=[], metavar=("CVAR", "VALUE"),
                        help="extra +set for both runs (repeatable)")
    parser.add_argument("--port", type=int, default=27995)
    parser.add_argument("--timeout", type=float, default=300.0, help="seconds allowed per run")
    parser.add_argument("--keep", action="store_true", help="keep the scratch homes")
    parser.add_argument("--check-layout", action="store_true", help="only hold the field tables to q_shared.h")
    parser.add_argument("--compare", nargs=2, type=Path, metavar=("FIRST", "SECOND"),
                        help="diff two existing recordings instead of running the server")
    args = parser.parse_args()

    constants = check_layouts()
    if args.check_layout:
        print(f"layout tables match {GAME_HEADER.relative_to(ROOT)}: "
              + ", ".join(f"{name} {layout_size(name, constants)} bytes" for name in LAYOUTS))
        return 0

    if args.compare:
        try:
            a, b = (read_recording(path) for path in args.compare)
        except (OSError, RecordError) as error:
            print(f"cannot read the recordings: {error}")
            return HARNESS_ERROR
        runs = [f"{rec.path}: vm_gameProvider {rec.provider}, {rec.map}, seed {rec.seed}, {len(rec.frames)} frames"
                for rec in (a, b)]
        return diff_recordings(args, constants, a, b, "server lockstep comparison", runs)

    try:
        args.retail = find_retail_install(args.retail)
    except SystemExit:
        print("no retail Quake Live install; skipping")
        return SKIP
    if not args.engine or not Path(args.engine).is_file():
        print("pass --engine <fnql.ded.exe> or set FNQL_DEDICATED_EXE", file=sys.stderr)
        return HARNESS_ERROR
    args.engine = Path(args.engine).resolve()
    args.scenario = args.scenario.resolve()
    if not args.scenario.is_file():
        print(f"scenario not found: {args.scenario}", file=sys.stderr)
        return HARNESS_ERROR
    if not re.fullmatch(r"[A-Za-z0-9_.-]{1,48}", args.scenario.name):
        print(f"scenario file name {args.scenario.name!r} must be a short plain name", file=sys.stderr)
        return HARNESS_ERROR
    if args.frames <= 0:
        print("--frames must be positive", file=sys.stderr)
        return HARNESS_ERROR
    if not (0 <= args.seed <= 0xFFFFFFFF):
        print("--seed must be an unsigned 32-bit integer", file=sys.stderr)
        return HARNESS_ERROR
    if not args.self_check:
        args.module = args.module.resolve()
        if not args.module.is_file():
            print(f"module not found: {args.module} (build it first)", file=sys.stderr)
            return HARNESS_ERROR

    plan = [("retail-a", "retail"), ("retail-b", "retail")] if args.self_check else [("retail", "retail"), ("fnql", "fnql")]
    results: list[RunResult] = []
    try:
        for label, provider in plan:
            results.append(run_server(args, label, provider, args.scenario))
            if results[-1].error:
                break  # the second run cannot be compared with anything
        broken = [r for r in results if r.error]
        for result in broken:
            print(f"server lockstep run {result.label} FAILED: {result.error}")
            print(f"--- {result.label} log tail ---")
            print(result.log[-6000:])
        if broken:
            return HARNESS_ERROR
        title = (f"server lockstep {'self-check' if args.self_check else 'differential'}: {args.map} "
                 f"({args.factory}), seed {args.seed}, {args.frames} frames, scenario {args.scenario.name}")
        runs = [f"{r.label}: vm_gameProvider {r.provider}, {len(r.recording.frames)} frames, {r.seconds:.1f}s"
                for r in results]
        return diff_recordings(args, constants, results[0].recording, results[1].recording, title, runs)
    finally:
        for result in results:
            if args.keep:
                print(f"kept {result.home}")
            else:
                shutil.rmtree(result.home, ignore_errors=True)


if __name__ == "__main__":
    sys.exit(main())
