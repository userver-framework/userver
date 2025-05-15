from chaotic_openapi.front import model

from chaotic.front import types


def test_empty_swagger(simple_parser):
    assert simple_parser({
        'swagger': '2.0',
        'info': {},
        'paths': {},
    }) == model.Service(name='test', description='', operations=[])


def test_swagger_body_schema(simple_parser):
    assert simple_parser(
        {
            'swagger': '2.0',
            'info': {},
            'paths': {
                '/': {
                    'get': {
                        'consumes': ['application/json'],
                        'parameters': [
                            {
                                'name': 'pamparam',
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
                security=[],
            )
        ],
    )


def test_swagger_responses(simple_parser):
    assert simple_parser(
        {
            'swagger': '2.0',
            'info': {},
            'produces': ['text/plain; charset=utf-8', 'application/json'],
            'responses': {
                '500': {
                    'description': 'internal error',
                    'schema': {'type': 'string'},
                    'headers': {
                        'X-Header-Name-1': {'type': 'string'},
                    },
                    'examples': {
                        'text/plain; charset=utf-8': {'example1': 'any', 'example2': 'any'},
                        'application/json': {'example1': 'any'},
                    },
                }
            },
            'paths': {
                '/': {
                    'get': {
                        'produces': ['text/plain; charset=utf-8', 'application/json'],
                        'responses': {
                            '200': {
                                'description': 'ok',
                                'schema': {'type': 'string'},
                                'headers': {
                                    'X-Header-Name-1': {'type': 'string'},
                                    'X-Header-Name-2': {'type': 'array', 'items': {'type': 'integer'}},
                                },
                                'examples': {
                                    'text/plain; charset=utf-8': {'example1': 'any', 'example2': 'any'},
                                    'application/json': {'example1': 'any'},
                                },
                            },
                            '500': {'$ref': '#/responses/500'},
                        },
                    },
                },
            },
        },
    ) == model.Service(
        name='test',
        description='',
        responses={
            '<inline>#/responses/500': model.Response(
                description='internal error',
                headers={
                    'X-Header-Name-1': model.Parameter(
                        name='X-Header-Name-1',
                        in_=model.In.header,
                        description='',
                        examples={},
                        deprecated=False,
                        required=False,
                        allowEmptyValue=False,
                        style=model.Style.simple,
                        schema=types.String(),
                    )
                },
                content={
                    'text/plain; charset=utf-8': model.MediaType(
                        schema=types.String(),
                        examples={'example1': 'any', 'example2': 'any'},
                    ),
                    'application/json': model.MediaType(
                        schema=types.String(),
                        examples={'example1': 'any'},
                    ),
                },
            )
        },
        operations=[
            model.Operation(
                description='',
                path='/',
                method='get',
                operationId='Get',
                parameters=[],
                requestBody=[],
                responses={
                    200: model.Response(
                        description='ok',
                        headers={
                            'X-Header-Name-1': model.Parameter(
                                name='X-Header-Name-1',
                                in_=model.In.header,
                                description='',
                                examples={},
                                deprecated=False,
                                required=False,
                                allowEmptyValue=False,
                                style=model.Style.simple,
                                schema=types.String(),
                            ),
                            'X-Header-Name-2': model.Parameter(
                                name='X-Header-Name-2',
                                in_=model.In.header,
                                description='',
                                examples={},
                                deprecated=False,
                                required=False,
                                allowEmptyValue=False,
                                style=model.Style.simple,
                                schema=types.Array(items=types.Integer()),
                            ),
                        },
                        content={
                            'text/plain; charset=utf-8': model.MediaType(
                                schema=types.String(),
                                examples={'example1': 'any', 'example2': 'any'},
                            ),
                            'application/json': model.MediaType(
                                schema=types.String(),
                                examples={'example1': 'any'},
                            ),
                        },
                    ),
                    500: model.Ref('<inline>#/responses/500'),
                },
                security=[],
            )
        ],
    )


def test_swagger_securuty(simple_parser):
    assert simple_parser(
        {
            'swagger': '2.0',
            'info': {},
            'securityDefinitions': {
                'api_key': {
                    'type': 'apiKey',
                    'name': 'api_key',
                    'in': 'header',
                },
                'oauth_implicit': {
                    'type': 'oauth2',
                    'flow': 'implicit',
                    'authorizationUrl': 'https://example.com/api/oauth/dialog',
                    'scopes': {'read': 'read data', 'other': '-'},
                },
                'oauth_code': {
                    'type': 'oauth2',
                    'flow': 'accessCode',
                    'authorizationUrl': 'https://example.com/api/oauth/dialog',
                    'tokenUrl': 'https://example.com/api/oauth/token',
                    'scopes': {'write': 'modify data', 'read': 'read data', 'other': '-'},
                },
            },
            'security': {
                'api_key': [],
                'oauth_implicit': ['read'],
                'oauth_code': ['read', 'write'],
            },
            'paths': {
                '/': {
                    'get': {
                        'parameters': [],
                        'responses': {},
                        'security': {
                            'api_key': [],
                            'oauth_implicit': ['read'],
                            'oauth_code': ['read'],
                        },
                    },
                    'post': {
                        'parameters': [],
                        'responses': {},
                        'security': {
                            'api_key': [],
                            'oauth_implicit': ['write'],
                            'oauth_code': ['write'],
                        },
                    },
                    'put': {'parameters': [], 'responses': {}},
                }
            },
        },
    ) == model.Service(
        name='test',
        description='',
        security={
            '<inline>#/securityDefinitions/api_key': model.ApiKeySecurity(
                description='', name='api_key', in_=model.SecurityIn.header
            ),
            '<inline>#/securityDefinitions/oauth_implicit': model.OAuthSecurity(
                description='',
                flows=[
                    model.ImplicitFlow(
                        refreshUrl='',
                        scopes={'read': 'read data', 'other': '-'},
                        authorizationUrl='https://example.com/api/oauth/dialog',
                    ),
                ],
            ),
            '<inline>#/securityDefinitions/oauth_code': model.OAuthSecurity(
                description='',
                flows=[
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
                                refreshUrl='',
                                scopes={'read': 'read data'},
                                authorizationUrl='https://example.com/api/oauth/dialog',
                            ),
                        ],
                    ),
                    model.OAuthSecurity(
                        description='',
                        flows=[
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
                                refreshUrl='',
                                scopes={},
                                authorizationUrl='https://example.com/api/oauth/dialog',
                            ),
                        ],
                    ),
                    model.OAuthSecurity(
                        description='',
                        flows=[
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
                                refreshUrl='',
                                scopes={'read': 'read data'},
                                authorizationUrl='https://example.com/api/oauth/dialog',
                            )
                        ],
                    ),
                    model.OAuthSecurity(
                        description='',
                        flows=[
                            model.AuthCodeFlow(
                                refreshUrl='',
                                scopes={'write': 'modify data', 'read': 'read data'},
                                authorizationUrl='https://example.com/api/oauth/dialog',
                                tokenUrl='https://example.com/api/oauth/token',
                            )
                        ],
                    ),
                ],
            ),
        ],
    )


def test_swagger_parameters(simple_parser):
    assert simple_parser({
        'swagger': '2.0',
        'info': {},
        'consumes': ['application/json'],
        'parameters': {
            'pamparam1': {
                'name': 'pamparam1',
                'in': 'body',
                'description': '',
                'required': True,
                'schema': {'type': 'string'},
            },
        },
        'paths': {
            '/': {
                'parameters': [
                    {'$ref': '#/parameters/pamparam1'},
                    {
                        'name': 'pamparam2',
                        'in': 'header',
                        'description': '',
                        'required': True,
                        'type': 'array',
                        'items': {
                            'type': 'integer',
                        },
                    },
                    {
                        'name': 'pamparam2',
                        'in': 'path',
                        'description': '',
                        'required': True,
                        'type': 'string',
                    },
                ],
                'get': {
                    'consumes': ['text/plain; charset=utf-8', 'application/json'],
                    'responses': {},
                    'parameters': [
                        {
                            'name': 'pamparam1',
                            'in': 'body',
                            'description': 'override',
                            'required': True,
                            'schema': {'type': 'number'},
                        },
                        {
                            'name': 'pamparam2',
                            'in': 'path',
                            'description': 'override',
                            'required': True,
                            'type': 'number',
                        },
                    ],
                },
            },
        },
    }) == model.Service(
        name='test',
        description='',
        requestBodies={
            '<inline>#/requestBodies/pamparam1': [
                model.RequestBody(
                    content_type='application/json',
                    required=True,
                    schema=types.String(),
                ),
            ],
        },
        parameters={},
        operations=[
            model.Operation(
                description='',
                path='/',
                method='get',
                operationId='Get',
                responses={},
                requestBody=[
                    model.RequestBody(content_type='text/plain; charset=utf-8', required=True, schema=types.Number()),
                    model.RequestBody(content_type='application/json', required=True, schema=types.Number()),
                ],
                security=[],
                parameters=[
                    model.Parameter(
                        name='pamparam2',
                        in_=model.In.header,
                        description='',
                        required=True,
                        schema=types.Array(items=types.Integer()),
                        style=model.Style.simple,
                        examples={},
                        deprecated=False,
                        allowEmptyValue=False,
                    ),
                    model.Parameter(
                        name='pamparam2',
                        in_=model.In.path,
                        description='override',
                        required=True,
                        schema=types.Number(),
                        style=model.Style.simple,
                        examples={},
                        deprecated=False,
                        allowEmptyValue=False,
                    ),
                ],
            )
        ],
    )
