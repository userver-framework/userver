#include <userver/logging/json_string.hpp>

#include <algorithm>

#include <userver/formats/json/serialize.hpp>
#include <userver/formats/json/string_builder.hpp>
#include <userver/formats/json/value.hpp>
#include <userver/logging/log.hpp>
#include <userver/utils/assert.hpp>

USERVER_NAMESPACE_BEGIN

namespace logging {

namespace {

constexpr std::string_view kNull = "null";

}  // namespace

JsonString::JsonString(const formats::json::Value& value) : json_{ToString(value)} {
    // ToString builds one line string with RapidJson
    UASSERT(json_.find_first_of("\n\r") == std::string::npos);
}

JsonString::JsonString(std::string json) noexcept : json_{std::move(json)} {
#ifndef NDEBUG
    try {
        formats::json::FromString(json_);
    } catch (const formats::json::ParseException& error) {
        UASSERT_MSG(false, fmt::format("Invalid json: {}, error: {}", json_, error.what()));
    }
#endif

    // Remove extra new lines from user provided json string for two reasons:
    // 1. To avoid additional escaping in TSKV format
    // 2. To ensure a single line log in JSON format
    json_.erase(
        std::remove_if(json_.begin(), json_.end(), [](auto ch) { return ch == '\n' || ch == '\r'; }), json_.end()
    );
}

std::string_view JsonString::GetView() const noexcept {
    if (json_.empty()) {
        return kNull;
    }
    return json_;
}

void WriteToStream(const JsonString& value, formats::json::StringBuilder& sw) { sw.WriteRawString(value.GetView()); }

}  // namespace logging

USERVER_NAMESPACE_END
