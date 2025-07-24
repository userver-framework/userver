import dataclasses
from typing import Dict
from typing import Generator
from typing import List
from typing import Optional
from typing import Set

from chaotic import cpp_names
from chaotic import error
from chaotic.back.cpp import types as cpp_types


@dataclasses.dataclass
class Parameter:
    description: str
    raw_name: str
    cpp_name: str
    cpp_type: cpp_types.CppType
    parser: str
    required: bool

    def declaration_includes(self) -> List[str]:
        # TODO
        if not self.required:
            return ['optional']
        return []

    @classmethod
    def _validate_schema(cls, schema: cpp_types.CppType, *, is_array_allowed: bool) -> None:
        assert schema.json_schema
        if not is_array_allowed and isinstance(schema, cpp_types.CppArray):
            source_location = schema.json_schema.source_location()
            raise error.BaseError(
                full_filepath=source_location.filepath,
                infile_path=source_location.location,
                schema_type='jsonschema',
                msg='Too deep array for parameter schema',
            )

        if not isinstance(
            schema,
            (
                cpp_types.CppPrimitiveType,
                cpp_types.CppStringWithFormat,
                cpp_types.CppRef,
                cpp_types.CppArray,
                cpp_types.CppIntEnum,
                cpp_types.CppStringEnum,
            ),
        ):
            source_location = schema.json_schema.source_location()
            raise error.BaseError(
                full_filepath=source_location.filepath,
                infile_path=source_location.location,
                schema_type='jsonschema',
                msg='Unsupported parameter type',
            )

        if isinstance(schema, cpp_types.CppRef):
            cls._validate_schema(schema.orig_cpp_type, is_array_allowed=is_array_allowed)
        if isinstance(schema, cpp_types.CppArray):
            cls._validate_schema(schema.items, is_array_allowed=False)

    def __post_init__(self) -> None:
        self._validate_schema(self.cpp_type, is_array_allowed=True)


@dataclasses.dataclass
class Body:
    content_type: str
    schema: Optional[cpp_types.CppType]

    def cpp_name(self) -> str:
        return 'RequestBody' + cpp_names.camel_case(
            cpp_names.cpp_identifier(self.content_type),
        )


@dataclasses.dataclass
class Response:
    status: int
    body: Dict[str, cpp_types.CppType]
    headers: List[Parameter] = dataclasses.field(default_factory=list)

    def is_error(self) -> bool:
        return self.status >= 400

    def is_single_contenttype(self) -> bool:
        return len(self.body) == 1

    def single_body(self) -> cpp_types.CppType:
        return list(self.body.values())[0]

    def body_cpp_name(self, content_type: str) -> str:
        return f'Response{self.status}' + cpp_names.camel_case(
            cpp_names.cpp_identifier(content_type),
        )


def map_method(method: str) -> str:
    method = method.lower()
    if method == 'delete':
        return 'delete_'
    return method


@dataclasses.dataclass
class Operation:
    method: str
    path: str

    description: str = ''
    parameters: List[Parameter] = dataclasses.field(default_factory=list)
    request_bodies: List[Body] = dataclasses.field(default_factory=list)
    responses: List[Response] = dataclasses.field(default_factory=list)

    client_generate: bool = True

    def cpp_namespace(self) -> str:
        return cpp_names.namespace(self.path) + '::' + map_method(self.method)

    def cpp_method_name(self) -> str:
        return cpp_names.camel_case(
            cpp_names.namespace(self.path) + '_' + map_method(self.method),
        )

    def empty_request(self) -> bool:
        if self.parameters:
            return False
        for body in self.request_bodies:
            if body.schema:
                return False
        return True

    def has_single_2xx_responses(self) -> bool:
        return len(list(self.iter_2xx_responses())) == 1

    def has_2xx_responses(self) -> bool:
        return len(list(self.iter_2xx_responses())) > 0

    def iter_2xx_responses(self) -> Generator[Response, None, None]:
        for response in self.responses:
            if not response.is_error():
                yield response

    def iter_error_responses(self) -> Generator[Response, None, None]:
        for response in self.responses:
            if response.is_error():
                yield response


@dataclasses.dataclass
class ClientSpec:
    client_name: str
    cpp_namespace: str
    dynamic_config: str
    description: str = ''
    operations: List[Operation] = dataclasses.field(default_factory=list)

    # Internal types which cannot be referred to
    internal_schemas: Dict[str, cpp_types.CppType] = dataclasses.field(default_factory=dict)
    # Types which can be referred to
    schemas: Dict[str, cpp_types.CppType] = dataclasses.field(default_factory=dict)

    def has_multiple_content_type_request(self) -> bool:
        for op in self.operations:
            if len(op.request_bodies) > 1:
                return True
        return False

    def requests_declaration_includes(self) -> List[str]:
        includes: Set[str] = set()
        for op in self.operations:
            if not op.client_generate:
                continue

            for body in op.request_bodies:
                if body.schema:
                    includes.update(body.schema.declaration_includes())
            for param in op.parameters:
                includes.update(param.declaration_includes())

        return sorted(includes)

    def responses_declaration_includes(self) -> List[str]:
        includes: Set[str] = set()
        for op in self.operations:
            if not op.client_generate:
                continue

            for response in op.responses:
                for _, body in response.body.items():
                    includes.update(body.declaration_includes())
                for header in response.headers:
                    includes.update(header.declaration_includes())
        return sorted(includes)

    def responses_definitions_includes(self) -> List[str]:
        includes: Set[str] = set()
        for op in self.operations:
            if not op.client_generate:
                continue

            for response in op.responses:
                for _, body in response.body.items():
                    includes.update(body.definition_includes())
        return sorted(includes)

    def cpp_includes(self) -> List[str]:
        includes = []
        for cpp_type in self.extract_cpp_types().values():
            assert cpp_type.json_schema
            filepath = cpp_type.json_schema.source_location().filepath
            includes.append(
                'clients/{}/{}.hpp'.format(
                    self.client_name,
                    filepath.split('/')[-1].split('.')[0],
                ),
            )
        return includes

    def extract_cpp_types(self) -> Dict[str, cpp_types.CppType]:
        types = self.schemas.copy()
        types.update(self.internal_schemas)

        return types
