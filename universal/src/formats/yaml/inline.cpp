#include <userver/formats/yaml/inline.hpp>

#include <string>

#include <userver/utils/datetime_light.hpp>

USERVER_NAMESPACE_BEGIN

namespace formats::yaml::impl {
namespace {

std::string FormatTimePoint(std::chrono::system_clock::time_point value) {
    return utils::datetime::UtcTimestring(value, utils::datetime::kRfc3339Format);
}

}  // namespace

formats::yaml::Value InlineObjectBuilder::DoBuild() { return yaml_.ExtractValue(); }

void InlineObjectBuilder::Append(std::string_view key, const std::chrono::system_clock::time_point& value) {
    Append(key, FormatTimePoint(value));
}

formats::yaml::Value InlineArrayBuilder::Build() { return yaml_.ExtractValue(); }

void InlineArrayBuilder::Append(const std::chrono::system_clock::time_point& value) { Append(FormatTimePoint(value)); }

}  // namespace formats::yaml::impl

USERVER_NAMESPACE_END
