import argparse
import re
import subprocess
import sys

_top = """\
import gdb
import re
import sys
import traceback
import tempfile
import contextlib
import time


def gdb_print(expr):
    gdb.execute("set print elements 1000", to_string=True)
    output = gdb.execute('print %s' % expr, to_string=True)
    parts = output[:-1].split(' = ', 1)
    if len(parts) > 1:
        output = parts[1]
    else:
        output = parts[0]
    return output

def TEST_EXPR(expr, pattern, *args, **kwargs):
    def test():
        if args or kwargs:
            actual_args = [gdb_print(arg) for arg in args]
            actual_kwargs = dict([
                (k, gdb_print(v)) for (k,v) in kwargs.items() ])
            actual_pattern = pattern.format(*actual_args, **actual_kwargs)
        else:
            actual_pattern = pattern
        output = gdb_print(expr)
        try:
            if actual_pattern != output:
                print((
                    '{0}: error: expression "{1}" evaluates to\\n'
                    '{2}\\n'
                    'expected\\n'
                    '{3}\\n').format(
                        bp.location, expr, output, actual_pattern),
                    file=sys.stderr)
                gdb.execute('quit 1')
        except:
            raise
    return test

def assert_matches(pattern, text, flags=0):
    if not re.search(pattern, text, flags):
        print(
            'error: regexp does not match the text:\\n'
            f'{text}\\n'
            'regexp:\\n'
            f'{pattern}\\n',
            file=sys.stderr,
        )
        gdb.execute('quit 1')

def start_benchmark(tasks_cnt, threads_cnt, memory_per_task):
    print(f'Benchmarking utask cmd for {tasks_cnt=} {threads_cnt=} {memory_per_task=}:', file=benchmark_report)

@contextlib.contextmanager
def measure_time(text):
    start = time.time()
    yield
    end = time.time()
    print(f'{text} time: {end - start:.3f} s', file=benchmark_report)

def TEST_COMMAND(command, test_in_coredump=False):
    def test_command():
        errcmd = '\\n'.join(f'{num}: {line}' for num, line in enumerate(command.split('\\n')))
        try:
            exec(compile(command, f'{test_command.bp.location}', 'exec'))
        except SyntaxError as exc:
            print(f'Error: command is not a valid Python expression:\\n{errcmd}', file=sys.stderr)
            print(traceback.format_exc())
            gdb.execute('quit 1')
        except Exception:
            print(f'Error: command execution failed:\\n{errcmd}', file=sys.stderr)
            print(traceback.format_exc())
            gdb.execute('quit 1')
    test_command.test_in_coredump = test_in_coredump
    return test_command


def MAKE_COREDUMP_AND_SWITCH_TO():
    if not is_coredump:
        return lambda: None
    def command():
        corefile = tempfile.NamedTemporaryFile(delete=False, suffix='.core').name
        gdb.execute(f'gcore {corefile}', to_string=True)
        gdb.execute('kill', to_string=True)
        gdb.execute(f'core-file {corefile}', to_string=True)

        for test_command in _coredump_tests:
            test_command()

    return command

_return_code = 0
_tests_to_run = []
_coredump_tests = []
try:
"""

_benchmark_report = """
    benchmark_report = open('{benchmark_report}', 'w')
"""

_userver_namespace = """
    USERVER_NAMESPACE = {userver_namespace!r}
"""

_test_coredump = """
    is_coredump = {is_coredump}
"""

_breakpoint = """\
    test_command = {text}
    bp = gdb.Breakpoint('{input}:{line}', internal=True)
    test_command.bp = bp
    if is_coredump and hasattr(test_command, 'test_in_coredump') and test_command.test_in_coredump:
        _coredump_tests.append(test_command)
    else:
        last = len(_tests_to_run)
        _tests_to_run.append(test_command)
        bp.commands = 'py _tests_to_run[' + str(last) + ']()\\ncontinue'
"""

_bottom = """\
    gdb.execute('start', to_string=True)
    try:
        gdb.execute('continue', to_string=True)
    except gdb.error:
        pass

except BaseException:
    traceback.print_exc()
    gdb.execute('disable breakpoints')
    try:
        gdb.execute('continue')
    except:
        pass
    _return_code = 1

benchmark_report.close()
gdb.execute('quit %s' % _return_code)
"""


def parse_args(args):
    parser = argparse.ArgumentParser(
        prog=args[0],
        description=('Creates a Python script from C++ source file to control a GDB test of that source file'),
    )
    parser.add_argument('input', help='Input file')
    parser.add_argument(
        'output',
        nargs='?',
        help='Output file; STDOUT by default',
    )
    return parser.parse_args(args[1:])


def generate_test_script(
    input_path: str,
    output_path: str,
    is_coredump=False,
    benchmark_report: str | None = None,
    userver_namespace: str = '',
):
    regexp = re.compile(
        r'^\s*(TEST_EXPR|TEST_COMMAND|MAKE_COREDUMP_AND_SWITCH_TO)\s*\([^;]*\)', re.MULTILINE | re.DOTALL
    )

    with open(input_path, 'r', encoding='utf-8') as input:
        input_text = input.read()
    with open(output_path, 'w', encoding='utf-8') as output:
        output.write(_top)
        output.write(_userver_namespace.format(userver_namespace=userver_namespace))
        output.write(_benchmark_report.format(benchmark_report=benchmark_report))
        output.write(_test_coredump.format(is_coredump=is_coredump))
        for match in regexp.finditer(input_text):
            pos = match.start(1)
            line = input_text.count('\n', 0, pos) + 1
            expr = match.group().strip()
            # Expressions are compiled in the generated test script and then again by TEST_COMMAND.
            expr = expr.replace('[RE_DQSP]', '[\\\\t ]')
            expr = expr.replace('[RE_DQNSP]', '[\\\\t\\\\n ]')
            output.write(
                _breakpoint.format(input=input_path, line=line, text=expr),
            )
        output.write(_bottom)
    res = subprocess.run(['python3', '-m', 'py_compile', output_path], capture_output=True, text=True)
    if res.returncode != 0:
        print('Style stdout: ' + res.stdout, file=sys.stderr)
        print('Style stderr: ' + res.stderr, file=sys.stderr)
        with open(output_path, 'r', encoding='utf-8') as output:
            print(output.read(), file=sys.stderr)
        assert False, f'Failure detected within generated python test file from {input_path}'


def main(args, stdin, stdout):
    args = parse_args(args)
    generate_test_script(args.input, args.output)


if __name__ == '__main__':
    main(sys.argv, sys.stdin, sys.stdout)
