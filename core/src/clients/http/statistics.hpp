#pragma once

#include <cstdint>
#include <memory>
#include <unordered_map>
#include <vector>

#include <userver/utils/datetime.hpp>
#include <userver/utils/statistics/percentile.hpp>
#include <userver/utils/statistics/rate.hpp>
#include <userver/utils/statistics/rate_counter.hpp>
#include <userver/utils/statistics/recentperiod.hpp>
#include <userver/utils/statistics/writer.hpp>
#include <utils/statistics/http_codes.hpp>

USERVER_NAMESPACE_BEGIN

namespace clients::http {

class Statistics;

class RequestStats final {
 public:
  explicit RequestStats(Statistics& stats);
  ~RequestStats();

  void Start();
  void FinishOk(int code, unsigned int attempts) noexcept;
  void FinishEc(std::error_code ec, unsigned int attempts) noexcept;

  void StoreTimeToStart(std::chrono::microseconds micro_seconds) noexcept;

  void AccountOpenSockets(size_t sockets) noexcept;

  void AccountTimeoutUpdatedByDeadline() noexcept;
  void AccountCancelledByDeadline() noexcept;

 private:
  void StoreTiming() noexcept;

  Statistics& stats_;
  std::chrono::steady_clock::time_point start_time_;
};

struct MultiStats {
  utils::statistics::Rate socket_open;
  utils::statistics::Rate socket_close;
  utils::statistics::Rate socket_ratelimit;
  double current_load{0};

  MultiStats& operator+=(const MultiStats& other) {
    socket_open += other.socket_open;
    socket_close += other.socket_close;
    socket_ratelimit += other.socket_ratelimit;
    current_load += other.current_load;
    return *this;
  }
};

using Percentile =
    utils::statistics::Percentile</*buckets =*/2000, unsigned int,
                                  /*extra_buckets=*/1180,
                                  /*extra_bucket_size=*/100>;

class Statistics {
 public:
  Statistics() = default;

  std::shared_ptr<RequestStats> CreateRequestStats() {
    return std::make_shared<RequestStats>(*this);
  }

  enum class ErrorGroup {
    kOk,
    kHostResolutionFailed,
    kSocketError,
    kTimeout,
    kSslError,
    kTooManyRedirects,
    kCancelled,
    kUnknown,
    kCount,
  };
  static constexpr auto kErrorGroupCount =
      static_cast<std::size_t>(Statistics::ErrorGroup::kCount);

  static ErrorGroup ErrorCodeToGroup(std::error_code ec);

  static const char* ToString(ErrorGroup error);

  void AccountError(ErrorGroup error);

  void AccountStatus(int);

 private:
  std::atomic<uint64_t> easy_handles_{0};
  std::atomic<uint64_t> last_time_to_start_us_{0};
  utils::statistics::RecentPeriod<Percentile, Percentile,
                                  utils::datetime::SteadyClock>
      timings_percentile_;
  std::array<utils::statistics::RateCounter, kErrorGroupCount> error_count_;
  utils::statistics::RateCounter retries_;
  utils::statistics::RateCounter socket_open_{0};
  utils::statistics::RateCounter timeout_updated_by_deadline_;
  utils::statistics::RateCounter cancelled_by_deadline_;
  utils::statistics::HttpCodes reply_status_;

  friend struct InstanceStatistics;
  friend class RequestStats;
};

struct InstanceStatistics {
  InstanceStatistics() = default;

  explicit InstanceStatistics(const Statistics& other);

  uint64_t GetNotOkErrorCount() const;

  InstanceStatistics& operator+=(const InstanceStatistics& stat);

  using ErrorGroup = Statistics::ErrorGroup;

  std::size_t instances_aggregated{1};

  uint64_t easy_handles{0};
  uint64_t last_time_to_start_us{0};
  Percentile timings_percentile;
  std::array<utils::statistics::Rate, Statistics::kErrorGroupCount> error_count;
  utils::statistics::Rate retries{0};

  utils::statistics::Rate timeout_updated_by_deadline;
  utils::statistics::Rate cancelled_by_deadline;
  utils::statistics::HttpCodes::Snapshot reply_status;

  MultiStats multi;
};

struct PoolStatistics {
  std::vector<InstanceStatistics> multi;
};

struct DestinationStatisticsView {
  const InstanceStatistics& stats;
};

void DumpMetric(utils::statistics::Writer& writer,
                const DestinationStatisticsView& view);

void DumpMetric(utils::statistics::Writer& writer,
                const InstanceStatistics& stats);

void DumpMetric(utils::statistics::Writer& writer, const PoolStatistics& stats);

}  // namespace clients::http

USERVER_NAMESPACE_END
