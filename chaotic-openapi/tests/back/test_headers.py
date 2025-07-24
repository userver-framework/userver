from chaotic_openapi.back.cpp_client import types

from chaotic.back.cpp import type_name
from chaotic.back.cpp import types as cpp_types
from chaotic.front import types as chaotic_types


def test_headers(translate_single_schema):
    schema = {
        'openapi': '3.0.0',
        'info': {'title': '', 'version': '1.0'},
        'paths': {
            '/': {
                'get': {
                    'responses': {
                        '200': {
                            'description': 'OK',
                            'headers': {
                                'Header': {
                                    'description': 'header description',
                                    'schema': {
                                        'type': 'integer',
                                    },
                                },
                            },
                        },
                    },
                },
            },
        },
    }
    assert translate_single_schema(schema) == types.ClientSpec(
        client_name='test',
        cpp_namespace='test_namespace',
        dynamic_config='',
        operations=[
            types.Operation(
                method='GET',
                path='/',
                request_bodies=[],
                responses=[
                    types.Response(
                        status=200,
                        headers=[
                            types.Parameter(
                                description='header description',
                                raw_name='Header',
                                cpp_name='Header',
                                cpp_type=cpp_types.CppPrimitiveType(
                                    raw_cpp_type=type_name.TypeName('int'),
                                    user_cpp_type=None,
                                    nullable=True,
                                    json_schema=chaotic_types.Integer(
                                        type='integer',
                                    ),
                                    validators=cpp_types.CppPrimitiveValidator(
                                        namespace='::test_namespace::root_::get', prefix='Response200HeaderHeader'
                                    ),
                                ),
                                parser='openapi::TrivialParameter<openapi::In::kHeader, kHeader, int, int>',
                                required=False,
                            )
                        ],
                        body={},
                    )
                ],
            )
        ],
    )


def test_header_ref(translate_single_schema):
    schema = {
        'openapi': '3.0.0',
        'info': {'title': '', 'version': '1.0'},
        'paths': {
            '/': {
                'get': {
                    'responses': {
                        '200': {
                            'description': 'OK',
                            'headers': {
                                'X-Header': {
                                    '$ref': '#/components/headers/XHeader',
                                },
                            },
                        },
                    }
                }
            }
        },
        'components': {
            'headers': {
                'XHeader': {
                    'description': 'header description',
                    'schema': {
                        'type': 'boolean',
                    },
                },
            },
        },
    }
    assert translate_single_schema(schema) == types.ClientSpec(
        client_name='test',
        cpp_namespace='test_namespace',
        dynamic_config='',
        operations=[
            types.Operation(
                method='GET',
                path='/',
                request_bodies=[],
                responses=[
                    types.Response(
                        status=200,
                        body={},
                        headers=[
                            types.Parameter(
                                description='header description',
                                raw_name='XHeader',
                                cpp_name='XHeader',
                                cpp_type=cpp_types.CppPrimitiveType(
                                    raw_cpp_type=type_name.TypeName('bool'),
                                    user_cpp_type=None,
                                    nullable=True,
                                    json_schema=chaotic_types.Boolean(
                                        type='boolean',
                                    ),
                                    validators=cpp_types.CppPrimitiveValidator(prefix=''),
                                ),
                                parser='openapi::TrivialParameter<openapi::In::kHeader, kXHeader, bool, bool>',
                                required=False,
                            ),
                        ],
                    )
                ],
            )
        ],
    )
