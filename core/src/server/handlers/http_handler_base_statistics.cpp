#include <server/handlers/http_handler_base_statistics.hpp>

#include <algorithm>

#include <userver/server/request/task_inherited_data.hpp>

USERVER_NAMESPACE_BEGIN

namespace server::handlers {

namespace {

struct HttpHandlerStatisticsHelper {
    const HttpHandlerStatisticsSnapshot& snapshot;
};

[[maybe_unused]] void DumpMetric(utils::statistics::Writer& writer, HttpHandlerStatisticsHelper helper) {
    const auto& stats = helper.snapshot;
    writer["rps"] = stats.finished;
    writer["reply-codes"] = stats.reply_codes;
    writer["in-flight"] = stats.in_flight;
    writer["too-many-requests-in-flight"] = stats.too_many_requests_in_flight;
    writer["rate-limit-reached"] = stats.rate_limit_reached;
    writer["deadline-received"] = stats.deadline_received;
    writer["cancelled-by-deadline"] = stats.cancelled_by_deadline;
    writer["timings"] = stats.timings;
}

}  // namespace

void HttpHandlerMethodStatistics::Account(const HttpHandlerStatisticsEntry& stats) noexcept {
    reply_codes_.Account(static_cast<utils::statistics::HttpCodes::Code>(stats.code));
    timings_.GetCurrentCounter().Account(stats.timing.count());
    if (stats.deadline.IsReachable()) {
        ++deadline_received_;
    }
    if (stats.cancelled_by_deadline) {
        ++cancelled_by_deadline_;
    }
}

std::size_t HttpHandlerMethodStatistics::GetInFlight() const noexcept {
    const auto finished = finished_.Load();
    const auto started = started_.Load();
    return static_cast<std::size_t>(std::max(started, finished).value - finished.value);
}

void DumpMetric(utils::statistics::Writer& writer, const HttpHandlerMethodStatistics& stats) {
    writer = HttpHandlerStatisticsSnapshot{stats};
}

HttpHandlerStatisticsSnapshot::HttpHandlerStatisticsSnapshot(const HttpHandlerMethodStatistics& stats)
    : timings(stats.timings_.GetStatsForPeriod()),
      reply_codes(stats.reply_codes_),
      in_flight(stats.GetInFlight()),
      finished(stats.finished_.Load()),
      too_many_requests_in_flight(stats.too_many_requests_in_flight_.Load()),
      rate_limit_reached(stats.rate_limit_reached_.Load()),
      deadline_received(stats.deadline_received_.Load()),
      cancelled_by_deadline(stats.cancelled_by_deadline_.Load())
{}

void HttpHandlerStatisticsSnapshot::Add(const HttpHandlerStatisticsSnapshot& other) {
    timings.Add(other.timings);
    reply_codes += other.reply_codes;
    in_flight += other.in_flight;
    finished += other.finished;
    too_many_requests_in_flight += other.too_many_requests_in_flight;
    rate_limit_reached += other.rate_limit_reached;
    deadline_received += other.deadline_received;
    cancelled_by_deadline += other.cancelled_by_deadline;
}

void DumpMetric(utils::statistics::Writer& writer, const HttpHandlerStatisticsSnapshot& stats) {
    writer.ValueWithLabels(HttpHandlerStatisticsHelper{stats}, {"version", "2"});
}

bool IsOkMethod(http::HttpMethod method) noexcept {
    return static_cast<std::size_t>(method) <= http::kHandlerMethodsMax;
}

std::size_t HttpMethodToIndex(http::HttpMethod method) noexcept {
    UASSERT(IsOkMethod(method));
    return static_cast<std::size_t>(method);
}

HttpHandlerStatistics& HttpHandlerStatisticsAggregate::GetOverallStatistics() { return overall_handler_statistics_; }

utils::statistics::impl::MonotonicConcurrentStatisticsMap<HttpHandlerStatistics>&
HttpHandlerStatisticsAggregate::GetShardedStatisticsStorage() {
    return sharded_handler_statistics_;
}

HttpHandlerStatisticsScope::HttpHandlerStatisticsScope(
    HttpHandlerStatisticsAggregate& stats,
    http::HttpMethod method,
    server::http::HttpResponse& response
)
    : stats_(stats),
      method_(method),
      start_time_(std::chrono::steady_clock::now()),
      response_(response)
{
    stats_.GetOverallStatistics().ForMethod(method).IncrementInFlight();
}

void HttpHandlerStatisticsScope::SetShardedStats(
    std::string_view sensor_path_part,
    utils::statistics::LabelsSpan labels
) {
    UINVARIANT(
        std::ranges::is_sorted(labels, [](const auto& a, const auto& b) { return a.Name() < b.Name(); }),
        "Labels for sharded metrics should be lexicographically sorted"
    );
    auto [item, inserted] = stats_.GetShardedStatisticsStorage().TryEmplace({sensor_path_part, labels});
    sharded_stats_ = &item;
    sharded_stats_->ForMethod(method_).IncrementInFlight();
}

HttpHandlerStatisticsScope::~HttpHandlerStatisticsScope() {
    const auto finish_time = std::chrono::steady_clock::now();
    const auto* const data = request::kTaskInheritedData.GetOptional();

    HttpHandlerStatisticsEntry stats;
    stats.code = response_.GetStatus();
    stats.timing = std::chrono::duration_cast<std::chrono::milliseconds>(finish_time - start_time_);
    stats.deadline = data ? data->deadline : engine::Deadline{};
    stats.cancelled_by_deadline = cancelled_by_deadline_;
    auto& overall_for_method = stats_.GetOverallStatistics().ForMethod(method_);
    overall_for_method.Account(stats);
    overall_for_method.DecrementInFlight();
    if (sharded_stats_) {
        auto& sharded_for_method = sharded_stats_->ForMethod(method_);
        sharded_for_method.Account(stats);
        sharded_for_method.DecrementInFlight();
    }
}

void HttpHandlerStatisticsScope::OnCancelledByDeadline() noexcept { cancelled_by_deadline_ = true; }

}  // namespace server::handlers

USERVER_NAMESPACE_END
