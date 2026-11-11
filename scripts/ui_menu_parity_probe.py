#!/usr/bin/env python3
"""Require the installed menus to render as the retail UI renders them.

Both UIs run the same engine and installed menus. Three capture sets exist:

- ingame: the ten in-game menus on a cheat-enabled local map with the world,
  entities and HUD hidden (devmap, r_drawworld 0, r_drawentities 0,
  cg_draw2D 0), so a capture holds only UI pixels;
- tabs: every tab page of ingame_options (with each advanced sub-page) and
  ingame_controls. A page is reached by a private variant of its menu whose
  onOpen runs the tab button's own installed action script; variants live in
  the probe home and load through a homepath ui/ingame.txt, three per run so
  the copies fit the UI memory pool, and stay inside retail's 2048-byte
  script buffer (0x10014cf0);
- outgame: main, main_options, quit, demo, error_popmenu with a pending
  error, and the connect screen while connecting to an unused local port.
  Steam and the web UI run at their defaults, as in retail, so out of game
  the Awesomium web UI owns the screen (web_browserActive 1) and both UIs
  must yield to it (retail Menu_Paint 0x1001d82e). Each of those captures
  first requires web_browserActive 1 in both runs, so a web UI that failed
  to come up cannot pass as uix86 parity. The connect screen is uix86's even
  then (web_browserActive 0).

Menus open through retail menu_open; no input is synthesized. Captures are
engine screenshots from the windowless client (com_headless 2), with mouse
and joystick input disabled, in private unpure probe homes; installed files
are never changed.

Animated menu shaders differ by a few levels between any two runs, so a pixel
counts as different only above THRESHOLD levels. Every capture must have none.
A set whose retail capture misses its screens (load timing) is retried once.
"""
from __future__ import annotations

import argparse
from pathlib import Path
import re
import shutil
import subprocess
import sys
import tempfile
import time
import zipfile

from smoke_dedicated import find_retail_install
from ui_probe import linked

ROOT = Path(__file__).resolve().parents[1]
MENUS = ('ingame_about', 'ingame', 'ingame_callvote', 'ingame_addbot', 'ingame_admin',
         'ingame_join', 'ingame_controls', 'ingame_options', 'ingame_vote', 'demo')
OUTGAME = ('main', 'main_options', 'quit', 'demo')
# The web UI needs a moment after startup before it owns the screen.
WEB_UI_SETTLE = 'wait 200'
# Retail-against-retail captures differ by at most 4 levels.
THRESHOLD = 16
HIDE_WORLD = ['wait 40', 'devmap campgrounds ffa', 'wait 140', 'r_drawworld 0', 'r_drawentities 0',
              'cg_draw2D 0', 'wait 20', 'togglemenu', 'wait 20', 'menu_close ingame_about',
              'menu_close ingame']


def block_end(text: str, index: int) -> int:
    depth = 1
    while depth and index < len(text):
        depth += {'{': 1, '}': -1}.get(text[index], 0)
        index += 1
    return index


def tab_scripts(text: str) -> list[tuple[str, str]]:
    """(item name, action) for each installed button that switches a group."""
    body = re.sub(r'//[^\n]*', '', text)
    found = []
    for item in re.finditer(r'itemDef\s*\{', body):
        block = body[item.end():block_end(body, item.end()) - 1]
        action = re.search(r'\baction\s*\{', block)
        if not action:
            continue
        script = block[action.end():block_end(block, action.end()) - 1]
        if 'hide grp' in script:
            name = re.search(r'\bname\s+"?([^\s"]+)', block)
            found.append((name.group(1) if name else 'item', ' '.join(script.split())))
    return found


def switches_only(script: str) -> str:
    return ' ; '.join(part.strip() for part in script.split(';')
                      if part.strip().split()[:1] in (['hide'], ['show'], ['uiScript']))


def variant(text: str, menu: str, name: str, script: str) -> str:
    text = re.sub(r'(\bname\s+)"?%s"?' % menu, r'\1"%s"' % name, text, count=1)
    opened = re.search(r'\bonOpen\s*\{', text, re.I)
    if opened and opened.start() < text.find('itemDef'):
        end = block_end(text, opened.end()) - 1
        own = switches_only(text[opened.end():end])
        return text[:opened.end()] + ' ' + own + ' ; ' + script + ' ' + text[end:]
    index = text.find('"%s"' % name) + len(name) + 2
    return text[:index] + '\n\t\tonOpen { ' + script + ' }\n' + text[index:]


def tab_pages(retail: Path) -> list[tuple[str, str, str, str]]:
    pak = zipfile.ZipFile(retail / 'baseq3' / 'pak00.pk3')
    read = lambda name: pak.read(name).decode('latin-1')
    pages = []
    options = read('ui/ingame_options.menu')
    top = tab_scripts(options)
    opened = re.search(r'\bonOpen\s*\{', options, re.I)
    bare = options[:opened.start()] + options[block_end(options, opened.end()):] if opened else options
    for label, script in top:
        pages.append(('opt_' + label, 'ingame_options', bare, script))
    advanced = switches_only([s for _, s in top if 'show grpControlsAdvMenu' in s][0])
    for label, script in tab_scripts(read('ui/ingame_options_advanced.menu')):
        pages.append(('adv_' + label, 'ingame_options', bare, advanced + ' ; ' + script))
    controls = read('ui/ingame_controls.menu')
    for label, script in tab_scripts(controls):
        pages.append(('ctl_' + label, 'ingame_controls', controls, script))
    return pages


def run(client: Path, retail: Path, module: Path | None, home: Path, commands: list[str],
        width: int, height: int, files: dict[str, str] | None = None) -> None:
    base = home / 'baseq3'
    (base / 'ui').mkdir(parents=True)
    if module:
        shutil.copy2(module, base / 'uix86.dll')
    for name, text in (files or {}).items():
        (base / name).write_text(text, encoding='latin-1')
    (base / 'probe.cfg').write_text('\n'.join(commands + ['quit', '']), encoding='ascii')
    settings = {
        'com_headless': '2', 'fs_basepath': str(retail), 'fs_homepath': str(home), 'sv_pure': '0',
        'r_fullscreen': '0', 'r_mode': '-1', 'r_customwidth': str(width), 'r_customheight': str(height),
        'in_mouse': '0', 'in_nograb': '1', 'in_joystick': '0', 's_initsound': '0',
        'logfile': '2', 'developer': '1',
        'cl_allowDownload': '0', 'vm_gameProvider': 'retail', 'net_port': '28124', 'com_maxfps': '125',
    }
    command = [str(client)]
    for key, value in settings.items():
        command += ['+set', key, value]
    command += ['+exec', 'probe.cfg']
    startup = None
    if sys.platform == 'win32':
        startup = subprocess.STARTUPINFO()
        startup.dwFlags = subprocess.STARTF_USESHOWWINDOW
        startup.wShowWindow = 4
    process = subprocess.Popen(command, cwd=home, stdin=subprocess.DEVNULL, stdout=subprocess.DEVNULL,
                               stderr=subprocess.DEVNULL, startupinfo=startup)
    try:
        deadline = time.monotonic() + 150
        while process.poll() is None and time.monotonic() < deadline:
            time.sleep(.25)
    finally:
        if process.poll() is None:
            process.kill()
        process.wait(timeout=10)


def capture(kind: str, client: Path, retail: Path, module: Path | None, scratch: Path,
            width: int, height: int) -> Path:
    root = Path(tempfile.mkdtemp(prefix=f'parity-{kind}-' + ('fnql-' if module else 'retail-'),
                                 dir=scratch)).resolve()
    shots = root / 'shots'
    shots.mkdir()
    if kind == 'ingame':
        commands = list(HIDE_WORLD)
        for menu in MENUS:
            commands += [f'menu_open {menu}', 'wait 30', f'screenshotPNG {menu}', 'wait 5', f'menu_close {menu}']
        homes = [(root / 'run', commands, None)]
    elif kind == 'outgame':
        web = lambda label: [f'echo PXWEB {label}', 'cvarlist web_browserActive']
        commands = [WEB_UI_SETTLE]
        for menu in OUTGAME:
            commands += [f'menu_open {menu}', 'wait 20'] + web(f'out_{menu}') + [f'screenshotPNG out_{menu}',
                                                                                  'wait 3', f'menu_close {menu}']
        commands += ['set com_errorMessage "Probe error: the server refused the connection."',
                     'menu_open error_popmenu', 'wait 20'] + web('out_error') + [
                     'screenshotPNG out_error', 'wait 3',
                     'menu_close error_popmenu', 'set com_errorMessage ""', 'connect 127.0.0.1:27999',
                     'wait 6'] + web('out_connect') + ['screenshotPNG out_connect', 'wait 3', 'disconnect', 'wait 10']
        homes = [(root / 'run', commands, None)]
    else:
        pages = tab_pages(retail)
        homes = []
        for start in range(0, len(pages), 3):
            files = {}
            load = ['ui/ingame.menu', 'ui/ingame_about.menu']
            commands = list(HIDE_WORLD)
            for offset, (label, menu, text, script) in enumerate(pages[start:start + 3]):
                name = f'parity{start + offset:02d}'
                files[f'ui/{name}.menu'] = variant(text, menu, name, script)
                load.append(f'ui/{name}.menu')
                commands += [f'menu_open {name}', 'wait 30', f'screenshotPNG {label}', 'wait 5', f'menu_close {name}']
            files['ui/ingame.txt'] = '{\n' + ''.join(f' loadMenu {{ "{path}" }}\n' for path in load) + '}\n'
            homes.append((root / f'b{start:02d}', commands, files))
    for home, commands, files in homes:
        run(client, retail, module, home, commands, width, height, files)
        captured = home / 'baseq3' / 'screenshots'
        if captured.is_dir():
            for png in captured.glob('*.png'):
                shutil.copy2(png, shots / png.name)
    return root


def web_ui_failures(root: Path, which: str) -> list[str]:
    """Out-of-game captures need the web UI up; the connect screen needs it down."""
    states: dict[str, str] = {}
    label = None
    for log in root.rglob('qconsole.log'):
        for line in log.read_text(encoding='latin-1').splitlines():
            marker = re.match(r'PXWEB (\w+)\s*$', line)
            if marker:
                label = marker.group(1)
                continue
            row = re.match(r'\s.*\sweb_browserActive "(.*)"\s*$', line)
            if label and row:
                states[label] = row.group(1)
                label = None
    failures = []
    for name in [f'out_{menu}' for menu in OUTGAME] + ['out_error', 'out_connect']:
        expected = '0' if name == 'out_connect' else '1'
        if states.get(name) != expected:
            failures.append(f'{which} {name}: web_browserActive {states.get(name, "missing")}, expected {expected}')
    return failures


def compare(retail_root: Path, fnql_root: Path) -> dict[str, int | None]:
    from PIL import Image, ImageChops
    names = sorted({p.name for p in (retail_root / 'shots').glob('*.png')} |
                   {p.name for p in (fnql_root / 'shots').glob('*.png')})
    result: dict[str, int | None] = {}
    for name in names:
        paths = [root / 'shots' / name for root in (retail_root, fnql_root)]
        if not all(path.is_file() for path in paths):
            result[name] = None
            continue
        try:
            a, b = (Image.open(path).convert('RGB') for path in paths)
        except (OSError, ValueError):
            # A truncated or empty capture counts as missing, so the set retries.
            result[name] = None
            continue
        if a.size != b.size:
            result[name] = None
            continue
        mask = ImageChops.difference(a, b).convert('L').point(lambda value: 255 if value > THRESHOLD else 0)
        result[name] = mask.histogram()[255]
    return result


def remove(root: Path, scratch: Path) -> None:
    if root.parent == scratch.resolve() and not linked(root) and not any(linked(p) for p in root.rglob('*')):
        shutil.rmtree(root)
    else:
        print(f'Preserved unexpected probe output: {root}')


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__.splitlines()[0])
    parser.add_argument('--client', required=True, type=Path)
    parser.add_argument('--module', required=True, type=Path)
    parser.add_argument('--retail')
    parser.add_argument('--width', type=int, default=960)
    parser.add_argument('--height', type=int, default=600)
    parser.add_argument('--sets', default='ingame,tabs,outgame')
    parser.add_argument('--keep', action='store_true')
    args = parser.parse_args()
    try:
        import PIL  # noqa: F401
    except ImportError:
        print('Pillow is required for the menu parity comparison')
        return 77
    client = args.client.resolve(strict=True)
    module = args.module.resolve(strict=True)
    retail = find_retail_install(args.retail)
    scratch = ROOT / '.tmp/ui'
    scratch.mkdir(parents=True, exist_ok=True)
    failures = []
    total = 0
    for kind in [k.strip() for k in args.sets.split(',') if k.strip()]:
        if kind not in ('ingame', 'tabs', 'outgame'):
            parser.error(f'unknown capture set {kind}')
        for attempt in range(2):
            roots = [capture(kind, client, retail, None, scratch, args.width, args.height),
                     capture(kind, client, retail, module, scratch, args.width, args.height)]
            result = compare(*roots)
            web = []
            if kind == 'outgame':
                web = web_ui_failures(roots[0], 'retail') + web_ui_failures(roots[1], 'FnQL-game')
            clean = bool(result) and all(count == 0 for count in result.values()) and not web
            for root in roots:
                if args.keep or (not clean and attempt == 1):
                    print(f'Evidence: {root}')
                else:
                    remove(root, scratch)
            if clean:
                total += len(result)
                break
        else:
            failures += ([f'{name}: {"missing" if count is None else f"{count} px"}'
                          for name, count in result.items() if count != 0] + web) or [f'{kind}: no captures']
    if failures:
        print(f'Menus differ from retail at {args.width}x{args.height} (> {THRESHOLD} levels): ' + ', '.join(failures))
        return 1
    print(f'{total} captures match retail at {args.width}x{args.height} ({args.sets})')
    return 0


if __name__ == '__main__':
    sys.exit(main())
