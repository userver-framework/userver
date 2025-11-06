"""Walks through proto-schema-parser AST nodes to collect includes required for generating types."""

# Synopsis of proto-schema-parser AST nodes:
# https://github.com/criccomini/proto-schema-parser/blob/main/proto_schema_parser/ast.py

from collections.abc import Iterable
from typing import Any
from typing import List
from typing import Optional
from typing import Union

from proto_schema_parser import ast

from proto_structs.models import gen_node
from proto_structs.models import includes
from proto_structs.models import includes_bundles
from proto_structs.models import options
from proto_structs.models import type_overrides
from proto_structs.models import type_ref_consts


def collect(*, file_ast: ast.File, plugin_options: Optional[Any]) -> List[str]:
    """
    Recursively collect all includes that will be present in the generated structs hpp or cpp file.
    Includes to other structs or vanilla protobuf files are NOT accounted for.
    The result does not include proto structs bundles, which are always present.
    The result is always sorted.
    """
    parsed_options = options.PluginOptions(**plugin_options) if plugin_options else options.PluginOptions()
    includes_set = set(include.path for include in collect_file(file_ast, plugin_options=parsed_options))
    includes_set.difference_update(includes_bundles.BUNDLE_HPP)
    includes_set.difference_update(includes_bundles.BUNDLE_CPP)
    return sorted(includes_set)


def collect_file(file: ast.File, /, *, plugin_options: options.PluginOptions) -> Iterable[includes.Include]:
    for element in file.file_elements:
        if isinstance(element, ast.Message):
            yield from collect_message(element, plugin_options=plugin_options)
        elif isinstance(element, ast.Enum):
            yield from collect_enum(element, plugin_options=plugin_options)
        elif isinstance(element, ast.Service):
            yield from collect_service(element, plugin_options=plugin_options)


def collect_enum(enum: ast.Enum, /, *, plugin_options: options.PluginOptions) -> Iterable[includes.Include]:
    yield includes.Include(path='cstdint', kind=includes.IncludeKind.FOR_HPP)
    yield includes.Include(path='limits', kind=includes.IncludeKind.FOR_HPP)


def collect_message(
    message: Union[ast.Message, ast.Group], /, *, plugin_options: options.PluginOptions
) -> Iterable[includes.Include]:
    yield from gen_node.COMMON_STRUCT_INCLUDES

    for element in message.elements:
        if isinstance(element, ast.Field):
            yield from collect_field(element, plugin_options=plugin_options)
        elif isinstance(element, (ast.Group, ast.Message)):
            yield from collect_message(element, plugin_options=plugin_options)
        elif isinstance(element, ast.OneOf):
            yield from collect_oneof(element, plugin_options=plugin_options)
        elif isinstance(element, ast.Enum):
            yield from collect_enum(element, plugin_options=plugin_options)
        elif isinstance(element, ast.MapField):
            yield from collect_map_field(element, plugin_options=plugin_options)


def collect_field(field: ast.Field, /, *, plugin_options: options.PluginOptions) -> Iterable[includes.Include]:
    yield from _collect_field_type(field.type, plugin_options=plugin_options)
    yield from _collect_field_cardinality(field)


def _collect_field_type(
    proto_type_name: str, /, *, plugin_options: options.PluginOptions
) -> Iterable[includes.Include]:
    if primitive_type := type_ref_consts.PRIMITIVE_TYPES.get(proto_type_name):
        yield from primitive_type.collect_includes()
        return

    if override := type_overrides.get_type_override(proto_type_name=proto_type_name, plugin_options=plugin_options):
        yield from override.collect_includes()
        return

    # Else, this is a code-generated type.

    # Whether `utils::Box` is required depends on the existence of cyclic dependencies. For simplicity, always require.
    yield from type_ref_consts.BOX_TEMPLATE.collect_includes()


def _collect_field_cardinality(field: ast.Field) -> Iterable[includes.Include]:
    if field.cardinality == ast.FieldCardinality.REPEATED:
        yield from type_ref_consts.VECTOR_TEMPLATE.collect_includes()
        return

    # Whether `std::optional` is actually added depends on complex conditions, so just always require it.
    yield from type_ref_consts.OPTIONAL_TEMPLATE.collect_includes()


def collect_oneof(oneof: ast.OneOf, /, *, plugin_options: options.PluginOptions) -> Iterable[includes.Include]:
    yield from gen_node.COMMON_ONEOF_INCLUDES

    for element in oneof.elements:
        if isinstance(element, ast.Field):
            yield from collect_field(element, plugin_options=plugin_options)
        elif isinstance(element, ast.Group):
            yield from collect_message(element, plugin_options=plugin_options)


def collect_map_field(field: ast.MapField, /, *, plugin_options: options.PluginOptions) -> Iterable[includes.Include]:
    yield from type_ref_consts.HASH_MAP_TEMPLATE.collect_includes()
    yield from _collect_map_key_type(field)
    yield from _collect_map_value_type(field, plugin_options=plugin_options)


def _collect_map_key_type(field: ast.MapField) -> Iterable[includes.Include]:
    yield from type_ref_consts.PRIMITIVE_TYPES[field.key_type].collect_includes()


def _collect_map_value_type(field: ast.MapField, plugin_options: options.PluginOptions) -> Iterable[includes.Include]:
    yield from _collect_field_type(field.value_type, plugin_options=plugin_options)


def collect_service(service: ast.Service, /, *, plugin_options: options.PluginOptions) -> Iterable[includes.Include]:
    for element in service.elements:
        if isinstance(element, ast.Method):
            yield from collect_method(element, plugin_options=plugin_options)


def collect_method(method: ast.Method, /, *, plugin_options: options.PluginOptions) -> Iterable[includes.Include]:
    yield from _collect_method_input_type(method, plugin_options=plugin_options)
    yield from _collect_method_output_type(method, plugin_options=plugin_options)


def _collect_method_input_type(
    method: ast.Method, /, *, plugin_options: options.PluginOptions
) -> Iterable[includes.Include]:
    yield from _collect_field_type(method.input_type.type, plugin_options=plugin_options)


def _collect_method_output_type(
    method: ast.Method, /, *, plugin_options: options.PluginOptions
) -> Iterable[includes.Include]:
    yield from _collect_field_type(method.output_type.type, plugin_options=plugin_options)
