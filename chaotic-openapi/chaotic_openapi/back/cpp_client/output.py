import collections
import dataclasses
import os
import pathlib
from typing import Any
from typing import Optional

import yaml

from chaotic.back.cpp import types as cpp_types
from chaotic.front import parser as chaotic_parser
from chaotic.front import ref
from . import renderer

TYPES_INCLUDES = [
    'cstdint',
    'fmt/core.h',
    'optional',
    'string',
]


def filepath_with_stem(fpath: str) -> str:
    return fpath.rsplit('.', 1)[0]


def _get_template_includes(name: str, client_name: str, graph: dict[str, list[str]]) -> list[str]:
    includes = {
        'client.cpp': [
            f'clients/{client_name}/client.hpp',
        ],
        'client.hpp': [
            'requests.hpp',
            'responses.hpp',
            'userver/chaotic/openapi/client/command_control.hpp',
        ],
        'client_fwd.hpp': [],
        'client_impl.cpp': [
            f'clients/{client_name}/client_impl.hpp',
            'userver/chaotic/openapi/middlewares/follow_redirects_middleware.hpp',
            'userver/chaotic/openapi/middlewares/qos_middleware.hpp',
            'userver/components/component_context.hpp',
            'userver/components/component_base.hpp',
            'userver/yaml_config/merge_schemas.hpp',
        ],
        'client_impl.hpp': [
            f'clients/{client_name}/client.hpp',
            'userver/chaotic/openapi/client/config.hpp',
            'userver/chaotic/openapi/middlewares/manager.hpp',
            'userver/clients/http/client.hpp',
            'userver/components/component_config.hpp',
            'userver/yaml_config/schema.hpp',
        ],
        'qos.hpp': [
            'userver/chaotic/openapi/client/command_control.hpp',
        ],
        'exceptions.cpp': [
            f'clients/{client_name}/exceptions.hpp',
            'userver/clients/http/error_kind.hpp',
        ],
        'exceptions.hpp': [
            'userver/chaotic/openapi/client/exceptions.hpp',
        ],
        'component.cpp': [
            f'clients/{client_name}/component.hpp',
            'userver/chaotic/openapi/client/config.hpp',
            'userver/components/component_context.hpp',
            'userver/clients/http/component.hpp',
        ],
        'component.hpp': [
            'userver/components/component_base.hpp',
            'userver/yaml_config/schema.hpp',
            f'clients/{client_name}/client_impl.hpp',
        ],
        'requests.cpp': [
            f'clients/{client_name}/requests.hpp',
            'userver/chaotic/openapi/parameters_write.hpp',
            'userver/formats/json/value_builder.hpp',
            'userver/http/common_headers.hpp',
            'userver/http/url.hpp',
            'userver/clients/http/form.hpp',
            'userver/chaotic/openapi/form.hpp',
        ],
        'requests.hpp': [
            'string',
            'variant',
            *[f'clients/{client_name}/{dep}' for dep in graph],
        ],
        'responses.cpp': [
            f'clients/{client_name}/responses.hpp',
            'userver/clients/http/response.hpp',
            'userver/formats/json/serialize.hpp',
            'userver/http/common_headers.hpp',
            'userver/http/content_type.hpp',
            'userver/logging/log.hpp',
        ],
        'responses.hpp': [
            'variant',
            f'clients/{client_name}/exceptions.hpp',
            'userver/chaotic/openapi/client/exceptions.hpp',
            *[f'clients/{client_name}/{dep}' for dep in graph],
        ],
    }
    return includes[name]


def extract_includes(name: str, path: pathlib.Path, schemas_dir: pathlib.Path) -> Optional[list[str]]:
    with open(path) as ifile:
        content = yaml.safe_load(ifile)

    includes: list[str] = []
    is_file_produced = False

    relpath = os.path.relpath(os.path.dirname(path), schemas_dir)

    def visit(data) -> None:
        nonlocal is_file_produced
        if isinstance(data, dict):
            for v in data.values():
                visit(v)
            if '$ref' in data:
                ref_ = data['$ref']
                filename = ref.Ref(ref_).file
                if filename:
                    stem = os.path.join(relpath, filepath_with_stem(filename))
                    stem = chaotic_parser.SchemaParser._normalize_ref(stem)
                    includes.append(f'clients/{name}/{stem}.hpp')

            for field in ('x-taxi-cpp-type', 'x-usrv-cpp-type'):
                if field not in data:
                    continue
                header = extract_cpp_type_header(data[field])
                if header:
                    includes.append(header)

            if is_file_produced_feature(data):
                is_file_produced = True
        elif isinstance(data, list):
            for v in data:
                visit(v)

    visit(content)
    if content.get('definitions') or content.get('components', {}).get('schemas'):
        is_file_produced = True

    if includes or is_file_produced:
        return includes

    # No schemas in file, it will generate no .hpp/.cpp
    return None


def is_file_produced_feature(data: dict[str, Any]) -> bool:
    if data.get('type') == 'object':
        return True
    if data.get('oneOf'):
        return True
    if data.get('allOf'):
        return True
    return False


def extract_cpp_type_header(cpp_type: str) -> Optional[str]:
    parts = cpp_type.split('::')
    if len(parts) < 2:
        return None
    library = parts[0].replace('_', '-')
    if library == 'std':
        return None
    filename = cpp_types.camel_to_snake_case(parts[1])
    return '{}/{}.hpp'.format(library, filename)


def include_graph(name: str, schemas_dir: pathlib.Path) -> dict[str, list[str]]:
    result = {}
    for root, _, filenames in schemas_dir.walk():
        for filename in filenames:
            if not filename.endswith('.yaml'):
                continue
            filepath = pathlib.Path(root) / filename
            if filepath == schemas_dir / 'client.yaml' or filename == 'a.yaml':
                continue

            stem = filepath_with_stem(os.path.relpath(filepath, schemas_dir))
            result[stem + '.hpp'] = extract_includes(name, filepath, schemas_dir)

    return {key: result[key] for key in result if result[key] is not None}  # type: ignore


def get_includes(client_name: str, schemas_dir: str) -> dict[str, list[str]]:
    graph = include_graph(client_name, pathlib.Path(schemas_dir))

    output = collections.defaultdict(list)
    for name in renderer.TEMPLATE_NAMES:
        if name.endswith('.hpp'):
            rel_path = f'include/clients/{client_name}/{name}'
        else:
            rel_path = f'src/clients/{client_name}/{name}'
        output[rel_path] = _get_template_includes(name, client_name, graph)

    for file in graph:
        stem = filepath_with_stem(file)
        output[f'include/clients/{client_name}/{stem}_fwd.hpp'] = []
        output[f'include/clients/{client_name}/{stem}.hpp'] = [
            f'clients/{client_name}/{stem}_fwd.hpp',
            'userver/chaotic/type_bundle_hpp.hpp',
            *TYPES_INCLUDES,
            *graph[file],
        ]
        output[f'include/clients/{client_name}/{stem}_parsers.ipp'] = [
            f'clients/{client_name}/{stem}.hpp',
        ]
        output[f'src/clients/{client_name}/{stem}.cpp'] = [
            f'clients/{client_name}/{stem}.hpp',
            f'clients/{client_name}/{stem}_parsers.ipp',
            'userver/chaotic/type_bundle_cpp.hpp',
            *TYPES_INCLUDES,
            *graph[file],
        ]

    return output


@dataclasses.dataclass
class External:
    libraries: list[str]
    userver_modules: list[str]


# For Yandex uservices only, not for OSS
def external_libraries(schemas_dir: str) -> External:
    types = set()

    libraries = []
    userver_modules = []

    def visit(data) -> None:
        if isinstance(data, dict):
            for v in data.values():
                visit(v)
            if 'x-taxi-cpp-type' in data:
                types.add(data['x-taxi-cpp-type'])
            if 'x-usrv-cpp-type' in data:
                types.add(data['x-usrv-cpp-type'])
            if 'x-taxi-cpp-typedef-tag' in data:
                types.add(data['x-taxi-cpp-typedef-tag'])
            if 'x-usrv-cpp-typedef-tag' in data:
                types.add(data['x-usrv-cpp-typedef-tag'])

            if data.get('x-taxi-middlewares', {}).get('api-4.0'):
                libraries.append('passenger-authorizer-backend')
            if data.get('x-taxi-middlewares', {}).get('eats') == 'v1':
                libraries.append('eats-authproxy-backend')
            if data.get('x-taxi-middlewares', {}).get('bank-authproxy'):
                libraries.append('bank-authproxy-backend')

        elif isinstance(data, list):
            for v in data:
                visit(v)

    for file in pathlib.Path(schemas_dir).rglob('*.yaml'):
        with open(file) as ifile:
            content = yaml.safe_load(ifile)
            visit(content)

    for type_ in types:
        if type_.startswith('::'):
            type_ = type_[2:]
        library = type_.split('::')[0].replace('_', '-')
        # special namespaces (and unsigned) which are defined in userver/, not in libraries/
        if library not in {'std', 'storages', 'decimal64', 'unsigned'}:
            libraries.append(library)
        if type_.startswith('storages::postgres::'):
            userver_modules.append('postgresql')

    return External(libraries=libraries, userver_modules=userver_modules)
