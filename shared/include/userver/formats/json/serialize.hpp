#pragma once

/// @file userver/formats/json/serialize.hpp
/// @brief Parsers and serializers to/from string and stream

#include <iosfwd>
#include <string_view>

#include <userver/formats/json/value.hpp>

USERVER_NAMESPACE_BEGIN

namespace logging {
class LogHelper;
}  // namespace logging

namespace formats::json {

/// Parse JSON from string
formats::json::Value FromString(std::string_view doc);

/// Parse JSON from stream
formats::json::Value FromStream(std::istream& is);

/// Serialize JSON to stream
void Serialize(const formats::json::Value& doc, std::ostream& os);

/// Serialize JSON to string
std::string ToString(const formats::json::Value& doc);

/// Log JSON
logging::LogHelper& operator<<(logging::LogHelper&,
                               const formats::json::Value&);

/// Blocking operations that should not be used on main task processor after
/// startup
namespace blocking {
/// Read JSON from file
formats::json::Value FromFile(const std::string& path);
}  // namespace blocking

}  // namespace formats::json

USERVER_NAMESPACE_END
