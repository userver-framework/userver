"""Walks through proto-schema-parser AST nodes to collect includes required for generating types."""

# Synopsis of proto-schema-parser AST nodes:
# https://github.com/criccomini/proto-schema-parser/blob/main/proto_schema_parser/ast.py

from collections.abc import Iterable
import dataclasses
from typing import Any

from proto_schema_parser import ast

from proto_structs.bundles import includes_bundles
from proto_structs.models import gen_node
from proto_structs.models import includes
from proto_structs.models import options
from proto_structs.models import type_overrides
from proto_structs.models import type_ref_consts


def collect(*, file_ast: ast.File, plugin_options: Any | None) -> list[str]:
    """
    Recursively collect all includes that will be present in the generated structs hpp or cpp file.
    Includes to other structs or vanilla protobuf files are NOT accounted for.
    The result does not include proto structs bundles, which are always present.
    The result is always sorted.
    """
    parsed_options = options.PluginOptions(**plugin_options) if plugin_options else options.PluginOptions()
    includes_set = {include.path for include in collect_file(file_ast, plugin_options=parsed_options)}
    includes_set.difference_update(includes_bundles.bundle_hpp())
    includes_set.difference_update(includes_bundles.bundle_cpp())
    return sorted(includes_set)


@dataclasses.dataclass(frozen=True)
class FileContext:
    syntax: str
    package: str
    plugin_options: options.PluginOptions


def collect_file(file: ast.File, /, *, plugin_options: options.PluginOptions) -> Iterable[includes.Include]:
    package: str | None = None
    for element in file.file_elements:
        if isinstance(element, ast.Package):
            assert package is None
            package = element.name
    context = FileContext(syntax=file.syntax or 'proto2', package=package or '', plugin_options=plugin_options)

    for element in file.file_elements:
        if isinstance(element, ast.Message):
            yield from collect_message(element, context=context)
        elif isinstance(element, ast.Enum):
            yield from collect_enum(element, context=context)
        elif isinstance(element, ast.Service):
            yield from collect_service(element, context=context)


def collect_enum(enum: ast.Enum, /, *, context: FileContext) -> Iterable[includes.Include]:
    yield includes.Include(path='cstdint', kind=includes.IncludeKind.FOR_HPP)
    yield includes.Include(path='limits', kind=includes.IncludeKind.FOR_HPP)


def collect_message(message: ast.Message | ast.Group, /, *, context: FileContext) -> Iterable[includes.Include]:
    yield from gen_node.COMMON_STRUCT_INCLUDES

    for element in message.elements:
        if isinstance(element, ast.Field):
            yield from collect_field(element, context=context)
        elif isinstance(element, (ast.Group, ast.Message)):
            yield from collect_message(element, context=context)
        elif isinstance(element, ast.OneOf):
            yield from collect_oneof(element, context=context)
        elif isinstance(element, ast.Enum):
            yield from collect_enum(element, context=context)
        elif isinstance(element, ast.MapField):
            yield from collect_map_field(element, context=context)


def collect_field(field: ast.Field, /, *, context: FileContext) -> Iterable[includes.Include]:
    yield from _collect_field_type(field.type, context=context)
    yield from _collect_field_cardinality(field)


# Example: contains 'google.protobuf' because of 'google.protobuf.Timestamp' in `WELL_KNOWN_TYPES`.
_WELL_KNOWN_PACKAGES = {type_name.rsplit('.', 1)[0] for type_name in type_ref_consts.WELL_KNOWN_TYPES.keys()}


def _collect_field_type(proto_type_name: str, /, *, context: FileContext) -> Iterable[includes.Include]:
    if primitive_type := type_ref_consts.PRIMITIVE_TYPES.get(proto_type_name):
        yield from primitive_type.collect_includes()
        return

    if override := type_overrides.get_type_override(
        proto_type_name=proto_type_name,
        plugin_options=context.plugin_options,
    ):
        yield from override.collect_includes()
        return

    # For protobuf modules that define well-known types, they may refer to them without package.
    if context.package in _WELL_KNOWN_PACKAGES:
        if override := type_overrides.get_type_override(
            proto_type_name=f'{context.package}.{proto_type_name}',
            plugin_options=context.plugin_options,
        ):
            yield from override.collect_includes()
            return

    # Else, this is a code-generated type.

    # Whether `utils::Box` is required depends on the existence of cyclic dependencies. For simplicity, always require.
    yield from type_ref_consts.BOX_TEMPLATE.collect_includes()
    # Whether `UnbreakableDependencyCycle` is required depends on complex cycles. For simplicity, always require.
    yield from type_ref_consts.UNBREAKABLE_DEPENDENCY_CYCLE.collect_includes()


def _collect_field_cardinality(field: ast.Field) -> Iterable[includes.Include]:
    if field.cardinality == ast.FieldCardinality.REPEATED:
        yield from type_ref_consts.VECTOR_TEMPLATE.collect_includes()
        return

    # Whether `std::optional` is actually added depends on complex conditions, so just always require it.
    yield from type_ref_consts.OPTIONAL_TEMPLATE.collect_includes()


def collect_oneof(oneof: ast.OneOf, /, *, context: FileContext) -> Iterable[includes.Include]:
    yield from gen_node.COMMON_ONEOF_INCLUDES

    for element in oneof.elements:
        if isinstance(element, ast.Field):
            yield from collect_field(element, context=context)
        elif isinstance(element, ast.Group):
            yield from collect_message(element, context=context)


def collect_map_field(field: ast.MapField, /, *, context: FileContext) -> Iterable[includes.Include]:
    yield from type_ref_consts.HASH_MAP_TEMPLATE.collect_includes()
    yield from _collect_map_key_type(field)
    yield from _collect_map_value_type(field, context=context)


def _collect_map_key_type(field: ast.MapField) -> Iterable[includes.Include]:
    yield from type_ref_consts.PRIMITIVE_TYPES[field.key_type].collect_includes()


def _collect_map_value_type(field: ast.MapField, context: FileContext) -> Iterable[includes.Include]:
    yield from _collect_field_type(field.value_type, context=context)


def collect_service(service: ast.Service, /, *, context: FileContext) -> Iterable[includes.Include]:
    for element in service.elements:
        if isinstance(element, ast.Method):
            yield from collect_method(element, context=context)


def collect_method(method: ast.Method, /, *, context: FileContext) -> Iterable[includes.Include]:
    yield from _collect_method_input_type(method, context=context)
    yield from _collect_method_output_type(method, context=context)


def _collect_method_input_type(method: ast.Method, /, *, context: FileContext) -> Iterable[includes.Include]:
    yield from _collect_field_type(method.input_type.type, context=context)


def _collect_method_output_type(method: ast.Method, /, *, context: FileContext) -> Iterable[includes.Include]:
    yield from _collect_field_type(method.output_type.type, context=context)
