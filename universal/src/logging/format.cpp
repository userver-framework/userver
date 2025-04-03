#include <userver/logging/format.hpp>

#include <fmt/format.h>

#include <userver/utils/assert.hpp>
#include <userver/utils/trivial_map.hpp>

USERVER_NAMESPACE_BEGIN

namespace logging {

namespace {
constexpr utils::TrivialBiMap kLogFormats = [](auto selector) {
    return selector()
        .Case("tskv", Format::kTskv)
        .Case("ltsv", Format::kLtsv)
        .Case("raw", Format::kRaw)
        .Case("json", Format::kJson)
        .Case("json_yadeploy", Format::kJsonYaDeploy);
};

}  // namespace

Format FormatFromString(std::string_view format_str) {
    const auto result = kLogFormats.TryFindByFirst(format_str);
    UINVARIANT(
        result, fmt::format("Unknown logging format '{}' (must be one of {})", format_str, kLogFormats.DescribeFirst())
    );
    return *result;
}

}  // namespace logging

USERVER_NAMESPACE_END
