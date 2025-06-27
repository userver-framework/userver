import enum
from typing import Any
from typing import Dict
from typing import List
from typing import Optional
from typing import Union

import pydantic

from . import base_model
from . import errors


class Info(base_model.BaseModel):
    description: Optional[str] = None
    title: Optional[str] = None
    version: Optional[str] = None


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
    in_: In = pydantic.Field(alias='in')
    description: str = ''
    required: bool = False

    # in: body
    schema_: Any = pydantic.Field(alias='schema', default=None)

    # in != body
    type: Optional[str] = None
    format: Optional[str] = None
    allowEmptyValue: bool = False
    items: Optional[Dict] = None
    collectionFormat: Optional[str] = None
    default: Any = None

    # TODO: validators

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
    description: Optional[str] = None
    type: str
    format: Optional[str] = None
    items: Optional[Dict] = None
    collectionFormat: Optional[str] = None
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
    headers: Dict[str, Header] = pydantic.Field(default_factory=dict)
    examples: Dict[str, Any] = pydantic.Field(default_factory=dict)


Responses = Dict[Union[str, int], Union[Response, Ref]]


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
    type: SecurityType
    description: Optional[str] = None
    name: Optional[str] = None
    in_: Optional[SecurityIn] = pydantic.Field(alias='in', default=None)
    flow: Optional[OAuthFlow] = None
    authorizationUrl: Optional[str] = None
    tokenUrl: Optional[str] = None
    scopes: Dict[str, str] = pydantic.Field(default_factory=dict)

    def model_post_init(self, context: Any, /) -> None:
        if self.type == SecurityType.apiKey:
            if not self.name:
                raise ValueError(errors.missing_field_msg('name'))
            if not self.in_:
                raise ValueError(errors.missing_field_msg('in'))
        elif self.type == SecurityType.oauth2:
            if not self.flow:
                raise ValueError(errors.missing_field_msg('flow'))
            if self.flow == OAuthFlow.implicit:
                if not self.authorizationUrl:
                    raise ValueError(errors.missing_field_msg('authorizationUrl'))
            elif self.flow == OAuthFlow.password:
                if not self.tokenUrl:
                    raise ValueError(errors.missing_field_msg('tokenUrl'))
            elif self.flow == OAuthFlow.application:
                if not self.tokenUrl:
                    raise ValueError(errors.missing_field_msg('tokenUrl'))
            elif self.flow == OAuthFlow.accessCode:
                if not self.tokenUrl:
                    raise ValueError(errors.missing_field_msg('tokenUrl'))
                if not self.authorizationUrl:
                    raise ValueError(errors.missing_field_msg('authorizationUrl'))


# https://spec.openapis.org/oas/v2.0.html#security-requirement-object
Security = Dict[str, List[str]]

Parameters = List[Union[Parameter, Ref]]


# https://spec.openapis.org/oas/v2.0.html#operation-object
class Operation(base_model.BaseModel):
    tags: Optional[List[str]] = None
    summary: Optional[str] = None
    description: str = ''
    externalDocs: Optional[Dict] = None
    operationId: Optional[str] = None
    consumes: List[str] = pydantic.Field(default_factory=list)
    produces: List[str] = pydantic.Field(default_factory=list)
    parameters: Parameters = pydantic.Field(default_factory=list)
    responses: Responses
    schemes: List[str] = pydantic.Field(default_factory=list)
    deprecated: bool = False
    security: Optional[Security] = None


# https://spec.openapis.org/oas/v2.0.html#paths-object
class Path(base_model.BaseModel):
    get: Optional[Operation] = None
    post: Optional[Operation] = None
    put: Optional[Operation] = None
    delete: Optional[Operation] = None
    options: Optional[Operation] = None
    head: Optional[Operation] = None
    patch: Optional[Operation] = None
    parameters: Parameters = pydantic.Field(default_factory=list)


Paths = Dict[str, Path]


# https://spec.openapis.org/oas/v2.0.html#schema
class Swagger(base_model.BaseModel):
    swagger: str = '2.0'
    info: Optional[Info] = None
    host: Optional[str] = None
    basePath: str = ''
    schemes: List[str] = pydantic.Field(default_factory=list)
    consumes: List[str] = pydantic.Field(default_factory=list)
    produces: List[str] = pydantic.Field(default_factory=list)
    paths: Paths = pydantic.Field(default_factory=dict)
    definitions: Dict[str, Schema] = pydantic.Field(default_factory=dict)
    parameters: Dict[str, Parameter] = pydantic.Field(default_factory=dict)
    responses: Dict[str, Response] = pydantic.Field(default_factory=dict)
    securityDefinitions: Dict[str, SecurityDef] = pydantic.Field(default_factory=dict)
    security: Security = pydantic.Field(default_factory=dict)

    def validate_security(self, security: Optional[Security]) -> None:
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
