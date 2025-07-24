from chaotic_openapi.back.cpp_client import types

from chaotic.back.cpp import type_name
from chaotic.back.cpp import types as cpp_types
from chaotic.front import types as front_types


def test_response(translate_single_schema):
    schema = {
        'openapi': '3.0.0',
        'info': {'title': '', 'version': '1.0'},
        'paths': {
            '/': {
                'get': {
                    'responses': {
                        '200': {
                            'description': 'OK',
                            'content': {
                                'application/json': {
                                    'schema': {
                                        'type': 'boolean',
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
                        body={
                            'application/json': cpp_types.CppPrimitiveType(
                                raw_cpp_type=type_name.TypeName('bool'),
                                json_schema=front_types.Boolean(
                                    type='boolean',
                                    nullable=False,
                                ),
                                nullable=False,
                                user_cpp_type=None,
                                validators=cpp_types.CppPrimitiveValidator(),
                            ),
                        },
                    )
                ],
            )
        ],
    )


def test_response_ref(translate_single_schema):
    schema = {
        'openapi': '3.0.0',
        'info': {'title': '', 'version': '1.0'},
        'paths': {
            '/': {
                'get': {
                    'responses': {
                        '200': {
                            '$ref': '#/components/responses/Response200',
                        },
                    }
                }
            }
        },
        'components': {
            'responses': {
                'Response200': {
                    'description': 'OK',
                    'content': {
                        'application/json': {
                            'schema': {
                                'type': 'boolean',
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
                        body={
                            'application/json': cpp_types.CppPrimitiveType(
                                raw_cpp_type=type_name.TypeName('bool'),
                                json_schema=front_types.Boolean(
                                    type='boolean',
                                    nullable=False,
                                ),
                                nullable=False,
                                user_cpp_type=None,
                                validators=cpp_types.CppPrimitiveValidator(),
                            ),
                        },
                    )
                ],
            )
        ],
    )
