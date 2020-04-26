#pragma once

#include <iosfwd>
#include <string_view>

#include <formats/json/value.hpp>

namespace formats::json {

formats::json::Value FromString(std::string_view doc);
formats::json::Value FromStream(std::istream& is);

void Serialize(const formats::json::Value& doc, std::ostream& os);
std::string ToString(const formats::json::Value& doc);

namespace blocking {
formats::json::Value FromFile(const std::string& path);
}  // namespace blocking

}  // namespace formats::json
