#pragma once

#include <optional>

#include <userver/congestion_control/sensor.hpp>

USERVER_NAMESPACE_BEGIN

namespace storages::postgres {

namespace detail {
class ConnectionPool;
}

namespace cc {

class Sensor final : public congestion_control::v2::Sensor {
 public:
  explicit Sensor(detail::ConnectionPool& pool);

  Data GetCurrent() override;

 private:
  detail::ConnectionPool& pool_;
  std::size_t last_total_queries{0};
  std::size_t last_timeouted_queries{0};
};

}  // namespace cc

}  // namespace storages::postgres

USERVER_NAMESPACE_END
