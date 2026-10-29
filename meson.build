project('fnql-game', 'c',
  version: '0.1.0',  # canonical value lives in version/fnql_game_version.h
  meson_version: '>=1.1.0',
  default_options: [
    'buildtype=release',
    'warning_level=1',
    'b_vscrt=static_from_buildtype',
  ])

# FnQL-game builds drop-in replacements for the retail Quake Live game modules.
# FnQL is currently 32-bit x86 only (its AGENTS.md constraint 9); further
# targets are enabled one at a time through its Project Plan step 7 and
# FnQL-game milestone 11, each behind its own gates.

cc = meson.get_compiler('c')
fs = import('fs')

x86_target_probe = '''
#if !defined(_M_IX86) && !defined(__i386__)
#error FnQL-game requires an x86 target
#endif
typedef char fnql_game_pointer_width_must_be_32_bits[(sizeof(void *) == 4) ? 1 : -1];
int main(void) { return 0; }
'''
if host_machine.cpu_family() != 'x86' or not cc.compiles(x86_target_probe, name: '32-bit x86 target')
  error('FnQL-game supports only 32-bit x86 targets; Meson configured host CPU family "'
    + host_machine.cpu_family() + '". Use a vcvars32 (Win32) environment on Windows or an i686 cross file elsewhere.')
endif

win32 = host_machine.system() == 'windows'
linux = host_machine.system() == 'linux'
if not (win32 or linux)
  error('FnQL-game targets the platforms retail Quake Live ships game modules for: Windows and Linux.')
endif

# Retail module names carry the engine's ARCH_STRING: x86 on Windows, i386 on
# Linux.  FnQL-game prefixes "fnql_" so its modules never shadow the retail
# ones in a search path; the FnQL engine selects them by provider.
module_arch = win32 ? 'x86' : 'i386'
module_prefix = 'fnql_'

c_args = []
link_args = []
# Movement must reproduce the retail x87 arithmetic bit for bit (gate G3):
# bg_pmove.c writes it as IEEE double expressions, so the compiler must
# neither fuse them into FMAs nor widen them to 80-bit x87 temporaries.
# MSVC's default /fp:precise already does neither.
if cc.get_argument_syntax() == 'msvc'
  c_args += [
    '-D_CRT_SECURE_NO_WARNINGS',
    '-DWIN32',
    '/wd4244', '/wd4305', '/wd4018',  # baseline float/int narrowing and signedness noise
  ]
else
  c_args += cc.get_supported_arguments([
    '-fno-strict-aliasing',
    '-fvisibility=hidden',
    '-Wno-unused-parameter',
    '-ffp-contract=off',
  ])
  if host_machine.cpu_family() == 'x86'
    c_args += cc.get_supported_arguments(['-msse2', '-mfpmath=sse'])
  endif
  link_args += cc.get_supported_link_arguments(['-Wl,--no-undefined'])
endif

math_dep = cc.find_library('m', required: false)

game_sources = files(
  'code/game/g_main.c',
  'code/game/ai_chat.c',
  'code/game/ai_cmd.c',
  'code/game/ai_dmnet.c',
  'code/game/ai_dmq3.c',
  'code/game/ai_main.c',
  'code/game/ai_team.c',
  'code/game/ai_vcmd.c',
  'code/game/bg_lib.c',
  'code/game/bg_misc.c',
  'code/game/bg_pmove_settings.c',
  'code/game/bg_pmove.c',
  'code/game/bg_slidemove.c',
  'code/game/g_active.c',
  'code/game/g_arenas.c',
  'code/game/g_bot.c',
  'code/game/g_client.c',
  'code/game/g_cmds.c',
  'code/game/g_combat.c',
  'code/game/g_items.c',
  'code/game/g_mem.c',
  'code/game/g_misc.c',
  'code/game/g_missile.c',
  'code/game/g_mover.c',
  'code/game/g_native.c',
  'code/game/g_ql_entities.c',
  'code/game/g_rotation.c',
  'code/game/g_session.c',
  'code/game/g_spawn.c',
  'code/game/g_svcmds.c',
  'code/game/g_target.c',
  'code/game/g_team.c',
  'code/game/g_trigger.c',
  'code/game/g_unlagged.c',
  'code/game/g_utils.c',
  'code/game/g_weapon.c',
  'code/game/q_math.c',
  'code/game/q_shared.c',
)

qagame_args = ['-DQAGAME', '-DMISSIONPACK']
if get_option('trace-rand')
  qagame_args += ['-DFNQL_TRACE_RAND']
endif

qagame = shared_library(module_prefix + 'qagame' + module_arch,
  game_sources,
  name_prefix: '',
  c_args: c_args + qagame_args,
  link_args: link_args,
  dependencies: [math_dep],
  gnu_symbol_visibility: 'hidden',
  install: true,
  install_dir: 'baseq3')

# Gate G0: shared structure layouts must match the FnQL engine headers.
python = find_program('python3', 'python')
fnql_root = get_option('fnql-root')
if fnql_root == ''
  fnql_root = meson.project_source_root() / '..' / 'FnQL'
endif
test('engine-abi-parity', python,
  args: [files('scripts/check_engine_abi.py'), '--fnql', fnql_root, '--cc', ' '.join(cc.cmd_array())],
  suite: 'abi')

# Gate G2: shared tables (items, enums, configstrings, spawn table, bot item
# indices) must match the retail wire contract in docs/contract/.
test('wire-contract', python,
  args: [files('scripts/check_wire_contract.py'), '--cc', ' '.join(cc.cmd_array())],
  suite: 'contract')

# The committed retail tables must still match the user's retail install.
# Skips (exit 77) when no install is found.
retail_table_args = [files('scripts/extract_retail_contract.py')]
if get_option('retail-ql') != ''
  retail_table_args += ['--retail', get_option('retail-ql')]
endif
test('retail-tables-current', python, args: retail_table_args, suite: 'contract')

# Gate G3: FnQL-game's Pmove runs in lockstep with the retail one.  The
# harness loads the retail Windows module and calls its Pmove through MSVC
# inline assembly, so it exists only for Windows x86 MSVC builds; it skips
# (exit 77) without a retail install.
if win32 and host_machine.cpu_family() == 'x86' and cc.get_id() == 'msvc'
  pmove_lockstep = executable('pmove_lockstep',
    files('tests/lockstep/pmove_lockstep.c', 'tests/lockstep/lockstep_world.c',
      'code/game/bg_lib.c', 'code/game/bg_misc.c', 'code/game/bg_pmove.c', 'code/game/bg_pmove_settings.c',
      'code/game/bg_slidemove.c', 'code/game/q_math.c', 'code/game/q_shared.c'),
    c_args: c_args + ['-DQAGAME', '-DMISSIONPACK'],
    dependencies: [math_dep])
  lockstep_args = [files('scripts/pmove_lockstep.py'), '--harness', pmove_lockstep.full_path()]
  if get_option('retail-ql') != ''
    lockstep_args += ['--retail', get_option('retail-ql')]
  endif
  test('pmove-lockstep', python, args: lockstep_args, depends: pmove_lockstep,
    suite: 'parity', timeout: 120)
endif

# Gate G1: boot the module on a retail map in an FnQL dedicated server.
if get_option('fnql-dedicated') != ''
  smoke_args = [files('scripts/smoke_dedicated.py'),
    '--engine', get_option('fnql-dedicated'),
    '--module', qagame.full_path()]
  if get_option('retail-ql') != ''
    smoke_args += ['--retail', get_option('retail-ql')]
  endif
  test('dedicated-smoke', python, args: smoke_args, depends: qagame,
    suite: 'runtime', timeout: 180, is_parallel: false)

  # Milestone 4: bots fight a sustained match with the combat code.
  soak_args = [files('scripts/combat_soak.py'),
    '--engine', get_option('fnql-dedicated'),
    '--module', qagame.full_path()]
  if get_option('retail-ql') != ''
    soak_args += ['--retail', get_option('retail-ql')]
  endif
  test('combat-soak', python, args: soak_args, depends: qagame,
    suite: 'runtime', timeout: 240, is_parallel: false)

  # Every stock map must load (G1 sweep); slow, so it is its own suite.
  sweep_args = [files('scripts/sweep_maps.py'),
    '--engine', get_option('fnql-dedicated'),
    '--module', qagame.full_path()]
  if get_option('retail-ql') != ''
    sweep_args += ['--retail', get_option('retail-ql')]
  endif
  test('map-sweep', python, args: sweep_args, depends: qagame,
    suite: 'sweep', timeout: 1200, is_parallel: false)

  # Gate G5b: the user's retail Steam client joins this server and stays in.
  # It launches the retail game through Steam, so it is its own opt-in suite:
  # meson test --suite retail-client.  Skips without Steam running.
  retail_client_args = [files('scripts/retail_client_probe.py'),
    '--engine', get_option('fnql-dedicated'),
    '--module', qagame.full_path()]
  if get_option('retail-ql') != ''
    retail_client_args += ['--retail', get_option('retail-ql')]
  endif
  test('retail-client-join', python, args: retail_client_args, depends: qagame,
    suite: 'retail-client', timeout: 400, is_parallel: false)

  # Gate G3: the full-server lockstep differential (meson test --suite
  # differential).  FnQL's lockstep dedicated server (FnQL
  # docs/fnql/SERVER_LOCKSTEP.md) hosts the retail qagame and FnQL-game's in
  # turn with the same map, seed and scripted clients, and every scripted
  # playerState, entity, configstring and server command is diffed frame by
  # frame.  Each scenario's self-check hosts the retail qagame twice and must
  # not diverge: it proves the rig is deterministic.  The differentials fail
  # until FnQL-game's game logic reaches retail parity (milestone M4).
  server_lockstep_args = [files('scripts/server_lockstep.py'),
    '--engine', get_option('fnql-dedicated')]
  if get_option('retail-ql') != ''
    server_lockstep_args += ['--retail', get_option('retail-ql')]
  endif
  foreach scenario : ['ffa_two_players', 'ffa_kill_restart', 'ffa_bot_restart']
    scenario_args = server_lockstep_args + [
      '--scenario', files('tests/lockstep/server/' + scenario + '.txt')]
    test('server-lockstep-selfcheck-' + scenario, python, args: scenario_args + ['--self-check'],
      suite: 'differential', timeout: 300, is_parallel: false)
    test('server-lockstep-differential-' + scenario, python,
      args: scenario_args + ['--module', qagame.full_path()],
      depends: qagame, suite: 'differential', timeout: 300, is_parallel: false)
  endforeach
  # Item pickups need setviewpos, a cheat: the scenario runs under devmap.
  items_args = server_lockstep_args + ['--cheats', '--frames', '1400',
    '--scenario', files('tests/lockstep/server/ffa_items.txt')]
  test('server-lockstep-selfcheck-ffa_items', python, args: items_args + ['--self-check'],
    suite: 'differential', timeout: 300, is_parallel: false)
  test('server-lockstep-differential-ffa_items', python,
    args: items_args + ['--module', qagame.full_path()],
    depends: qagame, suite: 'differential', timeout: 300, is_parallel: false)
  # Team deathmatch: loadouts, team locations and the team overlay, the team
  # scoreboard with its item-control header, and a team-game death and
  # forced respawn.
  tdm_args = server_lockstep_args + ['--cheats', '--factory', 'tdm',
    '--scenario', files('tests/lockstep/server/tdm_items.txt')]
  test('server-lockstep-selfcheck-tdm_items', python, args: tdm_args + ['--self-check'],
    suite: 'differential', timeout: 300, is_parallel: false)
  test('server-lockstep-differential-tdm_items', python,
    args: tdm_args + ['--module', qagame.full_path()],
    depends: qagame, suite: 'differential', timeout: 300, is_parallel: false)
  # Duel: the spectate-only flag and the play queue (so, pq, the teamtask
  # refresh), repeated and refused team requests, scores_duel, and a duelist
  # moving to the spectators.
  duel_args = server_lockstep_args + ['--factory', 'duel',
    '--scenario', files('tests/lockstep/server/duel_queue.txt')]
  test('server-lockstep-selfcheck-duel_queue', python, args: duel_args + ['--self-check'],
    suite: 'differential', timeout: 300, is_parallel: false)
  test('server-lockstep-differential-duel_queue', python,
    args: duel_args + ['--module', qagame.full_path()],
    depends: qagame, suite: 'differential', timeout: 300, is_parallel: false)
  # A played duel: ready-up, the countdown's attack lockout, the restart into
  # the match, a forfeit, the intermission with its map vote, and ExitLevel
  # (the run stops at the frame before the voted map loads).
  duel_match_args = server_lockstep_args + ['--factory', 'duel', '--frames', '1827',
    '--scenario', files('tests/lockstep/server/duel_match.txt')]
  test('server-lockstep-selfcheck-duel_match', python, args: duel_match_args + ['--self-check'],
    suite: 'differential', timeout: 300, is_parallel: false)
  test('server-lockstep-differential-duel_match', python,
    args: duel_match_args + ['--module', qagame.full_path()],
    depends: qagame, suite: 'differential', timeout: 300, is_parallel: false)
  # The forfeit command: refused in warmup and to the leader, then taken.
  forfeit_args = server_lockstep_args + ['--factory', 'duel', '--frames', '800',
    '--scenario', files('tests/lockstep/server/duel_forfeit.txt')]
  test('server-lockstep-selfcheck-duel_forfeit', python, args: forfeit_args + ['--self-check'],
    suite: 'differential', timeout: 300, is_parallel: false)
  test('server-lockstep-differential-duel_forfeit', python,
    args: forfeit_args + ['--module', qagame.full_path()],
    depends: qagame, suite: 'differential', timeout: 300, is_parallel: false)
  # A team deathmatch match: the warmup delay, team scoring and its sounds,
  # a team forfeit, and the intermission's team statistics.
  tdm_match_args = server_lockstep_args + ['--factory', 'tdm', '--frames', '1600',
    '--scenario', files('tests/lockstep/server/tdm_match.txt')]
  test('server-lockstep-selfcheck-tdm_match', python, args: tdm_match_args + ['--self-check'],
    suite: 'differential', timeout: 300, is_parallel: false)
  test('server-lockstep-differential-tdm_match', python,
    args: tdm_match_args + ['--module', qagame.full_path()],
    depends: qagame, suite: 'differential', timeout: 300, is_parallel: false)

  # Every retail factory must host FnQL-game without a fatal error, even on a
  # map without the mode's spawns or items (meson test --suite factories).
  # The divergence count is not the point here: the per-mode rules are
  # milestones 5 and 6.
  foreach factory : ['duel', 'race', 'ca', 'ctf', 'har', 'ft', 'dom', 'ad', 'rr']
    test('server-lockstep-factory-' + factory, python,
      args: server_lockstep_args + ['--cheats', '--factory', factory, '--max-divergent', '100000000',
        '--scenario', files('tests/lockstep/server/tdm_items.txt'), '--module', qagame.full_path()],
      depends: qagame, suite: 'factories', timeout: 300, is_parallel: false)
  endforeach
endif

# Gate G5a: the retail cgame runs against FnQL-game's qagame in one client.
if get_option('fnql-client') != ''
  probe_args = [files('scripts/listen_probe.py'),
    '--client', get_option('fnql-client'),
    '--module', qagame.full_path()]
  if get_option('retail-ql') != ''
    probe_args += ['--retail', get_option('retail-ql')]
  endif
  test('retail-cgame-probe', python, args: probe_args, depends: qagame,
    suite: 'interop', timeout: 240, is_parallel: false)

  # Gate G4a: the retail cgame's prediction of the local player must agree
  # with FnQL-game's server-side Pmove exactly (no "prediction error" at
  # all), with VQL and with PQL movement and its per-player options.
  prediction_args = [files('scripts/prediction_probe.py'),
    '--client', get_option('fnql-client'),
    '--module', qagame.full_path()]
  if get_option('retail-ql') != ''
    prediction_args += ['--retail', get_option('retail-ql')]
  endif
  test('retail-cgame-prediction', python, args: prediction_args, depends: qagame,
    suite: 'interop', timeout: 300, is_parallel: false)
  test('retail-cgame-prediction-pql', python,
    args: prediction_args + ['--set', 'pmove_AirControl', '1', '--set', 'pmove_CrouchSlide', '1',
      '--set', 'pmove_DoubleJump', '1'],
    depends: qagame, suite: 'interop', timeout: 300, is_parallel: false)
endif

summary({
  'target': host_machine.system() + ' ' + host_machine.cpu_family(),
  'qagame': fs.name(qagame.full_path()),
}, section: 'FnQL-game')
