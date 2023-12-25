#include <userver/ugrpc/server/rpc.hpp>

#include <fmt/chrono.h>
#include <fmt/compile.h>
#include <fmt/format.h>

#include <userver/compiler/thread_local.hpp>
#include <userver/logging/impl/logger_base.hpp>
#include <userver/logging/logger.hpp>
#include <userver/ugrpc/status_codes.hpp>
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

using SecondsTimePoint =
    std::chrono::time_point<std::chrono::system_clock, std::chrono::seconds>;

constexpr std::string_view kTimeTemplate = "0000-00-00T00:00:00";

struct CachedTime final {
  SecondsTimePoint cached_time{};
  char cached_time_string[kTimeTemplate.size()]{};
};

compiler::ThreadLocal local_time_cache = [] { return CachedTime{}; };

std::string_view GetCurrentTimeString(
    std::chrono::system_clock::time_point start_time) noexcept {
  auto cache = local_time_cache.Use();
  const auto rounded_now =
      std::chrono::time_point_cast<std::chrono::seconds>(start_time);
  if (rounded_now != cache->cached_time) {
    fmt::format_to(
        cache->cached_time_string, FMT_COMPILE("{:%FT%T}"),
        fmt::localtime(std::chrono::system_clock::to_time_t(start_time)));
    cache->cached_time = rounded_now;
  }
  return std::string_view{cache->cached_time_string, kTimeTemplate.size()};
}

}  // namespace

namespace impl {

std::string FormatLogMessage(
    const std::multimap<grpc::string_ref, grpc::string_ref>& metadata,
    std::string_view peer, std::chrono::system_clock::time_point start_time,
    std::string_view call_name, grpc::StatusCode code) {
  static const auto timezone =
      utils::datetime::LocalTimezoneTimestring(start_time, "%z");

  auto it = metadata.find("user-agent");
  std::string_view user_agent;
  if (it != metadata.end()) {
    auto ref = it->second;
    user_agent = std::string_view(ref.data(), ref.size());
  }

  auto ip = ParseIp(peer);

  auto now = std::chrono::system_clock::now();
  auto response_time =
      std::chrono::duration_cast<std::chrono::microseconds>(now - start_time)
          .count();

  // FMT_COMPILE makes it slower
  return fmt::format(
      "tskv"
      "\ttimestamp={}"
      "\ttimezone={}"
      "\tuser_agent={}"
      "\tip={}"
      "\tx_real_ip={}"
      "\trequest={}"
      "\tupstream_response_time_ms={}.{:0>3}"
      "\tgrpc_status={}"
      "\tgrpc_status_code={}\n",
      GetCurrentTimeString(start_time), timezone,
      EscapeForAccessTskvLog(user_agent), ip, ip,
      EscapeForAccessTskvLog(call_name), response_time / 1000,
      response_time % 1000, static_cast<int>(code), ToString(code));
}

}  // namespace impl

ugrpc::impl::RpcStatisticsScope& CallAnyBase::Statistics(
    ugrpc::impl::InternalTag) {
  return params_.statistics;
}

void CallAnyBase::LogFinish(grpc::Status status) const {
  constexpr auto kLevel = logging::Level::kInfo;
  if (!params_.access_tskv_logger.ShouldLog(kLevel)) {
    return;
  }

  auto str = impl::FormatLogMessage(params_.context.client_metadata(),
                                    params_.context.peer(),
                                    params_.call_span.GetStartSystemTime(),
                                    params_.call_name, status.error_code());
  params_.access_tskv_logger.Log(logging::Level::kInfo, std::move(str));
}

}  // namespace ugrpc::server

USERVER_NAMESPACE_END
