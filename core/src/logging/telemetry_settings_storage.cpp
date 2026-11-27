#include <logging/telemetry_settings_storage.hpp>

#include <algorithm>
#include <optional>
#include <utility>

#include <userver/logging/impl/logger_base.hpp>
#include <userver/utils/assert.hpp>

USERVER_NAMESPACE_BEGIN

namespace logging::impl {
namespace {

void ApplySettings(LoggerRef logger, Level initial_level, const TelemetrySettings& settings) {
    logger.SetLevel(std::max(initial_level, settings.min_log_level));
    logger.SetSamplingProbability(
        settings.log_sample_rate == 1.0 ? std::nullopt : std::optional{settings.log_sample_rate}
    );
}

void ValidateSettings(const utils::DefaultDict<TelemetrySettings>& settings) {
    for (const auto& [name, value] : settings) {
        value.Validate();
    }
}

}  // namespace

TelemetrySettings TelemetrySettingsStorage::Policy::GetEffectiveSettings(std::string_view name) const {
    return settings.GetOptional(name)
        .value_or(TelemetrySettings{})
        .Merge(restrictions.GetOptional(name).value_or(TelemetrySettings{}));
}

void TelemetrySettingsStorage::RegisterLogger(const std::string& name, LoggerPtr logger) {
    UINVARIANT(logger, "Cannot register a null logger for telemetry settings");
    const auto initial_level = logger->GetLevel();
    const auto initial_sampling_probability = logger->GetSamplingProbability();
    auto state = state_.Lock();
    const auto [it, inserted] =
        state->loggers.emplace(name, LoggerState{std::move(logger), initial_level, initial_sampling_probability});
    UINVARIANT(inserted, "Logger is already registered for telemetry settings");
    if (state->policy) {
        ApplySettings(*it->second.logger, it->second.initial_level, state->policy->GetEffectiveSettings(name));
    }
}

void TelemetrySettingsStorage::Apply(
    utils::DefaultDict<TelemetrySettings> settings,
    utils::DefaultDict<TelemetrySettings> restrictions
) {
    ValidateSettings(settings);
    ValidateSettings(restrictions);
    auto state = state_.Lock();
    state->policy = Policy{std::move(settings), std::move(restrictions)};
    for (const auto& [name, logger] : state->loggers) {
        ApplySettings(*logger.logger, logger.initial_level, state->policy->GetEffectiveSettings(name));
    }
}

void TelemetrySettingsStorage::Restore() {
    auto state = state_.Lock();
    if (!state->policy) {
        return;
    }
    for (const auto& [name, logger] : state->loggers) {
        logger.logger->SetLevel(logger.initial_level);
        logger.logger->SetSamplingProbability(logger.initial_sampling_probability);
    }
    state->policy.reset();
}

}  // namespace logging::impl

USERVER_NAMESPACE_END
