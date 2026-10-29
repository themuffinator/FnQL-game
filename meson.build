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
  ])
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

qagame = shared_library(module_prefix + 'qagame' + module_arch,
  game_sources,
  name_prefix: '',
  c_args: c_args + ['-DQAGAME', '-DMISSIONPACK'],
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

  # Every stock map must load (G1 sweep); slow, so it is its own suite.
  sweep_args = [files('scripts/sweep_maps.py'),
    '--engine', get_option('fnql-dedicated'),
    '--module', qagame.full_path()]
  if get_option('retail-ql') != ''
    sweep_args += ['--retail', get_option('retail-ql')]
  endif
  test('map-sweep', python, args: sweep_args, depends: qagame,
    suite: 'sweep', timeout: 1200, is_parallel: false)
endif

summary({
  'target': host_machine.system() + ' ' + host_machine.cpu_family(),
  'qagame': fs.name(qagame.full_path()),
}, section: 'FnQL-game')
