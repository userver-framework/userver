"""Models for usages of C++ types."""

from __future__ import annotations

import abc
from collections.abc import Iterable
from collections.abc import Sequence
import dataclasses
import enum
import itertools
from typing import Optional

from typing_extensions import override

from proto_structs.models import includes
from proto_structs.models import names


class TypeDependencyKind(enum.Enum):
    """The kind in which a C++ entity depends on another."""

    #: A C++ type **definition** is required.
    STRONG = enum.auto()
    #: A C++ type **declaration** is required.
    WEAK = enum.auto()


@dataclasses.dataclass
class TypeDependency:
    """A dependency that a C++ entity has on a C++ type."""

    #: The C++ type that is required.
    type_reference: TypeReference
    #: What is required from the type.
    kind: TypeDependencyKind


class HasTypeDependencies(abc.ABC):
    """A C++ entity that requires type declarations or definitions."""

    @abc.abstractmethod
    def type_dependencies(self) -> Iterable[TypeDependency]:
        """Returns types which this C++ entity requires."""
        raise NotImplementedError()


class TypeReference(names.HasCppName, includes.HasCppIncludes, HasTypeDependencies, abc.ABC):
    """A usage of C++ type (not its definition)."""

    @override
    def type_dependencies(self) -> Iterable[TypeDependency]:
        # Usage of a C++ type requires its definition by default.
        return [TypeDependency(type_reference=self, kind=TypeDependencyKind.STRONG)]


@dataclasses.dataclass(frozen=True)
class TemplateType(TypeReference):
    """A usage of a compound template type, together with its template arguments."""

    #: The type template without args.
    template: TypeReference
    #: Entities passed as template parameter values. May be empty, `Foo<>` can be a valid 0-arg template instantiation.
    template_args: Sequence[TypeReference]
    #: Template can be defined without template args definitions.
    works_with_forward_references: bool = False

    @override
    def full_cpp_name(self) -> str:
        own_name = self.template.full_cpp_name()
        args = self.template_args
        return f'{own_name}<{", ".join(arg.full_cpp_name() for arg in args)}>'

    @override
    def contextual_cpp_name(self, *, context: names.HasCppName) -> str:
        own_name = self.template.contextual_cpp_name(context=context)
        args = self.template_args
        return f'{own_name}<{", ".join(arg.contextual_cpp_name(context=context) for arg in args)}>'

    @override
    def full_cpp_name_segments(self) -> Sequence[str]:
        # Example: `foo::Bar::Baz<A, B> -> ['foo', 'Bar', 'Baz<A, B>']
        template_name_segments = self.template.full_cpp_name_segments()
        assert template_name_segments
        args = self.template_args
        last_segment = f'{template_name_segments[-1]}<{", ".join(arg.full_cpp_name() for arg in args)}>'
        return *itertools.islice(template_name_segments, len(template_name_segments) - 1), last_segment

    @override
    def collect_includes(self) -> Iterable[str]:
        yield from self.template.collect_includes()
        for arg in self.template_args:
            yield from arg.collect_includes()

    @override
    def type_dependencies(self) -> Iterable[TypeDependency]:
        yield from self.template.type_dependencies()
        for arg in self.template_args:
            if self.works_with_forward_references:
                for sub_arg in arg.type_dependencies():
                    yield TypeDependency(sub_arg.type_reference, TypeDependencyKind.WEAK)
            else:
                yield from arg.type_dependencies()


class KeywordType(TypeReference, names.HasCppName):
    """A C++ keyword that is represents a type."""

    def __init__(self, *, full_cpp_name: str) -> None:
        super().__init__()
        self._full_cpp_name = full_cpp_name

    @override
    def full_cpp_name(self) -> str:
        return self._full_cpp_name

    @override
    def contextual_cpp_name(self, *, context: names.HasCppName) -> str:
        return self._full_cpp_name

    @override
    def full_cpp_name_segments(self) -> Sequence[str]:
        return (self._full_cpp_name,)

    @override
    def collect_includes(self) -> Sequence[str]:
        return ()


class BuiltinType(TypeReference, names.HasCppNameImpl):
    """A usage of a non-code-generated C++ type."""

    def __init__(self, *, full_cpp_name: str, include: str) -> None:
        super().__init__()
        self._full_cpp_name = full_cpp_name
        self._include = include

    @override
    def full_cpp_name_segments(self) -> Sequence[str]:
        # Return a single segment so that `contextual_cpp_name(context) == full_cpp_name`.
        return (self._full_cpp_name,)

    @override
    def collect_includes(self) -> Iterable[str]:
        return [self._include]


_hardcoded_userver_namespace: Optional[str] = None


def set_hardcoded_userver_namespace(namespace: Optional[str]) -> None:
    global _hardcoded_userver_namespace
    _hardcoded_userver_namespace = namespace


class UserverLibraryType(TypeReference, names.HasCppNameImpl):
    """A usage of a non-code-generated type from userver."""

    def __init__(self, *, full_cpp_name_wo_userver: str, include: str) -> None:
        super().__init__()
        self._full_cpp_name_wo_userver = full_cpp_name_wo_userver
        self._include = include

    @override
    def full_cpp_name_segments(self) -> Sequence[str]:
        if _hardcoded_userver_namespace is not None:
            userver_namespace = _hardcoded_userver_namespace
        else:
            userver_namespace = 'USERVER_NAMESPACE'

        # Return a single segment so that `contextual_cpp_name(context) == full_cpp_name`.
        return (f'{userver_namespace}::{self._full_cpp_name_wo_userver}',)

    @override
    def collect_includes(self) -> Iterable[str]:
        return [self._include]


BOX_TEMPLATE = UserverLibraryType(full_cpp_name_wo_userver='utils::Box', include='userver/utils/box.hpp')


class UserverCodegenType(TypeReference, names.HasCppNameImpl):
    """A usage of a C++ type generated by userver proto_structs plugin."""

    def __init__(self, *, name: names.TypeName, include: str) -> None:
        super().__init__()
        self._name = name
        self._include = include

    @override
    def full_cpp_name_segments(self) -> Sequence[str]:
        return self._name.name_segments()

    @override
    def collect_includes(self) -> Iterable[str]:
        return [self._include]


class VanillaCodegenType(TypeReference, names.HasCppNameImpl):
    """A usage of a type generated by vanilla C++ protoc plugin."""

    def __init__(self, *, name: names.TypeName, include: str) -> None:
        super().__init__()
        self._name = name
        self._include = include

    @override
    def full_cpp_name_segments(self) -> Sequence[str]:
        return self._name.name_segments()

    @override
    def collect_includes(self) -> Iterable[str]:
        return [self._include]
