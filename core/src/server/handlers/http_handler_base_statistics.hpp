#pragma once

#include <algorithm>
#include <chrono>
#include <type_traits>

#include <server/http/handler_methods.hpp>
#include <userver/engine/deadline.hpp>
#include <userver/server/handlers/http_handler_base.hpp>
#include <userver/server/http/http_status.hpp>
#include <userver/utils/statistics/aggregated_values.hpp>
#include <userver/utils/statistics/percentile.hpp>
#include <userver/utils/statistics/recentperiod.hpp>
#include <utils/statistics/http_codes.hpp>

USERVER_NAMESPACE_BEGIN

namespace server::handlers {

// Statistics for a single request from the handler perspective.
struct HttpHandlerStatisticsEntry final {
  http::HttpStatus code{http::HttpStatus::kInternalServerError};
  std::chrono::milliseconds timing{};
  engine::Deadline deadline{};
  bool cancelled_by_deadline{false};
};

class HttpHandlerMethodStatistics final {
 public:
  void Account(const HttpHandlerStatisticsEntry& stats) noexcept;

  const utils::statistics::HttpCodes& GetReplyCodes() const {
    return reply_codes_;
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
  using RecentPeriod =
      utils::statistics::RecentPeriod<Percentile, Percentile,
                                      utils::datetime::SteadyClock>;

  RecentPeriod timings_;
  utils::statistics::HttpCodes reply_codes_;
  std::atomic<std::size_t> in_flight_{0};
  std::atomic<std::uint64_t> too_many_requests_in_flight_{0};
  std::atomic<std::uint64_t> rate_limit_reached_{0};
  std::atomic<std::uint64_t> deadline_received_{0};
  std::atomic<std::uint64_t> cancelled_by_deadline_{0};
};

void DumpMetric(utils::statistics::Writer& writer,
                const HttpHandlerMethodStatistics& stats);

struct HttpHandlerStatisticsSnapshot final {
  HttpHandlerStatisticsSnapshot() = default;

  explicit HttpHandlerStatisticsSnapshot(
      const HttpHandlerMethodStatistics& stats);

  void Add(const HttpHandlerStatisticsSnapshot& other);

  HttpHandlerMethodStatistics::Percentile timings;
  utils::statistics::HttpCodes::Snapshot reply_codes;
  std::size_t in_flight{0};
  std::uint64_t too_many_requests_in_flight{0};
  std::uint64_t rate_limit_reached{0};
  std::uint64_t deadline_received{0};
  std::uint64_t cancelled_by_deadline{0};
};

void DumpMetric(utils::statistics::Writer& writer,
                const HttpHandlerStatisticsSnapshot& stats);

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

  // TODO(TAXICOMMON-6584) detect automatically?
  //  symptom: we didn't send a normal response due to deadline expiration
  void OnCancelledByDeadline() noexcept;

 private:
  HttpHandlerStatistics& stats_;
  const http::HttpMethod method_;
  const std::chrono::steady_clock::time_point start_time_;
  server::http::HttpResponse& response_;
  bool cancelled_by_deadline_{false};
};

}  // namespace server::handlers

USERVER_NAMESPACE_END
