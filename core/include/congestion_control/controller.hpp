#pragma once

#include <concurrent/variable.hpp>
#include <congestion_control/limiter.hpp>
#include <congestion_control/sensor.hpp>
#include <formats/json/value.hpp>

namespace congestion_control {

struct Policy {
  size_t min_limit{2};
  double up_rate_percent{2};
  double down_rate_percent{5};

  size_t overload_on{10};
  size_t overload_off{3};

  size_t up_count{3};
  size_t down_count{3};
  size_t no_limit_count{1000};
};

Policy MakePolicy(formats::json::Value policy);

struct PolicyState {
  size_t times_with_overload{0};
  size_t times_wo_overload{0};
  bool is_overloaded{false};
  std::optional<size_t> current_limit;
};

class Controller final {
 public:
  Controller(std::string name, Policy policy);

  void Feed(const Sensor::Data&);

  Limit GetLimit() const;

  // Thread-safe
  void SetPolicy(const Policy& policy);

  void SetEnabled(bool enabled);

 private:
  bool IsOverloadedNow(const Sensor::Data& data, const Policy& policy) const;

  size_t CalcNewLimit(const Sensor::Data& data, const Policy& policy) const;

  const std::string name_;
  Limit limit_;
  concurrent::Variable<Policy, std::mutex> policy_;
  PolicyState state_;
  std::atomic<bool> is_enabled_;
};

struct ControllerInfo {
  Sensor& sensor;
  Limiter& limiter;
  Controller& controller;
};

}  // namespace congestion_control
