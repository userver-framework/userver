"""Concepts and helpers for io functions."""

import dataclasses
import enum


class ReadVanillaFieldKind(enum.Enum):
    """The kind of ReadProtoStruct getter. A kind is depended only from protobuf type."""

    #: Oneof.
    ONEOF = enum.auto()
    #: std::optional.
    OPTIONAL = enum.auto()
    #: Other.
    OTHER = enum.auto()


class WriteVanillaFieldKind(enum.Enum):
    """The kind of WriteProtoStruct setter. A kind is depended only from protobuf type."""

    #: std::string or std::optional<std::string>.
    STRING = enum.auto()
    #: Oneof.
    ONEOF = enum.auto()
    #: Other.
    OTHER = enum.auto()
    #: Vector|Map|Message.
    VECTOR_MAP_MESSAGE = enum.auto()


@dataclasses.dataclass
class IoKind:
    """IO kinds for getters and setters."""

    #: Read kind.
    read: ReadVanillaFieldKind = ReadVanillaFieldKind.OTHER
    #: Write kind.
    write: WriteVanillaFieldKind = WriteVanillaFieldKind.OTHER
