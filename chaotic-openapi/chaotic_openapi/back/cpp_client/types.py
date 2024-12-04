import dataclasses
from typing import List
from typing import Optional

from chaotic.back.cpp import types as cpp_types


@dataclasses.dataclass
class Parameter:
    raw_name: str
    cpp_name: str
    parser: str


@dataclasses.dataclass
class RequestBody:
    content_type: str
    schema: Optional[cpp_types.CppType]


@dataclasses.dataclass
class Operation:
    method: str
    path: str

    description: str
    parameters: List[Parameter]
    request_bodies: List[RequestBody]

    def cpp_namespace(self) -> str:
        # TODO
        return self.method + '_' + self.path.replace('/', '_')

    def cpp_method_name(self) -> str:
        # TODO
        return self.method + self.path.replace('/', '_')


@dataclasses.dataclass
class ClientSpec:
    client_name: str
    cpp_namespace: str
    operations: List[Operation]
