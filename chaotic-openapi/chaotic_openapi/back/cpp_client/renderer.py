import dataclasses
import os
import pathlib

import jinja2

from chaotic import cpp_format
from chaotic import jinja_env
from chaotic_openapi.back.cpp_client import types

PARENT_DIR = os.path.dirname(__file__)

TEMPLATE_NAMES = [
    'client.hpp',
    # no 'client.cpp',
    'client_impl.hpp',
    'client_impl.cpp',
    # 'requests.hpp',
    # 'requests.cpp',
    # 'responses.hpp',
    # 'responses.cpp',
    # 'exceptions.hpp',
]


@dataclasses.dataclass
class Context:
    generate_path: pathlib.Path
    clang_format_bin: str


@dataclasses.dataclass
class CppOutput:
    rel_path: str
    content: str

    def save(self, prefix: str) -> None:
        with open(prefix + self.rel_path, 'w') as ofile:
            ofile.write(self.content)


def make_env() -> jinja2.Environment:
    env = jinja_env.make_env(
        'chaotic-openapi/chaotic_openapi/back/cpp_client',
        os.path.join(PARENT_DIR),
    )

    return env


JINJA_ENV = make_env()


def render(spec: types.ClientSpec, context: Context) -> None:
    env = {'namespace': spec.cpp_namespace, 'operations': spec.operations}

    output = []
    for name in TEMPLATE_NAMES:
        tpl = JINJA_ENV.get_template(f'templates/{name}.jinja')
        pp = tpl.render(**env)
        pp = cpp_format.format_pp(pp, binary=context.clang_format_bin)

        if name.endswith('.hpp'):
            rel_path = f'include/client/{spec.client_name}/{name}'
        else:
            rel_path = f'src/client/{spec.client_name}/{name}'

        output.append(CppOutput(rel_path=rel_path, content=pp))
    return output
