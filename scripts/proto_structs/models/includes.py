"""Concepts and helpers related to C++ includes and Protobuf imports."""

import abc
from collections.abc import Iterable
import dataclasses
import enum
import pathlib
from typing import List
from typing import Optional

BUNDLE_STRUCTS_HPP = 'userver/proto-structs/impl/bundles/structs_hpp.hpp'
BUNDLE_STRUCTS_CPP = 'userver/proto-structs/impl/bundles/structs_cpp.hpp'


class IncludeKind(enum.Enum):
    """The kind of C++ include."""

    #: For .hpp.
    FOR_HPP = enum.auto()
    #: For .cpp.
    FOR_CPP = enum.auto()
    #: Vanille include.
    VANILLA = enum.auto()


@dataclasses.dataclass
class Include:
    """A C++ include"""

    #: Include path without '<>'.
    path: str
    #: Include type.
    kind: IncludeKind

    def __hash__(self):
        return hash(self.path)

    def __lt__(self, other: 'Include') -> bool:
        return self.path < other.path


class HasCppIncludes(abc.ABC):
    """A C++ entity or entity group that may require includes to define."""

    @abc.abstractmethod
    def collect_includes(self) -> Iterable[Include]:
        """Collects C++ includes required for rendering this and nested C++ entities. May contain duplicates."""
        raise NotImplementedError()


def sorted_includes(entity: HasCppIncludes, *, current_hpp: Optional[str] = None) -> List[Include]:
    """Returns a ready-to-use list of C++ includes required for rendering this and nested C++ entities."""
    includes_set = set(entity.collect_includes())
    if current_hpp:
        includes_set.discard(Include(path=current_hpp, kind=IncludeKind.FOR_HPP))
    return sorted(includes_set)


def proto_path_to_structs_path(proto_relative_path: pathlib.Path, *, ext: str) -> pathlib.Path:
    """Returns the path of structs hpp or cpp based on the path of original proto file (relative to source dir)."""
    return proto_relative_path.with_suffix(f'.structs.usrv.pb.{ext}')


def proto_path_to_vanilla_pb_h(proto_relative_path: pathlib.Path) -> pathlib.Path:
    """Returns the path of vanilla C++ `.pb.h` file (relative to source dir)."""
    return proto_relative_path.with_suffix('.pb.h')
