#pragma once

#include <iosfwd>

#include <formats/json/value.hpp>

namespace formats {
namespace json {

formats::json::Value FromString(const std::string& doc);
formats::json::Value FromStream(std::istream& is);

void Serialize(const formats::json::Value& doc, std::ostream& os);
std::string ToString(const formats::json::Value& doc);

namespace blocking {
formats::json::Value FromFile(const std::string& path);
}

}  // namespace json
}  // namespace formats
