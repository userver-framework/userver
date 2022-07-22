#include <server/handlers/http_handler_base_statistics.hpp>

#include <userver/server/request/task_inherited_data.hpp>
#include <userver/utils/statistics/metadata.hpp>
#include <userver/utils/statistics/percentile_format_json.hpp>

USERVER_NAMESPACE_BEGIN

namespace server::handlers {

void HttpHandlerMethodStatistics::Account(
    const HttpHandlerStatisticsEntry& stats) noexcept {
  reply_codes_.Account(
      static_cast<utils::statistics::HttpCodes::CodeType>(stats.code));
  timings_.GetCurrentCounter().Account(stats.timing.count());
  if (stats.deadline.IsReachable()) ++deadline_received_;
  if (stats.cancellation == engine::TaskCancellationReason::kDeadline) {
    ++cancelled_by_deadline_;
  }
}

formats::json::Value Serialize(const HttpHandlerMethodStatistics& stats,
                               formats::serialize::To<formats::json::Value>) {
  formats::json::ValueBuilder result;
  formats::json::ValueBuilder total;

  total["reply-codes"] = stats.FormatReplyCodes();
  total["in-flight"] = stats.GetInFlight();
  total["too-many-requests-in-flight"] = stats.GetTooManyRequestsInFlight();
  total["rate-limit-reached"] = stats.GetRateLimitReached();
  total["deadline-received"] = stats.GetDeadlineReceived();
  total["cancelled-by-deadline"] = stats.GetCancelledByDeadline();

  total["timings"]["1min"] =
      utils::statistics::PercentileToJson(stats.GetTimings());
  utils::statistics::SolomonSkip(total["timings"]["1min"]);

  utils::statistics::SolomonSkip(total);
  result["total"] = std::move(total);
  return result.ExtractValue();
}

void HttpRequestMethodStatistics::Account(
    const HttpRequestStatisticsEntry& stats) noexcept {
  timings_.GetCurrentCounter().Account(stats.timing.count());
}

formats::json::Value Serialize(const HttpRequestMethodStatistics& stats,
                               formats::serialize::To<formats::json::Value>) {
  formats::json::ValueBuilder result;
  formats::json::ValueBuilder total;

  total["timings"]["1min"] =
      utils::statistics::PercentileToJson(stats.GetTimings());
  utils::statistics::SolomonSkip(total["timings"]["1min"]);

  utils::statistics::SolomonSkip(total);
  result["total"] = std::move(total);
  return result.ExtractValue();
}

bool IsOkMethod(http::HttpMethod method) noexcept {
  return static_cast<std::size_t>(method) <= http::kHandlerMethodsMax;
}

std::size_t HttpMethodToIndex(http::HttpMethod method) noexcept {
  UASSERT(IsOkMethod(method));
  return static_cast<std::size_t>(method);
}

HttpHandlerStatisticsScope::HttpHandlerStatisticsScope(
    HttpHandlerStatistics& stats, http::HttpMethod method,
    server::http::HttpResponse& response)
    : stats_(stats),
      method_(method),
      start_time_(std::chrono::steady_clock::now()),
      response_(response) {
  stats_.ForMethodAndTotal(method, [&](HttpHandlerMethodStatistics& stats) {
    stats.IncrementInFlight();
  });
}

HttpHandlerStatisticsScope::~HttpHandlerStatisticsScope() {
  const auto finish_time = std::chrono::steady_clock::now();
  const auto* const data = request::kTaskInheritedData.GetOptional();

  HttpHandlerStatisticsEntry stats;
  stats.code = response_.GetStatus();
  stats.timing = std::chrono::duration_cast<std::chrono::milliseconds>(
      finish_time - start_time_);
  stats.deadline = data ? data->deadline : engine::Deadline{};
  stats.cancellation = engine::current_task::CancellationReason();
  stats_.Account(method_, stats);

  stats_.ForMethodAndTotal(method_, [&](HttpHandlerMethodStatistics& stats) {
    stats.DecrementInFlight();
  });
}

}  // namespace server::handlers

USERVER_NAMESPACE_END
