#pragma once

#include <cstdint>

#include <userver/congestion_control/sensor.hpp>
#include <userver/engine/task/task_processor_fwd.hpp>
#include <userver/rcu/rcu.hpp>
#include <userver/utils/statistics/recentperiod.hpp>

USERVER_NAMESPACE_BEGIN

namespace server::congestion_control {

class RequestsSource {
 public:
  virtual ~RequestsSource() = default;

  virtual std::uint64_t GetTotalRequests() const = 0;
};

class Sensor final : public USERVER_NAMESPACE::congestion_control::Sensor {
 public:
  explicit Sensor(engine::TaskProcessor& tp);

  Data FetchCurrent() override;

  void RegisterRequestsSource(RequestsSource& source);

 private:
  engine::TaskProcessor& tp_;
  rcu::Variable<std::vector<RequestsSource*>> requests_sources_;

  std::chrono::steady_clock::time_point last_fetch_tp_;
  std::uint64_t last_overloads_{0};
  std::uint64_t last_no_overloads_{0};
  std::uint64_t last_requests_{0};
};

}  // namespace server::congestion_control

USERVER_NAMESPACE_END
