#include <userver/ugrpc/server/rpc.hpp>

#include <fmt/format.h>

#include <userver/logging/impl/logger_base.hpp>
#include <userver/logging/logger.hpp>
#include <userver/utils/datetime.hpp>
#include <userver/utils/encoding/tskv.hpp>
#include <userver/utils/text.hpp>

#include <ugrpc/impl/internal_tag.hpp>

USERVER_NAMESPACE_BEGIN

namespace ugrpc::server {

namespace {

std::string EscapeForAccessTskvLog(std::string_view str) {
  if (str.empty()) return "-";

  std::string encoded_str;
  EncodeTskv(encoded_str, str, utils::encoding::EncodeTskvMode::kValue);
  return encoded_str;
}

std::string ParseIp(std::string_view sv) {
  static constexpr std::string_view kIpv6 = "ipv6:";
  if (utils::text::StartsWith(sv, kIpv6)) sv = sv.substr(kIpv6.size());

  auto pos1 = sv.find("%5B");
  auto pos2 = sv.find("%5D");
  if (pos1 != std::string::npos && pos2 != std::string::npos) {
    sv = sv.substr(pos1 + 3, pos2 - pos1 - 3);
  }

  return std::string{sv};
}

}  // namespace

ugrpc::impl::RpcStatisticsScope& CallAnyBase::Statistics(
    ugrpc::impl::InternalTag) {
  return params_.statistics;
}

void CallAnyBase::LogFinish(grpc::Status status) const {
  constexpr auto kLevel = logging::Level::kInfo;
  if (!params_.access_tskv_logger.ShouldLog(kLevel)) {
    return;
  }

  auto md = params_.context.client_metadata();
  auto it = md.find("user-agent");
  std::string user_agent;
  if (it != md.end()) {
    auto ref = it->second;
    user_agent = std::string(ref.begin(), ref.end());
  }

  auto ip = ParseIp(params_.context.peer());

  auto start_time = params_.call_span.GetStartSystemTime();

  auto now = std::chrono::system_clock::now();
  auto response_time =
      std::chrono::duration_cast<std::chrono::milliseconds>(now - start_time)
          .count();

  params_.access_tskv_logger.Log(
      logging::Level::kInfo,
      fmt::format("tskv"
                  "\t{}"
                  "\tuser_agent={}"
                  "\tip={}"
                  "\tx_real_ip={}"
                  "\trequest={}"
                  "\tupstream_response_time_ms={}"
                  "\tgrpc_status={}\n",
                  utils::datetime::LocalTimezoneTimestring(
                      start_time, "timestamp=%Y-%m-%dT%H:%M:%S\ttimezone=%Ez"),
                  EscapeForAccessTskvLog(user_agent),
                  EscapeForAccessTskvLog(ip), EscapeForAccessTskvLog(ip),
                  EscapeForAccessTskvLog(params_.call_name), response_time,
                  static_cast<int>(status.error_code())));
}

}  // namespace ugrpc::server

USERVER_NAMESPACE_END
