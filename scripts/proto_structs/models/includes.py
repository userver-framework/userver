"""Concepts and helpers related to C++ includes and Protobuf imports."""

import abc
from collections.abc import Iterable
import pathlib
from typing import List
from typing import Optional

BUNDLE_STRUCTS_HPP = 'userver/proto-structs/impl/bundles/structs_hpp.hpp'
BUNDLE_STRUCTS_CPP = 'userver/proto-structs/impl/bundles/structs_cpp.hpp'


class HasCppIncludes(abc.ABC):
    """A C++ entity or entity group that may require includes to define."""

    @abc.abstractmethod
    def collect_includes(self) -> Iterable[str]:
        """Collects C++ includes required for rendering this and nested C++ entities. May contain duplicates."""
        raise NotImplementedError()


def sorted_includes(entity: HasCppIncludes, *, current_hpp: Optional[str] = None) -> List[str]:
    """Returns a ready-to-use list of C++ includes required for rendering this and nested C++ entities."""
    includes_set = set(entity.collect_includes())
    if current_hpp:
        includes_set.discard(current_hpp)
    return sorted(includes_set)


def proto_path_to_structs_path(proto_relative_path: pathlib.Path, *, ext: str) -> pathlib.Path:
    """Returns the path of structs hpp or cpp based on the path of original proto file (relative to source dir)."""
    stem = proto_relative_path.name.removesuffix('.proto')
    return proto_relative_path.parent / f'{stem}.structs.usrv.pb.{ext}'
