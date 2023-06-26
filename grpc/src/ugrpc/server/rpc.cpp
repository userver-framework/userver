#include <userver/ugrpc/server/rpc.hpp>

#include <fmt/format.h>

#include <userver/logging/impl/logger_base.hpp>
#include <userver/logging/logger.hpp>
#include <userver/utils/datetime.hpp>
#include <userver/utils/encoding/tskv.hpp>

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

}  // namespace

ugrpc::impl::RpcStatisticsScope& CallAnyBase::Statistics(
    ugrpc::impl::InternalTag) {
  return params_.statistics;
}

void CallAnyBase::LogFinish(grpc::Status status) const {
  auto md = params_.context.client_metadata();
  auto it = md.find("user-agent");
  std::string user_agent;
  if (it != md.end()) {
    auto ref = it->second;
    user_agent = std::string(ref.begin(), ref.end());
  }

  // TODO: extract IP
  auto ip = params_.context.peer();

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
                  "\treal_ip={}"
                  "\trequest={}"
                  "\tupstream_response_time_ms={}"
                  "\tgrpc_status={}",
                  utils::datetime::LocalTimezoneTimestring(
                      start_time, "timestamp=%Y-%m-%dT%H:%M:%S\ttimezone=%Ez"),
                  EscapeForAccessTskvLog(user_agent),
                  EscapeForAccessTskvLog(ip), EscapeForAccessTskvLog(ip),
                  EscapeForAccessTskvLog(params_.call_name), response_time,
                  static_cast<int>(status.error_code())));
}

}  // namespace ugrpc::server

USERVER_NAMESPACE_END
