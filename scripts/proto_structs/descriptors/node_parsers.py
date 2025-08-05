"""Parsers from Protobuf descriptors to CodegenNode."""

from collections.abc import Sequence
import pathlib
import typing
from typing import Dict
from typing import List
from typing import Union

import google.protobuf.descriptor as descriptor

from proto_structs.models import gen_node
from proto_structs.models import names

TypeDescriptor = Union[descriptor.Descriptor, descriptor.EnumDescriptor]


def parse_file(file: descriptor.FileDescriptor) -> gen_node.File:
    namespace_members: List[gen_node.CodegenNode] = []
    for message in typing.cast(Dict[str, descriptor.Descriptor], file.message_types_by_name).values():
        namespace_members.append(parse_message(message))
    structs_namespace = gen_node.NamespaceNode.make_for_structs(
        typing.cast(str, file.package),
        children=namespace_members,
    )
    return gen_node.File(
        proto_relative_path=pathlib.Path(typing.cast(str, file.name)),
        children=(structs_namespace,),
    )


def parse_message(message: descriptor.Descriptor) -> gen_node.StructNode:
    nested_types: List[gen_node.TypeNode] = []
    for nested_message in typing.cast(List[descriptor.Descriptor], message.nested_types):
        nested_types.append(parse_message(nested_message))

    return gen_node.StructNode(name=_get_name(message), nested_types=nested_types)


def _get_outer_structs_names(proto_type: TypeDescriptor) -> Sequence[str]:
    names_list: List[str] = []
    while proto_type := typing.cast(descriptor.Descriptor, proto_type.containing_type):
        names_list.append(typing.cast(str, proto_type.name))
    return list(reversed(names_list))


def _get_name(proto_type: TypeDescriptor) -> names.TypeName:
    return names.make_type_name(
        package=typing.cast(str, proto_type.file.package),
        outer_type_names=_get_outer_structs_names(proto_type),
        short_name=typing.cast(str, proto_type.name),
    )
