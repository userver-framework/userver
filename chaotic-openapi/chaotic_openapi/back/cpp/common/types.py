from collections.abc import Generator
import dataclasses

from chaotic import cpp_names
from chaotic import error
from chaotic.back.cpp import types as cpp_types
from chaotic_openapi.back.cpp.client import middleware


class SpecBase:
    schemas: dict[str, cpp_types.CppType]
    internal_schemas: dict[str, cpp_types.CppType]

    def extract_cpp_types(self) -> dict[str, cpp_types.CppType]:
        types = self.schemas.copy()
        types.update(self.internal_schemas)
        return types


@dataclasses.dataclass
class Parameter:
    description: str
    raw_name: str
    cpp_name: str
    cpp_type: cpp_types.CppType
    parser: str
    required: bool

    query_log_mode_hide: bool

    def declaration_includes(self) -> list[str]:
        # TODO
        if not self.required:
            return ['optional']
        return []

    def _validate_schema(self, schema: cpp_types.CppType, *, is_array_allowed: bool) -> None:
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
                msg=f'Unsupported parameter type for parameter "{self.raw_name}"',
            )

        if isinstance(schema, cpp_types.CppRef):
            self._validate_schema(schema.orig_cpp_type, is_array_allowed=is_array_allowed)
        if isinstance(schema, cpp_types.CppArray):
            self._validate_schema(schema.items, is_array_allowed=False)

    def __post_init__(self) -> None:
        self._validate_schema(self.cpp_type, is_array_allowed=True)

    # TODO: for handler only
    def span_value(self) -> str:
        if self.required:
            arg = self.cpp_name
        else:
            arg = '*' + self.cpp_name
        if self.cpp_type.cpp_user_name() == 'std::string':
            return arg
        else:
            return f'::fmt::format("{{}}", {arg})'


@dataclasses.dataclass
class Body:
    content_type: str
    schema: cpp_types.CppType | None

    def cpp_name(self) -> str:
        return 'RequestBody' + cpp_names.camel_case(
            cpp_names.cpp_identifier(self.content_type),
        )

    def cpp_type(self) -> str:
        if self.content_type == 'application/octet-stream':
            return self.cpp_name()
        assert self.schema is not None
        return self.schema.cpp_user_name()


@dataclasses.dataclass
class Response:
    status: int
    body: dict[str, cpp_types.CppType]
    headers: list[Parameter] = dataclasses.field(default_factory=list)

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
    operation_id: str | None

    description: str = ''
    parameters: list[Parameter] = dataclasses.field(default_factory=list)
    request_bodies: list[Body] = dataclasses.field(default_factory=list)
    responses: list[Response] = dataclasses.field(default_factory=list)

    client_generate: bool = True
    handler_generate: bool = True

    middlewares: list[middleware.Middleware] = dataclasses.field(default_factory=list)

    def cpp_namespace(self) -> str:
        if self.operation_id:
            return cpp_names.namespace(self.operation_id)
        return cpp_names.namespace(self.path) + '::' + map_method(self.method)

    def cpp_method_name(self) -> str:
        if self.operation_id:
            return self.operation_id
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

    def has_any_hidden_query_parameters(self) -> bool:
        for parameter in self.parameters:
            if parameter.query_log_mode_hide:
                return True
        return False

    def destination_metric_name(self) -> str:
        return self.path.replace('{', '_').replace('}', '_')

    def requests_declaration_includes(self) -> list[str]:
        includes: set[str] = set()
        for param in self.parameters:
            includes.update(param.declaration_includes())
        for body in self.request_bodies:
            if body.schema:
                includes.update(body.schema.declaration_includes())
        return sorted(includes)

    def responses_declaration_includes(self) -> list[str]:
        includes: set[str] = set()
        for response in self.responses:
            for body in response.body.values():
                includes.update(body.declaration_includes())
            for header in response.headers:
                includes.update(header.declaration_includes())
        return sorted(includes)

    def request_body_definition_includes(self) -> list[str]:
        includes: set[str] = set()
        for body in self.request_bodies:
            if body.schema:
                includes.update(body.schema.definition_includes())
        return sorted(includes)

    def response_body_definition_includes(self) -> list[str]:
        includes: set[str] = {'userver/chaotic/sax_parser.hpp'}
        for response in self.responses:
            for body in response.body.values():
                includes.update(body.definition_includes())
        return sorted(includes)
