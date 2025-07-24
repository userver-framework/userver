#pragma once

#include <unordered_map>

#include <userver/congestion_control/sensor.hpp>

USERVER_NAMESPACE_BEGIN

namespace storages::mongo {

namespace impl {
class PoolImpl;
}

namespace cc {

struct AccumulatedData final {
    std::uint64_t total_queries{0};
    std::uint64_t timeouts{0};
    std::uint64_t timings_sum{0};
};

using AccumulatedDataByCollection = std::unordered_map<std::string, AccumulatedData>;

AccumulatedData operator-(const AccumulatedData& lhs, const AccumulatedData& rhs) noexcept;
bool operator<(const AccumulatedData& lhs, const AccumulatedData& rhs) noexcept;

AccumulatedDataByCollection
operator-(const AccumulatedDataByCollection& lhs, const AccumulatedDataByCollection& rhs) noexcept;

class Sensor final : public congestion_control::v2::Sensor {
public:
    explicit Sensor(impl::PoolImpl& pool);

    Data GetCurrent() override;

private:
    impl::PoolImpl& pool_;
    AccumulatedDataByCollection last_data_by_collection_;
};

}  // namespace cc

}  // namespace storages::mongo

USERVER_NAMESPACE_END
