"""Tests for the client renderer, specifically the unconditional per-schema
file emission introduced to fix the missing-file error when a schema has no
C++ types (e.g. empty components section)."""

import pathlib

from chaotic_openapi.back.cpp.client import renderer
from chaotic_openapi.back.cpp.client import translator
from chaotic_openapi.front import parser as front_parser


def _make_spec(schema: dict, name: str = 'test_client'):
    p = front_parser.Parser(name)
    p.parse_schema(schema, '<inline>', '<inline>')
    tr = translator.Translator(
        p.service(),
        dynamic_config='',
        cpp_namespace=f'clients::{name}',
        include_dirs=[],
        middleware_plugins=[],
    )
    return tr.spec()


def _make_context() -> renderer.Context:
    return renderer.Context(
        generate_path=pathlib.Path(''),
        clang_format_bin='',
        uservices_library_tvm_guard_hack=False,
    )


EMPTY_SCHEMA = {
    'openapi': '3.0.0',
    'info': {'title': 'empty', 'version': '1.0'},
    'paths': {
        '/test1': {
            'post': {
                'responses': {'200': {'description': 'OK'}},
            },
        },
    },
    'components': {},
}


def test_empty_schema_emits_all_five_files_per_schema():
    """When schema_files is provided and the schema has no C++ types, the
    renderer must unconditionally emit the full 5-file set so that both
    CMake and ya.make can declare them without special-casing."""
    spec = _make_spec(EMPTY_SCHEMA)
    ctx = _make_context()

    outputs = renderer.render(spec, ctx, schema_files=['openapi.yaml'])
    rel_paths = {o.rel_path for o in outputs}

    client = spec.client_name
    assert f'include/clients/{client}/openapi_fwd.hpp' in rel_paths
    assert f'include/clients/{client}/openapi.hpp' in rel_paths
    assert f'include/clients/{client}/openapi_parsers.ipp' in rel_paths
    assert f'include/clients/{client}/openapi_sax_parsers.hpp' in rel_paths
    assert f'src/clients/{client}/openapi.cpp' in rel_paths


def test_empty_schema_stub_content_is_valid():
    """Stub files for type-less schemas must contain valid (compilable)
    content: .hpp files have #pragma once, .cpp includes its header."""
    spec = _make_spec(EMPTY_SCHEMA)
    ctx = _make_context()

    outputs = renderer.render(spec, ctx, schema_files=['openapi.yaml'])
    by_path = {o.rel_path: o for o in outputs}
    client = spec.client_name

    fwd = by_path[f'include/clients/{client}/openapi_fwd.hpp']
    hpp = by_path[f'include/clients/{client}/openapi.hpp']
    sax = by_path[f'include/clients/{client}/openapi_sax_parsers.hpp']
    cpp = by_path[f'src/clients/{client}/openapi.cpp']

    assert '#pragma once' in fwd.content
    assert '#pragma once' in hpp.content
    assert '#pragma once' in sax.content
    assert 'openapi.hpp' in cpp.content


def test_schema_with_types_not_duplicated():
    """When a schema does have C++ types, the renderer must NOT emit
    extra stub files on top of the real ones (no duplication)."""
    schema_with_types = {
        'openapi': '3.0.0',
        'info': {'title': 'typed', 'version': '1.0'},
        'paths': {},
        'components': {
            'schemas': {
                'MyObj': {'type': 'object', 'properties': {'x': {'type': 'string'}}},
            },
        },
    }
    spec = _make_spec(schema_with_types)
    ctx = _make_context()

    outputs = renderer.render(spec, ctx, schema_files=['openapi.yaml'])
    rel_paths = [o.rel_path for o in outputs]
    client = spec.client_name

    # Each per-schema file must appear exactly once.
    for suffix in ['openapi_fwd.hpp', 'openapi.hpp', 'openapi_parsers.ipp', 'openapi_sax_parsers.hpp']:
        count = rel_paths.count(f'include/clients/{client}/{suffix}')
        assert count == 1, f'{suffix} appeared {count} times (expected 1)'
    count = rel_paths.count(f'src/clients/{client}/openapi.cpp')
    assert count == 1, f'openapi.cpp appeared {count} times (expected 1)'


def test_schema_files_none_preserves_old_behavior():
    """Passing schema_files=None must not emit any stub files (backward
    compat for callers that do not supply the schema file list)."""
    spec = _make_spec(EMPTY_SCHEMA)
    ctx = _make_context()

    outputs = renderer.render(spec, ctx, schema_files=None)
    rel_paths = {o.rel_path for o in outputs}
    client = spec.client_name

    assert f'include/clients/{client}/openapi_fwd.hpp' not in rel_paths
    assert f'src/clients/{client}/openapi.cpp' not in rel_paths
