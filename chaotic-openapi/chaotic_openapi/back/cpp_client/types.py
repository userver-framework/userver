import dataclasses
from typing import List
from typing import Optional

from chaotic import cpp_names
from chaotic.back.cpp import types as cpp_types


@dataclasses.dataclass
class Parameter:
    raw_name: str
    cpp_name: str
    cpp_type: str
    parser: str


@dataclasses.dataclass
class RequestBody:
    content_type: str
    schema: Optional[cpp_types.CppType]


@dataclasses.dataclass
class Response:
    status: int

    def is_error(self) -> bool:
        return self.status >= 400


@dataclasses.dataclass
class Operation:
    method: str
    path: str

    description: str
    parameters: List[Parameter]
    request_bodies: List[RequestBody]
    responses: List[Response]

    client_generate: bool = True

    def cpp_namespace(self) -> str:
        return cpp_names.namespace(self.path + '_' + self.method)

    def cpp_method_name(self) -> str:
        return cpp_names.camel_case(
            cpp_names.namespace(self.path + '_' + self.method),
        )

    def empty_request(self) -> bool:
        if self.parameters:
            return False
        for body in self.request_bodies:
            if body.schema:
                return False
        return True


@dataclasses.dataclass
class ClientSpec:
    client_name: str
    cpp_namespace: str
    operations: List[Operation]
