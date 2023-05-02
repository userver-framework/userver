#pragma once

#include <optional>

#include <userver/congestion_control/sensor.hpp>

USERVER_NAMESPACE_BEGIN

namespace storages::mongo {

namespace impl {
class PoolImpl;
}

namespace cc {

class Sensor final : public congestion_control::v2::Sensor {
 public:
  explicit Sensor(impl::PoolImpl& pool);

  Data GetCurrent() override;

 private:
  impl::PoolImpl& pool_;
  int64_t last_total_queries{};
  int64_t last_timeouted_queries{};
  int64_t last_timings_sum{};
};

}  // namespace cc

}  // namespace storages::mongo

USERVER_NAMESPACE_END
