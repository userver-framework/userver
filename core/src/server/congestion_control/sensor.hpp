#pragma once

#include <congestion_control/sensor.hpp>
#include <engine/task/task_processor_fwd.hpp>
#include <server/server.hpp>
#include <utils/statistics/recentperiod.hpp>

namespace server::congestion_control {

class Sensor final : public ::congestion_control::Sensor {
 public:
  Sensor(const Server& server, engine::TaskProcessor& tp);

  Data FetchCurrent() override;

 private:
  const Server& server_;
  engine::TaskProcessor& tp_;

  std::chrono::steady_clock::time_point last_fetch_tp_;
  size_t last_overloads_;
  size_t last_no_overloads_;
  size_t last_requests_;
};

}  // namespace server::congestion_control
