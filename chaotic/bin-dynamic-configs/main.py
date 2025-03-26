#!/usr/bin/python3

import argparse
import pathlib
import sys

BINDIR = pathlib.Path(__file__).parent

if (BINDIR / '..' / 'chaotic' / 'main.py').exists():
    sys.path.append(str(BINDIR.parent))
else:
    sys.path.append(str(BINDIR.parent / 'lib' / 'userver'))

from chaotic.compilers import dynamic_config  # noqa: E402


def parse_args() -> argparse.Namespace:
    parser = argparse.ArgumentParser()
    parser.add_argument(
        '-o',
        '--output-dir',
        type=str,
        required=True,
        help='Directory to generate files in',
    )
    parser.add_argument(
        '-I',
        '--include-dir',
        action='append',
        help='Path to search for include files for x-usrv-cpp-type',
    )
    parser.add_argument(
        '--clang-format',
        default='clang-format',
        help=(
            'clang-format binary name. Can be either binary name in $PATH or '
            'full filepath to a binary file. Set to empty for no formatting.'
        ),
    )
    parser.add_argument(
        'file',
        type=str,
        nargs='+',
        help='yaml/json input filename',
    )
    return parser.parse_args()


def main():
    args = parse_args()
    for file in args.file:
        compiler = dynamic_config.Compiler()
        name = pathlib.Path(file).stem
        compiler.parse_variable(
            file,
            name,
            include_dirs=args.include_dir,
            namespace='dynamic_config',
        )
        compiler.generate_variable(
            name,
            args.output_dir,
            parse_extra_formats=True,
            generate_taxi_aliases=False,
            namespace='dynamic_config',
        )


if __name__ == '__main__':
    main()
