#pragma once

#include <cstdint>

#include <userver/congestion_control/sensor.hpp>
#include <userver/engine/task/task_processor_fwd.hpp>
#include <userver/server/server.hpp>
#include <userver/utils/statistics/recentperiod.hpp>

USERVER_NAMESPACE_BEGIN

namespace server::congestion_control {

class Sensor final : public USERVER_NAMESPACE::congestion_control::Sensor {
 public:
  Sensor(const Server& server, engine::TaskProcessor& tp);

  Data FetchCurrent() override;

 private:
  const Server& server_;
  engine::TaskProcessor& tp_;

  std::chrono::steady_clock::time_point last_fetch_tp_;
  std::uint64_t last_overloads_{0};
  std::uint64_t last_no_overloads_{0};
  std::uint64_t last_requests_{0};
};

}  // namespace server::congestion_control

USERVER_NAMESPACE_END
