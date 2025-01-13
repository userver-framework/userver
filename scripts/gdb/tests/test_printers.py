import os
import os.path
import subprocess
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

test_sources = list(
    filter(
        lambda path: len(path),
        os.environ.get('TEST_SOURCES', '').split(':'),
    ),
)
test_programs = list(
    filter(
        lambda path: len(path),
        os.environ.get('TEST_PROGRAMS', '').split(':'),
    ),
)
assert len(test_sources) == len(test_programs)

for i in range(len(test_programs)):
    if not os.path.exists(test_programs[i]):
        try:
            import yatest.common as yc

            test_programs[i] = yc.binary_path(test_programs[i])
        except ImportError:
            pass
    assert os.path.exists(
        test_programs[i],
    ), f'Test program {test_programs[i]} does not exist'
    assert os.path.exists(
        test_sources[i],
    ), f'Test source {test_sources[i]} does not exist'

test_sources = {test_program: test_source for test_program, test_source in zip(test_programs, test_sources)}


@pytest.mark.parametrize('test_program', test_programs)
def test_gdb_printers(test_program):
    tester = tempfile.NamedTemporaryFile(
        'w',
        encoding='utf-8',
        delete=False,
        suffix='.py',
    ).name
    generate_test_script(test_sources[test_program], tester)
    cmd = [
        gdb_executable,
        '-iex',
        f'add-auto-load-safe-path {test_program}',
        '--batch',
        '-x',
        tester,
        test_program,
    ]
    try:
        print(subprocess.check_output(cmd, stderr=subprocess.STDOUT))
    except subprocess.CalledProcessError as e:
        raise Exception(str(e) + '\n' + e.output.decode())
    os.unlink(tester)
