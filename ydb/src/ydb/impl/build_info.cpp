#include "build_info.hpp"

#include <ydb-cpp-sdk/client/driver/driver.h>

#include <userver/utils/algo.hpp>
#include <userver/utils/assert.hpp>
#include <userver/utils/userver_info.hpp>

USERVER_NAMESPACE_BEGIN

namespace ydb::impl {

namespace {

constexpr std::string_view kBuildInfoName = "ydb-userver";

bool IsNonEmptyAlnum(std::string_view s) {
    return !s.empty() && std::ranges::all_of(s, [](char c) { return isdigit(c) || (isalpha(c) && islower(c)); });
}

}  // namespace

std::string FormatUserverVersionForBuildInfo(std::string_view version) {
    const auto dash = version.find('-');
    if (dash != std::string_view::npos) {
        version = version.substr(0, dash);
    }

    const auto dot1 = version.find('.');
    UINVARIANT(dot1 != std::string_view::npos, "Invalid userver version");

    const auto dot2 = version.find('.', dot1 + 1);
    std::string result;
    if (dot2 == std::string_view::npos) {
        result = utils::StrCat(version, ".0");
    } else {
        UINVARIANT(version.find('.', dot2 + 1) == std::string_view::npos, "Invalid userver version");
        result.assign(version);
    }

    const auto result_dot1 = result.find('.');
    const auto result_dot2 = result.find('.', result_dot1 + 1);
    UINVARIANT(
        IsNonEmptyAlnum(std::string_view{result}.substr(0, result_dot1)
        ) && IsNonEmptyAlnum(std::string_view{result}.substr(result_dot1 + 1, result_dot2 - result_dot1 - 1)) &&
            IsNonEmptyAlnum(std::string_view{result}.substr(result_dot2 + 1)),
        "Invalid userver version for build info"
    );

    return result;
}

void AppendUserverYdbBuildInfo(NYdb::TDriverConfig& config) {
    const auto version = FormatUserverVersionForBuildInfo(utils::GetUserverVersion());
    config.AppendBuildInfo(utils::StrCat(kBuildInfoName, "/", version));
}

}  // namespace ydb::impl

USERVER_NAMESPACE_END
