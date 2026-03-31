#!/usr/bin/python3

import argparse
import dataclasses
import pathlib
import re
import sys
from typing import Any

import jinja2

sys.path += [str(pathlib.Path(__file__).parent)]

import parse
import prototype_parser
import tag_lexer


def unquote(s: str) -> str:
    return s.removesuffix('"').removeprefix('"')


def escape(s: str) -> str:
    if '$' in s:
        return f'"{s}"'
    else:
        return s


def oneline(s: str) -> str:
    return s.replace('\\\n', ' ')


def render(collection) -> str:
    text = ''
    for item in collection:
        text += item['pretty'] + '\n'
    return text


def only_command(collection, name: str) -> list[dict]:
    res = []
    for item in collection:
        if item['name'] == name:
            res.append(item)
    return res


def only_group(collection, name: str | None) -> list[dict]:
    res = []
    for item in collection:
        if item['group'] == name:
            res.append(item)
    return res


loader = jinja2.FileSystemLoader(pathlib.Path(__file__).resolve().parent)
jinja_env = jinja2.Environment(
    loader=loader,
    autoescape=False,
)
jinja_env.filters['unquote'] = unquote
jinja_env.filters['escape'] = escape
jinja_env.filters['oneline'] = oneline
jinja_env.filters['only_group'] = only_group
jinja_env.filters['only_command'] = only_command
jinja_env.filters['render'] = render

FUNCTION_TEMPLATE = jinja_env.get_template('function.md.jinja')


def parse_template(s: str) -> tuple[jinja2.Template, str]:
    parts = s.split(':')
    assert len(parts) == 2
    return jinja_env.get_template(parts[0]), parts[1]


def parse_args():
    parser = argparse.ArgumentParser()
    parser.add_argument('-t', type=parse_template, action='append')
    parser.add_argument('path', type=str, nargs='+')
    return parser.parse_args()


def enrich_symbol(symbol: parse.Symbol) -> dict[str, Any]:
    res = dataclasses.asdict(symbol)
    res['prototype'] = prototype_parser.parse(tag_lexer.tokenize(symbol.comments))

    res['pretty'] = FUNCTION_TEMPLATE.render({'symbol': res}).strip()
    return res


@dataclasses.dataclass
class Comment:
    text: str
    ingroup: str | None


def enrich_command(cmd: dict[str, Any]) -> dict[str, Any]:
    text = ''
    ingroup = None

    it = iter(tag_lexer.tokenize(cmd['comments']))
    for token in it:
        if isinstance(token, str):
            text += token
            continue

        tag = token
        assert isinstance(tag, tag_lexer.Tag)
        match tag.name:
            case 'ingroup':
                ingroup = next(it).strip()
                if not ingroup:
                    # skip spaces, read next non-space token
                    ingroup = next(it).strip()
            case _:
                raise BaseException(f'Unknown tag: @{tag.name}')

    cmd['group'] = ingroup
    cmd['pretty'] = text.strip()
    return cmd


def main():
    args = parse_args()
    symbols = []
    commands = []
    for path in args.path:
        f = parse.parse_file(path)
        symbols += parse.extract_functions(f)
        commands += parse.extract_commands(f)

    for template, path in args.t:
        content = template.render({
            'symbols': list(map(enrich_symbol, symbols)),
            'commands': list(map(enrich_command, commands)),
        })
        content = re.sub('\n\n\n+', '\n\n\n', content).strip() + '\n'
        with open(path, 'w') as ofile:
            ofile.write(content)


if __name__ == '__main__':
    main()
