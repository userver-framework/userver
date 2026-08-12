import dataclasses

from chaotic.back.cpp import types as cpp_types
from chaotic_openapi.back.cpp.common import types as common_types
from chaotic_openapi.back.cpp.common.types import Body
from chaotic_openapi.back.cpp.common.types import map_method
from chaotic_openapi.back.cpp.common.types import Operation
from chaotic_openapi.back.cpp.common.types import Parameter
from chaotic_openapi.back.cpp.common.types import Response

__all__ = ['Body', 'map_method', 'Operation', 'Parameter', 'Response']


@dataclasses.dataclass
class ClientSpec(common_types.SpecBase):
    client_name: str
    cpp_namespace: str
    dynamic_config: str
    description: str = ''
    operations: list[Operation] = dataclasses.field(default_factory=list)

    # Internal types which cannot be referred to
    internal_schemas: dict[str, cpp_types.CppType] = dataclasses.field(default_factory=dict)
    # Types which can be referred to
    schemas: dict[str, cpp_types.CppType] = dataclasses.field(default_factory=dict)

    def has_multiple_content_type_request(self) -> bool:
        for op in self.operations:
            if len(op.request_bodies) > 1:
                return True
        return False

    def has_array_in_request_body(self) -> bool:
        for op in self.operations:
            for body in op.request_bodies:
                if isinstance(body.schema, cpp_types.CppArray):
                    return True
        return False

    def requests_declaration_includes(self) -> list[str]:
        includes: set[str] = set()
        for op in self.operations:
            if not op.client_generate:
                continue
            includes.update(op.requests_declaration_includes())
            for mw in op.middlewares:
                includes.update(mw.requests_hpp_includes)
        return sorted(includes)

    def responses_declaration_includes(self) -> list[str]:
        includes: set[str] = set()
        for op in self.operations:
            if not op.client_generate:
                continue
            includes.update(op.responses_declaration_includes())
        return sorted(includes)

    def responses_definitions_includes(self) -> list[str]:
        includes: set[str] = set()
        for op in self.operations:
            if not op.client_generate:
                continue
            includes.update(op.response_body_definition_includes())
        return sorted(includes)

    def cpp_includes(self) -> list[str]:
        includes = []
        for cpp_type in self.extract_cpp_types().values():
            assert cpp_type.json_schema
            filepath = cpp_type.json_schema.source_location().filepath
            includes.append(
                'clients/{}/{}.hpp'.format(self.client_name, filepath.rsplit('.', 1)[0]),
            )
        return includes
