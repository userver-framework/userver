import enum
from typing import Any
from typing import Dict
from typing import List
from typing import Optional
from typing import Union

import pydantic


class Info(pydantic.BaseModel):
    title: str
    description: Optional[str] = None
    termsOfService: Optional[str] = None
    contact: Any = None
    license: Any = None
    version: str


class Server(pydantic.BaseModel):
    url: str
    description: Optional[str] = None
    variables: Dict[str, Any] = {}


Schema = Any


class Style(str, enum.Enum):
    matrix = 'matrix'
    label = 'label'
    form = 'form'
    simple = 'simple'
    spaceDelimited = 'spaceDelimited'
    pipeDelimited = 'pipeDelimited'
    # TODO: deepObject


class Header(pydantic.BaseModel):
    description: Optional[str] = None
    required: bool = False
    deprecated: bool = False
    allowEmptyValue: bool = False

    style: Optional[Style] = None
    explode: Optional[bool] = None
    allowReserved: bool = False
    schema_: Schema = pydantic.Field(alias='schema')
    example: Any = None
    examples: Dict[str, Any] = {}


class MediaType(pydantic.BaseModel):
    schema_: Schema = pydantic.Field(alias='schema', default=None)
    example: Any = None
    examples: Dict[str, Any] = {}
    # encoding: Dict[str, Encoding] = {}


class Ref(pydantic.BaseModel):
    ref: str = pydantic.Field(alias='$ref')


class Response(pydantic.BaseModel):
    description: str
    headers: Dict[str, Union[Header, Ref]] = {}
    content: Dict[str, MediaType] = {}
    # TODO: links


class In(str, enum.Enum):
    path = 'path'
    query = 'query'
    header = 'header'
    cookie = 'cookie'


class Parameter(pydantic.BaseModel):
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
    examples: Dict[str, Any] = {}

    # content: Dict[str, MediaType] = {}

    def model_post_init(self, context: Any, /) -> None:
        if self.style:
            return

        self.style = {
            In.query: Style.form,
            In.path: Style.simple,
            In.header: Style.simple,
            In.cookie: Style.form,
        }[self.in_]


class RequestBody(pydantic.BaseModel):
    description: Optional[str] = None
    content: Dict[str, MediaType]
    required: bool = False


class Components(pydantic.BaseModel):
    schemas: Dict[str, Schema] = {}
    responses: Dict[str, Response] = {}
    parameters: Dict[str, Parameter] = {}
    headers: Dict[str, Header] = {}
    requestBodies: Dict[str, RequestBody] = {}


class SecurityType(str, enum.Enum):
    apiKey = 'apiKey'
    http = 'http'
    oauth2 = 'oauth2'
    openIdConnect = 'openIdConnect'


class SecurityIn(str, enum.Enum):
    query = 'query'
    header = 'header'
    cookie = 'cookie'


class Security(pydantic.BaseModel):
    type: SecurityType
    description: Optional[str] = None
    name: Optional[str] = None
    in_: Optional[SecurityIn] = None
    scheme_: Optional[str] = pydantic.Field(alias='scheme', default=None)
    bearerFormat: Optional[str] = None
    # TODO: flows: Optional[str] = None
    openIdConnectUrl: Optional[str] = None

    def model_post_init(self, context: Any, /) -> None:
        if self.type == SecurityType.apiKey:
            assert self.name
            assert self.in_
        elif self.type == SecurityType.http:
            assert self.scheme_
        elif self.type == SecurityType.openIdConnect:
            assert self.openIdConnectUrl


class Operation(pydantic.BaseModel):
    tags: List[str] = []
    summary: Optional[str] = None
    description: str = ''
    externalDocs: Any = None

    operationId: Optional[str] = None
    parameters: List[Union[Parameter, Ref]] = []
    requestBody: Optional[Union[RequestBody, Ref]] = None
    responses: Dict[Union[str, int], Union[Response, Ref]]
    deprecated: bool = False
    security: Optional[Security] = None
    servers: List[Server] = []


class Path(pydantic.BaseModel):
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

    servers: List[Server] = []
    parameters: List[Parameter] = []


class OpenApi(pydantic.BaseModel):
    openapi: str = '3.0.0'
    info: Optional[Info] = None
    servers: List[Server] = []
    paths: Dict[str, Path] = {}
    components: Components = Components()
    security: Optional[Security] = None
    tags: List[Any] = []
    externalDocs: Any = None

    @staticmethod
    def schema_type() -> str:
        return 'openapi'
