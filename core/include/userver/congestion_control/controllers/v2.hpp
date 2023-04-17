#pragma once

#include <optional>

#include <userver/congestion_control/limiter.hpp>
#include <userver/congestion_control/sensor.hpp>
#include <userver/utils/periodic_task.hpp>

USERVER_NAMESPACE_BEGIN

namespace congestion_control::v2 {

class Controller {
 public:
  Controller(v2::Sensor& sensor, Limiter& limiter);

  virtual ~Controller() = default;

  void Start();

  void Step();

 protected:
  virtual Limit Update(v2::Sensor::Data& data) = 0;

  // NOLINTNEXTLINE(misc-non-private-member-variables-in-classes)
  std::optional<size_t> current_limit_;

 private:
  Sensor& sensor_;
  congestion_control::Limiter& limiter_;
  USERVER_NAMESPACE::utils::PeriodicTask periodic_;
};

}  // namespace congestion_control::v2

USERVER_NAMESPACE_END
