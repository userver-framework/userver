"""Concepts and helpers for io functions."""

import abc
import enum


class ReadGetterKind(enum.Enum):
    """The kind of ReadProtoStruct getter."""

    #: Oneof.
    ONEOF = enum.auto()
    #: std::optional.
    OPTIONAL = enum.auto()
    #: Other.
    OTHER = enum.auto()


class WriteSetterKind(enum.Enum):
    """The kind of WriteProtoStruct setter."""

    #: std::string or std::optional<std::string>.
    STRING = enum.auto()
    #: std::vector.
    VECTOR = enum.auto()
    #: proto_structs::HashMap.
    MAP = enum.auto()
    #: Proto message.
    MESSAGE = enum.auto()
    #: Oneof.
    ONEOF = enum.auto()
    #: Other.
    OTHER = enum.auto()
    #: Combine.
    VECTOR_MAP_MESSAGE = VECTOR | MAP | MESSAGE


class HasIO(abc.ABC):
    def read_kind_impl(self, kind: ReadGetterKind) -> str:
        return kind.name.lower()

    def write_kind_impl(self, kind: WriteSetterKind) -> str:
        return kind.name.lower()
