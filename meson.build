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
  'code/game/g_names.c',
  'code/game/g_cmds.c',
  'code/game/g_vote.c',
  'code/game/g_admin.c',
  'code/game/g_commandextras.c',
  'code/game/g_race.c',
  'code/game/g_access.c',
  'code/game/g_drop.c',
  'code/game/g_flagphysics.c',
  'code/game/g_runes.c',
  'code/game/g_combat.c',
  'code/game/g_items.c',
  'code/game/g_quadhog.c',
  'code/game/g_rounds.c',
  'code/game/g_domination.c',
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

# Command policy and state transitions, without retail assets or a server.
game_drops = executable('game_drops', files('tests/game_drops.c', 'code/game/g_drop.c'),
  c_args: c_args + ['-DQAGAME', '-DMISSIONPACK'])
test('game-drops', game_drops, suite: 'unit')
game_flagphysics = executable('game_flagphysics', files('tests/game_flagphysics.c', 'code/game/g_flagphysics.c'),
  c_args: c_args + ['-DQAGAME', '-DMISSIONPACK'])
test('game-flagphysics', game_flagphysics, suite: 'unit')
game_runes = executable('game_runes', files('tests/game_runes.c', 'code/game/g_runes.c'),
  c_args: c_args + ['-DQAGAME', '-DMISSIONPACK'])
test('game-runes', game_runes, suite: 'unit')
game_names = executable('game_names', files('tests/game_names.c', 'code/game/g_names.c'),
  c_args: c_args + ['-DQAGAME', '-DMISSIONPACK'])
test('game-names', game_names, suite: 'unit')
game_quadhog = executable('game_quadhog', files('tests/game_quadhog.c', 'code/game/g_quadhog.c'),
  c_args: c_args + ['-DQAGAME', '-DMISSIONPACK'])
test('game-quadhog', game_quadhog, suite: 'unit')

game_domination = executable('game_domination', files('tests/game_domination.c', 'code/game/g_domination.c'),
  c_args: c_args + ['-DQAGAME', '-DMISSIONPACK'], dependencies: [math_dep])
test('game-domination', game_domination, suite: 'unit')

game_admin = executable('game_admin', files('tests/game_admin.c', 'code/game/g_admin.c', 'code/game/g_access.c',
  'code/game/g_commandextras.c', 'code/game/g_race.c'),
  c_args: c_args + ['-DQAGAME', '-DMISSIONPACK'])
test('game-admin', game_admin, suite: 'unit')
game_race = executable('game_race', files('tests/game_race.c', 'code/game/g_race.c'),
  c_args: c_args + ['-DQAGAME', '-DMISSIONPACK'])
test('game-race', game_race, suite: 'unit')


game_voting = executable('game_voting', files('tests/game_voting.c', 'code/game/g_vote.c',
  'code/game/q_shared.c', 'code/game/bg_lib.c'),
  c_args: c_args + ['-DQAGAME', '-DMISSIONPACK'], dependencies: [math_dep])
test('game-voting', game_voting, suite: 'unit')

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
  # Rename broadcasts and MG/HMG/CG impact metadata from a nonzero shooter,
  # through both box and capsule traces. Includes spectator recipients.
  foreach cylinders : ['0', '1']
    metadata_args = server_lockstep_args + ['--cheats', '--factory', 'tdm', '--frames', '600',
      '--set', 'g_playerCylinders', cylinders,
      '--scenario', files('tests/lockstep/server/tdm_event_metadata.txt')]
    test('server-lockstep-selfcheck-tdm_event_metadata' + cylinders, python,
      args: metadata_args + ['--self-check'], suite: 'differential', timeout: 300, is_parallel: false)
    test('server-lockstep-differential-tdm_event_metadata' + cylinders, python,
      args: metadata_args + ['--module', qagame.full_path()],
      depends: qagame, suite: 'differential', timeout: 300, is_parallel: false)
  endforeach
  names_args = server_lockstep_args + ['--factory', 'tdm', '--frames', '500',
    '--scenario', files('tests/lockstep/server/tdm_colored_names.txt')]
  test('server-lockstep-selfcheck-tdm_colored_names', python,
    args: names_args + ['--self-check'], suite: 'differential', timeout: 300, is_parallel: false)
  test('server-lockstep-differential-tdm_colored_names', python,
    args: names_args + ['--module', qagame.full_path()],
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
  # Auto actions (cg_autoAction demos and screenshots with retail's file
  # names), acc, pstats and ragequit, and following: a follower stays a
  # spectator and resumes following after the restart into the match.
  duel_commands_args = server_lockstep_args + ['--factory', 'duel', '--frames', '1300',
    '--scenario', files('tests/lockstep/server/duel_commands.txt')]
  test('server-lockstep-selfcheck-duel_commands', python, args: duel_commands_args + ['--self-check'],
    suite: 'differential', timeout: 300, is_parallel: false)
  test('server-lockstep-differential-duel_commands', python,
    args: duel_commands_args + ['--module', qagame.full_path()],
    depends: qagame, suite: 'differential', timeout: 300, is_parallel: false)
  # Public voting: grammar, electorate, actions and match transitions.
  foreach scenario, spec : {'voting_rules': ['ffa', '1820'],
                           'voting_actions': ['ffa', '1080'],
                           'voting_match': ['tdm', '1410'],
                           'voting_teams': ['tdm', '600'],
                           'voting_smallteams': ['tdm', '340']}
    vote_args = server_lockstep_args + ['--factory', spec[0], '--frames', spec[1],
      '--port', '28015', '--scenario', files('tests/lockstep/server/' + scenario + '.txt')]
    test('server-lockstep-selfcheck-' + scenario, python, args: vote_args + ['--self-check'],
      suite: 'differential', timeout: 300, is_parallel: false)
    test('server-lockstep-differential-' + scenario, python,
      args: vote_args + ['--module', qagame.full_path()],
      depends: qagame, suite: 'differential', timeout: 300, is_parallel: false)
  endforeach

  # A team deathmatch match: the warmup delay, team scoring and its sounds,
  # a team forfeit, and the intermission's team statistics.
  tdm_match_args = server_lockstep_args + ['--factory', 'tdm', '--frames', '1600',
    '--scenario', files('tests/lockstep/server/tdm_match.txt')]
  test('server-lockstep-selfcheck-tdm_match', python, args: tdm_match_args + ['--self-check'],
    suite: 'differential', timeout: 300, is_parallel: false)
  test('server-lockstep-differential-tdm_match', python,
    args: tdm_match_args + ['--module', qagame.full_path()],
    depends: qagame, suite: 'differential', timeout: 300, is_parallel: false)
  # Timeouts: refused and counted calls, a pause that holds a rocket, a quad,
  # an item timer, a respawn and the match clock, the admin command table
  # ("?", players, an unprivileged pause), and the caller's time in.
  # Shared administrative, chat and spectator command compatibility.
  foreach scenario, frames : {'tdm_admin': '1250', 'tdm_access': '70',
                              'tdm_commands': '70', 'tdm_shuffle': '100', 'tdm_shuffle_live': '1100'}
    admin_args = server_lockstep_args + ['--cheats', '--factory', 'tdm', '--frames', frames,
      '--port', '28025', '--scenario', files('tests/lockstep/server/' + scenario + '.txt')]
    test('server-lockstep-selfcheck-' + scenario, python, args: admin_args + ['--self-check'],
      suite: 'differential', timeout: 300, is_parallel: false)
    test('server-lockstep-differential-' + scenario, python,
      args: admin_args + ['--module', qagame.full_path()],
      depends: qagame, suite: 'differential', timeout: 300, is_parallel: false)
  endforeach

  # Race checkpoint records and RR command shuffles: retail determinism and parity.
  foreach scenario, frames : {'race_editor': '40', 'race_records': '280',
                              'rr_shuffle': '100', 'rr_shuffle_odd': '100', 'rr_join': '40', 'rr_shuffle_live': '1600'}
    mode = scenario.startswith('race') ? 'race' : 'rr'
    command_mode_args = server_lockstep_args + ['--cheats', '--factory', mode, '--frames', frames,
      '--port', '28025', '--scenario', files('tests/lockstep/server/' + scenario + '.txt')]
    if scenario == 'rr_shuffle_odd'
      command_mode_args += ['--seed', '7']
    endif
    if scenario == 'race_records'
      command_mode_args += ['--entities', files('tests/lockstep/entities/race_course.ent')]
    endif
    test('server-lockstep-selfcheck-' + scenario, python, args: command_mode_args + ['--self-check'],
      suite: 'differential', timeout: 300, is_parallel: false)
    test('server-lockstep-differential-' + scenario, python,
      args: command_mode_args + ['--module', qagame.full_path()],
      depends: qagame, suite: 'differential', timeout: 300, is_parallel: false)
  endforeach

  tdm_timeout_args = server_lockstep_args + ['--cheats', '--factory', 'tdm', '--frames', '1600',
    '--scenario', files('tests/lockstep/server/tdm_timeout.txt')]
  test('server-lockstep-selfcheck-tdm_timeout', python, args: tdm_timeout_args + ['--self-check'],
    suite: 'differential', timeout: 300, is_parallel: false)
  test('server-lockstep-differential-tdm_timeout', python,
    args: tdm_timeout_args + ['--module', qagame.full_path()],
    depends: qagame, suite: 'differential', timeout: 300, is_parallel: false)
  # Manual drops: inventory transfer, decay, expiry, and the timeout clock.
  foreach scenario, frames : {'tdm_drops': '1800', 'tdm_drop_timeout': '1500'}
    drop_args = server_lockstep_args + ['--cheats', '--factory', 'tdm', '--frames', frames,
      '--scenario', files('tests/lockstep/server/' + scenario + '.txt')]
    test('server-lockstep-selfcheck-' + scenario, python, args: drop_args + ['--self-check'],
      suite: 'differential', timeout: 300, is_parallel: false)
    test('server-lockstep-differential-' + scenario, python,
      args: drop_args + ['--module', qagame.full_path()],
      depends: qagame, suite: 'differential', timeout: 300, is_parallel: false)
  endforeach
  # Rune spawning, ownership, effects, combat, drops and relocation.
  foreach scenario, frames : {'tdm_runes': '2200', 'tdm_rune_combat': '1500', 'tdm_rune_timeout': '1800'}
    rune_args = server_lockstep_args + ['--cheats', '--factory', 'tdm', '--set', 'g_runes', '1',
      '--frames', frames, '--scenario', files('tests/lockstep/server/' + scenario + '.txt')]
    test('server-lockstep-selfcheck-' + scenario, python, args: rune_args + ['--self-check'],
      suite: 'differential', timeout: 300, is_parallel: false)
    test('server-lockstep-differential-' + scenario, python, args: rune_args + ['--module', qagame.full_path()],
      depends: qagame, suite: 'differential', timeout: 300, is_parallel: false)
  endforeach
  foreach scenario, extra : {'tdm_runes_tiered': ['--factory', 'tdm', '--set', 'armor_tiered', '1',
        '--frames', '2200', '--scenario', files('tests/lockstep/server/tdm_runes.txt')],
      'ctf_runes': ['--factory', 'ctf', '--map', 'spidercrossings', '--frames', '1800',
        '--scenario', files('tests/lockstep/server/ctf_runes.txt')],
      'ctf_rune_nodrop': ['--factory', 'ctf', '--map', 'spacectf', '--frames', '1000',
        '--scenario', files('tests/lockstep/server/ctf_rune_nodrop.txt')]}
    rune_args = server_lockstep_args + ['--cheats', '--set', 'g_runes', '1'] + extra
    test('server-lockstep-selfcheck-' + scenario, python, args: rune_args + ['--self-check'],
      suite: 'differential', timeout: 300, is_parallel: false)
    test('server-lockstep-differential-' + scenario, python, args: rune_args + ['--module', qagame.full_path()],
      depends: qagame, suite: 'differential', timeout: 300, is_parallel: false)
  endforeach
  foreach enabled : ['0', '1']
    rune_disconnect_args = server_lockstep_args + ['--cheats', '--factory', 'tdm', '--set', 'g_runes', '1',
      '--set', 'g_dropPowerups', enabled, '--frames', '1500',
      '--scenario', files('tests/lockstep/server/tdm_rune_disconnect.txt')]
    test('server-lockstep-selfcheck-tdm_rune_disconnect' + enabled, python,
      args: rune_disconnect_args + ['--self-check'], suite: 'differential', timeout: 300, is_parallel: false)
    test('server-lockstep-differential-tdm_rune_disconnect' + enabled, python,
      args: rune_disconnect_args + ['--module', qagame.full_path()],
      depends: qagame, suite: 'differential', timeout: 300, is_parallel: false)
  endforeach
  flag_drop_args = server_lockstep_args + ['--cheats', '--map', 'spidercrossings', '--factory', 'ctf',
    '--frames', '1000', '--scenario', files('tests/lockstep/server/ctf_dropflag.txt')]
  test('server-lockstep-selfcheck-ctf_dropflag', python, args: flag_drop_args + ['--self-check'],
    suite: 'differential', timeout: 300, is_parallel: false)
  test('server-lockstep-differential-ctf_dropflag', python,
    args: flag_drop_args + ['--module', qagame.full_path()],
    depends: qagame, suite: 'differential', timeout: 300, is_parallel: false)
  # Flag physics, including the default-off path, portal roll replication,
  # and Ironworks' fallback intermission spawn (no explicit intermission).
  foreach enabled : ['0', '1']
    physics_args = server_lockstep_args + ['--cheats', '--map', 'ironworks', '--factory', 'ctf',
      '--frames', '1000', '--set', 'g_flagPhysics', enabled,
      '--scenario', files('tests/lockstep/server/ctf_flagphysics.txt')]
    test('server-lockstep-selfcheck-ctf_flagphysics' + enabled, python, args: physics_args + ['--self-check'],
      suite: 'differential', timeout: 300, is_parallel: false)
    test('server-lockstep-differential-ctf_flagphysics' + enabled, python,
      args: physics_args + ['--module', qagame.full_path()],
      depends: qagame, suite: 'differential', timeout: 300, is_parallel: false)
  endforeach
  zone_args = server_lockstep_args + ['--cheats', '--factory', 'ctf', '--frames', '600',
    '--entities', files('tests/lockstep/entities/capturezone.ent'),
    '--scenario', files('tests/lockstep/server/ctf_capturezone.txt')]
  test('server-lockstep-selfcheck-ctf_capturezone', python, args: zone_args + ['--self-check'],
    suite: 'differential', timeout: 300, is_parallel: false)
  test('server-lockstep-differential-ctf_capturezone', python,
    args: zone_args + ['--module', qagame.full_path()],
    depends: qagame, suite: 'differential', timeout: 300, is_parallel: false)
  # Capture the flag: a capture, a return, the CTF scoreboard's partly hidden
  # team header, a team forfeit, and the end-of-match awards.
  ctf_match_args = server_lockstep_args + ['--cheats', '--map', 'spidercrossings', '--factory', 'ctf',
    '--frames', '1700', '--scenario', files('tests/lockstep/server/ctf_match.txt')]
  test('server-lockstep-selfcheck-ctf_match', python, args: ctf_match_args + ['--self-check'],
    suite: 'differential', timeout: 300, is_parallel: false)
  test('server-lockstep-differential-ctf_match', python,
    args: ctf_match_args + ['--module', qagame.full_path()],
    depends: qagame, suite: 'differential', timeout: 300, is_parallel: false)
  # Capture the flag bonuses: a carrier frag announced with the carrier's
  # powerup, base and carrier defense, a dropped flag's bounce, the give
  # cheat, powerup announcements, the lost-flag return at the intermission,
  # and every end-of-match award.
  ctf_defense_args = server_lockstep_args + ['--cheats', '--map', 'spidercrossings', '--factory', 'ctf',
    '--frames', '1800', '--scenario', files('tests/lockstep/server/ctf_defense.txt')]
  test('server-lockstep-selfcheck-ctf_defense', python, args: ctf_defense_args + ['--self-check'],
    suite: 'differential', timeout: 300, is_parallel: false)
  test('server-lockstep-differential-ctf_defense', python,
    args: ctf_defense_args + ['--module', qagame.full_path()],
    depends: qagame, suite: 'differential', timeout: 300, is_parallel: false)

  # Factory integration is a strict G3 gate, distinct from mode smoke tests.
  # Reject invalid names and disabled votes; accept a host-provided factory,
  # apply it through a passed vote, then retain it across map-only/restarts.
  factory_fixture = files('tests/lockstep/factories/custom.factories')
  foreach flags : ['0', '1', '8']
    factory_vote_args = server_lockstep_args + ['--port', '27994', '--factory', 'duel',
      '--frames', '80', '--set', 'g_voteFlags', flags,
      '--scenario', files('tests/lockstep/server/factory_votes.txt'), '--expect-factory', '79', 'duel']
    test('factory-votes-selfcheck-' + flags, python, args: factory_vote_args + ['--self-check'],
      suite: 'factory-contract', timeout: 300, is_parallel: false)
    test('factory-votes-differential-' + flags, python,
      args: factory_vote_args + ['--module', qagame.full_path()],
      depends: qagame, suite: 'factory-contract', timeout: 300, is_parallel: false)
  endforeach
  factory_custom_args = server_lockstep_args + ['--port', '27994', '--factory', 'duel',
    '--frames', '600', '--factories', factory_fixture,
    '--scenario', files('tests/lockstep/server/factory_custom.txt'),
    '--expect-factory', '0', 'duel', '--expect-factory', '200', 'fnql_test_duel',
    '--expect-factory', '350', 'fnql_test_duel', '--expect-factory', '499', 'fnql_test_duel',
    '--expect-factory', '599', 'duel']
  factory_map_only_args = server_lockstep_args + ['--port', '27994', '--factory', 'fnql_test_duel',
    '--frames', '280', '--factories', factory_fixture, '--set', 'g_voteFlags', '8',
    '--scenario', files('tests/lockstep/server/factory_map_only.txt'),
    '--expect-factory', '0', 'fnql_test_duel', '--expect-factory', '279', 'fnql_test_duel']
  foreach name, factory_args : {'custom': factory_custom_args, 'map-only': factory_map_only_args}
    test('factory-' + name + '-selfcheck', python, args: factory_args + ['--self-check'],
      suite: 'factory-contract', timeout: 300, is_parallel: false)
    test('factory-' + name + '-differential', python,
      args: factory_args + ['--module', qagame.full_path()],
      depends: qagame, suite: 'factory-contract', timeout: 300, is_parallel: false)
  endforeach

  # Isolated factory rules complement the complete round-mode scenarios.
  factory_gameplay = {
    'instagib': ['--factory', 'iffa', '--frames', '500'],
    'ictf': ['--factory', 'ictf', '--map', 'spidercrossings', '--frames', '1700'],
    'quadhog': ['--factory', 'quadhog', '--frames', '1600'],
    'quadhog_combat': ['--factory', 'quadhog', '--frames', '900', '--set', 'g_quadHogTime', '3'],
    'rules': ['--factory', 'ffa'],
    'health': ['--factory', 'ffa', '--set', 'g_startingHealth', '300',
      '--set', 'g_startingHealthBonus', '0', '--set', 'g_vampiricDamage', '0.75'],
  }
  foreach name, options : factory_gameplay
    gameplay_args = server_lockstep_args + ['--port', '27994', '--cheats',
      '--scenario', files('tests/lockstep/server/factory_' + name + '.txt')] + options
    test('factory-gameplay-' + name + '-selfcheck', python,
      args: gameplay_args + ['--self-check'], suite: 'factory-gameplay', timeout: 300, is_parallel: false)
    test('factory-gameplay-' + name + '-differential', python,
      args: gameplay_args + ['--module', qagame.full_path()], depends: qagame,
      suite: 'factory-gameplay', timeout: 300, is_parallel: false)
  endforeach

  # Strict retail round-mode gates: each replay first proves retail-twice
  # determinism, then compares all recorded state against the source module.
  round_scenarios = {
    'ca-combat': ['round_ca_combat', 'ca', '2100'],
    'ca-follow': ['round_ca_follow', 'ca', '2000'],
    'ca-timeout': ['round_timeout', 'ca', '2400'],
    'ca-limit': ['round_ca_combat', 'ca', '2100', '--set', 'roundlimit', '1'],
    'ca-draw': ['round_idle', 'ca', '2400', '--set', 'roundtimelimit', '5'],
    'ca-health-tiebreak': ['round_tiebreak', 'ca', '2100', '--set', 'roundtimelimit', '5'],
    'vca-combat': ['round_ca_combat', 'vca', '2200'],
    'ft-combat': ['round_combat', 'ft', '2100'],
    'ft-thaw': ['round_thaw', 'ft', '1800'],
    'ft-auto-thaw': ['round_thaw', 'ft', '1800', '--set', 'g_freezeAutoThawTime', '1000'],
    'ft-continuous': ['round_combat', 'ft', '2100', '--set', 'g_freezeRoundDelay', '0'],
    'ft-preserve-kit': ['round_combat', 'ft', '2100', '--set', 'g_freezeResetHealthOnRound', '0',
      '--set', 'g_freezeResetArmorOnRound', '0', '--set', 'g_freezeResetWeaponsOnRound', '0'],
    'ft-environment': ['round_ft_environment', 'ft', '2100'],
    'ft-warmup': ['round_ft_warmup', 'ft', '2100'],
    'ft-timeout': ['round_ft_timeout', 'ft', '2400'],
    'ift-thaw': ['round_thaw', 'ift', '1800'],
    'rr-combat': ['round_combat', 'rr', '2300'],
    'rr-limit': ['round_combat', 'rr', '2100', '--set', 'roundlimit', '1'],
    'rr-scoring': ['round_combat', 'rr', '2300', '--set', 'g_rrDamageScoreBonus', '2',
      '--set', 'g_rrRoundScoreBonus', '3', '--set', 'g_rrAllowNegativeScores', '1'],
    'infected-spread': ['round_infected', 'infected', '2300'],
    'infected-combat': ['round_infected_combat', 'infected', '2000'],
  }
  foreach name, options : round_scenarios
    round_args = server_lockstep_args + ['--port', '27994', '--cheats',
      '--scenario', files('tests/lockstep/server/' + options[0] + '.txt'),
      '--factory', options[1], '--frames', options[2]]
    foreach index : range(3, options.length())
      round_args += [options[index]]
    endforeach
    test('round-' + name + '-selfcheck', python, args: round_args + ['--self-check'],
      suite: 'round-modes', timeout: 300, is_parallel: false)
    test('round-' + name + '-differential', python,
      args: round_args + ['--module', qagame.full_path()], depends: qagame,
      suite: 'round-modes', timeout: 300, is_parallel: false)
  endforeach

  # Objective gameplay: compare full snapshots, events, commands and timing.
  objective_scenarios = {
    'ad-capture': ['ad_capture', 'ad', 'attack_defend', '2800'],
    'ad-combat': ['objective_combat', 'ad', 'attack_defend', '2400'],
    'ad-follow': ['round_ca_follow', 'ad', 'attack_defend', '2300'],
    'ad-timeout': ['round_timeout', 'ad', 'attack_defend', '2400'],
    'ad-draw': ['round_idle', 'ad', 'attack_defend', '2400', '--set', 'roundtimelimit', '3'],
    'ad-history': ['round_idle', 'ad', 'attack_defend', '7000', '--set', 'roundtimelimit', '1',
      '--set', 'g_roundWarmupDelay', '0'],
    'ad-limit': ['ad_limit', 'ad', 'attack_defend', '3600'],
    'ad-drop': ['ad_drop', 'ad', 'attack_defend', '3100'],
    'ad-death-drop': ['ad_death_drop', 'ad', 'attack_defend', '3300'],
    'dom-capture': ['domination', 'dom', 'domination', '2500'],
    'dom-contention': ['domination_contention', 'dom', 'domination', '2700'],
    'dom-neutral': ['domination_contention', 'dom', 'domination', '2700', '--set', 'g_domNeutralFlag', '1'],
    'dom-no-contention': ['domination_contention', 'dom', 'domination', '2700', '--set', 'g_domNeutralFlag', '1', '--set', 'g_domEnableContention', '0'],
    'dom-regen': ['domination_regen', 'dom', 'domination', '3100'],
    'dom-tiered-regen': ['domination_regen', 'dom', 'domination', '3100', '--set', 'armor_tiered', '1'],
    'dom-timeout': ['domination_timeout', 'dom', 'domination', '3100'],
    'dom-defense': ['domination_defense', 'dom', 'domination', '2200'],
    'dom-limit': ['domination_limit', 'dom', 'domination', '3600'],
    'dom-spawn-fallback': ['domination_spawn', 'dom', 'domination_spawns', '2600'],
    'dom-owned-spawn': ['domination_spawn', 'dom', 'domination_spawns', '2600', '--set', 'g_spawnMinDistance', '0'],
    'har-capture': ['objective_combat', 'har', 'objectives', '2200'],
    'har-drop': ['harvester_drop', 'har', 'objectives', '2400'],
    'har-no-drop': ['harvester_drop', 'har', 'objectives', '2400', '--set', 'g_dropSkulls', '0'],
    'har-expiry': ['objective_combat', 'har', 'objectives', '2200', '--set', 'g_cubeTimeout', '2'],
    'har-cap': ['harvester_cap', 'har', 'objectives', '2100'],
    'har-limit': ['harvester_limit', 'har', 'objectives', '3600'],
    'har-restart': ['harvester_restart', 'har', 'objectives', '2400'],
    'overload-combat': ['overload_combat', 'fnql_test_overload', 'objectives', '2400'],
    'overload-limit': ['overload_limit', 'fnql_test_overload', 'objectives', '3600'],
    'overload-timeout': ['overload_timeout', 'fnql_test_overload', 'objectives', '2600'],
  }
  foreach name, options : objective_scenarios
    objective_args = server_lockstep_args + ['--port', '27994', '--cheats',
      '--scenario', files('tests/lockstep/server/' + options[0] + '.txt'),
      '--factory', options[1], '--entities', files('tests/lockstep/entities/' + options[2] + '.ent'), '--frames', options[3]]
    if options[1] == 'fnql_test_overload'
      objective_args += ['--factories', files('tests/lockstep/factories/objectives.factories'),
        '--set', 'g_obeliskHealth', '400', '--set', 'g_obeliskRespawnDelay', '2']
    endif
    foreach index : range(4, options.length())
      objective_args += [options[index]]
    endforeach
    test('objective-' + name + '-selfcheck', python, args: objective_args + ['--self-check'],
      suite: 'objective-modes', timeout: 300, is_parallel: false)
    test('objective-' + name + '-differential', python,
      args: objective_args + ['--module', qagame.full_path()], depends: qagame,
      suite: 'objective-modes', timeout: 300, is_parallel: false)
  endforeach

  # Direct startup complements the objective limit replays' automatic rotation.
  foreach name, options : {
    'brimstoneabbey': ['brimstoneabbey', 'dom'],
    'devilish': ['devilish', 'ca'],
    'push-sounds': ['campgrounds', 'ffa'],
  }
    stock_args = server_lockstep_args + ['--port', '27994', '--map', options[0],
      '--factory', options[1], '--frames', '500',
      '--scenario', files('tests/lockstep/server/stock_map_init.txt')]
    if name == 'push-sounds'
      stock_args += ['--entities', files('tests/lockstep/entities/push_sounds.ent')]
    endif
    test('stock-map-' + name + '-selfcheck', python, args: stock_args + ['--self-check'],
      suite: 'stock-maps', timeout: 300, is_parallel: false)
    test('stock-map-' + name + '-differential', python,
      args: stock_args + ['--module', qagame.full_path()], depends: qagame,
      suite: 'stock-maps', timeout: 300, is_parallel: false)
  endforeach

  # Every retail factory must host FnQL-game without a fatal error, even on a
  # map without the mode's spawns or items (meson test --suite factories).
  # The divergence count is not the point here: the per-mode rules are
  # milestones 5 and 6.
  foreach factory : ['duel', 'race', 'ca', 'ctf', 'har', 'ft', 'dom', 'ad', 'rr', 'ift', 'infected', 'vca']
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
