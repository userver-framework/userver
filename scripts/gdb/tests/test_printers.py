import os
import os.path
import re
import shutil
import stat
import subprocess
import sys
import tempfile

import pytest

if __package__:
    from .gen_gdb_printers_tests import generate_test_script
else:
    from gen_gdb_printers_tests import generate_test_script

gdb_executable = 'gdb'
try:
    import yatest.common as yc

    gdb_executable = yc.gdb_path()
except ImportError:
    pass
if gdb_exe := os.environ.get('GDB_BIN'):
    gdb_executable = gdb_exe


def get_paths_from_env(env_var):
    return list(filter(len, re.split(r':\s*', os.environ.get(env_var, ''))))


def run_objcopy(*args: str):
    command = ['objcopy', *args]
    result = subprocess.run(command, capture_output=True, text=True, check=False)
    if result.returncode != 0:
        raise AssertionError(
            f'Command {command!r} failed with exit code {result.returncode}\n'
            f'stdout:\n{result.stdout}\n'
            f'stderr:\n{result.stderr}'
        )


test_sources = get_paths_from_env('TEST_SOURCES')
test_programs = get_paths_from_env('TEST_PROGRAMS')
assert len(test_sources) == len(test_programs)

for i in range(len(test_programs)):
    if not os.path.exists(test_programs[i]):
        try:
            import yatest.common as yc

            test_programs[i] = yc.binary_path(test_programs[i])
        except ImportError:
            pass
    if not os.path.exists(test_sources[i]):
        try:
            import yatest.common as yc

            test_sources[i] = yc.source_path(test_sources[i])
        except ImportError:
            pass
    assert os.path.exists(
        test_programs[i],
    ), f'Test program {test_programs[i]} does not exist'
    assert os.path.exists(
        test_sources[i],
    ), f'Test source {test_sources[i]} does not exist'

test_params = {
    os.path.splitext(os.path.basename(test_source))[0]: (test_program, test_source)
    for test_program, test_source in zip(test_programs, test_sources, strict=True)
}

test_programs_in_release = get_paths_from_env('TESTS_IN_RELEASE')
build_type = os.environ.get('BUILD_TYPE', 'DEBUG').upper()
is_release_build = build_type not in ('DEBUG', 'DEBUGNOASSERTS', 'FASTDEBUG')

test_coredumps = get_paths_from_env('TESTS_COREDUMP')
userver_namespace = os.environ.get('USERVER_NAMESPACE', '')

gdb_init_script = os.environ.get('GDB_INIT_SCRIPT')
if gdb_init_script and not os.path.exists(gdb_init_script):
    try:
        import yatest.common as yc

        gdb_init_script = yc.source_path(gdb_init_script)
    except ImportError:
        pass
if gdb_init_script:
    assert os.path.exists(gdb_init_script), f'GDB initialization script {gdb_init_script} does not exist'


@pytest.mark.parametrize(
    'test_key,test_coredump,split_debug',
    [(key, False, False) for key in test_params]
    + [(key, True, False) for key in test_coredumps]
    + [
        (key, False, True)
        for key, (_, source) in test_params.items()
        if os.path.basename(source) == 'gdb_test_utask.cpp'
    ],
)
def test_gdb_printers(
    capsys: pytest.CaptureFixture[str],
    test_key: str,
    test_coredump: bool,
    split_debug: bool,
):
    test_program, test_source = test_params[test_key]
    cmd = ['objdump', '-h', '-j', '.debug_gdb_scripts', test_program]
    res = subprocess.run(cmd, capture_output=True, text=True, check=False)
    if res.returncode != 0 or '.debug_gdb_scripts' not in res.stdout:
        objdump_version = subprocess.run(['objdump', '--version'], capture_output=True, check=False)
        raise AssertionError(
            f'ELF binary {test_program} does not contain .debug_gdb_scripts. '
            'Link userver-gdb-scripts-section as a whole archive and retain the section during linker garbage collection. '
            f'objdump version: {objdump_version.stdout!r}'
        )

    if is_release_build and test_key not in test_programs_in_release:
        pytest.skip(f'Test {test_key} is skipped in release build')

    split_directory = None
    auto_load_safe_path = test_program
    if split_debug:
        split_directory = tempfile.TemporaryDirectory()
        split_program = os.path.join(split_directory.name, os.path.basename(test_program))
        split_debug_file = f'{split_program}.debug'
        shutil.copy2(test_program, split_program)
        os.chmod(split_program, stat.S_IMODE(os.stat(split_program).st_mode) | stat.S_IWUSR)
        run_objcopy('--only-keep-debug', split_program, split_debug_file)
        run_objcopy(
            '--strip-debug',
            '--remove-section=.gnu_debuglink',
            f'--add-gnu-debuglink={split_debug_file}',
            split_program,
        )
        test_program = split_program
        auto_load_safe_path = split_debug_file

    tester = tempfile.NamedTemporaryFile(
        'w',
        encoding='utf-8',
        delete=False,
        suffix='.py',
    ).name
    benchmark_report = tempfile.NamedTemporaryFile(
        'w',
        encoding='utf-8',
        delete=False,
        suffix='.log',
    ).name
    generate_test_script(test_source, tester, test_coredump, benchmark_report, userver_namespace)

    cmd = [
        gdb_executable,
        '-iex',
        'maint wait-for-index-cache',
        '-iex',
        f'add-auto-load-safe-path {auto_load_safe_path}',
        '-ex',
        'set confirm off',  # turn off confirmation on quit
        '-ex',
        'maint set internal-error backtrace on',
        '--batch',
        '-x',
        tester,
        '-ex',
        'quit 1',
        test_program,
    ]
    if gdb_init_script:
        cmd[1:1] = ['-ix', gdb_init_script]
    try:
        print(subprocess.check_output(cmd, stderr=subprocess.STDOUT).decode())
        with open(benchmark_report) as f:
            with capsys.disabled():
                sys.stderr.write(f.read())
    except subprocess.CalledProcessError as e:
        raise Exception(str(e) + '\n' + e.output.decode())
    finally:
        os.unlink(tester)
        os.unlink(benchmark_report)
        if split_directory:
            split_directory.cleanup()
