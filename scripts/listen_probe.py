#!/usr/bin/env python3
"""Run the retail cgame against FnQL-game's qagame in one FnQL client.

First interoperability gate (G5a): an FnQL listen server loads FnQL-game's
qagame while the client half runs the retail cgamex86.dll from the user's
install.  The retail cgame is exactly what a retail Quake Live client runs,
so everything it receives -- gamestate, configstrings, snapshots, server
commands -- crosses the same contract a retail peer sees.  The probe adds a
bot, takes a screenshot (kept with --keep for inspection) and requires:

  - qagame provided by FnQL-game and cgame by the retail module;
  - the retail cgame initialized and the client reached CS_ACTIVE;
  - no engine or cgame error.

With --play the client also plays the mode: it joins a team, the bot takes
the other side, the retail admin `allready` ends the ready-up warmup and the
match runs for the given time, so the retail cgame follows the mode's own
flow (the match-start map_restart, rounds, freezing, conversion,
objectives).  --expect-cs then checks the live configstrings.  That phase is
driven over authenticated loopback rcon in real time, because the match start
appends map_restart to the command buffer and a scripted `wait` would hold it
there.

The client runs windowless (com_headless 2) with sound off, and with Steam
and the WebUI at their defaults.
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

REQUIRED = [
    "qagame : native " + MODULE_NAME + " (FnQL-game ",
    "cgame : native cgamex86.dll (unidentified)",
    "CL_InitCGame:",
    "Going from CS_PRIMED to CS_ACTIVE",
    "Sarge^7 connected",  # retail announces connects only after the first five seconds
    "probe: done",
]
FORBIDDEN = ["ERROR:", "********************", "Rejected DLL", "loading the retail module instead"]


def rcon(port: int, password: str, command: str) -> str:
    """Run `command` on the listen server's console and return its output."""
    payload = b"\xff\xff\xff\xff" + f"rcon {password} {command}\n".encode("ascii")
    replies = []
    with socket.socket(socket.AF_INET, socket.SOCK_DGRAM) as sock:
        # A loaded machine can take seconds to run a frame; only the packets
        # that continue a long reply follow quickly.
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


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__.splitlines()[0])
    parser.add_argument("--client", default=os.environ.get("FNQL_CLIENT_EXE"),
                        help="x86 FnQL client (default: $FNQL_CLIENT_EXE)")
    parser.add_argument("--retail", help="Quake Live install (default: $FNQL_RETAIL_QL_PATH or Steam)")
    parser.add_argument("--module", type=Path, default=ROOT / "build" / "win32-x86" / MODULE_NAME)
    parser.add_argument("--map", default="campgrounds")
    parser.add_argument("--factory", default="ffa")
    parser.add_argument("--factories", type=Path, help="additional hand-authored factory fixture")
    parser.add_argument("--expect-gametype", type=int, choices=range(13),
                        help="require the selected retail gametype value")
    parser.add_argument("--expect-factory", help="require the published g_factory")
    parser.add_argument("--provider", choices=["fnql", "retail"], default="fnql",
                        help="qagame provider; retail runs the same checks against the retail qagame, "
                             "which is how the --expect-cs values were taken")
    parser.add_argument("--cheats", action="store_true",
                        help="use devmap for fixtures outside retail arena mode metadata")
    parser.add_argument("--play", type=float, default=0, metavar="SECONDS",
                        help="join a team, give the bot the other side, force ready-up and play this long "
                             "before the checks (0: startup only)")
    parser.add_argument("--team", default="f", help="team the client joins with --play (f, r or b)")
    parser.add_argument("--bot-team", default="", help="team given to the bot with --play (red or blue)")
    parser.add_argument("--expect-cs", nargs=2, action="append", default=[], metavar=("INDEX", "REGEX"),
                        help="after --play, require configstring INDEX to match REGEX (repeatable)")
    parser.add_argument("--port", type=int, default=27993)
    parser.add_argument("--timeout", type=float, default=150.0)
    parser.add_argument("--keep", action="store_true", help="keep the scratch home (log, screenshot)")
    args = parser.parse_args()

    if not args.client or not Path(args.client).is_file():
        raise SystemExit("pass --client <fnql.exe> or set FNQL_CLIENT_EXE")
    client = Path(args.client).resolve()
    retail = find_retail_install(args.retail)
    if args.factories and not args.factories.is_file():
        parser.error("--factories needs an existing file")

    scratch = ROOT / ".tmp"
    scratch.mkdir(exist_ok=True)
    home = Path(tempfile.mkdtemp(prefix="listen-probe-", dir=scratch))
    try:
        game_dir = home / "baseq3"
        game_dir.mkdir()
        shutil.copy2(args.module, game_dir / MODULE_NAME)
        if args.factories:
            scripts_dir = game_dir / "scripts"
            scripts_dir.mkdir()
            shutil.copy2(args.factories, scripts_dir / "listen.factories")
        map_command = f"{'devmap' if args.cheats else 'map'} {args.map} {args.factory}"
        tail = ["screenshotJPEG", "wait 60", "vminfo", "g_gametype", "g_factory", "echo probe: done", "quit"]
        rcon_password = secrets.token_hex(16)
        if args.play:
            # Played modes are driven in real time over authenticated
            # loopback rcon instead of `wait`: the match start appends
            # map_restart to the command buffer, and a pending `wait` would
            # hold it there for the rest of the script.
            script = [map_command]
        else:
            script = [map_command, "wait 1000", "addbot sarge 3", "wait 400", *tail]
        (game_dir / "probe.cfg").write_text("\n".join(script + [""]), encoding="ascii")
        # Windowless client with Steam and the WebUI at their defaults (on),
        # as in retail (AGENTS.md rules 13 and 14).
        command = [str(client), *headless.CLIENT,
                   "+set", "fs_basepath", str(retail), "+set", "fs_homepath", str(home),
                   "+set", "r_fullscreen", "0", "+set", "r_mode", "-1",
                   "+set", "r_customwidth", "1024", "+set", "r_customheight", "640",
                   "+set", "in_mouse", "0", "+set", "in_nograb", "1", "+set", "in_joystick", "0",
                   "+set", "s_initsound", "0", "+set", "logfile", "2", "+set", "developer", "1",
                   "+set", "cl_allowDownload", "0", "+set", "net_port", str(args.port),
                   "+set", "vm_gameProvider", args.provider]
        if args.play:
            # sv_serverType 1 admits loopback UDP, which retail's 0 ignores.
            command += ["+set", "sv_serverType", "1", "+set", "rconPassword", rcon_password]
        command += ["+exec", "probe.cfg"]
        process = subprocess.Popen(command, cwd=home, stdin=subprocess.DEVNULL,
                                   stdout=subprocess.DEVNULL, stderr=subprocess.DEVNULL,
                                   **headless.popen_options())
        log_path = game_dir / "qconsole.log"

        def read_log() -> str:
            return log_path.read_text(encoding="utf-8", errors="replace") if log_path.is_file() else ""

        deadline = time.monotonic() + args.timeout
        replies: list[str] = []
        configstrings = ""
        failures: list[str] = []
        if args.play:
            port = args.port

            def console(line: str, settle: float = 0.0) -> None:
                replies.append(rcon(port, rcon_password, line))
                if settle:
                    time.sleep(settle)

            while (process.poll() is None and time.monotonic() < deadline
                   and "Going from CS_PRIMED to CS_ACTIVE" not in read_log()):
                time.sleep(0.5)
            # The engine moves to the next port while one is taken (another
            # probe's client, say); the last socket it opened is the bound one.
            opened = re.findall(r"Opening IP socket: [^\s:]+:(\d+)", read_log())
            if opened:
                port = int(opened[-1])
            if process.poll() is None and "Going from CS_PRIMED to CS_ACTIVE" in read_log():
                # The client is a player and the bot its opponent, so the
                # mode's own rules run under the retail cgame: rounds,
                # freezing, conversion, objectives.  allready is the retail
                # admin command that ends the ready-up warmup.  Commands the
                # engine does not know are forwarded as the local client's.
                # Retail announces connects only after the level's first five
                # seconds, and REQUIRED expects the bot's announcement.
                time.sleep(6)
                console(f"team {args.team}", 1)
                console(f"addbot sarge 3 {args.bot_team}".rstrip(), 6)
                console("allready", args.play)
                configstrings = rcon(port, rcon_password, "configstrings")
                for line in tail:
                    if line.startswith("wait "):
                        time.sleep(1)
                    elif line == "quit":
                        console(line)
                    else:
                        console(line, 0.2)
            else:
                failures.append("the client never became active")
        while process.poll() is None and time.monotonic() < deadline:
            time.sleep(0.5)
        timed_out = process.poll() is None
        if timed_out:
            process.kill()
            try:
                process.wait(timeout=30)
            except subprocess.TimeoutExpired:
                if os.name == "nt":
                    subprocess.run(["taskkill", "/PID", str(process.pid), "/T", "/F"], capture_output=True)
                process.wait(timeout=30)
        process.wait()

        # rcon output is redirected to its replies instead of the log.
        log = read_log() + "\n" + "\n".join(replies)
        if args.keep and replies:
            (home / "rcon-replies.txt").write_text("\n".join(replies + [configstrings]), encoding="utf-8")
        failures += ["client did not quit before the timeout"] if timed_out else []
        required = REQUIRED if args.provider == "fnql" else [
            "qagame : native qagamex86.dll (unidentified)", *REQUIRED[1:]]
        if args.play:
            # The announcement waits for the level's first five seconds, which
            # a loaded machine can still be inside; the bot's player
            # configstring is checked after play instead.
            required = [line for line in required if line != "Sarge^7 connected"]
        failures += [f"missing: {line!r}" for line in required if line not in log]
        failures += [f"forbidden: {line!r}" for line in FORBIDDEN if line in log]
        if args.expect_gametype is not None:
            selected = re.search(r'"g_gametype" is:"([^"\^]*)', log)
            if not selected or selected.group(1) != str(args.expect_gametype):
                failures.append(f"expected g_gametype {args.expect_gametype}, got "
                                + (selected.group(1) if selected else "no cvar report"))
        if args.expect_factory is not None:
            selected = re.search(r'"g_factory" is:"([^"\^]*)', log)
            if not selected or selected.group(1) != args.expect_factory:
                failures.append(f"expected g_factory {args.expect_factory!r}, got "
                                + (repr(selected.group(1)) if selected else "no cvar report"))
        if args.play:
            strings = {int(m.group(1)): m.group(2)
                       for m in re.finditer(r"(?m)^\s*(\d+): (.*?)\r?$", configstrings)}
            if not strings:
                failures.append("the client printed no configstrings after play")
            # CS_PLAYERS 529 is the local client (slot 0), 530 the bot (slot 1).
            if not re.search(r"^n\\Sarge\\.*\\skill\\", strings.get(530, "")):
                failures.append(f"no bot in player configstring 530: {strings.get(530)!r}")
            for index, pattern in args.expect_cs:
                value = strings.get(int(index, 0))
                if value is None or not re.search(pattern, value):
                    failures.append(f"configstring {index}: {value!r} does not match {pattern!r}")
        shots = sorted((game_dir / "screenshots").glob("*.jpg")) if (game_dir / "screenshots").is_dir() else []
        if not shots:
            failures.append("no screenshot was written")

        if failures:
            print("listen probe FAILED:")
            for failure in failures:
                print(f"  - {failure}")
            print("--- log tail ---")
            print(log[-6000:])
            return 1
        owner = "FnQL-game" if args.provider == "fnql" else "the retail"
        print(f"listen probe OK: retail cgame ran against {owner} qagame on {args.map} ({args.factory})"
              + (f", playing {args.play:g}s on team {args.team}" if args.play else ""))
        if args.keep:
            print(f"  screenshot: {shots[-1]}")
        return 0
    finally:
        if args.keep:
            print(f"kept {home}")
        else:
            shutil.rmtree(home, ignore_errors=True)


if __name__ == "__main__":
    sys.exit(main())
