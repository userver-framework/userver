import enum
from typing import Any

import pydantic

from chaotic_openapi.front import base_model
from chaotic_openapi.front import errors


class Info(base_model.BaseModel):
    description: str | None = None
    title: str | None = None
    version: str | None = None


# https://spec.openapis.org/oas/v2.0.html#reference-object
class Ref(base_model.BaseModel):
    ref: str = pydantic.Field(alias='$ref')


class In(str, enum.Enum):
    body = 'body'
    path = 'path'
    query = 'query'
    header = 'header'
    formData = 'formData'


# https://spec.openapis.org/oas/v2.0.html#parameter-object
class Parameter(base_model.BaseModel):
    name: str
    in_: In = pydantic.Field(alias='in', strict=False)
    description: str = ''
    required: bool = False

    # in: body
    schema_: Any = pydantic.Field(alias='schema', default=None)

    # in != body
    type: str | None = None
    format: str | None = None
    allowEmptyValue: bool = False
    items: dict | None = None
    collectionFormat: str | None = None
    default: Any = None

    # TODO: validators

    x_cpp_name: str | None = pydantic.Field(
        default=None,
        validation_alias=pydantic.AliasChoices('x-taxi-cpp-name', 'x-usrv-cpp-name'),
    )

    def model_post_init(self, context: Any, /) -> None:
        if self.in_ == In.body:
            if not self.schema_:
                raise ValueError(errors.missing_field_msg('schema'))
        else:
            if not self.type:
                raise ValueError(errors.missing_field_msg('type'))
            if self.type == 'array':
                if not self.items:
                    raise ValueError(errors.missing_field_msg('items'))


# https://spec.openapis.org/oas/v2.0.html#header-object
class Header(base_model.BaseModel):
    description: str | None = None
    type: str
    format: str | None = None
    items: dict | None = None
    collectionFormat: str | None = None
    default: Any = None

    # TODO: validators

    def model_post_init(self, context: Any, /) -> None:
        if not self.type:
            raise ValueError(errors.missing_field_msg('type'))
        if self.type == 'array':
            if not self.items:
                raise ValueError(errors.missing_field_msg('items'))


Schema = Any


# https://spec.openapis.org/oas/v2.0.html#response-object
class Response(base_model.BaseModel):
    description: str
    schema_: Schema = pydantic.Field(alias='schema', default=None)
    headers: dict[str, Header] = pydantic.Field(default_factory=dict)
    examples: dict[str, Any] = pydantic.Field(default_factory=dict)


Responses = dict[str | int, Response | Ref]


class SecurityType(str, enum.Enum):
    basic = 'basic'
    apiKey = 'apiKey'
    oauth2 = 'oauth2'


class SecurityIn(str, enum.Enum):
    query = 'query'
    header = 'header'


class OAuthFlow(str, enum.Enum):
    implicit = 'implicit'
    password = 'password'
    application = 'application'
    accessCode = 'accessCode'


# https://spec.openapis.org/oas/v2.0.html#security-definitions-object
class SecurityDef(base_model.BaseModel):
    type: SecurityType = pydantic.Field(strict=False)
    description: str | None = None
    name: str | None = None
    in_: SecurityIn | None = pydantic.Field(alias='in', default=None, strict=False)
    flow: OAuthFlow | None = pydantic.Field(default=None, strict=False)
    authorizationUrl: str | None = None
    tokenUrl: str | None = None
    scopes: dict[str, str] = pydantic.Field(default_factory=dict)

    def model_post_init(self, context: Any, /) -> None:
        match self.type:
            case SecurityType.apiKey:
                if not self.name:
                    raise ValueError(errors.missing_field_msg('name'))
                if not self.in_:
                    raise ValueError(errors.missing_field_msg('in'))
            case SecurityType.oauth2:
                match self.flow:
                    case None:
                        raise ValueError(errors.missing_field_msg('flow'))
                    case OAuthFlow.implicit:
                        if not self.authorizationUrl:
                            raise ValueError(errors.missing_field_msg('authorizationUrl'))
                    case OAuthFlow.password:
                        if not self.tokenUrl:
                            raise ValueError(errors.missing_field_msg('tokenUrl'))
                    case OAuthFlow.application:
                        if not self.tokenUrl:
                            raise ValueError(errors.missing_field_msg('tokenUrl'))
                    case OAuthFlow.accessCode:
                        if not self.tokenUrl:
                            raise ValueError(errors.missing_field_msg('tokenUrl'))
                        if not self.authorizationUrl:
                            raise ValueError(errors.missing_field_msg('authorizationUrl'))


# https://spec.openapis.org/oas/v2.0.html#security-requirement-object
Security = dict[str, list[str]]

Parameters = list[Parameter | Ref]


# https://spec.openapis.org/oas/v2.0.html#operation-object
class Operation(base_model.BaseModel):
    tags: list[str] | None = None
    summary: str | None = None
    description: str = ''
    externalDocs: dict | None = None
    operationId: str | None = None
    consumes: list[str] = pydantic.Field(default_factory=list)
    produces: list[str] = pydantic.Field(default_factory=list)
    parameters: Parameters = pydantic.Field(default_factory=list)
    responses: Responses
    schemes: list[str] = pydantic.Field(default_factory=list)
    deprecated: bool = False
    security: Security | None = None

    x_taxi_middlewares: base_model.XMiddlewares | None = pydantic.Field(
        default=None,
        validation_alias=pydantic.AliasChoices('x-taxi-middlewares', 'x-usrv-middlewares'),
    )
    x_taxi_handler_codegen: bool = pydantic.Field(
        default=True,
        validation_alias=pydantic.AliasChoices('x-taxi-handler-codegen', 'x-usrv-handler-codegen'),
    )
    x_client_codegen: bool = pydantic.Field(
        default=True,
        validation_alias=pydantic.AliasChoices('x-taxi-client-codegen', 'x-usrv-client-codegen'),
    )


# https://spec.openapis.org/oas/v2.0.html#paths-object
class Path(base_model.BaseModel):
    get: Operation | None = None
    post: Operation | None = None
    put: Operation | None = None
    delete: Operation | None = None
    options: Operation | None = None
    head: Operation | None = None
    patch: Operation | None = None
    parameters: Parameters = pydantic.Field(default_factory=list)


Paths = dict[str, Path]


# https://spec.openapis.org/oas/v2.0.html#schema
class Swagger(base_model.BaseModel):
    swagger: str = '2.0'
    info: Info | None = None
    host: str | None = None
    basePath: str = ''
    schemes: list[str] = pydantic.Field(default_factory=list)
    consumes: list[str] = pydantic.Field(default_factory=list)
    produces: list[str] = pydantic.Field(default_factory=list)
    paths: Paths = pydantic.Field(default_factory=dict)
    definitions: dict[str, Schema] = pydantic.Field(default_factory=dict)
    parameters: dict[str, Parameter] = pydantic.Field(default_factory=dict)
    responses: dict[str, Response] = pydantic.Field(default_factory=dict)
    securityDefinitions: dict[str, SecurityDef] = pydantic.Field(default_factory=dict)
    security: Security = pydantic.Field(default_factory=dict)

    def validate_security(self, security: Security | None) -> None:
        if not security:
            return

        for name, values in security.items():
            if name not in self.securityDefinitions.keys():
                raise ValueError(f'Undefined security name="{name}". Expected on of: {self.securityDefinitions.keys()}')
            if self.securityDefinitions[name].type != SecurityType.oauth2:
                if len(values) != 0:
                    raise ValueError(f'For security "{name}" the array must be empty')

    def model_post_init(self, context: Any, /) -> None:
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

    @staticmethod
    def schema_type() -> str:
        return 'swagger 2.0'
