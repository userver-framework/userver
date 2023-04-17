#pragma once

#include <optional>

#include <userver/congestion_control/controllers/v2.hpp>
#include <userver/congestion_control/limiter.hpp>
#include <userver/utils/periodic_task.hpp>

USERVER_NAMESPACE_BEGIN

namespace congestion_control::v2 {

class LinearController final : public Controller {
 public:
  using Controller::Controller;

  Limit Update(Sensor::Data& current) override;

 private:
  std::optional<size_t> current_limit_;
};

}  // namespace congestion_control::v2

USERVER_NAMESPACE_END
