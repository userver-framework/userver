import os
from typing import Any
from typing import NoReturn

import jinja2

PARENT_DIR = os.path.dirname(__file__)


def _make_arcadia_loader(prefix: str) -> jinja2.FunctionLoader:
    import library.python.resource as arc_resource

    def arc_resource_loader(name: str) -> jinja2.BaseLoader:
        return arc_resource.resfs_read(
            f'taxi/uservices/userver/{prefix}/{name}',
        ).decode('utf-8')

    loader = jinja2.FunctionLoader(arc_resource_loader)  # type: ignore

    # try to load something and drop the result
    try:
        arc_resource.resfs_read(
            'taxi/uservices/userver/chaotic/chaotic/back/cpp/templates/type.hpp.jinja',
        )
    except Exception:
        raise ImportError('resfs is not available')

    return loader


def not_implemented(obj: Any = None) -> NoReturn:
    raise Exception('Not implemented: ' + repr(obj))


def indent_not_empty(value: str, width: int = 4, first: bool = True) -> str:
    """Like JINJA indent(first=True), but does not add whitespace to empty lines"""
    if not value:
        return ''

    return jinja2.filters.do_indent(value, width=width, first=first)


def make_env(prefix: str, full_path: str) -> jinja2.Environment:
    loader: jinja2.BaseLoader
    try:
        loader = _make_arcadia_loader(prefix)
    except ImportError:
        loader = jinja2.FileSystemLoader(os.path.join(full_path))
    env = jinja2.Environment(loader=loader, trim_blocks=True, lstrip_blocks=True)

    # common symbols used by everyone
    env.globals['NOT_IMPLEMENTED'] = not_implemented
    env.globals['len'] = len
    env.filters['indent_not_empty'] = indent_not_empty

    return env
