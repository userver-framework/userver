#pragma once

#include <array>
#include <chrono>
#include <cstddef>
#include <type_traits>

#include <server/http/handler_methods.hpp>
#include <userver/engine/deadline.hpp>
#include <userver/server/handlers/http_handler_base.hpp>
#include <userver/server/http/http_status.hpp>
#include <userver/utils/statistics/percentile.hpp>
#include <userver/utils/statistics/rate.hpp>
#include <userver/utils/statistics/rate_counter.hpp>
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

struct HttpHandlerStatisticsSnapshot;

class HttpHandlerMethodStatistics final {
 public:
  using Snapshot = HttpHandlerStatisticsSnapshot;

  void Account(const HttpHandlerStatisticsEntry& stats) noexcept;

  std::size_t GetInFlight() const noexcept;

  void IncrementInFlight() noexcept { ++started_; }

  void DecrementInFlight() noexcept { ++finished_; }

  void IncrementTooManyRequestsInFlight() noexcept {
    ++too_many_requests_in_flight_;
  }

  void IncrementRateLimitReached() noexcept { ++rate_limit_reached_; }

 private:
  friend struct HttpHandlerStatisticsSnapshot;

  using Percentile = utils::statistics::Percentile<2048, unsigned int, 120>;
  using RecentPeriod =
      utils::statistics::RecentPeriod<Percentile, Percentile,
                                      utils::datetime::SteadyClock>;

  RecentPeriod timings_;
  utils::statistics::HttpCodes reply_codes_;
  utils::statistics::RateCounter started_;
  utils::statistics::RateCounter finished_;
  utils::statistics::RateCounter too_many_requests_in_flight_;
  utils::statistics::RateCounter rate_limit_reached_;
  utils::statistics::RateCounter deadline_received_;
  utils::statistics::RateCounter cancelled_by_deadline_;
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
  utils::statistics::Rate finished;
  utils::statistics::Rate too_many_requests_in_flight;
  utils::statistics::Rate rate_limit_reached;
  utils::statistics::Rate deadline_received;
  utils::statistics::Rate cancelled_by_deadline;
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
  using Snapshot = void;

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
  using Snapshot = typename MethodStatistics::Snapshot;

  const MethodStatistics& GetByMethod(http::HttpMethod method) const noexcept {
    return by_method_[HttpMethodToIndex(method)];
  }

  MethodStatistics& ForMethod(http::HttpMethod method) noexcept {
    return by_method_[HttpMethodToIndex(method)];
  }

 private:
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
