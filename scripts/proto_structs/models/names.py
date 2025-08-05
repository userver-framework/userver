"""Concepts and helpers related to C++ and Protobuf naming."""

from __future__ import annotations

from collections.abc import Sequence
import dataclasses


def package_to_namespace(package: str) -> str:
    """Converts a proto package to C++ namespace."""
    return package.replace('.', '::')


def proto_namespace_to_segments(namespace: str) -> Sequence[str]:
    """Returns namespace parts."""
    if not namespace:
        return ()
    return (namespace,)


@dataclasses.dataclass(frozen=True)
class TypeName:
    """An abstraction around Protobuf type names. Maps to C++ types easily."""

    #: Namespace obtained from the Protobuf package.
    proto_namespace: str
    #: Containing types, if any, outer-to-inner, excluding self.
    outer_type_names: Sequence[str]
    #: Name of the type itself without outer types or namespaces.
    short_name: str


def make_type_name(*, package: str, outer_type_names: Sequence[str], short_name: str) -> TypeName:
    """Escapes a Protobuf full type name into a C++ `TypeName`."""
    return TypeName(
        proto_namespace=package_to_namespace(package),
        outer_type_names=outer_type_names,
        short_name=short_name,
    )
