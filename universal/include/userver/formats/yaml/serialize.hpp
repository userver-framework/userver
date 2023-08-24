#pragma once

/// @file userver/formats/yaml/serialize.hpp
/// @brief Parsers and serializers to/from string and stream

#include <iosfwd>

#include <userver/formats/yaml/value.hpp>

USERVER_NAMESPACE_BEGIN

namespace formats::yaml {

/// Parse YAML from string
formats::yaml::Value FromString(const std::string& doc);

/// Parse YAML from stream
formats::yaml::Value FromStream(std::istream& is);

/// Serialize YAML to stream
void Serialize(const formats::yaml::Value& doc, std::ostream& os);

/// Serialize YAML to string
std::string ToString(const formats::yaml::Value& doc);

/// Blocking operations that should not be used on main task processor after
/// startup
namespace blocking {
/// Read YAML from file
formats::yaml::Value FromFile(const std::string& path);
}  // namespace blocking

}  // namespace formats::yaml

USERVER_NAMESPACE_END
