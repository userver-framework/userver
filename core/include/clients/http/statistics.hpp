#pragma once

#include <memory>
#include <unordered_map>
#include <vector>

#include <formats/json/value.hpp>
#include <utils/datetime.hpp>
#include <utils/statistics/percentile.hpp>
#include <utils/statistics/recentperiod.hpp>

namespace clients {
namespace http {

struct Statistics;

class RequestStats final {
 public:
  explicit RequestStats(Statistics& stats);
  ~RequestStats();

  void Start();
  void FinishOk(int code, int attempts);
  void FinishEc(std::error_code ec, int attempts);

  void StoreTimeToStart(double seconds);

  void AccountOpenSockets(size_t sockets);

 private:
  void StoreTiming();

 private:
  Statistics& stats_;
  std::chrono::steady_clock::time_point start_time_;
};

struct MultiStats {
  long long socket_open{0}, socket_close{0}, socket_ratelimit{0};
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

struct Statistics {
  Statistics();

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
    kUnknown,
    kCount,
  };
  static constexpr int kErrorGroupCount =
      static_cast<int>(Statistics::ErrorGroup::kCount);

  static ErrorGroup ErrorCodeToGroup(std::error_code ec);

  static const char* ToString(ErrorGroup error);

  void AccountError(ErrorGroup error);

  void AccountStatus(int);

 private:
  std::atomic_llong easy_handles{0};
  std::atomic_llong last_time_to_start_us{0};
  utils::statistics::RecentPeriod<Percentile, Percentile,
                                  utils::datetime::SteadyClock>
      timings_percentile;
  std::array<std::atomic_llong, kErrorGroupCount> error_count{
      {0, 0, 0, 0, 0, 0, 0}};
  std::atomic_llong retries{0};
  std::atomic_llong socket_open{0};

  static constexpr size_t kMinHttpStatus = 100;
  static constexpr size_t kMaxHttpStatus = 600;
  std::array<std::atomic_llong, kMaxHttpStatus - kMinHttpStatus> reply_status{};

  friend struct InstanceStatistics;
  friend class RequestStats;
};

struct InstanceStatistics {
  InstanceStatistics() = default;

  static bool IsForcedStatusCode(int status);

  InstanceStatistics(const Statistics& other);

  long long GetNotOkErrorCount() const;

  void Add(const std::vector<InstanceStatistics>& stats);

  using ErrorGroup = Statistics::ErrorGroup;

 public:
  long long easy_handles{0};
  long long last_time_to_start_us{0};
  Percentile timings_percentile;
  std::array<long long, Statistics::kErrorGroupCount> error_count{
      {0, 0, 0, 0, 0, 0, 0}};
  std::unordered_map<int, long long> reply_status;
  long long retries{0};

  MultiStats multi;
};

struct PoolStatistics {
  std::vector<InstanceStatistics> multi;
};

enum class FormatMode {
  kModeAll,
  kModeDestination,
};

formats::json::ValueBuilder StatisticsToJson(
    const InstanceStatistics& stats,
    FormatMode format_mode = FormatMode::kModeAll);

formats::json::ValueBuilder PoolStatisticsToJson(const PoolStatistics& stats);

}  // namespace http
}  // namespace clients
