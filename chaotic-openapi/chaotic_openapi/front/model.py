from collections.abc import Callable
import dataclasses
import enum
from typing import Any

from chaotic.front import types
from chaotic_openapi.front import base_model


class In(str, enum.Enum):
    path = 'path'
    query = 'query'
    queryExplode = 'queryExplode'
    header = 'header'
    cookie = 'cookie'


class Style(str, enum.Enum):
    matrix = 'matrix'
    label = 'label'
    form = 'form'
    simple = 'simple'
    spaceDelimited = 'spaceDelimited'
    pipeDelimited = 'pipeDelimited'
    deepObject = 'deepObject'


@dataclasses.dataclass
class Parameter:
    name: str
    in_: In
    description: str
    examples: dict[str, Any]

    deprecated: bool
    required: bool
    allowEmptyValue: bool

    style: Style
    schema: types.Schema

    x_cpp_name: str | None
    x_query_log_mode_hide: bool


@dataclasses.dataclass
class MediaType:
    schema: types.Schema | types.Ref | None
    examples: dict[str, Any]


@dataclasses.dataclass
class Response:
    description: str
    headers: dict[str, Parameter]
    content: dict[str, MediaType]


@dataclasses.dataclass
class RequestBody:
    content_type: str
    required: bool
    schema: types.Ref | types.Schema


@dataclasses.dataclass
class Ref:
    ref: str


class SecurityIn(str, enum.Enum):
    query = 'query'
    header = 'header'
    cookie = 'cookie'


@dataclasses.dataclass
class Security:
    description: str


@dataclasses.dataclass
class ApiKeySecurity(Security):
    name: str
    in_: SecurityIn


@dataclasses.dataclass
class Flow:
    refreshUrl: str
    scopes: dict[str, str]


@dataclasses.dataclass
class ImplicitFlow(Flow):
    authorizationUrl: str


@dataclasses.dataclass
class PasswordFlow(Flow):
    tokenUrl: str


@dataclasses.dataclass
class ClientCredFlow(Flow):
    tokenUrl: str


@dataclasses.dataclass
class AuthCodeFlow(Flow):
    authorizationUrl: str
    tokenUrl: str


@dataclasses.dataclass
class OAuthSecurity(Security):
    flows: list[Flow]


@dataclasses.dataclass
class HttpSecurity(Security):
    scheme: str
    bearerFormat: str | None = None


@dataclasses.dataclass
class OpenIdConnectSecurity(Security):
    openIdConnectUrl: str


@dataclasses.dataclass
class Operation:
    description: str
    path: str
    method: str
    operationId: str | None
    parameters: list[Parameter]
    requestBody: list[RequestBody] | Ref
    responses: dict[int, Response | Ref]
    security: list[Security]

    x_client_codegen: bool
    x_middlewares: base_model.XMiddlewares


@dataclasses.dataclass
class Service:
    name: str
    description: str = ''
    operations: list[Operation] = dataclasses.field(default_factory=list)

    schemas: dict[str, types.Schema] = dataclasses.field(default_factory=dict)
    responses: dict[str, Response] = dataclasses.field(default_factory=dict)
    parameters: dict[str, Parameter] = dataclasses.field(default_factory=dict)
    headers: dict[str, Parameter] = dataclasses.field(default_factory=dict)
    requestBodies: dict[str, list[RequestBody]] = dataclasses.field(default_factory=dict)
    security: dict[str, Security] = dataclasses.field(default_factory=dict)

    # For tests only
    def visit_all_schemas(
        self,
        callback: Callable[[types.Schema], None],
    ) -> None:
        def callback2(child: types.Schema, _: types.Schema) -> None:
            callback(child)

        def clear_response(response):
            for body2 in response.content.values():
                callback(body2.schema)
                body2.schema.visit_children(callback2)
            for header in response.headers.values():
                callback(header.schema)
                header.schema.visit_children(callback2)

        for schema in self.schemas.values():
            callback(schema)
            schema.visit_children(callback2)
        for body in self.requestBodies.values():
            for b in body:
                callback(b.schema)
                b.schema.visit_children(callback2)
        for response in self.responses.values():
            clear_response(response)
        for parameter in self.parameters.values():
            callback(parameter.schema)
            parameter.schema.visit_children(callback2)

        for operation in self.operations:
            if not isinstance(operation.requestBody, Ref):
                for body3 in operation.requestBody:
                    callback(body3.schema)
                    body3.schema.visit_children(callback2)
            for response2 in operation.responses.values():
                if isinstance(response2, Ref):
                    continue
                clear_response(response2)
            for parameter in operation.parameters:
                callback(parameter.schema)
                parameter.schema.visit_children(callback2)
