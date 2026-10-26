from chaotic.back.cpp import type_name
from chaotic.back.cpp import types as cpp_types
from chaotic.front import types as front_types
from chaotic_openapi.back.cpp.client import types


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
                operation_id=None,
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
                    ),
                ],
            ),
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
                    },
                },
            },
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
                operation_id=None,
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
                    ),
                ],
            ),
        ],
    )


def test_media_type_without_schema(translate_single_schema):
    schema = {
        'openapi': '3.0.0',
        'info': {'title': '', 'version': '1.0'},
        'paths': {
            '/': {
                'post': {
                    'requestBody': {'required': True, 'content': {'application/json': {}}},
                    'responses': {
                        '200': {'description': 'OK', 'content': {'application/json': {}}},
                        '204': {'description': 'No body'},
                    },
                },
            },
        },
    }
    spec = translate_single_schema(schema)
    operation = spec.operations[0]
    # A media type without a schema carries any value, a response without content has no body
    assert operation.request_bodies[0].cpp_type() == 'USERVER_NAMESPACE::formats::json::Value'
    body, no_body = operation.responses
    assert isinstance(body.single_body(), cpp_types.CppAnyValue)
    assert body.single_body().cpp_user_name() == 'USERVER_NAMESPACE::formats::json::Value'
    assert no_body.body == {}
    assert spec.internal_schemas == {}
