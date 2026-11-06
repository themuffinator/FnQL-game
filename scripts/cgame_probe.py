#!/usr/bin/env python3
"""Windowed, input-free native cgame hosting probe using installed retail assets.

The module is copied under the retail filename only inside a disposable,
explicitly unpure home until FnQL's client provider/purity work is enabled.
This is a hosting gate, not a claim of prediction or visual parity.
"""

from __future__ import annotations
import argparse
import re
import secrets
import shutil
import socket
import stat
import subprocess
import sys
import tempfile
import time
from pathlib import Path
from smoke_dedicated import find_retail_install

ROOT = Path(__file__).resolve().parents[1]

def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__.splitlines()[0])
    parser.add_argument('--client', required=True, type=Path)
    parser.add_argument('--module', type=Path, default=ROOT/'build/win32-x86/fnql_cgamex86.dll')
    parser.add_argument('--retail-cgame', action='store_true', help='use installed retail cgame for reference captures')
    parser.add_argument('--retail')
    parser.add_argument('--qagame', type=Path)
    parser.add_argument('--map', default='campgrounds')
    parser.add_argument('--factory', default='ffa')
    parser.add_argument('--timeout', type=float, default=90)
    parser.add_argument('--combat', action='store_true', help='add four bots and observe autonomous combat')
    parser.add_argument('--match-end', action='store_true', help='end a bot match at the first frag and observe the result event')
    parser.add_argument('--overtime', action='store_true', help='hold a tied bot match through its first overtime event')
    parser.add_argument('--thaw', action='store_true', help='observe a real thaw event in a Freeze Tag bot match')
    parser.add_argument('--damage-through', action='store_true', help='observe a rocket penetrating a surface in a bot match')
    parser.add_argument('--poi', action='store_true', help='observe a Quad Hog carrier ping (EV_POI) in a bot match')
    parser.add_argument('--frozen', action='store_true', help='capture the local player after a Freeze Tag death in third person')
    parser.add_argument('--round-result', choices=('draw','survivor'), help='observe a timed CA draw or infected RR survivor result with paused bots')
    parser.add_argument('--race', action='store_true', help='fall through a hand-authored race course without movement input')
    parser.add_argument('--race-markers', action='store_true', help='start a race by falling, then capture the upcoming points without steering')
    parser.add_argument('--scoreboard', action='store_true', help='open the scoreboard through the engine console')
    parser.add_argument('--reload-hud', action='store_true', help='reload the HUD repeatedly before checking its scoreboard')
    parser.add_argument('--stats', action='store_true', help='show the accuracy panel and request player statistics')
    parser.add_argument('--weapon-bar', type=int, choices=range(5), help='capture a retail weapon-bar layout')
    parser.add_argument('--weapon-selection', action='store_true', help='verify spawn preference, queued primary and weapon config dispatch without input commands')
    parser.add_argument('--clock-hud', choices=('match','round','timeout','timein'), help='capture an active clock with a paused bot and no player input')
    parser.add_argument('--standings-hud', choices=('individual','team'), help='capture long player names or custom team labels with a paused bot')
    parser.add_argument('--pregame-hud', choices=('ffa','ca'), help='capture waiting, ready-up and countdown with server-side allready')
    parser.add_argument('--starting-loadout', action='store_true', help='use a limited starting/loadout kit while retaining map weapon assets')
    parser.add_argument('--full-weapon-bar', action='store_true', help='include registered unowned weapon icons')
    parser.add_argument('--status-hud', action='store_true', help='capture finite ammo and bonus health/armor with a hand-authored factory')
    parser.add_argument('--ghost-weapon', action='store_true', help='render the local weapon with retail cg_drawGun 3')
    parser.add_argument('--gib-watch', action='store_true', help='bots with rocket launchers and 20 starting health fight until a player is gibbed (EV_GIB_PLAYER)')
    parser.add_argument('--gore-standin', action='store_true', help='stage a throwaway dlc_gibs folder (retail sphere gib copies, never committed) so the gore DLC branch runs, then list the registered models')
    parser.add_argument('--ui', type=Path, help='UI module to load (needed on Linux, where the retail paks carry no uii386.so)')
    parser.add_argument('--record-demo', type=Path, help='record a short bot-match demo (with --combat) and copy it here')
    parser.add_argument('--play-demo', type=Path, help='play this recorded demo and capture a frame instead of hosting a map')
    parser.add_argument('--zoom', action='store_true', help='hold +zoom for the capture (with --combat, after the bots join)')
    parser.add_argument('--flag-track', action='store_true', help='spectate a CTF bot match until a flag event, then capture the spectator flag track')
    parser.add_argument('--item-timers', action='store_true', help='spectate a TDM bot match until a timed item pickup reaches the spectator, then capture the item timers')
    parser.add_argument('--overhead-names', action='store_true', help='follow a bot as a spectator in a TDM bot match to capture overhead names')
    parser.add_argument('--third-person', action='store_true', help='capture a side view of player materials without camera input')
    parser.add_argument('--evidence-dir', type=Path, help='retain the console log and engine capture, then discard the fresh test profile')
    parser.add_argument('--keep', action='store_true')
    args = parser.parse_args()
    if args.standings_hud:
        if args.clock_hud:
            parser.error('--standings-hud selects its own clock fixture')
        args.clock_hud = 'match' if args.standings_hud == 'individual' else 'round'
    if args.weapon_selection:
        args.status_hud = args.starting_loadout = True
    if args.race_markers: args.race = True
    if sum(map(bool,(args.match_end, args.overtime, args.thaw, args.damage_through, args.poi, args.frozen, args.round_result, args.race, args.weapon_selection, args.clock_hud, args.pregame_hud))) > 1:
        parser.error('choose only one event gate')
    if args.overtime and args.combat:
        parser.error('--overtime requires a tied match without combat')
    if args.round_result and args.combat:
        parser.error('--round-result requires a timed round without combat')
    if args.race and args.combat:
        parser.error('--race uses a gravity-only course with a paused bot')
    if args.clock_hud and args.combat:
        parser.error('--clock-hud requires a paused bot without combat')
    if args.pregame_hud and args.combat:
        parser.error('--pregame-hud requires a paused bot without combat')
    if args.match_end or args.thaw or args.damage_through or args.poi or args.frozen:
        args.combat = True
    if args.overhead_names:
        if args.match_end or args.overtime or args.thaw or args.damage_through or args.poi or args.frozen or args.round_result or args.race:
            parser.error('--overhead-names uses its own spectator fixture')
        args.combat, args.factory = True, 'tdm'
    if args.gib_watch:
        args.combat = True
    if args.flag_track:
        if args.item_timers or args.match_end or args.overtime or args.thaw or args.damage_through or args.poi or args.frozen or args.round_result or args.race or args.overhead_names:
            parser.error('--flag-track uses its own spectator fixture')
        args.combat, args.factory = True, 'ctf'
        if args.map == 'campgrounds':
            args.map = 'spidercrossings'    # campgrounds has no CTF flags
    if args.item_timers:
        if args.match_end or args.overtime or args.thaw or args.damage_through or args.poi or args.frozen or args.round_result or args.race or args.overhead_names:
            parser.error('--item-timers uses its own spectator fixture')
        args.combat, args.factory = True, 'tdm'
    if args.thaw or args.frozen:
        args.factory = 'ft'
    if args.round_result:
        args.factory = 'rr' if args.round_result == 'survivor' else 'ca'
    if args.race:
        args.factory, args.map = 'race', 'campgrounds'
    if args.clock_hud:
        args.factory = 'ffa' if args.clock_hud == 'match' else 'ca'
    if args.pregame_hud:
        args.factory = args.pregame_hud
    event_gate = args.flag_track or args.item_timers or args.match_end or args.overtime or args.thaw or args.damage_through or args.poi or args.frozen or args.round_result or args.race or args.weapon_selection or args.clock_hud or args.pregame_hud
    # Plain combat waits for the first client-observed kill instead of a fixed
    # window: bots on campgrounds sometimes go 40 s without one.
    combat_kill_gate = args.combat and not event_gate and not (args.gib_watch or args.record_demo or args.overhead_names)
    event_gate = event_gate or args.gib_watch
    event_gate = event_gate or combat_kill_gate
    target_event = ('EV_GIB_PLAYER' if args.gib_watch else 'EV_OBITUARY' if combat_kill_gate else 'EV_GLOBAL_TEAM_SOUND' if args.flag_track else 'EV_ITEM_PICKUP_SPEC' if args.item_timers else 'active clock' if args.clock_hud else 'weapon selection config' if args.weapon_selection else 'EV_RACE_START' if args.race_markers else 'EV_RACE_FINISH' if args.race else 'EV_GLOBAL_TEAM_SOUND' if args.round_result else 'EV_MISSILE_MISS_DMGTHROUGH' if args.damage_through else 'EV_POI' if args.poi else
                    'EV_THAW_PLAYER' if args.thaw else 'EV_OVERTIME' if args.overtime else
                    'EV_OBITUARY' if args.frozen else 'EV_GAMEOVER')
    target_pattern = (r'cs 662 "[1-9]\d*"' if args.clock_hud in ('round','timeout','timein') else r'g_gameState\\IN_PROGRESS' if args.clock_hud else r'(?m)^fnql-weapon-probe-ready\s*$' if args.weapon_selection else r'g_gameState\\IN_PROGRESS[\s\S]*EV_OBITUARY\s*FnQLFrozenProbe\^7' if args.frozen else
                      r'g_gameState\\IN_PROGRESS[\s\S]*event:\s*\d+\s+EV_RACE_START' if args.race_markers else
                      r'cs 662 "[1-9]\d*"[\s\S]*cs 662 "-1"[\s\S]*event:\s*\d+\s+EV_GLOBAL_TEAM_SOUND' if args.round_result else
                      r'event:\s*\d+\s+' + target_event)
    if args.match_end and args.factory in ('tdm','ca','ctf','1fctf','har','ft','dom','ad'):
        # Team modes announce the limit; no client-side EV_GAMEOVER is logged.
        target_event='team match end'
        target_pattern=r'hit the (?:fraglimit|capturelimit|scorelimit|roundlimit)|event:\s*\d+\s+EV_GAMEOVER'
    if args.pregame_hud:
        target_event='warmup countdown'
        target_pattern=r'cs 5 "\\time\\[1-9]\d*'
    if args.starting_loadout and not args.status_hud:
        parser.error('--starting-loadout requires --status-hud')
    if args.status_hud:
        if (event_gate and not args.weapon_selection) or args.combat:
            parser.error('--status-hud requires its standalone factory fixture')
        args.factory = 'fnql_test_weaponbar' if args.starting_loadout else 'fnql_test_status_hud'
    client = args.client.resolve(strict=True)
    module = None if args.retail_cgame else args.module.resolve(strict=True)
    retail = find_retail_install(args.retail)
    scratch = ROOT/'.tmp/cgame'
    scratch.mkdir(parents=True, exist_ok=True)
    home = Path(tempfile.mkdtemp(prefix='probe-', dir=scratch)).resolve()
    process = None
    try:
        base = home/'baseq3'; base.mkdir()
        if module:
            # Windows loads cgamex86.dll; Linux i386 builds load cgamei386.so.
            shutil.copy2(module, base/('cgamex86.dll' if module.suffix.lower() == '.dll' else 'cgamei386.so'))
        if args.status_hud:
            scripts_dir = base/'scripts'; scripts_dir.mkdir()
            fixture = 'weaponbar.factories' if args.starting_loadout else 'status_hud.factories'
            shutil.copy2(ROOT/'tests/lockstep/factories'/fixture, scripts_dir/'status_hud.factories')
        if args.qagame:
            qagame_path = args.qagame.resolve(strict=True)
            shutil.copy2(qagame_path, base/('fnql_qagamex86.dll' if qagame_path.suffix.lower() == '.dll' else 'fnql_qagamei386.so'))
        if args.gore_standin:
            import zipfile
            sphere = None
            for pk3 in sorted((retail/'baseq3').glob('*.pk3')):
                with zipfile.ZipFile(pk3) as z:
                    if 'models/gibs/sphere.md3' in z.namelist():
                        sphere = z.read('models/gibs/sphere.md3'); break
            if not sphere:
                parser.error('--gore-standin needs models/gibs/sphere.md3 in the retail paks')
            gore = base/'dlc_gibs'; gore.mkdir()
            for part in ('skull','brain','chest','abdomen','intestine','arm','forearm','fist','leg','foot','juicer'):
                (gore/f'{part}.md3').write_bytes(sphere)
        if args.ui:
            ui_path = args.ui.resolve(strict=True)
            shutil.copy2(ui_path, base/('uix86.dll' if ui_path.suffix.lower() == '.dll' else 'uii386.so'))
        if args.play_demo:
            demos = base/'demos'; demos.mkdir()
            source = args.play_demo.resolve(strict=True)
            shutil.copy2(source, demos/('fnqlprobe' + source.suffix))
        if args.race:
            entities = base/'cgame-entities'; entities.mkdir()
            fixture = 'race_markers.ent' if args.race_markers else 'race_drop.ent'
            shutil.copy2(ROOT/'tests/lockstep/entities'/fixture, entities/'campgrounds.ent')
        map_command = 'devmap' if args.zoom or args.combat or args.overtime or args.round_result or args.race or args.clock_hud or args.pregame_hud else 'map'
        team = 'red' if args.factory in ('tdm','ca','ctf','1fctf','har','ft','dom','ad') else 'free'
        if args.overhead_names or args.item_timers or args.flag_track: team = 'spectator'
        commands = [f'{map_command} {args.map} {args.factory}', 'wait 180', f'team {team}', 'wait 180']
        if args.weapon_bar == 4 and not event_gate:
            # Capture within the switch timer after spawning, without issuing
            # weapon/input commands. Persistent layouts retain the normal wait.
            commands[-1] = 'wait 30'
        if args.race:
            commands[2:2] = ['set cg_debugEvents 1', 'set g_warmup 1',
                             'addbot sarge 1', 'wait 180', 'set bot_pause 1']
        if args.round_result:
            commands += ['set g_warmup 1', 'set g_roundWarmupDelay 1000',
                         'set roundtimelimit 3', 'set g_roundDrawHealthCount 0',
                         'set g_roundDrawLivingCount 0', 'set g_lastManStandingWarning 0',
                         'set cg_debugEvents 1', 'addbot sarge 4 blue', 'wait 180', 'set bot_pause 1']
        if args.clock_hud:
            commands += ['set g_warmup 1', 'set g_roundWarmupDelay 1000',
                         'set roundtimelimit 25', 'set timelimit 5', 'set fraglimit 0',
                         'addbot sarge 1' + (' blue' if args.clock_hud != 'match' else ''),
                         'wait 180', 'set bot_pause 1']
        if args.pregame_hud:
            commands += ['screenshotJPEG', 'set g_warmup 8',
                         'addbot sarge 1' + (' blue' if args.factory=='ca' else ''),
                         'wait 180', 'set bot_pause 1', 'screenshotJPEG', 'allready']
        if args.combat:
            if args.match_end:
                commands += ['set g_warmup 1', 'set fraglimit 1']
                if team == 'red':
                    # Team bots may never score; a one-minute limit always
                    # reaches the end-of-match scoreboard.
                    commands += ['set timelimit 1']
            if args.thaw:
                commands += ['set g_warmup 1', 'set g_freezeRoundDelay 0',
                             'set g_freezeAutoThawTime 1000', 'set g_freezeThawTime 1000']
            if args.frozen:
                commands += ['set g_warmup 1', 'set g_freezeAutoThawTime 120000',
                             'set cg_thirdPerson 1', 'set cg_thirdPersonRange 100']
            if args.damage_through:
                commands += ['set g_warmup 1', 'set fraglimit 0', 'set g_startingWeapons 17',
                             'set g_startingAmmo_rl 100', 'set g_forceDmgThroughSurface 1',
                             'set g_dmgThroughSurfaceAngularThreshold -1']
            if args.gib_watch:
                # A rocket on 20 health leaves the victim far below -40.
                commands += ['set fraglimit 0', 'set g_startingWeapons 17', 'set g_startingAmmo_rl 100']
            if args.poi:
                # g_quadHog is latched (set before the map); the server
                # spawns the hog quad itself and bots pick it up unaided.
                commands += ['set g_warmup 1', 'set fraglimit 0']
            commands += ['set cg_debugEvents 1']
            if args.thaw or args.frozen:
                commands += ['addbot sarge 4 red', 'addbot visor 4 blue',
                             'addbot anarki 4 red', 'addbot keel 4 blue']
            elif args.match_end and team == 'red':
                # Balanced teams with the local red player.
                commands += ['addbot sarge 4 red', 'addbot visor 4 blue', 'addbot keel 4 blue']
            else:
                commands += ['addbot sarge 4', 'addbot visor 4', 'addbot anarki 4', 'addbot keel 4']
            if args.frozen:
                # Bots may never reach the local player; after about 20 s
                # freeze it through the console kill, which still produces
                # its own EV_OBITUARY.
                commands += ['wait 2500', 'kill']
            if args.match_end and team == 'red':
                # Team modes hold warmup until everyone is ready.
                commands += ['wait 250', 'allready']
            if args.record_demo:
                commands += ['wait 1000', 'record fnqlprobe', 'wait 1500', 'stoprecord', 'wait 30']
            elif not event_gate:
                commands += ['wait 5400']
        if args.overtime:
            commands += ['set g_warmup 1', 'set fraglimit 0', 'set timelimit 1',
                         'set g_overtime 120', 'set cg_debugEvents 1', 'addbot sarge 4',
                         'wait 360', 'set bot_pause 1', 'set timescale 4']
        if args.starting_loadout:
            commands += ['g_loadout']
        if args.status_hud:
            commands += ['g_startingHealth', 'g_startingHealthBonus', 'g_startingArmor', 'g_startingAmmo_mg']
        if args.scoreboard:
            if args.reload_hud:
                commands += ['loadhud', 'wait 2'] * 8
                # A missing custom HUD must take the documented default path,
                # without dropping the connection or losing its scoreboards.
                commands += ['set cg_hudFiles ui/fnql_missing_probe_hud.txt', 'loadhud',
                             'set cg_hudFiles ui/hud.txt', 'wait 2']
            commands += ['+scores', 'wait 180']
        if args.stats:
            commands += ['+acc', 'wait 300', 'pstats', 'wait 30']
        if args.weapon_selection:
            commands += ['echo fnql-weapon-probe-ready']
        if args.play_demo:
            # Demo playback replaces hosting; the cgame draws the recorded match.
            commands = ['demo fnqlprobe', 'wait 600']
        finish = ['vminfo', 'viewpos', 'screenshotJPEG', 'wait 10']
        if args.gore_standin:
            finish.insert(0, 'modellist')
        if args.standings_hud:
            finish[0:0] = ['cg_redTeamName', 'cg_blueTeamName', 'name']
        if args.clock_hud == 'timeout':
            finish = ['pause', 'wait 125', 'screenshotJPEG', 'wait 250'] + finish
        if args.clock_hud == 'timein':
            finish = ['pause', 'wait 125', 'screenshotJPEG', 'timein', 'wait 125',
                      'screenshotJPEG', 'wait 625'] + finish
        if args.weapon_selection:
            finish.insert(0, 'cg_weaponPrimary')
        if args.third_person:
            # Warmup's map_restart resets cg_thirdPerson in both modules.
            finish = ['set cg_thirdPerson 1', 'set cg_thirdPersonAngle 90',
                      'set cg_thirdPersonRange 120', 'wait 2'] + finish
        if args.overhead_names:
            # Retail cg_specNames 2 names every player in view with bars.
            finish = ['follow 1', 'wait 250', 'screenshotJPEG', 'follow 2', 'wait 250'] + finish
        if args.item_timers:
            # Let the countdown run visibly before the capture.
            finish = ['wait 375'] + finish
        if args.flag_track:
            finish = ['wait 125'] + finish
        if args.zoom:
            # cg_zoomTime defaults to 150 ms; settle well past it.
            # A fixed devmap position makes the zoomed frames comparable.
            finish = ['setviewpos -856 472 26 -45', 'wait 30', '+zoom', 'wait 60'] + finish
            finish.insert(finish.index('wait 10') + 1, '-zoom')
        if args.frozen:
            finish = ['set cg_draw2D 0', 'set cg_thirdPersonAngle 180', 'wait 2'] + finish
        if args.overtime:
            finish.insert(0,'set timescale 1')
        if args.stats:
            finish += ['-acc']
        finish += ['echo cgame-probe-done', 'quit', '']
        finish.insert(0, 'echo cgame-probe-finishing')
        if event_gate:
            # Warmup queues map_restart at the end of the engine command
            # buffer. A long wait followed by quit ahead of it freezes the
            # server at that transition. Leave the buffer free, then finish
            # through authenticated loopback RCON after observing the event.
            (base/'finish.cfg').write_text('\n'.join(finish), encoding='ascii')
        else:
            commands += finish
        (base/'probe.cfg').write_text('\n'.join(commands), encoding='ascii')
        command = [str(client)]
        settings = {
            'fs_basepath': str(retail), 'fs_homepath': str(home), 'sv_pure': '0',
            'r_fullscreen': '0', 'r_mode': '-1', 'r_customwidth': '960', 'r_customheight': '600',
            'in_mouse': '0', 'in_nograb': '1', 'in_joystick': '0',
            's_initsound': '0', 'logfile': '2', 'developer': '1',
            'cl_allowDownload': '0', 'net_port': '28097', 'com_maxfps': '125',
            'vm_gameProvider': 'fnql' if args.qagame else 'retail',
        }
        if args.combat or args.overtime or args.round_result or args.race or args.clock_hud:
            settings.update({'g_doWarmup': '0', 'g_log': 'combat.log', 'g_logSync': '1'})
        if args.pregame_hud:
            settings.update({'g_doWarmup': '1', 'g_startWhenReady': '1', 'g_teamSizeMin': '1'})
        if args.race:
            settings.update({'sv_altEntDir': 'cgame-entities', 'g_gravity': '100',
                             'name': 'FnQLRaceProbe'})
        if args.race_markers:
            # Same requested FOV as the retained retail reference; exercise
            # the default retail aspect path without the legacy override.
            settings['cg_fov'] = '90'
        if args.weapon_bar is not None:
            settings['cg_weaponBar'] = str(args.weapon_bar)
        if args.full_weapon_bar:
            settings['cg_drawFullWeaponBar'] = '1'
        if args.weapon_selection:
            settings.update({'cg_weaponPrimaryQueued': 'rl',
                             'cg_preferredStartingWeapons': 'g mg',
                             'cg_weaponConfig_g': 'echo fnql-weapon-selected-gauntlet'})
        if args.ghost_weapon:
            settings['cg_drawGun'] = '3'
        if args.round_result == 'survivor':
            settings['g_rrInfected'] = '1'
        if args.poi:
            settings['g_quadHog'] = '1'
        if args.overhead_names:
            settings.update({'cg_specNames': '2', 'cg_overheadNamesWidth': '75'})
        if args.gib_watch:
            settings['g_startingHealth'] = '20'
        if args.item_timers:
            # Every timed item, including yellow armor (bit 8, off by default).
            settings['cg_specItemTimers'] = '15'
        if args.frozen:
            settings['name'] = 'FnQLFrozenProbe'
        if args.standings_hud:
            settings.update({'name': '^2FnQL standings long player name',
                             'cg_redTeamName': '^1Crimson', 'cg_blueTeamName': '^4Azure'})
        result_seen = finish_sent = 0.0
        if event_gate:
            rcon_key = secrets.token_hex(16)
            with socket.socket(socket.AF_INET, socket.SOCK_DGRAM) as port_probe:
                port_probe.bind(('127.0.0.1', 0))
                rcon_port = port_probe.getsockname()[1]
            settings.update({'net_ip': '127.0.0.1', 'net_port': str(rcon_port), 'rconPassword': rcon_key})
        for key, value in settings.items(): command += ['+set', key, value]
        command += ['+exec', 'probe.cfg']
        startup = None
        if sys.platform == 'win32':
            startup = subprocess.STARTUPINFO()
            startup.dwFlags = subprocess.STARTF_USESHOWWINDOW
            startup.wShowWindow = 4  # SW_SHOWNOACTIVATE; never control the user's input.
        process = subprocess.Popen(command, cwd=home, stdin=subprocess.DEVNULL,
                                   stdout=subprocess.DEVNULL, stderr=subprocess.DEVNULL,
                                   startupinfo=startup)
        deadline = time.monotonic() + args.timeout
        log = ''
        while process.poll() is None and time.monotonic() < deadline:
            time.sleep(0.25)
            path = base/'qconsole.log'
            if path.is_file():
                log = path.read_text(encoding='utf-8', errors='replace')
                if 'Server fatal crashed:' in log or '********************' in log:
                    break
                if event_gate and re.search(target_pattern, log):
                    now = time.monotonic()
                    if not result_seen:
                        result_seen = now
                    if now - result_seen >= (3 if args.race_markers or args.clock_hud else 2 if args.match_end or args.overtime else 1 if args.pregame_hud else .5 if args.frozen else 0) and now - finish_sent > 1 and 'execing finish.cfg' not in log and 'cgame-probe-finishing' not in log:
                        with socket.socket(socket.AF_INET, socket.SOCK_DGRAM) as control:
                            control.sendto(b'\xff'*4 + f'rcon {rcon_key} exec finish.cfg\n'.encode('ascii'),
                                           ('127.0.0.1', rcon_port))
                        finish_sent = now
        timed_out = process.poll() is None
        if timed_out and args.race:
            # Keep an engine-rendered failure view to diagnose the fixture.
            with socket.socket(socket.AF_INET, socket.SOCK_DGRAM) as control:
                control.sendto(b'\xff'*4 + f'rcon {rcon_key} exec finish.cfg\n'.encode('ascii'),
                               ('127.0.0.1',rcon_port))
            try: process.wait(timeout=3)
            except subprocess.TimeoutExpired: pass
        if process.poll() is None: process.kill()
        process.wait(timeout=10)
        path = base/'qconsole.log'
        if path.is_file(): log = path.read_text(encoding='utf-8', errors='replace')
        identity = 'VM_LoadDll(cgame) found native exports' if args.retail_cgame else 'VM_LoadDll(cgame): FnQL-game'
        required = [identity, 'Going from CS_PRIMED to CS_ACTIVE',
                    'cgame-probe-done']
        if args.play_demo:
            # Demo playback logs the file it opened instead of a connection.
            required[required.index('Going from CS_PRIMED to CS_ACTIVE')] = 'Demo file: demos/fnqlprobe.'
        failures = [f'missing {s}' for s in required if s not in log]
        if timed_out: failures.append('client did not exit normally')
        if process.returncode: failures.append(f'client exit {process.returncode}')
        if any(s in log for s in ['ERROR:', '********************', 'Rejected DLL']): failures.append('engine/module error')
        if not list((base/'screenshots').glob('*.jpg')): failures.append('no engine screenshot')
        if args.overhead_names and len(list((base/'screenshots').glob('*.jpg'))) < 2:
            failures.append('overhead names require two followed views')
        if args.clock_hud == 'timeout' and len(list((base/'screenshots').glob('*.jpg'))) < 2:
            failures.append('timeout comparison requires two engine screenshots')
        if args.clock_hud == 'timein':
            if len(list((base/'screenshots').glob('*.jpg'))) < 3:
                failures.append('timein comparison requires three engine screenshots')
            if not re.search(r'cs 670 "[1-9]\d*"[\s\S]*cs 669 "(?:0)?"',log):
                failures.append('server did not complete timein')
        if args.pregame_hud and len(list((base/'screenshots').glob('*.jpg'))) < 3:
            failures.append('pregame comparison requires waiting, ready-up and countdown captures')
        if args.starting_loadout and not re.search(r'"g_loadout" is:"1(?:\^7)?"',log):
            failures.append('limited starting loadout was not enabled')
        if args.weapon_selection and not re.search(r'"cg_weaponPrimary" is:"5(?:\^7)?"',log):
            failures.append('queued primary was not published at spawn')
        if args.weapon_selection and not re.search(r'(?m)^fnql-weapon-selected-gauntlet\s*$',log):
            failures.append('preferred spawn weapon did not dispatch its config')
        if args.status_hud:
            for name, value in (('g_startingHealth',100),('g_startingHealthBonus',50),
                                ('g_startingArmor',150),('g_startingAmmo_mg',23)):
                if not re.search(r'"'+name+r'" is:"'+str(value)+r'(?:\^7)?"',log):
                    failures.append(f'status HUD factory missing {name}={value}')
        if args.standings_hud:
            for name, value in (('cg_redTeamName','^1Crimson'),('cg_blueTeamName','^4Azure')):
                if not re.search(r'"'+name+r'" is:"'+re.escape(value)+r'(?:\^7)?"',log):
                    failures.append('standings label override missing: '+name)
            if args.standings_hud == 'individual' and not re.search(r'cs 659 "[^"\n]+"',log):
                failures.append('missing first-place name configstring')
        if args.clock_hud in ('timeout','timein') and not re.search(r'cs 669 "[1-9]\d*"',log):
            failures.append('server timeout did not start')
        if args.clock_hud in ('timeout','timein') and re.search(r'Unknown client game command: pcp\b',log):
            failures.append('pause notice command rejected')
        if args.clock_hud in ('timeout','timein'):
            notices = ['has paused the match']
            if args.clock_hud == 'timein': notices.append('has called timein')
            for notice in notices:
                if not any(line.endswith(notice) and 'serverCommand:' not in line for line in log.splitlines()):
                    failures.append('missing console notice: '+notice)
        if args.race:
            for event in (('EV_RACE_START',) if args.race_markers else ('EV_RACE_START','EV_RACE_CHECKPOINT','EV_RACE_FINISH')):
                if not re.search(r'event:\s*\d+\s+'+event,log): failures.append('no '+event)
            if 'Loaded entity override cgame-entities/campgrounds.ent (' not in log:
                failures.append('race entity fixture not loaded')
            if 'race_init' not in log or re.search(r'(?:Unknown client game command:|Ignored malformed) race_',log):
                failures.append('race command negotiation failed')
            if not args.race_markers and not re.search(r'FnQLRaceProbe\^7 finished the race',log):
                failures.append('local runner did not finish')
            if not args.race_markers and not re.search(r'EV_RACE_FINISH[\s\S]*clientCommand: FnQLRaceProbe : \d+ : raceinit',log):
                failures.append('local cgame did not reset the completed run')
        if args.scoreboard:
            if args.reload_hud and 'menu file not found: ui/fnql_missing_probe_hud.txt, using default' not in log:
                failures.append('HUD fallback was not exercised')
            if not re.search(r'serverCommand:.*\b(?:scores(?:_\w+)?|smscores)\s+\d+', log):
                failures.append('no scoreboard response')
            if re.search(r'(?:Unknown client game command:|Ignored malformed) (?:scores|smscores)', log):
                failures.append('scoreboard command rejected')
        if args.gore_standin:
            for model in ('dlc_gibs/skull.md3', 'dlc_gibs/foot.md3', 'dlc_gibs/juicer.md3'):
                if model not in log:
                    failures.append(f'gore DLC model {model} not registered')
        if args.record_demo:
            recorded = sorted(base.glob('demos/fnqlprobe.*'))
            if not recorded:
                failures.append('no demo recorded')
            else:
                args.record_demo.parent.mkdir(parents=True, exist_ok=True)
                shutil.copy2(recorded[0], args.record_demo.with_suffix(recorded[0].suffix))
        if args.combat and not (args.record_demo or args.damage_through or args.poi or args.overhead_names or args.item_timers or args.flag_track):
            # cg_debugEvents is cheat-protected, hence the private devmap.
            # The client must actually consume an obituary, not merely load bots.
            if not re.search(r'event:\s*\d+\s+EV_OBITUARY', log):
                failures.append('no client-observed combat kill')
        if event_gate:
            if not re.search(target_pattern, log):
                failures.append('no client-observed '+target_event)
        if args.poi:
            if 'Unknown event' in log:
                failures.append('cgame rejected a server event')
            if not re.search(r'is the \^5Quad Hog!', log):
                failures.append('no Quad Hog pickup notice')
        if args.stats:
            for reply in ('acc', 'pstats'):
                payloads = re.findall(r'serverCommand:.*?\b'+reply+r'\s+([-\d ]+)', log)
                if not payloads or any(len(payload.split()) != 15 for payload in payloads):
                    failures.append(f'missing or invalid {reply} reply')
                if reply == 'acc' and len(payloads) < 2:
                    failures.append('accuracy panel did not refresh')
            if team == 'red' and not re.search(r'serverCommand:.*\btinfo\s+\d+', log):
                failures.append('no team-info reply')
            if re.search(r'(?:Unknown client game command:|Ignored malformed) (?:acc|pstats|tinfo)\b', log):
                failures.append('player/team statistics command rejected')
        if args.evidence_dir:
            evidence = args.evidence_dir.resolve()
            evidence.mkdir(parents=True, exist_ok=True)
            (evidence/'qconsole.log').write_text(log, encoding='utf-8')
            # Engine filenames can share a timestamp; suffix order is not
            # chronological (e.g. '-001.jpg' sorts before '.jpg').
            captures = sorted((base/'screenshots').glob('*.jpg'),key=lambda p:(p.stat().st_mtime_ns,p.name))
            if captures:
                shutil.copy2(captures[-1], evidence/'capture.jpg')
                if args.overhead_names and len(captures) >= 2:
                    shutil.copy2(captures[-2], evidence/'capture-first.jpg')
                if args.clock_hud in ('timeout','timein') and len(captures) >= 2:
                    shutil.copy2(captures[0], evidence/'capture-before.jpg')
                if args.clock_hud == 'timein' and len(captures) >= 3:
                    shutil.copy2(captures[1], evidence/'capture-resuming.jpg')
                if args.pregame_hud and len(captures)>=3:
                    shutil.copy2(captures[0],evidence/'capture-waiting.jpg')
                    shutil.copy2(captures[1],evidence/'capture-ready.jpg')
            print(f'Evidence: {evidence}')
        if failures:
            print('cgame hosting FAILED: '+', '.join(failures))
            print(log[-6500:])
            return 1
        print(f'cgame hosting OK: {args.map}/{args.factory}; '+('FnQL-game' if args.qagame else 'retail')+' qagame')
        return 0
    finally:
        if process and process.poll() is None:
            process.kill(); process.wait(timeout=10)
        if args.keep:
            print(f'Evidence: {home}')
        else:
            # Only this task's fresh, private directory; reject linked descendants.
            def linked(path):
                return path.is_symlink() or bool(getattr(path.lstat(), 'st_file_attributes', 0)
                                                & stat.FILE_ATTRIBUTE_REPARSE_POINT)
            if home.parent != scratch.resolve() or linked(home):
                raise RuntimeError('unsafe probe cleanup path')
            if any(linked(p) for p in home.rglob('*')):
                raise RuntimeError('linked probe output; preserving for inspection')
            shutil.rmtree(home)

if __name__ == '__main__':
    sys.exit(main())
