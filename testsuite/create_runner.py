import argparse
import pathlib
import pprint
import sys

TEMPLATE = """#!{python}
import sys

import pytest

TESTSUITE_PYTHONPATH = {python_path}
TESTSUITE_PYTEST_ARGS = {pytest_args}


def testsuite_runner():
    args = [
        *TESTSUITE_PYTEST_ARGS,
        *sys.argv[1:],
    ]
    sys.path.extend(TESTSUITE_PYTHONPATH)
    return pytest.main(args=args)


if __name__ == '__main__':
    sys.exit(testsuite_runner())
"""


def cmake_list(value):
    return value.split(';')


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument(
        '-o',
        '--output',
        type=pathlib.Path,
        help='Output script path',
        required=True,
    )
    parser.add_argument(
        '--python',
        help='Path to Python binary',
        type=pathlib.Path,
        default=sys.executable,
    )
    parser.add_argument(
        '--python-path',
        type=cmake_list,
        default=[],
        help='Semicolon separated Python path',
    )
    parser.add_argument(
        'pytest_args', nargs='*', help='Extra pytest arguments',
    )
    args = parser.parse_args()

    if not args.python.exists():
        parser.exit(
            status=1,
            message=f'Invalid path to python interpreter: {args.python}\n',
        )

    script = TEMPLATE.format(
        python=args.python,
        python_path=pprint.pformat(args.python_path),
        pytest_args=pprint.pformat(args.pytest_args),
    )
    args.output.write_text(script)
    args.output.chmod(0o755)


if __name__ == '__main__':
    main()
