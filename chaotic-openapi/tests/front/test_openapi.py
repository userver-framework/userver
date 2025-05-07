from chaotic_openapi.front import model

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
