#include <server/handlers/http_handler_base_statistics.hpp>

#include <userver/server/request/task_inherited_data.hpp>
#include <userver/utils/statistics/metadata.hpp>
#include <userver/utils/statistics/percentile_format_json.hpp>

USERVER_NAMESPACE_BEGIN

namespace server::handlers {

void HttpHandlerMethodStatistics::Account(
    const HttpHandlerStatisticsEntry& stats) noexcept {
  reply_codes_.Account(
      static_cast<utils::statistics::HttpCodes::Code>(stats.code));
  timings_.GetCurrentCounter().Account(stats.timing.count());
  if (stats.deadline.IsReachable()) ++deadline_received_;
  if (stats.cancellation == engine::TaskCancellationReason::kDeadline) {
    ++cancelled_by_deadline_;
  }
}

void DumpMetric(utils::statistics::Writer& writer,
                const HttpHandlerMethodStatistics& stats) {
  writer = HttpHandlerStatisticsSnapshot{stats};
}

HttpHandlerStatisticsSnapshot::HttpHandlerStatisticsSnapshot(
    const HttpHandlerMethodStatistics& stats)
    : timings(stats.GetTimings()),
      reply_codes(stats.GetReplyCodes()),
      in_flight(stats.GetInFlight()),
      too_many_requests_in_flight(stats.GetTooManyRequestsInFlight()),
      rate_limit_reached(stats.GetRateLimitReached()),
      deadline_received(stats.GetDeadlineReceived()),
      cancelled_by_deadline(stats.GetCancelledByDeadline()) {}

void HttpHandlerStatisticsSnapshot::Add(
    const HttpHandlerStatisticsSnapshot& other) {
  timings.Add(other.timings);
  reply_codes += other.reply_codes;
  in_flight += other.in_flight;
  too_many_requests_in_flight += other.too_many_requests_in_flight;
  rate_limit_reached += other.rate_limit_reached;
  deadline_received += other.deadline_received;
  cancelled_by_deadline += other.cancelled_by_deadline;
}

void DumpMetric(utils::statistics::Writer& writer,
                const HttpHandlerStatisticsSnapshot& stats) {
  writer["reply-codes"] = stats.reply_codes;
  writer["in-flight"] = stats.in_flight;
  writer["too-many-requests-in-flight"] = stats.too_many_requests_in_flight;
  writer["rate-limit-reached"] = stats.rate_limit_reached;
  writer["deadline-received"] = stats.deadline_received;
  writer["cancelled-by-deadline"] = stats.cancelled_by_deadline;
  writer["timings"] = stats.timings;
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
