"""
Options for proto-structs to generate.
These can come from the proto file, as well as passed to the protoc plugin by the build system.
"""

import dataclasses
import json
import pathlib
import re
from typing import Any
from typing import TypeAlias
from typing import TypeVar

import dataclasses_json

ShortName: TypeAlias = str
FullName: TypeAlias = str

_SHORT_NAME_REGEX = re.compile(r'^[a-zA-Z_]\w*$')
_FULL_NAME_REGEX = re.compile(r'^[a-zA-Z_]\w*(\.[a-zA-Z_]\w*)*$')


class ModelBase(dataclasses_json.DataClassJsonMixin):
    """Base class for options models."""

    dataclass_json_config = dataclasses_json.config(undefined=dataclasses_json.Undefined.RAISE)  # pyright: ignore


@dataclasses.dataclass
class FileOptions(ModelBase):
    """Options that apply to all definitions or types in a file."""


@dataclasses.dataclass
class MessageOptions(ModelBase):
    """Options that apply to struct definition or to all fields typed with this struct."""

    #: True, if the struct's type should always be wrapped in `utils::Box`.
    indirect: bool = False


@dataclasses.dataclass
class OneofOptions(ModelBase):
    """Options that apply to oneof definition."""

    #: Overrides the generated name of the C++ class to represent oneof.
    generated_type_name: ShortName | None = None

    def __post_init__(self) -> None:
        if self.generated_type_name is not None and not _SHORT_NAME_REGEX.match(self.generated_type_name):
            raise ValueError(f'Invalid generated_type_name: {self.generated_type_name}')


@dataclasses.dataclass
class FieldOptions(dataclasses_json.DataClassJsonMixin):
    """Options that apply to message fields."""

    #: True, if the field's type should be wrapped in `utils::Box`.
    indirect: bool = False


@dataclasses.dataclass
class PluginOptions(dataclasses_json.DataClassJsonMixin):
    """Options root."""

    #: Proto file path (as in `import`) `full/file/name.proto` -> options.
    file_options: dict[str, FileOptions] = dataclasses.field(default_factory=dict)

    #: Full message name in format `full.package.Message.SubMessage` -> options.
    message_options: dict[FullName, MessageOptions] = dataclasses.field(default_factory=dict)

    #: Full oneof name in format `full.package.Message.Submessage.oneof_name` -> options.
    oneof_options: dict[FullName, OneofOptions] = dataclasses.field(default_factory=dict)

    #: Full field name in format `full.package.Message.SubMessage.field` -> options.
    field_options: dict[FullName, FieldOptions] = dataclasses.field(default_factory=dict)

    def __post_init__(self) -> None:
        for keys in (self.message_options.keys(), self.oneof_options.keys(), self.field_options.keys()):
            for full_name in keys:
                if not _FULL_NAME_REGEX.match(full_name):
                    raise ValueError(f'Invalid Protobuf full name: {full_name}')


# Some options for common plugins are there to avoid requiring passing the options in cmake for common library schemas.
_DEFAULT_PLUGIN_OPTIONS = PluginOptions(
    field_options={
        # Built-in types.
        'google.protobuf.Struct.fields': FieldOptions(indirect=True),
        # Well-known types.
        'google.api.BackendRule.overrides_by_request_protocol': FieldOptions(indirect=True),
        # Protovalidate types.
        'buf.validate.RepeatedRules.items': FieldOptions(indirect=True),
        'buf.validate.MapRules.keys': FieldOptions(indirect=True),
        'buf.validate.MapRules.values': FieldOptions(indirect=True),
    },
)


def _merge_dicts_recursively(*, into: dict[str, Any], overrides: dict[str, Any]) -> None:
    for key, value in overrides.items():
        if key in into and isinstance(into[key], dict) and isinstance(value, dict):
            sub_into: dict[str, Any] = into[key]
            sub_overrides: dict[str, Any] = value
            _merge_dicts_recursively(into=sub_into, overrides=sub_overrides)
        else:
            into[key] = value


T = TypeVar('T', bound=dataclasses_json.DataClassJsonMixin)


def merge_models(base: T, *, overrides: T) -> T:
    """Merge two pydantic models recursively."""
    assert base.__class__ == overrides.__class__
    merged: dict[str, Any] = base.to_dict()  # pyright: ignore
    overrides_dict: dict[str, Any] = overrides.to_dict()  # pyright: ignore
    _merge_dicts_recursively(into=merged, overrides=overrides_dict)
    return base.__class__.from_dict(merged)  # pyright: ignore


def load_plugin_options(file_path: pathlib.Path | None) -> PluginOptions:
    """Load `PluginOptions` from file."""
    if not file_path:
        return _DEFAULT_PLUGIN_OPTIONS

    with open(file_path, 'r', encoding='utf-8') as file:
        options = PluginOptions.from_dict(json.load(file))  # pyright: ignore
    return merge_models(_DEFAULT_PLUGIN_OPTIONS, overrides=options)
