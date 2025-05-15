import os
import os.path
import re
import subprocess
import sys
import tempfile

from gen_gdb_printers_tests import generate_test_script
import pytest

gdb_executable = 'gdb'
try:
    import yatest.common as yc

    gdb_executable = yc.gdb_path()
except ImportError:
    pass
if gdb_exe := os.environ.get('GDB_BIN'):
    gdb_executable = gdb_exe


def get_paths_from_env(env_var):
    return list(filter(len, re.split(r'\:\s*', os.environ.get(env_var, ''), re.MULTILINE)))


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
    os.path.split(test_program)[-1]: (test_program, test_source)
    for test_program, test_source in zip(test_programs, test_sources)
}

test_programs_in_release = get_paths_from_env('TESTS_IN_RELEASE')
is_release_build = os.environ.get('BUILD_TYPE', 'DEBUG') not in ('DEBUG', 'DEBUGNOASSERTS', 'FASTDEBUG')

test_coredumps = get_paths_from_env('TESTS_COREDUMP')


@pytest.mark.parametrize(
    'test_key,test_coredump', [(key, False) for key in test_params] + [(key, True) for key in test_coredumps]
)
def test_gdb_printers(capsys: pytest.CaptureFixture[str], test_key: str, test_coredump: str):
    test_program, test_source = test_params[test_key]
    if is_release_build and test_key not in test_programs_in_release:
        pytest.skip(f'Test {test_key} is skipped in release build')
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
    generate_test_script(test_source, tester, test_coredump, benchmark_report)

    cmd = [
        gdb_executable,
        '-iex',
        f'add-auto-load-safe-path {test_program}',
        '--batch',
        '-x',
        tester,
        '-ex',
        'quit 1',
        test_program,
    ]
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
