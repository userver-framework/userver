import enum
from typing import Any
from typing import Dict
from typing import List
from typing import Optional

import pydantic


class Info(pydantic.BaseModel):
    pass


class In(str, enum.Enum):
    body = 'body'
    path = 'path'
    query = 'query'
    header = 'header'
    form = 'form'


class Parameter(pydantic.BaseModel):
    name: Optional[str] = None
    in_: In = pydantic.Field(alias='in')
    description: str = ''
    required: bool = False

    # in: body
    schema_: Any = pydantic.Field(alias='schema', default=None)

    # in != body
    type: Optional[str] = None
    format: Optional[str] = None
    allowEmptyValue: Optional[str] = None
    items: Optional[Dict] = None
    collectionFormat: Optional[str] = None
    default: Any = None

    # TODO: validators

    def model_post_init(self, context: Any, /) -> None:
        if self.in_ == In.body:
            assert self.schema_
        else:
            assert self.type
            if self.type == 'array':
                assert self.items


Parameters = List[Parameter]


class Header(pydantic.BaseModel):
    description: Optional[str] = None
    type: str
    format: Optional[str] = None
    items: Optional[Dict] = None
    collectionFormat: Optional[str] = None
    default: Any = None

    # TODO: validators


Headers = Dict[str, Header]

Schema = Any


class Response(pydantic.BaseModel):
    description: str
    schema_: Schema = pydantic.Field(alias='schema', default=None)
    headers: Optional[Headers] = None
    examples: Any = None


Responses = Dict[str, Response]


class Security(pydantic.BaseModel):
    def model_post_init(self, context: Any, /) -> None:
        raise BaseException('Security is not supported')


class Operation(pydantic.BaseModel):
    tags: Optional[str] = None
    summary: Optional[str] = None
    description: str = ''
    externalDocs: Optional[Dict] = None
    operationId: Optional[str] = None
    consumes: List[str] = []
    produces: List[str] = []
    parameters: Parameters = []
    responses: Responses
    schemes: List[str] = []
    deprecated: bool = False
    security: Optional[Security] = None


class Path(pydantic.BaseModel):
    get: Optional[Operation] = None
    post: Optional[Operation] = None
    put: Optional[Operation] = None
    delete: Optional[Operation] = None
    options: Optional[Operation] = None
    head: Optional[Operation] = None
    patch: Optional[Operation] = None
    parameters: Parameters = []


Paths = Dict[str, Path]


class Swagger(pydantic.BaseModel):
    swagger: str
    info: Info
    host: Optional[str] = None
    basePath: str = ''
    schemes: List[str] = []
    consumes: List[str] = []
    produces: List[str] = []
    paths: Paths
    definitions: Dict[str, Schema] = {}
    parameters: Parameters = []
    responses: Dict[str, Response] = {}

    @staticmethod
    def schema_type() -> str:
        return 'swagger 2.0'
