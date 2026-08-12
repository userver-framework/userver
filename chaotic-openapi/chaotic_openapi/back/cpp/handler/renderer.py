import os
from pathlib import Path
import re

import jinja2
import yaml

from chaotic import cpp_format
from chaotic import jinja_env
from chaotic.back.cpp import renderer as cpp_renderer
from chaotic_openapi.back.cpp.client.renderer import Context
from chaotic_openapi.back.cpp.client.renderer import CppOutput
from chaotic_openapi.back.cpp.common.types import Operation
from chaotic_openapi.back.cpp.handler.types import ServerSpec

PARENT_DIR = Path(__file__).parent

_UPPER_RE = re.compile(r'([A-Z])')
_BRACES_RE = re.compile(r'[{}]')
_MULTI_DASH_RE = re.compile(r'-+')

HANDLER_TEMPLATE_NAMES = [
    'requests.hpp',
    'requests.cpp',
    'responses.hpp',
    'responses.cpp',
    'handler.hpp',
    'handler.cpp',
]

SPEC_TEMPLATE_NAMES = [
    'chaotic_handlers_list.hpp',
]

VIEW_TEMPLATE_NAMES = [
    'view.hpp',
    'view.cpp',
]


def _to_kebab(s: str) -> str:
    s = _UPPER_RE.sub(r'-\1', s)
    return s.replace('_', '-').lower().lstrip('-')


def _path_slug(path: str) -> str:
    slug = _BRACES_RE.sub('', path).strip('/').replace('/', '-')
    slug = _MULTI_DASH_RE.sub('-', slug)
    return slug.lower()


def component_name(op: Operation) -> str:
    if op.operation_id:
        return 'handler-' + _to_kebab(op.operation_id)
    slug = _path_slug(op.path) + '-' + op.method.lower()
    return 'handler-' + slug


def make_op_relpath(op: Operation) -> str:
    if op.operation_id:
        return op.operation_id.lower()
    path = op.path.strip('/').replace('/', '_')
    return f'{path}/{op.method.lower()}'


def make_env() -> jinja2.Environment:
    env = jinja_env.make_env(
        'chaotic-openapi/chaotic_openapi/back/cpp/handler',
        str(PARENT_DIR),
    )
    env.globals['list'] = list
    env.globals['enumerate'] = enumerate
    env.globals['str'] = str
    env.globals['component_name'] = component_name
    env.globals['make_op_relpath'] = make_op_relpath
    return env


JINJA_ENV = make_env()


def _render_template(name: str, clang_format_bin: str, rel_path: str, **env: object) -> CppOutput:
    tpl = JINJA_ENV.get_template(f'templates/{name}.jinja')
    assert tpl is not None, f'Template not found: templates/{name}.jinja'
    pp = tpl.render(**env)
    pp = cpp_format.format_pp(pp, binary=clang_format_bin)
    return CppOutput(rel_path=rel_path, content=pp)


def render(spec: ServerSpec, context: Context, userver_namespace: str) -> list[CppOutput]:
    output = []

    for op in spec.operations:
        env = {'spec': spec, 'op': op, 'userver': userver_namespace}

        op_path = f'handlers/{spec.service_name}/{make_op_relpath(op)}'
        for name in HANDLER_TEMPLATE_NAMES:
            if name.endswith('.hpp'):
                rel_path = f'include/{op_path}/{name}'
            else:
                rel_path = f'src/{op_path}/{name}'
            output.append(_render_template(name, context.clang_format_bin, rel_path, **env))

    spec_env = {'spec': spec, 'userver': userver_namespace}
    for name in SPEC_TEMPLATE_NAMES:
        rel_path = f'include/handlers/{spec.service_name}/{name}'
        output.append(_render_template(name, context.clang_format_bin, rel_path, **spec_env))

    # Schema type files are service-level (shared across all operations)
    vfilepath_map = {}
    for cpp_type in spec.extract_cpp_types().values():
        assert cpp_type.json_schema
        filepath = cpp_type.json_schema.source_location().filepath
        vfilepath_map[filepath] = 'handlers/{}/{}'.format(spec.service_name, filepath)

    r = cpp_renderer.OneToOneFileRenderer(
        relative_to='',
        vfilepath_to_relfilepath=vfilepath_map,
        clang_format_bin=context.clang_format_bin,
        generate_serializer=True,
        generate_sax_parser=True,
    )
    cpp_outputs = r.render(
        spec.extract_cpp_types(),
        local_pair_header=False,
    )
    for cpp_output in cpp_outputs:
        for file in cpp_output.files:
            output.append(
                CppOutput(
                    rel_path=str(Path(file.subdir) / (cpp_output.filepath_wo_ext + file.ext)),
                    content=file.content,
                ),
            )

    return output


def render_config_yaml(spec: ServerSpec) -> str:
    components = {
        component_name(op): {
            'path': op.path,
            'method': op.method.upper(),
        }
        for op in spec.operations
    }
    config = {
        'components_manager': {
            'components': components,
        },
    }
    return yaml.dump(config, default_flow_style=False, sort_keys=False)


def save_config_yaml(content: str, output_dir: str) -> None:
    path = os.path.join(output_dir, 'config.chaotic.yaml')
    os.makedirs(os.path.dirname(path), exist_ok=True)

    with open(path, 'w') as ofile:
        ofile.write(content)


def render_views(spec: ServerSpec, context: Context, userver_namespace: str) -> list[CppOutput]:
    output = []
    for op in spec.operations:
        env = {'spec': spec, 'op': op, 'userver': userver_namespace}
        for name in VIEW_TEMPLATE_NAMES:
            rel_path = f'{make_op_relpath(op)}/{name}'
            output.append(_render_template(name, context.clang_format_bin, rel_path, **env))
    return output


def save_views(outputs: list[CppOutput], prefix: str) -> None:
    for output in outputs:
        path = os.path.join(prefix, output.rel_path)
        if os.path.exists(path):
            continue
        os.makedirs(os.path.dirname(path), exist_ok=True)
        with open(path, 'w') as ofile:
            ofile.write(output.content.strip() + '\n')
