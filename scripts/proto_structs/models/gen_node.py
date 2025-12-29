"""Models for definitions of C++ entities."""

from __future__ import annotations

import abc
from collections.abc import Iterable
from collections.abc import Sequence
import dataclasses
import pathlib
from typing import Final
from typing import final

from typing_extensions import override

from proto_structs.models import includes
from proto_structs.models import io
from proto_structs.models import names
from proto_structs.models import reserved_identifiers
from proto_structs.models import type_ref
from proto_structs.models import type_ref_consts


class CodegenNode(names.HasCppNameImpl, includes.HasCppIncludes, type_ref.HasTypeDependencies, abc.ABC):
    """A C++ entity definition that should be generated. Introduces the concept of child nodes."""

    @property
    @abc.abstractmethod
    def kind(self) -> str:
        """A short string representing the kind of this node for the jinja template."""
        raise NotImplementedError()

    @property
    def children(self) -> Sequence[CodegenNode]:
        """Returns the child C++ entities, if any. Empty by default, intended to be overridden."""
        return ()

    def iter_all_children(self) -> Iterable[CodegenNode]:
        """Iterate over all childrens, including inners. Pre-order."""
        yield self
        for child in self.children:
            yield from child.iter_all_children()

    @override
    @abc.abstractmethod
    def full_cpp_name_segments(self) -> Sequence[str]:
        raise NotImplementedError()

    @final  # Prevent derived classes from overriding this instead of `own_includes` by mistake.
    @override
    def collect_includes(self) -> Iterable[includes.Include]:
        yield from self.own_includes()
        for child in self.children:
            yield from child.collect_includes()

    def own_includes(self) -> Iterable[includes.Include]:
        """
        Returns includes required by the current node itself, not accounting for `children`.
        Empty by default, intended to be overridden.
        """
        return ()

    @override
    def type_dependencies(self) -> Iterable[type_ref.TypeDependency]:
        for child in self.children:
            yield from child.type_dependencies()


class File(names.HasCppName, includes.HasCppIncludes):
    """A C++ proto structs file scheduled for generation."""

    def __init__(
        self,
        *,
        proto_relative_path: pathlib.Path,
        children: Sequence[CodegenNode],
        services_type_dependencies: Iterable[type_ref.TypeReference],
    ) -> None:
        super().__init__()
        assert not proto_relative_path.is_absolute()
        #: Path to the original proto file relative to its base source directory.
        self.proto_relative_path: Final[pathlib.Path] = proto_relative_path
        #: The top-level C++ entities to define in the file.
        self.children = children
        self._services_type_dependencies = services_type_dependencies

    @override
    def full_cpp_name(self) -> str:
        return f'<file {self.proto_relative_path}>'

    @override
    def contextual_cpp_name(self, *, context: names.HasCppName) -> str:
        return self.full_cpp_name()

    @override
    def full_cpp_name_segments(self) -> Sequence[str]:
        return ()

    @override
    def collect_includes(self) -> Iterable[includes.Include]:
        for dependency in self._services_type_dependencies:
            yield from dependency.collect_includes()
        for child in self.children:
            yield from child.collect_includes()

    def gen_path(self, *, ext: str) -> pathlib.Path:
        """Path to the generated userver proto structs file."""
        return includes.proto_path_to_structs_path(self.proto_relative_path, ext=ext)


class NamespaceNode(CodegenNode):
    """A C++ namespace definition scheduled for generation."""

    def __init__(
        self,
        *,
        parent_name_segments: Iterable[str],
        name_segments: Iterable[str],
        children: Sequence[CodegenNode],
    ) -> None:
        super().__init__()
        self._parent_name_segments = parent_name_segments
        self._name_segments = name_segments
        self._children = children

    @property
    @override
    def kind(self) -> str:
        return 'namespace'

    @property
    @override
    def children(self) -> Sequence[CodegenNode]:
        return self._children

    @override
    def full_cpp_name_segments(self) -> Sequence[str]:
        return *self._parent_name_segments, *self._name_segments

    @property
    def short_name(self) -> str:
        """Name of the namespace itself without parent namespaces."""
        assert all(self._name_segments), 'Empty name segments are not allowed'
        return '::'.join(self._name_segments)

    @classmethod
    def make_for_structs(cls, package: str, children: Sequence[CodegenNode]) -> NamespaceNode:
        return NamespaceNode(
            parent_name_segments=(),
            name_segments=(*names.proto_namespace_to_segments(names.package_to_namespace(package)), 'structs'),
            children=children,
        )

    @classmethod
    def make_for_vanilla(cls, package: str, children: Sequence[CodegenNode]) -> NamespaceNode:
        return NamespaceNode(
            parent_name_segments=(),
            name_segments=names.proto_namespace_to_segments(names.package_to_namespace(package)),
            children=children,
        )


class TypeNode(CodegenNode, abc.ABC):
    """A base class that contains the common logic for any C++ type definition scheduled for generation."""

    def __init__(self, *, name: names.TypeName) -> None:
        super().__init__()
        #: Detailed type name.
        self.name: Final[names.TypeName] = name
        #: Whether a forward declaration should be generated for the type at the top of the containing scope.
        self.should_forward_declare: bool = False

    @override
    def full_cpp_name_segments(self) -> Sequence[str]:
        return self.name.name_segments()


def iter_type_nodes(node: CodegenNode) -> Iterable[TypeNode]:
    """Iterate through all types defined inside, recursively."""
    for child in node.iter_all_children():
        if isinstance(child, TypeNode):
            yield child


class StructNode(TypeNode, names.HasVanillaName):
    """A C++ proto struct definition scheduled for generation."""

    def __init__(
        self,
        *,
        vanilla_name: names.TypeName,
        proto_file: pathlib.Path,
        nested_types: Sequence[TypeNode],
        fields: Sequence[StructField],
    ) -> None:
        super().__init__(name=names.make_structs_type_name(vanilla_name))
        #: Vanilla message class name.
        self._vanilla_name: Final[names.TypeName] = vanilla_name
        #: Source proto file path, as it appears in imports.
        self.proto_file: Final[pathlib.Path] = proto_file
        #: Nested types definitions.
        self.nested_types: Final[Sequence[TypeNode]] = nested_types
        #: Struct fields.
        self.fields: Final[Sequence[StructField]] = [f for f in fields if f.short_name not in ['minor', 'major']]

    @property
    @override
    def kind(self) -> str:
        return 'struct'

    @property
    @override
    def children(self) -> Sequence[CodegenNode]:
        return self.nested_types

    @override
    def own_includes(self) -> Iterable[includes.Include]:
        yield from COMMON_STRUCT_INCLUDES
        # Need for IO.
        yield includes.Include(
            path=includes.proto_path_to_vanilla_pb_h(path=self.proto_file),
            kind=includes.IncludeKind.FOR_CPP,
        )
        for field in self.fields:
            yield from field.field_type.collect_includes()

    @override
    def type_dependencies(self) -> Iterable[type_ref.TypeDependency]:
        yield type_ref.TypeDependency(type_reference=self._vanilla_type_ref(), kind=type_ref.TypeDependencyKind.WEAK)
        for field in self.fields:
            yield from field.field_type.type_dependencies()
        for nested in self.nested_types:
            yield from nested.type_dependencies()

    def _vanilla_type_ref(self) -> type_ref.TypeReference:
        # TODO should move this method to HasVanillaName.
        # TODO should implement methods of HasVanillaName using TypeReference.full_cpp_name.
        return type_ref.VanillaCodegenType(name=self._vanilla_name, proto_file=self.proto_file)

    @property
    @override
    def vanilla_name(self) -> names.TypeName:
        return self._vanilla_name


COMMON_STRUCT_INCLUDES: Sequence[includes.Include] = [
    includes.Include(path='userver/proto-structs/io/fwd.hpp', kind=includes.IncludeKind.FOR_HPP),
    includes.Include(path='utility', kind=includes.IncludeKind.FOR_CPP),
    includes.Include(path='userver/proto-structs/io/impl/field_accessor.hpp', kind=includes.IncludeKind.FOR_CPP),
    includes.Include(path='userver/proto-structs/io/impl/read.hpp', kind=includes.IncludeKind.FOR_CPP),
    includes.Include(path='userver/proto-structs/io/impl/write.hpp', kind=includes.IncludeKind.FOR_CPP),
    includes.Include(path='userver/proto-structs/type_mapping.hpp', kind=includes.IncludeKind.FOR_CPP),
]


@dataclasses.dataclass
class StructField:
    """A field of a C++ proto struct."""

    #: Name of the field.
    short_name: Final[str]
    #: Type of the field.
    field_type: type_ref.TypeReference
    #: C++ code for the field initializer, e.g. '', '{}' or '{5}'.
    initializer: Final[str]
    #: Low-level numeric ID of the field for serialization. Absent for fields of type "oneof".
    number: Final[int | None]
    #: If this struct field maps to oneof, lists the fields of the oneof type.
    oneof_fields: Final[Sequence[StructField] | None]
    #: IoKind:
    io_kinds: Final[io.IoKind]

    @property
    def field_number_name(self) -> str:
        """
        Converts a `snake_case` or `camelCase` identifier to `PascalCase`.
        Example: some_bytes_my_word -> kSomeBytesMyWordFieldNumber
        Example: IYandexUid -> kIYandexUidFieldNumber
        """
        return f'k{names.to_pascal_case(self.short_name)}FieldNumber'

    @property
    def read_kind(self) -> str:
        return self.io_kinds.read.name.lower()

    @property
    def write_kind(self) -> str:
        return self.io_kinds.write.name.lower()

    @property
    def short_vanilla_name(self) -> str:
        if self.short_name.lower() in reserved_identifiers.CPP_KEYWORDS:
            return f'{self.short_name.lower()}_'
        if self.short_name.endswith('_'):
            if self.short_name[:-1].lower() in reserved_identifiers.CPP_KEYWORDS or self.short_name.endswith('__'):
                return self.short_name.lower()
        return self.short_name.removesuffix('_').lower()


def wrap_field_in_box(field: StructField) -> None:
    """
    Wrap type of the field in `utils::Box`.
    Policy:
     * Message types `T` are replaced with `utils::Box<T>`.
     * For optionals `std::optional<T>`, the inside is wrapped: `std::optional<utils::Box<T>>`.
       This prevents an unnecessary allocation for empty fields.
     * For repeated and map containers that require type definition, the whole container is wrapped, not each element.
     * For custom / unknown types, `T` is replaced with `utils::Box<T>`.
    """
    if isinstance(field.field_type, type_ref.TemplateType):
        if field.field_type.template.full_cpp_name() == type_ref_consts.OPTIONAL_TEMPLATE.full_cpp_name():
            # Wrap the type inside the optional in `utils::Box`, not the optional itself.
            value_type = field.field_type.template_args[0]
            field.field_type = type_ref_consts.make_optional(type_ref_consts.make_box(value_type))
            return
    field.field_type = type_ref_consts.make_box(field.field_type)


class EnumNode(TypeNode, names.HasVanillaName):
    """A C++ proto enum class definition scheduled for generation."""

    def __init__(
        self,
        *,
        vanilla_name: names.TypeName,
        proto_file: pathlib.Path,
        values: Sequence[EnumValue],
    ) -> None:
        super().__init__(name=names.make_structs_type_name(vanilla_name))
        #: Vanilla enum type name.
        self._vanilla_name: Final[names.TypeName] = vanilla_name
        #: Source proto file path, as it appears in imports.
        self.proto_file: Final[pathlib.Path] = proto_file
        #: Enum values (cases).
        self.values: Final[Sequence[EnumValue]] = values

    @property
    @override
    def kind(self) -> str:
        return 'enum'

    @override
    def own_includes(self) -> Iterable[includes.Include]:
        yield includes.Include(path='cstdint', kind=includes.IncludeKind.FOR_HPP)
        yield includes.Include(path='limits', kind=includes.IncludeKind.FOR_HPP)
        yield includes.Include(
            path=includes.proto_path_to_vanilla_pb_h(self.proto_file),
            kind=includes.IncludeKind.FOR_CPP,
        )

    @property
    @override
    def vanilla_name(self) -> names.TypeName:
        return self._vanilla_name


@dataclasses.dataclass(frozen=True)
class EnumValue:
    """A single value (case) of a C++ proto enum class."""

    #: Name of the enum case.
    short_name: str
    #: Integer value of the enum case.
    number: int


class OneofNode(TypeNode):
    """A C++ oneof class definition scheduled for generation."""

    def __init__(
        self,
        *,
        name: names.TypeName,
        fields: Sequence[StructField],
    ) -> None:
        super().__init__(name=name)
        #: Struct fields included in the oneof.
        self.fields: Final[Sequence[StructField]] = fields

    @property
    @override
    def kind(self) -> str:
        return 'oneof'

    @override
    def own_includes(self) -> Iterable[includes.Include]:
        yield from COMMON_ONEOF_INCLUDES
        for field in self.fields:
            yield from field.field_type.collect_includes()

    @override
    def type_dependencies(self) -> Iterable[type_ref.TypeDependency]:
        yield from super().type_dependencies()
        for field in self.fields:
            yield from field.field_type.type_dependencies()


COMMON_ONEOF_INCLUDES: Sequence[includes.Include] = [
    includes.Include(path='userver/proto-structs/impl/oneof_codegen.hpp', kind=includes.IncludeKind.FOR_HPP),
]


class VanillaTypeDeclaration(TypeNode, abc.ABC):
    """A C++ vanilla type."""

    def __init__(
        self,
        *,
        vanilla_name: names.TypeName,
    ) -> None:
        super().__init__(name=names.make_structs_type_name(vanilla_name))
        #: Vanilla type name.
        self.vanilla_name: Final[names.TypeName] = vanilla_name
        # `forward_decls.py` does not work across files, so just require the forward declaration here.
        self.should_forward_declare = True


class VanillaClassDeclaration(VanillaTypeDeclaration):
    """A C++ vanilla class."""

    @property
    @override
    def kind(self) -> str:
        return 'vanilla_class'
