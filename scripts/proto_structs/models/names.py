"""Concepts and helpers related to C++ and Protobuf naming."""

from __future__ import annotations

import abc
from collections.abc import Sequence
import dataclasses
import re

from proto_structs.models import reserved_identifiers


class HasCppName(abc.ABC):
    """A C++ entity with a name."""

    @abc.abstractmethod
    def full_cpp_name(self) -> str:
        """Returns the fully-qualified name of the C++ entity, including parents. Mostly intended for diagnostics."""
        raise NotImplementedError()

    @abc.abstractmethod
    def contextual_cpp_name(self, *, context: HasCppName) -> str:
        """
        Returns the "enough-qualified" name of the C++ entity usable from inside the definition of `context`.
        Examples:
        contextual_cpp_name(foo::Bar::Baz::Qux, foo::Bar) -> Baz::Qux
        contextual_cpp_name(std::optional<foo::Bar::Baz::Qux>, foo::Bar) -> std::optional<Baz::Qux>
        contextual_cpp_name(foo, foo::bar) -> foo
        """
        raise NotImplementedError()

    @abc.abstractmethod
    def full_cpp_name_segments(self) -> Sequence[str]:
        """Returns parts of the full C++ name."""
        raise NotImplementedError()


class HasCppNameImpl(HasCppName, abc.ABC):
    """
    An implementation of `HasCppName` suitable for most cases where a name is a single qualified ID,
    e.g. `foo::bar::Baz::Qux` is OK, but `std::optional<foo::Bar>` is NOT ok.
    """

    def full_cpp_name(self) -> str:
        segments = self.full_cpp_name_segments()
        assert all(segments), f'Empty name segments are not allowed for {self}'
        return '::'.join(segments)

    def contextual_cpp_name(self, *, context: HasCppName) -> str:
        return _get_contextual_cpp_name(self, context=context)


_GLOBAL_PREFIXES: Sequence[str] = (
    '::',
    'std::',
    'USERVER_NAMESPACE::',
)


def _get_contextual_cpp_name(entity: HasCppName, *, context: HasCppName) -> str:
    node_names = entity.full_cpp_name_segments()
    assert node_names, f'Cannot render a reference to an instance of {entity.__class__} by an empty name'
    assert all(node_names), f'Empty name segments are not allowed for {entity=}'

    if node_names[0] in reserved_identifiers.CPP_KEYWORDS:
        assert len(node_names) == 1
        return node_names[0]

    if node_names[0] == 'std' or any(node_names[0].startswith(prefix) for prefix in _GLOBAL_PREFIXES):
        # Always mention std:: entities by full name, global namespace prefixing is not required.
        # Do not prefix USERVER_NAMESPACE:: names with global namespace as well (it results in a syntax error).
        return '::'.join(node_names)

    context_names = context.full_cpp_name_segments()
    assert all(context_names), f'Empty name segments are not allowed for {context=}'

    if not context_names:
        # If no context then all we can do is returning a full cpp name.
        return '::' + '::'.join(node_names)

    if node_names[0] != context_names[0]:
        # If no common ancestor then all we can do is returning a full cpp name.
        return '::' + '::'.join(node_names)

    # "index + 1 < len(node_names)" covers case:
    # struct A {  // node=['A']
    #   struct B {  // context=['A', 'B']
    #     A field;  // get_contextual_cpp_name(node=A, context=B)
    #   }
    # }
    #
    # "index < len(context_names)" covers case:
    # struct A {  // context=['A']
    #   struct B {}  // node=['A', 'B']
    #
    #   B field;  // get_contextual_cpp_name(node=B, context=A)
    # }
    #
    # "node_names[index] == context_names[index]" covers case:
    # struct A {
    #   struct B {
    #       struct C {}  // node=['A', 'B', 'C']
    #   }
    #
    #   struct D {  // context=['A', 'D']
    #       B::C field;  // get_contextual_cpp_name(node=C, context=D)
    #   }
    # }
    index = 0
    while index + 1 < len(node_names) and index < len(context_names) and node_names[index] == context_names[index]:
        index += 1

    return '::'.join(node_names[index:])


_ID_REGEX = re.compile(r'[a-zA-Z_][a-zA-Z0-9_]*')


def is_valid_id(identifier: str) -> bool:
    return _ID_REGEX.fullmatch(identifier) is not None


def escape_id(identifier: str) -> str:
    """
    Escapes a Protobuf identifier for use in C++.
    Uses the same escaping technique as the native C++ Protobuf codegen.
    """
    if identifier in reserved_identifiers.CPP_RESERVED_IDENTIFIERS:
        return f'{identifier}_'
    return identifier


def package_to_namespace(package: str) -> str:
    """Converts a proto package to C++ namespace."""
    return '::'.join(escape_id(part) for part in package.split('.'))


def proto_namespace_to_segments(namespace: str) -> Sequence[str]:
    """Returns namespace parts, as required by `HasCppName.full_cpp_name_segments`."""
    if not namespace:
        return ()
    # Returning a single segment to force `contextual_cpp_name` to either mention the package-namespace as a whole
    # or to omit it completely, mimicking proto naming rules.
    return (namespace,)


@dataclasses.dataclass(frozen=True)
class TypeName:
    """The qualified name of a C++ type."""

    #: Namespace obtained from the Protobuf package.
    namespace_segments: Sequence[str]
    #: Containing types, if any, outer-to-inner, e.g. for `foo::Bar::Baz::Qux` it is `[Bar, Baz]`.
    outer_type_names: Sequence[str]
    #: Name of the type itself without outer types or namespaces.
    short_name: str

    def name_segments(self) -> Sequence[str]:
        """Returns all segments of the qualified name."""
        return *self.namespace_segments, *self.outer_type_names, self.short_name


def make_escaped_type_name(*, package: str, outer_type_names: Sequence[str], short_name: str) -> TypeName:
    """Escapes a Protobuf full type name into a C++ `TypeName`."""
    return TypeName(
        namespace_segments=proto_namespace_to_segments(package_to_namespace(package)),
        outer_type_names=[escape_id(type_name) for type_name in outer_type_names],
        short_name=escape_id(short_name),
    )


def make_structs_type_name(vanilla_type_name: TypeName) -> TypeName:
    """Makes a name for a proto-structs type from a vanilla type name."""
    return dataclasses.replace(vanilla_type_name, namespace_segments=(*vanilla_type_name.namespace_segments, 'structs'))


def make_nested_type_name(containing_type_name: TypeName, short_name: str) -> TypeName:
    """Makes a `TypeName` for a nested type."""
    return TypeName(
        namespace_segments=containing_type_name.namespace_segments,
        outer_type_names=(*containing_type_name.outer_type_names, containing_type_name.short_name),
        short_name=short_name,
    )


def to_pascal_case(name: str) -> str:
    """Converts a `snake_case` or `camelCase` identifier to `PascalCase`."""
    return ''.join(part.capitalize() for part in name.split('_'))


def to_upper_case(name: str) -> str:
    """Converts a `snake_case`, `camelCase`, `PascalCase` or `UPPER_CASE` identifier to `UPPER_CASE`."""
    if name.isupper():
        return name
    result = ''
    for i, char in enumerate(name):
        if char.isupper() and i > 0:
            result += '_'
        result += char
    return result.upper()
