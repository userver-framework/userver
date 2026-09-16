import pathlib

import pytest

from chaotic_openapi.back.cpp.client import renderer as client_renderer
from chaotic_openapi.back.cpp.handler import renderer as handler_renderer
from chaotic_openapi.back.cpp.handler import translator as handler_translator
from chaotic_openapi.front import parser as front_parser

_HANDLER_TEMPLATE_NAMES = (
    handler_renderer.HANDLER_TEMPLATE_NAMES
    + handler_renderer.SPEC_TEMPLATE_NAMES
    + handler_renderer.VIEW_TEMPLATE_NAMES
)

_SCHEMA = {
    'openapi': '3.0.0',
    'info': {'title': '', 'version': '1.0'},
    'paths': {
        '/test': {
            'get': {
                'operationId': 'getTest',
                'responses': {'200': {'description': 'OK'}},
            },
        },
    },
}


@pytest.mark.parametrize('name', client_renderer.TEMPLATE_NAMES)
def test_client_template_is_available(name):
    assert client_renderer.JINJA_ENV.get_template(f'templates/{name}.jinja') is not None


@pytest.mark.parametrize('name', _HANDLER_TEMPLATE_NAMES)
def test_handler_template_is_available(name):
    assert handler_renderer.JINJA_ENV.get_template(f'templates/{name}.jinja') is not None


def test_render_views():
    """View stubs are rendered by `--gen views` and `--gen handlers+views` only,
    so they are not covered by the handler rendering tests."""
    parser = front_parser.Parser('test')
    parser.parse_schema(_SCHEMA, '<inline>', '<inline>')
    spec = handler_translator.HandlerTranslator(
        parser.service(),
        cpp_namespace='handlers::test',
        include_dirs=[],
    ).spec()
    context = client_renderer.Context(
        generate_path=pathlib.Path(''),
        clang_format_bin='',
        uservices_library_tvm_guard_hack=False,
    )

    outputs = handler_renderer.render_views(spec, context, 'USERVER_NAMESPACE')

    by_path = {output.rel_path: output.content for output in outputs}
    assert sorted(by_path) == ['gettest/view.cpp', 'gettest/view.hpp']
    assert 'class View final' in by_path['gettest/view.hpp']
    assert 'Response View::Handle(' in by_path['gettest/view.cpp']
