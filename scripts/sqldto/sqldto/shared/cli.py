import argparse
import pathlib

from sqldto.shared.types import models
from sqldto.shared.utils import rendering


def parse_args() -> argparse.Namespace:
    parser = argparse.ArgumentParser()
    parser.add_argument(
        '--namespace',
        type=str,
        required=True,
        help='c++ namespace to use',
    )
    parser.add_argument(
        '--output-dir',
        type=pathlib.Path,
        required=True,
        help='path to the directory with .hpp-.cpp files to generate',
    )
    parser.add_argument(
        '--dump-dir',
        type=pathlib.Path,
        required=True,
        help='path to the directory with schema dumps',
    )
    parser.add_argument(
        '--dialect',
        type=models.Dialect.parse,
        default=models.Dialect.postgresql,
        choices=[models.Dialect.postgresql],
        required=False,
        help='dialect to use (default: postgresql)',
    )
    parser.add_argument(
        '--migrations-dir',
        type=pathlib.Path,
        required=True,
        help='path to the directory with migration files',
    )
    parser.add_argument(
        '--migrations-output-dir',
        type=pathlib.Path,
        required=False,
        help='path to the directory for code-generated migration files',
    )
    parser.add_argument(
        '--queries-dir',
        type=pathlib.Path,
        required=False,
        help='path to the directory with queries files',
    )
    parser.add_argument(
        'queries',
        nargs='*',
        help='input query files to process (relative to the cwd, not source-dir)',
    )
    parser.add_argument(
        '--clang-format',
        type=str,
        required=False,
        help='clang-format binary name. Set to empty for no formatting',
    )
    args = parser.parse_args()
    return args


def make_context(
    args: argparse.Namespace,
    dump_command: str = '',
) -> rendering.Context:
    return rendering.Context(
        namespace=args.namespace,
        dialect=args.dialect,
        output_dir=args.output_dir,
        dump_dir=args.dump_dir,
        migrations_dir=args.migrations_dir,
        migrations_output_dir=args.migrations_output_dir,
        queries_dir=args.queries_dir,
        query_files=list(args.queries),
        clang_format=args.clang_format,
        dump_command=dump_command,
    )
