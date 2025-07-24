from chaotic_openapi.front import model
import pytest

from chaotic import error
from chaotic.front import types


def test_empty_openapi(simple_parser):
    assert simple_parser({
        'openapi': '3.0.0',
        'info': {'title': '', 'version': '1.0'},
        'paths': {},
    }) == model.Service(name='test', description='', operations=[])


def test_openapi_body_schema(simple_parser):
    assert simple_parser({
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
                security=[],
            )
        ],
    )


def test_openapi_security(simple_parser):
    assert simple_parser({
        'openapi': '3.0.0',
        'info': {'title': '', 'version': '1.0'},
        'components': {
            'securitySchemes': {
                'api_key': {
                    'type': 'apiKey',
                    'name': 'api_key',
                    'in': 'header',
                },
                'oauth': {
                    'type': 'oauth2',
                    'flows': {
                        'implicit': {
                            'refreshUrl': 'https://example.com/api/oauth/dialog',
                            'authorizationUrl': 'https://example.com/api/oauth/dialog',
                            'scopes': {'read': 'read data', 'other': '-'},
                        },
                        'authorizationCode': {
                            'authorizationUrl': 'https://example.com/api/oauth/dialog',
                            'tokenUrl': 'https://example.com/api/oauth/token',
                            'scopes': {'write': 'modify data', 'read': 'read data', 'other': '-'},
                        },
                    },
                },
                'oauth_other': {'$ref': '#/components/securitySchemes/oauth'},
            },
        },
        'security': {'api_key': [], 'oauth': ['write', 'read']},
        'paths': {
            '/': {
                'get': {'responses': {}, 'security': {'api_key': [], 'oauth': ['read']}},
                'post': {'responses': {}, 'security': {'api_key': [], 'oauth': ['write']}},
                'put': {'responses': {}},
            }
        },
    }) == model.Service(
        name='test',
        description='',
        security={
            '<inline>#/components/securitySchemes/api_key': model.ApiKeySecurity(
                description='', name='api_key', in_=model.SecurityIn.header
            ),
            '<inline>#/components/securitySchemes/oauth': model.OAuthSecurity(
                description='',
                flows=[
                    model.ImplicitFlow(
                        refreshUrl='https://example.com/api/oauth/dialog',
                        scopes={'read': 'read data', 'other': '-'},
                        authorizationUrl='https://example.com/api/oauth/dialog',
                    ),
                    model.AuthCodeFlow(
                        refreshUrl='',
                        scopes={'write': 'modify data', 'read': 'read data', 'other': '-'},
                        authorizationUrl='https://example.com/api/oauth/dialog',
                        tokenUrl='https://example.com/api/oauth/token',
                    ),
                ],
            ),
            '<inline>#/components/securitySchemes/oauth_other': model.OAuthSecurity(
                description='',
                flows=[
                    model.ImplicitFlow(
                        refreshUrl='https://example.com/api/oauth/dialog',
                        scopes={'read': 'read data', 'other': '-'},
                        authorizationUrl='https://example.com/api/oauth/dialog',
                    ),
                    model.AuthCodeFlow(
                        refreshUrl='',
                        scopes={'write': 'modify data', 'read': 'read data', 'other': '-'},
                        authorizationUrl='https://example.com/api/oauth/dialog',
                        tokenUrl='https://example.com/api/oauth/token',
                    ),
                ],
            ),
        },
        operations=[
            model.Operation(
                description='',
                path='/',
                method='get',
                operationId='Get',
                parameters=[],
                responses={},
                requestBody=[],
                security=[
                    model.ApiKeySecurity(description='', name='api_key', in_=model.SecurityIn.header),
                    model.OAuthSecurity(
                        description='',
                        flows=[
                            model.ImplicitFlow(
                                refreshUrl='https://example.com/api/oauth/dialog',
                                scopes={'read': 'read data'},
                                authorizationUrl='https://example.com/api/oauth/dialog',
                            ),
                            model.AuthCodeFlow(
                                refreshUrl='',
                                scopes={'read': 'read data'},
                                authorizationUrl='https://example.com/api/oauth/dialog',
                                tokenUrl='https://example.com/api/oauth/token',
                            ),
                        ],
                    ),
                ],
            ),
            model.Operation(
                description='',
                path='/',
                method='post',
                operationId='Post',
                parameters=[],
                responses={},
                requestBody=[],
                security=[
                    model.ApiKeySecurity(description='', name='api_key', in_=model.SecurityIn.header),
                    model.OAuthSecurity(
                        description='',
                        flows=[
                            model.ImplicitFlow(
                                refreshUrl='https://example.com/api/oauth/dialog',
                                scopes={},
                                authorizationUrl='https://example.com/api/oauth/dialog',
                            ),
                            model.AuthCodeFlow(
                                refreshUrl='',
                                scopes={'write': 'modify data'},
                                authorizationUrl='https://example.com/api/oauth/dialog',
                                tokenUrl='https://example.com/api/oauth/token',
                            ),
                        ],
                    ),
                ],
            ),
            model.Operation(
                description='',
                path='/',
                method='put',
                operationId='Put',
                parameters=[],
                responses={},
                requestBody=[],
                security=[
                    model.ApiKeySecurity(description='', name='api_key', in_=model.SecurityIn.header),
                    model.OAuthSecurity(
                        description='',
                        flows=[
                            model.ImplicitFlow(
                                refreshUrl='https://example.com/api/oauth/dialog',
                                scopes={'read': 'read data'},
                                authorizationUrl='https://example.com/api/oauth/dialog',
                            ),
                            model.AuthCodeFlow(
                                refreshUrl='',
                                scopes={'write': 'modify data', 'read': 'read data'},
                                authorizationUrl='https://example.com/api/oauth/dialog',
                                tokenUrl='https://example.com/api/oauth/token',
                            ),
                        ],
                    ),
                ],
            ),
        ],
    )


def test_openapi_parameters(simple_parser):
    assert simple_parser({
        'openapi': '3.0.0',
        'info': {'title': '', 'version': '1.0'},
        'components': {
            'parameters': {
                'pamparam1': {
                    'name': 'pamparam1',
                    'in': 'header',
                    'description': '',
                    'required': True,
                    'schema': {
                        'type': 'array',
                        'items': {
                            'type': 'integer',
                        },
                    },
                    'style': 'simple',
                }
            },
        },
        'paths': {
            '/': {
                'parameters': [
                    {'$ref': '#/components/parameters/pamparam1'},
                    {
                        'name': 'pamparam2',
                        'in': 'path',
                        'description': '',
                        'required': True,
                        'schema': {'type': 'string'},
                    },
                    {
                        'name': 'pamparam2',
                        'in': 'query',
                        'description': '',
                        'required': False,
                        'schema': {'type': 'array', 'items': {'type': 'string'}},
                        'style': 'form',
                        'explode': True,
                    },
                ],
                'get': {
                    'responses': {},
                    'parameters': [
                        {
                            'name': 'pamparam1',
                            'in': 'header',
                            'description': 'override',
                            'required': True,
                            'schema': {
                                'type': 'array',
                                'items': {
                                    'type': 'number',
                                },
                            },
                            'style': 'simple',
                        },
                        {
                            'name': 'pamparam2',
                            'in': 'query',
                            'description': 'override',
                            'required': False,
                            'schema': {'type': 'array', 'items': {'type': 'number'}},
                            'style': 'form',
                            'explode': True,
                        },
                    ],
                },
            },
        },
    }) == model.Service(
        name='test',
        description='',
        parameters={
            '<inline>#/components/parameters/pamparam1': model.Parameter(
                name='pamparam1',
                in_=model.In.header,
                description='',
                required=True,
                schema=types.Array(items=types.Integer()),
                style=model.Style.simple,
                examples={},
                deprecated=False,
                allowEmptyValue=False,
            )
        },
        operations=[
            model.Operation(
                description='',
                path='/',
                method='get',
                operationId='Get',
                responses={},
                requestBody=[],
                security=[],
                parameters=[
                    model.Parameter(
                        name='pamparam1',
                        in_=model.In.header,
                        description='override',
                        required=True,
                        schema=types.Array(items=types.Number()),
                        style=model.Style.simple,
                        examples={},
                        deprecated=False,
                        allowEmptyValue=False,
                    ),
                    model.Parameter(
                        name='pamparam2',
                        in_=model.In.path,
                        description='',
                        required=True,
                        schema=types.String(),
                        style=model.Style.simple,
                        examples={},
                        deprecated=False,
                        allowEmptyValue=False,
                    ),
                    model.Parameter(
                        name='pamparam2',
                        in_=model.In.query,
                        description='override',
                        required=False,
                        schema=types.Array(items=types.Number()),
                        style=model.Style.form,
                        examples={},
                        deprecated=False,
                        allowEmptyValue=False,
                    ),
                ],
            )
        ],
    )


def test_unknown_usrv_tag(simple_parser):
    expected = """
===============================================================
Unhandled error while processing <inline>
Path "paths./.get", Format "openapi"
Error:
Assertion failed, Field x-usrv-tag is not allowed in this context
==============================================================="""

    with pytest.raises(error.BaseError, match=expected):
        simple_parser({
            'openapi': '3.0.0',
            'info': {'title': '', 'version': '1.0'},
            'paths': {
                '/': {
                    'get': {
                        'x-usrv-tag': '1',
                        'responses': {},
                    },
                },
            },
        })
