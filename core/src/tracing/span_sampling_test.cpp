#include <cmath>
#include <limits>
#include <string_view>

#include <gmock/gmock.h>

#include <logging/logging_test.hpp>
#include <userver/tracing/span.hpp>
#include <userver/tracing/span_builder.hpp>
#include <userver/tracing/tracer.hpp>
#include <userver/utest/utest.hpp>

using testing::HasSubstr;
using testing::Not;

USERVER_NAMESPACE_BEGIN

namespace {

// the lowest 8 bytes of the trace id define the sampling decision
constexpr std::string_view kAlwaysSampledTraceId = "aaaaaaaaaaaaaaaa0000000000000000";
constexpr std::string_view kNeverSampledTraceId = "aaaaaaaaaaaaaaaaffffffffffffffff";

class SpanSampling : public LoggingTest {};

}  // namespace

TEST(SpanSamplingConfig, ProbabilityIsClamped) {
    const tracing::TracingStateGuard guard;

    tracing::SetTraceSamplingProbability(1.5);
    EXPECT_EQ(tracing::GetTraceSamplingProbability(), 1.0);

    tracing::SetTraceSamplingProbability(-0.5);
    EXPECT_EQ(tracing::GetTraceSamplingProbability(), 0.0);

    tracing::SetTraceSamplingProbability(std::nullopt);
    EXPECT_EQ(tracing::GetTraceSamplingProbability(), std::nullopt);
}

TEST(SpanSamplingConfig, NonHexTraceIdFallsBackToHash) {
    const tracing::TracingStateGuard guard;

    tracing::SetTraceSamplingProbability(1.0);
    EXPECT_TRUE(tracing::impl::ShouldSampleTrace("not-a-hex-trace-id"));

    tracing::SetTraceSamplingProbability(0.0);
    EXPECT_FALSE(tracing::impl::ShouldSampleTrace("not-a-hex-trace-id"));
}

TEST(SpanSamplingConfig, IntegerThresholdHasExactBoundary) {
    const tracing::TracingStateGuard guard;
    constexpr std::string_view kBelowThreshold = "aaaaaaaaaaaaaaaa7fffffffffffffff";
    constexpr std::string_view kAtThreshold = "aaaaaaaaaaaaaaaa8000000000000000";
    tracing::SetTraceSamplingProbability(0.5);
    EXPECT_TRUE(tracing::impl::ShouldSampleTrace(kBelowThreshold));
    EXPECT_FALSE(tracing::impl::ShouldSampleTrace(kAtThreshold));
}

TEST(SpanSamplingConfig, ProbabilityBelowResolutionIsRoundedDown) {
    const tracing::TracingStateGuard guard;
    constexpr double kProbabilityStep = 0x1p-64;
    tracing::SetTraceSamplingProbability(kProbabilityStep / 2);
    EXPECT_EQ(tracing::GetTraceSamplingProbability(), 0.0);
    EXPECT_FALSE(tracing::impl::ShouldSampleTrace(kAlwaysSampledTraceId));

    tracing::SetTraceSamplingProbability(kProbabilityStep);
    EXPECT_EQ(tracing::GetTraceSamplingProbability(), kProbabilityStep);
    EXPECT_TRUE(tracing::impl::ShouldSampleTrace(kAlwaysSampledTraceId));
    constexpr std::string_view kAtThreshold = "aaaaaaaaaaaaaaaa0000000000000001";
    EXPECT_FALSE(tracing::impl::ShouldSampleTrace(kAtThreshold));
}

TEST(SpanSamplingConfig, NearFullProbabilityDoesNotCollideWithSentinels) {
    const tracing::TracingStateGuard guard;
    const auto probability = std::nextafter(1.0, 0.0);
    tracing::SetTraceSamplingProbability(probability);
    EXPECT_EQ(tracing::GetTraceSamplingProbability(), probability);
    EXPECT_FALSE(tracing::impl::ShouldSampleTrace(kNeverSampledTraceId));

    tracing::SetTraceSamplingProbability(1.0);
    EXPECT_EQ(tracing::GetTraceSamplingProbability(), 1.0);
    EXPECT_TRUE(tracing::impl::ShouldSampleTrace(kNeverSampledTraceId));

    tracing::SetTraceSamplingProbability(std::nullopt);
    EXPECT_EQ(tracing::GetTraceSamplingProbability(), std::nullopt);
    EXPECT_TRUE(tracing::impl::ShouldSampleTrace(kNeverSampledTraceId));
}

TEST(SpanSamplingConfig, NonFiniteProbabilitiesAreHandled) {
    const tracing::TracingStateGuard guard;
    tracing::SetTraceSamplingProbability(std::numeric_limits<double>::infinity());
    EXPECT_EQ(tracing::GetTraceSamplingProbability(), 1.0);
    EXPECT_TRUE(tracing::impl::ShouldSampleTrace(kNeverSampledTraceId));

    tracing::SetTraceSamplingProbability(-std::numeric_limits<double>::infinity());
    EXPECT_EQ(tracing::GetTraceSamplingProbability(), 0.0);
    EXPECT_FALSE(tracing::impl::ShouldSampleTrace(kAlwaysSampledTraceId));

    tracing::SetTraceSamplingProbability(std::numeric_limits<double>::quiet_NaN());
    EXPECT_EQ(tracing::GetTraceSamplingProbability(), std::nullopt);
    EXPECT_TRUE(tracing::impl::ShouldSampleTrace(kNeverSampledTraceId));
}

TEST(SpanSamplingConfig, GuardRestoresSamplingProbability) {
    const tracing::TracingStateGuard outer_guard;
    tracing::SetTraceSamplingProbability(0.5);
    {
        const tracing::TracingStateGuard inner_guard;
        EXPECT_EQ(tracing::GetTraceSamplingProbability(), std::nullopt);
        tracing::SetTraceSamplingProbability(0.0);
    }
    EXPECT_EQ(tracing::GetTraceSamplingProbability(), 0.5);
}

UTEST_F(SpanSampling, ZeroProbabilityDropsRootSpan) {
    tracing::SetTraceSamplingProbability(0.0);
    {
        const auto span = tracing::Span::MakeRootSpan("root_span");
        EXPECT_FALSE(span.IsSampled());
    }
    logging::LogFlush();
    EXPECT_THAT(GetStreamString(), Not(HasSubstr("stopwatch_name=root_span")));
}

UTEST_F(SpanSampling, FullProbabilitySamplesRootSpan) {
    tracing::SetTraceSamplingProbability(1.0);
    {
        const auto span = tracing::Span::MakeRootSpan("root_span");
        EXPECT_TRUE(span.IsSampled());
    }
    logging::LogFlush();
    EXPECT_THAT(GetStreamString(), HasSubstr("stopwatch_name=root_span"));
}

UTEST_F(SpanSampling, DecisionIsDeterministicByTraceId) {
    tracing::SetTraceSamplingProbability(0.5);

    const auto sampled = tracing::Span::MakeSpan("sampled_span", kAlwaysSampledTraceId, {});
    EXPECT_TRUE(sampled.IsSampled());

    const auto not_sampled = tracing::Span::MakeSpan("not_sampled_span", kNeverSampledTraceId, {});
    EXPECT_FALSE(not_sampled.IsSampled());
}

UTEST_F(SpanSampling, ChildInheritsPositiveDecision) {
    const auto parent = tracing::Span::MakeRootSpan("parent_span");
    EXPECT_TRUE(parent.IsSampled());

    // the child keeps the parent's decision, the probability is not rechecked
    tracing::SetTraceSamplingProbability(0.0);
    const auto child = parent.CreateChild("child_span");
    EXPECT_TRUE(child.IsSampled());
}

UTEST_F(SpanSampling, ChildInheritsNegativeDecision) {
    tracing::SetTraceSamplingProbability(0.0);
    const auto parent = tracing::Span::MakeRootSpan("parent_span");
    EXPECT_FALSE(parent.IsSampled());

    tracing::SetTraceSamplingProbability(std::nullopt);
    const auto child = parent.CreateChild("child_span");
    EXPECT_FALSE(child.IsSampled());
}

UTEST_F(SpanSampling, ExplicitSetSampledOverridesDecision) {
    tracing::SetTraceSamplingProbability(0.0);

    tracing::SpanBuilder builder("builder_span");
    builder.SetTraceId(kNeverSampledTraceId);
    builder.SetSampled(true);
    const auto span = std::move(builder).Build();
    EXPECT_TRUE(span.IsSampled());
}

UTEST_F(SpanSampling, DecisionRecomputedForIncomingTraceId) {
    tracing::SetTraceSamplingProbability(0.5);

    tracing::SpanBuilder builder("builder_span");
    builder.SetTraceId(kAlwaysSampledTraceId);
    const auto span = std::move(builder).Build();
    EXPECT_TRUE(span.IsSampled());
}

USERVER_NAMESPACE_END
