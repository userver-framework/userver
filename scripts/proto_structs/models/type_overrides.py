"""
Protobuf expects us to translate primitive types to primitive C++ types and use generated types for the rest.
However, it's more helpful to replace some well-known message types with custom C++ types,
e.g. `std::chrono::time_point` or `decimal64::Decimal`.
This module deals with such replacements.
"""

from proto_structs.models import options
from proto_structs.models import type_ref
from proto_structs.models import type_ref_consts


def get_type_override(
    *,
    proto_type_name: str,
    plugin_options: options.PluginOptions,
) -> type_ref.TypeReference | None:
    """
    If we wish to use a custom C++ type for the referenced Protobuf type, returns it. Otherwise, returns `None`.
    `proto_type_name` is the full Protobuf type name in the form `package.of.Type.Nested`.
    """
    return type_ref_consts.WELL_KNOWN_TYPES.get(proto_type_name)
