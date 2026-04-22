#pragma once

/// @file userver/protobuf/json/convert_options.hpp
/// @brief Protobuf messages to/from JSON `ValueBuilder`/`Value` conversion options.

USERVER_NAMESPACE_BEGIN

namespace protobuf::json {

/// @brief Options which affect how JSON `Value` is converted to a protobuf message.
struct ParseOptions {
    /// @brief Ignore unknown JSON fields and enum value names.
    /// If not set, conversion of a JSON object containing unknown field (i.e. field that is not mapped to any protobuf
    /// message field) or unknown enum value name will fail.
    bool ignore_unknown_fields = false;
};

/// @brief Options which affect how protobuf message is converted to a JSON `ValueBuilder`/`Value`.
struct PrintOptions {
    /// @brief Always output protobuf message fields that do not have explicit presence.
    /// This fields are:
    /// * implicit presence fields set to default value
    /// * empty repeated/map fields
    bool always_print_fields_with_no_presence = false;

    /// @brief Output enum values as integers, not as their string names.
    bool always_print_enums_as_ints = false;

    /// @brief Output field names exactly how specified in the proto file (usually *lower_snake_case*).
    /// By default, protobuf message field names are converted to `lowerCamelCase` format.
    /// @note If protobuf message field has `json_name` option attached, then it's value is used for the JSON key
    ///       name and this option does not have effect.
    /// @warning According to protobuf rules this option **does not have effect** on the field names specified in the
    ///          `google.protobuf.FieldMask` paths during conversion. I.e., field names inside `FieldMask` are always
    ///          encoded in `lowerCamelCase` no matter whether `preserve_proto_field_names` is on or off. Moreover,
    ///          `json_name` option is also not taken into account for `FieldMask` paths.
    bool preserve_proto_field_names = false;
};

}  // namespace protobuf::json

USERVER_NAMESPACE_END
