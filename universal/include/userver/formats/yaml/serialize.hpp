#pragma once

/// @file userver/formats/yaml/serialize.hpp
/// @brief Parsers and serializers to/from string and stream

#include <iosfwd>

#include <userver/formats/json_fwd.hpp>
#include <userver/formats/yaml/value.hpp>

USERVER_NAMESPACE_BEGIN

namespace formats::parse {

/// @brief Converts a YAML value to JSON format
///
/// @throws formats::yaml::Exception if the YAML value is missing or contains
///         an unknown node type that cannot be converted to JSON
formats::json::Value Parse(const formats::yaml::Value& yaml, formats::parse::To<formats::json::Value>);

/// @brief Converts a JSON value to YAML format
///
/// @throws formats::json::Exception if the JSON value is missing or contains
///         an unknown node type that cannot be converted to YAML
formats::yaml::Value Parse(const formats::json::Value& json, formats::parse::To<formats::yaml::Value>);

}  // namespace formats::parse

namespace formats::yaml {

/// Parse YAML from string
formats::yaml::Value FromString(const std::string& doc);

/// Parse YAML from stream
formats::yaml::Value FromStream(std::istream& is);

/// Serialize YAML to stream
void Serialize(const formats::yaml::Value& doc, std::ostream& os);

/// Serialize YAML to string
std::string ToString(const formats::yaml::Value& doc);

/// @brief Blocking operations that should not be used on main task processor after startup
namespace blocking {
/// @brief Read YAML from file
/// @see formats::yaml::FromFile
formats::yaml::Value FromFile(const std::string& path);
}  // namespace blocking

namespace impl {
formats::yaml::Value FromStringAllowRepeatedKeys(const std::string& doc);
}  // namespace impl

}  // namespace formats::yaml

USERVER_NAMESPACE_END
