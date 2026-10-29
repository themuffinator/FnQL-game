#!/usr/bin/env python3
"""Run the pmove lockstep harness against the retail qagame (gate G3).

The harness (tests/lockstep/pmove_lockstep.c) calls the retail Pmove at a
fixed address, so it only runs against the module it was written for: the
runner extracts qagamex86.dll from the retail install's baseq3/bin.pk3 into
a scratch directory and checks its SHA-256 first.  Without a retail install
or with a different module it skips (exit 77).
"""

from __future__ import annotations

import argparse
import hashlib
import shutil
import subprocess
import sys
import tempfile
import zipfile
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
sys.path.insert(0, str(ROOT / "scripts"))
from smoke_dedicated import find_retail_install  # noqa: E402

# Quake Live 1069 win-x86, Jun 3 2016 (the last retail Steam build)
RETAIL_QAGAME_SHA256 = "9bfad1b5df4cbbb3fcfb20781024fc5c0abe73ba8389c2f20be8c0a55552e83d"
SKIP = 77


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__.splitlines()[0])
    parser.add_argument("--harness", required=True, type=Path)
    parser.add_argument("--retail")
    args, extra = parser.parse_known_args()  # the rest goes to the harness

    try:
        retail = find_retail_install(args.retail)
    except SystemExit:
        print("no retail Quake Live install; skipping")
        return SKIP
    with zipfile.ZipFile(retail / "baseq3" / "bin.pk3") as package:
        module = package.read("qagamex86.dll")
    digest = hashlib.sha256(module).hexdigest()
    if digest != RETAIL_QAGAME_SHA256:
        print(f"retail qagamex86.dll {digest} is not the build the harness maps; skipping")
        return SKIP

    scratch = ROOT / ".tmp"
    scratch.mkdir(exist_ok=True)
    work = Path(tempfile.mkdtemp(prefix="pmove-lockstep-", dir=scratch))
    try:
        dll = work / "qagamex86.dll"
        dll.write_bytes(module)
        return subprocess.run([str(args.harness), str(dll), str(retail), *extra]).returncode
    finally:
        shutil.rmtree(work, ignore_errors=True)


if __name__ == "__main__":
    sys.exit(main())
