#include <clients/http/statistics.hpp>

#include <curl-ev/error_code.hpp>

#include <utils/statistics/common.hpp>
#include <utils/statistics/metadata.hpp>
#include <utils/statistics/percentile_format_json.hpp>

namespace clients {
namespace http {

RequestStats::RequestStats(Statistics& stats) : stats_(stats) {
  stats_.easy_handles++;
}

RequestStats::~RequestStats() { stats_.easy_handles--; }

void RequestStats::Start() { start_time_ = std::chrono::steady_clock::now(); }

void RequestStats::FinishOk([[maybe_unused]] int code) {
  stats_.AccountError(Statistics::ErrorGroup::kOk);
  StoreTiming();
}

void RequestStats::FinishEc(std::error_code ec) {
  stats_.AccountError(Statistics::ErrorCodeToGroup(ec));
  StoreTiming();
}

void RequestStats::StoreTiming() {
  auto now = std::chrono::steady_clock::now();
  auto diff = now - start_time_;
  auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(diff).count();
  stats_.timings_percentile.GetCurrentCounter().Account(ms);
}

void RequestStats::StoreTimeToStart(double seconds) {
  /* There is a race between multiple easy handles, we don't care which of them
   * writes its time_to_start. If boost::asio pool is full, we'll see big
   * numbers anyway. */
  stats_.last_time_to_start_us = static_cast<int>(seconds * 1000 * 1000);
}

Statistics::ErrorGroup Statistics::ErrorCodeToGroup(std::error_code ec) {
  if (ec.category() != curl::errc::get_easy_category())
    return ErrorGroup::kUnknown;

  switch (ec.value()) {
    case curl::errc::easy::could_not_resovle_host:
      return ErrorGroup::kHostResolutionFailed;

    case curl::errc::easy::operation_timedout:
      return ErrorGroup::kTimeout;

    case curl::errc::easy::ssl_connect_error:
    case curl::errc::easy::peer_failed_verification:
    case curl::errc::easy::ssl_cipher:
    case curl::errc::easy::ssl_certproblem:
#if (LIBCURL_VERSION_MAJOR < 7 || \
     (LIBCURL_VERSION_MAJOR == 7 && LIBCURL_VERSION_MINOR < 62))
    case curl::errc::easy::ssl_cacert:
#endif
    case curl::errc::easy::ssl_cacert_badfile:
    case curl::errc::easy::ssl_issuer_error:
    case curl::errc::easy::ssl_crl_badfile:
      return ErrorGroup::kSslError;

    case curl::errc::easy::too_many_redirects:
      return ErrorGroup::kTooManyRedirects;

    case curl::errc::easy::send_error:
    case curl::errc::easy::recv_error:
    case curl::errc::easy::could_not_connect:
      return ErrorGroup::kSocketError;

    default:
      return ErrorGroup::kUnknown;
  }
}

const char* Statistics::ToString(ErrorGroup error) {
  switch (error) {
    case ErrorGroup::kOk:
      return "ok";
    case ErrorGroup::kHostResolutionFailed:
      return "host-resolution-failed";
    case ErrorGroup::kSocketError:
      return "socket-error";
    case ErrorGroup::kTimeout:
      return "timeout";
    case ErrorGroup::kSslError:
      return "ssl-error";
    case ErrorGroup::kTooManyRedirects:
      return "too-many-redirects";
    case ErrorGroup::kUnknown:
    case ErrorGroup::kCount:
      break;
  }
  return "unknown-error";
}

void Statistics::AccountError(ErrorGroup error) {
  error_count[static_cast<int>(error)]++;
}

formats::json::ValueBuilder StatisticsToJson(const InstanceStatistics& stats) {
  formats::json::ValueBuilder json;
  json["timings"]["1min"] =
      utils::statistics::PercentileToJson(stats.timings_percentile)
          .ExtractValue();
  utils::statistics::SolomonSkip(json["timings"]["1min"]);

  formats::json::ValueBuilder errors;
  for (int i = 0; i < Statistics::kErrorGroupCount; i++) {
    const auto error_group = static_cast<Statistics::ErrorGroup>(i);
    errors[Statistics::ToString(error_group)] = stats.error_count[i];
  }
  utils::statistics::SolomonChildrenAreLabelValues(errors, "http_error");
  json["errors"] = errors;
  json["pending-requests"] = stats.easy_handles;
  json["last-time-to-start-us"] = stats.last_time_to_start_us;
  json["event-loop-load"][utils::statistics::DurationToString(
      utils::statistics::kDefaultMaxPeriod)] =
      std::to_string(stats.multi.current_load);

  json["sockets"]["open"] = stats.multi.socket_open;
  json["sockets"]["close"] = stats.multi.socket_close;
  json["sockets"]["active"] =
      stats.multi.socket_open - stats.multi.socket_close;

  return json;
}

formats::json::ValueBuilder PoolStatisticsToJson(const PoolStatistics& stats) {
  formats::json::ValueBuilder json;
  InstanceStatistics sum_stats;

  sum_stats.Add(stats.multi);
  for (size_t i = 0; i < stats.multi.size(); i++) {
    auto key = "worker-" + std::to_string(i);
    json[key] = StatisticsToJson(stats.multi[i]);
    utils::statistics::SolomonLabelValue(json[key], "http_worker_id");
  }

  json["pool-total"] = StatisticsToJson(sum_stats);
  utils::statistics::SolomonSkip(json["pool-total"]);
  return json;
}

long long InstanceStatistics::GetNotOkErrorCount() const {
  long long result{0};

  for (size_t i = 0; i < Statistics::kErrorGroupCount; i++) {
    auto error_group = static_cast<Statistics::ErrorGroup>(i);
    if (error_group == ErrorGroup::kOk) continue;

    result += error_count[i];
  }
  return result;
}

void InstanceStatistics::Add(const std::vector<InstanceStatistics>& stats) {
  for (const auto& stat : stats) {
    easy_handles += stat.easy_handles;
    last_time_to_start_us +=
        stat.last_time_to_start_us;  // will be divided later

    timings_percentile.Add(stat.timings_percentile);

    for (size_t i = 0; i < Statistics::kErrorGroupCount; i++) {
      error_count[i] += stat.error_count[i];
    }

    multi += stat.multi;
  }

  if (!stats.empty()) {
    last_time_to_start_us /= stats.size();
    multi.current_load /= stats.size();
  }
}

}  // namespace http
}  // namespace clients
