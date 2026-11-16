import dataclasses
import os

from chaotic import layout
from chaotic.back.cpp import types as cpp_types
from chaotic_openapi.back.cpp.common import types as common_types
from chaotic_openapi.back.cpp.common.types import Operation


@dataclasses.dataclass
class ServerSpec(common_types.SpecBase):
    """Service-level spec: one per service, holds all translated operations."""

    service_name: str
    cpp_namespace: str
    include_dir: str = ''
    closure: layout.Closure | None = None
    operations: list[Operation] = dataclasses.field(default_factory=list)

    # Internal types which cannot be referred to
    internal_schemas: dict[str, cpp_types.CppType] = dataclasses.field(default_factory=dict)
    # Types which can be referred to
    schemas: dict[str, cpp_types.CppType] = dataclasses.field(default_factory=dict)

    def __post_init__(self) -> None:
        if not self.include_dir and not self.closure:
            self.include_dir = f'handlers/{self.service_name}'

    def cpp_type_includes(self) -> list[str]:
        if self.closure:
            return self.tagged_schema_includes()
        includes = []
        for cpp_type in self.extract_cpp_types().values():
            assert cpp_type.json_schema
            filepath = cpp_type.json_schema.source_location().filepath
            includes.append(
                'handlers/{}/{}.hpp'.format(self.service_name, os.path.basename(filepath).rsplit('.', 1)[0]),
            )
        return includes
