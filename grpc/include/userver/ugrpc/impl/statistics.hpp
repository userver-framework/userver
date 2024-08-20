#pragma once

#include <chrono>
#include <cstddef>
#include <cstdint>
#include <optional>
#include <string_view>

#include <userver/ugrpc/impl/code_statistics.hpp>
#include <userver/ugrpc/impl/static_metadata.hpp>

#include <userver/utils/fixed_array.hpp>
#include <userver/utils/statistics/fwd.hpp>
#include <userver/utils/statistics/percentile.hpp>
#include <userver/utils/statistics/rate_counter.hpp>
#include <userver/utils/statistics/recentperiod.hpp>

USERVER_NAMESPACE_BEGIN

namespace utils::statistics {
class StripedRateCounter;
}  // namespace utils::statistics

namespace ugrpc::impl {

enum class StatisticsDomain { kClient, kServer };

std::string_view ToString(StatisticsDomain);

class MethodStatistics final {
 public:
  explicit MethodStatistics(
      StatisticsDomain domain,
      utils::statistics::StripedRateCounter& global_started);

  void AccountStarted() noexcept;

  void AccountStatus(grpc::StatusCode code) noexcept;

  void AccountTiming(std::chrono::milliseconds timing) noexcept;

  // All errors without gRPC status codes are categorized as "network errors".
  // See server::RpcInterruptedError.
  void AccountNetworkError() noexcept;

  // Occurs when the service forgot to finish a request, oftentimes due to a
  // thrown exception. Always indicates a programming error in our service.
  // UNKNOWN status code is automatically returned in this case.
  void AccountInternalError() noexcept;

  void AccountCancelledByDeadlinePropagation() noexcept;

  void AccountDeadlinePropagated() noexcept;

  void AccountCancelled() noexcept;

  friend void DumpMetric(utils::statistics::Writer& writer,
                         const MethodStatistics& stats);

  std::uint64_t GetStarted() const noexcept;

  void MoveStartedTo(MethodStatistics& other) noexcept;

 private:
  using Percentile =
      utils::statistics::Percentile<2000, std::uint32_t, 256, 100>;
  using Timings = utils::statistics::RecentPeriod<Percentile, Percentile>;
  using RateCounter = utils::statistics::RateCounter;

  friend struct MethodStatisticsSnapshot;

  const StatisticsDomain domain_;
  utils::statistics::StripedRateCounter& global_started_;

  RateCounter started_{0};
  RateCounter started_renamed_{0};
  CodeStatistics status_codes_{};
  Timings timings_;
  RateCounter network_errors_{0};
  RateCounter internal_errors_{0};
  RateCounter cancelled_{0};

  RateCounter deadline_updated_{0};
  RateCounter deadline_cancelled_{0};
};

struct MethodStatisticsSnapshot final {
  using Rate = utils::statistics::Rate;

  explicit MethodStatisticsSnapshot(const MethodStatistics& stats);

  explicit MethodStatisticsSnapshot(const StatisticsDomain domain);

  void Add(const MethodStatisticsSnapshot& other);

  StatisticsDomain domain;

  Rate started{0};
  Rate started_renamed{0};
  CodeStatistics::Snapshot status_codes{};
  MethodStatistics::Percentile timings;
  Rate network_errors{0};
  Rate internal_errors{0};
  Rate cancelled{0};

  Rate deadline_updated{0};
  Rate deadline_cancelled{0};
};

void DumpMetric(utils::statistics::Writer& writer,
                const MethodStatisticsSnapshot& stats);

void DumpMetricWithLabels(utils::statistics::Writer& writer,
                          const MethodStatisticsSnapshot& stats,
                          std::optional<std::string_view> client_name,
                          std::string_view call_name,
                          std::string_view service_name);

class ServiceStatistics final {
 public:
  ServiceStatistics(const StaticServiceMetadata& metadata,
                    StatisticsDomain domain,
                    utils::statistics::StripedRateCounter& global_started);

  ~ServiceStatistics();

  MethodStatistics& GetMethodStatistics(std::size_t method_id);
  const MethodStatistics& GetMethodStatistics(std::size_t method_id) const;

  const StaticServiceMetadata& GetMetadata() const;

  std::uint64_t GetStartedRequests() const;

  void DumpAndCountTotal(utils::statistics::Writer& writer,
                         std::optional<std::string_view> client_name,
                         MethodStatisticsSnapshot& total) const;

 private:
  const StaticServiceMetadata metadata_;
  utils::FixedArray<MethodStatistics> method_statistics_;
};

}  // namespace ugrpc::impl

USERVER_NAMESPACE_END
