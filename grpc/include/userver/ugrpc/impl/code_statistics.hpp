#pragma once

#include <array>

#include <grpcpp/support/status.h>

#include <userver/utils/statistics/fwd.hpp>
#include <userver/utils/statistics/rate.hpp>
#include <userver/utils/statistics/rate_counter.hpp>
#include <userver/utils/statistics/writer.hpp>

USERVER_NAMESPACE_BEGIN

namespace ugrpc::impl {

struct CodeStatisticsSummary final {
  utils::statistics::Rate total_requests{0};
  utils::statistics::Rate error_requests{0};
};

class CodeStatistics final {
 public:
  class Snapshot;

  CodeStatistics() = default;
  CodeStatistics(const CodeStatistics&) = delete;
  CodeStatistics& operator=(const CodeStatistics&) = delete;

  void Account(grpc::StatusCode code) noexcept;

 private:
  // StatusCode enum cases have consecutive underlying values, starting from 0.
  // UNAUTHENTICATED currently has the largest value.
  static constexpr std::size_t kCodesCount =
      static_cast<std::size_t>(grpc::StatusCode::UNAUTHENTICATED) + 1;

  std::array<utils::statistics::RateCounter, kCodesCount> codes_;
};

class CodeStatistics::Snapshot final {
 public:
  Snapshot() = default;
  Snapshot(const Snapshot&) = default;
  Snapshot& operator=(const Snapshot&) = default;

  explicit Snapshot(const CodeStatistics& other) noexcept;

  Snapshot& operator+=(const Snapshot& other);

  void DumpMetric(utils::statistics::Writer& writer, const Snapshot& snapshot);

  CodeStatisticsSummary DumpMetricHelper(
      utils::statistics::Writer& writer) const;

 private:
  std::array<utils::statistics::Rate, kCodesCount> codes_;
};

}  // namespace ugrpc::impl

USERVER_NAMESPACE_END
