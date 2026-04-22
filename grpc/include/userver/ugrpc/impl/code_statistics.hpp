#pragma once

#include <grpcpp/support/status.h>

#include <userver/utils/statistics/fwd.hpp>
#include <userver/utils/statistics/rate.hpp>
#include <userver/utils/statistics/rate_counter.hpp>
#include <userver/utils/statistics/writer.hpp>

USERVER_NAMESPACE_BEGIN

namespace ugrpc::impl {

struct CodeStatisticsSummary final {
    utils::statistics::Rate total_requests{};
    utils::statistics::Rate error_requests{};
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
    static constexpr std::size_t kCodesCount = static_cast<std::size_t>(grpc::StatusCode::UNAUTHENTICATED) + 1;

    utils::statistics::RateCounter codes_[kCodesCount]{};
};

class CodeStatistics::Snapshot final {
public:
    Snapshot() = default;
    Snapshot(const Snapshot&) = default;
    Snapshot& operator=(const Snapshot&) = default;

    explicit Snapshot(const CodeStatistics& other) noexcept;

    Snapshot& operator+=(const Snapshot& other);

    CodeStatisticsSummary DumpMetricAndGetSummary(utils::statistics::Writer& writer) const;

private:
    utils::statistics::Rate codes_[kCodesCount]{};
};

}  // namespace ugrpc::impl

USERVER_NAMESPACE_END
