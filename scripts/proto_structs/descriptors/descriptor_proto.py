"""Functions for working with raw descriptors from `google.protobuf.descriptor_pb2`."""

from collections.abc import Mapping
import functools

import google.protobuf.descriptor as descriptor
import google.protobuf.descriptor_pb2 as descriptor_pb2  # pyright: ignore


@functools.cache
def to_descriptor_proto(message: descriptor.Descriptor) -> descriptor_pb2.DescriptorProto:  # pyright: ignore
    """Converts `Descriptor` to `DescriptorProto`."""
    message_proto = descriptor_pb2.DescriptorProto()  # pyright: ignore
    message.CopyToProto(message_proto)  # pyright: ignore
    return message_proto  # pyright: ignore


@functools.cache
def message_fields_map(  # pyright: ignore
    message: descriptor.Descriptor,
) -> Mapping[str, descriptor_pb2.FieldDescriptorProto]:  # pyright: ignore
    """Returns a mapping from field names to their descriptors."""
    message_proto = to_descriptor_proto(message)  # pyright: ignore
    fields: list[descriptor_pb2.FieldDescriptorProto] = message_proto.field  # pyright: ignore
    result: dict[str, descriptor_pb2.FieldDescriptorProto] = {}  # pyright: ignore
    for field in fields:  # pyright: ignore
        field_name: str = field.name  # pyright: ignore
        result[field_name] = field  # pyright: ignore
    return result  # pyright: ignore


def to_field_descriptor_proto(  # pyright: ignore
    field: descriptor.FieldDescriptor,
) -> descriptor_pb2.FieldDescriptorProto:  # pyright: ignore
    """Converts `FieldDescriptor` to `FieldDescriptorProto`."""
    field_name: str = field.name
    containing_message: descriptor.Descriptor = field.containing_type
    field_proto = message_fields_map(containing_message).get(field_name)  # pyright: ignore
    assert field_proto, f'Field "{field_name}" of descriptors.Descriptor is missing in descriptor_pb2.DescriptorProto'
    return field_proto  # pyright: ignore
