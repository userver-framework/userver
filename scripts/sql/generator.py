import argparse
import dataclasses
import enum
import os
import pathlib
from typing import List
from typing import NoReturn
from typing import Optional

import jinja2

PARENT_DIR = os.path.dirname(__file__)


class QueryLogMode(enum.Enum):
    FULL = 1
    NAME_ONLY = 2

    @staticmethod
    def parse(arg: str) -> 'QueryLogMode':
        if arg == 'full':
            return QueryLogMode.FULL
        elif arg == 'name-only':
            return QueryLogMode.NAME_ONLY
        else:
            raise Exception(f'Unknown query log mode: {arg}')


def parse_args():
    parser = argparse.ArgumentParser()
    parser.add_argument(
        '--namespace',
        type=str,
        required=True,
        help='C++ namespace to use',
    )
    parser.add_argument(
        '--output-dir',
        type=str,
        required=True,
        help='full path to output .hpp file',
    )
    parser.add_argument(
        '--query-log-mode',
        type=QueryLogMode.parse,
        choices=[QueryLogMode.FULL, QueryLogMode.NAME_ONLY],
        required=True,
        help='whether to log query text, "full" for "do log", "name-only" for "do not log"',
    )
    parser.add_argument(
        '--testsuite-output-dir',
        type=str,
        required=True,
        help='full path to generated tests',
    )
    parser.add_argument('files', nargs='*', help='input .sql files to process')
    return parser.parse_args()


@dataclasses.dataclass
class SqlQuery:
    source: str
    variable: str
    contents: str
    name: str


@dataclasses.dataclass
class SqlParams:
    namespace: str
    query_log_mode: QueryLogMode
    src_root: str
    testsuite_output_dir: str = ''


def camel_case(string: str) -> str:
    result = ''
    set_upper = True
    for char in string:
        if char in {'_', '-', '.'}:
            set_upper = True
        else:
            if set_upper:
                char = char.upper()
            else:
                char = char.lower()
            result += char
            set_upper = False
    return result


def read_items(args) -> List[SqlQuery]:
    items: List[SqlQuery] = []
    for filename in args.files:
        with open(filename, 'r') as file:
            content = file.read()
        name = pathlib.Path(filename).stem  # TODO: CamelCase
        items.append(
            SqlQuery(
                source=filename,
                contents=content,
                name=name,
                variable=('k' + camel_case(name)),
            ),
        )
    return items


def raise_error(*args) -> NoReturn:
    raise Exception(''.join(map(str, args)))


def render(
    params: SqlParams,
    sql_items: List[SqlQuery],
    yql_items: List[SqlQuery],
    env: Optional[jinja2.Environment] = None,
) -> None:
    os.makedirs(
        pathlib.Path(f'{params.src_root}/include/{params.namespace}'),
        exist_ok=True,
    )
    os.makedirs(
        pathlib.Path(f'{params.src_root}/src/{params.namespace}'),
        exist_ok=True,
    )

    if params.testsuite_output_dir:
        os.makedirs(
            pathlib.Path(params.testsuite_output_dir),
            exist_ok=True,
        )

    if not env:
        loader = jinja2.FileSystemLoader(PARENT_DIR)
        env = jinja2.Environment(loader=loader)
    env.globals['raise_error'] = raise_error
    env.globals['QueryLogMode'] = QueryLogMode

    context = {
        'sql_service_files': sql_items,
        'yql_service_files': yql_items,
        'namespace': params.namespace,
        'query_log_mode': params.query_log_mode,
    }

    with open(f'{params.src_root}/include/{params.namespace}/sql_queries.hpp', 'w') as hpp_file:
        tpl = env.get_template('templates/sql.hpp.jinja')
        content = tpl.render(**context)
        hpp_file.write(content)

    with open(f'{params.src_root}/src/{params.namespace}/sql_queries.cpp', 'w') as cpp_file:
        tpl = env.get_template('templates/sql.cpp.jinja')
        content = tpl.render(**context)
        cpp_file.write(content)

    if params.testsuite_output_dir:
        with open(f'{params.testsuite_output_dir}/sql_files.py', 'w') as py_file:
            tpl = env.get_template('templates/sql_files.py.jinja')
            content = tpl.render(**context)
            py_file.write(content)


def main():
    args = parse_args()
    items = read_items(args)
    render(
        SqlParams(
            namespace=args.namespace,
            src_root=args.output_dir,
            query_log_mode=args.query_log_mode,
            testsuite_output_dir=args.testsuite_output_dir,
        ),
        items,
        [],
    )


if __name__ == '__main__':
    main()
