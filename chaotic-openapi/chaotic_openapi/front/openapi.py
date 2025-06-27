import enum
from typing import Any
from typing import Dict
from typing import List
from typing import Optional
from typing import Union

import pydantic

from . import base_model
from . import errors


# https://spec.openapis.org/oas/v3.0.0.html#info-object
class Info(base_model.BaseModel):
    title: str
    description: Optional[str] = None
    termsOfService: Optional[str] = None
    contact: Any = None
    license: Any = None
    version: str


# https://spec.openapis.org/oas/v3.0.0.html#server-object
class Server(base_model.BaseModel):
    url: str
    description: Optional[str] = None
    variables: Dict[str, Any] = pydantic.Field(default_factory=dict)


Schema = Any


# https://spec.openapis.org/oas/v3.0.0.html#style-values
class Style(str, enum.Enum):
    matrix = 'matrix'
    label = 'label'
    form = 'form'
    simple = 'simple'
    spaceDelimited = 'spaceDelimited'
    pipeDelimited = 'pipeDelimited'
    # TODO: deepObject


# https://spec.openapis.org/oas/v3.0.0.html#header-object
class Header(base_model.BaseModel):
    description: Optional[str] = None
    required: bool = False
    deprecated: bool = False
    allowEmptyValue: bool = False

    style: Optional[Style] = None
    explode: Optional[bool] = None
    allowReserved: bool = False
    schema_: Schema = pydantic.Field(alias='schema')
    example: Any = None
    examples: Dict[str, Any] = pydantic.Field(default_factory=dict)


# https://spec.openapis.org/oas/v3.0.0.html#media-type-object
class MediaType(base_model.BaseModel):
    schema_: Schema = pydantic.Field(alias='schema', default=None)
    example: Any = None
    examples: Dict[str, Any] = pydantic.Field(default_factory=dict)
    # encoding: Dict[str, Encoding] = {}


# https://spec.openapis.org/oas/v3.0.0.html#reference-object
class Ref(base_model.BaseModel):
    ref: str = pydantic.Field(alias='$ref')


# https://spec.openapis.org/oas/v3.0.0.html#responses-object
class Response(base_model.BaseModel):
    description: str
    headers: Dict[str, Union[Header, Ref]] = pydantic.Field(default_factory=dict)
    content: Dict[str, MediaType] = pydantic.Field(default_factory=dict)
    # TODO: links


class In(str, enum.Enum):
    path = 'path'
    query = 'query'
    header = 'header'
    cookie = 'cookie'


# https://spec.openapis.org/oas/v3.0.0.html#parameter-object
class Parameter(base_model.BaseModel):
    name: str
    in_: In = pydantic.Field(alias='in')
    description: Optional[str] = None
    required: bool = False
    deprecated: bool = False
    allowEmptyValue: bool = False

    style: Optional[Style] = None
    explode: Optional[bool] = None
    allowReserved: bool = False
    schema_: Schema = pydantic.Field(alias='schema')
    example: Any = None
    examples: Dict[str, Any] = pydantic.Field(default_factory=dict)

    # content: Dict[str, MediaType] = {}

    def model_post_init(self, context: Any, /) -> None:
        super().model_post_init(context)
        if self.style:
            return

        self.style = {
            In.query: Style.form,
            In.path: Style.simple,
            In.header: Style.simple,
            In.cookie: Style.form,
        }[self.in_]


# https://spec.openapis.org/oas/v3.0.0.html#request-body-object
class RequestBody(base_model.BaseModel):
    description: Optional[str] = None
    content: Dict[str, MediaType]
    required: bool = False


class SecurityType(str, enum.Enum):
    apiKey = 'apiKey'
    http = 'http'
    oauth2 = 'oauth2'
    openIdConnect = 'openIdConnect'


class SecurityIn(str, enum.Enum):
    query = 'query'
    header = 'header'
    cookie = 'cookie'


class ImplicitFlow(base_model.BaseModel):
    refreshUrl: Optional[str] = None
    scopes: Dict[str, str] = pydantic.Field(default_factory=dict)
    authorizationUrl: str


class PasswordFlow(base_model.BaseModel):
    refreshUrl: Optional[str] = None
    scopes: Dict[str, str] = pydantic.Field(default_factory=dict)
    tokenUrl: str


class ClientCredFlow(base_model.BaseModel):
    refreshUrl: Optional[str] = None
    scopes: Dict[str, str] = pydantic.Field(default_factory=dict)
    tokenUrl: str


class AuthCodeFlow(base_model.BaseModel):
    refreshUrl: Optional[str] = None
    scopes: Dict[str, str] = pydantic.Field(default_factory=dict)
    authorizationUrl: str
    tokenUrl: str


# https://spec.openapis.org/oas/v3.0.0.html#oauth-flows-object
class OAuthFlows(base_model.BaseModel):
    implicit: Optional[ImplicitFlow] = None
    password: Optional[PasswordFlow] = None
    clientCredentials: Optional[ClientCredFlow] = None
    authorizationCode: Optional[AuthCodeFlow] = None


# https://spec.openapis.org/oas/v3.0.0.html#security-scheme-object
class SecurityScheme(base_model.BaseModel):
    type: SecurityType
    description: Optional[str] = None
    name: Optional[str] = None
    in_: Optional[SecurityIn] = pydantic.Field(alias='in', default=None)
    scheme_: Optional[str] = pydantic.Field(alias='scheme', default=None)
    bearerFormat: Optional[str] = None
    flows: Optional[OAuthFlows] = None
    openIdConnectUrl: Optional[str] = None

    def model_post_init(self, context: Any, /) -> None:
        super().model_post_init(context)

        if self.type == SecurityType.apiKey:
            if not self.name:
                raise ValueError(errors.missing_field_msg('name'))
            if not self.in_:
                raise ValueError(errors.missing_field_msg('in'))
        elif self.type == SecurityType.oauth2:
            if not self.flows:
                raise ValueError(errors.missing_field_msg('flows'))
        elif self.type == SecurityType.http:
            if not self.scheme_:
                raise ValueError(errors.missing_field_msg('scheme'))
        elif self.type == SecurityType.openIdConnect:
            if not self.openIdConnectUrl:
                raise ValueError(errors.missing_field_msg('openIdConnectUrl'))


SecuritySchemes = Dict[str, Union[SecurityScheme, Ref]]


# https://spec.openapis.org/oas/v3.0.0.html#security-requirement-object
Security = Dict[str, List[str]]


# https://spec.openapis.org/oas/v3.0.0.html#components-object
class Components(base_model.BaseModel):
    schemas: Dict[str, Schema] = pydantic.Field(default_factory=dict)
    requests: Dict[str, Any] = pydantic.Field(default_factory=dict)  # TODO
    responses: Dict[str, Response] = pydantic.Field(default_factory=dict)
    parameters: Dict[str, Parameter] = pydantic.Field(default_factory=dict)
    headers: Dict[str, Header] = pydantic.Field(default_factory=dict)
    requestBodies: Dict[str, RequestBody] = pydantic.Field(default_factory=dict)
    securitySchemes: SecuritySchemes = pydantic.Field(default_factory=dict)


class XTaxiMiddlewares(base_model.BaseModel):
    tvm: bool = True


# https://spec.openapis.org/oas/v3.0.0.html#operation-object
class Operation(base_model.BaseModel):
    tags: List[str] = pydantic.Field(default_factory=list)
    summary: Optional[str] = None
    description: str = ''
    externalDocs: Any = None

    operationId: Optional[str] = None
    parameters: List[Union[Parameter, Ref]] = pydantic.Field(default_factory=list)
    requestBody: Optional[Union[RequestBody, Ref]] = None
    responses: Dict[Union[str, int], Union[Response, Ref]]
    deprecated: bool = False
    security: Optional[Security] = None
    servers: List[Server] = pydantic.Field(default_factory=list)

    x_taxi_middlewares: Optional[XTaxiMiddlewares] = pydantic.Field(
        default=None,
        alias='x-taxi-middlewares',
    )


# https://spec.openapis.org/oas/v3.0.0.html#path-item-object
class Path(base_model.BaseModel):
    summary: Optional[str] = None
    description: str = ''

    get: Optional[Operation] = None
    post: Optional[Operation] = None
    put: Optional[Operation] = None
    delete: Optional[Operation] = None
    options: Optional[Operation] = None
    head: Optional[Operation] = None
    patch: Optional[Operation] = None
    trace: Optional[Operation] = None

    servers: List[Server] = pydantic.Field(default_factory=list)
    parameters: List[Union[Parameter, Ref]] = pydantic.Field(default_factory=list)


class XTaxiClientQos(base_model.BaseModel):
    taxi_config: str = pydantic.Field(alias='taxi-config')


# https://spec.openapis.org/oas/v3.0.0.html#schema
class OpenApi(base_model.BaseModel):
    openapi: str = '3.0.0'
    info: Optional[Info] = None
    servers: List[Server] = pydantic.Field(default_factory=list)
    paths: Dict[str, Path] = pydantic.Field(default_factory=dict)
    components: Components = Components()
    security: Security = pydantic.Field(default_factory=dict)
    tags: List[Any] = pydantic.Field(default_factory=list)
    externalDocs: Any = None

    x_taxi_client_qos: Optional[XTaxiClientQos] = pydantic.Field(
        default=None,
        alias='x-taxi-client-qos',
    )
    x_taxi_middlewares: Optional[XTaxiMiddlewares] = pydantic.Field(
        default=None,
        alias='x-taxi-middlewares',
    )

    def validate_security(self, security: Optional[Security]) -> None:
        if not security:
            return

        for name, values in security.items():
            if name not in self.components.securitySchemes:
                raise ValueError(
                    f'Undefined security name="{name}". Expected on of: {self.components.securitySchemes.keys()}'
                )
            sec_scheme = self.components.securitySchemes[name]

            if isinstance(sec_scheme, Ref):
                if sec_scheme not in self.components.securitySchemes:
                    raise ValueError(
                        f'Invalid reference "{sec_scheme}". Expected one of: "{self.components.securitySchemes.keys()}"'
                    )
            elif isinstance(sec_scheme, SecurityScheme):
                if sec_scheme.type not in [SecurityType.oauth2, SecurityType.openIdConnect]:
                    if len(values) != 0:
                        raise ValueError(f'For security "{name}" the array must be empty')

    def model_post_init(self, context: Any, /) -> None:
        super().model_post_init(context)

        self.validate_security(self.security)

        for path in self.paths.values():
            if path.get:
                self.validate_security(path.get.security)
            if path.post:
                self.validate_security(path.post.security)
            if path.put:
                self.validate_security(path.put.security)
            if path.delete:
                self.validate_security(path.delete.security)
            if path.options:
                self.validate_security(path.options.security)
            if path.head:
                self.validate_security(path.head.security)
            if path.patch:
                self.validate_security(path.patch.security)
            if path.trace:
                self.validate_security(path.trace.security)

    @staticmethod
    def schema_type() -> str:
        return 'openapi'
