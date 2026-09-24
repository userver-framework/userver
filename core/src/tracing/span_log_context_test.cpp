#include <userver/tracing/span_log_context.hpp>

#include <string_view>
#include <utility>

#include <userver/engine/async.hpp>
#include <userver/logging/log.hpp>
#include <userver/tracing/span.hpp>
#include <userver/tracing/span_builder.hpp>
#include <userver/utest/log_capture_fixture.hpp>
#include <userver/utest/utest.hpp>

USERVER_NAMESPACE_BEGIN

namespace {

constexpr std::string_view kTraceId = "0123456789abcdef0123456789abcdef";
constexpr std::string_view kSpanId = "0123456789abcdef";
constexpr std::string_view kLink = "fedcba9876543210fedcba9876543210";

tracing::Span MakeSpan(bool sampled = true) {
    tracing::SpanBuilder builder{"context-test"};
    builder.SetTraceId(kTraceId);
    builder.SetSpanId(kSpanId);
    builder.SetLink(kLink);
    builder.SetParentSpanId("");
    builder.SetSampled(sampled);
    return std::move(builder).BuildDetachedFromCoroStack();
}

tracing::SpanLogContext MakeContext(const tracing::Span& span) {
    tracing::SpanLogContext context;
    context.SetFromSpan(span);
    return context;
}

class SpanLogContext : public utest::LogCaptureFixture<> {
protected:
    utest::LogRecord Capture(const tracing::SpanLogContext& context) {
        GetLogCapture().Clear();
        LOG_INFO() << context.GetLogExtra();
        return utest::GetSingleLog(GetLogCapture().GetAll());
    }
};

UTEST_F(SpanLogContext, Empty) {
    const tracing::SpanLogContext context;
    ASSERT_EQ(context, tracing::SpanLogContext{});
    EXPECT_EQ(testing::PrintToString(context), "SpanLogContext{}");
    EXPECT_TRUE(context.IsEmpty());
    EXPECT_FALSE(context.IsSampled());

    const auto log = Capture(context);
    for (const auto tag : {"trace_id", "span_id", "link", "trace_sampled"}) {
        EXPECT_FALSE(log.GetTagOptional(tag).has_value());
    }
}

UTEST_F(SpanLogContext, SurvivesSpanAndTask) {
    const auto expected_context = MakeContext(MakeSpan());
    const auto context =
        engine::AsyncNoTracing([] {
            const auto span = MakeSpan();
            return MakeContext(span);
        }).Get();

    ASSERT_EQ(context, expected_context);
    EXPECT_FALSE(context.IsEmpty());
    EXPECT_TRUE(context.IsSampled());
    EXPECT_EQ(
        testing::PrintToString(context),
        "SpanLogContext{trace_id=0123456789abcdef0123456789abcdef, span_id=0123456789abcdef, "
        "link=fedcba9876543210fedcba9876543210, trace_sampled=true}"
    );

    const auto log = Capture(context);
    EXPECT_EQ(log.GetTag("trace_id"), kTraceId);
    EXPECT_EQ(log.GetTag("span_id"), kSpanId);
    EXPECT_EQ(log.GetTag("link"), kLink);
    EXPECT_EQ(log.GetTag("trace_sampled"), "1");
}

UTEST_F(SpanLogContext, UnsampledSpan) {
    auto span = MakeSpan();
    auto context = MakeContext(span);
    const auto sampled_context = context;
    const auto expected_context = MakeContext(MakeSpan(false));

    span.SetSampled(false);
    context.SetFromSpan(span);

    ASSERT_EQ(context, expected_context);
    EXPECT_NE(context, sampled_context);
    EXPECT_FALSE(context.IsEmpty());
    EXPECT_FALSE(context.IsSampled());
    EXPECT_EQ(Capture(context).GetTag("trace_sampled"), "0");
}

UTEST_F(SpanLogContext, ReplacingWithUnloggableSpanClearsIds) {
    auto span = MakeSpan();
    auto context = MakeContext(span);
    const auto original_context = context;

    span.SetLogLevel(logging::Level::kNone);
    const auto expected_context = MakeContext(span);
    context.SetFromSpan(span);

    ASSERT_EQ(context, expected_context);
    EXPECT_NE(context, original_context);
    EXPECT_NE(context, tracing::SpanLogContext{});
    EXPECT_FALSE(context.IsEmpty());
    const auto log = Capture(context);
    EXPECT_FALSE(log.GetTagOptional("trace_id").has_value());
    EXPECT_FALSE(log.GetTagOptional("span_id").has_value());
    EXPECT_FALSE(log.GetTagOptional("link").has_value());
}

UTEST_F(SpanLogContext, UsesParentIdForUnloggableSpan) {
    constexpr std::string_view kParentId = "fedcba9876543210";
    tracing::SpanBuilder builder{"context-child"};
    builder.SetParentSpanId(kParentId);
    auto span = std::move(builder).BuildDetachedFromCoroStack();
    span.SetLogLevel(logging::Level::kNone);
    const auto context = MakeContext(span);

    tracing::SpanBuilder expected_builder{"context-parent"};
    expected_builder.SetTraceId(span.GetTraceId());
    expected_builder.SetSpanId(kParentId);
    expected_builder.SetLink(span.GetLink());
    const auto expected_span = std::move(expected_builder).BuildDetachedFromCoroStack();
    const auto expected_context = MakeContext(expected_span);

    ASSERT_EQ(context, expected_context);
    EXPECT_EQ(Capture(context).GetTag("span_id"), kParentId);
}

}  // namespace

USERVER_NAMESPACE_END
