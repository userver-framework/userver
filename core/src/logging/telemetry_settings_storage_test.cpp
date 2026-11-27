#include <logging/telemetry_settings_storage.hpp>

#include <optional>
#include <stdexcept>

#include <userver/formats/json/serialize.hpp>
#include <userver/logging/impl/logger_base.hpp>
#include <userver/logging/null_logger.hpp>
#include <userver/utest/utest.hpp>

USERVER_NAMESPACE_BEGIN

namespace {

using Settings = utils::DefaultDict<logging::TelemetrySettings>;

}

UTEST(TelemetrySettingsStorage, ExplicitEntryReplacesFallback) {
    const auto ordinary = logging::impl::MakeNoopLoggerForTests();
    const auto access = logging::impl::MakeNoopLoggerForTests();
    logging::impl::TelemetrySettingsStorage storage;
    storage.RegisterLogger("default", ordinary);
    storage.RegisterLogger("access", access);
    storage.Apply(
        formats::json::FromString(R"({
            "__default__":{"min_log_level":"warning","log_sample_rate":0.125},
            "access":{"min_log_level":"trace","log_sample_rate":0.5}
        })")
            .As<Settings>(),
        {}
    );
    EXPECT_EQ(ordinary->GetLevel(), logging::Level::kWarning);
    EXPECT_EQ(ordinary->GetSamplingProbability(), 0.125);
    EXPECT_EQ(access->GetLevel(), logging::Level::kInfo);
    EXPECT_EQ(access->GetSamplingProbability(), 0.5);
}

UTEST(TelemetrySettingsStorage, NeutralEntryOptsOutOfFallback) {
    const auto logger = logging::impl::MakeNoopLoggerForTests();
    logging::impl::TelemetrySettingsStorage storage;
    storage.RegisterLogger("access", logger);
    storage
        .Apply(
            formats::json::FromString(R"({"__default__":{"min_log_level":"warning","log_sample_rate":0},"access":{"min_log_level":"trace","log_sample_rate":1.0}})"
            )
                .As<Settings>(),
            {}
        );
    EXPECT_EQ(logger->GetLevel(), logging::Level::kInfo);
    EXPECT_EQ(logger->GetSamplingProbability(), std::nullopt);
}

UTEST(TelemetrySettingsStorage, RemovingEntryUsesFallbackThenRestoresInitialLevel) {
    const auto logger = logging::impl::MakeNoopLoggerForTests();
    logging::impl::TelemetrySettingsStorage storage;
    storage.RegisterLogger("access", logger);
    storage.Apply(
        formats::json::FromString(R"({"access":{"min_log_level":"error","log_sample_rate":0}})").As<Settings>(),
        {}
    );
    EXPECT_EQ(logger->GetLevel(), logging::Level::kError);
    storage.Apply(
        formats::json::FromString(R"({"__default__":{"min_log_level":"trace","log_sample_rate":0.5}})").As<Settings>(),
        {}
    );
    EXPECT_EQ(logger->GetLevel(), logging::Level::kInfo);
    EXPECT_EQ(logger->GetSamplingProbability(), 0.5);
    storage.Apply({}, {});
    EXPECT_EQ(logger->GetLevel(), logging::Level::kInfo);
    EXPECT_EQ(logger->GetSamplingProbability(), std::nullopt);
}

UTEST(TelemetrySettingsStorage, LateLoggerReceivesCurrentSettingsAndRestrictions) {
    const auto logger = logging::impl::MakeNoopLoggerForTests();
    logging::impl::TelemetrySettingsStorage storage;
    storage.Apply(
        formats::json::FromString(R"({"access":{"min_log_level":"trace","log_sample_rate":0.125}})").As<Settings>(),
        {{"__default__", {.min_log_level = logging::Level::kWarning, .log_sample_rate = 0.5}}}
    );
    storage.RegisterLogger("access", logger);
    EXPECT_EQ(logger->GetLevel(), logging::Level::kWarning);
    EXPECT_EQ(logger->GetSamplingProbability(), 0.125);
}

UTEST(TelemetrySettingsStorage, RestoreReturnsInitialLevelAndSampling) {
    const auto logger = logging::impl::MakeNoopLoggerForTests();
    logger->SetLevel(logging::Level::kInfo);
    logger->SetSamplingProbability(0.5);
    logging::impl::TelemetrySettingsStorage storage;
    storage.RegisterLogger("access", logger);
    storage.Apply({}, {{"__default__", {.min_log_level = logging::Level::kWarning, .log_sample_rate = 0}}});
    logger->SetLevel(logging::Level::kError);
    storage.Restore();
    EXPECT_EQ(logger->GetLevel(), logging::Level::kInfo);
    EXPECT_EQ(logger->GetSamplingProbability(), 0.5);
}

UTEST(TelemetrySettingsStorage, PolicyNeverLowersInitialLevel) {
    const auto logger = logging::impl::MakeNoopLoggerForTests();
    logger->SetLevel(logging::Level::kError);
    logging::impl::TelemetrySettingsStorage storage;
    storage.RegisterLogger("access", logger);
    storage.Apply({}, {{"__default__", {.min_log_level = logging::Level::kWarning}}});
    EXPECT_EQ(logger->GetLevel(), logging::Level::kError);
    EXPECT_FALSE(logger->ShouldLog(logging::Level::kWarning));
    storage.Apply({}, {});
    EXPECT_EQ(logger->GetLevel(), logging::Level::kError);
}

UTEST(TelemetrySettingsStorage, InvalidUpdatePreservesEveryLogger) {
    const auto logger = logging::impl::MakeNoopLoggerForTests();
    logging::impl::TelemetrySettingsStorage storage;
    storage.RegisterLogger("access", logger);
    storage.Apply({}, {{"__default__", {.log_sample_rate = 0.25}}});
    const Settings invalid{{"access", {.log_sample_rate = 0.5}}, {"unused", {.log_sample_rate = 1.5}}};
    EXPECT_THROW(storage.Apply(invalid, {}), std::runtime_error);
    EXPECT_EQ(logger->GetSamplingProbability(), 0.25);
    EXPECT_THROW(storage.Apply({}, invalid), std::runtime_error);
    EXPECT_EQ(logger->GetSamplingProbability(), 0.25);
    EXPECT_THROW(storage.Apply({}, {{"__default__", {.log_sample_rate = -0.5}}}), std::runtime_error);
    EXPECT_EQ(logger->GetSamplingProbability(), 0.25);
}

UTEST(TelemetrySettingsStorage, PoliciesResolveFallbackIndependently) {
    const auto access = logging::impl::MakeNoopLoggerForTests();
    const auto audit = logging::impl::MakeNoopLoggerForTests();
    logging::impl::TelemetrySettingsStorage storage;
    storage.RegisterLogger("access", access);
    storage.RegisterLogger("audit", audit);
    storage.Apply(
        formats::json::FromString(R"({
            "__default__":{"min_log_level":"warning","log_sample_rate":0.5},
            "access":{"min_log_level":"trace","log_sample_rate":0.125}
        })")
            .As<Settings>(),
        formats::json::FromString(R"({
            "__default__":{"min_log_level":"error","log_sample_rate":0.25},
            "audit":{"min_log_level":"trace","log_sample_rate":1.0}
        })")
            .As<Settings>()
    );
    EXPECT_EQ(access->GetLevel(), logging::Level::kError);
    EXPECT_EQ(access->GetSamplingProbability(), 0.125);
    EXPECT_EQ(audit->GetLevel(), logging::Level::kWarning);
    EXPECT_EQ(audit->GetSamplingProbability(), 0.5);
}

USERVER_NAMESPACE_END
