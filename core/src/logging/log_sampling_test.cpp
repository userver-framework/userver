#include <cmath>
#include <limits>
#include <optional>
#include <string>

#include <gmock/gmock.h>

#include <logging/dynamic_debug.hpp>
#include <logging/logging_test.hpp>
#include <userver/logging/impl/logger_base.hpp>
#include <userver/logging/log.hpp>
#include <userver/logging/logger.hpp>
#include <userver/tracing/span.hpp>
#include <userver/utest/death_tests.hpp>
#include <userver/utest/utest.hpp>
#include <userver/utils/assert.hpp>

using testing::HasSubstr;

USERVER_NAMESPACE_BEGIN

namespace {

class LogSampling : public LoggingTest {
protected:
    ~LogSampling() override {
        logging::GetDefaultLogger().SetSamplingProbability(std::nullopt);
        logging::RemoveAllDynamicDebugLog();
    }
};

class LogSamplingSpan : public LogSampling {};

class LogSamplingDeathTest : public LogSampling {
protected:
    void SetUp() override {
        LogSampling::SetUp();
        if constexpr (!utils::impl::kEnableAssert) {
            GTEST_SKIP() << "Sampling preconditions are checked by UASSERT in debug builds";
        }
    }
};

}  // namespace

TEST_F(LogSampling, ZeroProbabilityDropsBelowWarning) {
    logging::GetDefaultLogger().SetSamplingProbability(0.0);

    LOG_INFO() << "info_message";
    logging::LogFlush();
    EXPECT_EQ(GetRecordsCount(), 0);

    LOG_WARNING() << "warning_message";
    LOG_ERROR() << "error_message";
    logging::LogFlush();
    EXPECT_EQ(GetRecordsCount(), 2);
}

TEST_F(LogSampling, FullProbabilityWritesEverything) {
    logging::GetDefaultLogger().SetSamplingProbability(1.0);

    LOG_INFO() << "info_message";
    logging::LogFlush();
    EXPECT_EQ(GetRecordsCount(), 1);
}

TEST_F(LogSampling, RawZeroProbabilityPreservesWarningAndHigher) {
    const auto logger = GetStreamLogger();
    logger->SetSamplingProbability(0.0);

    logging::impl::LogRaw(*logger, logging::Level::kTrace, "trace_message");
    logging::impl::LogRaw(*logger, logging::Level::kDebug, "debug_message");
    logging::impl::LogRaw(*logger, logging::Level::kInfo, "info_message");
    logging::impl::LogRaw(*logger, logging::Level::kWarning, "warning_message");
    logging::impl::LogRaw(*logger, logging::Level::kError, "error_message");
    logging::impl::LogRaw(*logger, logging::Level::kCritical, "critical_message");
    logging::LogFlush(*logger);

    EXPECT_EQ(GetStreamString(), "warning_message\nerror_message\ncritical_message\n");
}

TEST_F(LogSampling, RawFullAndDisabledSamplingPreservePayload) {
    const auto logger = GetStreamLogger();
    logger->SetSamplingProbability(1.0);
    logging::impl::LogRaw(*logger, logging::Level::kInfo, R"({"event":"full_sampling"})");
    logger->SetSamplingProbability(std::nullopt);
    logging::impl::LogRaw(*logger, logging::Level::kInfo, R"({"event":"disabled_sampling"})");
    logging::LogFlush(*logger);

    EXPECT_EQ(GetStreamString(), "{\"event\":\"full_sampling\"}\n{\"event\":\"disabled_sampling\"}\n");
}

TEST_F(LogSampling, RawUsesTargetLoggerSampling) {
    const auto default_logger = GetStreamLogger();
    const auto custom = MakeNamedStreamLogger("custom", logging::Format::kRaw);
    default_logger->SetSamplingProbability(0.0);
    custom.logger->SetSamplingProbability(1.0);

    logging::impl::LogRaw(*default_logger, logging::Level::kInfo, "default_dropped");
    logging::impl::LogRaw(*custom.logger, logging::Level::kInfo, "custom_kept");
    logging::LogFlush(*default_logger);
    logging::LogFlush(*custom.logger);
    EXPECT_TRUE(GetStreamString().empty());
    EXPECT_EQ(custom.stream.str(), "custom_kept\n");

    default_logger->SetSamplingProbability(1.0);
    custom.logger->SetSamplingProbability(0.0);
    logging::impl::LogRaw(*default_logger, logging::Level::kInfo, "default_kept");
    logging::impl::LogRaw(*custom.logger, logging::Level::kInfo, "custom_dropped");
    logging::LogFlush(*default_logger);
    logging::LogFlush(*custom.logger);
    EXPECT_EQ(GetStreamString(), "default_kept\n");
    EXPECT_EQ(custom.stream.str(), "custom_kept\n");
}

TEST_F(LogSampling, RawSamplingPreservesLevelBypass) {
    const auto logger = GetStreamLogger();
    logger->SetLevel(logging::Level::kNone);
    logger->SetSamplingProbability(1.0);

    logging::impl::LogRaw(*logger, logging::Level::kInfo, "raw_message");
    logging::LogFlush(*logger);

    EXPECT_EQ(GetStreamString(), "raw_message\n");
}

TEST_F(LogSampling, LoggersHaveIndependentSampling) {
    const auto custom = MakeNamedStreamLogger("custom", logging::Format::kTskv);
    logging::SetLoggerLevel(*custom.logger, logging::Level::kTrace);
    logging::GetDefaultLogger().SetSamplingProbability(0.0);
    custom.logger->SetSamplingProbability(1.0);

    LOG_INFO() << "default_dropped";
    LOG_INFO_TO(custom.logger) << "custom_kept";
    logging::LogFlush();
    logging::LogFlush(*custom.logger);
    EXPECT_EQ(GetRecordsCount(), 0);
    EXPECT_THAT(custom.stream.str(), HasSubstr("custom_kept"));

    logging::GetDefaultLogger().SetSamplingProbability(1.0);
    custom.logger->SetSamplingProbability(0.0);
    LOG_INFO() << "default_kept";
    LOG_INFO_TO(custom.logger) << "custom_dropped";
    LOG_WARNING_TO(custom.logger) << "custom_warning";
    logging::LogFlush();
    logging::LogFlush(*custom.logger);
    EXPECT_THAT(GetStreamString(), HasSubstr("default_kept"));
    EXPECT_THAT(custom.stream.str(), testing::Not(HasSubstr("custom_dropped")));
    EXPECT_THAT(custom.stream.str(), HasSubstr("custom_warning"));
    EXPECT_EQ(logging::GetDefaultLogger().GetSamplingProbability(), 1.0);
    EXPECT_EQ(custom.logger->GetSamplingProbability(), 0.0);
}

TEST_F(LogSampling, ForceEnabledCustomLoggerBypassesSampling) {
    const auto custom = MakeNamedStreamLogger("custom", logging::Format::kTskv);
    logging::SetLoggerLevel(*custom.logger, logging::Level::kTrace);
    custom.logger->SetSamplingProbability(0.0);
    logging::AddDynamicDebugLog(std::string{USERVER_FILEPATH}, logging::kAnyLine);
    LOG_INFO_TO(custom.logger) << "forced_custom";
    logging::LogFlush(*custom.logger);
    EXPECT_THAT(custom.stream.str(), HasSubstr("forced_custom"));
}

TEST_F(LogSampling, DisabledSamplingWritesEverything) {
    logging::GetDefaultLogger().SetSamplingProbability(std::nullopt);

    LOG_INFO() << "info_message";
    logging::LogFlush();
    EXPECT_EQ(GetRecordsCount(), 1);
}

TEST_F(LogSampling, ProbabilityIsClamped) {
    logging::GetDefaultLogger().SetSamplingProbability(1.5);
    EXPECT_EQ(logging::GetDefaultLogger().GetSamplingProbability(), 1.0);

    logging::GetDefaultLogger().SetSamplingProbability(-0.5);
    EXPECT_EQ(logging::GetDefaultLogger().GetSamplingProbability(), 0.0);

    logging::GetDefaultLogger().SetSamplingProbability(std::nullopt);
    EXPECT_EQ(logging::GetDefaultLogger().GetSamplingProbability(), std::nullopt);
}

TEST_F(LogSampling, ProbabilityIsQuantizedDown) {
    constexpr double kProbabilityStep = 0x1p-32;
    constexpr double kProbability = 0.1;
    logging::GetDefaultLogger().SetSamplingProbability(kProbability);
    const auto probability = logging::GetDefaultLogger().GetSamplingProbability();
    ASSERT_TRUE(probability.has_value());
    EXPECT_EQ(probability.value(), std::floor(kProbability / kProbabilityStep) * kProbabilityStep);
    EXPECT_LE(probability.value(), kProbability);
    EXPECT_LT(kProbability - probability.value(), kProbabilityStep);

    logging::GetDefaultLogger().SetSamplingProbability(kProbabilityStep);
    EXPECT_EQ(logging::GetDefaultLogger().GetSamplingProbability(), kProbabilityStep);

    logging::GetDefaultLogger().SetSamplingProbability(1.0 - kProbabilityStep);
    EXPECT_EQ(logging::GetDefaultLogger().GetSamplingProbability(), 1.0 - kProbabilityStep);
}

TEST_F(LogSampling, ProbabilityBelowResolutionDropsLogs) {
    constexpr double kProbabilityBelowResolution = 0x1p-33;
    logging::GetDefaultLogger().SetSamplingProbability(kProbabilityBelowResolution);
    EXPECT_EQ(logging::GetDefaultLogger().GetSamplingProbability(), 0.0);

    LOG_INFO() << "info_message";
    logging::LogFlush();
    EXPECT_EQ(GetRecordsCount(), 0);
}

TEST_F(LogSamplingDeathTest, NanProbabilityViolatesPrecondition) {
    UEXPECT_DEATH(
        logging::GetDefaultLogger().SetSamplingProbability(std::numeric_limits<double>::quiet_NaN()),
        "std::isfinite"
    );
}

TEST_F(LogSamplingDeathTest, InfiniteProbabilityViolatesPrecondition) {
    UEXPECT_DEATH(
        logging::GetDefaultLogger().SetSamplingProbability(std::numeric_limits<double>::infinity()),
        "std::isfinite"
    );
    UEXPECT_DEATH(
        logging::GetDefaultLogger().SetSamplingProbability(-std::numeric_limits<double>::infinity()),
        "std::isfinite"
    );
}

TEST_F(LogSampling, ForceEnabledLocationBypassesSampling) {
    logging::GetDefaultLogger().SetSamplingProbability(0.0);
    logging::AddDynamicDebugLog(std::string{USERVER_FILEPATH}, logging::kAnyLine);

    LOG_INFO() << "forced_message";
    logging::LogFlush();
    EXPECT_THAT(GetStreamString(), HasSubstr("forced_message"));
}

UTEST_F(LogSamplingSpan, DoesNotAffectSpans) {
    logging::GetDefaultLogger().SetSamplingProbability(0.0);

    {
        const tracing::Span span("sampled_span");
    }
    logging::LogFlush();
    EXPECT_THAT(GetStreamString(), HasSubstr("stopwatch_name=sampled_span"));
}

USERVER_NAMESPACE_END
