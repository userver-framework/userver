import pytest

from chaotic import error as chaotic_error
from chaotic_openapi.back.cpp.client import translator as client_translator
from chaotic_openapi.back.cpp.handler import translator as handler_translator
from chaotic_openapi.front import parser as front_parser


def _openapi(paths: dict) -> dict:
    return {'openapi': '3.0.0', 'info': {'title': '', 'version': '1.0'}, 'paths': paths}


def _operation(operation_id: str | None = None) -> dict:
    operation: dict = {'responses': {}}
    if operation_id is not None:
        operation['operationId'] = operation_id
    return operation


def _service(paths: dict):
    parser = front_parser.Parser('test')
    parser.parse_schema(_openapi(paths), '<inline>', '<inline>')
    return parser.service()


def _translate_client(paths: dict):
    return client_translator.Translator(
        _service(paths),
        dynamic_config='',
        cpp_namespace='test_namespace',
        include_dirs=[],
        middleware_plugins=[],
    ).spec()


def _translate_handlers(paths: dict):
    return handler_translator.HandlerTranslator(
        _service(paths),
        cpp_namespace='test_namespace',
        include_dirs=[],
    ).spec()


TRANSLATORS = [_translate_client, _translate_handlers]


@pytest.mark.parametrize('translate', TRANSLATORS)
@pytest.mark.parametrize('operation_id', ['getUser', 'get_user', '_internal', 'GetUser2'])
def test_valid_operation_id(translate, operation_id):
    spec = translate({'/user': {'get': _operation(operation_id)}})
    assert [op.operation_id for op in spec.operations] == [operation_id]


@pytest.mark.parametrize('translate', TRANSLATORS)
@pytest.mark.parametrize('operation_id', ['get-user', '2fa', 'get user', 'delete', 'class', 'getUser!'])
def test_invalid_operation_id(translate, operation_id):
    with pytest.raises(chaotic_error.BaseError) as exc:
        translate({'/user': {'get': _operation(operation_id)}})
    assert exc.value.infile_path == '/get/operationId'
    assert exc.value.msg == (
        f'Operation "GET /user": operationId "{operation_id}" is not a valid C++ identifier. '
        'Use letters, digits and underscores, do not start with a digit and do not use a C++ keyword'
    )


@pytest.mark.parametrize('translate', TRANSLATORS)
@pytest.mark.parametrize('second_operation_id', ['GetUser', 'getuser'])
def test_operation_id_namespace_clash(translate, second_operation_id):
    with pytest.raises(chaotic_error.BaseError) as exc:
        translate({
            '/user': {'get': _operation('GetUser')},
            '/other': {'post': _operation(second_operation_id)},
        })
    assert exc.value.infile_path == '/post/operationId'
    assert exc.value.msg == (
        'Operation "POST /other": generates the C++ namespace "getuser" already generated for operation '
        '"GET /user". Give the operations distinct operationIds (operationIds that differ only in letter case clash)'
    )


def test_client_method_name_clash():
    with pytest.raises(chaotic_error.BaseError) as exc:
        _translate_client({
            '/test1': {'get': _operation()},
            '/other': {'post': _operation('Test1Get')},
        })
    assert exc.value.msg == (
        'Operation "POST /other": generates the client method "Test1Get" already generated for operation '
        '"GET /test1". Give the operations distinct operationIds'
    )


def test_handler_component_name_clash():
    with pytest.raises(chaotic_error.BaseError) as exc:
        _translate_handlers({
            '/test1': {'get': _operation()},
            '/other': {'post': _operation('Test1Get')},
        })
    assert exc.value.msg == (
        'Operation "POST /other": generates the handler component "handler-test1-get" already generated for '
        'operation "GET /test1". Give the operations distinct operationIds'
    )


@pytest.mark.parametrize('translate', TRANSLATORS)
def test_methods_of_one_path_are_distinct(translate):
    spec = translate({'/user': {'get': _operation(), 'post': _operation(), 'delete': _operation()}})
    assert [op.cpp_namespace() for op in spec.operations] == ['user::get', 'user::post', 'user::delete_']
