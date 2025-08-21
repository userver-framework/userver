"""Parsers from Protobuf descriptors to TypeReference."""

from collections.abc import Sequence
import pathlib
import typing
from typing import List
from typing import Mapping
from typing import Optional
from typing import Union

import google.protobuf.descriptor as descriptor

from proto_structs.models import includes
from proto_structs.models import names
from proto_structs.models import type_ref

TypeDescriptor = Union[descriptor.Descriptor, descriptor.EnumDescriptor]

_BUNDLE_HPP = includes.BUNDLE_STRUCTS_HPP

BUILTIN_TYPES: Mapping[int, type_ref.TypeReference] = {
    descriptor.FieldDescriptor.TYPE_BOOL: type_ref.KeywordType(full_cpp_name='bool'),
    descriptor.FieldDescriptor.TYPE_FLOAT: type_ref.KeywordType(full_cpp_name='float'),
    descriptor.FieldDescriptor.TYPE_DOUBLE: type_ref.KeywordType(full_cpp_name='double'),
    descriptor.FieldDescriptor.TYPE_STRING: type_ref.BuiltinType(full_cpp_name='std::string', include=_BUNDLE_HPP),
    descriptor.FieldDescriptor.TYPE_BYTES: type_ref.BuiltinType(full_cpp_name='std::string', include=_BUNDLE_HPP),
    descriptor.FieldDescriptor.TYPE_INT32: type_ref.BuiltinType(full_cpp_name='std::int32_t', include=_BUNDLE_HPP),
    descriptor.FieldDescriptor.TYPE_INT64: type_ref.BuiltinType(full_cpp_name='std::int64_t', include=_BUNDLE_HPP),
    descriptor.FieldDescriptor.TYPE_UINT32: type_ref.BuiltinType(full_cpp_name='std::uint32_t', include=_BUNDLE_HPP),
    descriptor.FieldDescriptor.TYPE_UINT64: type_ref.BuiltinType(full_cpp_name='std::uint64_t', include=_BUNDLE_HPP),
    descriptor.FieldDescriptor.TYPE_SINT32: type_ref.BuiltinType(full_cpp_name='std::int32_t', include=_BUNDLE_HPP),
    descriptor.FieldDescriptor.TYPE_SINT64: type_ref.BuiltinType(full_cpp_name='std::int64_t', include=_BUNDLE_HPP),
    descriptor.FieldDescriptor.TYPE_FIXED32: type_ref.BuiltinType(full_cpp_name='std::uint32_t', include=_BUNDLE_HPP),
    descriptor.FieldDescriptor.TYPE_FIXED64: type_ref.BuiltinType(full_cpp_name='std::uint64_t', include=_BUNDLE_HPP),
    descriptor.FieldDescriptor.TYPE_SFIXED32: type_ref.BuiltinType(full_cpp_name='std::int32_t', include=_BUNDLE_HPP),
    descriptor.FieldDescriptor.TYPE_SFIXED64: type_ref.BuiltinType(full_cpp_name='std::int64_t', include=_BUNDLE_HPP),
}

_OPTIONAL_TEMPLATE = type_ref.BuiltinType(full_cpp_name='std::optional', include=_BUNDLE_HPP)
_VECTOR_TEMPLATE = type_ref.BuiltinType(full_cpp_name='std::vector', include=_BUNDLE_HPP)


def parse_enum_reference(field_type: descriptor.EnumDescriptor) -> type_ref.TypeReference:
    # TODO
    return type_ref.BuiltinType(full_cpp_name='std::monostate', include=_BUNDLE_HPP)


def parse_struct_reference(field_type: descriptor.Descriptor) -> type_ref.TypeReference:
    # TODO
    return type_ref.BuiltinType(full_cpp_name='std::monostate', include=_BUNDLE_HPP)


def handle_type_label(
    field: descriptor.FieldDescriptor,
    parsed_type: type_ref.TypeReference,
) -> type_ref.TypeReference:
    label = typing.cast(int, field.label)
    if label == descriptor.FieldDescriptor.LABEL_OPTIONAL:
        if typing.cast(bool, field.has_presence):
            return type_ref.TemplateType(template=_OPTIONAL_TEMPLATE, template_args=(parsed_type,))
        else:
            return parsed_type
    if label == descriptor.FieldDescriptor.LABEL_REQUIRED:
        return parsed_type
    if label == descriptor.FieldDescriptor.LABEL_REPEATED:
        return type_ref.TemplateType(template=_VECTOR_TEMPLATE, template_args=(parsed_type,))
    raise RuntimeError(f'Unknown type label {label}')


def parse_type_name(proto_type: TypeDescriptor) -> names.TypeName:
    return names.make_escaped_type_name(
        package=typing.cast(str, proto_type.file.package),
        outer_type_names=_get_outer_structs_names(proto_type),
        short_name=typing.cast(str, proto_type.name),
    )


def _get_outer_structs_names(proto_type: TypeDescriptor) -> Sequence[str]:
    names_list: List[str] = []
    parent_type = proto_type
    while parent_type := typing.cast(Optional[descriptor.Descriptor], parent_type.containing_type):
        names_list.append(typing.cast(str, parent_type.name))
    return list(reversed(names_list))


def parse_include(proto_type: TypeDescriptor) -> str:
    file = typing.cast(descriptor.FileDescriptor, proto_type.file)
    file_name = typing.cast(str, file.name)
    return str(includes.proto_path_to_structs_path(pathlib.Path(file_name), ext='hpp'))
