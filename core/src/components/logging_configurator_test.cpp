#include <userver/components/logging_configurator.hpp>

#include <limits>
#include <optional>
#include <stdexcept>
#include <string>
#include <string_view>
#include <utility>

#include <gmock/gmock.h>

#include <components/component_list_test.hpp>
#include <userver/components/component_context.hpp>
#include <userver/components/minimal_component_list.hpp>
#include <userver/components/run.hpp>
#include <userver/dynamic_config/storage/component.hpp>
#include <userver/dynamic_config/value.hpp>
#include <userver/formats/json/serialize.hpp>
#include <userver/formats/yaml/serialize.hpp>
#include <userver/formats/yaml/value_builder.hpp>
#include <userver/fs/blocking/read.hpp>
#include <userver/fs/blocking/temp_directory.hpp>
#include <userver/logging/component.hpp>
#include <userver/logging/impl/logger_base.hpp>
#include <userver/logging/log.hpp>
#include <userver/logging/null_logger.hpp>
#include <userver/logging/telemetry_settings.hpp>
#include <userver/tracing/span.hpp>
#include <userver/tracing/tracer.hpp>
#include <userver/utest/utest.hpp>

USERVER_NAMESPACE_BEGIN

namespace {

struct TelemetryTestState {
    explicit TelemetryTestState(const components::ComponentContext& context)
        : logging(context.FindComponent<components::LoggingConfigurator>()),
          config(context.FindComponent<components::DynamicConfig>()),
          loggers(context.FindComponent<components::Logging>())
    {}

    void SetSettings(std::string_view value) { SetConfig("USERVER_TELEMETRY_SETTINGS", value); }

    void SetConfig(const std::string& name, std::string_view value) {
        auto docs = config.GetDefaultDocsMap();
        docs.Set(name, formats::json::FromString(value));
        static_cast<components::DynamicConfigUpdatesSinkBase&>(config).SetConfig("telemetry-verifier", std::move(docs));
    }

    components::LoggingConfigurator& logging;
    components::DynamicConfig& config;
    components::Logging& loggers;
};

template <auto Scenario>
class TelemetryVerifier final : public components::RawComponentBase {
public:
    static constexpr std::string_view kName = "telemetry-verifier";

    TelemetryVerifier(const components::ComponentConfig&, const components::ComponentContext& context)
        : state_(context)
    {}

    void OnAllComponentsLoaded() override { Scenario(state_); }

private:
    TelemetryTestState state_;
};

struct ScenarioSettings {
    bool configure_default_logger{true};
    std::string default_logger_path{"@null"};
    double initial_log_sample_rate{0.25};
};

template <auto Scenario>
void RunScenario(const ScenarioSettings& settings = {}) {
    const auto initial_logs = logging::GetDefaultLogger().GetSamplingProbability();
    const auto initial_traces = tracing::GetTraceSamplingProbability();
    auto config = tests::MergeYaml(tests::kMinimalStaticConfig, R"(
components_manager:
  components:
    logging:
      loggers:
        default:
          level: info
        access:
          file_path: '@null'
          level: info
        audit:
          file_path: '@null'
          level: info
    logging-configurator:
      limited-logging-enable: true
      limited-logging-interval: 1s
    dynamic-config:
      updates-enabled: false
      defaults:
# /// [telemetry-settings]
# yaml
        USERVER_TELEMETRY_SETTINGS:
          logging:
            default:
              min_log_level: trace
              log_sample_rate: 0.25
          tracing:
            trace_sample_rate: 0.5
# /// [telemetry-settings]
    telemetry-verifier:
)");
    formats::yaml::ValueBuilder builder{formats::yaml::FromString(config)};
    auto components = builder["components_manager"]["components"];
    if (settings.configure_default_logger) {
        components["logging"]["loggers"]["default"]["file_path"] = settings.default_logger_path;
    } else {
        components["logging"]["loggers"].Remove("default");
    }
    components["dynamic-config"]["defaults"]["USERVER_TELEMETRY_SETTINGS"]["logging"]["default"]["log_sample_rate"] =
        settings.initial_log_sample_rate;
    config = formats::yaml::ToString(builder.ExtractValue());
    components::RunOnce(
        components::InMemoryConfig{config},
        components::MinimalComponentList()
            .Append<components::LoggingConfigurator>()
            .Append<TelemetryVerifier<Scenario>>()
    );
    EXPECT_EQ(logging::GetDefaultLogger().GetSamplingProbability(), initial_logs);
    EXPECT_EQ(tracing::GetTraceSamplingProbability(), initial_traces);
}

}  // namespace

TEST_F(ComponentList, TelemetryInitialSettingsFilterDefaultLoggerOutput) {
    static constexpr auto kSampledMessage = "telemetry-startup-sampled-info";
    static constexpr auto kPreservedMessage = "telemetry-startup-preserved-warning";
    const auto directory = fs::blocking::TempDirectory::Create();
    const auto log_path = directory.GetPath() + "/telemetry.log";
    RunScenario<[](TelemetryTestState&) {
        LOG_INFO() << kSampledMessage;
        LOG_WARNING() << kPreservedMessage;
        logging::LogFlush();
    }>({.default_logger_path = log_path, .initial_log_sample_rate = 0.0});
    const auto logs = fs::blocking::ReadFileContents(log_path);
    EXPECT_THAT(logs, testing::Not(testing::HasSubstr(kSampledMessage)));
    EXPECT_THAT(logs, testing::HasSubstr(kPreservedMessage));
}

TEST_F(ComponentList, TelemetryOrdinaryConfigAppliesWithoutAdapter) {
    RunScenario<[](TelemetryTestState& state) {
        EXPECT_EQ(logging::GetDefaultLogger().GetSamplingProbability(), 0.25);
        EXPECT_EQ(tracing::GetTraceSamplingProbability(), 0.5);
        state.SetSettings(R"({
            "logging": {
                "default": {"min_log_level":"warning","log_sample_rate":0.125}
            },
            "tracing": {"trace_sample_rate":0.25}
        })");
        EXPECT_FALSE(logging::ShouldLog(logging::Level::kInfo));
        EXPECT_TRUE(logging::ShouldLog(logging::Level::kWarning));
        EXPECT_EQ(logging::GetDefaultLogger().GetSamplingProbability(), 0.125);
        EXPECT_EQ(tracing::GetTraceSamplingProbability(), 0.25);
    }>();
}

TEST_F(ComponentList, TelemetryRestrictionsNeverIncreaseTelemetry) {
    RunScenario<[](TelemetryTestState& state) {
        state.SetSettings(R"({
            "logging": {
                "default": {"min_log_level":"error","log_sample_rate":0.125}
            },
            "tracing": {"trace_sample_rate":0.25}
        })");
        state.logging.SetRestrictions(
            {.loggers = {{"__default__", {.min_log_level = logging::Level::kWarning, .log_sample_rate = 0.5}}},
             .tracing = {.trace_sample_rate = 0.125}}
        );
        EXPECT_EQ(logging::GetDefaultLoggerLevel(), logging::Level::kError);
        EXPECT_EQ(logging::GetDefaultLogger().GetSamplingProbability(), 0.125);
        EXPECT_EQ(tracing::GetTraceSamplingProbability(), 0.125);
    }>();
}

TEST_F(ComponentList, TelemetryNeutralRestrictionValuesPreserveSampling) {
    RunScenario<[](TelemetryTestState& state) {
        state.logging
            .SetRestrictions({.loggers = {{"__default__", {.min_log_level = logging::Level::kWarning}}}, .tracing = {}}
            );
        EXPECT_EQ(logging::GetDefaultLogger().GetSamplingProbability(), 0.25);
        EXPECT_EQ(tracing::GetTraceSamplingProbability(), 0.5);
    }>();
}

TEST_F(ComponentList, TelemetryConfigUpdatePreservesRestrictions) {
    RunScenario<
        [](TelemetryTestState& state) {
            state.logging.SetRestrictions(
                {.loggers = {{"__default__", {.min_log_level = logging::Level::kWarning}}},
                 .tracing = {.trace_sample_rate = 0.125}}
            );
            state
                .SetSettings(
                    R"({"logging":{"default":{"min_log_level":"trace","log_sample_rate":0.5}},"tracing":{"trace_sample_rate":0.25}})"
                );
            EXPECT_EQ(logging::GetDefaultLogger().GetSamplingProbability(), 0.5);
            EXPECT_EQ(tracing::GetTraceSamplingProbability(), 0.125);
            state.logging.SetRestrictions({});
            EXPECT_EQ(logging::GetDefaultLoggerLevel(), logging::Level::kInfo);
            EXPECT_EQ(tracing::GetTraceSamplingProbability(), 0.25);
        }>();
}

TEST_F(ComponentList, TelemetryRemovingRestrictionsRestoresInitialLevel) {
    RunScenario<[](TelemetryTestState& state) {
        state.logging
            .SetRestrictions({.loggers = {{"__default__", {.min_log_level = logging::Level::kWarning}}}, .tracing = {}}
            );
        EXPECT_EQ(logging::GetDefaultLoggerLevel(), logging::Level::kWarning);
        EXPECT_FALSE(logging::ShouldLog(logging::Level::kInfo));
        logging::SetDefaultLoggerLevel(logging::Level::kError);
        state.logging.SetRestrictions({});
        EXPECT_EQ(logging::GetDefaultLoggerLevel(), logging::Level::kInfo);
    }>();
}

TEST_F(ComponentList, TelemetryUnchangedPolicyPreservesRuntimeSettings) {
    RunScenario<[](TelemetryTestState& state) {
        logging::SetDefaultLoggerLevel(logging::Level::kError);
        logging::GetDefaultLogger().SetSamplingProbability(0.125);
        tracing::SetTraceSamplingProbability(0.25);
        state.SetConfig("USERVER_NO_LOG_SPANS", R"({"names":["test-span"],"prefixes":[]})");
        EXPECT_EQ(logging::GetDefaultLoggerLevel(), logging::Level::kError);
        EXPECT_EQ(logging::GetDefaultLogger().GetSamplingProbability(), 0.125);
        EXPECT_EQ(tracing::GetTraceSamplingProbability(), 0.25);

        state.SetConfig("USERVER_LOG_DYNAMIC_DEBUG", R"({"force-disabled":["unused-file"],"force-enabled":[]})");
        EXPECT_EQ(logging::GetDefaultLoggerLevel(), logging::Level::kError);
        EXPECT_EQ(logging::GetDefaultLogger().GetSamplingProbability(), 0.125);
        EXPECT_EQ(tracing::GetTraceSamplingProbability(), 0.25);

        state.SetSettings(R"({"logging":{"default":{"min_log_level":"warning","log_sample_rate":0.5}}})");
        EXPECT_EQ(logging::GetDefaultLoggerLevel(), logging::Level::kWarning);
        EXPECT_EQ(logging::GetDefaultLogger().GetSamplingProbability(), 0.5);
        EXPECT_EQ(tracing::GetTraceSamplingProbability(), std::nullopt);
    }>();
}

TEST_F(ComponentList, TelemetryRemovingOrdinarySettingsKeepsEmergencyRestrictions) {
    RunScenario<[](TelemetryTestState& state) {
        state.logging.SetRestrictions(
            {.loggers = {{"__default__", {.min_log_level = logging::Level::kWarning}}},
             .tracing = {.trace_sample_rate = 0.125}}
        );
        state.SetSettings("{}");
        EXPECT_EQ(logging::GetDefaultLoggerLevel(), logging::Level::kWarning);
        EXPECT_EQ(logging::GetDefaultLogger().GetSamplingProbability(), std::nullopt);
        EXPECT_EQ(tracing::GetTraceSamplingProbability(), 0.125);
        state.logging.SetRestrictions({});
        EXPECT_EQ(logging::GetDefaultLoggerLevel(), logging::Level::kInfo);
        EXPECT_EQ(tracing::GetTraceSamplingProbability(), std::nullopt);
    }>();
}

TEST_F(ComponentList, TelemetryInvalidUpdatesPreservePreviousSettings) {
    RunScenario<
        [](TelemetryTestState& state) {
            state.logging.SetRestrictions({.loggers = {}, .tracing = {.trace_sample_rate = 0.125}});
            for (const double invalid :
                 {-0.1, 1.1, std::numeric_limits<double>::infinity(), std::numeric_limits<double>::quiet_NaN()})
            {
                EXPECT_THROW(
                    state.logging.SetRestrictions(
                        {.loggers = {{"__default__", {.min_log_level = logging::Level::kNone}}},
                         .tracing = {.trace_sample_rate = invalid}}
                    ),
                    std::runtime_error
                );
                EXPECT_EQ(logging::GetDefaultLoggerLevel(), logging::Level::kInfo);
                EXPECT_EQ(tracing::GetTraceSamplingProbability(), 0.125);
            }
            EXPECT_ANY_THROW(
                state
                    .SetSettings(
                        R"({"logging":{"default":{"min_log_level":"trace","log_sample_rate":0}},"tracing":{"trace_sample_rate":1.5}})"
                    )
            );
            state.logging.SetRestrictions({});
            EXPECT_EQ(logging::GetDefaultLogger().GetSamplingProbability(), 0.25);
            EXPECT_EQ(tracing::GetTraceSamplingProbability(), 0.5);
        }>();
}

TEST_F(ComponentList, TelemetrySpanLocalLevelKeepsItsOverride) {
    RunScenario<[](TelemetryTestState& state) {
        state.logging
            .SetRestrictions({.loggers = {{"__default__", {.min_log_level = logging::Level::kError}}}, .tracing = {}});
        EXPECT_FALSE(logging::ShouldLog(logging::Level::kInfo));
        tracing::Span span{"local_override"};
        span.SetLocalLogLevel(logging::Level::kInfo);
        EXPECT_TRUE(logging::ShouldLog(logging::Level::kInfo));
    }>();
}

TEST_F(ComponentList, TelemetryConfigSelectsLoggerAndFallback) {
    RunScenario<[](TelemetryTestState& state) {
        const auto access = state.loggers.GetLogger("access");
        const auto audit = state.loggers.GetLogger("audit");
        state.SetSettings(R"({
            "logging": {
                "__default__": {"min_log_level":"warning","log_sample_rate":0.125},
                "access": {"min_log_level":"error","log_sample_rate":0.5},
                "default": {"min_log_level":"trace","log_sample_rate":1.0}
            }
        })");
        EXPECT_EQ(logging::GetDefaultLoggerLevel(), logging::Level::kInfo);
        EXPECT_EQ(logging::GetDefaultLogger().GetSamplingProbability(), std::nullopt);
        EXPECT_EQ(access->GetLevel(), logging::Level::kError);
        EXPECT_EQ(access->GetSamplingProbability(), 0.5);
        EXPECT_EQ(audit->GetLevel(), logging::Level::kWarning);
        EXPECT_EQ(audit->GetSamplingProbability(), 0.125);

        state.SetSettings(R"({"logging":{"__default__":{"min_log_level":"trace","log_sample_rate":0.25}}})");
        EXPECT_EQ(access->GetLevel(), logging::Level::kInfo);
        EXPECT_EQ(access->GetSamplingProbability(), 0.25);
        EXPECT_EQ(logging::GetDefaultLogger().GetSamplingProbability(), 0.25);
        state.SetSettings("{}");
        EXPECT_EQ(access->GetSamplingProbability(), std::nullopt);
        EXPECT_EQ(audit->GetSamplingProbability(), std::nullopt);
    }>();
}

TEST_F(ComponentList, TelemetryConfigAppliesToLateRegisteredLogger) {
    RunScenario<[](TelemetryTestState& state) {
        state.SetSettings(R"({"logging":{"__default__":{"min_log_level":"warning","log_sample_rate":0.25}}})");
        const auto logger = logging::impl::MakeNoopLoggerForTests();
        logging::SetLoggerLevel(*logger, logging::Level::kInfo);
        state.loggers.SetLogger("late", logger);
        EXPECT_EQ(logger->GetLevel(), logging::Level::kWarning);
        EXPECT_EQ(logger->GetSamplingProbability(), 0.25);
        logging::SetLoggerLevel(*logger, logging::Level::kError);
        state.SetSettings("{}");
        EXPECT_EQ(logging::GetLoggerLevel(*logger), logging::Level::kInfo);
    }>();
}

TEST_F(ComponentList, TelemetryInvalidLoggerMapPreservesAppliedSettings) {
    RunScenario<[](TelemetryTestState& state) {
        const auto access = state.loggers.GetLogger("access");
        state.SetSettings(R"({"logging":{"access":{"min_log_level":"trace","log_sample_rate":0.25}}})");
        for (const auto value :
             {"null",
              "[]",
              R"({"logging":null})",
              R"({"logging":[]})",
              R"({"tracing":null})",
              R"({"tracing":[]})",
              R"({"tracing":{}})",
              R"({"tracing":{"log_sample_rate":0.5}})",
              R"({"logging":{"access":{}}})",
              R"({"logging":{"access":{"min_log_level":"info"}}})",
              R"({"logging":{"access":{"log_sample_rate":0.5}}})",
              R"({"logging":{"access":{"min_log_level":"trace","log_sample_rate":1.5}}})",
              R"({"logging":{"__default__":{"min_log_level":"invalid","log_sample_rate":1.0}}})",
              R"({"logging":{"access":{"trace_sample_rate":0.5}}})"})
        {
            EXPECT_ANY_THROW(state.SetSettings(value)) << value;
            EXPECT_EQ(access->GetSamplingProbability(), 0.25);
        }
    }>();
}

TEST_F(ComponentList, TelemetryRestrictionsCombineWithEachLoggerPolicy) {
    RunScenario<[](TelemetryTestState& state) {
        const auto access = state.loggers.GetLogger("access");
        const auto audit = state.loggers.GetLogger("audit");
        state.SetSettings(R"({
            "logging": {
                "access": {"min_log_level":"error","log_sample_rate":0.125},
                "__default__": {"min_log_level":"trace","log_sample_rate":0.5}
            }
        })");
        state.logging.SetRestrictions(
            {.loggers = {{"__default__", {.min_log_level = logging::Level::kWarning, .log_sample_rate = 0.25}}},
             .tracing = {}}
        );
        EXPECT_EQ(access->GetLevel(), logging::Level::kError);
        EXPECT_EQ(access->GetSamplingProbability(), 0.125);
        EXPECT_EQ(audit->GetLevel(), logging::Level::kWarning);
        EXPECT_EQ(audit->GetSamplingProbability(), 0.25);

        state.SetSettings(R"({"logging":{"access":{"min_log_level":"trace","log_sample_rate":0.5}}})");
        EXPECT_EQ(access->GetSamplingProbability(), 0.25);
        state.logging.SetRestrictions({});
        EXPECT_EQ(access->GetLevel(), logging::Level::kInfo);
        EXPECT_EQ(access->GetSamplingProbability(), 0.5);
        EXPECT_EQ(audit->GetLevel(), logging::Level::kInfo);
        EXPECT_EQ(audit->GetSamplingProbability(), std::nullopt);
    }>();
}

TEST_F(ComponentList, TelemetryAppliesToExternallyOwnedDefaultLogger) {
    RunScenario<[](TelemetryTestState& state) {
        EXPECT_EQ(logging::GetDefaultLogger().GetSamplingProbability(), 0.25);
        state.SetSettings(R"({"logging":{"__default__":{"min_log_level":"warning","log_sample_rate":0.5}}})");
        EXPECT_EQ(logging::GetDefaultLoggerLevel(), logging::Level::kWarning);
        EXPECT_EQ(logging::GetDefaultLogger().GetSamplingProbability(), 0.5);
    }>({.configure_default_logger = false});
}

TEST_F(ComponentList, TelemetryMissingSectionsRemoveTheirOrdinaryRestrictions) {
    RunScenario<[](TelemetryTestState& state) {
        state.SetSettings(R"({"logging":{"default":{"min_log_level":"warning","log_sample_rate":0.125}}})");
        EXPECT_EQ(logging::GetDefaultLoggerLevel(), logging::Level::kWarning);
        EXPECT_EQ(logging::GetDefaultLogger().GetSamplingProbability(), 0.125);
        EXPECT_EQ(tracing::GetTraceSamplingProbability(), std::nullopt);

        state.SetSettings(R"({"tracing":{"trace_sample_rate":0.25}})");
        EXPECT_EQ(logging::GetDefaultLoggerLevel(), logging::Level::kInfo);
        EXPECT_EQ(logging::GetDefaultLogger().GetSamplingProbability(), std::nullopt);
        EXPECT_EQ(tracing::GetTraceSamplingProbability(), 0.25);

        state.SetSettings("{}");
        EXPECT_EQ(tracing::GetTraceSamplingProbability(), std::nullopt);
    }>();
}

TEST_F(ComponentList, TelemetryInvalidSectionPreservesBothPolicies) {
    RunScenario<
        [](TelemetryTestState& state) {
            state.SetSettings(R"({
            "logging": {
                "default": {"min_log_level":"trace","log_sample_rate":0.125}
            },
            "tracing": {"trace_sample_rate":0.25}
        })");
            for (const auto value : {
                     R"({"logging":{"default":{"min_log_level":"trace","log_sample_rate":1.5}},"tracing":{"trace_sample_rate":0.5}})",
                     R"({"logging":{"default":{"min_log_level":"trace","log_sample_rate":0.5}},"tracing":{"trace_sample_rate":1.5}})"
                 })
            {
                EXPECT_ANY_THROW(state.SetSettings(value));
                EXPECT_EQ(logging::GetDefaultLogger().GetSamplingProbability(), 0.125);
                EXPECT_EQ(tracing::GetTraceSamplingProbability(), 0.25);
            }
        }>();
}

TEST_F(ComponentList, TelemetryIgnoresUnknownFieldsAndSections) {
    RunScenario<[](TelemetryTestState& state) {
        const auto access = state.loggers.GetLogger("access");
        for (const auto value :
             {R"({"log_sample_rate":0.5})",
              R"({"unknown":{}})",
              R"({"logging":{"access":{"min_log_level":"trace","log_sample_rate":1.0,"sample_rate":0.5}}})"})
        {
            EXPECT_NO_THROW(state.SetSettings(value));
            EXPECT_EQ(access->GetSamplingProbability(), std::nullopt);
            EXPECT_EQ(tracing::GetTraceSamplingProbability(), std::nullopt);
        }

        state.SetSettings(R"({
            "logging":{
                "access":{"min_log_level":"warning","log_sample_rate":0.125,"future_setting":true}
            },
            "tracing":{"trace_sample_rate":0.25,"future_setting":{"enabled":true}},
            "future_section":{"enabled":true}
        })");
        EXPECT_EQ(access->GetLevel(), logging::Level::kWarning);
        EXPECT_EQ(access->GetSamplingProbability(), 0.125);
        EXPECT_EQ(tracing::GetTraceSamplingProbability(), 0.25);
    }>();
}

USERVER_NAMESPACE_END
