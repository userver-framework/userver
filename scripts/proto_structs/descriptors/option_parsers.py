"""Parsers from Protobuf descriptor options to `models/options.py`."""

# Documentation on Protobuf descriptors and extensions:
# https://googleapis.dev/python/protobuf/latest/google/protobuf/descriptor.html
# https://googleapis.dev/python/protobuf/latest/google/protobuf/message.html

from collections.abc import Callable
from collections.abc import Mapping
import typing
from typing import Any
from typing import TypeVar

import google.protobuf.descriptor as descriptor_module
import google.protobuf.message as message_module
from userver.structs import annotations_pb2  # pyright: ignore

from proto_structs.models import options


def parse_file(file: descriptor_module.FileDescriptor, /, defaults: options.PluginOptions) -> options.FileOptions:
    """Parses file options from descriptor, merging with options manually passed to this protoc plugin."""
    return options.FileOptions()


def parse_message(message: descriptor_module.Descriptor, /, defaults: options.PluginOptions) -> options.MessageOptions:
    """Parses message options from descriptor, merging with options manually passed to this protoc plugin."""
    options_raw = _get_option(message, annotations_pb2.message, annotations_pb2.MessageAnnotations)  # pyright: ignore

    indirect_raw = typing.cast(bool, options_raw.indirect)
    # Add new options here.

    message_full_name: str = message.full_name
    message_defaults = defaults.message_options.get(message_full_name, options.MessageOptions())

    return options.MessageOptions(
        indirect=indirect_raw or message_defaults.indirect,
        # Add new options here.
    )


def parse_oneof(oneof: descriptor_module.OneofDescriptor, /, defaults: options.PluginOptions) -> options.OneofOptions:
    """Parses oneof options from descriptor, merging with options manually passed to this protoc plugin."""
    options_raw = _get_option(oneof, annotations_pb2.oneof, annotations_pb2.OneofAnnotations)  # pyright: ignore

    generated_type_name_raw = typing.cast(str, options_raw.generated_type_name)
    # Add new options here.

    oneof_full_name: str = oneof.full_name
    oneof_defaults = defaults.oneof_options.get(oneof_full_name, options.OneofOptions())

    return options.OneofOptions(
        generated_type_name=generated_type_name_raw or oneof_defaults.generated_type_name,
        # Add new options here.
    )


def parse_field(field: descriptor_module.FieldDescriptor, /, defaults: options.PluginOptions) -> options.FieldOptions:
    """Parses field options from descriptor, merging with options manually passed to this protoc plugin."""
    options_raw = _get_option(field, annotations_pb2.field, annotations_pb2.FieldAnnotations)  # pyright: ignore

    indirect_raw = typing.cast(bool, options_raw.indirect)  # pyright: ignore
    # Add new options here.

    field_full_name: str = field.full_name
    field_defaults = defaults.field_options.get(field_full_name, options.FieldOptions())

    return options.FieldOptions(
        indirect=indirect_raw or field_defaults.indirect,
        # Add new options here.
    )


TExtension = TypeVar('TExtension', bound=message_module.Message)


def _get_option(  # noqa: UP047
    any_descriptor: descriptor_module.DescriptorBase,
    /,
    extension_handle: Any,
    extension_type: type[TExtension],
) -> TExtension:
    options_message = typing.cast(message_module.Message, any_descriptor.GetOptions())
    has_extension_func = typing.cast(Callable[[Any], bool], options_message.HasExtension)
    has_extension = has_extension_func(extension_handle)
    if has_extension:
        extensions: Mapping[Any, Any] = getattr(options_message, 'Extensions')  # noqa: B009
        extension: TExtension = extensions[extension_handle]
        return extension
    else:
        return extension_type()
