#pragma once

#include <algorithm>
#include <chrono>
#include <type_traits>

#include <server/http/handler_methods.hpp>
#include <userver/engine/deadline.hpp>
#include <userver/engine/task/cancel.hpp>
#include <userver/formats/json_fwd.hpp>
#include <userver/server/handlers/http_handler_base.hpp>
#include <userver/server/http/http_status.hpp>
#include <userver/utils/statistics/aggregated_values.hpp>
#include <userver/utils/statistics/http_codes.hpp>
#include <userver/utils/statistics/percentile.hpp>
#include <userver/utils/statistics/recentperiod.hpp>

USERVER_NAMESPACE_BEGIN

namespace server::handlers {

// Statistics for a single request from the handler perspective.
struct HttpHandlerStatisticsEntry final {
  http::HttpStatus code{http::HttpStatus::kInternalServerError};
  std::chrono::milliseconds timing{};
  engine::Deadline deadline{};
  engine::TaskCancellationReason cancellation{
      engine::TaskCancellationReason::kNone};
};

class HttpHandlerMethodStatistics final {
 public:
  void Account(const HttpHandlerStatisticsEntry& stats) noexcept;

  formats::json::Value FormatReplyCodes() const {
    return reply_codes_.FormatReplyCodes();
  }

  using Percentile = utils::statistics::Percentile<2048, unsigned int, 120>;

  Percentile GetTimings() const { return timings_.GetStatsForPeriod(); }

  size_t GetInFlight() const noexcept { return in_flight_; }

  void IncrementInFlight() noexcept { in_flight_++; }

  void DecrementInFlight() noexcept { in_flight_--; }

  void IncrementTooManyRequestsInFlight() noexcept {
    too_many_requests_in_flight_++;
  }

  size_t GetTooManyRequestsInFlight() const noexcept {
    return too_many_requests_in_flight_;
  }

  void IncrementRateLimitReached() noexcept { rate_limit_reached_++; }

  size_t GetRateLimitReached() const noexcept { return rate_limit_reached_; }

  std::uint64_t GetDeadlineReceived() const noexcept {
    return deadline_received_.load();
  }

  std::uint64_t GetCancelledByDeadline() const noexcept {
    return cancelled_by_deadline_.load();
  }

 private:
  utils::statistics::RecentPeriod<Percentile, Percentile,
                                  utils::datetime::SteadyClock>
      timings_;
  utils::statistics::HttpCodes reply_codes_{400, 401, 499, 500};
  std::atomic<size_t> in_flight_{0};
  std::atomic<size_t> too_many_requests_in_flight_{0};
  std::atomic<size_t> rate_limit_reached_{0};
  std::atomic<std::uint64_t> deadline_received_{0};
  std::atomic<std::uint64_t> cancelled_by_deadline_{0};
};

formats::json::Value Serialize(const HttpHandlerMethodStatistics& stats,
                               formats::serialize::To<formats::json::Value>);

// Statistics for a single request from the overall server or the external
// client perspective. Includes the time spent in queue.
struct HttpRequestStatisticsEntry final {
  std::chrono::milliseconds timing;
};

class HttpRequestMethodStatistics final {
 public:
  void Account(const HttpRequestStatisticsEntry& stats) noexcept;

  using Percentile = utils::statistics::Percentile<2048, unsigned int, 120>;

  Percentile GetTimings() const { return timings_.GetStatsForPeriod(); }

 private:
  utils::statistics::RecentPeriod<Percentile, Percentile,
                                  utils::datetime::SteadyClock>
      timings_;
};

formats::json::Value Serialize(const HttpRequestMethodStatistics& stats,
                               formats::serialize::To<formats::json::Value>);

bool IsOkMethod(http::HttpMethod method) noexcept;

std::size_t HttpMethodToIndex(http::HttpMethod method) noexcept;

template <typename MethodStatistics>
class ByMethodStatistics {
 public:
  MethodStatistics& GetByMethod(http::HttpMethod method) noexcept {
    return by_method_[HttpMethodToIndex(method)];
  }

  const MethodStatistics& GetByMethod(http::HttpMethod method) const noexcept {
    return by_method_[HttpMethodToIndex(method)];
  }

  MethodStatistics& GetTotal() noexcept { return total_; }

  const MethodStatistics& GetTotal() const noexcept { return total_; }

  template <typename Func>
  void ForMethodAndTotal(http::HttpMethod method, const Func& func) {
    static_assert(std::is_invocable_v<const Func&, MethodStatistics&>);
    func(total_);
    if (IsOkMethod(method)) func(by_method_[HttpMethodToIndex(method)]);
  }

  template <typename Entry>
  void Account(http::HttpMethod method, const Entry& entry) noexcept {
    ForMethodAndTotal(method, [&](auto& stats) { stats.Account(entry); });
  }

 private:
  MethodStatistics total_;
  std::array<MethodStatistics, http::kHandlerMethodsMax + 1> by_method_;
};

class HttpHandlerStatistics final
    : public ByMethodStatistics<HttpHandlerMethodStatistics> {};

class HttpRequestStatistics final
    : public ByMethodStatistics<HttpRequestMethodStatistics> {};

class HttpHandlerStatisticsScope final {
 public:
  HttpHandlerStatisticsScope(HttpHandlerStatistics& stats,
                             http::HttpMethod method,
                             server::http::HttpResponse& response);

  ~HttpHandlerStatisticsScope();

 private:
  HttpHandlerStatistics& stats_;
  const http::HttpMethod method_;
  const std::chrono::steady_clock::time_point start_time_;
  server::http::HttpResponse& response_;
};

}  // namespace server::handlers

USERVER_NAMESPACE_END
