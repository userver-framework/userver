"""Parsers from Protobuf descriptors to CodegenNode."""

# Documentation on Protobuf descriptors:
# https://googleapis.dev/python/protobuf/latest/google/protobuf/descriptor.html

import pathlib
import typing
from typing import Dict
from typing import List
from typing import Union

import google.protobuf.descriptor as descriptor

from proto_structs.descriptors import type_mapping
from proto_structs.models import gen_node
from proto_structs.models import names
from proto_structs.models import type_ref

TypeDescriptor = Union[descriptor.Descriptor, descriptor.EnumDescriptor]


def parse_file(file: descriptor.FileDescriptor) -> gen_node.File:
    namespace_members: List[gen_node.CodegenNode] = []
    for enum in typing.cast(Dict[str, descriptor.EnumDescriptor], file.enum_types_by_name).values():
        namespace_members.append(parse_enum(enum))
    for message in typing.cast(Dict[str, descriptor.Descriptor], file.message_types_by_name).values():
        namespace_members.append(parse_message(message))
    # TODO perform a topological sort of namespace_members
    structs_namespace = gen_node.NamespaceNode.make_for_structs(
        typing.cast(str, file.package),
        children=namespace_members,
    )
    return gen_node.File(
        proto_relative_path=pathlib.Path(typing.cast(str, file.name)),
        children=(structs_namespace,),
    )


def parse_enum(enum: descriptor.EnumDescriptor) -> gen_node.EnumNode:
    values: List[gen_node.EnumValue] = []
    for value in typing.cast(List[descriptor.EnumValueDescriptor], enum.values):
        values.append(parse_enum_value(value))
    return gen_node.EnumNode(name=type_mapping.parse_type_name(enum), values=values)


def parse_enum_value(value: descriptor.EnumValueDescriptor) -> gen_node.EnumValue:
    # TODO cut enum value name if it begins with the enum name, e.g. FooEnum.FOO_ENUM_BAR -> FooEnum::BAR
    return gen_node.EnumValue(
        short_name=names.escape_id(typing.cast(str, value.name)),
        number=typing.cast(int, value.number),
    )


def parse_message(message: descriptor.Descriptor) -> gen_node.StructNode:
    nested_types: List[gen_node.TypeNode] = []
    for nested_enum in typing.cast(List[descriptor.EnumDescriptor], message.enum_types):
        nested_types.append(parse_enum(nested_enum))
    for nested_message in typing.cast(List[descriptor.Descriptor], message.nested_types):
        nested_types.append(parse_message(nested_message))

    fields: List[gen_node.StructField] = []
    for field in typing.cast(List[descriptor.FieldDescriptor], message.fields):
        fields.append(parse_field(field, nested_types_to_generate=nested_types))

    # TODO account for oneof

    return gen_node.StructNode(name=type_mapping.parse_type_name(message), nested_types=nested_types, fields=fields)


def parse_field(
    field: descriptor.FieldDescriptor,
    nested_types_to_generate: List[gen_node.TypeNode],
) -> gen_node.StructField:
    parsed_type = _parse_type_reference(field, nested_types_to_generate)
    type_with_label = type_mapping.handle_type_label(field, parsed_type)

    return gen_node.StructField(
        short_name=names.escape_id(typing.cast(str, field.name)),
        field_type=type_with_label,
        number=typing.cast(int, field.number),
    )


def _parse_type_reference(
    field_type: descriptor.FieldDescriptor,
    nested_types_to_generate: List[gen_node.TypeNode],
) -> type_ref.TypeReference:
    type_kind = typing.cast(int, field_type.type)
    if builtin_type := type_mapping.BUILTIN_TYPES.get(type_kind):
        return builtin_type

    if type_kind == descriptor.FieldDescriptor.TYPE_ENUM:
        enum_type = typing.cast(descriptor.EnumDescriptor, field_type.enum_type)
        return type_mapping.parse_enum_reference(enum_type)

    if type_kind == descriptor.FieldDescriptor.TYPE_MESSAGE:
        message_type = typing.cast(descriptor.Descriptor, field_type.message_type)
        return type_mapping.parse_struct_reference(message_type)

    if type_kind == descriptor.FieldDescriptor.TYPE_GROUP:
        # Details on groups:
        # https://protobuf.com/docs/descriptors#synthetic-oneofs
        message_type = typing.cast(descriptor.Descriptor, field_type.message_type)
        nested_types_to_generate.append(parse_message(message_type))
        return type_mapping.parse_struct_reference(message_type)

    raise RuntimeError(f'Invalid field type kind: {type_kind}')
