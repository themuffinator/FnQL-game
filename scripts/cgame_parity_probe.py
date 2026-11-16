#!/usr/bin/env python3
"""Require FnQL-game's cgame to draw recorded matches as the retail cgame does.

The FnQL client first records a short bot match on a stock map with the
retail qagame and the retail cgame (a demo holds server snapshots and
commands only, so the recording does not depend on the cgame). It then plays
that demo back twice under `timedemo 1`, once with the retail cgamex86.dll
read from the install's bin.pk3 and once with FnQL-game's module. Timedemo
advances the client by exactly one snapshot per frame, so the same frame
count shows both modules the same snapshot at the same cg.time; each run
takes engine screenshots at the same frames.

Each pair of captures must match: a pixel differs when any channel differs
by more than the menu parity gate's THRESHOLD levels, and a capture may have
at most --tolerance such pixels (a fraction of the frame). The tolerance
exists for effects that both modules randomize with their own generators
(smoke puffs, sparks, shell ejection, explosion rotation); it does not cover
HUD layout or world presentation differences, which change far more pixels.
Failing runs keep the captures and a difference image for inspection.

The client is windowless (com_headless 2: it renders but never shows a
window), with sound, mouse and joystick disabled and console notifications
off, in private unpure probe homes; installed files are never changed.
"""
from __future__ import annotations

import argparse
from pathlib import Path
import shutil
import subprocess
import sys
import tempfile
import time
import zipfile

from smoke_dedicated import find_retail_install
from ui_menu_parity_probe import THRESHOLD, remove
import headless

ROOT = Path(__file__).resolve().parents[1]
DEMO = 'fnqlparity'
# Under timedemo each client frame advances the demo by 50 ms. Both
# playbacks capture at these frames; SCOREBOARD_FRAME holds +scores for a
# capture of the scoreboard the recorder requested shortly before.
FRAMES = tuple(range(50, 601, 50))
SCOREBOARD_FRAME = 325
TEAM_MODES = ('tdm', 'ca', 'ctf', '1fctf', 'har', 'ft', 'dom', 'ad', 'rr')


def capture_names() -> list[str]:
    return [f'parity_{frame:04d}.png' for frame in FRAMES] + [f'parity_{SCOREBOARD_FRAME:04d}_scores.png']


def settings_for(retail: Path, home: Path, width: int, height: int, port: int) -> dict[str, str]:
    return {
        'fs_basepath': str(retail), 'fs_homepath': str(home), 'sv_pure': '0',
        'r_fullscreen': '0', 'r_mode': '-1', 'r_customwidth': str(width), 'r_customheight': str(height),
        'in_mouse': '0', 'in_nograb': '1', 'in_joystick': '0', 's_initsound': '0',
        'logfile': '2', 'developer': '1', 'con_notifytime': '0', 'cl_allowDownload': '0',
        'net_port': str(port), 'com_maxfps': '125', 'vm_gameProvider': 'retail',
        **headless.CLIENT_SETTINGS,
    }


def launch(client: Path, home: Path, settings: dict[str, str], lines: list[str], timeout: float) -> str:
    base = home / 'baseq3'
    base.mkdir(parents=True, exist_ok=True)
    (base / 'probe.cfg').write_text('\n'.join(lines) + '\n', encoding='ascii')
    command = [str(client)]
    for key, value in settings.items():
        command += ['+set', key, value]
    command += ['+exec', 'probe.cfg']
    process = subprocess.Popen(command, cwd=home, stdin=subprocess.DEVNULL, stdout=subprocess.DEVNULL,
                               stderr=subprocess.DEVNULL, **headless.popen_options())
    try:
        deadline = time.monotonic() + timeout
        while process.poll() is None and time.monotonic() < deadline:
            time.sleep(.25)
    finally:
        if process.poll() is None:
            process.kill()
        process.wait(timeout=10)
    log = base / 'qconsole.log'
    return log.read_text(encoding='latin-1') if log.is_file() else ''


def record(client: Path, retail: Path, scratch: Path, map_name: str, factory: str,
           width: int, height: int, port: int) -> tuple[Path, str]:
    root = Path(tempfile.mkdtemp(prefix='parity-record-', dir=scratch)).resolve()
    if factory in TEAM_MODES:
        # Team bots join only with a side; the local player is red.
        join = ['team red', 'wait 120', 'addbot sarge 4 red', 'addbot visor 4 blue',
                'addbot anarki 4 blue', 'wait 60', 'allready']
    else:
        join = ['team free', 'wait 120', 'addbot sarge 4', 'addbot visor 4', 'addbot anarki 4']
    # The recording starts once the countdown has passed (125 frames a
    # second) and lasts 30 seconds. Twelve seconds in, the recorder holds the
    # scoreboard so the demo carries a fresh scores reply for the playback's
    # scoreboard capture.
    lines = [f'devmap {map_name} {factory}', 'wait 180', *join, 'wait 1500',
             f'record {DEMO}', 'wait 1500', '+scores', 'wait 125', '-scores', 'wait 2125',
             'stoprecord', 'wait 30', 'echo parity-record-done', 'quit']
    settings = settings_for(retail, root / 'run', width, height, port)
    settings.update({'g_doWarmup': '0', 'fraglimit': '0', 'timelimit': '0'})
    log = launch(client, root / 'run', settings, lines, 180)
    return root, log


def play(client: Path, retail: Path, scratch: Path, label: str, module: Path, demo: Path,
         width: int, height: int, port: int) -> tuple[Path, str]:
    root = Path(tempfile.mkdtemp(prefix=f'parity-{label}-', dir=scratch)).resolve()
    base = root / 'run' / 'baseq3'
    (base / 'demos').mkdir(parents=True)
    shutil.copy2(module, base / 'cgamex86.dll')
    # The client names recordings itself; play the copy under a fixed name.
    shutil.copy2(demo, base / 'demos' / (DEMO + demo.suffix))
    lines = ['timedemo 1', f'demo {DEMO}']
    previous = 0
    for frame in sorted((*FRAMES, SCOREBOARD_FRAME)):
        lines.append(f'wait {frame - previous}')
        if frame == SCOREBOARD_FRAME:
            # +scores, the capture and -scores each take a frame of their own.
            lines += ['+scores', 'wait 1', f'screenshotPNG parity_{frame:04d}_scores', 'wait 1', '-scores']
            frame += 2
        else:
            lines.append(f'screenshotPNG parity_{frame:04d}')
        previous = frame
    lines += ['wait 5', 'echo parity-play-done', 'quit']
    log = launch(client, root / 'run', settings_for(retail, root / 'run', width, height, port),
                 lines, 120)
    shots = root / 'shots'
    shots.mkdir()
    for capture in (base / 'screenshots').glob('parity_*.png'):
        shutil.copy2(capture, shots / capture.name)
    return root, log


def log_failures(log: str, label: str, needed: tuple[str, ...]) -> list[str]:
    failures = [f'{label}: missing {text!r}' for text in needed if text not in log]
    if '********************' in log or 'ERROR:' in log:
        failures.append(f'{label}: engine error in the log')
    return failures


def wall_clock_boxes(retail: Path, factory: str, width: int, height: int) -> list[tuple[int, int, int, int]]:
    """Screen boxes of the scoreboard's CG_LOCALTIME text (wall-clock time).

    The two playbacks run seconds apart, so a minute can turn between them;
    that text is masked in both scoreboard captures. Boxes come from the
    installed scoreboard menu: the item's rect offset by its menu's rect,
    mapped through the item's (or menu's) widescreen placement, widened by
    the item width in the direction of its alignment plus a margin.
    """
    import re
    try:
        with zipfile.ZipFile(retail / 'baseq3' / 'pak00.pk3') as pak:
            text = pak.read(f'ui/ingame_scoreboard_{factory}.menu').decode('latin-1')
    except (KeyError, OSError):
        return []
    scale = height / 480.0
    bias = (width - 640 * scale) / 2
    boxes = []
    for menu in re.split(r'\bmenuDef\b', text)[1:]:
        menu_rect = re.search(r'\brect\s+(-?[\d.]+)\s+(-?[\d.]+)', menu)
        menu_wide = re.search(r'\bwidescreen\s+(\w+)', menu.split('itemDef')[0])
        mx, my = (float(menu_rect.group(1)), float(menu_rect.group(2))) if menu_rect else (0.0, 0.0)
        for item in menu.split('itemDef')[1:]:
            if not re.search(r'ownerdraw\s+CG_LOCALTIME\b', item):
                continue
            rect = re.search(r'\brect\s+(-?[\d.]+)\s+(-?[\d.]+)\s+(-?[\d.]+)\s+(-?[\d.]+)', item)
            if not rect:
                continue
            x, y, w, h = (float(v) for v in rect.groups())
            wide = re.search(r'\bwidescreen\s+(\w+)', item) or menu_wide
            mode = wide.group(1) if wide else 'WIDESCREEN_CENTER'
            offset = {'WIDESCREEN_LEFT': 0, '1': 0, 'WIDESCREEN_RIGHT': 2 * bias, '3': 2 * bias}.get(mode, bias)
            align = re.search(r'\b(?:text)?align\s+(\w+)', item)
            align = align.group(1) if align else '0'
            left = x - w if align in ('ITEM_ALIGN_RIGHT', '2') else x - w / 2 if align in ('ITEM_ALIGN_CENTER', '1') else x
            sx0, sx1 = (mx + left - 4) * scale + offset, (mx + left + w + 4) * scale + offset
            sy0, sy1 = (my + y - h - 4) * scale, (my + y + 6) * scale
            boxes.append((max(0, int(sx0)), max(0, int(sy0)), min(width, int(sx1) + 1), min(height, int(sy1) + 1)))
    return boxes


def compare(retail_root: Path, fnql_root: Path, tolerance: float,
            masks: dict[str, list[tuple[int, int, int, int]]]) -> list[str]:
    from PIL import Image, ImageChops, ImageDraw
    failures = []
    for name in capture_names():
        paths = [root / 'shots' / name for root in (retail_root, fnql_root)]
        if not all(path.is_file() for path in paths):
            failures.append(f'{name}: missing capture')
            continue
        a, b = (Image.open(path).convert('RGB') for path in paths)
        if a.size != b.size:
            failures.append(f'{name}: sizes differ')
            continue
        for box in masks.get(name, ()):
            for image in (a, b):
                ImageDraw.Draw(image).rectangle((box[0], box[1], box[2] - 1, box[3] - 1), fill=(0, 0, 0))
        mask = ImageChops.difference(a, b).convert('L').point(lambda value: 255 if value > THRESHOLD else 0)
        differing = mask.histogram()[255]
        limit = int(a.size[0] * a.size[1] * tolerance)
        print(f'{name}: {differing} pixels differ by more than {THRESHOLD} levels (limit {limit})')
        if differing > limit:
            mask.save(fnql_root / 'shots' / name.replace('.png', '_diff.png'))
            failures.append(f'{name}: {differing} differing pixels exceed {limit}')
    return failures


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__.splitlines()[0])
    parser.add_argument('--client', required=True, type=Path)
    parser.add_argument('--module', required=True, type=Path)
    parser.add_argument('--retail')
    parser.add_argument('--width', type=int, default=960)
    parser.add_argument('--height', type=int, default=600)
    parser.add_argument('--map', default='campgrounds')
    parser.add_argument('--factory', default='ffa')
    parser.add_argument('--tolerance', type=float, default=0.002,
                        help='fraction of a capture allowed to differ (randomized effects)')
    parser.add_argument('--self-check', action='store_true',
                        help='play the demo with the retail cgame twice (measures the noise floor)')
    parser.add_argument('--port', type=int, default=28133,
                        help='client net_port for the recording; the playbacks use the next two')
    parser.add_argument('--keep', action='store_true')
    args = parser.parse_args()
    if sys.platform != 'win32':
        print('The cgame parity gate hosts the retail Windows x86 cgame')
        return 77
    try:
        import PIL  # noqa: F401
    except ImportError:
        print('Pillow is required for the cgame parity comparison')
        return 77
    try:
        retail = find_retail_install(args.retail)
    except SystemExit:
        print('No retail Quake Live install; skipping')
        return 77
    scratch = ROOT / '.tmp' / 'cgame-parity'
    scratch.mkdir(parents=True, exist_ok=True)
    roots: list[Path] = []
    passed = False
    try:
        client = args.client.resolve(strict=True)
        reference = Path(tempfile.mkdtemp(prefix='retail-cgame-', dir=scratch)).resolve()
        roots.append(reference)
        with zipfile.ZipFile(retail / 'baseq3' / 'bin.pk3') as pak:
            (reference / 'cgamex86.dll').write_bytes(pak.read('cgamex86.dll'))
        module = reference / 'cgamex86.dll' if args.self_check else args.module.resolve(strict=True)
        record_root, log = record(client, retail, scratch, args.map, args.factory, args.width, args.height, args.port)
        roots.append(record_root)
        failures = log_failures(log, 'record', ('Going from CS_PRIMED to CS_ACTIVE', 'parity-record-done'))
        demos = sorted((record_root / 'run' / 'baseq3' / 'demos').glob('*.dm_*'))
        if not demos:
            failures.append('record: no demo written')
        if not failures:
            retail_root, retail_log = play(client, retail, scratch, 'retail', reference / 'cgamex86.dll',
                                           demos[0], args.width, args.height, args.port + 1)
            roots.append(retail_root)
            fnql_root, fnql_log = play(client, retail, scratch, 'fnql', module, demos[0],
                                       args.width, args.height, args.port + 2)
            roots.append(fnql_root)
            identity = 'VM_LoadDll(cgame) found native exports' if args.self_check else 'VM_LoadDll(cgame): FnQL-game'
            failures += log_failures(retail_log, 'retail', ('VM_LoadDll(cgame) found native exports', 'parity-play-done'))
            failures += log_failures(fnql_log, 'FnQL-game', (identity, 'parity-play-done'))
            if not failures:
                # A dead player sees the scoreboard in gameplay frames too.
                clock = wall_clock_boxes(retail, args.factory, args.width, args.height)
                masks = {name: clock for name in capture_names()}
                failures += compare(retail_root, fnql_root, args.tolerance, masks)
        if failures:
            print(f'cgame demo playback differs from retail on {args.map}/{args.factory} at '
                  f'{args.width}x{args.height}: ' + '; '.join(failures))
            for root in roots[1:]:
                print(f'Evidence: {root}')
            return 1
        print(f'cgame demo playback matches retail on {args.map}/{args.factory} at {args.width}x{args.height}')
        passed = True
        return 0
    finally:
        # Failed runs keep their evidence; passing runs leave nothing behind.
        if (passed and not args.keep) or (roots and not args.keep and len(roots) == 1):
            for root in roots:
                if root.exists():
                    remove(root, scratch)


if __name__ == '__main__':
    sys.exit(main())
