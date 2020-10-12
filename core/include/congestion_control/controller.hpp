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

  double start_limit_factor{0.75};
};

Policy MakePolicy(formats::json::Value policy);

struct PolicyState {
  size_t times_with_overload{0};
  size_t times_wo_overload{0};
  bool is_overloaded{false};
  std::optional<size_t> current_limit;

  size_t max_up_delta{1};
};

struct Stats final {
  std::atomic<size_t> no_limit{0};
  std::atomic<size_t> not_overload_no_pressure{0};
  std::atomic<size_t> not_overload_pressure{0};
  std::atomic<size_t> overload_no_pressure{0};
  std::atomic<size_t> overload_pressure{0};

  std::atomic<size_t> current_state{0};
};

class Controller final {
 public:
  Controller(std::string name, Policy policy);

  void Feed(const Sensor::Data&);

  Limit GetLimit() const;

  Limit GetLimitRaw() const;

  // Thread-safe
  void SetPolicy(const Policy& policy);

  void SetEnabled(bool enabled);

  bool IsEnabled() const;

  const Stats& GetStats() const;

 private:
  bool IsOverloadedNow(const Sensor::Data& data, const Policy& policy) const;

  size_t CalcNewLimit(const Sensor::Data& data, const Policy& policy) const;

  const std::string name_;
  Limit limit_;
  concurrent::Variable<Policy, std::mutex> policy_;
  PolicyState state_;
  std::atomic<bool> is_enabled_;

  Stats stats_;
};

struct ControllerInfo {
  Sensor& sensor;
  Limiter& limiter;
  Controller& controller;
};

}  // namespace congestion_control
