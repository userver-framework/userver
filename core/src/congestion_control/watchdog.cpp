#include <congestion_control/watchdog.hpp>

#include <userver/engine/task/task.hpp>
#include <userver/formats/parse/common_containers.hpp>
#include <userver/logging/log.hpp>
#include <userver/testsuite/testpoint.hpp>
#include <userver/utils/thread_name.hpp>

USERVER_NAMESPACE_BEGIN

namespace congestion_control {

namespace {
const auto kThreadName = "cc-watchdog";
}

Watchdog::Watchdog()
    : should_stop_(false),
      tp_(engine::current_task::GetTaskProcessor()),
      thread_([this] {
          utils::SetCurrentThreadName(kThreadName);
          while (!should_stop_.load()) {
              Check();
              std::this_thread::sleep_for(std::chrono::seconds(1));
          }
      })
{}

Watchdog::~Watchdog() { Stop(); }

void Watchdog::Register(ControllerInfo ci) {
    auto cis = cis_.Lock();
    cis->push_back(ci);
}

void Watchdog::Stop() {
    should_stop_ = true;
    if (thread_.joinable()) {
        thread_.join();
    }
}

void Watchdog::Check() {
    auto cis = cis_.Lock();
    for (const auto& ci : *cis) {
        auto data = ci.sensor.FetchCurrent();

        TESTPOINT_CALLBACK_NONCORO(
            "congestion-control-sensor",
            formats::json::Value{},
            tp_,
            [&data](const formats::json::Value& doc) {
                data.current_load = doc["current-load"].As<std::uint64_t>(data.current_load);
                data.overload_events_count = doc["overload-events-count"].As<std::uint64_t>(data.overload_events_count);
                data.no_overload_events_count =
                    doc["no-overload-events-count"].As<std::uint64_t>(data.no_overload_events_count);
                LOG_INFO() << "Forcing CC Sensor data from testpoint: " << doc;
            }
        );

        ci.controller.Feed(data);

        auto limit = ci.controller.GetLimit();

        TESTPOINT_CALLBACK_NONCORO(
            "congestion-control",
            formats::json::Value{},
            tp_,
            [&limit](const formats::json::Value& doc) {
                if (doc.HasMember("force-rps-limit")) {
                    limit.load_limit = doc["force-rps-limit"].As<std::optional<std::size_t>>();
                    LOG_ERROR() << "Forcing RPS limit from testpoint: " << limit.load_limit;
                }
            }
        );

        ci.limiter.SetLimit(limit);

        TESTPOINT_NONCORO("congestion-control-apply", formats::json::Value{}, tp_);
    }
}

}  // namespace congestion_control

USERVER_NAMESPACE_END
