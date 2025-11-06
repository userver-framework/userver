"""Loading of embedded resources."""

import functools
import pathlib
import typing
from typing import Callable
from typing import Optional

import library.python.resource


def read_file_text(path_relative_to_userver: str) -> str:
    find_resource = typing.cast(Callable[[str], Optional[bytes]], library.python.resource.resfs_read)
    if (file_contents := find_resource(path_relative_to_userver)) is not None:
        return file_contents.decode()
    return _read_file_tier1(path_relative_to_userver)


@functools.cache
def _read_file_tier1(path_relative_to_userver: str) -> str:
    userver_root = pathlib.Path(__file__).parent.parent.parent.parent
    return (userver_root / path_relative_to_userver).read_text()
