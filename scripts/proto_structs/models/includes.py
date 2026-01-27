"""Concepts and helpers related to C++ includes and Protobuf imports."""

import abc
from collections.abc import Iterable
import dataclasses
import enum
import pathlib

from proto_structs.bundles import includes_bundles
from proto_structs.models import names


class IncludeKind(enum.Enum):
    """The kind of C++ include."""

    #: For .hpp.
    FOR_HPP = enum.auto()
    #: For .cpp.
    FOR_CPP = enum.auto()


@dataclasses.dataclass(order=True, frozen=True)
class Include:
    """A C++ include"""

    #: Include path without '<>'.
    path: str
    #: Include type.
    kind: IncludeKind


class HasCppIncludes(abc.ABC):
    """A C++ entity or entity group that may require includes to define."""

    @abc.abstractmethod
    def collect_includes(self) -> Iterable[Include]:
        """Collects C++ includes required for rendering this and nested C++ entities. May contain duplicates."""
        raise NotImplementedError()


def sorted_includes(entity: HasCppIncludes, *, current_hpp: str | None = None) -> dict[IncludeKind, list[str]]:
    """Returns a ready-to-use list of C++ includes required for rendering this and nested C++ entities."""
    includes_set = set(entity.collect_includes())
    if current_hpp:
        includes_set.discard(Include(path=current_hpp, kind=IncludeKind.FOR_HPP))
    sorted_includes = sorted(includes_set)
    return {
        IncludeKind.FOR_HPP: [
            include.path
            for include in sorted_includes
            if include.kind == IncludeKind.FOR_HPP and include.path not in includes_bundles.bundle_hpp()
        ],
        IncludeKind.FOR_CPP: [
            include.path
            for include in sorted_includes
            if include.kind == IncludeKind.FOR_CPP and include.path not in includes_bundles.bundle_cpp()
        ],
    }


def proto_path_to_structs_path(proto_relative_path: pathlib.Path, *, ext: str) -> pathlib.Path:
    """
    Returns the path of structs hpp or cpp based on the path of original proto file (relative to source dir).
    Example: 'simple/subdirectory/subdirectory.proto' -> 'simple/subdirectory/subdirectory.structs.usrv.pb.{ext}'
    """
    assert proto_relative_path.suffix == '.proto'

    return proto_relative_path.with_suffix(f'.structs.usrv.pb.{ext}')


def proto_path_to_vanilla_pb_h(path: pathlib.Path) -> str:
    """Returns the path of vanilla C++ `.pb.h` file (relative to source dir)."""
    return str(path.with_suffix('.pb.h'))


def io_includes_by_full_name(name: str, *, prefix: str) -> list[Include]:
    """Returns includes for IO."""

    name = names.to_snake_case(name)
    path: str = prefix + '/'.join([word.removeprefix('_') for word in name.split('::')])
    return [
        Include(path=path + '.hpp', kind=IncludeKind.FOR_HPP),
        Include(path=path + '_conv.hpp', kind=IncludeKind.FOR_CPP),
    ]
