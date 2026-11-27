#pragma once

#include <optional>
#include <string>
#include <string_view>
#include <unordered_map>

#include <userver/concurrent/variable.hpp>
#include <userver/logging/fwd.hpp>
#include <userver/logging/level.hpp>
#include <userver/logging/telemetry_settings.hpp>
#include <userver/utils/default_dict.hpp>

USERVER_NAMESPACE_BEGIN

namespace logging::impl {

class TelemetrySettingsStorage final {
public:
    void RegisterLogger(const std::string& name, LoggerPtr logger);

    void Apply(utils::DefaultDict<TelemetrySettings> settings, utils::DefaultDict<TelemetrySettings> restrictions);

    void Restore();

private:
    struct LoggerState {
        LoggerPtr logger;
        Level initial_level;
        std::optional<double> initial_sampling_probability;
    };

    struct Policy {
        utils::DefaultDict<TelemetrySettings> settings;
        utils::DefaultDict<TelemetrySettings> restrictions;

        TelemetrySettings GetEffectiveSettings(std::string_view name) const;
    };

    struct State {
        std::unordered_map<std::string, LoggerState> loggers;
        std::optional<Policy> policy;
    };

    concurrent::Variable<State> state_;
};

}  // namespace logging::impl

USERVER_NAMESPACE_END
