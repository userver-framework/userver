#pragma once

#include <iosfwd>

#include <formats/json/value.hpp>
#include <utils/string_view.hpp>

namespace formats {
namespace json {

formats::json::Value FromString(utils::string_view doc);
formats::json::Value FromStream(std::istream& is);

void Serialize(const formats::json::Value& doc, std::ostream& os);
std::string ToString(const formats::json::Value& doc);

namespace blocking {
formats::json::Value FromFile(const std::string& path);
}  // namespace blocking

}  // namespace json
}  // namespace formats
