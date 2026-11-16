#!/usr/bin/env python3
"""Check the match statistics a game module reports against the retail schema.

Retail qagame reports a match as events (PLAYER_CONNECT, PLAYER_KILL,
PLAYER_DEATH, PLAYER_MEDAL, PLAYER_STATS, MATCH_STARTED, ROUND_OVER, ...) and
a closing MATCH_REPORT.  The engine publishes them on its ZMQ stats socket and
derives Steam statistics from them.  This probe hosts a bot match on an FnQL
dedicated server with the engine's stats journal on (FnQL `sv_statsJournal`,
which needs no libzmq) and checks every journalled document against
docs/contract/stats_events.json: no unknown event or key, every required key
present, every value of a retail JSON type and from retail's vocabulary.

With --provider retail it hosts the retail qagame instead, which keeps the
committed schema honest against the user's install; with --compare it hosts
both in turn and also requires the documents that do not depend on how the
bots play (MATCH_STARTED, PLAYER_CONNECT) to be equal value for value.

A bot match is not deterministic, so this gate proves the shape and the
vocabulary of the reports, not their numbers (tests/game_json.c gates the
writer underneath).  Exit code 77 means the engine could not read the retail
module's objects at all, so there was nothing to compare.  The retail assets
come from the user's legitimate Quake Live install.  Scratch files go under
.tmp/ and are removed.
"""

from __future__ import annotations

import argparse
import json
import os
import shutil
import subprocess
import sys
import tempfile
import time
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parent))
from smoke_dedicated import (  # noqa: E402
    FORBIDDEN, MODULE_NAME, ROOT, find_retail_install, hosted_module_name)
import headless  # noqa: E402

SCHEMA = ROOT / "docs" / "contract" / "stats_events.json"
JOURNAL = "stats.jsonl"

# Documents whose content does not depend on how the bots play.  The match
# GUID is drawn per server start, and TIME moves with the frame a bot joins.
STABLE = {"MATCH_STARTED": {"MATCH_GUID"}, "PLAYER_CONNECT": {"MATCH_GUID", "TIME"}}

# Factories without sides, whose bots join on their own.
TEAMLESS = {"ffa", "duel", "race", "rr", "iffa", "vql"}

# Where a string must come from one of the schema's vocabularies.
VOCABULARY = {
    "MOD": "MOD", "KILLER.WEAPON": "WEAPON", "VICTIM.WEAPON": "WEAPON", "GAME_TYPE": "GAME_TYPE",
    "KILLER.POWERUPS[]": "POWERUPS[]", "VICTIM.POWERUPS[]": "POWERUPS[]",
    "KILLER.HOLDABLE": "HOLDABLE", "VICTIM.HOLDABLE": "HOLDABLE", "MEDAL": "MEDAL", "STATUS": "STATUS",
    "FLAG_TEAM": "TEAM_NAME", "KILLER.OLD_TEAM": "TEAM_NAME", "TEAM_WON": "TEAM_WON",
}


def json_type(value) -> str:
    if value is None:
        return "null"
    if isinstance(value, bool):
        return "bool"
    if isinstance(value, int):
        return "int"
    if isinstance(value, float):
        return "real"
    if isinstance(value, str):
        return "string"
    return "array" if isinstance(value, list) else "object"


def parse_rules(event_rules: dict) -> dict:
    """The schema's shorthand: "int|null" lists the types, a trailing ? marks an optional key."""
    return {path: (text.rstrip("?").split("|"), text.endswith("?")) for path, text in event_rules.items()}


def validate(value, path: str, rules: dict, vocabulary: dict, kind_of_event: str, failures: list[str]) -> None:
    kind = json_type(value)
    if path:
        rule = rules.get(path)
        if rule is None:
            failures.append(f"{path} is not in the retail document")
            return
        # a real that happens to be whole may be written without a fraction
        if kind not in rule[0] and not (kind == "int" and "real" in rule[0]):
            failures.append(f"{path} is {kind}, retail has {'/'.join(rule[0])}")
        words = VOCABULARY.get(path)
        if words == "TEAM_NAME" and kind_of_event == "PLAYER_KILL":
            words = None
        if kind == "string" and words and value not in vocabulary[words]:
            failures.append(f"{path} is {value!r}, not one of retail's {words} names")
    if isinstance(value, dict):
        for key, child in value.items():
            validate(child, f"{path}.{key}" if path else key, rules, vocabulary, kind_of_event, failures)
        for candidate, (_, optional) in rules.items():
            parent, _, leaf = candidate.rpartition(".")
            if parent == path and not optional and not leaf.endswith("[]") and leaf not in value:
                failures.append(f"{candidate} is missing")
    elif isinstance(value, list):
        for child in value:
            validate(child, path + "[]", rules, vocabulary, kind_of_event, failures)


def check(events: list[dict], schema: dict) -> list[str]:
    failures = []
    for index, event in enumerate(events):
        kind = event.get("TYPE")
        rules = schema["events"].get(kind)
        where = f"event {index} ({kind})"
        if rules is None:
            failures.append(f"{where}: not a retail event")
            continue
        data = event.get("DATA")
        if not isinstance(data, dict):
            failures.append(f"{where}: DATA is {json_type(data)}, not an object")
            continue
        found: list[str] = []
        validate(data, "", parse_rules(rules), schema["vocabulary"], kind, found)
        # PLAYER_SWITCHTEAM names its teams; the kill documents number them
        if kind == "PLAYER_SWITCHTEAM" and data.get("KILLER", {}).get("TEAM") not in schema["vocabulary"]["TEAM_NAME"]:
            found.append(f"KILLER.TEAM is {data.get('KILLER', {}).get('TEAM')!r}, not a team name")
        failures += [f"{where}: {failure}" for failure in found]
    return failures


def check_abort(events: list[dict]) -> list[str]:
    """A player left mid-match and the level was then shut down under the match."""
    failures = []
    stats = [event["DATA"] for event in events if event.get("TYPE") == "PLAYER_STATS"]
    reports = [event["DATA"] for event in events if event.get("TYPE") == "MATCH_REPORT"]
    leavers = [data for data in stats if data.get("QUIT") == 1]
    if not leavers:
        failures.append("no PLAYER_STATS with QUIT 1 for the kicked player")
    for data in leavers:
        if data.get("RANK") != -1 or data.get("TIED_RANK") != 1 or data.get("WIN") != 0 or data.get("LOSE") != 0:
            failures.append(f"a leaver's result is not retail's (RANK -1, TIED_RANK 1, WIN 0, LOSE 0): {data.get('NAME')}")
        if data.get("ABORTED") is not False:
            failures.append(f"a leaver's PLAYER_STATS is marked aborted: {data.get('NAME')}")
    if len(reports) != 1 or reports[0].get("ABORTED") is not True:
        failures.append("no single MATCH_REPORT with ABORTED true")
    elif reports[0].get("EXIT_MSG") != "Shutdown":
        failures.append(f"the aborted report's EXIT_MSG is {reports[0].get('EXIT_MSG')!r}, retail says 'Shutdown'")
    if not [data for data in stats if data.get("QUIT") == 0 and data.get("ABORTED") is True]:
        failures.append("no aborted PLAYER_STATS for the players still in the match")
    # the leaver's statistics come before its PLAYER_DISCONNECT
    kinds = [event.get("TYPE") for event in events]
    if "PLAYER_DISCONNECT" in kinds and "PLAYER_STATS" in kinds \
            and kinds.index("PLAYER_STATS") > kinds.index("PLAYER_DISCONNECT"):
        failures.append("PLAYER_DISCONNECT came before the leaver's PLAYER_STATS")
    return failures


def repair_retail_integers(value):
    """The low 32 bits of every integer, sign-extended.

    Observed with the FnQL Steam provider's retail JSON adapter (0.3.x):
    JsonCpp keeps a 32-bit int in the low half of an 8-byte union, and the
    adapter reads all eight bytes, so retail integers arrive with stale upper
    halves (TIME 11 as 12884901899).  Until the adapter is fixed the probe
    reads them the way JsonCpp stores them.
    """
    if isinstance(value, bool):
        return value
    if isinstance(value, int):
        return ((value & 0xFFFFFFFF) ^ 0x80000000) - 0x80000000
    if isinstance(value, dict):
        return {key: repair_retail_integers(child) for key, child in value.items()}
    if isinstance(value, list):
        return [repair_retail_integers(child) for child in value]
    return value


def host(args, provider: str, retail: Path) -> tuple[list[dict], str]:
    scratch_root = ROOT / ".tmp"
    scratch_root.mkdir(exist_ok=True)
    home = Path(tempfile.mkdtemp(prefix=f"stats-probe-{provider}-", dir=scratch_root))
    try:
        (home / "baseq3").mkdir()
        if provider == "fnql":
            shutil.copy2(args.module, home / "baseq3" / hosted_module_name(args.module))
        journal = home / "baseq3" / JOURNAL
        log_path = home / "baseq3" / "qconsole.log"
        bots = [name.strip() for name in args.bots.split(",") if name.strip()]
        # the factory sets the limits when the map loads, so the probe's own
        # come after; the bots then ready up through the server's allready
        startup = [f"map {args.map} {args.factory}", "wait 40", f"fraglimit {args.fraglimit}",
                   f"capturelimit {args.fraglimit}", f"roundlimit {args.roundlimit}", "timelimit 0"]
        # In the team modes a bot is put on a side when it is added: left to
        # themselves the retail module's bots stay spectators there (observed:
        # four bots, five minutes of Clan Arena, no team change).
        if args.factory in TEAMLESS:
            startup += [f"addbot {name} {args.skill}" for name in bots]
        else:
            startup += [f"addbot {name} {args.skill} {('red', 'blue')[index % 2]}" for index, name in enumerate(bots)]
        startup += ["wait 240", "allready"]
        if args.script:
            startup += [part.strip() for part in args.script.split(";") if part.strip()]
        command = [
            *headless.exe_wrapper(),
            str(args.engine),
            *headless.SERVER,
            "+set", "fs_basepath", str(retail),
            "+set", "fs_homepath", str(home),
            "+set", "logfile", "2",
            "+set", "net_port", str(args.port),
            "+set", "sv_pure", "0",
            "+set", "bot_enable", "1",
            "+set", "vm_gameProvider", provider,
            "+set", "sv_statsJournal", JOURNAL,
            "+set", "sv_hostname", "FnQL stats probe",
            "+set", "g_warmup", "5",
            "+set", "serverstartup", "; ".join(startup),
        ]
        process = subprocess.Popen(command, cwd=Path(args.engine).parent, stdin=subprocess.DEVNULL,
                                   stdout=subprocess.DEVNULL, stderr=subprocess.DEVNULL,
                                   **headless.popen_options())
        started = time.monotonic()
        try:
            while time.monotonic() - started < args.seconds and process.poll() is None:
                time.sleep(0.5)
                # the report closes the match: nothing more to wait for
                if journal.is_file() and b'"TYPE":"MATCH_REPORT"' in journal.read_bytes():
                    time.sleep(1.0)
                    break
        finally:
            if process.poll() is None:
                process.kill()
            process.wait(timeout=10)
        log = log_path.read_text(encoding="utf-8", errors="replace") if log_path.is_file() else ""
        events = []
        if journal.is_file():
            for line in journal.read_text(encoding="utf-8", errors="replace").splitlines():
                if line.strip():
                    events.append(json.loads(line))
        if args.keep:
            print(f"kept {home}")
        return events, log
    finally:
        if not args.keep:
            shutil.rmtree(home, ignore_errors=True)


def summarize(events: list[dict]) -> str:
    counts: dict[str, int] = {}
    for event in events:
        counts[event.get("TYPE", "?")] = counts.get(event.get("TYPE", "?"), 0) + 1
    return ", ".join(f"{kind} {count}" for kind, count in sorted(counts.items())) or "none"


def stable_documents(events: list[dict]) -> dict[str, list]:
    out: dict[str, list] = {}
    for event in events:
        ignored = STABLE.get(event.get("TYPE"))
        if ignored is not None and isinstance(event.get("DATA"), dict):
            out.setdefault(event["TYPE"], []).append(
                {key: value for key, value in event["DATA"].items() if key not in ignored})
    return out


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__.splitlines()[0])
    parser.add_argument("--engine", default=os.environ.get("FNQL_DEDICATED_EXE"),
                        help="x86 FnQL dedicated server (default: $FNQL_DEDICATED_EXE)")
    parser.add_argument("--retail", help="Quake Live install (default: $FNQL_RETAIL_QL_PATH or Steam)")
    parser.add_argument("--module", type=Path, default=ROOT / "build" / "win32-x86" / MODULE_NAME)
    parser.add_argument("--provider", choices=("fnql", "retail"), default="fnql")
    parser.add_argument("--compare", action="store_true", help="host retail and FnQL-game in turn")
    parser.add_argument("--map", default="campgrounds")
    parser.add_argument("--factory", default="ffa")
    parser.add_argument("--bots", default="sarge,ranger,visor,keel")
    parser.add_argument("--skill", type=int, default=4)
    parser.add_argument("--fraglimit", type=int, default=5, help="frag and capture limit that ends the match")
    parser.add_argument("--roundlimit", type=int, default=2)
    parser.add_argument("--seconds", type=float, default=180.0, help="longest wait for the match report")
    parser.add_argument("--port", type=int, default=27993)
    parser.add_argument("--require",
                        default="PLAYER_CONNECT,PLAYER_SWITCHTEAM,PLAYER_KILL,PLAYER_DEATH,PLAYER_STATS,MATCH_STARTED,MATCH_REPORT",
                        help="event types the run must produce")
    parser.add_argument("--script", default="",
                        help="server commands to run after the bots ready up, separated by ';' "
                             "(for example 'wait 700; clientkick 0; wait 80; quit')")
    parser.add_argument("--expect-abort", action="store_true",
                        help="the script kicks a player and shuts the server down mid-match: require the "
                             "leaver's PLAYER_STATS (QUIT 1, RANK -1) and an aborted report")
    parser.add_argument("--dump", type=Path, help="write the journalled events here (one JSON document per line)")
    parser.add_argument("--keep", action="store_true", help="keep the scratch home directory")
    args = parser.parse_args()

    if not args.engine or not Path(args.engine).is_file():
        raise SystemExit("pass --engine <fnql.ded.exe> or set FNQL_DEDICATED_EXE")
    args.engine = Path(args.engine).resolve()
    retail = find_retail_install(args.retail)
    schema = json.loads(SCHEMA.read_text(encoding="utf-8"))
    providers = ["retail", "fnql"] if args.compare else [args.provider]
    if "fnql" in providers and not args.module.is_file():
        raise SystemExit(f"module not found: {args.module} (build it first)")

    failures, runs = [], {}
    for provider in providers:
        events, log = host(args, provider, retail)
        if any(event.get("DATA") is None for event in events):
            # The engine could not read the retail module's JsonCpp objects (no
            # 32-bit Steam provider with the retail JSON adapter): nothing to check.
            print(f"{provider}: the engine published empty envelopes; the retail JSON adapter is unavailable")
            return 77
        if provider == "retail":
            events = [repair_retail_integers(event) for event in events]
        runs[provider] = events
        print(f"{provider}: {len(events)} events ({summarize(events)})")
        if args.dump:
            target = args.dump if len(providers) == 1 else args.dump.with_suffix(f".{provider}{args.dump.suffix}")
            target.write_text("".join(json.dumps(e, sort_keys=True) + "\n" for e in events), encoding="utf-8")
        failures += [f"{provider}: forbidden log line {line!r}" for line in FORBIDDEN if line in log]
        seen = {event.get("TYPE") for event in events}
        failures += [f"{provider}: no {kind} event" for kind in args.require.split(",") if kind and kind not in seen]
        if args.expect_abort:
            failures += [f"{provider}: {failure}" for failure in check_abort(events)]
        found = check(events, schema)
        failures += [f"{provider}: {failure}" for failure in found[:40]]
        if len(found) > 40:
            failures.append(f"{provider}: ... and {len(found) - 40} more")

    if args.compare and not failures:
        retail_docs, fnql_docs = stable_documents(runs["retail"]), stable_documents(runs["fnql"])
        for kind in STABLE:
            if retail_docs.get(kind) != fnql_docs.get(kind):
                failures.append(f"{kind} documents differ:\n    retail {json.dumps(retail_docs.get(kind), sort_keys=True)}"
                                f"\n    fnql   {json.dumps(fnql_docs.get(kind), sort_keys=True)}")

    if failures:
        print(f"stats probe FAILED ({len(failures)}):")
        for failure in failures:
            print(f"  - {failure}")
        return 1
    print("stats probe OK: every document matches the retail schema")
    return 0


if __name__ == "__main__":
    sys.exit(main())
