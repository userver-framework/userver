#pragma once

#include <atomic>
#include <chrono>
#include <cstddef>
#include <optional>
#include <string>

#include <userver/congestion_control/limiter.hpp>
#include <userver/congestion_control/sensor.hpp>
#include <userver/dynamic_config/source.hpp>
#include <userver/formats/json_fwd.hpp>

#include <dynamic_config/variables/USERVER_RPS_CCONTROL.hpp>

USERVER_NAMESPACE_BEGIN

/// Congestion Control
namespace congestion_control {

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
    std::atomic<std::chrono::seconds> last_overload_pressure{
        std::chrono::duration_cast<std::chrono::seconds>(std::chrono::steady_clock::now().time_since_epoch()) -
        std::chrono::hours(1)};
};

using Policy = ::dynamic_config::userver_rps_ccontrol::VariableType;

class Controller final {
public:
    Controller(std::string name, dynamic_config::Source config_source);

    void Feed(const Sensor::Data&);

    Limit GetLimit() const;

    Limit GetLimitRaw() const;

    void SetEnabled(bool enabled);

    bool IsEnabled() const;

    const Stats& GetStats() const;

private:
    bool IsOverloadedNow(const Sensor::Data& data, const Policy& policy) const;

    size_t CalcNewLimit(const Sensor::Data& data, const Policy& policy) const;

    static bool IsThresholdReached(const Sensor::Data& data, int percent);

    const std::string name_;
    Limit limit_;
    dynamic_config::Source config_source_;
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

USERVER_NAMESPACE_END
