import dataclasses
import enum
from typing import Any
from typing import Optional
from typing import Union

from chaotic.front import types


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

    x_cpp_name: Optional[str]


@dataclasses.dataclass
class MediaType:
    schema: Union[types.Schema, types.Ref]
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
    schema: Union[types.Ref, types.Schema]


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
    bearerFormat: Optional[str] = None


@dataclasses.dataclass
class OpenIdConnectSecurity(Security):
    openIdConnectUrl: str


@dataclasses.dataclass
class Operation:
    description: str
    path: str
    method: str
    operationId: Union[str, None]
    parameters: list[Parameter]
    requestBody: Union[list[RequestBody], Ref]
    responses: dict[int, Union[Response, Ref]]
    security: list[Security]


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
