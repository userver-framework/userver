"""Tests for the handler renderer, specifically the unconditional per-schema
file emission introduced to fix the missing-file error when a schema has no
C++ types (e.g. empty components section)."""

import pathlib

from chaotic_openapi.back.cpp.client.renderer import Context
from chaotic_openapi.back.cpp.handler import renderer as renderer
from chaotic_openapi.back.cpp.handler import translator as translator
from chaotic_openapi.front import parser as front_parser


def _make_spec(schema: dict) -> translator.ServerSpec:
    p = front_parser.Parser('test')
    p.parse_schema(schema, '<inline>', '<inline>')
    tr = translator.HandlerTranslator(
        p.service(),
        cpp_namespace='handlers::test',
        include_dirs=[],
    )
    return tr.spec()


def _make_context() -> Context:
    return Context(
        generate_path=pathlib.Path(''),
        clang_format_bin='',
        uservices_library_tvm_guard_hack=False,
    )


_NO_TYPES_SCHEMA = {
    'openapi': '3.0.0',
    'info': {'title': 'no types', 'version': '1.0'},
    'paths': {
        '/test1': {'get': {'operationId': 'test1Get', 'responses': {'200': {'description': 'OK'}}}},
    },
    'components': {},
}

_SCHEMA_WITH_TYPES = {
    'openapi': '3.0.0',
    'info': {'title': 'typed', 'version': '1.0'},
    'paths': {
        '/test1': {'get': {'operationId': 'test1Get', 'responses': {'200': {'description': 'OK'}}}},
    },
    'components': {'schemas': {'MyObj': {'type': 'object', 'properties': {'x': {'type': 'string'}}}}},
}


def test_type_less_schema_emits_stub_files():
    """A schema file that produced no C++ types must still yield the pair of
    files that CMake declares unconditionally."""
    spec = _make_spec(_NO_TYPES_SCHEMA)
    ctx = _make_context()

    outputs = renderer.render(spec, ctx, 'userver', schema_files=['typeless.yaml'])
    rel_paths = {o.rel_path for o in outputs}

    assert 'include/handlers/test/typeless.hpp' in rel_paths
    assert 'src/handlers/test/typeless.cpp' in rel_paths


def test_stub_file_content_is_valid():
    """Stub files must contain compilable content."""
    spec = _make_spec(_NO_TYPES_SCHEMA)
    ctx = _make_context()

    outputs = renderer.render(spec, ctx, 'userver', schema_files=['typeless.yaml'])
    by_path = {o.rel_path: o for o in outputs}

    hpp = by_path['include/handlers/test/typeless.hpp']
    cpp = by_path['src/handlers/test/typeless.cpp']

    assert hpp.content == '#pragma once\n'
    assert 'typeless.hpp' in cpp.content


def test_schema_with_types_not_duplicated():
    """A schema that already produced C++ types must not get stub files."""
    spec = _make_spec(_SCHEMA_WITH_TYPES)
    ctx = _make_context()

    outputs = renderer.render(spec, ctx, 'userver', schema_files=['openapi.yaml'])
    rel_paths = [o.rel_path for o in outputs]

    assert rel_paths.count('include/handlers/test/openapi.hpp') == 1
    assert rel_paths.count('src/handlers/test/openapi.cpp') == 1


def test_schema_files_none_preserves_old_behavior():
    """Passing schema_files=None must not emit any stub files."""
    spec = _make_spec(_NO_TYPES_SCHEMA)
    ctx = _make_context()

    outputs = renderer.render(spec, ctx, 'userver', schema_files=None)
    rel_paths = {o.rel_path for o in outputs}

    assert 'include/handlers/test/typeless.hpp' not in rel_paths
    assert 'src/handlers/test/typeless.cpp' not in rel_paths


def test_bearer_handler_preserves_hidden_query_arguments():
    schema = {
        'openapi': '3.0.0',
        'info': {'title': 'bearer', 'version': '1.0'},
        'components': {
            'securitySchemes': {
                'myBearer': {'type': 'http', 'scheme': 'bearer', 'bearerFormat': 'JWT'},
            },
        },
        'paths': {
            '/test': {
                'get': {
                    'operationId': 'testGet',
                    'security': [{'myBearer': []}],
                    'parameters': [
                        {
                            'in': 'query',
                            'name': 'token',
                            'required': True,
                            'x-usrv-query-log-mode': 'hide',
                            'schema': {'type': 'string'},
                        },
                    ],
                    'responses': {'200': {'description': 'OK'}},
                },
            },
        },
    }
    spec = _make_spec(schema)
    outputs = renderer.render(spec, _make_context(), 'userver')

    handler = next(output for output in outputs if output.rel_path.endswith('/handler.hpp'))
    assert 'HiddenQueryArguments, true' in handler.content
