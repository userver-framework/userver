"""Tests for the generated handler view stubs (view.hpp / view.cpp)."""

import pathlib

from chaotic_openapi.back.cpp.client import renderer as client_renderer
from chaotic_openapi.back.cpp.handler import renderer as handler_renderer
from chaotic_openapi.back.cpp.handler import translator as handler_translator
from chaotic_openapi.back.cpp.handler.types import ServerSpec
from chaotic_openapi.front import parser as front_parser


def _translate(schema, *, cpp_namespace='handlers::test') -> ServerSpec:
    parser = front_parser.Parser('test')
    parser.parse_schema(schema, '<inline>', '<inline>')
    tr = handler_translator.HandlerTranslator(
        parser.service(),
        cpp_namespace=cpp_namespace,
        include_dirs=[],
    )
    return tr.spec()


def _render_views(spec: ServerSpec):
    ctx = client_renderer.Context(
        generate_path=pathlib.Path(''),
        clang_format_bin='',
        uservices_library_tvm_guard_hack=False,
    )
    return handler_renderer.render_views(spec, ctx, userver_namespace='USERVER_NAMESPACE')


_MINIMAL_SCHEMA = {
    'openapi': '3.0.0',
    'info': {'title': '', 'version': '1.0'},
    'paths': {
        '/testme': {
            'post': {
                'operationId': 'testmePost',
                'parameters': [],
                'requestBody': {
                    'content': {
                        'application/json': {'schema': {'type': 'integer'}},
                    },
                },
                'responses': {200: {'description': 'OK'}},
            },
        },
    },
}


def _view_outputs():
    spec = _translate(_MINIMAL_SCHEMA)
    outputs = _render_views(spec)
    return {o.rel_path: o for o in outputs}


def test_view_stubs_generated_per_operation():
    by_path = _view_outputs()
    assert set(by_path) == {'testmepost/view.hpp', 'testmepost/view.cpp'}


def test_view_hpp_contract():
    hpp = _view_outputs()['testmepost/view.hpp'].content

    assert '#include <userver/server/request/request_context.hpp>' in hpp
    assert 'using RequestContext = USERVER_NAMESPACE::server::request::RequestContext;' in hpp
    assert 'static Response Handle(Request&& request, Deps&& deps, RequestContext& context);' in hpp
    assert 'GetResponseForLogging(' in hpp
    assert 'RequestContext& context);' in hpp

    # The legacy 2-argument Handle must not be generated anymore.
    assert 'Handle(Request&& request, Deps&& deps);' not in hpp


def test_view_cpp_stub_contract():
    cpp = _view_outputs()['testmepost/view.cpp'].content

    assert 'Response View::Handle(' in cpp
    assert 'RequestContext& /*context*/) {' in cpp
    assert 'GetResponseForLogging(' in cpp
    assert 'RequestContext& context) {' in cpp