import pytest

from chaotic import error as chaotic_error
from chaotic_openapi.back.cpp.client import translator as client_translator
from chaotic_openapi.back.cpp.common import types as common_types
from chaotic_openapi.back.cpp.handler import translator as handler_translator
from chaotic_openapi.front import parser as front_parser

_BODY = {
    'required': True,
    'content': {'application/json': {'schema': {'type': 'object', 'properties': {}, 'additionalProperties': False}}},
}

_PATH_PARAMETER = {'in': 'path', 'name': 'id', 'required': True, 'schema': {'type': 'string'}}


def _openapi(method: str, operation: dict) -> dict:
    return {
        'openapi': '3.0.0',
        'info': {'title': '', 'version': '1.0'},
        'paths': {'/test/{id}': {method: {'parameters': [_PATH_PARAMETER], 'responses': {}, **operation}}},
    }


def _service(openapi_doc: dict):
    parser = front_parser.Parser('test')
    parser.parse_schema(openapi_doc, '<inline>', '<inline>')
    return parser.service()


def _client_operations(method: str, operation: dict) -> list[common_types.Operation]:
    return client_translator.Translator(
        _service(_openapi(method, operation)),
        dynamic_config='',
        cpp_namespace='test_namespace',
        include_dirs=[],
        middleware_plugins=[],
    ).spec().operations


def _client_operation(method: str, operation: dict) -> common_types.Operation:
    operations = _client_operations(method, operation)
    assert len(operations) == 1
    return operations[0]


@pytest.mark.parametrize(
    'method,operation,cpp_http_method',
    [
        ('get', {}, 'kGet'),
        ('head', {}, 'kHead'),
        ('post', {}, 'kPost'),
        ('post', {'requestBody': _BODY}, 'kPost'),
        ('put', {}, 'kPut'),
        ('put', {'requestBody': _BODY}, 'kPut'),
        ('patch', {'requestBody': _BODY}, 'kPatch'),
        ('delete', {}, 'kDelete'),
        ('delete', {'requestBody': _BODY}, 'kDelete'),
        ('options', {}, 'kOptions'),
        ('get', {'requestBody': _BODY}, None),
    ],
)
def test_cpp_http_method(method, operation, cpp_http_method):
    assert _client_operation(method, operation).cpp_http_method() == cpp_http_method


def test_trace_is_unsupported_by_client():
    with pytest.raises(chaotic_error.BaseError) as exc:
        _client_operation('trace', {})
    assert exc.value.msg == (
        'Operation "TRACE /test/{id}": HTTP method "TRACE" is not supported by the generated client yet. '
        'Set "x-usrv-client-codegen: false" on the operation to skip it'
    )


def test_trace_without_client_codegen_is_skipped():
    assert _client_operations('trace', {'x-usrv-client-codegen': False}) == []


def test_head_with_body_is_rejected_by_client():
    with pytest.raises(chaotic_error.BaseError) as exc:
        _client_operation('head', {'requestBody': _BODY})
    assert exc.value.msg == (
        'Operation "HEAD /test/{id}": a HEAD request cannot carry a request body, remove "requestBody"'
    )


def test_trace_is_allowed_for_handlers():
    spec = handler_translator.HandlerTranslator(
        _service(_openapi('trace', {})),
        cpp_namespace='test_namespace',
        include_dirs=[],
    ).spec()
    assert [op.method for op in spec.operations] == ['TRACE']
