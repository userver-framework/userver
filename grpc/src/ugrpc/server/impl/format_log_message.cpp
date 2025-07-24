#include "format_log_message.hpp"

#include <fmt/chrono.h>
#include <fmt/compile.h>
#include <fmt/format.h>

#include <userver/compiler/thread_local.hpp>
#include <userver/ugrpc/impl/to_string.hpp>
#include <userver/utils/datetime.hpp>
#include <userver/utils/encoding/tskv.hpp>
#include <userver/utils/text_light.hpp>

USERVER_NAMESPACE_BEGIN

namespace ugrpc::server::impl {

namespace {

std::string EscapeForAccessTskvLog(std::string_view str) {
    if (str.empty()) return "-";

    std::string encoded_str;
    EncodeTskv(encoded_str, str, utils::encoding::EncodeTskvMode::kValue);
    return encoded_str;
}

std::string ParseIp(std::string_view sv) {
    static constexpr std::string_view kIpv6 = "ipv6:";
    static constexpr std::string_view kIpv4 = "ipv4:";
    if (utils::text::StartsWith(sv, kIpv6)) sv = sv.substr(kIpv6.size());
    if (utils::text::StartsWith(sv, kIpv4)) sv = sv.substr(kIpv4.size());

    auto pos1 = sv.find("%5B");
    auto pos2 = sv.find("%5D");
    if (pos1 != std::string::npos && pos2 != std::string::npos) {
        sv = sv.substr(pos1 + 3, pos2 - pos1 - 3);
    }

    return EscapeForAccessTskvLog(sv);
}

using SecondsTimePoint = std::chrono::time_point<std::chrono::system_clock, std::chrono::seconds>;

constexpr std::string_view kTimeTemplate = "0000-00-00T00:00:00";

struct CachedTime final {
    SecondsTimePoint cached_time{};
    char cached_time_string[kTimeTemplate.size()]{};
};

compiler::ThreadLocal local_time_cache = [] { return CachedTime{}; };

std::string_view GetCurrentTimeString(std::chrono::system_clock::time_point start_time) noexcept {
    auto cache = local_time_cache.Use();
    const auto rounded_now = std::chrono::time_point_cast<std::chrono::seconds>(start_time);
    if (rounded_now != cache->cached_time) {
        fmt::format_to(
            cache->cached_time_string,
            FMT_COMPILE("{:%FT%T}"),
            fmt::localtime(std::chrono::system_clock::to_time_t(start_time))
        );
        cache->cached_time = rounded_now;
    }
    return std::string_view{cache->cached_time_string, kTimeTemplate.size()};
}

}  // namespace

std::string FormatLogMessage(
    const std::multimap<grpc::string_ref, grpc::string_ref>& metadata,
    std::string_view peer,
    std::chrono::system_clock::time_point start_time,
    std::string_view call_name,
    grpc::StatusCode code
) {
    static const auto timezone = utils::datetime::LocalTimezoneTimestring(start_time, "%z");

    const auto it = metadata.find("user-agent");
    std::string_view user_agent;
    if (it != metadata.end()) {
        user_agent = ugrpc::impl::ToStringView(it->second);
    }

    const auto ip = ParseIp(peer);

    const auto now = std::chrono::system_clock::now();
    const auto request_time = std::chrono::duration_cast<std::chrono::milliseconds>(now - start_time);
    const auto request_time_seconds = std::chrono::duration_cast<std::chrono::seconds>(request_time);
    const auto request_time_milliseconds = request_time - request_time_seconds;

    // FMT_COMPILE makes it slower
    return fmt::format(
        "tskv"
        "\ttimestamp={}"
        "\ttimezone={}"
        "\tuser_agent={}"
        "\tip={}"
        "\tx_real_ip={}"
        "\trequest={}"
        "\trequest_time={}.{:0>3}"
        "\tupstream_response_time={}.{:0>3}"
        "\tgrpc_status={}"
        "\tgrpc_status_code={}\n",
        GetCurrentTimeString(start_time),
        timezone,
        EscapeForAccessTskvLog(user_agent),
        ip,
        ip,
        EscapeForAccessTskvLog(call_name),
        // request_time should represent the time from the first byte received to the last byte sent.
        // We are currently being inexact here by not including the time for initial request deserialization
        // and the time for the final response serialization.
        request_time_seconds.count(),
        request_time_milliseconds.count(),
        // TODO remove, this is for safe migration from old access log parsers.
        request_time_seconds.count(),
        request_time_milliseconds.count(),
        static_cast<int>(code),
        ToString(code)
    );
}

}  // namespace ugrpc::server::impl

USERVER_NAMESPACE_END
