#!/usr/bin/env python3
"""An FnQL client joins a server run by the retail engine (gate G5, M10).

The FnQL client <-> retail server half of the interop matrix, pure and
unpure, and the drop-in check for FnQL-game's qagame.  The user's retail
quakelive_steam.exe, launched through Steam, hosts a listen server on a
retail map (`+map`), from a scratch base path whose bin.pk3 is the retail one
with, under --qagame, qagamex86.dll replaced by FnQL-game's module; the
install is never touched (retail_client_probe.py explains the scratch base
path).  An FnQL client then connects over UDP with vm_cgameProvider and
vm_uiProvider as asked, and the gate checks:

  - the retail server admitted the FnQL client and kept it in (its own
    status, over rcon);
  - under --qagame, the retail process loaded FnQL-game's qagame from the
    scratch home (by SHA-256);
  - which cgame the FnQL client ran: a retail pure server never publishes
    sv_fnqlClientPaks, so there it must be the retail cgame, while an
    unpure one may run FnQL-game's;
  - the fnql_game negotiation: only FnQL-game's qagame announces it, and
    only an FnQL-game cgame answers.

The retail client cannot run windowless: its windows are hidden as they
appear, and it runs only with FNQL_ALLOW_RETAIL_CLIENT=1 (AGENTS.md).  The
retail engine authenticates remote players through Steam; an FnQL client
that cannot get a ticket is refused, and the probe reports the retail
server's decision instead of passing.  Skips (77) as retail_client_probe.py
does.
"""

from __future__ import annotations

import argparse
import os
import re
import secrets
import shutil
import subprocess
import sys
import tempfile
import time
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
sys.path.insert(0, str(ROOT / "scripts"))
from smoke_dedicated import find_retail_install  # noqa: E402
import headless  # noqa: E402
from retail_client_probe import (RETAIL_APPID, RETAIL_CLIENT, SKIP, client_pids, kill_tree,  # noqa: E402
                                 loaded_modules, restore_files, retail_files, scratch_basepath,
                                 sha256, steam_exe)
from client_package_probe import level_cgame  # noqa: E402

CLIENT_NAME = "M10Probe"


def read(path: Path) -> str:
    return path.read_text(encoding="utf-8", errors="replace") if path.is_file() else ""


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__.splitlines()[0])
    parser.add_argument("--client", help="FnQL client (fnql.exe) that joins; without it, only the "
                                           "--qagame drop-in is checked (the host's own player)")
    parser.add_argument("--retail", help="Quake Live install (default: $FNQL_RETAIL_QL_PATH or Steam)")
    parser.add_argument("--qagame", type=Path, help="FnQL-game qagame to host in the retail engine")
    parser.add_argument("--package", type=Path, required=True, help="FnQL-game client package")
    parser.add_argument("--pure", choices=["0", "1"], default="1")
    parser.add_argument("--cgame-provider", choices=["retail", "fnql", "auto"], default="fnql")
    parser.add_argument("--ui-provider", choices=["retail", "fnql", "auto"], default="fnql")
    parser.add_argument("--expect-cgame", choices=["fnql", "retail"], required=True)
    parser.add_argument("--expect-handshake", choices=["yes", "no"], required=True)
    parser.add_argument("--map", default="campgrounds")
    parser.add_argument("--factory", default="ffa")
    parser.add_argument("--port", type=int, default=27986)
    parser.add_argument("--seconds", type=float, default=15.0)
    parser.add_argument("--timeout", type=float, default=180.0)
    parser.add_argument("--keep", action="store_true")
    args = parser.parse_args()

    if os.name != "nt":
        print("the retail client is Windows only; skipping")
        return SKIP
    if not headless.retail_client_allowed():
        print("launching the retail client needs FNQL_ALLOW_RETAIL_CLIENT=1; skipping")
        return SKIP
    client = Path(args.client).resolve() if args.client else None
    if (client and not client.is_file()) or not args.package.is_file() or (
            args.qagame and not args.qagame.is_file()):
        raise SystemExit("--client, --package and --qagame must exist")
    if client is None and not args.qagame:
        parser.error("without --client the probe only checks the --qagame drop-in")
    try:
        retail = find_retail_install(args.retail)
    except SystemExit:
        print("no retail Quake Live install; skipping")
        return SKIP
    steam = steam_exe()
    if steam is None:
        print("Steam is not running; skipping")
        return SKIP
    if client_pids():
        print("a retail client is already running; skipping so it is left alone")
        return SKIP

    scratch = ROOT / ".tmp"
    scratch.mkdir(exist_ok=True)
    root = Path(tempfile.mkdtemp(prefix="retail-server-", dir=scratch))
    retail_home = root / "retail-home"
    retail_base = root / "retail-base"
    fnql_home = root / "fnql-client"
    saved = retail_files(retail)
    started_at = time.time()
    password = secrets.token_hex(16)
    hosts: set[int] = set()
    fnql = None
    failures: list[str] = []
    hider = headless.RetailWindowHider(RETAIL_CLIENT)
    hider.__enter__()
    try:
        replaced = {"qagamex86.dll": args.qagame} if args.qagame else {}
        scratch_basepath(retail_base, retail, replaced)
        (retail_home / "baseq3").mkdir(parents=True)
        (fnql_home / "baseq3").mkdir(parents=True)
        shutil.copy2(args.package, fnql_home / "baseq3" / args.package.name)

        subprocess.run([str(steam), "-applaunch", RETAIL_APPID,
                        "+set", "fs_basepath", str(retail_base), "+set", "fs_homepath", str(retail_home),
                        "+set", "r_fullscreen", "0", "+set", "r_mode", "-1",
                        "+set", "r_customwidth", "960", "+set", "r_customheight", "540",
                        "+set", "in_mouse", "0", "+set", "in_nograb", "1", "+set", "com_ignoreCrash", "1",
                        "+set", "logfile", "2", "+set", "developer", "1",
                        "+set", "sv_serverType", "1", "+set", "net_port", str(args.port),
                        "+set", "sv_pure", args.pure, "+set", "rconPassword", password,
                        "+map", f"{args.map}", f"{args.factory}"],
                       stdin=subprocess.DEVNULL, stdout=subprocess.DEVNULL, stderr=subprocess.DEVNULL,
                       timeout=60)
        deadline = time.monotonic() + args.timeout
        while not hosts and time.monotonic() < deadline:
            hosts = client_pids()
            time.sleep(0.5)
        if not hosts:
            failures.append("Steam did not start the retail client")
        # The retail engine answers no UDP rcon (its remote console is ZMQ),
        # so the host's own console log (developer 1) is the evidence: the
        # listen server is up once its local player is active.
        host_log = retail_home / "baseq3" / "qconsole.log"
        while not failures and time.monotonic() < deadline:
            if not hosts & client_pids():
                failures.append("the retail client exited before hosting")
                break
            if re.search(r"Going from CS_PRIMED to CS_ACTIVE for (?!" + CLIENT_NAME + ")", read(host_log)):
                break
            time.sleep(1)
        else:
            if not failures:
                failures.append("the retail listen server never started")

        if not failures and client is None:
            # drop-in only: the retail engine keeps running FnQL-game's
            # qagame with its own player in the game
            stay = time.monotonic() + args.seconds
            while time.monotonic() < stay and hosts & client_pids():
                time.sleep(1)
            if not hosts & client_pids():
                failures.append("the retail host exited while running the game")
        if not failures and args.qagame:
            wanted = sha256(args.qagame)
            loaded = [p for pid in hosts for p in loaded_modules(pid, "qagamex86.dll")]
            if not any(p.is_file() and p.resolve().is_relative_to(retail_home.resolve())
                       and sha256(p) == wanted for p in loaded):
                failures.append("the retail engine did not load FnQL-game's qagame from its home"
                                + (f" (loaded: {', '.join(map(str, loaded))})" if loaded else ""))
        if not failures and client is not None:
            fnql = subprocess.Popen(
                [str(client), *headless.CLIENT, "+set", "fs_basepath", str(retail),
                 "+set", "fs_homepath", str(fnql_home), "+set", "logfile", "2",
                 "+set", "developer", "1", "+set", "name", CLIENT_NAME,
                 "+set", "r_fullscreen", "0", "+set", "r_mode", "-1",
                 "+set", "r_customwidth", "960", "+set", "r_customheight", "600",
                 "+set", "in_mouse", "0", "+set", "in_nograb", "1", "+set", "s_initsound", "0",
                 "+set", "cl_allowDownload", "0", "+set", "net_port", str(args.port + 100),
                 "+set", "vm_cgameProvider", args.cgame_provider,
                 "+set", "vm_uiProvider", args.ui_provider,
                 "+connect", f"127.0.0.1:{args.port}"],
                cwd=fnql_home, stdin=subprocess.DEVNULL, stdout=subprocess.DEVNULL,
                stderr=subprocess.DEVNULL, **headless.popen_options())
            fnql_log = fnql_home / "baseq3" / "qconsole.log"
            began = None
            while time.monotonic() < deadline:
                if fnql.poll() is not None:
                    failures.append("the FnQL client exited")
                    break
                if began is None and f"Going from CS_PRIMED to CS_ACTIVE for {CLIENT_NAME}" in read(host_log):
                    began = time.monotonic()
                if began is not None and time.monotonic() - began >= args.seconds:
                    break
                time.sleep(1)
            if began is None and not failures:
                failures.append("the FnQL client never entered the retail server's game")
            if not hosts & client_pids():
                failures.append("the retail host exited during the game")
    finally:
        if fnql is not None and fnql.poll() is None:
            fnql.kill()
            fnql.wait(timeout=30)
        for pid in hosts:
            kill_tree(pid)
        gone_by = time.monotonic() + 30
        while hosts & client_pids() and time.monotonic() < gone_by:
            time.sleep(0.5)
        time.sleep(1)
        hider.__exit__(None, None, None)
        logs = [retail / "baseq3" / "qconsole.log"] + [
            a / "baseq3" / "qconsole.log" for a in retail.iterdir() if re.fullmatch(r"\d{17}", a.name)]
        retail_log = read(retail_home / "baseq3" / "qconsole.log")
        for path in logs:
            if path.is_file() and path.stat().st_mtime >= started_at and not retail_log:
                retail_log = read(path)
        for created in logs:
            if created not in saved and created.is_file():
                created.unlink()
        restored = restore_files(saved)
        if restored:
            print("restored retail files: " + ", ".join(str(p.relative_to(retail)) for p in restored))

    fnql_log = read(fnql_home / "baseq3" / "qconsole.log")
    try:
        if client is None:
            if "ERROR:" in retail_log or "********************" in retail_log:
                failures.append("the retail host logged an error")
            if failures:
                print("retail server probe FAILED:")
                for failure in failures:
                    print(f"  - {failure}")
                print("--- retail host log tail ---")
                print(retail_log[-4000:])
                return 1
            print(f"retail server probe OK: the retail engine hosted FnQL-game's qagame on {args.map} "
                  f"({args.factory}, sv_pure {args.pure}) with its own player for {args.seconds:.0f}s")
            return 0
        # The retail engine authenticates remote players through Steam.  Its
        # refusal of a client that holds no ticket is its decision to report,
        # not a pass and not an FnQL failure.
        ticketless = "No Steam auth token" in fnql_log and (
            f"Going from CS_PRIMED to CS_ACTIVE for {CLIENT_NAME}" not in retail_log)
        if ticketless:
            print("retail server probe: the retail server refused the FnQL client with 'No Steam auth "
                  "token.' (this client could not get a Steam ticket); skipping")
            return SKIP
        entered = f"Going from CS_PRIMED to CS_ACTIVE for {CLIENT_NAME}" in retail_log
        refused = re.search(r"(?m)^.*(ticket|Steam|authenticat|pure).*(required|denied|failed|invalid|detected).*$",
                            fnql_log)
        if refused and not entered:
            failures.append(f"the retail server refused the FnQL client: {refused.group(0).strip()!r}")
        left = re.search(rf"(?mi)^.*{CLIENT_NAME}.*(disconnected|timed out|dropped|kicked|unpure).*$", retail_log)
        if left:
            failures.append(f"the retail server let the FnQL client go: {left.group(0).strip()!r}")
        cgame = level_cgame(fnql_log)
        if cgame != args.expect_cgame:
            failures.append(f"the FnQL client ran the {cgame} cgame, expected {args.expect_cgame}")
        if args.pure == "1" and args.expect_cgame == "fnql":
            failures.append("bad expectation: a retail pure server never accepts an FnQL-game cgame")
        # the host logs every userinfo its clients send (developer 1)
        answered = bool(re.search(rf'clientCommand: {CLIENT_NAME} : \d+ : userinfo "[^"]*\\fnql_cg\\', retail_log))
        hello = re.search(r"FnQL-game server (\S+): extensions (\d+)", fnql_log)
        if args.expect_handshake == "yes" and not (answered and hello):
            failures.append("the FnQL extension handshake did not complete through the retail engine")
        if args.expect_handshake == "no" and (answered or hello):
            failures.append("FnQL extension negotiation reached a server that did not announce it")
        if "ERROR:" in fnql_log or "********************" in fnql_log:
            failures.append("the FnQL client logged an error")
        if failures:
            print("retail server probe FAILED:")
            for failure in failures:
                print(f"  - {failure}")
            print("--- FnQL client log tail ---")
            print(fnql_log[-4000:])
            print("--- retail host log tail ---")
            print(retail_log[-4000:])
            return 1
        print(f"retail server probe OK: the FnQL client ran the {cgame} cgame on a "
              f"{'pure' if args.pure == '1' else 'unpure'} retail-engine server hosting "
              + ("FnQL-game's qagame" if args.qagame else "the retail qagame")
              + (f", extension handshake with {hello.group(1)}" if hello else ""))
        return 0
    finally:
        if args.keep:
            (root / "fnql-client.log").write_text(fnql_log, encoding="utf-8")
            (root / "retail-host.log").write_text(retail_log, encoding="utf-8")
            print(f"kept {root}")
        else:
            shutil.rmtree(root, ignore_errors=True)


if __name__ == "__main__":
    sys.exit(main())
