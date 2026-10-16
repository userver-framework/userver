#include <userver/logging/telemetry_settings.hpp>

#include <limits>
#include <stdexcept>
#include <type_traits>

#include <gtest/gtest.h>

#include <userver/formats/json/serialize.hpp>

USERVER_NAMESPACE_BEGIN

TEST(TelemetrySettings, MergeKeepsStricterLimits) {
    const logging::TelemetrySettings settings{.min_log_level = logging::Level::kError, .log_sample_rate = 0.125};
    const logging::TelemetrySettings restrictions{.min_log_level = logging::Level::kWarning, .log_sample_rate = 0.5};
    const auto original_settings = settings;
    const auto original_restrictions = restrictions;
    const auto merged = settings.Merge(restrictions);
    const logging::TelemetrySettings expected{.min_log_level = logging::Level::kError, .log_sample_rate = 0.125};
    EXPECT_EQ(merged, expected);
    EXPECT_EQ(settings, original_settings);
    EXPECT_EQ(restrictions, original_restrictions);
}

TEST(TelemetrySettings, MergePreservesLimitsWithNeutralValues) {
    const logging::TelemetrySettings settings{.log_sample_rate = 0.125};
    const auto merged = settings.Merge({.min_log_level = logging::Level::kWarning});
    const logging::TelemetrySettings expected{.min_log_level = logging::Level::kWarning, .log_sample_rate = 0.125};
    EXPECT_EQ(merged, expected);
    EXPECT_EQ(settings.Merge({}), settings);
    const logging::TelemetrySettings empty;
    EXPECT_EQ(empty.Merge(merged), expected);
}

TEST(TelemetrySettings, ValidateRejectsInvalidProbabilities) {
    for (const double invalid :
         {-0.1, 1.1, std::numeric_limits<double>::infinity(), std::numeric_limits<double>::quiet_NaN()})
    {
        const logging::TelemetrySettings invalid_logs{.log_sample_rate = invalid};
        const logging::TracingSettings invalid_traces{.trace_sample_rate = invalid};
        EXPECT_THROW(invalid_logs.Validate(), std::runtime_error);
        EXPECT_THROW(invalid_traces.Validate(), std::runtime_error);
    }
}

TEST(TelemetrySettings, ParsesRequiredFields) {
    const auto settings =
        formats::json::FromString(R"({"min_log_level":"warning","log_sample_rate":0.25})")
            .As<logging::TelemetrySettings>();
    EXPECT_EQ(settings.min_log_level, logging::Level::kWarning);
    EXPECT_EQ(settings.log_sample_rate, 0.25);
    const auto tracing = formats::json::FromString(R"({"trace_sample_rate":0.5})").As<logging::TracingSettings>();
    EXPECT_EQ(tracing.trace_sample_rate, 0.5);
    EXPECT_EQ(formats::json::FromString("{}").As<logging::TelemetryConfig>(), logging::TelemetryConfig{});
}

TEST(TelemetrySettings, RejectsInvalidJsonValues) {
    for (const auto value :
         {"null",
          "[]",
          "{}",
          R"({"min_log_level":"info"})",
          R"({"log_sample_rate":0.5})",
          R"({"min_log_level":"invalid","log_sample_rate":0.5})",
          R"({"min_log_level":"trace","log_sample_rate":-0.1})",
          R"({"min_log_level":"trace","log_sample_rate":1.1})",
          R"({"min_log_level":"trace","log_sample_rate":"0.1"})",
          R"({"min_log_level":null,"log_sample_rate":0.5})",
          R"({"min_log_level":"trace","log_sample_rate":null})"})
    {
        EXPECT_ANY_THROW(formats::json::FromString(value).As<logging::TelemetrySettings>()) << value;
    }
}

TEST(TelemetryConfig, RejectsMisplacedSettings) {
    static_assert(!std::is_assignable_v<logging::TelemetrySettings&, logging::TracingSettings>);
    static_assert(!std::is_assignable_v<logging::TracingSettings&, logging::TelemetrySettings>);
    for (const auto value : {
             R"({"logging":{"access":{"trace_sample_rate":0.25}}})",
             R"({"tracing":{"min_log_level":"warning"}})",
             R"({"tracing":{"log_sample_rate":0.25}})",
         })
    {
        EXPECT_ANY_THROW(formats::json::FromString(value).As<logging::TelemetryConfig>()) << value;
    }
}

TEST(TracingSettings, MergeKeepsStricterRate) {
    const logging::TracingSettings settings{.trace_sample_rate = 0.25};
    const logging::TracingSettings restrictions{.trace_sample_rate = 0.125};
    EXPECT_EQ(settings.Merge(restrictions), restrictions);
    EXPECT_EQ(restrictions.Merge(settings), restrictions);
    EXPECT_EQ(settings.Merge({}), settings);
    EXPECT_EQ(logging::TracingSettings{}.Merge(settings), settings);
    EXPECT_EQ(settings.trace_sample_rate, 0.25);
    EXPECT_EQ(restrictions.trace_sample_rate, 0.125);
}

TEST(TracingSettings, RejectsInvalidJsonValues) {
    for (const auto value : {
             "null",
             "[]",
             "{}",
             R"({"trace_sample_rate":-0.1})",
             R"({"trace_sample_rate":1.1})",
             R"({"trace_sample_rate":"0.1"})",
             R"({"trace_sample_rate":null})",
         })
    {
        EXPECT_ANY_THROW(formats::json::FromString(value).As<logging::TracingSettings>()) << value;
    }
}

USERVER_NAMESPACE_END
