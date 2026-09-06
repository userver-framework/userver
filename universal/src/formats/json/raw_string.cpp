#include <userver/formats/json/raw_string.hpp>

#include <userver/formats/json/serialize.hpp>
#include <userver/formats/json/string_builder.hpp>
#include <userver/utils/assert.hpp>
#include <userver/utils/string_literal.hpp>

USERVER_NAMESPACE_BEGIN

namespace formats::json {

namespace {

constexpr utils::StringLiteral kNull = "{}";

}  // namespace

RawString::RawString(const Value& value)
    : json_{ToString(value)}
{}

RawString::RawString(std::string json) noexcept : json_{std::move(json)} {
#ifndef NDEBUG
    try {
        FromString(json_);
    } catch (const ParseException& error) {
        UASSERT_MSG(false, fmt::format("Invalid json: {}, error: {}", json_, error.what()));
    }
#endif
}

std::string_view RawString::GetView() const noexcept {
    if (json_.empty()) {
        return kNull;
    }
    return json_;
}

void WriteToStream(const RawString& value, StringBuilder& sw) { sw.WriteRawString(value.GetView()); }

Value Serialize(const RawString& json, formats::serialize::To<Value>) {
    return formats::json::FromString(json.GetView());
}

RawString Parse(const Value& value, formats::parse::To<RawString>) { return RawString(value); }

}  // namespace formats::json

USERVER_NAMESPACE_END
