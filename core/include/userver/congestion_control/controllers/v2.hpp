#pragma once

#include <atomic>
#include <optional>

#include <userver/congestion_control/limiter.hpp>
#include <userver/congestion_control/sensor.hpp>
#include <userver/utils/periodic_task.hpp>
#include <userver/utils/statistics/fwd.hpp>

USERVER_NAMESPACE_BEGIN

namespace congestion_control::v2 {

struct Stats {
  std::atomic<bool> is_enabled{false};
  std::atomic<bool> is_fake_mode{false};
  std::atomic<int64_t> current_limit{0};
  std::atomic<int64_t> enabled_epochs{0};
};

void DumpMetric(utils::statistics::Writer& writer, const Stats& stats);

class Controller {
 public:
  struct Config {
    bool fake_mode{false};
    bool enabled{true};
  };

  Controller(const std::string& name, v2::Sensor& sensor, Limiter& limiter,
             Stats& stats, const Config& config);

  virtual ~Controller() = default;

  void Start();

  void Step();

  const std::string& GetName() const;

  void SetEnabled(bool enabled);

 protected:
  virtual Limit Update(const v2::Sensor::Data& data) = 0;

  // NOLINTNEXTLINE(misc-non-private-member-variables-in-classes)
  std::optional<size_t> current_limit_;

 private:
  std::string_view LogFakeMode() const;

  const std::string name_;
  Sensor& sensor_;
  congestion_control::Limiter& limiter_;
  Stats& stats_;
  const Config config_;
  USERVER_NAMESPACE::utils::PeriodicTask periodic_;
  std::atomic<bool> enabled_{true};
};

}  // namespace congestion_control::v2

USERVER_NAMESPACE_END
