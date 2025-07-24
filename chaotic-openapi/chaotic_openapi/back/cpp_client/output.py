import collections
import pathlib
from typing import Dict
from typing import List
from typing import Optional

import yaml

from . import renderer

TYPES_INCLUDES = [
    'cstdint',
    'fmt/core.h',
    'optional',
    'string',
]


def _get_template_includes(name: str, client_name: str, graph: Dict[str, List[str]]) -> List[str]:
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
            # TODO
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
            # TODO
        ],
    }
    return includes[name]


def trim_suffix(string: str, suffix: str) -> Optional[str]:
    if string.endswith(suffix):
        return string[: -len(suffix)]
    return None


def extract_includes(name: str, path: pathlib.Path) -> Optional[List[str]]:
    with open(path) as ifile:
        content = yaml.safe_load(ifile)

    includes: List[str] = []

    def visit(data) -> None:
        if isinstance(data, dict):
            for v in data.values():
                visit(v)
            if '$ref' in data:
                ref = data['$ref']
                ref = ref.split('#')[0]
                ref_fname = ref.rsplit('/', 1)[-1]
                if ref_fname:
                    stem = ref_fname.rsplit('.', 1)[0]
                    includes.append(f'clients/{name}/{stem}.hpp')
            if 'x-taxi-cpp-type' in data:
                pass
            if 'x-usrv-cpp-type' in data:
                pass
        elif isinstance(data, list):
            for v in data:
                visit(v)

    if 'definitions' in content or 'components' in content or 'paths' in content:
        visit(content)
        return includes
    else:
        return None


def include_graph(name: str, schemas_dir: pathlib.Path) -> Dict[str, List[str]]:
    result = {}
    for root, _, filenames in schemas_dir.walk():
        for filename in filenames:
            filepath = pathlib.Path(root) / filename
            if filepath == schemas_dir / 'client.yaml' or filename == 'a.yaml':
                continue
            result[filepath.stem + '.hpp'] = extract_includes(name, filepath)

    return {key: result[key] for key in result if result[key] is not None}  # type: ignore


def get_includes(client_name: str, schemas_dir: str) -> Dict[str, List[str]]:
    graph = include_graph(client_name, pathlib.Path(schemas_dir))

    output = collections.defaultdict(list)
    for name in renderer.TEMPLATE_NAMES:
        if name.endswith('.hpp'):
            rel_path = f'include/clients/{client_name}/{name}'
        else:
            rel_path = f'src/clients/{client_name}/{name}'
        output[rel_path] = _get_template_includes(name, client_name, graph)

    for file in graph:
        stem = pathlib.Path(file).stem
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


def external_libraries(schemas_dir: str) -> List[str]:
    types = set()

    def visit(data) -> None:
        if isinstance(data, dict):
            for v in data.values():
                visit(v)
            if 'x-taxi-cpp-type' in data:
                types.add(data['x-taxi-cpp-type'])
            if 'x-usrv-cpp-type' in data:
                types.add(data['x-usrv-cpp-type'])
        elif isinstance(data, list):
            for v in data:
                visit(v)

    for file in pathlib.Path(schemas_dir).rglob('*.yaml'):
        with open(file) as ifile:
            content = yaml.safe_load(ifile)
            visit(content)

    libraries = []
    for type_ in types:
        library = type_.split('::')[0].replace('_', '-')
        # special namespaces (and unsigned) which are defined in userver/, not in libraries/
        if library not in {'std', 'storages', 'decimal64', 'unsigned'}:
            libraries.append(library)
    return libraries
