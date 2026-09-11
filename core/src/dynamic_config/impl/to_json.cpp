#include <userver/dynamic_config/impl/to_json.hpp>

#include <userver/formats/json/string_builder.hpp>

USERVER_NAMESPACE_BEGIN

namespace dynamic_config::impl {

namespace {

template <typename T>
std::string ToJsonStringImpl(const T& value) {
    formats::json::StringBuilder builder;
    WriteToStream(value, builder);
    return builder.GetString();
}

}  // namespace

std::string DoToJsonString(bool value) { return ToJsonStringImpl(value); }

std::string DoToJsonString(double value) { return ToJsonStringImpl(value); }

std::string DoToJsonString(std::uint64_t value) { return ToJsonStringImpl(value); }

std::string DoToJsonString(std::int64_t value) { return ToJsonStringImpl(value); }

std::string DoToJsonString(std::string_view value) { return ToJsonStringImpl(value); }

}  // namespace dynamic_config::impl

USERVER_NAMESPACE_END
