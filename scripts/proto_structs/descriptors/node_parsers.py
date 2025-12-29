"""Parsers from Protobuf descriptors to CodegenNode."""

# Documentation on Protobuf descriptors:
# https://googleapis.dev/python/protobuf/latest/google/protobuf/descriptor.html

from collections.abc import Mapping
from collections.abc import MutableSet
import pathlib
import re
import typing
from typing import Any

import google.protobuf.descriptor as descriptor

from proto_structs.descriptors import option_parsers
from proto_structs.descriptors import services
from proto_structs.descriptors import type_mapping
from proto_structs.models import forward_decls
from proto_structs.models import gen_node
from proto_structs.models import io
from proto_structs.models import names
from proto_structs.models import options
from proto_structs.models import sort_dependencies
from proto_structs.models import type_ref
from proto_structs.models import type_ref_consts
from proto_structs.models import vanilla


def parse_file(file: descriptor.FileDescriptor, *, plugin_options: options.PluginOptions) -> gen_node.File:
    types: list[gen_node.TypeNode] = []
    for enum in typing.cast(dict[str, descriptor.EnumDescriptor], file.enum_types_by_name).values():
        types.append(parse_enum(enum))
    for message in typing.cast(dict[str, descriptor.Descriptor], file.message_types_by_name).values():
        if message_type := parse_message(message, plugin_options=plugin_options):
            types.append(message_type)

    types = sort_dependencies.sort_types_topologically(types)

    vanilla_namespace = gen_node.NamespaceNode.make_for_vanilla(
        typing.cast(str, file.package),
        children=vanilla.collect_vanilla_types(types=types),
    )
    structs_namespace = gen_node.NamespaceNode.make_for_structs(
        typing.cast(str, file.package),
        children=types,
    )

    forward_decls.add_forward_declarations((structs_namespace,))

    return gen_node.File(
        proto_relative_path=pathlib.Path(typing.cast(str, file.name)),
        children=[vanilla_namespace, structs_namespace],
        services_type_dependencies=list(services.collect_rpc_input_output_types(file)),
    )


def parse_enum(enum: descriptor.EnumDescriptor) -> gen_node.EnumNode:
    proto_file_descriptor: descriptor.FileDescriptor = enum.file
    proto_file_name: str = proto_file_descriptor.name
    values: list[gen_node.EnumValue] = []
    for value in typing.cast(list[descriptor.EnumValueDescriptor], enum.values):
        values.append(parse_enum_value(value))
    return gen_node.EnumNode(
        vanilla_name=type_mapping.parse_type_name(enum),
        proto_file=pathlib.Path(proto_file_name),
        values=values,
    )


def parse_enum_value(value: descriptor.EnumValueDescriptor) -> gen_node.EnumValue:
    enum_value_name = typing.cast(str, value.name)
    enum_descriptor = typing.cast(descriptor.EnumDescriptor, value.type)
    enum_name = typing.cast(str, enum_descriptor.name)

    return gen_node.EnumValue(
        short_name=names.escape_id(_format_enum_value_name(enum_value_name, enum_name=enum_name)),
        number=typing.cast(int, value.number),
    )


def _format_enum_value_name(value_name: str, *, enum_name: str) -> str:
    name = _cut_enum_value_name(value_name, enum_name=enum_name)
    return f'k{names.to_pascal_case(name, to_lower=True)}'


def _cut_enum_value_name(value_name: str, *, enum_name: str) -> str:
    if value_name.startswith(upper_enum_name := names.to_upper_case(enum_name)):
        # ENUM_NAME_VALUE_NAME -> VALUE_NAME
        cut_name = value_name[len(upper_enum_name) :].removeprefix('_')
        if names.is_valid_id(cut_name):
            return cut_name
    return value_name


def parse_message(
    message: descriptor.Descriptor,
    *,
    plugin_options: options.PluginOptions,
) -> gen_node.TypeNode | None:
    if _is_map_entry(message):
        return None

    vanilla_type_name = type_mapping.parse_type_name(message)
    proto_file_descriptor: descriptor.FileDescriptor = message.file
    proto_file_name: str = proto_file_descriptor.name

    taken_member_names = _collect_taken_member_names(message)

    nested_types: list[gen_node.TypeNode] = []
    for nested_enum in typing.cast(list[descriptor.EnumDescriptor], message.enum_types):
        nested_types.append(parse_enum(nested_enum))
    for nested_message in typing.cast(list[descriptor.Descriptor], message.nested_types):
        if message_type := parse_message(nested_message, plugin_options=plugin_options):
            nested_types.append(message_type)

    nested_types = sort_dependencies.sort_types_topologically(nested_types)

    fields: list[gen_node.StructField] = []

    for field in typing.cast(list[descriptor.FieldDescriptor], message.fields):
        if oneof := typing.cast(descriptor.OneofDescriptor | None, field.containing_oneof):
            if not _is_synthetic_oneof(oneof):
                oneof_fields = typing.cast(list[descriptor.FieldDescriptor], oneof.fields)
                assert len(oneof_fields) >= 1
                if oneof_fields[0] is field:
                    fields.append(
                        parse_oneof(
                            oneof,
                            taken_member_names=taken_member_names,
                            nested_types_to_generate=nested_types,
                            plugin_options=plugin_options,
                        ),
                    )
                continue
        fields.append(parse_field(field, plugin_options=plugin_options))

    nested_types = sort_dependencies.sort_types_topologically(nested_types)

    result = gen_node.StructNode(
        vanilla_name=vanilla_type_name,
        proto_file=pathlib.Path(proto_file_name),
        nested_types=nested_types,
        fields=fields,
    )
    if result_enum := _try_transform_enum_wrapper(result):
        return result_enum

    return result


_SYNTHETIC_ONEOF_NAME_REGEX = re.compile(r'X*_\w*')


def _io_kind_read(field: descriptor.FieldDescriptor) -> io.ReadVanillaFieldKind:
    if typing.cast(int, field.label) == descriptor.FieldDescriptor.LABEL_OPTIONAL:
        if typing.cast(bool, field.has_presence):
            return io.ReadVanillaFieldKind.OPTIONAL

    return io.ReadVanillaFieldKind.OTHER


def _io_kind_write(field: descriptor.FieldDescriptor) -> io.WriteVanillaFieldKind:
    type_kind: int = field.type
    label = typing.cast(int, field.label)

    # Check repeated firstly. Also handle a repeated map entry.
    if label == descriptor.FieldDescriptor.LABEL_REPEATED:
        return io.WriteVanillaFieldKind.VECTOR_MAP_MESSAGE

    if type_kind in (descriptor.FieldDescriptor.TYPE_STRING, descriptor.FieldDescriptor.TYPE_BYTES):
        return io.WriteVanillaFieldKind.STRING

    if type_kind in type_mapping.PRIMITIVE_TYPES_TO_PROTOBUF_NAME or type_kind == descriptor.FieldDescriptor.TYPE_ENUM:
        return io.WriteVanillaFieldKind.OTHER

    if type_kind in (descriptor.FieldDescriptor.TYPE_MESSAGE, descriptor.FieldDescriptor.TYPE_GROUP):
        return io.WriteVanillaFieldKind.VECTOR_MAP_MESSAGE

    raise Exception('unreachable')


def _io_kind_by_field(field: descriptor.FieldDescriptor) -> io.IoKind:
    return io.IoKind(read=_io_kind_read(field), write=_io_kind_write(field))


def _io_kind_oneof() -> io.IoKind:
    return io.IoKind(read=io.ReadVanillaFieldKind.ONEOF, write=io.WriteVanillaFieldKind.ONEOF)


def _apply_options_to_field(
    field: descriptor.FieldDescriptor,
    struct_field: gen_node.StructField,
    *,
    plugin_options: options.PluginOptions,
) -> gen_node.StructField:
    message_type = _get_optional_message_type(field)
    if message_type:
        message_options = option_parsers.parse_message(message_type, plugin_options)
    else:
        message_options = None

    field_options = option_parsers.parse_field(field, plugin_options)

    should_box = field_options.indirect or (message_options and message_options.indirect)
    if should_box:
        gen_node.wrap_field_in_box(struct_field)

    return struct_field


def _is_synthetic_oneof(oneof: descriptor.OneofDescriptor) -> bool:
    # https://protobuf.com/docs/descriptors#synthetic-oneofs
    oneof_fields = typing.cast(list[descriptor.FieldDescriptor], oneof.fields)
    if len(oneof_fields) != 1:
        return False

    oneof_name = typing.cast(str, oneof.name)

    return _SYNTHETIC_ONEOF_NAME_REGEX.fullmatch(oneof_name) is not None


def _try_transform_enum_wrapper(struct: gen_node.StructNode) -> gen_node.EnumNode | None:
    # FooBarEnum { enum FooBar { ... } } -> FooBarEnum
    if len(struct.fields) != 0:
        return None

    if len(struct.nested_types) != 1:
        return None
    nested_type = struct.nested_types[0]

    if not isinstance(nested_type, gen_node.EnumNode):
        return None

    if struct.name.short_name != f'{nested_type.name.short_name}Enum':
        return None
    return gen_node.EnumNode(vanilla_name=struct.vanilla_name, proto_file=struct.proto_file, values=nested_type.values)


def _is_map_entry(message: descriptor.Descriptor) -> bool:
    # https://protobuf.com/docs/descriptors#map-fields
    options: Any = message.GetOptions()
    is_map_entry: bool = getattr(options, 'map_entry', False)
    return is_map_entry


def parse_field(
    field: descriptor.FieldDescriptor,
    *,
    plugin_options: options.PluginOptions,
    ignore_label: bool = False,
) -> gen_node.StructField:
    if map_field_type := _try_parse_map_field_type(field, plugin_options=plugin_options):
        parsed_type = map_field_type
        initializer = ''
    else:
        parsed_type = type_mapping.parse_type_reference(field, plugin_options=plugin_options)
        initializer = '' if isinstance(parsed_type, type_ref.UserverCodegenType) else '{}'
        if not ignore_label:
            parsed_type = type_mapping.handle_type_label(field, parsed_type)

    result_field = gen_node.StructField(
        short_name=names.escape_id(typing.cast(str, field.name)),
        field_type=parsed_type,
        initializer=initializer,
        number=typing.cast(int, field.number),
        oneof_fields=None,
        io_kinds=_io_kind_by_field(field),
    )
    return _apply_options_to_field(field, result_field, plugin_options=plugin_options)


def _try_parse_map_field_type(
    field: descriptor.FieldDescriptor,
    plugin_options: options.PluginOptions,
) -> type_ref.TypeReference | None:
    # https://protobuf.com/docs/descriptors#map-fields
    if typing.cast(int, field.type) != descriptor.FieldDescriptor.TYPE_MESSAGE:
        return None
    message_type = typing.cast(descriptor.Descriptor, field.message_type)
    if not _is_map_entry(message_type):
        return None

    fields_by_name = typing.cast(Mapping[str, descriptor.FieldDescriptor], message_type.fields_by_name)
    key_type = type_mapping.parse_type_reference(fields_by_name['key'], plugin_options=plugin_options)
    value_type = type_mapping.parse_type_reference(fields_by_name['value'], plugin_options=plugin_options)

    return type_ref_consts.make_hash_map(key_type, value_type)


def parse_oneof(
    oneof: descriptor.OneofDescriptor,
    taken_member_names: MutableSet[str],
    nested_types_to_generate: list[gen_node.TypeNode],
    plugin_options: options.PluginOptions,
) -> gen_node.StructField:
    oneof_options = option_parsers.parse_oneof(oneof, plugin_options)

    fields: list[gen_node.StructField] = []

    for field in typing.cast(list[descriptor.FieldDescriptor], oneof.fields):
        assert typing.cast(int, field.label) == descriptor.FieldDescriptor.LABEL_OPTIONAL
        fields.append(parse_field(field, plugin_options=plugin_options, ignore_label=True))

    containing_type_descriptor = typing.cast(descriptor.Descriptor, oneof.containing_type)
    containing_type_name = names.make_structs_type_name(type_mapping.parse_type_name(containing_type_descriptor))

    oneof_field_short_name = names.escape_id(typing.cast(str, oneof.name))
    oneof_type_short_name = oneof_options.generated_type_name or _synthesize_oneof_type_name(
        oneof_field_short_name,
        taken_member_names,
    )

    oneof_type_name = names.make_nested_type_name(containing_type_name, oneof_type_short_name)

    oneof_node = gen_node.OneofNode(name=oneof_type_name, fields=fields)
    nested_types_to_generate.append(oneof_node)

    type_reference = type_ref.UserverCodegenType(
        name=oneof_type_name,
        proto_file=type_mapping.parse_file_path(containing_type_descriptor),
    )

    return gen_node.StructField(
        short_name=oneof_field_short_name,
        field_type=type_reference,
        initializer='',
        number=None,
        oneof_fields=fields,
        io_kinds=_io_kind_oneof(),
    )


def _synthesize_oneof_type_name(oneof_field_short_name: str, taken_member_names: MutableSet[str]) -> str:
    if oneof_field_short_name[0].isupper():
        oneof_type_short_name = f'T{names.to_pascal_case(oneof_field_short_name)}'
    else:
        oneof_type_short_name = names.to_pascal_case(oneof_field_short_name)
    return _make_unique_member_name(oneof_type_short_name, taken_member_names)


def _collect_taken_member_names(message: descriptor.Descriptor) -> MutableSet[str]:
    result: MutableSet[str] = set()
    result.add(typing.cast(str, message.name))
    for nested_type in typing.cast(list[descriptor.Descriptor], message.nested_types):
        result.add(typing.cast(str, nested_type.name))
    for enum_type in typing.cast(list[descriptor.Descriptor], message.enum_types):
        result.add(typing.cast(str, enum_type.name))
    for neighbour_field in typing.cast(list[descriptor.FieldDescriptor], message.fields):
        result.add(typing.cast(str, neighbour_field.name))
    for oneof in typing.cast(list[descriptor.OneofDescriptor], message.oneofs):
        result.add(typing.cast(str, oneof.name))
    return result


def _make_unique_member_name(base_name: str, taken_member_names: MutableSet[str]) -> str:
    while base_name in taken_member_names:
        base_name = f'X{base_name}'
    taken_member_names.add(base_name)
    return base_name


def _get_optional_message_type(field: descriptor.FieldDescriptor) -> descriptor.Descriptor | None:
    type_kind: int = field.type
    if type_kind in (descriptor.FieldDescriptor.TYPE_MESSAGE, descriptor.FieldDescriptor.TYPE_GROUP):
        # Details on groups:
        # https://protobuf.com/docs/descriptors#groups
        message_type: descriptor.Descriptor = field.message_type
        return message_type
    return None
