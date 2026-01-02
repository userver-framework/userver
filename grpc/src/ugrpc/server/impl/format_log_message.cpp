#include "format_log_message.hpp"

#include <fmt/chrono.h>
#include <fmt/compile.h>
#include <fmt/format.h>

#include <userver/logging/impl/logger_base.hpp>
#include <userver/logging/impl/timestamp.hpp>
#include <userver/utils/datetime.hpp>
#include <userver/utils/encoding/tskv.hpp>
#include <userver/utils/text_light.hpp>

#include <userver/ugrpc/impl/to_string.hpp>
#include <userver/ugrpc/status_codes.hpp>

USERVER_NAMESPACE_BEGIN

namespace ugrpc::server::impl {

namespace {

std::string EscapeForAccessTskvLog(std::string_view str) {
    if (str.empty()) {
        return "-";
    }

    std::string encoded_str;
    EncodeTskv(encoded_str, str, utils::encoding::EncodeTskvMode::kValue);
    return encoded_str;
}

std::string ParseIp(std::string_view sv) {
    static constexpr std::string_view kIpv6 = "ipv6:";
    static constexpr std::string_view kIpv4 = "ipv4:";
    if (utils::text::StartsWith(sv, kIpv6)) {
        sv = sv.substr(kIpv6.size());
    }
    if (utils::text::StartsWith(sv, kIpv4)) {
        sv = sv.substr(kIpv4.size());
    }

    auto pos1 = sv.find("%5B");
    auto pos2 = sv.find("%5D");
    if (pos1 != std::string::npos && pos2 != std::string::npos) {
        sv = sv.substr(pos1 + 3, pos2 - pos1 - 3);
    }

    return EscapeForAccessTskvLog(sv);
}

}  // namespace

logging::impl::LogExtraTskvFormatter FormatLogMessage(
    const std::multimap<grpc::string_ref, grpc::string_ref>& metadata,
    std::string_view peer,
    std::chrono::system_clock::time_point start_time,
    std::string_view call_name,
    std::optional<grpc::StatusCode> code,
    const logging::LogExtra* log_extra
) {
    static const auto kTimezone = utils::datetime::LocalTimezoneTimestring(start_time, "%z");

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

    logging::impl::LogExtraTskvFormatter formatter(logging::Format::kRaw);

    fmt::format_to(
        std::back_inserter(formatter.GetTextLogItem().log_line),
        "\ttimestamp={}"
        "\ttimezone={}"
        "\tuser_agent={}"
        "\tip={}"
        "\tx_real_ip={}"
        "\trequest={}"
        "\trequest_time={}.{:0>3}"
        "\tupstream_response_time={}.{:0>3}"
        "\tgrpc_status={}"
        "\tgrpc_status_code={}",
        logging::impl::GetCurrentLocalTimeString(start_time).ToStringView(),
        kTimezone,
        EscapeForAccessTskvLog(user_agent),
        ip,
        ip,
        EscapeForAccessTskvLog(call_name),
        request_time_seconds.count(),
        request_time_milliseconds.count(),
        // TODO remove, this is for safe migration from old access log parsers.
        request_time_seconds.count(),
        request_time_milliseconds.count(),
        code.has_value() ? std::to_string(static_cast<int>(*code)) : "-",
        code.has_value() ? ToString(*code) : "-"
    );

    if (log_extra) {
        formatter.Append(*log_extra);
    }

    return formatter;
}

}  // namespace ugrpc::server::impl

USERVER_NAMESPACE_END
