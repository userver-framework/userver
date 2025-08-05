"""Concepts and helpers related to C++ includes and Protobuf imports."""

import abc
from collections.abc import Iterable
import pathlib
from typing import final
from typing import List
from typing import Optional


class HasCppIncludes(abc.ABC):
    """A C++ entity or entity group that may require includes to define."""

    @final  # Prevent derived classes from overriding this instead of `collect_includes` by mistake.
    def sorted_includes(self, *, current_hpp: Optional[str] = None) -> List[str]:
        """Returns a ready-to-use list of C++ includes required for rendering this and nested C++ entities."""
        includes_set = set(self.collect_includes())
        if current_hpp:
            includes_set.discard(current_hpp)
        return sorted(includes_set)

    @abc.abstractmethod
    def collect_includes(self) -> Iterable[str]:
        """Collects C++ includes required for rendering this and nested C++ entities. May contain duplicates."""
        raise NotImplementedError()


def proto_path_to_structs_path(proto_relative_path: pathlib.Path, *, ext: str) -> pathlib.Path:
    """Returns the path of structs hpp or cpp based on the path of original proto file (relative to source dir)."""
    stem = proto_relative_path.name.removesuffix('.proto')
    return proto_relative_path.parent / f'{stem}.structs.usrv.pb.{ext}'
