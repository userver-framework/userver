#pragma once

#include <optional>

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

AccumulatedData operator-(const AccumulatedData& lhs,
                          const AccumulatedData& rhs) noexcept;

class Sensor final : public congestion_control::v2::Sensor {
 public:
  explicit Sensor(impl::PoolImpl& pool);

  Data GetCurrent() override;

 private:
  impl::PoolImpl& pool_;
  AccumulatedData last_data_;
};

}  // namespace cc

}  // namespace storages::mongo

USERVER_NAMESPACE_END
