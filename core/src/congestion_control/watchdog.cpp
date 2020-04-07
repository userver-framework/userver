#include <congestion_control/watchdog.hpp>

namespace congestion_control {

Watchdog::Watchdog()
    : should_stop_(false), thread_([this] {
        while (!should_stop_.load()) {
          Check();
          std::this_thread::sleep_for(std::chrono::seconds(1));
        }
      }) {}

Watchdog::~Watchdog() {
  should_stop_ = true;
  thread_.join();
}

void Watchdog::Register(ControllerInfo ci) {
  auto cis = cis_.Lock();
  cis->push_back(ci);
}

void Watchdog::Check() {
  auto cis = cis_.Lock();
  for (const auto& ci : *cis) {
    auto data = ci.sensor.FetchCurrent();
    ci.controller.Feed(data);
    auto limit = ci.controller.GetLimit();
    ci.limiter.SetLimit(limit);
  }
}

}  // namespace congestion_control
