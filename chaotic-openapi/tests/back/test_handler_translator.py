from chaotic_openapi.back.cpp.handler import translator as handler_translator
from chaotic_openapi.back.cpp.handler.renderer import component_name
from chaotic_openapi.back.cpp.handler.renderer import make_op_relpath
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


_MINIMAL_SCHEMA = {
    'openapi': '3.0.0',
    'info': {'title': '', 'version': '1.0'},
    'paths': {
        '/testme': {
            'post': {
                'operationId': 'testmePost',
                'parameters': [],
                'responses': {200: {'description': 'OK'}},
            },
        },
    },
}


def test_basic_operation_produces_one_spec():
    spec = _translate(_MINIMAL_SCHEMA)
    assert len(spec.operations) == 1
    op = spec.operations[0]
    assert spec.service_name == 'test'
    assert component_name(op) == 'handler-testme-post'
    assert f'{spec.cpp_namespace}::{op.cpp_namespace()}' == 'handlers::test::testmepost'


def test_handler_name_from_operation_id():
    schema = {
        'openapi': '3.0.0',
        'info': {'title': '', 'version': '1.0'},
        'paths': {
            '/foo': {
                'get': {
                    'operationId': 'getFoo',
                    'responses': {200: {'description': 'OK'}},
                },
            },
        },
    }
    spec = _translate(schema)
    assert component_name(spec.operations[0]) == 'handler-get-foo'


def test_handler_name_fallback_from_path_and_method():
    schema = {
        'openapi': '3.0.0',
        'info': {'title': '', 'version': '1.0'},
        'paths': {
            '/api/v1/orders': {
                'get': {
                    'responses': {200: {'description': 'OK'}},
                },
            },
        },
    }
    spec = _translate(schema)
    assert component_name(spec.operations[0]) == 'handler-api-v1-orders-get'


def test_x_handler_codegen_false_skips_operation():
    schema = {
        'openapi': '3.0.0',
        'info': {'title': '', 'version': '1.0'},
        'paths': {
            '/testme': {
                'post': {
                    'x-usrv-handler-codegen': False,
                    'responses': {200: {'description': 'OK'}},
                },
            },
        },
    }
    spec = _translate(schema)
    assert spec.operations == []


def test_x_handler_codegen_true_includes_operation():
    schema = {
        'openapi': '3.0.0',
        'info': {'title': '', 'version': '1.0'},
        'paths': {
            '/testme': {
                'post': {
                    'x-usrv-handler-codegen': True,
                    'responses': {200: {'description': 'OK'}},
                },
            },
        },
    }
    spec = _translate(schema)
    assert len(spec.operations) == 1


def test_required_parameter():
    schema = {
        'openapi': '3.0.0',
        'info': {'title': '', 'version': '1.0'},
        'paths': {
            '/testme': {
                'get': {
                    'parameters': [
                        {
                            'in': 'query',
                            'name': 'q',
                            'required': True,
                            'schema': {'type': 'string'},
                        },
                    ],
                    'responses': {200: {'description': 'OK'}},
                },
            },
        },
    }
    spec = _translate(schema)
    param = spec.operations[0].parameters[0]
    assert param.required is True
    assert not param.cpp_type.nullable


def test_optional_parameter():
    schema = {
        'openapi': '3.0.0',
        'info': {'title': '', 'version': '1.0'},
        'paths': {
            '/testme': {
                'get': {
                    'parameters': [
                        {
                            'in': 'query',
                            'name': 'q',
                            'schema': {'type': 'string'},
                        },
                    ],
                    'responses': {200: {'description': 'OK'}},
                },
            },
        },
    }
    spec = _translate(schema)
    param = spec.operations[0].parameters[0]
    assert param.required is False
    assert param.cpp_type.nullable


def test_error_response_is_error():
    schema = {
        'openapi': '3.0.0',
        'info': {'title': '', 'version': '1.0'},
        'paths': {
            '/testme': {
                'get': {
                    'responses': {
                        200: {'description': 'OK'},
                        404: {'description': 'Not found'},
                    },
                },
            },
        },
    }
    spec = _translate(schema)
    op = spec.operations[0]
    assert not op.responses[0].is_error()
    assert op.responses[1].is_error()


def test_op_relpath_from_operation_id():
    spec = _translate(_MINIMAL_SCHEMA)
    assert make_op_relpath(spec.operations[0]) == 'testmepost'


def test_op_relpath_from_path_and_method():
    schema = {
        'openapi': '3.0.0',
        'info': {'title': '', 'version': '1.0'},
        'paths': {
            '/api/orders': {
                'get': {
                    'responses': {200: {'description': 'OK'}},
                },
            },
        },
    }
    spec = _translate(schema)
    assert make_op_relpath(spec.operations[0]) == 'api_orders/get'


def test_multiple_operations_all_translated():
    schema = {
        'openapi': '3.0.0',
        'info': {'title': '', 'version': '1.0'},
        'paths': {
            '/a': {
                'get': {'responses': {200: {'description': 'OK'}}},
                'post': {'responses': {200: {'description': 'OK'}}},
            },
        },
    }
    spec = _translate(schema)
    assert len(spec.operations) == 2


def test_multiple_content_type_body():
    schema = {
        'openapi': '3.0.0',
        'info': {'title': '', 'version': '1.0'},
        'paths': {
            '/upload': {
                'post': {
                    'requestBody': {
                        'content': {
                            'application/json': {'schema': {'type': 'string'}},
                            'application/octet-stream': {'schema': {'type': 'string'}},
                        },
                    },
                    'responses': {200: {'description': 'OK'}},
                },
            },
        },
    }
    spec = _translate(schema)
    assert len(spec.operations) == 1
    op = spec.operations[0]
    assert len(op.request_bodies) == 2
    assert op.request_bodies[0].content_type == 'application/json'
    assert op.request_bodies[1].content_type == 'application/octet-stream'
