#!/usr/bin/env python3
"""Require FnQL-game's level loading screen to match the retail cgame's.

The test-only snapshot hold (tests/cgame_snapshot_hold_proxy.c) is installed
as the probe home's cgamex86.dll and loads the module named by
FNQL_CGAME_PROXY_TARGET: the retail cgamex86.dll read from the install's
baseq3/bin.pk3 into the probe's private scratch, or FnQL-game's module.
While the hold is on the wrapped module sees no snapshot, so after its
CG_Init it keeps drawing its loading screen in the "Awaiting Snapshot" state
(levelshot, bars, logos, map name, authors, gametype, the waiting text and
all four progress squares) during ordinary frames. The FnQL client hosts
both runs on the same map with the retail qagame, so the configstrings the
screen shows are identical, takes an engine screenshot, then releases the
hold and requires the module to reach the game. Every pixel of the two
captures must match within the menu parity gate's THRESHOLD, and each must
show the loading screen's top bar. The client is windowless (com_headless 2:
it renders but never shows a window), with sound, mouse and joystick
disabled and console notifications off, in private unpure probe homes;
installed files are never changed.
"""
from __future__ import annotations

import argparse
import os
from pathlib import Path
import shutil
import subprocess
import sys
import tempfile
import time
import zipfile

from smoke_dedicated import find_retail_install
from ui_menu_parity_probe import THRESHOLD, compare, remove
import headless

ROOT = Path(__file__).resolve().parents[1]
SHOT = 'loading_hold'
def script(map_name: str, factory: str) -> list[str]:
    return [
        f'devmap {map_name} {factory}',
        'wait 400',
        f'screenshotPNG {SHOT}',
        'wait 20',
        'pxholdsnap 0',
        'wait 150',
        'echo loading-probe-done',
        'quit',
    ]


def run(client: Path, proxy: Path, target: Path, retail: Path, scratch: Path, label: str,
        width: int, height: int, map_name: str, factory: str) -> Path:
    root = Path(tempfile.mkdtemp(prefix=f'loading-{label}-', dir=scratch)).resolve()
    home = root / 'run'
    base = home / 'baseq3'
    base.mkdir(parents=True)
    shutil.copy2(proxy, base / 'cgamex86.dll')
    (base / 'probe.cfg').write_text('\n'.join(script(map_name, factory)) + '\n', encoding='ascii')
    settings = {
        'fs_basepath': str(retail), 'fs_homepath': str(home), 'sv_pure': '0',
        'r_fullscreen': '0', 'r_mode': '-1', 'r_customwidth': str(width), 'r_customheight': str(height),
        'in_mouse': '0', 'in_nograb': '1', 'in_joystick': '0', 's_initsound': '0',
        'logfile': '2', 'developer': '1', 'con_notifytime': '0', 'cl_allowDownload': '0',
        'net_port': '28131', 'com_maxfps': '125', 'vm_gameProvider': 'retail',
        **headless.CLIENT_SETTINGS,
    }
    command = [str(client)]
    for key, value in settings.items():
        command += ['+set', key, value]
    command += ['+exec', 'probe.cfg']
    env = dict(os.environ, FNQL_CGAME_PROXY_TARGET=str(target))
    process = subprocess.Popen(command, cwd=home, env=env, stdin=subprocess.DEVNULL, stdout=subprocess.DEVNULL,
                               stderr=subprocess.DEVNULL, **headless.popen_options())
    try:
        deadline = time.monotonic() + 180
        while process.poll() is None and time.monotonic() < deadline:
            time.sleep(.25)
    finally:
        if process.poll() is None:
            process.kill()
        process.wait(timeout=10)
    shots = root / 'shots'
    shots.mkdir()
    captured = base / 'screenshots' / f'{SHOT}.png'
    if captured.is_file():
        shutil.copy2(captured, shots / captured.name)
    return root


def failures_for(root: Path, label: str) -> list[str]:
    log = root / 'run' / 'baseq3' / 'qconsole.log'
    text = log.read_text(encoding='latin-1') if log.is_file() else ''
    failures = []
    if not (root / 'shots' / f'{SHOT}.png').is_file():
        failures.append(f'{label}: no loading-screen capture')
    # The map must load and the held module must be the one answering the
    # proxy's command, then reach the game once released.
    for needed in ('CM_LoadMap', 'Going from CS_PRIMED to CS_ACTIVE', 'loading-probe-done'):
        if needed not in text:
            failures.append(f'{label}: missing {needed!r}')
    shot = text.find(f'screenshots/{SHOT}.png')
    if shot < 0 or shot < text.find('CM_LoadMap'):
        failures.append(f'{label}: the capture was not taken after the map loaded')
    if 'Unknown command "pxholdsnap' in text:
        failures.append(f'{label}: the snapshot-hold proxy was not the loaded cgame')
    capture = root / 'shots' / f'{SHOT}.png'
    if capture.is_file():
        # A windowless capture must hold the frame itself, not an empty
        # buffer: the loading screen's dark-red top bar must be there.
        from PIL import Image
        image = Image.open(capture).convert('RGB')
        red, green, blue = image.getpixel((image.width // 2, 4))
        if not (red > green + 12 and red > blue + 12):
            failures.append(f'{label}: the capture lacks the loading screen (top pixel {red},{green},{blue})')
    if '********************' in text or 'ERROR:' in text:
        failures.append(f'{label}: engine error in the log')
    return failures


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__.splitlines()[0])
    parser.add_argument('--client', required=True, type=Path)
    parser.add_argument('--module', required=True, type=Path)
    parser.add_argument('--proxy', required=True, type=Path)
    parser.add_argument('--retail')
    parser.add_argument('--width', type=int, default=960)
    parser.add_argument('--height', type=int, default=600)
    parser.add_argument('--map', default='campgrounds')
    parser.add_argument('--factory', default='ffa')
    parser.add_argument('--keep', action='store_true')
    args = parser.parse_args()
    if sys.platform != 'win32':
        print('The loading-screen gate hosts the retail Windows x86 cgame')
        return 77
    try:
        import PIL  # noqa: F401
    except ImportError:
        print('Pillow is required for the loading-screen comparison')
        return 77
    try:
        retail = find_retail_install(args.retail)
    except SystemExit:
        print('No retail Quake Live install; skipping')
        return 77
    scratch = ROOT / '.tmp' / 'loading'
    scratch.mkdir(parents=True, exist_ok=True)
    roots: list[Path] = []
    passed = False
    try:
        reference = Path(tempfile.mkdtemp(prefix='retail-cgame-', dir=scratch)).resolve()
        roots.append(reference)
        with zipfile.ZipFile(retail / 'baseq3' / 'bin.pk3') as pak:
            (reference / 'cgamex86.dll').write_bytes(pak.read('cgamex86.dll'))
        client, proxy = args.client.resolve(strict=True), args.proxy.resolve(strict=True)
        module = args.module.resolve(strict=True)
        retail_root = run(client, proxy, reference / 'cgamex86.dll', retail, scratch, 'retail',
                          args.width, args.height, args.map, args.factory)
        roots.append(retail_root)
        fnql_root = run(client, proxy, module, retail, scratch, 'fnql', args.width, args.height,
                        args.map, args.factory)
        roots.append(fnql_root)
        failures = failures_for(retail_root, 'retail') + failures_for(fnql_root, 'FnQL-game')
        if not failures:
            differing = compare(retail_root, fnql_root).get(f'{SHOT}.png')
            if differing is None:
                failures.append('captures are missing or differ in size')
            elif differing:
                failures.append(f'{differing} pixels differ by more than {THRESHOLD} levels')
        if failures:
            print(f'Loading screen differs from retail on {args.map} at {args.width}x{args.height}: '
                  + '; '.join(failures))
            print(f'Evidence: {retail_root}')
            print(f'Evidence: {fnql_root}')
            return 1
        print(f'Loading screen matches retail on {args.map} at {args.width}x{args.height}')
        passed = True
        return 0
    finally:
        # Failed runs keep their evidence; passing runs leave nothing behind.
        if passed and not args.keep:
            for root in roots:
                if root.exists():
                    remove(root, scratch)


if __name__ == '__main__':
    sys.exit(main())
