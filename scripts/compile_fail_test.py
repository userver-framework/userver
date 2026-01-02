#!/usr/bin/python3

from collections.abc import Callable
from collections.abc import Iterable
import dataclasses
import json
import re
import subprocess
import sys


def make_compile_commands(path: str) -> list[dict]:
    output = subprocess.check_output(
        ['ya', 'dump', 'compile-commands'],
        cwd=path,
        encoding='utf-8',
        stderr=subprocess.DEVNULL,
    )
    return json.loads(output)


def read_compile_commands() -> list[dict]:
    with open('compile_commands.json') as ifile:
        return json.load(ifile)


def compile_command_yamake(cc_rule: dict) -> str:
    cmd = cc_rule['command']

    # clang++ ... -> ya tool c++ ...
    cmd = 'ya tool c++ ' + cmd.split(' ', 1)[1]

    # remove '-o xxx.cpp.o'
    cmd = re.sub(' -o [^ ]* ', ' -o /dev/null ', cmd)

    cmd = re.sub(' -fcolor', ' -fno-color', cmd)

    return cmd


def compile_command_cmake(cc_rule: dict) -> str:
    cmd = cc_rule['command']

    # remove '-o xxx.cpp.o'
    cmd = re.sub(' -o [^ ]* ', ' -o /dev/null ', cmd)

    cmd = re.sub(' -fcolor', ' -fno-color', cmd)

    return cmd


@dataclasses.dataclass
class Message:
    line: int
    text: str


FAIL_RE = re.compile(r'.*FAIL\(([^)]*)\).*')
FAIL_NEXT_RE = re.compile(r'.*FAILNEXTLINE\(([^)]*)\).*')


def read_file_messages(filename: str) -> list[Message]:
    result = []

    with open(filename) as ifile:
        for linenum, line in enumerate(ifile):
            match = FAIL_RE.match(line)
            if match:
                result.append(Message(line=linenum + 1, text=match.group(1)))

            match = FAIL_NEXT_RE.match(line)
            if match:
                result.append(Message(line=linenum + 2, text=match.group(1)))

    return result


def find_if(collection: Iterable, pred: Callable):
    for item in collection:
        if pred(item):
            return item
    return None


class CheckFailure(Exception):
    pass


def handle_rule(cc_rule: dict) -> None:
    cpp = cc_rule['file']

    cmd = compile_command_cmake(cc_rule)
    stderr, errors = compile_for_errors(cmd)

    asserts = []

    expected_msgs = read_file_messages(cpp)
    for msg in expected_msgs:
        # search for FAIL(...) line
        if not find_if(errors, lambda x: x.file == cpp and x.line == msg.line):
            asserts.append(f'Expected to get a compilation error/note at {cpp}:{msg.line}, but failed.')
            continue

        # search for FAIL(...) message
        if not find_if(errors, lambda x: msg.text in x.text):
            asserts.append(f'Expected to get a compilation error with text "{msg.text}", but failed.')

    if not asserts:
        return

    for line in asserts:
        print('error: ', line)

    print('\nexpected errors:')
    for msg in expected_msgs:
        print(f'  {cpp}:{msg.line}: {msg.text}')

    print('\nactual compiler output:')
    print(stderr)

    raise CheckFailure()


@dataclasses.dataclass
class CompilationMessage:
    file: str
    line: int
    level: str
    text: str

    orig_text: str


def compile_for_errors(cmd: str) -> tuple[str, list[CompilationMessage]]:
    proc = subprocess.run(
        cmd,
        shell=True,
        encoding='utf-8',
        capture_output=True,
    )

    result = []
    stderr = proc.stderr
    for line in stderr.splitlines():
        if not line.startswith('/'):
            continue

        parts = line.split(':', 4)

        try:
            linenum = int(parts[1])
        except ValueError:
            continue

        if len(parts) >= 5:
            level = parts[3].strip()
            text = parts[4].strip()
        else:
            level = '<none>'
            text = parts[3].strip()

        result.append(
            CompilationMessage(
                file=parts[0],
                line=linenum,
                level=level,
                text=text,
                orig_text=line,
            )
        )
    return stderr, result


def matches_prefix(rule, src_prefix):
    file = rule['file']
    if not file.startswith(src_prefix):
        return False

    return file.endswith('_compilefailtest.cpp')


def main() -> None:
    src_prefix = sys.argv[1]
    cc = read_compile_commands()

    status = 0
    for rule in cc:
        if not matches_prefix(rule, src_prefix):
            continue

        try:
            handle_rule(rule)
        except CheckFailure:
            status = 1
    return status


sys.exit(main())
