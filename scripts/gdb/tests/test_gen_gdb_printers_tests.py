import ast
from pathlib import Path
import sys
import types

import pytest

if __package__:
    from .gen_gdb_printers_tests import _top
else:
    from gen_gdb_printers_tests import _top

UTASK_GDB_SCRIPT_FUNCTION_PREFIX = ('scripts/gdb/pretty_printers/cmd/utask/cmd.py', 'utask_')
UNIVERSAL_GDB_SCRIPT_FUNCTION_PREFIXES = (
    ('scripts/gdb/pretty_printers/formats/json/printers.py', 'formats_json_'),
    ('scripts/gdb/pretty_printers/formats/yaml/printers.py', 'formats_yaml_'),
    ('scripts/gdb/pretty_printers/utils/fast_pimpl_printers.py', 'utils_fast_pimpl_'),
)


def get_source_path(path: str) -> Path:
    try:
        import yatest.common as yc

        return Path(yc.source_path(f'taxi/uservices/userver/{path}'))
    except ImportError:
        return Path(__file__).resolve().parents[3] / path


def test_test_command_preserves_runtime_error(capsys, monkeypatch):
    gdb = types.ModuleType('gdb')
    executed_commands = []
    gdb.execute = executed_commands.append
    monkeypatch.setitem(sys.modules, 'gdb', gdb)

    namespace = {}
    exec(_top.removesuffix('try:\n'), namespace)
    test_command = namespace['TEST_COMMAND']("raise AssertionError('original failure')")
    test_command.bp = types.SimpleNamespace(location='test.cpp:1')
    test_command()

    output = capsys.readouterr()
    assert 'Error: command execution failed:\n0: raise AssertionError' in output.err
    assert 'AssertionError: original failure' in output.out
    assert 'NameError' not in output.out + output.err
    assert executed_commands == ['quit 1']


def _assert_embedded_script_function_prefixes(script_path, function_prefix):
    source_path = get_source_path(script_path)
    module = ast.parse(source_path.read_text(encoding='utf-8'), filename=str(source_path))
    function_names = [node.name for node in module.body if isinstance(node, (ast.FunctionDef, ast.AsyncFunctionDef))]

    assert function_names
    assert len(function_names) == len(set(function_names)), function_names
    assert all(name.startswith(function_prefix) for name in function_names), function_names


def test_utask_embedded_script_function_prefixes():
    _assert_embedded_script_function_prefixes(*UTASK_GDB_SCRIPT_FUNCTION_PREFIX)


@pytest.mark.parametrize(('script_path', 'function_prefix'), UNIVERSAL_GDB_SCRIPT_FUNCTION_PREFIXES)
def test_universal_embedded_script_function_prefixes(script_path, function_prefix):
    _assert_embedded_script_function_prefixes(script_path, function_prefix)
