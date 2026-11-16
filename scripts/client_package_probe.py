#!/usr/bin/env python3
"""An FnQL client joins an FnQL dedicated server over UDP (gate G5, M10).

This is the FnQL <-> FnQL half of the interop matrix, pure and unpure, and
the gate for honest purity of the FnQL-game client modules
(FnQL docs/fnql/GAME_MODULES.md, docs/COMPATIBILITY.md):

  - the server hosts FnQL-game's qagame (or, with --server-provider retail,
    the retail one) on a retail map, optionally with the FnQL-game client
    package (fnql_game_client.pk3) in its home;
  - the client runs vm_cgameProvider/vm_uiProvider as asked, with the
    package, the loose modules, or nothing in its home;
  - the gate checks which cgame the client actually ran for the level, that
    a pure server accepted it (and why), the fnql_game / fnql_cg
    negotiation and its handshake, and that the client stays in the game.

A pure server publishes the packages it accepts in sv_fnqlClientPaks; the
client runs the FnQL-game cgame there only from such a pak, and otherwise
falls back to the retail cgame, which every pure server accepts.  On a
server that does not run FnQL-game's qagame the cgame never answers, so the
server's copy of the client's userinfo must not carry fnql_cg.

Both peers run headless (com_headless 1 and 2) with Steam and the WebUI at
their defaults.  The engine build carries no Steam provider unless one is
staged beside it; without one the server reports Steam unavailable and
admits the ticketless client, which is the honest fallback.  With one, a
client that cannot get a ticket is refused, and the probe reports that.
"""

from __future__ import annotations

import argparse
import os
import re
import secrets
import shutil
import socket
import subprocess
import sys
import tempfile
import time
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
sys.path.insert(0, str(ROOT / "scripts"))
from smoke_dedicated import MODULE_NAME, find_retail_install  # noqa: E402
import headless  # noqa: E402

SKIP = 77
CLIENT_NAME = "M10Probe"
PACKAGE = "fnql_game_client.pk3"


def rcon(port: int, password: str, command: str) -> str:
    payload = b"\xff\xff\xff\xff" + f"rcon {password} {command}\n".encode("ascii")
    replies = []
    with socket.socket(socket.AF_INET, socket.SOCK_DGRAM) as sock:
        sock.settimeout(5.0)
        sock.connect(("127.0.0.1", port))
        sock.send(payload)
        while True:
            try:
                data = sock.recv(65535).decode("latin-1")
            except (socket.timeout, ConnectionResetError):
                break
            replies.append(data.removeprefix("\xff\xff\xff\xffprint\n"))
            sock.settimeout(0.5)
    return "".join(replies)


def read(path: Path) -> str:
    return path.read_text(encoding="utf-8", errors="replace") if path.is_file() else ""


def level_cgame(log: str) -> str | None:
    """The cgame the client ran for the level: the last module that loaded
    before the level's CL_InitCGame ("fnql" or "retail")."""
    init = log.rfind("CL_InitCGame:")
    if init < 0:
        return None
    loads = re.findall(r"Loading dll file (fnql_)?cgame\.", log[:init])
    if not loads:
        return None
    return "fnql" if loads[-1] else "retail"


def stop(process: subprocess.Popen | None) -> None:
    if process is None or process.poll() is not None:
        return
    process.kill()
    try:
        process.wait(timeout=30)
    except subprocess.TimeoutExpired:
        if os.name == "nt":
            subprocess.run(["taskkill", "/PID", str(process.pid), "/T", "/F"], capture_output=True)
        process.wait(timeout=30)


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__.splitlines()[0])
    parser.add_argument("--engine", required=True, help="FnQL dedicated server (fnql.ded.exe)")
    parser.add_argument("--client", required=True, help="FnQL client (fnql.exe)")
    parser.add_argument("--retail", help="Quake Live install (default: $FNQL_RETAIL_QL_PATH or Steam)")
    parser.add_argument("--module", type=Path, required=True, help="FnQL-game qagame")
    parser.add_argument("--cgame", type=Path, required=True, help="FnQL-game cgame")
    parser.add_argument("--ui", type=Path, required=True, help="FnQL-game ui")
    parser.add_argument("--package", type=Path, required=True, help="FnQL-game client package (pk3)")
    # "default" sets no provider cvar, so the engine's own default decides:
    # auto since M12, which takes FnQL-game's modules (they claim retail wire
    # parity).  The release-default rows run that way.
    parser.add_argument("--server-provider", choices=["fnql", "retail", "default"], default="fnql")
    parser.add_argument("--pure", choices=["0", "1"], default="1")
    parser.add_argument("--server-package", action="store_true",
                        help="load the client package on the server")
    parser.add_argument("--server-client-modules", choices=["0", "1"], default="1",
                        help="the server's sv_fnqlClientModules")
    parser.add_argument("--client-modules", choices=["package", "loose", "release", "none"],
                        default="package",
                        help="how the client holds the FnQL-game cgame and ui "
                             "(release: the loose modules and the package, as FnQL releases ship them)")
    parser.add_argument("--cgame-provider", choices=["retail", "fnql", "auto", "default"], default="fnql")
    parser.add_argument("--ui-provider", choices=["retail", "fnql", "auto", "default"], default="fnql")
    parser.add_argument("--expect-cgame", choices=["fnql", "retail"], required=True)
    parser.add_argument("--expect-ui", choices=["fnql", "retail"])
    parser.add_argument("--expect-package", action="store_true",
                        help="require the server to accept the client by the client package")
    parser.add_argument("--expect-handshake", choices=["yes", "no"], required=True)
    parser.add_argument("--map", default="campgrounds")
    parser.add_argument("--factory", default="ffa")
    parser.add_argument("--port", type=int, default=27971)
    parser.add_argument("--seconds", type=float, default=8.0,
                        help="how long the client must stay in after the level starts")
    parser.add_argument("--timeout", type=float, default=150.0)
    parser.add_argument("--keep", action="store_true")
    args = parser.parse_args()

    for name in ("module", "cgame", "ui", "package"):
        if not getattr(args, name).is_file():
            parser.error(f"--{name} {getattr(args, name)} does not exist")
    engine = Path(args.engine).resolve()
    client = Path(args.client).resolve()
    if not engine.is_file() or not client.is_file():
        print("client package probe: engine or client binary missing; skipping")
        return SKIP
    try:
        retail = find_retail_install(args.retail)
    except SystemExit as error:
        print(f"client package probe: {error}; skipping")
        return SKIP

    scratch = ROOT / ".tmp"
    scratch.mkdir(exist_ok=True)
    root = Path(tempfile.mkdtemp(prefix="client-package-probe-", dir=scratch))
    server_home, client_home = root / "server", root / "client"
    server = client_process = None
    failures: list[str] = []
    replies: dict[str, str] = {}
    port = args.port
    password = secrets.token_hex(16)
    try:
        server_game = server_home / "baseq3"
        server_game.mkdir(parents=True)
        shutil.copy2(args.module, server_game / MODULE_NAME)
        if args.server_package:
            shutil.copy2(args.package, server_game / PACKAGE)
        client_game = client_home / "baseq3"
        client_game.mkdir(parents=True)
        if args.client_modules in ("package", "release"):
            shutil.copy2(args.package, client_game / PACKAGE)
        if args.client_modules in ("loose", "release"):
            shutil.copy2(args.cgame, client_game / args.cgame.name)
            shutil.copy2(args.ui, client_game / args.ui.name)

        def provider(cvar: str, value: str) -> list[str]:
            return [] if value == "default" else ["+set", cvar, value]

        server = subprocess.Popen(
            [str(engine), *headless.SERVER, "+set", "fs_basepath", str(retail),
             "+set", "fs_homepath", str(server_home), "+set", "logfile", "2",
             "+set", "developer", "1", "+set", "net_port", str(args.port),
             "+set", "sv_pure", args.pure, "+set", "sv_serverType", "1",
             "+set", "sv_fnqlClientModules", args.server_client_modules,
             *provider("vm_gameProvider", args.server_provider),
             "+set", "rconPassword", password, "+set", "sv_hostname", "FnQL-game M10 probe",
             "+set", "serverstartup", f"map {args.map} {args.factory}"],
            cwd=server_home, stdin=subprocess.DEVNULL, stdout=subprocess.DEVNULL,
            stderr=subprocess.DEVNULL, **headless.popen_options())
        server_log_path = server_game / "qconsole.log"
        deadline = time.monotonic() + args.timeout
        # the level runs once the server answers rcon status with its map; the
        # engine binds the next free port while net_port is taken
        while time.monotonic() < deadline and server.poll() is None:
            opened = re.findall(r"Opening IP socket: [^\s:]+:(\d+)", read(server_log_path))
            if opened:
                port = int(opened[-1])
                if "map:" in rcon(port, password, "status"):
                    break
            time.sleep(0.5)
        if server.poll() is not None:
            failures.append("the server exited during start-up")

        client_process = subprocess.Popen(
            [str(client), *headless.CLIENT, "+set", "fs_basepath", str(retail),
             "+set", "fs_homepath", str(client_home), "+set", "logfile", "2",
             "+set", "developer", "1", "+set", "name", CLIENT_NAME,
             "+set", "r_fullscreen", "0", "+set", "r_mode", "-1",
             "+set", "r_customwidth", "960", "+set", "r_customheight", "600",
             "+set", "in_mouse", "0", "+set", "in_nograb", "1", "+set", "s_initsound", "0",
             "+set", "cl_allowDownload", "0", "+set", "net_port", str(args.port + 100),
             *provider("vm_cgameProvider", args.cgame_provider),
             *provider("vm_uiProvider", args.ui_provider),
             "+connect", f"127.0.0.1:{port}"],
            cwd=client_home, stdin=subprocess.DEVNULL, stdout=subprocess.DEVNULL,
            stderr=subprocess.DEVNULL, **headless.popen_options())
        client_log_path = client_game / "qconsole.log"

        active_at = None
        while time.monotonic() < deadline and not failures:
            if client_process.poll() is not None:
                failures.append("the client exited")
                break
            # the engine's own line (developer 1), whichever qagame runs
            if active_at is None and f"Going from CS_PRIMED to CS_ACTIVE for {CLIENT_NAME}" in read(server_log_path):
                active_at = time.monotonic()
            if active_at is not None and time.monotonic() - active_at >= args.seconds:
                break
            time.sleep(0.5)
        if active_at is None and not failures:
            failures.append("the client never entered the game")
        if server.poll() is None:
            for command in ("status", f"dumpuser {CLIENT_NAME}", "sv_fnqlClientPaks", "serverinfo"):
                replies[command] = rcon(port, password, command)
    finally:
        stop(client_process)
        if server is not None and server.poll() is None:
            try:
                rcon(port, password, "quit")
                server.wait(timeout=10)
            except Exception:  # noqa: BLE001 - teardown only
                pass
        stop(server)

    server_log = read(server_home / "baseq3" / "qconsole.log")
    client_log = read(client_home / "baseq3" / "qconsole.log")
    try:
        if "A valid platform authentication ticket is required" in client_log:
            # the server's honest decision about a client that holds no
            # Steam ticket; it says nothing about module selection or purity
            print("client package probe: the server's Steam provider requires a ticket this client "
                  "could not get (an engine build without a staged Steam provider admits it); skipping")
            return SKIP
        if "Unpure client detected" in server_log + client_log:
            failures.append("the server dropped the client as unpure")
        if CLIENT_NAME not in replies.get("status", ""):
            failures.append("the client is not in the server's status")
        cgame = level_cgame(client_log)
        if cgame != args.expect_cgame:
            failures.append(f"the level ran the {cgame} cgame, expected {args.expect_cgame}")
        if args.expect_ui:
            ui_loads = re.findall(r"Loading dll file (fnql_)?ui\.", client_log)
            ui = ("fnql" if ui_loads[-1] else "retail") if ui_loads else None
            if ui != args.expect_ui:
                failures.append(f"the client ran the {ui} ui, expected {args.expect_ui}")
        published = re.search(r'"sv_fnqlClientPaks" is:"([^"^]*)', replies.get("sv_fnqlClientPaks", ""))
        published = published.group(1) if published else ""
        should_publish = args.pure == "1" and args.server_package and args.server_client_modules == "1"
        if bool(published) != should_publish:
            failures.append(f"sv_fnqlClientPaks is {published!r}; expected it "
                            + ("published" if should_publish else "empty"))
        accepted = "is pure with the FnQL-game client package" in server_log
        if args.expect_package != accepted:
            failures.append("the server " + ("did not accept" if args.expect_package else "accepted")
                            + " the client by the FnQL-game client package")
        if args.pure == "1" and args.expect_cgame == "fnql" and not args.expect_package:
            failures.append("bad expectation: a pure server accepts an FnQL-game cgame only by its package")
        # under the engine default (auto) the server hosts FnQL-game's qagame
        server_runs_fnql = args.server_provider in ("fnql", "default")
        announced = "fnql_game" in replies.get("serverinfo", "")
        if announced != server_runs_fnql:
            failures.append(f"serverinfo fnql_game announced: {announced}")
        answered = "fnql_cg" in replies.get(f"dumpuser {CLIENT_NAME}", "")
        should_answer = server_runs_fnql and args.expect_cgame == "fnql"
        if answered != should_answer:
            failures.append(f"the server's copy of the userinfo {'has' if answered else 'lacks'} fnql_cg")
        hello = re.search(r"FnQL-game server (\S+): extensions (\d+)", client_log)
        confirmed = re.search(r"FnQL extensions confirmed for client \d+ \(mask (\d+)\)", server_log)
        if args.expect_handshake == "yes":
            if not hello or hello.group(2) != "1":
                failures.append("the cgame did not complete the FnQL extension handshake")
            if not confirmed or confirmed.group(1) != "1":
                failures.append("the server did not confirm the extension channel")
        elif hello or confirmed or "fnql_hello" in client_log:
            failures.append("an extension handshake happened where none may")
        for line in ("ERROR:", "********************"):
            if line in client_log or line in server_log:
                failures.append(f"forbidden {line!r} in a log")

        if failures:
            print("client package probe FAILED:")
            for failure in failures:
                print(f"  - {failure}")
            print("--- client log tail ---")
            print(client_log[-4000:])
            print("--- server log tail ---")
            print(server_log[-4000:])
            print("--- rcon ---")
            for command, reply in replies.items():
                print(f"> {command}\n{reply}")
            return 1
        print(f"client package probe OK: {args.expect_cgame} cgame on a "
              f"{'pure' if args.pure == '1' else 'unpure'} server running the {args.server_provider} qagame"
              + (", accepted by the client package" if accepted else "")
              + (f", handshake with server {hello.group(1)}" if hello else ""))
        return 0
    finally:
        if args.keep:
            print(f"kept {root}")
        else:
            shutil.rmtree(root, ignore_errors=True)


if __name__ == "__main__":
    sys.exit(main())
