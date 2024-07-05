#include "userver/logging/json_string.hpp"

#include <userver/logging/log.hpp>
#include <userver/utils/assert.hpp>

#include <algorithm>

USERVER_NAMESPACE_BEGIN

namespace logging {

namespace {
constexpr std::string_view kNull = "null";

bool DebugValid(std::string_view json) {
  try {
    formats::json::FromString(json);
  } catch (const formats::json::ParseException& error) {
    LOG_ERROR() << "Invalid json: " << json << ", error: " << error;
    return false;
  }
  return true;
}

}  // namespace

JsonString::JsonString(const formats::json::Value& value)
    : json_{ToString(value)} {
  // ToString builds one line string by RapidJson
}

JsonString::JsonString(std::string json) noexcept : json_{std::move(json)} {
  UASSERT(DebugValid(json_));

  // Remove extra new lines from user provided json string
  json_.erase(std::remove_if(json_.begin(), json_.end(),
                             [](auto ch) { return ch == '\n' || ch == '\r'; }),
              json_.end());
}

std::string_view JsonString::Value() const {
  if (json_.empty()) {
    return kNull;
  }
  return json_;
}

}  // namespace logging

USERVER_NAMESPACE_END
