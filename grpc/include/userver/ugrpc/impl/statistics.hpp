#pragma once

#include <array>
#include <atomic>
#include <chrono>
#include <cstddef>
#include <cstdint>

#include <grpcpp/support/status.h>

#include <userver/utils/fixed_array.hpp>
#include <userver/utils/statistics/fwd.hpp>
#include <userver/utils/statistics/percentile.hpp>
#include <userver/utils/statistics/recentperiod.hpp>

#include <userver/ugrpc/impl/static_metadata.hpp>

USERVER_NAMESPACE_BEGIN

namespace ugrpc::impl {

class MethodStatistics final {
 public:
  MethodStatistics();

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

  friend void DumpMetric(utils::statistics::Writer& writer,
                         const MethodStatistics& stats);

 private:
  using Percentile =
      utils::statistics::Percentile<2000, std::uint32_t, 256, 100>;
  using Counter = std::atomic<std::uint64_t>;

  // StatusCode enum cases have consecutive underlying values, starting from 0.
  // UNAUTHENTICATED currently has the largest value.
  static constexpr std::size_t kCodesCount =
      static_cast<std::size_t>(grpc::StatusCode::UNAUTHENTICATED) + 1;

  Counter started_{0};
  std::array<Counter, kCodesCount> status_codes_{};
  utils::statistics::RecentPeriod<Percentile, Percentile> timings_;
  Counter network_errors_{0};
  Counter internal_errors_{0};
};

class ServiceStatistics final {
 public:
  explicit ServiceStatistics(const StaticServiceMetadata& metadata);

  ~ServiceStatistics();

  MethodStatistics& GetMethodStatistics(std::size_t method_id);
  const MethodStatistics& GetMethodStatistics(std::size_t method_id) const;

  const StaticServiceMetadata& GetMetadata() const;

  friend void DumpMetric(utils::statistics::Writer& writer,
                         const ServiceStatistics& stats);

 private:
  const StaticServiceMetadata metadata_;
  utils::FixedArray<MethodStatistics> method_statistics_;
};

}  // namespace ugrpc::impl

USERVER_NAMESPACE_END
