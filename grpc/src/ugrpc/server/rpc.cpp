#include <userver/ugrpc/server/rpc.hpp>

#include <fmt/format.h>
#include <userver/logging/impl/logger_base.hpp>
#include <userver/logging/logger.hpp>
#include <userver/utils/datetime.hpp>
#include <userver/utils/encoding/tskv.hpp>

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

void CallAnyBase::LogFinish(grpc::Status status) const {
  auto md = context_.client_metadata();
  auto it = md.find("user-agent");
  std::string user_agent;
  if (it != md.end()) {
    auto ref = it->second;
    user_agent = std::string(ref.begin(), ref.end());
  }

  // TODO: extract IP
  auto ip = context_.peer();

  auto start_time = call_span_.GetStartSystemTime();

  auto now = std::chrono::system_clock::now();
  auto response_time =
      std::chrono::duration_cast<std::chrono::milliseconds>(now - start_time)
          .count();

  access_tskv_logger_.Log(
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
                  EscapeForAccessTskvLog(call_name_), response_time,
                  static_cast<int>(status.error_code())));
}

}  // namespace ugrpc::server

USERVER_NAMESPACE_END
