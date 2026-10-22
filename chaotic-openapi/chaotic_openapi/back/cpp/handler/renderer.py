import os
from pathlib import Path
import re

import jinja2
import yaml

from chaotic import cpp_format
from chaotic import jinja_env
from chaotic import layout
from chaotic.back.cpp import renderer as cpp_renderer
from chaotic.back.cpp import tagged
from chaotic_openapi.back.cpp.client.renderer import Context
from chaotic_openapi.back.cpp.client.renderer import CppOutput
from chaotic_openapi.back.cpp.client.renderer import empty_schema_outputs
from chaotic_openapi.back.cpp.client.renderer import schema_paths
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


def component_name(op: Operation, spec: ServerSpec | None = None) -> str:
    if op.operation_id:
        name = _to_kebab(op.operation_id)
    else:
        name = _path_slug(op.path) + '-' + op.method.lower()
    if spec and spec.closure:
        # Unique across the service: the namespace is the document's own
        return '-'.join([*(_to_kebab(part) for part in spec.cpp_namespace.split('::')), name])
    return 'handler-' + name


def op_relpath(operation_id: str | None, path: str, method: str) -> str:
    """Directory of an operation's files relative to the spec's; path is transformed by transform_path()."""
    if operation_id:
        return operation_id.lower()
    path = path.strip('/').replace('/', '_')
    return f'{path}/{method.lower()}'


def make_op_relpath(op: Operation) -> str:
    return op_relpath(op.operation_id, op.path, op.method)


def _view_dir(include_dir: str, tagged: bool, relpath: str) -> str:
    if tagged:
        return f'{include_dir}/{relpath}'
    # The views source root of an untagged spec is the spec's own directory
    return relpath


def view_dir(spec: ServerSpec, op: Operation) -> str:
    """Directory of the operation's view, relative to the views source root."""
    return _view_dir(spec.include_dir, bool(spec.closure), make_op_relpath(op))


def _template_path(directory: str, name: str) -> str:
    if name.endswith('.hpp'):
        return f'include/{directory}/{name}'
    return f'src/{directory}/{name}'


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


def output_files(
    include_dir: str,
    tagged: bool,
    op_relpaths: list[str],
    schema_paths: list[str],
    *,
    handlers: bool,
    views: bool,
) -> tuple[list[str], list[str]]:
    """
    Files render(), render_config_yaml() and render_views() generate, relative to the output root and to the
    views source root, derived without rendering.
    """
    files = []
    if handlers:
        for relpath in op_relpaths:
            files.extend(_template_path(f'{include_dir}/{relpath}', name) for name in HANDLER_TEMPLATE_NAMES)
        files.extend(_template_path(include_dir, name) for name in SPEC_TEMPLATE_NAMES)
        for path in schema_paths:
            files.extend(layout.schema_output_files(path))
        files.append(_config_yaml_path(include_dir, tagged))
    view_files = []
    if views:
        for relpath in op_relpaths:
            view_files.extend(f'{_view_dir(include_dir, tagged, relpath)}/{name}' for name in VIEW_TEMPLATE_NAMES)
    return files, view_files


def render(
    spec: ServerSpec,
    context: Context,
    userver_namespace: str,
    schema_files: list[str] | None = None,
) -> list[CppOutput]:
    output = []

    for op in spec.operations:
        env = {'spec': spec, 'op': op, 'userver': userver_namespace}

        op_dir = f'{spec.include_dir}/{make_op_relpath(op)}'
        for name in HANDLER_TEMPLATE_NAMES:
            output.append(_render_template(name, context.clang_format_bin, _template_path(op_dir, name), **env))

    spec_env = {'spec': spec, 'userver': userver_namespace}
    for name in SPEC_TEMPLATE_NAMES:
        output.append(
            _render_template(name, context.clang_format_bin, _template_path(spec.include_dir, name), **spec_env)
        )

    if spec.closure:
        for cpp_output in tagged.render(spec.extract_cpp_types(), spec.closure, context.clang_format_bin):
            for file in cpp_output.files:
                output.append(
                    CppOutput(
                        rel_path=str(Path(file.subdir) / (cpp_output.filepath_wo_ext + file.ext)),
                        content=file.content,
                    ),
                )
        return output

    # Schema type files are service-level (shared across all operations)
    vfilepath_map = {}
    for cpp_type in spec.extract_cpp_types().values():
        assert cpp_type.json_schema
        filepath = cpp_type.json_schema.source_location().filepath
        vfilepath_map[filepath] = 'handlers/{}/{}'.format(spec.service_name, os.path.basename(filepath))

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

    if schema_files is not None:
        output.extend(
            empty_schema_outputs(
                schema_paths(spec.include_dir, schema_files),
                {cpp_output.filepath_wo_ext for cpp_output in cpp_outputs},
            )
        )

    return output


def render_config_yaml(spec: ServerSpec) -> str:
    components = {}
    for op in spec.operations:
        entry: dict = {'path': op.path, 'method': op.method.upper()}
        if op.security:
            entry['auth'] = {'types': [op.security.auth_type]}
        components[component_name(op, spec)] = entry
    config = {
        'components_manager': {
            'components': components,
        },
    }
    return yaml.dump(config, default_flow_style=False, sort_keys=False)


def _config_yaml_path(include_dir: str, tagged: bool) -> str:
    if tagged:
        # Several tagged specs may share one output root
        return f'{include_dir}/config.chaotic.yaml'
    return 'config.chaotic.yaml'


def config_yaml_path(spec: ServerSpec) -> str:
    """Path of config.chaotic.yaml relative to the output root."""
    return _config_yaml_path(spec.include_dir, bool(spec.closure))


def save_config_yaml(content: str, output_dir: str, rel_path: str = 'config.chaotic.yaml') -> None:
    path = os.path.join(output_dir, rel_path)
    os.makedirs(os.path.dirname(path), exist_ok=True)

    with open(path, 'w') as ofile:
        ofile.write(content)


def render_views(spec: ServerSpec, context: Context, userver_namespace: str) -> list[CppOutput]:
    output = []
    for op in spec.operations:
        env = {'spec': spec, 'op': op, 'userver': userver_namespace}
        for name in VIEW_TEMPLATE_NAMES:
            rel_path = f'{view_dir(spec, op)}/{name}'
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
