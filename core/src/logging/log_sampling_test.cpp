#include <cmath>
#include <limits>
#include <optional>
#include <string>

#include <gmock/gmock.h>

#include <logging/dynamic_debug.hpp>
#include <logging/logging_test.hpp>
#include <userver/logging/log.hpp>
#include <userver/tracing/span.hpp>
#include <userver/utest/utest.hpp>

using testing::HasSubstr;

USERVER_NAMESPACE_BEGIN

namespace {

class LogSampling : public LoggingTest {
protected:
    ~LogSampling() override {
        logging::SetLogSamplingProbability(std::nullopt);
        logging::RemoveAllDynamicDebugLog();
    }
};

class LogSamplingSpan : public LogSampling {};

}  // namespace

TEST_F(LogSampling, ZeroProbabilityDropsBelowWarning) {
    logging::SetLogSamplingProbability(0.0);

    LOG_INFO() << "info_message";
    logging::LogFlush();
    EXPECT_EQ(GetRecordsCount(), 0);

    LOG_WARNING() << "warning_message";
    LOG_ERROR() << "error_message";
    logging::LogFlush();
    EXPECT_EQ(GetRecordsCount(), 2);
}

TEST_F(LogSampling, FullProbabilityWritesEverything) {
    logging::SetLogSamplingProbability(1.0);

    LOG_INFO() << "info_message";
    logging::LogFlush();
    EXPECT_EQ(GetRecordsCount(), 1);
}

TEST_F(LogSampling, DisabledSamplingWritesEverything) {
    logging::SetLogSamplingProbability(std::nullopt);

    LOG_INFO() << "info_message";
    logging::LogFlush();
    EXPECT_EQ(GetRecordsCount(), 1);
}

TEST_F(LogSampling, ProbabilityIsClamped) {
    logging::SetLogSamplingProbability(1.5);
    EXPECT_EQ(logging::GetLogSamplingProbability(), 1.0);

    logging::SetLogSamplingProbability(-0.5);
    EXPECT_EQ(logging::GetLogSamplingProbability(), 0.0);

    logging::SetLogSamplingProbability(std::nullopt);
    EXPECT_EQ(logging::GetLogSamplingProbability(), std::nullopt);
}

TEST_F(LogSampling, ProbabilityIsQuantizedDown) {
    constexpr double kProbabilityStep = 0x1p-32;
    constexpr double kProbability = 0.1;
    logging::SetLogSamplingProbability(kProbability);
    const auto probability = logging::GetLogSamplingProbability();
    ASSERT_TRUE(probability.has_value());
    EXPECT_EQ(probability.value(), std::floor(kProbability / kProbabilityStep) * kProbabilityStep);
    EXPECT_LE(probability.value(), kProbability);
    EXPECT_LT(kProbability - probability.value(), kProbabilityStep);

    logging::SetLogSamplingProbability(kProbabilityStep);
    EXPECT_EQ(logging::GetLogSamplingProbability(), kProbabilityStep);

    logging::SetLogSamplingProbability(1.0 - kProbabilityStep);
    EXPECT_EQ(logging::GetLogSamplingProbability(), 1.0 - kProbabilityStep);
}

TEST_F(LogSampling, ProbabilityBelowResolutionDropsLogs) {
    constexpr double kProbabilityBelowResolution = 0x1p-33;
    logging::SetLogSamplingProbability(kProbabilityBelowResolution);
    EXPECT_EQ(logging::GetLogSamplingProbability(), 0.0);

    LOG_INFO() << "info_message";
    logging::LogFlush();
    EXPECT_EQ(GetRecordsCount(), 0);
}

TEST_F(LogSampling, NanDisablesSampling) {
    logging::SetLogSamplingProbability(0.0);
    logging::SetLogSamplingProbability(std::numeric_limits<double>::quiet_NaN());
    EXPECT_EQ(logging::GetLogSamplingProbability(), std::nullopt);

    LOG_INFO() << "info_message";
    logging::LogFlush();
    EXPECT_EQ(GetRecordsCount(), 1);
}

TEST_F(LogSampling, InfiniteProbabilityIsClamped) {
    logging::SetLogSamplingProbability(std::numeric_limits<double>::infinity());
    EXPECT_EQ(logging::GetLogSamplingProbability(), 1.0);

    logging::SetLogSamplingProbability(-std::numeric_limits<double>::infinity());
    EXPECT_EQ(logging::GetLogSamplingProbability(), 0.0);
}

TEST_F(LogSampling, ForceEnabledLocationBypassesSampling) {
    logging::SetLogSamplingProbability(0.0);
    logging::AddDynamicDebugLog(std::string{USERVER_FILEPATH}, logging::kAnyLine);

    LOG_INFO() << "forced_message";
    logging::LogFlush();
    EXPECT_THAT(GetStreamString(), HasSubstr("forced_message"));
}

UTEST_F(LogSamplingSpan, DoesNotAffectSpans) {
    logging::SetLogSamplingProbability(0.0);

    {
        const tracing::Span span("sampled_span");
    }
    logging::LogFlush();
    EXPECT_THAT(GetStreamString(), HasSubstr("stopwatch_name=sampled_span"));
}

USERVER_NAMESPACE_END
