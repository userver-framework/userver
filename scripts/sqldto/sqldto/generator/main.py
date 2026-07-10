import logging
import sys

from sqldto.generator.postgres import generator
from sqldto.shared import cli
from sqldto.shared.types import errors
from sqldto.shared.utils import rendering


def dump_command() -> str:
    # Dumper requires more heavy dependencies than generator (psycopg2, testsuite)
    # So, dumper command is run in a separate interpreter, on user side
    DUMPER_PYTHON = 'python3'

    scripts_dir = rendering.ROOT_DIR
    requirements = scripts_dir / 'sqldto' / 'dumper' / 'requirements.txt'
    install_command = f'{DUMPER_PYTHON} -m pip install -r {requirements}'
    dump_command = ' '.join([DUMPER_PYTHON, '-m', 'sqldto.dumper.main', *sys.argv[1:]])
    return f'{install_command} && PYTHONPATH={scripts_dir} {dump_command}'


def plan(context: rendering.Context) -> list[rendering.ToGenerate]:
    if not context.schemas:
        context.schemas = context.fetcher.load_or_raise()

    return [
        *generator.PgModelsGenerator(context).generate(),
        *generator.PgQueriesGenerator(context).generate(),
    ]


def generate(context: rendering.Context) -> None:
    for to_generate in plan(context):
        to_generate.render()


def main() -> int:
    logging.basicConfig(level=logging.INFO, stream=sys.stdout)

    try:
        context = cli.make_context(cli.parse_args(), dump_command=dump_command())
        generate(context)
        return 0

    except errors.UnexpectedError:
        raise

    except errors.BaseError as e:
        print('#' * 80)
        print(e)
        print('#' * 80)
        return 1


if __name__ == '__main__':
    sys.exit(main())
