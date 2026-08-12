import dataclasses

from chaotic.back.cpp import types as cpp_types
from chaotic_openapi.back.cpp.common import types as common_types
from chaotic_openapi.back.cpp.common.types import Operation


@dataclasses.dataclass
class ServerSpec(common_types.SpecBase):
    """Service-level spec: one per service, holds all translated operations."""

    service_name: str
    cpp_namespace: str
    operations: list[Operation] = dataclasses.field(default_factory=list)

    # Internal types which cannot be referred to
    internal_schemas: dict[str, cpp_types.CppType] = dataclasses.field(default_factory=dict)
    # Types which can be referred to
    schemas: dict[str, cpp_types.CppType] = dataclasses.field(default_factory=dict)

    def cpp_type_includes(self) -> list[str]:
        includes = []
        for cpp_type in self.extract_cpp_types().values():
            assert cpp_type.json_schema
            filepath = cpp_type.json_schema.source_location().filepath
            includes.append(
                'handlers/{}/{}.hpp'.format(self.service_name, filepath.rsplit('.', 1)[0]),
            )
        return includes
