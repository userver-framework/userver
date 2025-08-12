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
from proto_structs.models import names
from proto_structs.models import type_ref


class CodegenNode(names.HasCppNameImpl, includes.HasCppIncludes, abc.ABC):
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

    @override
    @abc.abstractmethod
    def full_cpp_name_segments(self) -> Sequence[str]:
        raise NotImplementedError()

    @final  # Prevent derived classes from overriding this instead of `own_includes` by mistake.
    @override
    def collect_includes(self) -> Iterable[str]:
        yield from self.own_includes()
        for child in self.children:
            yield from child.collect_includes()

    def own_includes(self) -> Iterable[str]:
        """
        Returns includes required by the current node itself, not accounting for `children`.
        Empty by default, intended to be overridden.
        """
        return ()


class File(includes.HasCppIncludes):
    """A C++ proto structs file scheduled for generation."""

    def __init__(self, *, proto_relative_path: pathlib.Path, children: Sequence[CodegenNode]) -> None:
        super().__init__()
        assert not proto_relative_path.is_absolute()
        #: Path to the original proto file relative to its base source directory.
        self.proto_relative_path: Final[pathlib.Path] = proto_relative_path
        #: The top-level C++ entities to define in the file.
        self.children = children

    @override
    def collect_includes(self) -> Iterable[str]:
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


class TypeNode(CodegenNode, abc.ABC):
    """A base class that contains the common logic for any C++ type definition scheduled for generation."""

    def __init__(self, *, name: names.TypeName) -> None:
        super().__init__()
        #: Detailed type name.
        self.name: Final[names.TypeName] = name

    @override
    def full_cpp_name_segments(self) -> Sequence[str]:
        return (
            *names.proto_namespace_to_segments(self.name.proto_namespace),
            'structs',
            *self.name.outer_type_names,
            self.name.short_name,
        )


class StructNode(TypeNode):
    """A C++ proto struct definition scheduled for generation."""

    def __init__(
        self,
        *,
        name: names.TypeName,
        nested_types: Sequence[TypeNode],
        fields: Sequence[StructField],
    ) -> None:
        super().__init__(name=name)
        #: Nested types definitions.
        self.nested_types: Final[Sequence[TypeNode]] = nested_types
        #: Struct fields.
        self.fields: Final[Sequence[StructField]] = fields

    @property
    @override
    def kind(self) -> str:
        return 'struct'

    @property
    @override
    def children(self) -> Sequence[CodegenNode]:
        return self.nested_types

    @override
    def own_includes(self) -> Iterable[str]:
        for field in self.fields:
            yield from field.field_type.collect_includes()


@dataclasses.dataclass(frozen=True)
class StructField:
    """A field of a C++ proto struct."""

    #: Name of the field.
    short_name: str
    #: Type of the field.
    field_type: type_ref.TypeReference
    #: Integer value of the enum case.
    number: int


class EnumNode(TypeNode):
    """A C++ proto enum class definition scheduled for generation."""

    def __init__(self, *, name: names.TypeName, values: Sequence[EnumValue]) -> None:
        super().__init__(name=name)
        #: Enum values (cases).
        self.values: Final[Sequence[EnumValue]] = values

    @property
    @override
    def kind(self) -> str:
        return 'enum'

    @override
    def own_includes(self) -> Iterable[str]:
        # 'cstdint' and 'limits' are included in bundle_hpp.
        return [includes.BUNDLE_STRUCTS_HPP]


@dataclasses.dataclass(frozen=True)
class EnumValue:
    """A single value (case) of a C++ proto enum class."""

    #: Name of the enum case.
    short_name: str
    #: Integer value of the enum case.
    number: int
