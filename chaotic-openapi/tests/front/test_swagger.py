from chaotic_openapi.front import model
from chaotic_openapi.front import parser as front_parser

from chaotic.front import types


def parse_single_schema(schema):
    parser = front_parser.Parser('test')
    parser.parse_schema(schema, '<inline>')
    return parser.service()


def test_empty_swagger():
    assert parse_single_schema({
        'swagger': '2.0',
        'info': {},
        'paths': {},
    }) == model.Service(name='test', description='', operations=[])


def test_empty_openapi():
    assert parse_single_schema({
        'openapi': '3.0.0',
        'info': {'title': '', 'version': '1.0'},
        'paths': {},
    }) == model.Service(name='test', description='', operations=[])


def test_openapi_body_schema():
    assert parse_single_schema({
        'openapi': '3.0.0',
        'info': {'title': '', 'version': '1.0'},
        'paths': {
            '/': {
                'get': {
                    'requestBody': {
                        'required': True,
                        'content': {
                            'application/json': {
                                'schema': {
                                    'type': 'boolean',
                                },
                            },
                        },
                    },
                    'responses': {},
                },
            },
        },
    }) == model.Service(
        name='test',
        description='',
        operations=[
            model.Operation(
                description='',
                path='/',
                method='get',
                operationId='Get',
                parameters=[],
                requestBody=[
                    model.RequestBody(
                        content_type='application/json',
                        schema=types.Boolean(),
                        required=True,
                    )
                ],
                responses={},
            )
        ],
    )


def test_swagger_body_schema():
    assert parse_single_schema(
        {
            'swagger': '2.0',
            'info': {},
            'paths': {
                '/': {
                    'get': {
                        'parameters': [
                            {
                                'in': 'body',
                                'required': True,
                                'schema': {
                                    'type': 'boolean',
                                },
                            },
                        ],
                        'responses': {},
                    },
                },
            },
        },
    ) == model.Service(
        name='test',
        description='',
        operations=[
            model.Operation(
                description='',
                path='/',
                method='get',
                operationId='Get',
                parameters=[],
                requestBody=[
                    model.RequestBody(
                        content_type='application/json',
                        schema=types.Boolean(),
                        required=True,
                    )
                ],
                responses={},
            )
        ],
    )
